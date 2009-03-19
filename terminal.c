/* $Id$ */
/*
 * Copyright (c) 2008, 2009 Kristaps Dzonsons <kristaps@openbsd.org>
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
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "term.h"

#ifdef __linux__
extern	size_t		  strlcpy(char *, const char *, size_t);
extern	size_t		  strlcat(char *, const char *, size_t);
#endif

static	struct termp	 *termp_alloc(enum termenc);
static	void		  termp_free(struct termp *);
static	void		  termp_body(struct termp *, struct termpair *,
				const struct mdoc_meta *,
				const struct mdoc_node *);
static	void		  termp_head(struct termp *,
				const struct mdoc_meta *);
static	void		  termp_foot(struct termp *,
				const struct mdoc_meta *);
static	void		  termp_pword(struct termp *, const char *, int);
static	void		  termp_pescape(struct termp *, 
				const char *, int *, int);
static	void		  termp_nescape(struct termp *,
				const char *, size_t);
static	void		  termp_chara(struct termp *, char);
static	void		  termp_stringa(struct termp *, 
				const char *, size_t);
static	void		  sanity(const struct mdoc_node *); /* XXX */


void *
latin1_alloc(void)
{

	return(termp_alloc(TERMENC_LATIN1));
}


void *
utf8_alloc(void)
{

	return(termp_alloc(TERMENC_UTF8));
}


void *
ascii_alloc(void)
{

	return(termp_alloc(TERMENC_ASCII));
}


int
terminal_run(void *arg, const struct mdoc *mdoc)
{
	struct termp	*p;

	p = (struct termp *)arg;

	if (NULL == p->symtab)
		p->symtab = ascii2htab();

	termp_head(p, mdoc_meta(mdoc));
	termp_body(p, NULL, mdoc_meta(mdoc), mdoc_node(mdoc));
	termp_foot(p, mdoc_meta(mdoc));

	return(1);
}


void
terminal_free(void *arg)
{

	termp_free((struct termp *)arg);
}


static void
termp_free(struct termp *p)
{

	if (p->buf)
		free(p->buf);
	if (TERMENC_ASCII == p->enc && p->symtab)
		asciifree(p->symtab);

	free(p);
}


static struct termp *
termp_alloc(enum termenc enc)
{
	struct termp *p;

	if (NULL == (p = malloc(sizeof(struct termp))))
		err(1, "malloc");
	bzero(p, sizeof(struct termp));
	p->maxrmargin = 78;
	p->enc = enc;
	return(p);
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
 *  - TERMP_NOBREAK: this is the most important and is used when making
 *    columns.  In short: don't print a newline and instead pad to the
 *    right margin.  Used in conjunction with TERMP_NOLPAD.
 *
 *  - TERMP_NONOBREAK: don't newline when TERMP_NOBREAK is specified.
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
	int		 i, j;
	size_t		 vsz, vis, maxvis, mmax, bp;

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
		for (j = 0; j < (int)p->offset; j++)
			putchar(' ');

	for (i = 0; i < (int)p->col; i++) {
		/*
		 * Count up visible word characters.  Control sequences
		 * (starting with the CSI) aren't counted.  A space
		 * generates a non-printing word, which is valid (the
		 * space is printed according to regular spacing rules).
		 */

		/* LINTED */
		for (j = i, vsz = 0; j < (int)p->col; j++) {
			if (' ' == p->buf[j])
				break;
			else if (8 == p->buf[j])
				j += 1;
			else
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
				for (j = 0; j < (int)p->offset; j++)
					putchar(' ');
				vis = 0;
			} else if (vis + vsz > bp)
				warnx("word breaks right margin");

			/* TODO: hyphenate. */

		} else {
			if (vis && vis + vsz > bp) {
				putchar('\n');
				for (j = 0; j < (int)p->rmargin; j++)
					putchar(' ');
				vis = p->rmargin - p->offset;
			} else if (vis + vsz > bp) 
				warnx("word breaks right margin");

			/* TODO: hyphenate. */
		}

		/* 
		 * Write out the word and a trailing space.  Omit the
		 * space if we're the last word in the line or beyond
		 * our breakpoint.
		 */

		for ( ; i < (int)p->col; i++) {
			if (' ' == p->buf[i])
				break;
			putchar(p->buf[i]);
		}
		vis += vsz;
		if (i < (int)p->col && vis <= bp) {
			putchar(' ');
			vis++;
		}
	}

	/*
	 * If we've overstepped our maximum visible no-break space, then
	 * cause a newline and offset at the right margin.
	 */

	if ((TERMP_NOBREAK & p->flags) && vis >= maxvis) {
		if ( ! (TERMP_NONOBREAK & p->flags)) {
			putchar('\n');
			for (i = 0; i < (int)p->rmargin; i++)
				putchar(' ');
		}
		p->col = 0;
		return;
	}

	/*
	 * If we're not to right-marginalise it (newline), then instead
	 * pad to the right margin and stay off.
	 */

	if (p->flags & TERMP_NOBREAK) {
		if ( ! (TERMP_NONOBREAK & p->flags))
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
	int 		 i, j, len;

	if (p->flags & TERMP_LITERAL) {
		termp_pword(p, word, (int)strlen(word));
		return;
	}

	if (0 == (len = (int)strlen(word)))
		errx(1, "blank line not in literal context");

	if (mdoc_isdelim(word)) {
		if ( ! (p->flags & TERMP_IGNDELIM))
			p->flags |= TERMP_NOSPACE;
		p->flags &= ~TERMP_IGNDELIM;
	}

	/* LINTED */
	for (j = i = 0; i < len; i++) {
		if (' ' != word[i]) {
			j++;
			continue;
		} 
		
		/* Escaped spaces don't delimit... */
		if (i && ' ' == word[i] && '\\' == word[i - 1]) {
			j++;
			continue;
		}

		if (0 == j)
			continue;
		assert(i >= j);
		termp_pword(p, &word[i - j], j);
		j = 0;
	}
	if (j > 0) {
		assert(i >= j);
		termp_pword(p, &word[i - j], j);
	}
}


/*
 * This is the main function for printing out nodes.  It's constituted
 * of PRE and POST functions, which correspond to prefix and infix
 * processing.  The termpair structure allows data to persist between
 * prefix and postfix invocations.
 */
static void
termp_body(struct termp *p, struct termpair *ppair,
		const struct mdoc_meta *meta,
		const struct mdoc_node *node)
{
	int		 dochild;
	struct termpair	 pair;

	/* Some quick sanity-checking. */

	sanity(node);

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
		word(p, node->string);

	/* Children. */

	if (TERMPAIR_FLAG & pair.type)
		p->flags |= pair.flag;

	if (dochild && node->child)
		termp_body(p, &pair, meta, node->child);

	if (TERMPAIR_FLAG & pair.type)
		p->flags &= ~pair.flag;

	/* Post-processing. */

	if (MDOC_TEXT != node->type)
		if (termacts[node->tok].post)
			(*termacts[node->tok].post)(p, &pair, meta, node);

	/* Siblings. */

	if (node->next)
		termp_body(p, ppair, meta, node->next);
}


static void
termp_foot(struct termp *p, const struct mdoc_meta *meta)
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
termp_head(struct termp *p, const struct mdoc_meta *meta)
{
	char		*buf, *title;

	p->rmargin = p->maxrmargin;
	p->offset = 0;

	if (NULL == (buf = malloc(p->rmargin)))
		err(1, "malloc");
	if (NULL == (title = malloc(p->rmargin)))
		err(1, "malloc");

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

	assert(meta->vol);
	(void)strlcpy(buf, meta->vol, p->rmargin);

	if (meta->arch) {
		(void)strlcat(buf, " (", p->rmargin);
		(void)strlcat(buf, meta->arch, p->rmargin);
		(void)strlcat(buf, ")", p->rmargin);
	}

	(void)snprintf(title, p->rmargin, "%s(%d)", 
			meta->title, meta->msec);

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


/*
 * Determine the symbol indicated by an escape sequences, that is, one
 * starting with a backslash.  Once done, we pass this value into the
 * output buffer by way of the symbol table.
 */
static void
termp_nescape(struct termp *p, const char *word, size_t len)
{
	const char	*rhs;
	size_t		 sz;

	if (NULL == (rhs = a2ascii(p->symtab, word, len, &sz))) 
		return;
	termp_stringa(p, rhs, sz);
}


/*
 * Handle an escape sequence: determine its length and pass it to the
 * escape-symbol look table.  Note that we assume mdoc(3) has validated
 * the escape sequence (we assert upon badly-formed escape sequences).
 */
static void
termp_pescape(struct termp *p, const char *word, int *i, int len)
{
	int		 j;

	if (++(*i) >= len)
		return;

	if ('(' == word[*i]) {
		(*i)++;
		if (*i + 1 >= len)
			return;

		termp_nescape(p, &word[*i], 2);
		(*i)++;
		return;

	} else if ('*' == word[*i]) { 
		(*i)++;
		if (*i >= len)
			return;

		switch (word[*i]) {
		case ('('):
			(*i)++;
			if (*i + 1 >= len)
				return;

			termp_nescape(p, &word[*i], 2);
			(*i)++;
			return;
		case ('['):
			break;
		default:
			termp_nescape(p, &word[*i], 1);
			return;
		}

	} else if ('[' != word[*i]) {
		termp_nescape(p, &word[*i], 1);
		return;
	}

	(*i)++;
	for (j = 0; word[*i] && ']' != word[*i]; (*i)++, j++)
		/* Loop... */ ;

	if (0 == word[*i])
		return;

	termp_nescape(p, &word[*i - j], (size_t)j);
}


/*
 * Handle pwords, partial words, which may be either a single word or a
 * phrase that cannot be broken down (such as a literal string).  This
 * handles word styling.
 */
static void
termp_pword(struct termp *p, const char *word, int len)
{
	int		 i;

	if ( ! (TERMP_NOSPACE & p->flags) && 
			! (TERMP_LITERAL & p->flags))
		termp_chara(p, ' ');

	if ( ! (p->flags & TERMP_NONOSPACE))
		p->flags &= ~TERMP_NOSPACE;

	/* 
	 * If ANSI (word-length styling), then apply our style now,
	 * before the word.
	 */

	for (i = 0; i < len; i++) {
		if ('\\' == word[i]) {
			termp_pescape(p, word, &i, len);
			continue;
		}

		if (TERMP_STYLE & p->flags) {
			if (TERMP_BOLD & p->flags) {
				termp_chara(p, word[i]);
				termp_chara(p, 8);
			}
			if (TERMP_UNDER & p->flags) {
				termp_chara(p, '_');
				termp_chara(p, 8);
			}
		}

		termp_chara(p, word[i]);
	}
}


/*
 * Like termp_chara() but for arbitrary-length buffers.  Resize the
 * buffer by a factor of two (if the buffer is less than that) or the
 * buffer's size.
 */
static void
termp_stringa(struct termp *p, const char *c, size_t sz)
{
	size_t		 s;

	if (0 == sz)
		return;

	assert(c);
	if (p->col + sz >= p->maxcols) {
		if (0 == p->maxcols)
			p->maxcols = 256;
		s = sz > p->maxcols * 2 ? sz : p->maxcols * 2;
		p->buf = realloc(p->buf, s);
		if (NULL == p->buf)
			err(1, "realloc");
		p->maxcols = s;
	}

	(void)memcpy(&p->buf[(int)p->col], c, sz);
	p->col += sz;
}


/*
 * Insert a single character into the line-buffer.  If the buffer's
 * space is exceeded, then allocate more space by doubling the buffer
 * size.
 */
static void
termp_chara(struct termp *p, char c)
{
	size_t		 s;

	if (p->col + 1 >= p->maxcols) {
		if (0 == p->maxcols)
			p->maxcols = 256;
		s = p->maxcols * 2;
		p->buf = realloc(p->buf, s);
		if (NULL == p->buf)
			err(1, "realloc");
		p->maxcols = s;
	}
	p->buf[(int)(p->col)++] = c;
}


static void
sanity(const struct mdoc_node *n)
{

	switch (n->type) {
	case (MDOC_TEXT):
		if (n->child) 
			errx(1, "regular form violated (1)");
		if (NULL == n->parent) 
			errx(1, "regular form violated (2)");
		if (NULL == n->string)
			errx(1, "regular form violated (3)");
		switch (n->parent->type) {
		case (MDOC_TEXT):
			/* FALLTHROUGH */
		case (MDOC_ROOT):
			errx(1, "regular form violated (4)");
			/* NOTREACHED */
		default:
			break;
		}
		break;
	case (MDOC_ELEM):
		if (NULL == n->parent)
			errx(1, "regular form violated (5)");
		switch (n->parent->type) {
		case (MDOC_TAIL):
			/* FALLTHROUGH */
		case (MDOC_BODY):
			/* FALLTHROUGH */
		case (MDOC_HEAD):
			break;
		default:
			errx(1, "regular form violated (6)");
			/* NOTREACHED */
		}
		if (n->child) switch (n->child->type) {
		case (MDOC_TEXT):
			break;
		default:
			errx(1, "regular form violated (7(");
			/* NOTREACHED */
		}
		break;
	case (MDOC_HEAD):
		/* FALLTHROUGH */
	case (MDOC_BODY):
		/* FALLTHROUGH */
	case (MDOC_TAIL):
		if (NULL == n->parent)
			errx(1, "regular form violated (8)");
		if (MDOC_BLOCK != n->parent->type)
			errx(1, "regular form violated (9)");
		if (n->child) switch (n->child->type) {
		case (MDOC_BLOCK):
			/* FALLTHROUGH */
		case (MDOC_ELEM):
			/* FALLTHROUGH */
		case (MDOC_TEXT):
			break;
		default:
			errx(1, "regular form violated (a)");
			/* NOTREACHED */
		}
		break;
	case (MDOC_BLOCK):
		if (NULL == n->parent)
			errx(1, "regular form violated (b)");
		if (NULL == n->child)
			errx(1, "regular form violated (c)");
		switch (n->parent->type) {
		case (MDOC_ROOT):
			/* FALLTHROUGH */
		case (MDOC_HEAD):
			/* FALLTHROUGH */
		case (MDOC_BODY):
			/* FALLTHROUGH */
		case (MDOC_TAIL):
			break;
		default:
			errx(1, "regular form violated (d)");
			/* NOTREACHED */
		}
		switch (n->child->type) {
		case (MDOC_ROOT):
			/* FALLTHROUGH */
		case (MDOC_ELEM):
			errx(1, "regular form violated (e)");
			/* NOTREACHED */
		default:
			break;
		}
		break;
	case (MDOC_ROOT):
		if (n->parent)
			errx(1, "regular form violated (f)");
		if (NULL == n->child)
			errx(1, "regular form violated (10)");
		switch (n->child->type) {
		case (MDOC_BLOCK):
			break;
		default:
			errx(1, "regular form violated (11)");
			/* NOTREACHED */
		}
		break;
	}
}
