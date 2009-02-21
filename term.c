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
	size_t		  rmargin;
	size_t		  maxrmargin;
	size_t		  maxcols;
	size_t		  offset;
	size_t		  col;
	int		  flags;
#define	TERMP_BOLD	 (1 << 0)	/* Embolden words. */
#define	TERMP_UNDERLINE	 (1 << 1)	/* Underline words. */
#define	TERMP_NOSPACE	 (1 << 2)	/* No space before words. */
#define	TERMP_NOLPAD	 (1 << 3)	/* No left-padding. */
#define	TERMP_NOBREAK	 (1 << 4)	/* No break after line */
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

static	int		  arg_hasattr(int, size_t, 
				const struct mdoc_arg *);
static	int		  arg_getattr(int, size_t, 
				const struct mdoc_arg *);

static	void		  newln(struct termp *);
static	void		  vspace(struct termp *);
static	void		  pword(struct termp *, const char *, size_t);
static	void		  word(struct termp *, const char *);

#define	decl_prepost(name, suffix) \
static	int		  name##_##suffix(struct termp *, \
				const struct mdoc_meta *, \
				const struct mdoc_node *)

#define	decl_pre(name)	  decl_prepost(name, pre)
#define	decl_post(name)   decl_prepost(name, post)

decl_pre(termp_fl);
decl_pre(termp_it);
decl_pre(termp_nd);
decl_pre(termp_ns);
decl_pre(termp_op);
decl_pre(termp_pp);
decl_pre(termp_sh);

decl_post(termp_bl);
decl_post(termp_it);
decl_post(termp_op);
decl_post(termp_sh);

decl_pre(termp_bold);
decl_pre(termp_under);

decl_post(termp_bold);
decl_post(termp_under);

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
	{ termp_it_pre, termp_it_post }, /* It */
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


static int
arg_hasattr(int arg, size_t argc, const struct mdoc_arg *argv)
{

	return(-1 != arg_getattr(arg, argc, argv));
}


static int
arg_getattr(int arg, size_t argc, const struct mdoc_arg *argv)
{
	int		 i;

	for (i = 0; i < (int)argc; i++) 
		if (argv[i].arg == arg)
			return(i);
	return(-1);
}


static void
flushln(struct termp *p)
{
	size_t		 i, j, vsz, vis, maxvis;

	/*
	 * First, establish the maximum columns of "visible" content.
	 * This is usually the difference between the right-margin and
	 * an indentation, but can be, for tagged lists or columns, a
	 * small set of values.
	 */

	assert(p->offset < p->rmargin);
	maxvis = p->rmargin - p->offset;
	vis = 0;

	/*
	 * If in the standard case (left-justified), then begin with our
	 * indentation, otherwise (columns, etc.) just start spitting
	 * out text.
	 */

	if ( ! (p->flags & TERMP_NOLPAD))
		/* LINTED */
		for (j = 0; j < p->offset; j++)
			putchar(' ');

	for (i = 0; i < p->col; i++) {
		/*
		 * Count up visible word characters.  Control sequences
		 * (starting with the CSI) aren't counted. 
		 */
		assert( ! isspace(p->buf[i]));

		/* LINTED */
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

		/*
		 * If a word is too long and we're within a line, put it
		 * on the next line.  Puke if we're being asked to write
		 * something that will exceed the right margin (i.e.,
		 * from a fresh line or when we're not allowed to break
		 * the line with TERMP_NOBREAK).
		 */

		if (vis && vis + vsz >= maxvis) {
			/* FIXME */
			if (p->flags & TERMP_NOBREAK)
				errx(1, "word breaks right margin");
			putchar('\n');
			for (j = 0; j < p->offset; j++)
				putchar(' ');
			vis = 0;
		} else if (vis + vsz >= maxvis) {
			/* FIXME */
			errx(1, "word breaks right margin");
		}

		/* 
		 * Write out the word and a trailing space.  Omit the
		 * space if we're the last word in the line.
		 */

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

	/*
	 * If we're not to right-marginalise it (newline), then instead
	 * pad to the right margin and stay off.
	 */

	if (p->flags & TERMP_NOBREAK) {
		for ( ; vis <= maxvis; vis++)
			putchar(' ');
	} else
		putchar('\n');

	p->col = 0;
}


static void
newln(struct termp *p)
{

	/* 
	 * A newline only breaks an existing line; it won't assert
	 * vertical space.
	 */
	p->flags |= TERMP_NOSPACE;
	if (0 == p->col) 
		return;
	flushln(p);
}


static void
vspace(struct termp *p)
{

	/*
	 * Asserts a vertical space (a full, empty line-break between
	 * lines).
	 */
	newln(p);
	putchar('\n');
}


static void
chara(struct termp *p, char c)
{

	/* TODO: dynamically expand the buffer. */
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

	/* TODO: escape patterns. */

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

	if (mdoc_isdelim(word))
		p->flags |= TERMP_NOSPACE;

	len = strlen(word);
	assert(len > 0);

	/* LINTED */
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


/* ARGSUSED */
static int
termp_it_post(struct termp *p, const struct mdoc_meta *meta,
		const struct mdoc_node *node)
{
	const struct mdoc_node *n, *it;
	const struct mdoc_block *bl;
	int		 i;
	size_t		 width;

	switch (node->type) {
	case (MDOC_BODY):
		/* FALLTHROUGH */
	case (MDOC_HEAD):
		break;
	default:
		return(1);
	}

	it = node->parent;
	assert(MDOC_BLOCK == it->type);
	assert(MDOC_It == it->tok);

	n = it->parent;
	assert(MDOC_BODY == n->type);
	assert(MDOC_Bl == n->tok);
	n = n->parent;
	bl = &n->data.block;

	/* If `-tag', adjust our margins accordingly. */

	if (arg_hasattr(MDOC_Tag, bl->argc, bl->argv)) {
		i = arg_getattr(MDOC_Width, bl->argc, bl->argv);
		assert(i >= 0);
		assert(1 == bl->argv[i].sz);
		width = strlen(*bl->argv[i].value); /* XXX */

		if (MDOC_HEAD == node->type) {
			flushln(p);
			/* FIXME: nested lists. */
			p->rmargin = p->maxrmargin;
			p->flags &= ~TERMP_NOBREAK;
		} else {
			flushln(p);
			p->offset -= width + 1;
			p->flags &= ~TERMP_NOLPAD;
		}
	}

	return(1);
}


/* ARGSUSED */
static int
termp_it_pre(struct termp *p, const struct mdoc_meta *meta,
		const struct mdoc_node *node)
{
	const struct mdoc_node *n, *it;
	const struct mdoc_block *bl;
	int		 i;
	size_t		 width;

	switch (node->type) {
	case (MDOC_BODY):
		/* FALLTHROUGH */
	case (MDOC_HEAD):
		it = node->parent;
		break;
	case (MDOC_BLOCK):
		it = node;
		break;
	default:
		return(1);
	}

	assert(MDOC_BLOCK == it->type);
	assert(MDOC_It == it->tok);

	n = it->parent;
	assert(MDOC_BODY == n->type);
	assert(MDOC_Bl == n->tok);
	n = n->parent;
	bl = &n->data.block;

	/* If `-compact', don't assert vertical space. */

	if (MDOC_BLOCK == node->type) {
		if (arg_hasattr(MDOC_Compact, bl->argc, bl->argv))
			newln(p);
		else
			vspace(p);
		return(1);
	}

	assert(MDOC_HEAD == node->type 
			|| MDOC_BODY == node->type);

	/* If `-tag', adjust our margins accordingly. */

	if (arg_hasattr(MDOC_Tag, bl->argc, bl->argv)) {
		i = arg_getattr(MDOC_Width, bl->argc, bl->argv);
		assert(i >= 0); /* XXX */
		assert(1 == bl->argv[i].sz);
		width = strlen(*bl->argv[i].value); /* XXX */

		/* FIXME: nested lists. */

		if (MDOC_HEAD == node->type) {
			p->flags |= TERMP_NOBREAK;
			p->flags |= TERMP_NOSPACE;
			p->rmargin = p->offset + width;
		} else {
			p->flags |= TERMP_NOSPACE;
			p->flags |= TERMP_NOLPAD;
			p->offset += width + 1;
		}
	}

	return(1);
}


/* ARGSUSED */
static int
termp_bold_post(struct termp *p, const struct mdoc_meta *meta,
		const struct mdoc_node *node)
{

	p->flags &= ~TERMP_BOLD;
	return(1);
}


/* ARGSUSED */
static int
termp_under_pre(struct termp *p, const struct mdoc_meta *meta,
		const struct mdoc_node *node)
{

	p->flags |= TERMP_UNDERLINE;
	return(1);
}


/* ARGSUSED */
static int
termp_bold_pre(struct termp *p, const struct mdoc_meta *meta,
		const struct mdoc_node *node)
{

	p->flags |= TERMP_BOLD;
	return(1);
}


/* ARGSUSED */
static int
termp_ns_pre(struct termp *p, const struct mdoc_meta *meta,
		const struct mdoc_node *node)
{

	p->flags |= TERMP_NOSPACE;
	return(1);
}


/* ARGSUSED */
static int
termp_pp_pre(struct termp *p, const struct mdoc_meta *meta,
		const struct mdoc_node *node)
{

	vspace(p);
	return(1);
}


/* ARGSUSED */
static int
termp_under_post(struct termp *p, const struct mdoc_meta *meta,
		const struct mdoc_node *node)
{

	p->flags &= ~TERMP_UNDERLINE;
	return(1);
}


/* ARGSUSED */
static int
termp_nd_pre(struct termp *p, const struct mdoc_meta *meta,
		const struct mdoc_node *node)
{

	word(p, "-");
	return(1);
}


/* ARGSUSED */
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


/* ARGSUSED */
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


/* ARGSUSED */
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
		p->offset -= 4;
		break;
	default:
		break;
	}
	return(1);
}


/* ARGSUSED */
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
		p->offset += 4;
		break;
	default:
		break;
	}
	return(1);
}


/* ARGSUSED */
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


/* ARGSUSED */
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

	if (NULL == (buf = malloc(p->rmargin)))
		err(1, "malloc");
	if (NULL == (os = malloc(p->rmargin)))
		err(1, "malloc");

	tm = localtime(&meta->date);
	if (NULL == strftime(buf, p->rmargin, "%B %d, %Y", tm))
		err(1, "strftime");

	osz = strlcpy(os, meta->os, p->rmargin);

	sz = strlen(buf);
	ssz = sz + osz + 1;

	if (ssz > p->rmargin) {
		ssz -= p->rmargin;
		assert(ssz <= osz);
		os[osz - ssz] = 0;
		ssz = 1;
	} else
		ssz = p->rmargin - ssz + 1;

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

	if (NULL == (buf = malloc(p->rmargin)))
		err(1, "malloc");
	if (NULL == (title = malloc(p->rmargin)))
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

	tsz = strlcpy(buf, pp, p->rmargin);
	assert(tsz < p->rmargin);

	if ((pp = mdoc_arch2a(meta->arch))) {
		tsz = strlcat(buf, " (", p->rmargin);
		assert(tsz < p->rmargin);
		tsz = strlcat(buf, pp, p->rmargin);
		assert(tsz < p->rmargin);
		tsz = strlcat(buf, ")", p->rmargin);
		assert(tsz < p->rmargin);
	}

	ttsz = strlcpy(title, meta->title, p->rmargin);

	if (NULL == (msec = mdoc_msec2a(meta->msec)))
		msec = "";

	ssz = (2 * (ttsz + 2 + strlen(msec))) + tsz + 2;

	if (ssz > p->rmargin) {
		if ((ssz -= p->rmargin) % 2)
			ssz++;
		ssz /= 2;
	
		assert(ssz <= ttsz);
		title[ttsz - ssz] = 0;
		ssz = 1;
	} else
		ssz = ((p->rmargin - ssz) / 2) + 1;

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

	p.maxrmargin = columns < 60 ? 60 : (size_t)columns;
	p.rmargin = p.maxrmargin;
	p.maxcols = 1024;
	p.offset = p.col = 0;
	p.flags = TERMP_NOSPACE;

	if (NULL == (p.buf = malloc(p.maxcols)))
		err(1, "malloc");

	termprint_header(&p, meta);
	termprint_r(&p, meta, node);
	termprint_footer(&p, meta);

	free(p.buf);

	return(1);
}


