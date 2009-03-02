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
#include <sys/utsname.h>

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

static	void		  body(struct termp *,
				struct termpair *,
				const struct mdoc_meta *,
				const struct mdoc_node *);
static	void		  header(struct termp *,
				const struct mdoc_meta *);
static	void		  footer(struct termp *,
				const struct mdoc_meta *);

static	void		  pword(struct termp *, const char *, size_t);
static	void		  pescape(struct termp *, const char *, 
				size_t *, size_t);
static	void		  style(struct termp *, enum tstyle);
static	void		  nescape(struct termp *,
				const char *, size_t);
static	void		  chara(struct termp *, char);
static	void		  stringa(struct termp *, 
				const char *, size_t);
static	void		  symbola(struct termp *, enum tsym);

#ifdef __linux__
extern	size_t		  strlcat(char *, const char *, size_t);
extern	size_t		  strlcpy(char *, const char *, size_t);
#endif

static	struct termsym	  termsym_ansi[] = {
	{ "]", 1 },		/* TERMSYM_RBRACK */
	{ "[", 1 },		/* TERMSYM_LBRACK */
	{ "<-", 2 },		/* TERMSYM_LARROW */
	{ "->", 2 },		/* TERMSYM_RARROW */
	{ "^", 1 },		/* TERMSYM_UARROW */
	{ "v", 1 },		/* TERMSYM_DARROW */
	{ "`", 1 },		/* TERMSYM_LSQUOTE */
	{ "\'", 1 },		/* TERMSYM_RSQUOTE */
	{ "\'", 1 },		/* TERMSYM_SQUOTE */
	{ "``", 2 },		/* TERMSYM_LDQUOTE */
	{ "\'\'", 2 },		/* TERMSYM_RDQUOTE */
	{ "\"", 1 },		/* TERMSYM_DQUOTE */
	{ "<", 1 },		/* TERMSYM_LT */
	{ ">", 1 },		/* TERMSYM_GT */
	{ "<=", 2 },		/* TERMSYM_LE */
	{ ">=", 2 },		/* TERMSYM_GE */
	{ "==", 2 },		/* TERMSYM_EQ */
	{ "!=", 2 },		/* TERMSYM_NEQ */
	{ "\'", 1 },		/* TERMSYM_ACUTE */
	{ "`", 1 },		/* TERMSYM_GRAVE */
	{ "pi", 2 },		/* TERMSYM_PI */
	{ "+=", 2 },		/* TERMSYM_PLUSMINUS */
	{ "oo", 2 },		/* TERMSYM_INF */
	{ "infinity", 8 },	/* TERMSYM_INF2 */
	{ "NaN", 3 },		/* TERMSYM_NAN */
	{ "|", 1 },		/* TERMSYM_BAR */
	{ "o", 1 },		/* TERMSYM_BULLET */
	{ "&", 1 },		/* TERMSYM_AND */
	{ "|", 1 },		/* TERMSYM_OR */
};

static	const char	  ansi_clear[]  = { 27, '[', '0', 'm' };
static	const char	  ansi_bold[]  = { 27, '[', '1', 'm' };
static	const char	  ansi_under[]  = { 27, '[', '4', 'm' };

static	struct termsym	  termstyle_ansi[] = {
	{ ansi_clear, 4 },
	{ ansi_bold, 4 },
	{ ansi_under, 4 }
};


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

	termp.maxrmargin = 78; /* XXX */
	termp.rmargin = termp.maxrmargin;
	termp.maxcols = 1024;
	termp.offset = termp.col = 0;
	termp.flags = TERMP_NOSPACE;
	termp.symtab = termsym_ansi;
	termp.styletab = termstyle_ansi;

	if (NULL == (termp.buf = malloc(termp.maxcols)))
		err(1, "malloc");

	header(&termp, mdoc_meta(mdoc));
	body(&termp, NULL, mdoc_meta(mdoc), mdoc_node(mdoc));
	footer(&termp, mdoc_meta(mdoc));

	free(termp.buf);

	mmain_exit(p, 0);
	/* NOTREACHED */
}


/*
 * Flush a line of text.  A "line" is loosely defined as being something
 * that should be followed by a newline, regardless of whether it's
 * broken apart by newlines getting there.  A line can also be a
 * fragment of a columnar list.
 *
 * Specifically, a line is whatever's in p->buf of length p->col, which
 * is zeroed after this function returns.
 *
 * The variables TERMP_NOLPAD, TERMP_LITERAL and TERMP_NOBREAK are of
 * critical importance here.  Their behaviour follows:
 *
 *  - TERMP_NOLPAD: when beginning to write the line, don't left-pad the
 *    offset value.  This is useful when doing columnar lists where the
 *    prior column has right-padded.
 *
 *  - TERMP_LITERAL: don't break apart words.  Note that a long literal
 *    word will violate the right margin.
 *
 *  - TERMP_NOBREAK: this is the most important and is used when making
 *    columns.  In short: don't print a newline and instead pad to the
 *    right margin.  Used in conjunction with TERMP_NOLPAD.
 *
 *  In-line line breaking:
 *
 *  If TERMP_NOBREAK is specified and the line overruns the right
 *  margin, it will break and pad-right to the right margin after
 *  writing.  If maxrmargin is violated, it will break and continue
 *  writing from the right-margin, which will lead to the above
 *  scenario upon exit.
 *
 *  Otherwise, the line will break at the right margin.  Extremely long
 *  lines will cause the system to emit a warning (TODO: hyphenate, if
 *  possible).
 */
void
flushln(struct termp *p)
{
	size_t		 i, j, vsz, vis, maxvis, mmax, bp;

	/*
	 * First, establish the maximum columns of "visible" content.
	 * This is usually the difference between the right-margin and
	 * an indentation, but can be, for tagged lists or columns, a
	 * small set of values.
	 */

	assert(p->offset < p->rmargin);
	maxvis = p->rmargin - p->offset;
	mmax = p->maxrmargin - p->offset;
	bp = TERMP_NOBREAK & p->flags ? mmax : maxvis;
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
		 * (starting with the CSI) aren't counted.  A space
		 * generates a non-printing word, which is valid (the
		 * space is printed according to regular spacing rules).
		 */

		/* FIXME: make non-ANSI friendly. */

		/* LINTED */
		for (j = i, vsz = 0; j < p->col; j++) {
			if (isspace((int)p->buf[j]))
				break;
			else if (27 == p->buf[j]) {
				assert(j + 4 <= p->col);
				j += 3;
			} else
				vsz++;
		}

		/*
		 * Do line-breaking.  If we're greater than our
		 * break-point and already in-line, break to the next
		 * line and start writing.  If we're at the line start,
		 * then write out the word (TODO: hyphenate) and break
		 * in a subsequent loop invocation.
		 */

		if ( ! (TERMP_NOBREAK & p->flags)) {
			if (vis && vis + vsz > bp) {
				putchar('\n');
				for (j = 0; j < p->offset; j++)
					putchar(' ');
				vis = 0;
			} else if (vis + vsz > bp)
				warnx("word breaks right margin");

			/* TODO: hyphenate. */

		} else {
			if (vis && vis + vsz > bp) {
				putchar('\n');
				for (j = 0; j < p->rmargin; j++)
					putchar(' ');
				vis = p->rmargin;
			} else if (vis + vsz > bp) 
				warnx("word breaks right margin");

			/* TODO: hyphenate. */
		}

		/* 
		 * Write out the word and a trailing space.  Omit the
		 * space if we're the last word in the line or beyond
		 * our breakpoint.
		 */

		for ( ; i < p->col; i++) {
			if (isspace((int)p->buf[i]))
				break;
			putchar(p->buf[i]);
		}
		vis += vsz;
		if (i < p->col && vis <= bp) {
			putchar(' ');
			vis++;
		}
	}

	/*
	 * If we've overstepped our maximum visible no-break space, then
	 * cause a newline and offset at the right margin.
	 */

	if ((TERMP_NOBREAK & p->flags) && vis >= maxvis) {
		putchar('\n');
		for (i = 0; i < p->rmargin; i++)
			putchar(' ');
		p->col = 0;
		return;
	}

	/*
	 * If we're not to right-marginalise it (newline), then instead
	 * pad to the right margin and stay off.
	 */

	if (p->flags & TERMP_NOBREAK) {
		for ( ; vis < maxvis; vis++)
			putchar(' ');
	} else
		putchar('\n');

	p->col = 0;
}


/* 
 * A newline only breaks an existing line; it won't assert vertical
 * space.  All data in the output buffer is flushed prior to the newline
 * assertion.
 */
void
newln(struct termp *p)
{

	p->flags |= TERMP_NOSPACE;
	if (0 == p->col) {
		p->flags &= ~TERMP_NOLPAD;
		return;
	}
	flushln(p);
	p->flags &= ~TERMP_NOLPAD;
}


/*
 * Asserts a vertical space (a full, empty line-break between lines).
 * Note that if used twice, this will cause two blank spaces and so on.
 * All data in the output buffer is flushed prior to the newline
 * assertion.
 */
void
vspace(struct termp *p)
{

	newln(p);
	putchar('\n');
}


/*
 * Break apart a word into "pwords" (partial-words, usually from
 * breaking up a phrase into individual words) and, eventually, put them
 * into the output buffer.  If we're a literal word, then don't break up
 * the word and put it verbatim into the output buffer.
 */
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
		if ( ! isspace((int)word[i])) {
			j++;
			continue;
		} 
		
		/* Escaped spaces don't delimit... */
		if (i > 0 && isspace((int)word[i]) && 
				'\\' == word[i - 1]) {
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


/*
 * This is the main function for printing out nodes.  It's constituted
 * of PRE and POST functions, which correspond to prefix and infix
 * processing.  The termpair structure allows data to persist between
 * prefix and postfix invocations.
 */
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

	/*
	 * This is /slightly/ different from regular groff output
	 * because we don't have page numbers.  Print the following:
	 *
	 * OS                                            MDOCDATE
	 */

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
	char		*buf, *title, *bufp, *vbuf;
	const char	*pp;
	struct utsname	 uts;

	p->rmargin = p->maxrmargin;
	p->offset = 0;

	if (NULL == (buf = malloc(p->rmargin)))
		err(1, "malloc");
	if (NULL == (title = malloc(p->rmargin)))
		err(1, "malloc");
	if (NULL == (vbuf = malloc(p->rmargin)))
		err(1, "malloc");

	if (NULL == (pp = mdoc_vol2a(meta->vol))) {
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
			break;
		}
	}
	vbuf[0] = 0;

	if (pp) {
		if (-1 == uname(&uts)) 
			err(1, "uname");
		(void)strlcat(vbuf, uts.sysname, p->rmargin);
		(void)strlcat(vbuf, " ", p->rmargin);
	} else if (NULL == (pp = mdoc_msec2a(meta->msec)))
		pp = mdoc_msec2a(MSEC_local);

	(void)strlcat(vbuf, pp, p->rmargin);

	/*
	 * The header is strange.  It has three components, which are
	 * really two with the first duplicated.  It goes like this:
	 *
	 * IDENTIFIER              TITLE                   IDENTIFIER
	 *
	 * The IDENTIFIER is NAME(SECTION), which is the command-name
	 * (if given, or "unknown" if not) followed by the manual page
	 * section.  These are given in `Dt'.  The TITLE is a free-form
	 * string depending on the manual volume.  If not specified, it
	 * switches on the manual section.
	 */

	if (mdoc_arch2a(meta->arch))
		(void)snprintf(buf, p->rmargin, "%s (%s)",
				vbuf, mdoc_arch2a(meta->arch));
	else
		(void)strlcpy(buf, vbuf, p->rmargin);

	pp = mdoc_msec2a(meta->msec);

	(void)snprintf(title, p->rmargin, "%s(%s)",
			meta->title, pp ? pp : "");

	for (bufp = title; *bufp; bufp++)
		*bufp = toupper(*bufp);
	
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
	free(vbuf);
	free(buf);
}


/*
 * Determine the symbol indicated by an escape sequences, that is, one
 * starting with a backslash.  Once done, we pass this value into the
 * output buffer by way of the symbol table.
 */
static void
nescape(struct termp *p, const char *word, size_t len)
{

	switch (len) {
	case (1):
		switch (word[0]) {
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
			chara(p, word[0]); /* FIXME */
			break;
		case ('&'):
			break;
		case ('e'):
			chara(p, '\\'); /* FIXME */
			break;
		case ('q'):
			symbola(p, TERMSYM_DQUOTE);
			break;
		default:
			warnx("escape sequence not supported: %c",
					word[0]);
			break;
		}
		break;

	case (2):
		if ('r' == word[0] && 'B' == word[1])
			symbola(p, TERMSYM_RBRACK);
		else if ('l' == word[0] && 'B' == word[1])
			symbola(p, TERMSYM_LBRACK);
		else if ('l' == word[0] && 'q' == word[1])
			symbola(p, TERMSYM_LDQUOTE);
		else if ('r' == word[0] && 'q' == word[1])
			symbola(p, TERMSYM_RDQUOTE);
		else if ('o' == word[0] && 'q' == word[1])
			symbola(p, TERMSYM_LSQUOTE);
		else if ('a' == word[0] && 'q' == word[1])
			symbola(p, TERMSYM_RSQUOTE);
		else if ('<' == word[0] && '-' == word[1])
			symbola(p, TERMSYM_LARROW);
		else if ('-' == word[0] && '>' == word[1])
			symbola(p, TERMSYM_RARROW);
		else if ('b' == word[0] && 'u' == word[1])
			symbola(p, TERMSYM_BULLET);
		else if ('<' == word[0] && '=' == word[1])
			symbola(p, TERMSYM_LE);
		else if ('>' == word[0] && '=' == word[1])
			symbola(p, TERMSYM_GE);
		else if ('=' == word[0] && '=' == word[1])
			symbola(p, TERMSYM_EQ);
		else if ('+' == word[0] && '-' == word[1])
			symbola(p, TERMSYM_PLUSMINUS);
		else if ('u' == word[0] && 'a' == word[1])
			symbola(p, TERMSYM_UARROW);
		else if ('d' == word[0] && 'a' == word[1])
			symbola(p, TERMSYM_DARROW);
		else if ('a' == word[0] && 'a' == word[1])
			symbola(p, TERMSYM_ACUTE);
		else if ('g' == word[0] && 'a' == word[1])
			symbola(p, TERMSYM_GRAVE);
		else if ('!' == word[0] && '=' == word[1])
			symbola(p, TERMSYM_NEQ);
		else if ('i' == word[0] && 'f' == word[1])
			symbola(p, TERMSYM_INF);
		else if ('n' == word[0] && 'a' == word[1])
			symbola(p, TERMSYM_NAN);
		else if ('b' == word[0] && 'a' == word[1])
			symbola(p, TERMSYM_BAR);

		/* Deprecated forms. */
		else if ('A' == word[0] && 'm' == word[1])
			symbola(p, TERMSYM_AMP);
		else if ('B' == word[0] && 'a' == word[1])
			symbola(p, TERMSYM_BAR);
		else if ('I' == word[0] && 'f' == word[1])
			symbola(p, TERMSYM_INF2);
		else if ('G' == word[0] && 'e' == word[1])
			symbola(p, TERMSYM_GE);
		else if ('G' == word[0] && 't' == word[1])
			symbola(p, TERMSYM_GT);
		else if ('L' == word[0] && 'e' == word[1])
			symbola(p, TERMSYM_LE);
		else if ('L' == word[0] && 'q' == word[1])
			symbola(p, TERMSYM_LDQUOTE);
		else if ('L' == word[0] && 't' == word[1])
			symbola(p, TERMSYM_LT);
		else if ('N' == word[0] && 'a' == word[1])
			symbola(p, TERMSYM_NAN);
		else if ('N' == word[0] && 'e' == word[1])
			symbola(p, TERMSYM_NEQ);
		else if ('P' == word[0] && 'i' == word[1])
			symbola(p, TERMSYM_PI);
		else if ('P' == word[0] && 'm' == word[1])
			symbola(p, TERMSYM_PLUSMINUS);
		else if ('R' == word[0] && 'q' == word[1])
			symbola(p, TERMSYM_RDQUOTE);
		else
			warnx("escape sequence not supported: %c%c",
					word[0], word[1]);
		break;

	default:
		warnx("escape sequence not supported");
		break;
	}
}


/*
 * Apply a style to the output buffer.  This is looked up by means of
 * the styletab.
 */
static void
style(struct termp *p, enum tstyle esc)
{

	if (p->col + 4 >= p->maxcols)
		errx(1, "line overrun");

	p->buf[(p->col)++] = 27;
	p->buf[(p->col)++] = '[';
	switch (esc) {
	case (TERMSTYLE_CLEAR):
		p->buf[(p->col)++] = '0';
		break;
	case (TERMSTYLE_BOLD):
		p->buf[(p->col)++] = '1';
		break;
	case (TERMSTYLE_UNDER):
		p->buf[(p->col)++] = '4';
		break;
	default:
		abort();
		/* NOTREACHED */
	}
	p->buf[(p->col)++] = 'm';
}


/*
 * Handle an escape sequence: determine its length and pass it to the
 * escape-symbol look table.  Note that we assume mdoc(3) has validated
 * the escape sequence (we assert upon badly-formed escape sequences).
 */
static void
pescape(struct termp *p, const char *word, size_t *i, size_t len)
{
	size_t		 j;

	(*i)++;
	assert(*i < len);

	if ('(' == word[*i]) {
		(*i)++;
		assert(*i + 1 < len);
		nescape(p, &word[*i], 2);
		(*i)++;
		return;

	} else if ('*' == word[*i]) { 
		/* XXX - deprecated! */
		(*i)++;
		assert(*i < len);
		switch (word[*i]) {
		case ('('):
			(*i)++;
			assert(*i + 1 < len);
			nescape(p, &word[*i], 2);
			(*i)++;
			return;
		case ('['):
			break;
		default:
			nescape(p, &word[*i], 1);
			return;
		}

	} else if ('[' != word[*i]) {
		nescape(p, &word[*i], 1);
		return;
	}

	(*i)++;
	for (j = 0; word[*i] && ']' != word[*i]; (*i)++, j++)
		/* Loop... */ ;

	assert(word[*i]);
	nescape(p, &word[*i - j], j);
}


/*
 * Handle pwords, partial words, which may be either a single word or a
 * phrase that cannot be broken down (such as a literal string).  This
 * handles word styling.
 */
static void
pword(struct termp *p, const char *word, size_t len)
{
	size_t		 i;

	if ( ! (TERMP_NOSPACE & p->flags) && 
			! (TERMP_LITERAL & p->flags))
		chara(p, ' ');

	if ( ! (p->flags & TERMP_NONOSPACE))
		p->flags &= ~TERMP_NOSPACE;

	/* 
	 * XXX - if literal and underlining, this will underline the
	 * spaces between literal words. 
	 */

	if (p->flags & TERMP_BOLD)
		style(p, TERMSTYLE_BOLD);
	if (p->flags & TERMP_UNDERLINE)
		style(p, TERMSTYLE_UNDER);

	for (i = 0; i < len; i++) {
		if ('\\' == word[i]) {
			pescape(p, word, &i, len);
			continue;
		}
		chara(p, word[i]);
	}

	if (p->flags & TERMP_BOLD ||
			p->flags & TERMP_UNDERLINE)
		style(p, TERMSTYLE_CLEAR);
}


/*
 * Add a symbol to the output line buffer.
 */
static void
symbola(struct termp *p, enum tsym sym)
{

	assert(p->symtab[sym].sym);
	stringa(p, p->symtab[sym].sym, p->symtab[sym].sz);
}


/*
 * Like chara() but for arbitrary-length buffers.  Resize the buffer by
 * a factor of two (if the buffer is less than that) or the buffer's
 * size.
 */
static void
stringa(struct termp *p, const char *c, size_t sz)
{
	size_t		 s;

	s = sz > p->maxcols * 2 ? sz : p->maxcols * 2;
	
	assert(c);
	if (p->col + sz >= p->maxcols) {
		p->buf = realloc(p->buf, s);
		if (NULL == p->buf)
			err(1, "realloc");
		p->maxcols = s;
	}

	(void)memcpy(&p->buf[p->col], c, sz);
	p->col += sz;
}


/*
 * Insert a single character into the line-buffer.  If the buffer's
 * space is exceeded, then allocate more space by doubling the buffer
 * size.
 */
static void
chara(struct termp *p, char c)
{

	if (p->col + 1 >= p->maxcols) {
		p->buf = realloc(p->buf, p->maxcols * 2);
		if (NULL == p->buf)
			err(1, "malloc");
		p->maxcols *= 2;
	}
	p->buf[(p->col)++] = c;
}
