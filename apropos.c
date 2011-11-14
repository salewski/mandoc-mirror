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
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apropos_db.h"
#include "mandoc.h"

static	int	 cmp(const void *, const void *);
static	void	 list(struct rec *, size_t, void *);
static	void	 usage(void);

static	char	*progname;

int
main(int argc, char *argv[])
{
	int		 ch;
	size_t		 sz;
	char		*buf;
	struct opts	 opts;
	struct expr	*e;
	extern int	 optind;
	extern char	*optarg;

	memset(&opts, 0, sizeof(struct opts));

	progname = strrchr(argv[0], '/');
	if (progname == NULL)
		progname = argv[0];
	else
		++progname;

	while (-1 != (ch = getopt(argc, argv, "S:s:"))) 
		switch (ch) {
		case ('S'):
			opts.arch = optarg;
			break;
		case ('s'):
			opts.cat = optarg;
			break;
		default:
			usage();
			return(EXIT_FAILURE);
		}

	argc -= optind;
	argv += optind;

	if (0 == argc) 
		return(EXIT_SUCCESS);

	/* 
	 * Collapse expressions into a single string.  
	 * First count up the contained strings, adding a space at the
	 * end of each (plus nil-terminator).  Then merge.
	 */

	for (sz = 0, ch = 0; ch < argc; ch++)
		sz += strlen(argv[ch]) + 1;

	buf = mandoc_malloc(++sz);

	for (*buf = '\0', ch = 0; ch < argc; ch++) {
		strlcat(buf, argv[ch], sz);
		strlcat(buf, " ", sz);
	}

	buf[sz - 2] = '\0';

	if (NULL == (e = exprcomp(buf))) {
		fprintf(stderr, "Bad expression\n");
		free(buf);
		return(EXIT_FAILURE);
	}

	free(buf);

	/*
	 * Configure databases.
	 * The keyword database is a btree that allows for duplicate
	 * entries.
	 * The index database is a recno.
	 */

	apropos_search(&opts, e, NULL, list);
	exprfree(e);
	return(EXIT_SUCCESS);
}

/* ARGSUSED */
static void
list(struct rec *res, size_t sz, void *arg)
{
	int		 i;

	qsort(res, sz, sizeof(struct rec), cmp);

	for (i = 0; i < (int)sz; i++)
		printf("%s(%s%s%s) - %s\n", res[i].title, 
				res[i].cat, 
				*res[i].arch ? "/" : "",
				*res[i].arch ? res[i].arch : "",
				res[i].desc);
}

static int
cmp(const void *p1, const void *p2)
{

	return(strcmp(((const struct rec *)p1)->title,
				((const struct rec *)p2)->title));
}

static void
usage(void)
{

	fprintf(stderr, "usage: %s "
			"[-I] "
			"[-S arch] "
			"[-s section] "
			"EXPR\n", 
			progname);
}
