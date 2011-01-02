/*	$Id$ */
/*
 * Copyright (c) 2009, 2010 Kristaps Dzonsons <kristaps@bsd.lv>
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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mandoc.h"
#include "roff.h"
#include "libmandoc.h"
#include "libroff.h"

static	void	 tbl_calc(struct tbl_node *);
static	void	 tbl_calc_data(struct tbl_node *, struct tbl_dat *);
static	void	 tbl_calc_data_literal(struct tbl_dat *);
static	void	 tbl_calc_data_number(struct tbl_node *, struct tbl_dat *);
static	void	 tbl_calc_data_spanner(struct tbl_dat *);

enum rofferr
tbl_read(struct tbl_node *tbl, int ln, const char *p, int offs)
{
	int		 len;
	const char	*cp;

	cp = &p[offs];
	len = (int)strlen(cp);

	/*
	 * If we're in the options section and we don't have a
	 * terminating semicolon, assume we've moved directly into the
	 * layout section.  No need to report a warning: this is,
	 * apparently, standard behaviour.
	 */

	if (TBL_PART_OPTS == tbl->part && len)
		if (';' != cp[len - 1])
			tbl->part = TBL_PART_LAYOUT;

	/* Now process each logical section of the table.  */

	switch (tbl->part) {
	case (TBL_PART_OPTS):
		return(tbl_option(tbl, ln, p) ? ROFF_IGN : ROFF_ERR);
	case (TBL_PART_LAYOUT):
		return(tbl_layout(tbl, ln, p) ? ROFF_IGN : ROFF_ERR);
	case (TBL_PART_DATA):
		break;
	}

	/*
	 * This only returns zero if the line is empty, so we ignore it
	 * and continue on.
	 */
	return(tbl_data(tbl, ln, p) ? ROFF_TBL : ROFF_IGN);
}

struct tbl_node *
tbl_alloc(int pos, int line, void *data, const mandocmsg msg)
{
	struct tbl_node	*p;

	p = mandoc_calloc(1, sizeof(struct tbl_node));
	p->line = line;
	p->pos = pos;
	p->data = data;
	p->msg = msg;
	p->part = TBL_PART_OPTS;
	p->opts.tab = '\t';
	p->opts.linesize = 12;
	p->opts.decimal = '.';
	return(p);
}

void
tbl_free(struct tbl_node *p)
{
	struct tbl_row	*rp;
	struct tbl_cell	*cp;
	struct tbl_span	*sp;
	struct tbl_dat	*dp;
	struct tbl_head	*hp;

	while (NULL != (rp = p->first_row)) {
		p->first_row = rp->next;
		while (rp->first) {
			cp = rp->first;
			rp->first = cp->next;
			free(cp);
		}
		free(rp);
	}

	while (NULL != (sp = p->first_span)) {
		p->first_span = sp->next;
		while (sp->first) {
			dp = sp->first;
			sp->first = dp->next;
			if (dp->string)
				free(dp->string);
			free(dp);
		}
		free(sp);
	}

	while (NULL != (hp = p->first_head)) {
		p->first_head = hp->next;
		free(hp);
	}

	free(p);
}

void
tbl_restart(int line, int pos, struct tbl_node *tbl)
{

	tbl->part = TBL_PART_LAYOUT;
	tbl->line = line;
	tbl->pos = pos;

	if (NULL == tbl->first_span || NULL == tbl->first_span->first)
		TBL_MSG(tbl, MANDOCERR_TBLNODATA, tbl->line, tbl->pos);
}

const struct tbl_span *
tbl_span(const struct tbl_node *tbl)
{

	assert(tbl);
	return(tbl->last_span);
}

void
tbl_end(struct tbl_node *tbl)
{

	if (NULL == tbl->first_span || NULL == tbl->first_span->first)
		TBL_MSG(tbl, MANDOCERR_TBLNODATA, tbl->line, tbl->pos);
	else
		tbl_calc(tbl);

	if (tbl->last_span)
		tbl->last_span->flags |= TBL_SPAN_LAST;
}

static void
tbl_calc(struct tbl_node *tbl)
{
	struct tbl_span	*sp;
	struct tbl_dat	*dp;
	struct tbl_head	*hp;

	/* Calculate width as the max of column cells' widths. */

	for (sp = tbl->first_span; sp; sp = sp->next) {
		switch (sp->pos) {
		case (TBL_DATA_HORIZ):
			/* FALLTHROUGH */
		case (TBL_DATA_DHORIZ):
			continue;
		default:
			break;
		}
		for (dp = sp->first; dp; dp = dp->next)
			tbl_calc_data(tbl, dp);
	}

	/* Calculate width as the simple spanner value. */

	for (hp = tbl->first_head; hp; hp = hp->next) 
		switch (hp->pos) {
		case (TBL_HEAD_VERT):
			hp->width = 1;
			break;
		case (TBL_HEAD_DVERT):
			hp->width = 2;
			break;
		default:
			break;
		}
}

static void
tbl_calc_data(struct tbl_node *tbl, struct tbl_dat *data)
{

	/*
	 * This is the case with overrunning cells... 
	 */
	if (NULL == data->layout)
		return;

	/* Branch down into data sub-types. */

	switch (data->layout->pos) {
	case (TBL_CELL_HORIZ):
		/* FALLTHROUGH */
	case (TBL_CELL_DHORIZ):
		tbl_calc_data_spanner(data);
		break;
	case (TBL_CELL_LONG):
		/* FALLTHROUGH */
	case (TBL_CELL_CENTRE):
		/* FALLTHROUGH */
	case (TBL_CELL_LEFT):
		/* FALLTHROUGH */
	case (TBL_CELL_RIGHT):
		tbl_calc_data_literal(data);
		break;
	case (TBL_CELL_NUMBER):
		tbl_calc_data_number(tbl, data);
		break;
	default:
		abort();
		/* NOTREACHED */
	}
}

static void
tbl_calc_data_spanner(struct tbl_dat *data)
{

	/* N.B., these are horiz spanners (not vert) so always 1. */
	data->layout->head->width = 1;
}

static void
tbl_calc_data_number(struct tbl_node *tbl, struct tbl_dat *data)
{
	int 		 sz, d;
	char		*dp, pnt;

	/*
	 * First calculate number width and decimal place (last + 1 for
	 * no-decimal numbers).  If the stored decimal is subsequent
	 * ours, make our size longer by that difference
	 * (right-"shifting"); similarly, if ours is subsequent the
	 * stored, then extend the stored size by the difference.
	 * Finally, re-assign the stored values.
	 */

	/* TODO: use spacing modifier. */

	assert(data->string);
	sz = (int)strlen(data->string);
	pnt = tbl->opts.decimal;

	if (NULL == (dp = strchr(data->string, pnt)))
		d = sz + 1;
	else
		d = (int)(dp - data->string) + 1;

	sz += 2;

	if (data->layout->head->decimal > d) {
		sz += data->layout->head->decimal - d;
		d = data->layout->head->decimal;
	} else
		data->layout->head->width += 
			d - data->layout->head->decimal;

	if (sz > data->layout->head->width)
		data->layout->head->width = sz;
	if (d > data->layout->head->decimal)
		data->layout->head->decimal = d;
}

static void
tbl_calc_data_literal(struct tbl_dat *data)
{
	int		 sz, bufsz;

	/* 
	 * Calculate our width and use the spacing, with a minimum
	 * spacing dictated by position (centre, e.g,. gets a space on
	 * either side, while right/left get a single adjacent space).
	 */

	assert(data->string);
	sz = (int)strlen(data->string);

	switch (data->layout->pos) {
	case (TBL_CELL_LONG):
		/* FALLTHROUGH */
	case (TBL_CELL_CENTRE):
		bufsz = 2;
		break;
	default:
		bufsz = 1;
		break;
	}

	if (data->layout->spacing)
		bufsz = bufsz > data->layout->spacing ? 
			bufsz : data->layout->spacing;

	sz += bufsz;
	if (data->layout->head->width < sz)
		data->layout->head->width = sz;
}


