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
#include <err.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef __OpenBSD__
#include <time.h>
#endif

#include "mmain.h"
#include "term.h"

#ifdef __NetBSD__
#define xisspace(x) isspace((int)(x))
#else
#define	xisspace(x) isspace((x))
#endif

enum	termstyle {
	STYLE_CLEAR,
	STYLE_BOLD,
	STYLE_UNDERLINE
};

static	void		  body(struct termp *,
				struct termpair *,
				const struct mdoc_meta *,
				const struct mdoc_node *);
static	void		  header(struct termp *,
				const struct mdoc_meta *);
static	void		  footer(struct termp *,
				const struct mdoc_meta *);

static	void		  pword(struct termp *, const char *, size_t);
static	void		  pescape(struct termp *, 
				const char *, size_t *, size_t);
static	void		  nescape(struct termp *, 
				const char *, size_t);
static	void		  chara(struct termp *, char);
static	void		  stringa(struct termp *, const char *);
static	void		  style(struct termp *, enum termstyle);

#ifdef __linux__
extern	size_t		  strlcat(char *, const char *, size_t);
extern	size_t		  strlcpy(char *, const char *, size_t);
#endif


int
main(int argc, char *argv[])
{
	struct mmain	*p;
	const struct mdoc *mdoc;
	struct termp	 termp;

	p = mmain_alloc();

	if ( ! mmain_getopt(p, argc, argv, NULL, NULL, NULL, NULL))
		mmain_exit(p, 1);

	if (NULL == (mdoc = mmain_mdoc(p)))
		mmain_exit(p, 1);

	termp.maxrmargin = 80; /* XXX */
	termp.rmargin = termp.maxrmargin;
	termp.maxcols = 1024;
	termp.offset = termp.col = 0;
	termp.flags = TERMP_NOSPACE;

	if (NULL == (termp.buf = malloc(termp.maxcols)))
		err(1, "malloc");

	header(&termp, mdoc_meta(mdoc));
	body(&termp, NULL, mdoc_meta(mdoc), mdoc_node(mdoc));
	footer(&termp, mdoc_meta(mdoc));

	free(termp.buf);

	mmain_exit(p, 0);
	/* NOTREACHED */
}


void
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

	/*
	 * If we're literal, print out verbatim.
	 */
	if (p->flags & TERMP_LITERAL) {
		/* FIXME: count non-printing chars. */
		for (i = 0; i < p->col; i++)
			putchar(p->buf[i]);
		putchar('\n');
		p->col = 0;
		return;
	}

	for (i = 0; i < p->col; i++) {
		/*
		 * Count up visible word characters.  Control sequences
		 * (starting with the CSI) aren't counted. 
		 */
		assert( ! xisspace(p->buf[i]));

		/* LINTED */
		for (j = i, vsz = 0; j < p->col; j++) {
			if (xisspace(p->buf[j]))
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

		if (vis && vis + vsz > maxvis) {
			/* FIXME */
			if (p->flags & TERMP_NOBREAK)
				errx(1, "word breaks right margin");
			putchar('\n');
			for (j = 0; j < p->offset; j++)
				putchar(' ');
			vis = 0;
		} else if (vis + vsz > maxvis)
			/* FIXME */
			errx(1, "word breaks right margin");

		/* 
		 * Write out the word and a trailing space.  Omit the
		 * space if we're the last word in the line.
		 */

		for ( ; i < p->col; i++) {
			if (xisspace(p->buf[i]))
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
		if ( ! (p->flags & TERMP_NORPAD))
			for ( ; vis < maxvis; vis++)
				putchar(' ');
	} else
		putchar('\n');

	p->col = 0;
}


void
newln(struct termp *p)
{

	/* 
	 * A newline only breaks an existing line; it won't assert
	 * vertical space.
	 */
	p->flags |= TERMP_NOSPACE;
	if (0 == p->col) {
		p->flags &= ~TERMP_NOLPAD;
		return;
	}
	flushln(p);
	p->flags &= ~TERMP_NOLPAD;
}


void
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
stringa(struct termp *p, const char *s)
{

	/* XXX - speed up if not passing to chara. */
	for ( ; *s; s++)
		chara(p, *s);
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
style(struct termp *p, enum termstyle esc)
{

	if (p->col + 4 >= p->maxcols)
		errx(1, "line overrun");

	p->buf[(p->col)++] = 27;
	p->buf[(p->col)++] = '[';
	switch (esc) {
	case (STYLE_CLEAR):
		p->buf[(p->col)++] = '0';
		break;
	case (STYLE_BOLD):
		p->buf[(p->col)++] = '1';
		break;
	case (STYLE_UNDERLINE):
		p->buf[(p->col)++] = '4';
		break;
	default:
		abort();
		/* NOTREACHED */
	}
	p->buf[(p->col)++] = 'm';
}


static void
nescape(struct termp *p, const char *word, size_t len)
{

	switch (len) {
	case (2):
		if ('r' == word[0] && 'B' == word[1])
			chara(p, ']');
		else if ('l' == word[0] && 'B' == word[1])
			chara(p, '[');
		else if ('<' == word[0] && '-' == word[1])
			stringa(p, "<-");
		else if ('-' == word[0] && '>' == word[1])
			stringa(p, "->");
		else if ('l' == word[0] && 'q' == word[1])
			chara(p, '\"');
		else if ('r' == word[0] && 'q' == word[1])
			chara(p, '\"');
		else if ('b' == word[0] && 'u' == word[1])
			chara(p, 'o');
		break;
	default:
		break;
	}
}


static void
pescape(struct termp *p, const char *word, size_t *i, size_t len)
{
	size_t		 j;

	(*i)++;
	assert(*i < len);

	if ('(' == word[*i]) {
		/* Two-character escapes. */
		(*i)++;
		assert(*i + 1 < len);
		nescape(p, &word[*i], 2);
		(*i)++;
		return;

	} else if ('[' != word[*i]) {
		/* One-character escapes. */
		switch (word[*i]) {
		case ('\\'):
			/* FALLTHROUGH */
		case ('\''):
			/* FALLTHROUGH */
		case ('`'):
			/* FALLTHROUGH */
		case ('-'):
			/* FALLTHROUGH */
		case (' '):
			/* FALLTHROUGH */
		case ('.'):
			chara(p, word[*i]);
		default:
			break;
		}
		return;
	}

	(*i)++;
	for (j = 0; word[*i] && ']' != word[*i]; (*i)++, j++)
		/* Loop... */ ;

	nescape(p, &word[*i - j], j);
}


static void
pword(struct termp *p, const char *word, size_t len)
{
	size_t		 i;

	/*assert(len > 0);*/ /* Can be, if literal. */

	if ( ! (p->flags & TERMP_NOSPACE) && 
			! (p->flags & TERMP_LITERAL))
		chara(p, ' ');

	if ( ! (p->flags & TERMP_NONOSPACE))
		p->flags &= ~TERMP_NOSPACE;

	if (p->flags & TERMP_BOLD)
		style(p, STYLE_BOLD);
	if (p->flags & TERMP_UNDERLINE)
		style(p, STYLE_UNDERLINE);

	for (i = 0; i < len; i++) {
		if ('\\' == word[i]) {
			pescape(p, word, &i, len);
			continue;
		}
		chara(p, word[i]);
	}

	if (p->flags & TERMP_BOLD ||
			p->flags & TERMP_UNDERLINE)
		style(p, STYLE_CLEAR);
}


void
word(struct termp *p, const char *word)
{
	size_t 		 i, j, len;

	if (p->flags & TERMP_LITERAL) {
		pword(p, word, strlen(word));
		return;
	}

	len = strlen(word);
	assert(len > 0);

	if (mdoc_isdelim(word)) {
		if ( ! (p->flags & TERMP_IGNDELIM))
			p->flags |= TERMP_NOSPACE;
		p->flags &= ~TERMP_IGNDELIM;
	}

	/* LINTED */
	for (j = i = 0; i < len; i++) {
		if ( ! xisspace(word[i])) {
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


static void
body(struct termp *p, struct termpair *ppair,
		const struct mdoc_meta *meta,
		const struct mdoc_node *node)
{
	int		 dochild;
	struct termpair	 pair;

	/* Pre-processing. */

	dochild = 1;
	pair.ppair = ppair;
	pair.type = 0;
	pair.offset = pair.rmargin = 0;
	pair.flag = 0;
	pair.count = 0;

	if (MDOC_TEXT != node->type) {
		if (termacts[node->tok].pre)
			if ( ! (*termacts[node->tok].pre)(p, &pair, meta, node))
				dochild = 0;
	} else /* MDOC_TEXT == node->type */
		word(p, node->data.text.string);

	/* Children. */

	if (TERMPAIR_FLAG & pair.type)
		p->flags |= pair.flag;

	if (dochild && node->child)
		body(p, &pair, meta, node->child);

	if (TERMPAIR_FLAG & pair.type)
		p->flags &= ~pair.flag;

	/* Post-processing. */

	if (MDOC_TEXT != node->type)
		if (termacts[node->tok].post)
			(*termacts[node->tok].post)(p, &pair, meta, node);

	/* Siblings. */

	if (node->next)
		body(p, ppair, meta, node->next);
}


static void
footer(struct termp *p, const struct mdoc_meta *meta)
{
	struct tm	*tm;
	char		*buf, *os;

	if (NULL == (buf = malloc(p->rmargin)))
		err(1, "malloc");
	if (NULL == (os = malloc(p->rmargin)))
		err(1, "malloc");

	tm = localtime(&meta->date);

#ifdef __OpenBSD__
	if (NULL == strftime(buf, p->rmargin, "%B %d, %Y", tm))
#else
	if (0 == strftime(buf, p->rmargin, "%B %d, %Y", tm))
#endif
		err(1, "strftime");

	(void)strlcpy(os, meta->os, p->rmargin);

	vspace(p);

	p->flags |= TERMP_NOSPACE | TERMP_NOBREAK;
	p->rmargin = p->maxrmargin - strlen(buf);
	p->offset = 0;

	word(p, os);
	flushln(p);

	p->flags |= TERMP_NOLPAD | TERMP_NOSPACE;
	p->offset = p->rmargin;
	p->rmargin = p->maxrmargin;
	p->flags &= ~TERMP_NOBREAK;

	word(p, buf);
	flushln(p);

	free(buf);
	free(os);
}


static void
header(struct termp *p, const struct mdoc_meta *meta)
{
	char		*buf, *title;
	const char	*pp;

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

	if (mdoc_arch2a(meta->arch))
		(void)snprintf(buf, p->rmargin, "%s(%s)",
				pp, mdoc_arch2a(meta->arch));
	else
		(void)strlcpy(buf, pp, p->rmargin);

	pp = mdoc_msec2a(meta->msec);

	(void)snprintf(title, p->rmargin, "%s(%s)",
			meta->title, pp ? pp : "");

	p->offset = 0;
	p->rmargin = (p->maxrmargin - strlen(buf)) / 2;
	p->flags |= TERMP_NOBREAK | TERMP_NOSPACE;

	word(p, title);
	flushln(p);

	p->flags |= TERMP_NOLPAD | TERMP_NOSPACE;
	p->offset = p->rmargin;
	p->rmargin = p->maxrmargin - strlen(title);

	word(p, buf);
	flushln(p);

	p->offset = p->rmargin;
	p->rmargin = p->maxrmargin;
	p->flags &= ~TERMP_NOBREAK;
	p->flags |= TERMP_NOLPAD | TERMP_NOSPACE;

	word(p, title);
	flushln(p);

	p->rmargin = p->maxrmargin;
	p->offset = 0;
	p->flags &= ~TERMP_NOSPACE;

	free(title);
	free(buf);
}
