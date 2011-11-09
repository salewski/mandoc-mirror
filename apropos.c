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
#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apropos.h"
#include "mandoc.h"

struct	type {
	int		 mask;
	const char	*name; /* command-line type name */
};

static	const struct type types[] = {
	{ TYPE_NAME, "name" },
	{ TYPE_FUNCTION, "func" },
	{ TYPE_UTILITY, "utility" },
	{ TYPE_INCLUDES, "incl" },
	{ TYPE_VARIABLE, "var" },
	{ TYPE_STANDARD, "stand" },
	{ TYPE_AUTHOR, "auth" },
	{ TYPE_CONFIG, "conf" },
	{ TYPE_DESC, "desc" },
	{ TYPE_XREF, "xref" },
	{ TYPE_PATH, "path" },
	{ TYPE_ENV, "env" },
	{ TYPE_ERR, "err" },
	{ INT_MAX, "all" },
	{ 0, NULL }
};

static	int	 cmp(const void *, const void *);
static	void	 list(struct rec *, size_t, void *);
static	void	 usage(void);

static	char	*progname;

int
main(int argc, char *argv[])
{
	int		 ch, i;
	char		*q, *v;
	struct opts	 opts;
	extern int	 optind;
	extern char	*optarg;

	memset(&opts, 0, sizeof(struct opts));

	q = NULL;

	progname = strrchr(argv[0], '/');
	if (progname == NULL)
		progname = argv[0];
	else
		++progname;

	while (-1 != (ch = getopt(argc, argv, "a:c:I:t:"))) 
		switch (ch) {
		case ('a'):
			opts.arch = optarg;
			break;
		case ('c'):
			opts.cat = optarg;
			break;
		case ('I'):
			opts.flags |= OPTS_INSENS;
			break;
		case ('t'):
			while (NULL != (v = strsep(&optarg, ","))) {
				if ('\0' == *v)
					continue;
				for (i = 0; types[i].mask; i++) {
					if (strcmp(types[i].name, v))
						continue;
					break;
				}
				if (0 == types[i].mask)
					break;
				opts.types |= types[i].mask;
			}
			if (NULL == v)
				break;
			
			fprintf(stderr, "%s: Bad type\n", v);
			return(EXIT_FAILURE);
		default:
			usage();
			return(EXIT_FAILURE);
		}

	argc -= optind;
	argv += optind;

	if (0 == argc || '\0' == **argv) {
		usage();
		return(EXIT_SUCCESS);
	} else
		q = *argv;

	if (0 == opts.types)
		opts.types = TYPE_NAME | TYPE_DESC;

	/*
	 * Configure databases.
	 * The keyword database is a btree that allows for duplicate
	 * entries.
	 * The index database is a recno.
	 */

	apropos_search(&opts, q, NULL, list);
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
			"[-a arch] "
			"[-c cat] "
			"[-t type[,...]] "
			"key\n", progname);
}
