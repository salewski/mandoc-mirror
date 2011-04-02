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
#include <sys/param.h>

#include <assert.h>
#ifdef __linux__
# include <db_185.h>
#else
# include <db.h>
#endif
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "man.h"
#include "mdoc.h"
#include "mandoc.h"

#define	MANDOC_DB	 "mandoc.db"
#define	MANDOC_BUFSZ	  10

enum	type {
	MANDOC_NONE = 0,
	MANDOC_NAME,
	MANDOC_FUNCTION,
	MANDOC_UTILITY,
	MANDOC_INCLUDES,
	MANDOC_VARIABLE
};

#define	MDOC_ARGS	  DB *db, \
			  const char *dbn, \
			  DBT *key, size_t *ksz, \
			  DBT *val, \
			  const struct mdoc_node *n

static	void		  dbt_append(DBT *, size_t *, const char *);
static	void		  dbt_appendb(DBT *, size_t *, 
				const void *, size_t);
static	void		  dbt_init(DBT *, size_t *);
static	void		  version(void);
static	void		  usage(void);
static	void		  pmdoc(DB *, const char *, 
				DBT *, size_t *, 
				DBT *, size_t *, 
				const char *, struct mdoc *);
static	void		  pmdoc_node(MDOC_ARGS);
static	void		  pmdoc_Fd(MDOC_ARGS);
static	void		  pmdoc_In(MDOC_ARGS);
static	void		  pmdoc_Fn(MDOC_ARGS);
static	void		  pmdoc_Fo(MDOC_ARGS);
static	void		  pmdoc_Nm(MDOC_ARGS);
static	void		  pmdoc_Vt(MDOC_ARGS);

typedef	void		(*pmdoc_nf)(MDOC_ARGS);

static	const char	 *progname;

static	const pmdoc_nf	  mdocs[MDOC_MAX] = {
	NULL, /* Ap */
	NULL, /* Dd */
	NULL, /* Dt */
	NULL, /* Os */
	NULL, /* Sh */ 
	NULL, /* Ss */ 
	NULL, /* Pp */ 
	NULL, /* D1 */
	NULL, /* Dl */
	NULL, /* Bd */
	NULL, /* Ed */
	NULL, /* Bl */ 
	NULL, /* El */
	NULL, /* It */
	NULL, /* Ad */ 
	NULL, /* An */ 
	NULL, /* Ar */
	NULL, /* Cd */ 
	NULL, /* Cm */
	NULL, /* Dv */ 
	NULL, /* Er */ 
	NULL, /* Ev */ 
	NULL, /* Ex */ 
	NULL, /* Fa */ 
	pmdoc_Fd, /* Fd */
	NULL, /* Fl */
	pmdoc_Fn, /* Fn */ 
	NULL, /* Ft */ 
	NULL, /* Ic */ 
	pmdoc_In, /* In */ 
	NULL, /* Li */
	NULL, /* Nd */
	pmdoc_Nm, /* Nm */
	NULL, /* Op */
	NULL, /* Ot */
	NULL, /* Pa */
	NULL, /* Rv */
	NULL, /* St */ 
	pmdoc_Vt, /* Va */
	pmdoc_Vt, /* Vt */ 
	NULL, /* Xr */ 
	NULL, /* %A */
	NULL, /* %B */
	NULL, /* %D */
	NULL, /* %I */
	NULL, /* %J */
	NULL, /* %N */
	NULL, /* %O */
	NULL, /* %P */
	NULL, /* %R */
	NULL, /* %T */
	NULL, /* %V */
	NULL, /* Ac */
	NULL, /* Ao */
	NULL, /* Aq */
	NULL, /* At */ 
	NULL, /* Bc */
	NULL, /* Bf */
	NULL, /* Bo */
	NULL, /* Bq */
	NULL, /* Bsx */
	NULL, /* Bx */
	NULL, /* Db */
	NULL, /* Dc */
	NULL, /* Do */
	NULL, /* Dq */
	NULL, /* Ec */
	NULL, /* Ef */ 
	NULL, /* Em */ 
	NULL, /* Eo */
	NULL, /* Fx */
	NULL, /* Ms */ 
	NULL, /* No */
	NULL, /* Ns */
	NULL, /* Nx */
	NULL, /* Ox */
	NULL, /* Pc */
	NULL, /* Pf */
	NULL, /* Po */
	NULL, /* Pq */
	NULL, /* Qc */
	NULL, /* Ql */
	NULL, /* Qo */
	NULL, /* Qq */
	NULL, /* Re */
	NULL, /* Rs */
	NULL, /* Sc */
	NULL, /* So */
	NULL, /* Sq */
	NULL, /* Sm */ 
	NULL, /* Sx */
	NULL, /* Sy */
	NULL, /* Tn */
	NULL, /* Ux */
	NULL, /* Xc */
	NULL, /* Xo */
	pmdoc_Fo, /* Fo */ 
	NULL, /* Fc */ 
	NULL, /* Oo */
	NULL, /* Oc */
	NULL, /* Bk */
	NULL, /* Ek */
	NULL, /* Bt */
	NULL, /* Hf */
	NULL, /* Fr */
	NULL, /* Ud */
	NULL, /* Lb */
	NULL, /* Lp */ 
	NULL, /* Lk */ 
	NULL, /* Mt */ 
	NULL, /* Brq */ 
	NULL, /* Bro */ 
	NULL, /* Brc */ 
	NULL, /* %C */
	NULL, /* Es */
	NULL, /* En */
	NULL, /* Dx */
	NULL, /* %Q */
	NULL, /* br */
	NULL, /* sp */
	NULL, /* %U */
	NULL, /* Ta */
};

int
main(int argc, char *argv[])
{
	struct mparse	*mp;
	struct mdoc	*mdoc;
	struct man	*man;
	const char	*f, *fn;
	size_t		 sz;
	char		 fbuf[MAXPATHLEN];
	int		 c;
	DB		*db;
	DBT		 key, val;
	size_t		 ksz, vsz;
	BTREEINFO	 info;
	extern int	 optind;
	extern char	*optarg;

	f = MANDOC_DB;

	progname = strrchr(argv[0], '/');
	if (progname == NULL)
		progname = argv[0];
	else
		++progname;

	while (-1 != (c = getopt(argc, argv, "f:V")))
		switch (c) {
		case ('f'):
			f = optarg;
			break;
		case ('V'):
			version();
			return((int)MANDOCLEVEL_OK);
		default:
			usage();
			return((int)MANDOCLEVEL_BADARG);
		}

	argc -= optind;
	argv += optind;

	/*
	 * Set up a temporary file-name into which we're going to write
	 * all of our data.  This is securely renamed to the real
	 * file-name after we've written all of our data.
	 */

	if (0 == (sz = strlen(f)) || sz + 5 >= MAXPATHLEN) {
		fprintf(stderr, "%s: Bad filename\n", progname);
		exit((int)MANDOCLEVEL_SYSERR);
	}

	memcpy(fbuf, f, sz);
	memcpy(fbuf + (int)sz, ".bak", 4);
	fbuf[(int)sz + 4] = '\0';

	/*
	 * Open a BTREE database that allows duplicates.  If the
	 * database already exists (it's a backup anyway), then blow it
	 * away with O_TRUNC.
	 */

	memset(&info, 0, sizeof(BTREEINFO));
	info.flags = R_DUP;

	db = dbopen(fbuf, O_CREAT|O_TRUNC|O_RDWR, 
			0644, DB_BTREE, &info);

	if (NULL == db) {
		perror(f);
		exit((int)MANDOCLEVEL_SYSERR);
	}

	/* Use the auto-parser and don't report any errors. */

	mp = mparse_alloc(MPARSE_AUTO, MANDOCLEVEL_FATAL, NULL, NULL);

	/*
	 * Try parsing the manuals given on the command line.  If we
	 * totally fail, then just keep on going.  Take resulting trees
	 * and push them down into the database code.
	 */

	memset(&key, 0, sizeof(DBT));
	memset(&val, 0, sizeof(DBT));
	ksz = vsz = 0;

	while (NULL != (fn = *argv++)) {
		printf("Trying: %s\n", fn);
		mparse_reset(mp);
		if (mparse_readfd(mp, -1, fn) >= MANDOCLEVEL_FATAL)
			continue;
		mparse_result(mp, &mdoc, &man);
		if (mdoc)
			pmdoc(db, fbuf, &key, &ksz, 
				&val, &vsz, fn, mdoc);
	}

	(*db->close)(db);
	mparse_free(mp);

	free(key.data);
	free(val.data);

	/* Atomically replace the file with our temporary one. */

	if (-1 == rename(fbuf, f))
		perror(f);

	return((int)MANDOCLEVEL_OK);
}

/*
 * Initialise the stored database key whose data buffer is shared
 * between uses (as the key must sometimes be constructed from an array
 * of 
 */
static void
dbt_init(DBT *key, size_t *ksz)
{

	if (0 == *ksz) {
		assert(0 == key->size);
		assert(NULL == key->data);
		key->data = mandoc_malloc(MANDOC_BUFSZ);
		*ksz = MANDOC_BUFSZ;
	}

	key->size = 0;
}

/*
 * Append a binary value to a database entry.  This can be invoked
 * multiple times; the buffer is automatically resized.
 */
static void
dbt_appendb(DBT *key, size_t *ksz, const void *cp, size_t sz)
{

	assert(key->data);

	/* Overshoot by MANDOC_BUFSZ. */

	while (key->size + sz >= *ksz) {
		*ksz = key->size + sz + MANDOC_BUFSZ;
		*ksz = *ksz + (4 - (*ksz % 4));
		key->data = mandoc_realloc(key->data, *ksz);
	}

	memcpy(key->data + (int)key->size, cp, sz);
	key->size += sz;
}

/*
 * Append a nil-terminated string to the database entry.  This can be
 * invoked multiple times.  The database entry will be nil-terminated as
 * well; if invoked multiple times, a space is put between strings.
 */
static void
dbt_append(DBT *key, size_t *ksz, const char *cp)
{
	size_t		 sz;

	assert(key->data);
	assert(key->size <= *ksz);

	if (0 == (sz = strlen(cp)))
		return;

	/* Overshoot by MANDOC_BUFSZ (and nil terminator). */

	while (key->size + sz + 1 >= *ksz) {
		*ksz = key->size + sz + 1 + MANDOC_BUFSZ;
		*ksz = *ksz + (4 - (*ksz % 4));
		key->data = mandoc_realloc(key->data, *ksz);
	}

	/* Space-separate appended tokens. */

	if (key->size)
		((char *)key->data)[(int)key->size - 1] = ' ';

	memcpy(key->data + (int)key->size, cp, sz + 1);
	key->size += sz + 1;
}

/* ARGSUSED */
static void
pmdoc_Fd(MDOC_ARGS)
{
	uint32_t	 fl;
	const char	*start, *end;
	size_t		 sz;
	char		 nil;
	
	if (SEC_SYNOPSIS != n->sec)
		return;
	if (NULL == (n = n->child) || MDOC_TEXT != n->type)
		return;
	if (strcmp("#include", n->string))
		return;
	if (NULL == (n = n->next) || MDOC_TEXT != n->type)
		return;

	start = n->string;
	if ('<' == *start)
		start++;

	if (0 == (sz = strlen(start)))
		return;

	end = &start[(int)sz - 1];
	if ('>' == *end)
		end--;

	nil = '\0';
	dbt_appendb(key, ksz, start, end - start + 1);
	dbt_appendb(key, ksz, &nil, 1);

	fl = MANDOC_INCLUDES;
	memcpy(val->data, &fl, 4);
}

/* ARGSUSED */
static void
pmdoc_In(MDOC_ARGS)
{
	uint32_t	 fl;
	
	if (SEC_SYNOPSIS != n->sec)
		return;
	if (NULL == n->child || MDOC_TEXT != n->child->type)
		return;

	dbt_append(key, ksz, n->child->string);
	fl = MANDOC_INCLUDES;
	memcpy(val->data, &fl, 4);
}

/* ARGSUSED */
static void
pmdoc_Fn(MDOC_ARGS)
{
	uint32_t	 fl;
	const char	*cp;
	
	if (SEC_SYNOPSIS != n->sec)
		return;
	if (NULL == n->child || MDOC_TEXT != n->child->type)
		return;

	/* .Fn "struct type *arg" "foo" */

	cp = strrchr(n->child->string, ' ');
	if (NULL == cp)
		cp = n->child->string;

	/* Ignore pointers. */

	while ('*' == *cp)
		cp++;

	dbt_append(key, ksz, cp);
	fl = MANDOC_FUNCTION;
	memcpy(val->data, &fl, 4);
}

/* ARGSUSED */
static void
pmdoc_Vt(MDOC_ARGS)
{
	uint32_t	 fl;
	const char	*start, *end;
	size_t		 sz;
	char		 nil;
	
	if (SEC_SYNOPSIS != n->sec)
		return;
	if (MDOC_Vt == n->tok && MDOC_BODY != n->type)
		return;
	if (NULL == n->child || MDOC_TEXT != n->child->type)
		return;

	/*
	 * Strip away leading '*' and trailing ';'.
	 */

	start = n->last->string;

	while ('*' == *start)
		start++;

	if (0 == (sz = strlen(start)))
		return;

	end = &start[sz - 1];
	while (end > start && ';' == *end)
		end--;

	if (end == start)
		return;

	nil = '\0';
	dbt_appendb(key, ksz, start, end - start + 1);
	dbt_appendb(key, ksz, &nil, 1);
	fl = MANDOC_VARIABLE;
	memcpy(val->data, &fl, 4);
}

/* ARGSUSED */
static void
pmdoc_Fo(MDOC_ARGS)
{
	uint32_t	 fl;
	
	if (SEC_SYNOPSIS != n->sec || MDOC_HEAD != n->type)
		return;
	if (NULL == n->child || MDOC_TEXT != n->child->type)
		return;

	dbt_append(key, ksz, n->child->string);
	fl = MANDOC_FUNCTION;
	memcpy(val->data, &fl, 4);
}

/* ARGSUSED */
static void
pmdoc_Nm(MDOC_ARGS)
{
	uint32_t	 fl;
	
	if (SEC_NAME == n->sec) {
		for (n = n->child; n; n = n->next) {
			if (MDOC_TEXT != n->type)
				continue;
			dbt_append(key, ksz, n->string);
		}
		fl = MANDOC_NAME;
		memcpy(val->data, &fl, 4);
		return;
	} else if (SEC_SYNOPSIS != n->sec || MDOC_HEAD != n->type)
		return;

	for (n = n->child; n; n = n->next) {
		if (MDOC_TEXT != n->type)
			continue;
		dbt_append(key, ksz, n->string);
	}

	fl = MANDOC_UTILITY;
	memcpy(val->data, &fl, 4);
}

/*
 * Call out to per-macro handlers after clearing the persistent database
 * key.  If the macro sets the database key, flush it to the database.
 */
static void
pmdoc_node(MDOC_ARGS)
{

	if (NULL == n)
		return;

	switch (n->type) {
	case (MDOC_HEAD):
		/* FALLTHROUGH */
	case (MDOC_BODY):
		/* FALLTHROUGH */
	case (MDOC_TAIL):
		/* FALLTHROUGH */
	case (MDOC_BLOCK):
		/* FALLTHROUGH */
	case (MDOC_ELEM):
		if (NULL == mdocs[n->tok])
			break;

		dbt_init(key, ksz);
		(*mdocs[n->tok])(db, dbn, key, ksz, val, n);

		if (0 == key->size)
			break;
		if (0 == (*db->put)(db, key, val, 0))
			break;
		
		perror(dbn);
		exit((int)MANDOCLEVEL_SYSERR);
		/* NOTREACHED */
	default:
		break;
	}

	pmdoc_node(db, dbn, key, ksz, val, n->child);
	pmdoc_node(db, dbn, key, ksz, val, n->next);
}

static void
pmdoc(DB *db, const char *dbn, 
		DBT *key, size_t *ksz, 
		DBT *val, size_t *valsz,
		const char *path, struct mdoc *m)
{
	uint32_t	 flag;

	flag = MANDOC_NONE;

	/* 
	 * Database values are a 4-byte bit-field followed by the path
	 * of the manual.  Allocate all the space we'll need now; we
	 * change the bit-field depending on the key type.
	 */

	dbt_init(val, valsz);
	dbt_appendb(val, valsz, &flag, 4);
	dbt_append(val, valsz, path);

	pmdoc_node(db, dbn, key, ksz, val, mdoc_node(m));
}

static void
version(void)
{

	printf("%s %s\n", progname, VERSION);
}

static void
usage(void)
{

	fprintf(stderr, "usage: %s "
			"[-V] "
			"[-f path] "
			"[file...]\n", 
			progname);
}
