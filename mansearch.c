/*	$Id$ */
/*
 * Copyright (c) 2012 Kristaps Dzonsons <kristaps@bsd.lv>
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

#include <sys/param.h>

#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_OHASH
#include <ohash.h>
#else
#include "compat_ohash.h"
#endif
#include <sqlite3.h>

#include "mandoc.h"
#include "manpath.h"
#include "mandocdb.h"
#include "mansearch.h"

#define	SQL_BIND_TEXT(_db, _s, _i, _v) \
	if (SQLITE_OK != sqlite3_bind_text \
		((_s), (_i)++, (_v), -1, SQLITE_STATIC)) \
		fprintf(stderr, "%s\n", sqlite3_errmsg((_db)))
#define	SQL_BIND_INT64(_db, _s, _i, _v) \
	if (SQLITE_OK != sqlite3_bind_int64 \
		((_s), (_i)++, (_v))) \
		fprintf(stderr, "%s\n", sqlite3_errmsg((_db)))

struct	expr {
	int		 glob; /* is glob? */
	uint64_t 	 bits; /* type-mask */
	const char	*v; /* search value */
	struct expr	*next; /* next in sequence */
};

struct	match {
	uint64_t	 id; /* identifier in database */
	char		*file; /* relative filepath of manpage */
	char		*desc; /* description of manpage */
	int		 form; /* 0 == catpage */
};

struct	type {
	uint64_t	 bits;
	const char	*name;
};

static	const struct type types[] = {
	{ TYPE_An,  "An" },
	{ TYPE_Ar,  "Ar" },
	{ TYPE_At,  "At" },
	{ TYPE_Bsx, "Bsx" },
	{ TYPE_Bx,  "Bx" },
	{ TYPE_Cd,  "Cd" },
	{ TYPE_Cm,  "Cm" },
	{ TYPE_Dv,  "Dv" },
	{ TYPE_Dx,  "Dx" },
	{ TYPE_Em,  "Em" },
	{ TYPE_Er,  "Er" },
	{ TYPE_Ev,  "Ev" },
	{ TYPE_Fa,  "Fa" },
	{ TYPE_Fl,  "Fl" },
	{ TYPE_Fn,  "Fn" },
	{ TYPE_Fn,  "Fo" },
	{ TYPE_Ft,  "Ft" },
	{ TYPE_Fx,  "Fx" },
	{ TYPE_Ic,  "Ic" },
	{ TYPE_In,  "In" },
	{ TYPE_Lb,  "Lb" },
	{ TYPE_Li,  "Li" },
	{ TYPE_Lk,  "Lk" },
	{ TYPE_Ms,  "Ms" },
	{ TYPE_Mt,  "Mt" },
	{ TYPE_Nd,  "Nd" },
	{ TYPE_Nm,  "Nm" },
	{ TYPE_Nx,  "Nx" },
	{ TYPE_Ox,  "Ox" },
	{ TYPE_Pa,  "Pa" },
	{ TYPE_Rs,  "Rs" },
	{ TYPE_Sh,  "Sh" },
	{ TYPE_Ss,  "Ss" },
	{ TYPE_St,  "St" },
	{ TYPE_Sy,  "Sy" },
	{ TYPE_Tn,  "Tn" },
	{ TYPE_Va,  "Va" },
	{ TYPE_Va,  "Vt" },
	{ TYPE_Xr,  "Xr" },
	{ ~0ULL,    "any" },
	{ 0ULL, NULL }
};

static	void		*hash_alloc(size_t, void *);
static	void		 hash_free(void *, size_t, void *);
static	void		*hash_halloc(size_t, void *);
static	struct expr	*exprcomp(int, char *[]);
static	void		 exprfree(struct expr *);
static	struct expr	*exprterm(char *);
static	char		*sql_statement(const struct expr *,
				const char *, const char *);

int
mansearch(const struct manpaths *paths, 
		const char *arch, const char *sec,
		int argc, char *argv[], 
		struct manpage **res, size_t *sz)
{
	int		 fd, rc, c;
	int64_t		 id;
	char		 buf[MAXPATHLEN];
	char		*sql;
	struct expr	*e, *ep;
	sqlite3		*db;
	sqlite3_stmt	*s;
	struct match	*mp;
	struct ohash_info info;
	struct ohash	 htab;
	unsigned int	 idx;
	size_t		 i, j, cur, maxres;

	memset(&info, 0, sizeof(struct ohash_info));

	info.halloc = hash_halloc;
	info.alloc = hash_alloc;
	info.hfree = hash_free;
	info.key_offset = offsetof(struct match, id);

	*sz = cur = maxres = 0;
	sql = NULL;
	*res = NULL;
	fd = -1;
	e = NULL;
	rc = 0;

	if (0 == argc)
		goto out;
	if (NULL == (e = exprcomp(argc, argv)))
		goto out;

	/*
	 * Save a descriptor to the current working directory.
	 * Since pathnames in the "paths" variable might be relative,
	 * and we'll be chdir()ing into them, we need to keep a handle
	 * on our current directory from which to start the chdir().
	 */

	if (NULL == getcwd(buf, MAXPATHLEN)) {
		perror(NULL);
		goto out;
	} else if (-1 == (fd = open(buf, O_RDONLY, 0))) {
		perror(buf);
		goto out;
	}

	sql = sql_statement(e, arch, sec);

	/*
	 * Loop over the directories (containing databases) for us to
	 * search.
	 * Don't let missing/bad databases/directories phase us.
	 * In each, try to open the resident database and, if it opens,
	 * scan it for our match expression.
	 */

	for (i = 0; i < paths->sz; i++) {
		if (-1 == fchdir(fd)) {
			perror(buf);
			free(*res);
			break;
		} else if (-1 == chdir(paths->paths[i])) {
			perror(paths->paths[i]);
			continue;
		} 

		c =  sqlite3_open_v2
			(MANDOC_DB, &db, 
			 SQLITE_OPEN_READONLY, NULL);

		if (SQLITE_OK != c) {
			perror(MANDOC_DB);
			sqlite3_close(db);
			continue;
		}

		j = 1;
		c = sqlite3_prepare_v2(db, sql, -1, &s, NULL);
		if (SQLITE_OK != c)
			fprintf(stderr, "%s\n", sqlite3_errmsg(db));

		if (NULL != arch)
			SQL_BIND_TEXT(db, s, j, arch);
		if (NULL != sec)
			SQL_BIND_TEXT(db, s, j, arch);

		for (ep = e; NULL != ep; ep = ep->next) {
			SQL_BIND_TEXT(db, s, j, ep->v);
			SQL_BIND_INT64(db, s, j, ep->bits);
		}

		memset(&htab, 0, sizeof(struct ohash));
		ohash_init(&htab, 4, &info);

		/*
		 * Hash each entry on its [unique] document identifier.
		 * This is a uint64_t.
		 * Instead of using a hash function, simply convert the
		 * uint64_t to a uint32_t, the hash value's type.
		 * This gives good performance and preserves the
		 * distribution of buckets in the table.
		 */
		while (SQLITE_ROW == (c = sqlite3_step(s))) {
			id = sqlite3_column_int64(s, 0);
			idx = ohash_lookup_memory
				(&htab, (char *)&id, 
				 sizeof(uint64_t), (uint32_t)id);

			if (NULL != ohash_find(&htab, idx))
				continue;

			mp = mandoc_calloc(1, sizeof(struct match));
			mp->id = id;
			mp->file = mandoc_strdup
				((char *)sqlite3_column_text(s, 3));
			mp->desc = mandoc_strdup
				((char *)sqlite3_column_text(s, 4));
			mp->form = sqlite3_column_int(s, 5);
			ohash_insert(&htab, idx, mp);
		}

		if (SQLITE_DONE != c)
			fprintf(stderr, "%s\n", sqlite3_errmsg(db));

		sqlite3_finalize(s);
		sqlite3_close(db);

		for (mp = ohash_first(&htab, &idx);
				NULL != mp;
				mp = ohash_next(&htab, &idx)) {
			if (cur + 1 > maxres) {
				maxres += 1024;
				*res = mandoc_realloc
					(*res, maxres * sizeof(struct manpage));
			}
			strlcpy((*res)[cur].file, 
				paths->paths[i], MAXPATHLEN);
			strlcat((*res)[cur].file, "/", MAXPATHLEN);
			strlcat((*res)[cur].file, mp->file, MAXPATHLEN);
			(*res)[cur].desc = mp->desc;
			(*res)[cur].form = mp->form;
			free(mp->file);
			free(mp);
			cur++;
		}
		ohash_delete(&htab);
	}
	rc = 1;
out:
	exprfree(e);
	if (-1 != fd)
		close(fd);
	free(sql);
	*sz = cur;
	return(rc);
}

/*
 * Prepare the search SQL statement.
 * We search for any of the words specified in our match expression.
 * We filter the per-doc AND expressions when collecting results.
 */
static char *
sql_statement(const struct expr *e, const char *arch, const char *sec)
{
	char		*sql;
	const char	*glob = "(key GLOB ? AND bits & ?)";
	const char	*eq = "(key = ? AND bits & ?)";
	const char	*andarch = "arch = ? AND ";
	const char	*andsec = "sec = ? AND ";
	size_t	 	 globsz;
	size_t	 	 eqsz;
	size_t		 sz;

	sql = mandoc_strdup
		("SELECT docid,bits,key,file,desc,form,sec,arch "
		 "FROM keys "
		 "INNER JOIN docs ON docs.id=keys.docid "
		 "WHERE ");
	sz = strlen(sql);
	globsz = strlen(glob);
	eqsz = strlen(eq);

	if (NULL != arch) {
		sz += strlen(andarch) + 1;
		sql = mandoc_realloc(sql, sz);
		strlcat(sql, andarch, sz);
	}

	if (NULL != sec) {
		sz += strlen(andsec) + 1;
		sql = mandoc_realloc(sql, sz);
		strlcat(sql, andsec, sz);
	}

	sz += 2;
	sql = mandoc_realloc(sql, sz);
	strlcat(sql, "(", sz);

	for ( ; NULL != e; e = e->next) {
		sz += (e->glob ? globsz : eqsz) + 
			(NULL == e->next ? 3 : 5);
		sql = mandoc_realloc(sql, sz);
		strlcat(sql, e->glob ? glob : eq, sz);
		strlcat(sql, NULL == e->next ? ");" : " OR ", sz);
	}

	return(sql);
}

/*
 * Compile a set of string tokens into an expression.
 * Tokens in "argv" are assumed to be individual expression atoms (e.g.,
 * "(", "foo=bar", etc.).
 */
static struct expr *
exprcomp(int argc, char *argv[])
{
	int		 i;
	struct expr	*first, *next, *cur;

	first = cur = NULL;

	for (i = 0; i < argc; i++) {
		next = exprterm(argv[i]);
		if (NULL == next) {
			exprfree(first);
			return(NULL);
		}
		if (NULL != first) {
			cur->next = next;
			cur = next;
		} else
			cur = first = next;
	}

	return(first);
}

static struct expr *
exprterm(char *buf)
{
	struct expr	*e;
	char		*key, *v;
	size_t		 i;

	if ('\0' == *buf)
		return(NULL);

	e = mandoc_calloc(1, sizeof(struct expr));

	/*
	 * If no =~ is specified, search with equality over names and
	 * descriptions.
	 * If =~ begins the phrase, use name and description fields.
	 */

	if (NULL == (v = strpbrk(buf, "=~"))) {
		e->v = buf;
		e->bits = TYPE_Nm | TYPE_Nd;
		return(e);
	} else if (v == buf)
		e->bits = TYPE_Nm | TYPE_Nd;

	e->glob = '~' == *v;
	*v++ = '\0';
	e->v = v;

	/*
	 * Parse out all possible fields.
	 * If the field doesn't resolve, bail.
	 */

	while (NULL != (key = strsep(&buf, ","))) {
		if ('\0' == *key)
			continue;
		i = 0;
		while (types[i].bits && 
			strcasecmp(types[i].name, key))
			i++;
		if (0 == types[i].bits) {
			free(e);
			return(NULL);
		}
		e->bits |= types[i].bits;
	}

	return(e);
}

static void
exprfree(struct expr *p)
{
	struct expr	*pp;

	while (NULL != p) {
		pp = p->next;
		free(p);
		p = pp;
	}
}

static void *
hash_halloc(size_t sz, void *arg)
{

	return(mandoc_calloc(sz, 1));
}

static void *
hash_alloc(size_t sz, void *arg)
{

	return(mandoc_malloc(sz));
}

static void
hash_free(void *p, size_t sz, void *arg)
{

	free(p);
}
