/*	$Id$ */
/*
 * Copyright (c) 2011 Kristaps Dzonsons <kristaps@bsd.lv>
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
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "man.h"
#include "mdoc.h"
#include "mandoc.h"

static	void	 pline(int, int *, int *);
static	void	 pman(const struct man_node *, int *, int *);
static	void	 pmandoc(struct mparse *, int, const char *);
static	void	 pmdoc(const struct mdoc_node *, int *, int *);
static	void	 pstring(const char *, int, int *);
static	void	 usage(void);

static	const char	 *progname;

int
main(int argc, char *argv[])
{
	struct mparse	*mp;
	int		 ch, i;
	extern int	 optind;
	extern char	*optarg;

	progname = strrchr(argv[0], '/');
	if (progname == NULL)
		progname = argv[0];
	else
		++progname;

	mp = NULL;

	while (-1 != (ch = getopt(argc, argv, "")))
		switch (ch) {
		default:
			usage();
			return((int)MANDOCLEVEL_BADARG);
		}

	argc -= optind;
	argv += optind;

	mp = mparse_alloc(MPARSE_AUTO, MANDOCLEVEL_FATAL, NULL, NULL);
	assert(mp);

	if (0 == argc)
		pmandoc(mp, STDIN_FILENO, "<stdin>");

	for (i = 0; i < argc; i++) {
		mparse_reset(mp);
		pmandoc(mp, -1, argv[i]);
	}

	mparse_free(mp);
	return(MANDOCLEVEL_OK);
}

static void
usage(void)
{

	fprintf(stderr, "usage: %s [files...]\n", progname);
}

static void
pmandoc(struct mparse *mp, int fd, const char *fn)
{
	struct mdoc	*mdoc;
	struct man	*man;
	int		 line, col;

	if (mparse_readfd(mp, fd, fn) >= MANDOCLEVEL_FATAL) {
		fprintf(stderr, "%s: Parse failure\n", fn);
		return;
	}

	mparse_result(mp, &mdoc, &man);
	line = 1;
	col = 0;

	if (mdoc) 
		pmdoc(mdoc_node(mdoc), &line, &col);
	else if (man)
		pman(man_node(man), &line, &col);
	else
		return;

	putchar('\n');
}

/*
 * Strip the escapes out of a string, emitting the results.
 */
static void
pstring(const char *p, int col, int *colp)
{
	enum mandoc_esc	 esc;

	while (*colp < col) {
		putchar(' ');
		(*colp)++;
	}

	while ('\0' != *p) {
		if ('\\' == *p) {
			p++;
			esc = mandoc_escape(&p, NULL, NULL);
			if (ESCAPE_ERROR == esc)
				return;
		} else {
			putchar(*p++);
			(*colp)++;
		}
	}
}

/*
 * Emit lines until we're in sync with our input.
 */
static void
pline(int line, int *linep, int *col)
{

	while (*linep < line) {
		putchar('\n');
		(*linep)++;
	}
	*col = 0;
}

static void
pmdoc(const struct mdoc_node *p, int *line, int *col)
{

	for ( ; p; p = p->next) {
		if (MDOC_LINE & p->flags)
			pline(p->line, line, col);
		if (MDOC_TEXT == p->type)
			pstring(p->string, p->pos, col);
		if (p->child) 
			pmdoc(p->child, line, col);
	}
}

static void
pman(const struct man_node *p, int *line, int *col)
{

	for ( ; p; p = p->next) {
		if (MAN_LINE & p->flags)
			pline(p->line, line, col);
		if (MAN_TEXT == p->type)
			pstring(p->string, p->pos, col);
		if (p->child) 
			pman(p->child, line, col);
	}
}
