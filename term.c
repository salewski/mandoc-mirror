/* $Id$ */
/*
 * Copyright (c) 2008 Kristaps Dzonsons <kristaps@kth.se>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
#include <assert.h>
#include <ctype.h>
#include <curses.h>
#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <term.h>
#include <unistd.h>

#include "private.h"


enum	termesc {
	ESC_CLEAR,
	ESC_BOLD,
	ESC_UNDERLINE
};

struct	termp {
	size_t		  maxvisible;
	size_t		  maxcols;
	size_t		  indent;
	size_t		  col;
	int		  flags;
#define	TERMP_BOLD	 (1 << 0)
#define	TERMP_UNDERLINE	 (1 << 1)
#define	TERMP_NOSPACE	 (1 << 2)
	char		 *buf;
};

struct	termact {
	int		(*pre)(struct termp *,
				const struct mdoc_meta *,
				const struct mdoc_node *);
	int		(*post)(struct termp *,
				const struct mdoc_meta *,
				const struct mdoc_node *);
};

static	void		  termprint_r(struct termp *,
				const struct mdoc_meta *,
				const struct mdoc_node *);
static	void		  termprint_header(struct termp *,
				const struct mdoc_meta *);
static	void		  termprint_footer(struct termp *,
				const struct mdoc_meta *);

static	void		  newln(struct termp *);
static	void		  vspace(struct termp *);
static	void		  pword(struct termp *, const char *, size_t);
static	void		  word(struct termp *, const char *);

static	int		  termp_it_pre(struct termp *, 
				const struct mdoc_meta *,
				const struct mdoc_node *);
static	int		  termp_ns_pre(struct termp *, 
				const struct mdoc_meta *,
				const struct mdoc_node *);
static	int		  termp_pp_pre(struct termp *, 
				const struct mdoc_meta *,
				const struct mdoc_node *);
static	int		  termp_fl_pre(struct termp *, 
				const struct mdoc_meta *,
				const struct mdoc_node *);
static	int		  termp_op_pre(struct termp *, 
				const struct mdoc_meta *,
				const struct mdoc_node *);
static	int		  termp_op_post(struct termp *, 
				const struct mdoc_meta *,
				const struct mdoc_node *);
static	int		  termp_bl_post(struct termp *, 
				const struct mdoc_meta *,
				const struct mdoc_node *);
static	int		  termp_sh_post(struct termp *, 
				const struct mdoc_meta *,
				const struct mdoc_node *);
static	int		  termp_sh_pre(struct termp *, 
				const struct mdoc_meta *,
				const struct mdoc_node *);
static	int		  termp_nd_pre(struct termp *, 
				const struct mdoc_meta *,
				const struct mdoc_node *);
static	int		  termp_bold_pre(struct termp *, 
				const struct mdoc_meta *,
				const struct mdoc_node *);
static	int		  termp_under_pre(struct termp *, 
				const struct mdoc_meta *,
				const struct mdoc_node *);
static	int		  termp_bold_post(struct termp *, 
				const struct mdoc_meta *,
				const struct mdoc_node *);
static	int		  termp_under_post(struct termp *, 
				const struct mdoc_meta *,
				const struct mdoc_node *);

const	struct termact termacts[MDOC_MAX] = {
	{ NULL, NULL }, /* \" */
	{ NULL, NULL }, /* Dd */
	{ NULL, NULL }, /* Dt */
	{ NULL, NULL }, /* Os */
	{ termp_sh_pre, termp_sh_post }, /* Sh */
	{ NULL, NULL }, /* Ss */ 
	{ termp_pp_pre, NULL }, /* Pp */ 
	{ NULL, NULL }, /* D1 */
	{ NULL, NULL }, /* Dl */
	{ NULL, NULL }, /* Bd */
	{ NULL, NULL }, /* Ed */
	{ NULL, termp_bl_post }, /* Bl */
	{ NULL, NULL }, /* El */
	{ termp_it_pre, NULL }, /* It */
	{ NULL, NULL }, /* Ad */ 
	{ NULL, NULL }, /* An */
	{ termp_under_pre, termp_under_post }, /* Ar */
	{ NULL, NULL }, /* Cd */
	{ NULL, NULL }, /* Cm */
	{ NULL, NULL }, /* Dv */ 
	{ NULL, NULL }, /* Er */ 
	{ NULL, NULL }, /* Ev */ 
	{ NULL, NULL }, /* Ex */
	{ NULL, NULL }, /* Fa */ 
	{ NULL, NULL }, /* Fd */ 
	{ termp_fl_pre, termp_bold_post }, /* Fl */
	{ NULL, NULL }, /* Fn */ 
	{ NULL, NULL }, /* Ft */ 
	{ NULL, NULL }, /* Ic */ 
	{ NULL, NULL }, /* In */ 
	{ NULL, NULL }, /* Li */
	{ termp_nd_pre, NULL }, /* Nd */ 
	{ termp_bold_pre, termp_bold_post }, /* Nm */ 
	{ termp_op_pre, termp_op_post }, /* Op */
	{ NULL, NULL }, /* Ot */
	{ NULL, NULL }, /* Pa */
	{ NULL, NULL }, /* Rv */
	{ NULL, NULL }, /* St */ 
	{ NULL, NULL }, /* Va */
	{ NULL, NULL }, /* Vt */ 
	{ NULL, NULL }, /* Xr */
	{ NULL, NULL }, /* %A */
	{ NULL, NULL }, /* %B */
	{ NULL, NULL }, /* %D */
	{ NULL, NULL }, /* %I */
	{ NULL, NULL }, /* %J */
	{ NULL, NULL }, /* %N */
	{ NULL, NULL }, /* %O */
	{ NULL, NULL }, /* %P */
	{ NULL, NULL }, /* %R */
	{ NULL, NULL }, /* %T */
	{ NULL, NULL }, /* %V */
	{ NULL, NULL }, /* Ac */
	{ NULL, NULL }, /* Ao */
	{ NULL, NULL }, /* Aq */
	{ NULL, NULL }, /* At */
	{ NULL, NULL }, /* Bc */
	{ NULL, NULL }, /* Bf */ 
	{ NULL, NULL }, /* Bo */
	{ NULL, NULL }, /* Bq */
	{ NULL, NULL }, /* Bsx */
	{ NULL, NULL }, /* Bx */
	{ NULL, NULL }, /* Db */
	{ NULL, NULL }, /* Dc */
	{ NULL, NULL }, /* Do */
	{ NULL, NULL }, /* Dq */
	{ NULL, NULL }, /* Ec */
	{ NULL, NULL }, /* Ef */
	{ NULL, NULL }, /* Em */ 
	{ NULL, NULL }, /* Eo */
	{ NULL, NULL }, /* Fx */
	{ NULL, NULL }, /* Ms */
	{ NULL, NULL }, /* No */
	{ termp_ns_pre, NULL }, /* Ns */
	{ NULL, NULL }, /* Nx */
	{ NULL, NULL }, /* Ox */
	{ NULL, NULL }, /* Pc */
	{ NULL, NULL }, /* Pf */
	{ NULL, NULL }, /* Po */
	{ NULL, NULL }, /* Pq */
	{ NULL, NULL }, /* Qc */
	{ NULL, NULL }, /* Ql */
	{ NULL, NULL }, /* Qo */
	{ NULL, NULL }, /* Qq */
	{ NULL, NULL }, /* Re */
	{ NULL, NULL }, /* Rs */
	{ NULL, NULL }, /* Sc */
	{ NULL, NULL }, /* So */
	{ NULL, NULL }, /* Sq */
	{ NULL, NULL }, /* Sm */
	{ NULL, NULL }, /* Sx */
	{ NULL, NULL }, /* Sy */
	{ NULL, NULL }, /* Tn */
	{ NULL, NULL }, /* Ux */
	{ NULL, NULL }, /* Xc */
	{ NULL, NULL }, /* Xo */
	{ NULL, NULL }, /* Fo */ 
	{ NULL, NULL }, /* Fc */ 
	{ NULL, NULL }, /* Oo */
	{ NULL, NULL }, /* Oc */
	{ NULL, NULL }, /* Bk */
	{ NULL, NULL }, /* Ek */
	{ NULL, NULL }, /* Bt */
	{ NULL, NULL }, /* Hf */
	{ NULL, NULL }, /* Fr */
	{ NULL, NULL }, /* Ud */
};


static void
flush(struct termp *p)
{
	size_t		 i, j, vsz, vis, maxvis;

	maxvis = p->maxvisible - (p->indent * 4);
	vis = 0;

	for (j = 0; j < (p->indent * 4); j++)
		putchar(' ');

	for (i = 0; i < p->col; i++) {
		for (j = i, vsz = 0; j < p->col; j++) {
			if (isspace(p->buf[j]))
				break;
			else if (27 == p->buf[j]) {
				assert(j + 4 <= p->col);
				j += 3;
			} else
				vsz++;
		}
		assert(vsz > 0);

		if (vis && vis + vsz >= maxvis) {
			putchar('\n');
			for (j = 0; j < (p->indent * 4); j++)
				putchar(' ');
			vis = 0;
		} 

		for ( ; i < p->col; i++) {
			if (isspace(p->buf[i]))
				break;
			putchar(p->buf[i]);
		}
		vis += vsz;
		if (i < p->col) {
			putchar(' ');
			vis++;
		}
	}

	putchar('\n');
	p->col = 0;
}


static void
newln(struct termp *p)
{

	p->flags |= TERMP_NOSPACE;
	if (0 == p->col) 
		return;
	flush(p);
}


static void
vspace(struct termp *p)
{

	newln(p);
	putchar('\n');
}


static void
chara(struct termp *p, char c)
{

	if (p->col + 1 >= p->maxcols)
		errx(1, "line overrun");

	p->buf[(p->col)++] = c;
}


static void
escape(struct termp *p, enum termesc esc)
{

	if (p->col + 4 >= p->maxcols)
		errx(1, "line overrun");

	p->buf[(p->col)++] = 27;
	p->buf[(p->col)++] = '[';
	switch (esc) {
	case (ESC_CLEAR):
		p->buf[(p->col)++] = '0';
		break;
	case (ESC_BOLD):
		p->buf[(p->col)++] = '1';
		break;
	case (ESC_UNDERLINE):
		p->buf[(p->col)++] = '4';
		break;
	default:
		abort();
		/* NOTREACHED */
	}
	p->buf[(p->col)++] = 'm';
}


static void
pword(struct termp *p, const char *word, size_t len)
{
	size_t		 i;

	assert(len > 0);

	if ( ! (p->flags & TERMP_NOSPACE))
		chara(p, ' ');

	p->flags &= ~TERMP_NOSPACE;

	if (p->flags & TERMP_BOLD)
		escape(p, ESC_BOLD);
	if (p->flags & TERMP_UNDERLINE)
		escape(p, ESC_UNDERLINE);

	for (i = 0; i < len; i++) 
		chara(p, word[i]);

	if (p->flags & TERMP_BOLD ||
			p->flags & TERMP_UNDERLINE)
		escape(p, ESC_CLEAR);
}


static void
word(struct termp *p, const char *word)
{
	size_t 		 i, j, len;

	/* TODO: delimiters? */

	len = strlen(word);
	assert(len > 0);

	for (j = i = 0; i < len; i++) {
		if ( ! isspace(word[i])) {
			j++;
			continue;
		}
		if (0 == j)
			continue;
		assert(i >= j);
		pword(p, &word[i - j], j);
		j = 0;
	}
	if (j > 0) {
		assert(i >= j);
		pword(p, &word[i - j], j);
	}
}


static int
termp_it_pre(struct termp *p, const struct mdoc_meta *meta,
		const struct mdoc_node *node)
{

	switch (node->type) {
	case (MDOC_HEAD):
		/* TODO: only print one, if compat. */
		vspace(p);
		break;
	default:
		break;
	}
	return(1);
}


static int
termp_bold_post(struct termp *p, const struct mdoc_meta *meta,
		const struct mdoc_node *node)
{

	p->flags &= ~TERMP_BOLD;
	return(1);
}


static int
termp_under_pre(struct termp *p, const struct mdoc_meta *meta,
		const struct mdoc_node *node)
{

	p->flags |= TERMP_UNDERLINE;
	return(1);
}


static int
termp_bold_pre(struct termp *p, const struct mdoc_meta *meta,
		const struct mdoc_node *node)
{

	p->flags |= TERMP_BOLD;
	return(1);
}


static int
termp_ns_pre(struct termp *p, const struct mdoc_meta *meta,
		const struct mdoc_node *node)
{

	p->flags |= TERMP_NOSPACE;
	return(1);
}


static int
termp_pp_pre(struct termp *p, const struct mdoc_meta *meta,
		const struct mdoc_node *node)
{

	vspace(p);
	return(1);
}


static int
termp_under_post(struct termp *p, const struct mdoc_meta *meta,
		const struct mdoc_node *node)
{

	p->flags &= ~TERMP_UNDERLINE;
	return(1);
}


static int
termp_nd_pre(struct termp *p, const struct mdoc_meta *meta,
		const struct mdoc_node *node)
{

	word(p, "-");
	return(1);
}


static int
termp_bl_post(struct termp *p, const struct mdoc_meta *meta,
		const struct mdoc_node *node)
{

	switch (node->type) {
	case (MDOC_BLOCK):
		newln(p);
		break;
	default:
		break;
	}
	return(1);
}


static int
termp_op_post(struct termp *p, const struct mdoc_meta *meta,
		const struct mdoc_node *node)
{

	switch (node->type) {
	case (MDOC_BODY):
		p->flags |= TERMP_NOSPACE;
		word(p, "\\(rB");
		break;
	default:
		break;
	}
	return(1);
}


static int
termp_sh_post(struct termp *p, const struct mdoc_meta *meta,
		const struct mdoc_node *node)
{

	switch (node->type) {
	case (MDOC_HEAD):
		p->flags &= ~TERMP_BOLD;
		newln(p);
		break;
	case (MDOC_BODY):
		newln(p);
		(p->indent)--;
		break;
	default:
		break;
	}
	return(1);
}


static int
termp_sh_pre(struct termp *p, const struct mdoc_meta *meta,
		const struct mdoc_node *node)
{

	switch (node->type) {
	case (MDOC_HEAD):
		vspace(p);
		p->flags |= TERMP_BOLD;
		break;
	case (MDOC_BODY):
		(p->indent)++;
		break;
	default:
		break;
	}
	return(1);
}


static int
termp_op_pre(struct termp *p, const struct mdoc_meta *meta,
		const struct mdoc_node *node)
{

	switch (node->type) {
	case (MDOC_BODY):
		word(p, "\\(lB");
		p->flags |= TERMP_NOSPACE;
		break;
	default:
		break;
	}
	return(1);
}


static int
termp_fl_pre(struct termp *p, const struct mdoc_meta *meta,
		const struct mdoc_node *node)
{

	p->flags |= TERMP_BOLD;
	word(p, "-");
	p->flags |= TERMP_NOSPACE;
	return(1);
}


static void
termprint_r(struct termp *p, const struct mdoc_meta *meta,
		const struct mdoc_node *node)
{

	/* Pre-processing ----------------- */

	if (MDOC_TEXT != node->type) {
		if (termacts[node->tok].pre)
			if ( ! (*termacts[node->tok].pre)(p, meta, node))
				return;
	} else /* MDOC_TEXT == node->type */
		word(p, node->data.text.string);

	/* Children ---------------------- */

	if (NULL == node->child) {
		/* No-child processing. */
		switch (node->type) {
		case (MDOC_ELEM):
			switch (node->tok) {
			case (MDOC_Nm):
				word(p, "progname"); /* TODO */
				break;
			case (MDOC_Ar):
				word(p, "..."); 
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
	} else
		termprint_r(p, meta, node->child);

	/* Post-processing --------------- */

	if (MDOC_TEXT != node->type) {
		if (termacts[node->tok].post)
			if ( ! (*termacts[node->tok].post)(p, meta, node))
				return;
	} 

	/* Siblings ---------------------- */

	if (node->next)
		termprint_r(p, meta, node->next);
}


static void
termprint_footer(struct termp *p, const struct mdoc_meta *meta)
{
	struct tm	*tm;
	char		*buf, *os;
	size_t		 sz, osz, ssz, i;

	if (NULL == (buf = malloc(p->maxvisible)))
		err(1, "malloc");
	if (NULL == (os = malloc(p->maxvisible)))
		err(1, "malloc");

	tm = localtime(&meta->date);
	if (NULL == strftime(buf, p->maxvisible, "%B %d, %Y", tm))
		err(1, "strftime");

	osz = strlcpy(os, meta->os, p->maxvisible);

	sz = strlen(buf);
	ssz = sz + osz + 1;

	if (ssz > p->maxvisible) {
		ssz -= p->maxvisible;
		assert(ssz <= osz);
		os[osz - ssz] = 0;
		ssz = 1;
	} else
		ssz = p->maxvisible - ssz + 1;

	printf("\n");
	printf("%s", os);
	for (i = 0; i < ssz; i++)
		printf(" ");

	printf("%s\n", buf);
	fflush(stdout);

	free(buf);
	free(os);
}


static void
termprint_header(struct termp *p, const struct mdoc_meta *meta)
{
	char		*msec, *buf, *title, *pp;
	size_t		 ssz, tsz, ttsz, i;;

	if (NULL == (buf = malloc(p->maxvisible)))
		err(1, "malloc");
	if (NULL == (title = malloc(p->maxvisible)))
		err(1, "malloc");

	if (NULL == (pp = mdoc_vol2a(meta->vol)))
		switch (meta->msec) {
		case (MSEC_1):
			/* FALLTHROUGH */
		case (MSEC_6):
			/* FALLTHROUGH */
		case (MSEC_7):
			pp = mdoc_vol2a(VOL_URM);
			break;
		case (MSEC_8):
			pp = mdoc_vol2a(VOL_SMM);
			break;
		case (MSEC_2):
			/* FALLTHROUGH */
		case (MSEC_3):
			/* FALLTHROUGH */
		case (MSEC_4):
			/* FALLTHROUGH */
		case (MSEC_5):
			pp = mdoc_vol2a(VOL_PRM);
			break;
		case (MSEC_9):
			pp = mdoc_vol2a(VOL_KM);
			break;
		default:
			/* FIXME: capitalise. */
			if (NULL == (pp = mdoc_msec2a(meta->msec)))
				pp = mdoc_msec2a(MSEC_local);
			break;
		}
	assert(pp);

	tsz = strlcpy(buf, pp, p->maxvisible);
	assert(tsz < p->maxvisible);

	if ((pp = mdoc_arch2a(meta->arch))) {
		tsz = strlcat(buf, " (", p->maxvisible);
		assert(tsz < p->maxvisible);
		tsz = strlcat(buf, pp, p->maxvisible);
		assert(tsz < p->maxvisible);
		tsz = strlcat(buf, ")", p->maxvisible);
		assert(tsz < p->maxvisible);
	}

	ttsz = strlcpy(title, meta->title, p->maxvisible);

	if (NULL == (msec = mdoc_msec2a(meta->msec)))
		msec = "";

	ssz = (2 * (ttsz + 2 + strlen(msec))) + tsz + 2;

	if (ssz > p->maxvisible) {
		if ((ssz -= p->maxvisible) % 2)
			ssz++;
		ssz /= 2;
	
		assert(ssz <= ttsz);
		title[ttsz - ssz] = 0;
		ssz = 1;
	} else
		ssz = ((p->maxvisible - ssz) / 2) + 1;

	printf("%s(%s)", title, msec);

	for (i = 0; i < ssz; i++)
		printf(" ");

	printf("%s", buf);

	for (i = 0; i < ssz; i++)
		printf(" ");

	printf("%s(%s)\n", title, msec);
	fflush(stdout);

	free(title);
	free(buf);
}


int
termprint(const struct mdoc_node *node,
		const struct mdoc_meta *meta)
{
	struct termp	 p;

	if (ERR == setupterm(NULL, STDOUT_FILENO, NULL))
		return(0);

	p.maxvisible = columns < 60 ? 60 : (size_t)columns;
	p.maxcols = 1024;
	p.indent = p.col = 0;
	p.flags = TERMP_NOSPACE;

	if (NULL == (p.buf = malloc(p.maxcols)))
		err(1, "malloc");

	termprint_header(&p, meta);
	termprint_r(&p, meta, node);
	termprint_footer(&p, meta);

	free(p.buf);

	return(1);
}


