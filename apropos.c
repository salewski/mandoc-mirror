/*	$Id$ */
/*
 * Copyright (c) 2011 Kristaps Dzonsons <kristaps@bsd.lv>
 * Copyright (c) 2011 Ingo Schwarze <schwarze@openbsd.org>
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
#include <ctype.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "apropos_db.h"
#include "mandoc.h"

/*
 * FIXME: add support for manpath(1), which everybody but OpenBSD and
 * NetBSD seem to use.
 */
#define MAN_CONF_FILE	"/etc/man.conf"
#define MAN_CONF_KEY	"_whatdb"

/*
 * List of paths to be searched for manual databases.
 */
struct	manpaths {
	int	  sz;
	char	**paths;
};

static	int	 cmp(const void *, const void *);
static	void	 list(struct res *, size_t, void *);
static	void	 manpath_add(struct manpaths *, const char *);
static	void	 manpath_parse(struct manpaths *, char *);
static	void	 manpath_parseconf(struct manpaths *);
static	void	 usage(void);

static	char	*progname;

int
main(int argc, char *argv[])
{
	int		 i, ch, rc;
	struct manpaths	 paths;
	size_t		 terms;
	struct opts	 opts;
	struct expr	*e;
	char		*defpaths, *auxpaths;
	extern int	 optind;
	extern char	*optarg;

	progname = strrchr(argv[0], '/');
	if (progname == NULL)
		progname = argv[0];
	else
		++progname;

	memset(&paths, 0, sizeof(struct manpaths));
	memset(&opts, 0, sizeof(struct opts));

	auxpaths = defpaths = NULL;
	e = NULL;
	rc = 0;

	while (-1 != (ch = getopt(argc, argv, "M:m:S:s:"))) 
		switch (ch) {
		case ('M'):
			defpaths = optarg;
			break;
		case ('m'):
			auxpaths = optarg;
			break;
		case ('S'):
			opts.arch = optarg;
			break;
		case ('s'):
			opts.cat = optarg;
			break;
		default:
			usage();
			goto out;
		}

	argc -= optind;
	argv += optind;

	if (0 == argc) {
		rc = 1;
		goto out;
	}

	if (NULL != getenv("MANPATH"))
		defpaths = getenv("MANPATH");

	if (NULL == defpaths)
		manpath_parseconf(&paths);
	else
		manpath_parse(&paths, defpaths);

	manpath_parse(&paths, auxpaths);

	if (NULL == (e = exprcomp(argc, argv, &terms))) {
		fprintf(stderr, "%s: Bad expression\n", progname);
		goto out;
	}

	rc = apropos_search
		(paths.sz, paths.paths, 
		 &opts, e, terms, NULL, list);

	if (0 == rc) 
		fprintf(stderr, "%s: Error reading "
				"manual database\n", progname);

out:
	for (i = 0; i < paths.sz; i++)
		free(paths.paths[i]);

	free(paths.paths);
	exprfree(e);

	return(rc ? EXIT_SUCCESS : EXIT_FAILURE);
}

/* ARGSUSED */
static void
list(struct res *res, size_t sz, void *arg)
{
	int		 i;

	qsort(res, sz, sizeof(struct res), cmp);

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

	return(strcmp(((const struct res *)p1)->title,
				((const struct res *)p2)->title));
}

static void
usage(void)
{

	fprintf(stderr, "usage: %s "
			"[-M dirs] "
			"[-m dirs] "
			"[-S arch] "
			"[-s section] "
			"expression...\n", progname);
}

/*
 * Parse a FULL pathname from a colon-separated list of arrays.
 */
static void
manpath_parse(struct manpaths *dirs, char *path) 
{
	char	*dir;

	if (NULL == path)
		return;

	for (dir = strtok(path, ":"); dir; dir = strtok(NULL, ":"))
		manpath_add(dirs, dir);
}

/*
 * Add a directory to the array, ignoring bad directories.
 * Grow the array one-by-one for simplicity's sake.
 */
static void
manpath_add(struct manpaths *dirs, const char *dir) 
{
	char		 buf[PATH_MAX];
	char		*cp;
	int		 i;

	if (NULL == (cp = realpath(dir, buf)))
		return;

	for (i = 0; i < dirs->sz; i++)
		if (0 == strcmp(dirs->paths[i], dir))
			return;

	dirs->paths = mandoc_realloc
		(dirs->paths, 
		 ((size_t)dirs->sz + 1) * sizeof(char *));

	dirs->paths[dirs->sz++] = mandoc_strdup(cp);
}

static void
manpath_parseconf(struct manpaths *dirs) 
{
	FILE		*stream;
#ifdef	USE_MANPATH
	char		*buf;
	size_t		 sz, bsz;

	stream = popen("manpath", "r");
	if (NULL == stream)
		return;

	buf = NULL;
	bsz = 0;

	do {
		buf = mandoc_realloc(buf, bsz + 1024);
		sz = fread(buf + (int)bsz, 1, 1024, stream);
		bsz += sz;
	} while (sz > 0);

	assert(bsz && '\n' == buf[bsz - 1]);
	buf[bsz - 1] = '\0';

	manpath_parse(dirs, buf);
	free(buf);
	pclose(stream);
#else
	char		*p, *q;
	size_t	 	 len, keysz;

	keysz = strlen(MAN_CONF_KEY);
	assert(keysz > 0);

	if (NULL == (stream = fopen(MAN_CONF_FILE, "r")))
		return;

	while (NULL != (p = fgetln(stream, &len))) {
		if (0 == len || '\n' == p[--len])
			break;
		p[len] = '\0';
		while (isspace((unsigned char)*p))
			p++;
		if (strncmp(MAN_CONF_KEY, p, keysz))
			continue;
		p += keysz;
		while (isspace(*p))
			p++;
		if ('\0' == *p)
			continue;
		if (NULL == (q = strrchr(p, '/')))
			continue;
		*q = '\0';
		manpath_add(dirs, p);
	}

	fclose(stream);
#endif
}
