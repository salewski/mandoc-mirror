/*	$Id$ */
/*
 * Copyright (c) 2009 Kristaps Dzonsons <kristaps@kth.se>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mandoc.h"
#include "out.h"
#include "term.h"

/* FIXME: `n' modifier doesn't always do the right thing. */
/* FIXME: `n' modifier doesn't use the cell-spacing buffer. */

static	inline void	 tbl_char(struct termp *, char, int);
static	void		 tbl_hframe(struct termp *, 
				const struct tbl_span *);
static	void		 tbl_data_number(struct termp *, 
				const struct tbl *, 
				const struct tbl_dat *, int);
static	void		 tbl_data_literal(struct termp *, 
				const struct tbl_dat *, int);
static	void		 tbl_data_spanner(struct termp *, 
				const struct tbl_dat *, int);
static	void		 tbl_data(struct termp *, const struct tbl *,
				const struct tbl_dat *, int);
static	void		 tbl_spanner(struct termp *, 
				const struct tbl_head *);
static	void		 tbl_hrule(struct termp *, 
				const struct tbl_span *);
static	void		 tbl_vframe(struct termp *, const struct tbl *);

void
term_tbl(struct termp *tp, const struct tbl_span *sp)
{
	const struct tbl_head *hp;
	const struct tbl_dat *dp;

	if (TBL_SPAN_FIRST & sp->flags)
		term_flushln(tp);

	if (TBL_SPAN_FIRST & sp->flags)
		tbl_hframe(tp, sp);

	tp->flags |= TERMP_NONOSPACE;
	tp->flags |= TERMP_NOSPACE;

	tbl_vframe(tp, sp->tbl);

	switch (sp->pos) {
	case (TBL_SPAN_HORIZ):
		/* FALLTHROUGH */
	case (TBL_SPAN_DHORIZ):
		tbl_hrule(tp, sp);
		tbl_vframe(tp, sp->tbl);
		term_newln(tp);
		tp->flags &= ~TERMP_NONOSPACE;
		return;
	default:
		break;
	}

	dp = sp->first;
	for (hp = sp->head; hp; hp = hp->next) {
		switch (hp->pos) {
		case (TBL_HEAD_VERT):
			/* FALLTHROUGH */
		case (TBL_HEAD_DVERT):
			tbl_spanner(tp, hp);
			break;
		case (TBL_HEAD_DATA):
			tbl_data(tp, sp->tbl, dp, hp->width);
			if (dp)
				dp = dp->next;
			break;
		default:
			abort();
			/* NOTREACHED */
		}
	}

	tbl_vframe(tp, sp->tbl);
	term_flushln(tp);

	if (TBL_SPAN_LAST & sp->flags)
		tbl_hframe(tp, sp);

	tp->flags &= ~TERMP_NONOSPACE;

}

static void
tbl_hrule(struct termp *tp, const struct tbl_span *sp)
{
	const struct tbl_head *hp;
	char		 c;

	/*
	 * An hrule extends across the entire table and is demarked by a
	 * standalone `_' or whatnot in lieu of a table row.  Spanning
	 * headers are marked by a `+', as are table boundaries.
	 */

	c = '-';
	if (TBL_SPAN_DHORIZ == sp->pos)
		c = '=';

	/* FIXME: don't use `+' between data and a spanner! */

	for (hp = sp->head; hp; hp = hp->next) {
		switch (hp->pos) {
		case (TBL_HEAD_DATA):
			tbl_char(tp, c, hp->width);
			break;
		case (TBL_HEAD_DVERT):
			tbl_char(tp, '+', hp->width);
			/* FALLTHROUGH */
		case (TBL_HEAD_VERT):
			tbl_char(tp, '+', hp->width);
			break;
		default:
			abort();
			/* NOTREACHED */
		}
	}
}

static void
tbl_hframe(struct termp *tp, const struct tbl_span *sp)
{
	const struct tbl_head *hp;

	if ( ! (TBL_OPT_BOX & sp->tbl->opts || 
			TBL_OPT_DBOX & sp->tbl->opts))
		return;

	tp->flags |= TERMP_NONOSPACE;
	tp->flags |= TERMP_NOSPACE;

	/* 
	 * Print out the horizontal part of a frame or double frame.  A
	 * double frame has an unbroken `-' outer line the width of the
	 * table, bordered by `+'.  The frame (or inner frame, in the
	 * case of the double frame) is a `-' bordered by `+' and broken
	 * by `+' whenever a span is encountered.
	 */

	if (TBL_OPT_DBOX & sp->tbl->opts) {
		term_word(tp, "+");
		for (hp = sp->head; hp; hp = hp->next)
			tbl_char(tp, '-', hp->width);
		term_word(tp, "+");
		term_flushln(tp);
	}

	term_word(tp, "+");
	for (hp = sp->head; hp; hp = hp->next) {
		switch (hp->pos) {
		case (TBL_HEAD_DATA):
			tbl_char(tp, '-', hp->width);
			break;
		default:
			tbl_char(tp, '+', hp->width);
			break;
		}
	}
	term_word(tp, "+");
	term_flushln(tp);
}

static void
tbl_data(struct termp *tp, const struct tbl *tbl,
		const struct tbl_dat *dp, int width)
{
	enum tbl_cellt	 pos;

	if (NULL == dp) {
		tbl_char(tp, ASCII_NBRSP, width);
		return;
	}

	switch (dp->pos) {
	case (TBL_DATA_HORIZ):
		/* FALLTHROUGH */
	case (TBL_DATA_DHORIZ):
		tbl_data_spanner(tp, dp, width);
		return;
	default:
		break;
	}
	
	pos = dp->layout ? dp->layout->pos : TBL_CELL_LEFT;

	switch (pos) {
	case (TBL_CELL_HORIZ):
		/* FALLTHROUGH */
	case (TBL_CELL_DHORIZ):
		tbl_data_spanner(tp, dp, width);
		break;
	case (TBL_CELL_LONG):
		/* FALLTHROUGH */
	case (TBL_CELL_CENTRE):
		/* FALLTHROUGH */
	case (TBL_CELL_LEFT):
		/* FALLTHROUGH */
	case (TBL_CELL_RIGHT):
		tbl_data_literal(tp, dp, width);
		break;
	case (TBL_CELL_NUMBER):
		tbl_data_number(tp, tbl, dp, width);
		break;
	default:
		abort();
		/* NOTREACHED */
	}
}
static void
tbl_spanner(struct termp *tp, const struct tbl_head *hp)
{

	switch (hp->pos) {
	case (TBL_HEAD_VERT):
		term_word(tp, "|");
		break;
	case (TBL_HEAD_DVERT):
		term_word(tp, "||");
		break;
	default:
		break;
	}
}

static void
tbl_vframe(struct termp *tp, const struct tbl *tbl)
{
	/* Always just a single vertical line. */

	if (TBL_OPT_BOX & tbl->opts || TBL_OPT_DBOX & tbl->opts)
		term_word(tp, "|");
}


static inline void
tbl_char(struct termp *tp, char c, int len)
{
	int		i;
	char		cp[2];

	cp[0] = c;
	cp[1] = '\0';

	for (i = 0; i < len; i++)
		term_word(tp, cp);
}

static void
tbl_data_spanner(struct termp *tp, const struct tbl_dat *dp, int width)
{

	switch (dp->pos) {
	case (TBL_DATA_HORIZ):
	case (TBL_DATA_NHORIZ):
		tbl_char(tp, '-', width);
		break;
	case (TBL_DATA_DHORIZ):
	case (TBL_DATA_NDHORIZ):
		tbl_char(tp, '=', width);
		break;
	default:
		break;
	}
}

static void
tbl_data_literal(struct termp *tp, const struct tbl_dat *dp, int width)
{
	int		 padl, padr;
	enum tbl_cellt	 pos;

	padl = padr = 0;

	pos = dp->layout ? dp->layout->pos : TBL_CELL_LEFT;

	switch (pos) {
	case (TBL_CELL_LONG):
		padl = 1;
		padr = width - (int)strlen(dp->string) - 1;
		break;
	case (TBL_CELL_CENTRE):
		padl = width - (int)strlen(dp->string);
		if (padl % 2)
			padr++;
		padl /= 2;
		padr += padl;
		break;
	case (TBL_CELL_RIGHT):
		padl = width - (int)strlen(dp->string);
		break;
	default:
		padr = width - (int)strlen(dp->string);
		break;
	}

	tbl_char(tp, ASCII_NBRSP, padl);
	term_word(tp, dp->string);
	tbl_char(tp, ASCII_NBRSP, padr);
}

static void
tbl_data_number(struct termp *tp, const struct tbl *tbl,
		const struct tbl_dat *dp, int width)
{
	char		*decp, pnt;
	int		 d, padl, sz;

	/*
	 * See calc_data_number().  Left-pad by taking the offset of our
	 * and the maximum decimal; right-pad by the remaining amount.
	 */

	sz = (int)strlen(dp->string);
	pnt = tbl->decimal;

	if (NULL == (decp = strchr(dp->string, pnt))) {
		d = sz + 1;
	} else {
		d = (int)(decp - dp->string) + 1;
	}

	assert(d <= dp->layout->head->decimal);
	assert(sz - d <= dp->layout->head->width -
			dp->layout->head->decimal);

	padl = dp->layout->head->decimal - d + 1;
	assert(width - sz - padl);

	tbl_char(tp, ASCII_NBRSP, padl);
	term_word(tp, dp->string);
	tbl_char(tp, ASCII_NBRSP, width - sz - padl);
}
