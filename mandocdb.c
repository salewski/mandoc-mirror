/*	$Id$ */
/*
 * Copyright (c) 2011, 2012 Kristaps Dzonsons <kristaps@bsd.lv>
 * Copyright (c) 2011, 2012, 2013 Ingo Schwarze <schwarze@openbsd.org>
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

#include <sys/stat.h>

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <fts.h>
#include <getopt.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef HAVE_OHASH
#include <ohash.h>
#else
#include "compat_ohash.h"
#endif
#include <sqlite3.h>

#include "mdoc.h"
#include "man.h"
#include "mandoc.h"
#include "manpath.h"
#include "mansearch.h"

#define	SQL_EXEC(_v) \
	if (SQLITE_OK != sqlite3_exec(db, (_v), NULL, NULL, NULL)) \
		fprintf(stderr, "%s\n", sqlite3_errmsg(db))
#define	SQL_BIND_TEXT(_s, _i, _v) \
	if (SQLITE_OK != sqlite3_bind_text \
		((_s), (_i)++, (_v), -1, SQLITE_STATIC)) \
		fprintf(stderr, "%s\n", sqlite3_errmsg(db))
#define	SQL_BIND_INT(_s, _i, _v) \
	if (SQLITE_OK != sqlite3_bind_int \
		((_s), (_i)++, (_v))) \
		fprintf(stderr, "%s\n", sqlite3_errmsg(db))
#define	SQL_BIND_INT64(_s, _i, _v) \
	if (SQLITE_OK != sqlite3_bind_int64 \
		((_s), (_i)++, (_v))) \
		fprintf(stderr, "%s\n", sqlite3_errmsg(db))
#define SQL_STEP(_s) \
	if (SQLITE_DONE != sqlite3_step((_s))) \
		fprintf(stderr, "%s\n", sqlite3_errmsg(db))

enum	op {
	OP_DEFAULT = 0, /* new dbs from dir list or default config */
	OP_CONFFILE, /* new databases from custom config file */
	OP_UPDATE, /* delete/add entries in existing database */
	OP_DELETE, /* delete entries from existing database */
	OP_TEST /* change no databases, report potential problems */
};

enum	form {
	FORM_SRC, /* format is -man or -mdoc */
	FORM_CAT, /* format is cat */
	FORM_NONE /* format is unknown */
};

struct	str {
	char		*utf8; /* key in UTF-8 form */
	const struct of *of; /* if set, the owning parse */
	struct str	*next; /* next in owning parse sequence */
	uint64_t	 mask; /* bitmask in sequence */
	char		 key[]; /* the string itself */
};

struct	id {
	ino_t		 ino;
	dev_t		 dev;
};

struct	of {
	struct id	 id; /* used for hashing routine */
	struct of	*next; /* next in ofs */
	enum form	 dform; /* path-cued form */
	enum form	 sform; /* suffix-cued form */
	char		 file[PATH_MAX]; /* filename rel. to manpath */
	const char	*desc; /* parsed description */
	const char	*sec; /* suffix-cued section (or empty) */
	const char	*dsec; /* path-cued section (or empty) */
	const char	*arch; /* path-cued arch. (or empty) */
	const char	*name; /* name (from filename) (not empty) */
};

enum	stmt {
	STMT_DELETE = 0, /* delete manpage */
	STMT_INSERT_DOC, /* insert manpage */
	STMT_INSERT_KEY, /* insert parsed key */
	STMT__MAX
};

typedef	int (*mdoc_fp)(struct of *, const struct mdoc_node *);

struct	mdoc_handler {
	mdoc_fp		 fp; /* optional handler */
	uint64_t	 mask;  /* set unless handler returns 0 */
	int		 flags;  /* for use by pmdoc_node */
#define	MDOCF_CHILD	 0x01  /* automatically index child nodes */
};

static	void	 dbclose(int);
static	void	 dbindex(struct mchars *, int, const struct of *);
static	int	 dbopen(int);
static	void	 dbprune(void);
static	void	 fileadd(struct of *);
static	int	 filecheck(const char *);
static	void	 filescan(const char *);
static	struct str *hashget(const char *, size_t);
static	void	*hash_alloc(size_t, void *);
static	void	 hash_free(void *, size_t, void *);
static	void	*hash_halloc(size_t, void *);
static	void	 inoadd(const struct stat *, struct of *);
static	int	 inocheck(const struct stat *);
static	void	 ofadd(int, const char *, const char *, const char *,
			const char *, const char *, const struct stat *);
static	void	 offree(void);
static	void	 ofmerge(struct mchars *, struct mparse *);
static	void	 parse_catpage(struct of *);
static	void	 parse_man(struct of *, const struct man_node *);
static	void	 parse_mdoc(struct of *, const struct mdoc_node *);
static	int	 parse_mdoc_body(struct of *, const struct mdoc_node *);
static	int	 parse_mdoc_head(struct of *, const struct mdoc_node *);
static	int	 parse_mdoc_Fd(struct of *, const struct mdoc_node *);
static	int	 parse_mdoc_Fn(struct of *, const struct mdoc_node *);
static	int	 parse_mdoc_In(struct of *, const struct mdoc_node *);
static	int	 parse_mdoc_Nd(struct of *, const struct mdoc_node *);
static	int	 parse_mdoc_Nm(struct of *, const struct mdoc_node *);
static	int	 parse_mdoc_Sh(struct of *, const struct mdoc_node *);
static	int	 parse_mdoc_St(struct of *, const struct mdoc_node *);
static	int	 parse_mdoc_Xr(struct of *, const struct mdoc_node *);
static	int	 set_basedir(const char *);
static	void	 putkey(const struct of *, 
			const char *, uint64_t);
static	void	 putkeys(const struct of *, 
			const char *, int, uint64_t);
static	void	 putmdockey(const struct of *,
			const struct mdoc_node *, uint64_t);
static	void	 say(const char *, const char *, ...);
static	char 	*stradd(const char *);
static	char 	*straddbuf(const char *, size_t);
static	int	 treescan(void);
static	size_t	 utf8(unsigned int, char [7]);
static	void	 utf8key(struct mchars *, struct str *);
static	void 	 wordaddbuf(const struct of *, 
			const char *, size_t, uint64_t);

static	char		*progname;
static	int	 	 use_all; /* use all found files */
static	int		 nodb; /* no database changes */
static	int	  	 verb; /* print what we're doing */
static	int	  	 warnings; /* warn about crap */
static	int		 exitcode; /* to be returned by main */
static	enum op	  	 op; /* operational mode */
static	char		 basedir[PATH_MAX]; /* current base directory */
static	struct ohash	 inos; /* table of inodes/devices */
static	struct ohash	 filenames; /* table of filenames */
static	struct ohash	 strings; /* table of all strings */
static	struct of	*ofs = NULL; /* vector of files to parse */
static	struct str	*words = NULL; /* word list in current parse */
static	sqlite3		*db = NULL; /* current database */
static	sqlite3_stmt	*stmts[STMT__MAX]; /* current statements */

static	const struct mdoc_handler mdocs[MDOC_MAX] = {
	{ NULL, 0, 0 },  /* Ap */
	{ NULL, 0, 0 },  /* Dd */
	{ NULL, 0, 0 },  /* Dt */
	{ NULL, 0, 0 },  /* Os */
	{ parse_mdoc_Sh, TYPE_Sh, MDOCF_CHILD }, /* Sh */
	{ parse_mdoc_head, TYPE_Ss, MDOCF_CHILD }, /* Ss */
	{ NULL, 0, 0 },  /* Pp */
	{ NULL, 0, 0 },  /* D1 */
	{ NULL, 0, 0 },  /* Dl */
	{ NULL, 0, 0 },  /* Bd */
	{ NULL, 0, 0 },  /* Ed */
	{ NULL, 0, 0 },  /* Bl */
	{ NULL, 0, 0 },  /* El */
	{ NULL, 0, 0 },  /* It */
	{ NULL, 0, 0 },  /* Ad */
	{ NULL, TYPE_An, MDOCF_CHILD },  /* An */
	{ NULL, TYPE_Ar, MDOCF_CHILD },  /* Ar */
	{ NULL, TYPE_Cd, MDOCF_CHILD },  /* Cd */
	{ NULL, TYPE_Cm, MDOCF_CHILD },  /* Cm */
	{ NULL, TYPE_Dv, MDOCF_CHILD },  /* Dv */
	{ NULL, TYPE_Er, MDOCF_CHILD },  /* Er */
	{ NULL, TYPE_Ev, MDOCF_CHILD },  /* Ev */
	{ NULL, 0, 0 },  /* Ex */
	{ NULL, TYPE_Fa, MDOCF_CHILD },  /* Fa */
	{ parse_mdoc_Fd, TYPE_In, 0 },  /* Fd */
	{ NULL, TYPE_Fl, MDOCF_CHILD },  /* Fl */
	{ parse_mdoc_Fn, 0, 0 },  /* Fn */
	{ NULL, TYPE_Ft, MDOCF_CHILD },  /* Ft */
	{ NULL, TYPE_Ic, MDOCF_CHILD },  /* Ic */
	{ parse_mdoc_In, TYPE_In, MDOCF_CHILD },  /* In */
	{ NULL, TYPE_Li, MDOCF_CHILD },  /* Li */
	{ parse_mdoc_Nd, TYPE_Nd, MDOCF_CHILD },  /* Nd */
	{ parse_mdoc_Nm, TYPE_Nm, MDOCF_CHILD },  /* Nm */
	{ NULL, 0, 0 },  /* Op */
	{ NULL, 0, 0 },  /* Ot */
	{ NULL, TYPE_Pa, MDOCF_CHILD },  /* Pa */
	{ NULL, 0, 0 },  /* Rv */
	{ parse_mdoc_St, TYPE_St, 0 },  /* St */
	{ NULL, TYPE_Va, MDOCF_CHILD },  /* Va */
	{ parse_mdoc_body, TYPE_Va, MDOCF_CHILD },  /* Vt */
	{ parse_mdoc_Xr, TYPE_Xr, 0 },  /* Xr */
	{ NULL, 0, 0 },  /* %A */
	{ NULL, 0, 0 },  /* %B */
	{ NULL, 0, 0 },  /* %D */
	{ NULL, 0, 0 },  /* %I */
	{ NULL, 0, 0 },  /* %J */
	{ NULL, 0, 0 },  /* %N */
	{ NULL, 0, 0 },  /* %O */
	{ NULL, 0, 0 },  /* %P */
	{ NULL, 0, 0 },  /* %R */
	{ NULL, 0, 0 },  /* %T */
	{ NULL, 0, 0 },  /* %V */
	{ NULL, 0, 0 },  /* Ac */
	{ NULL, 0, 0 },  /* Ao */
	{ NULL, 0, 0 },  /* Aq */
	{ NULL, TYPE_At, MDOCF_CHILD },  /* At */
	{ NULL, 0, 0 },  /* Bc */
	{ NULL, 0, 0 },  /* Bf */
	{ NULL, 0, 0 },  /* Bo */
	{ NULL, 0, 0 },  /* Bq */
	{ NULL, TYPE_Bsx, MDOCF_CHILD },  /* Bsx */
	{ NULL, TYPE_Bx, MDOCF_CHILD },  /* Bx */
	{ NULL, 0, 0 },  /* Db */
	{ NULL, 0, 0 },  /* Dc */
	{ NULL, 0, 0 },  /* Do */
	{ NULL, 0, 0 },  /* Dq */
	{ NULL, 0, 0 },  /* Ec */
	{ NULL, 0, 0 },  /* Ef */
	{ NULL, TYPE_Em, MDOCF_CHILD },  /* Em */
	{ NULL, 0, 0 },  /* Eo */
	{ NULL, TYPE_Fx, MDOCF_CHILD },  /* Fx */
	{ NULL, TYPE_Ms, MDOCF_CHILD },  /* Ms */
	{ NULL, 0, 0 },  /* No */
	{ NULL, 0, 0 },  /* Ns */
	{ NULL, TYPE_Nx, MDOCF_CHILD },  /* Nx */
	{ NULL, TYPE_Ox, MDOCF_CHILD },  /* Ox */
	{ NULL, 0, 0 },  /* Pc */
	{ NULL, 0, 0 },  /* Pf */
	{ NULL, 0, 0 },  /* Po */
	{ NULL, 0, 0 },  /* Pq */
	{ NULL, 0, 0 },  /* Qc */
	{ NULL, 0, 0 },  /* Ql */
	{ NULL, 0, 0 },  /* Qo */
	{ NULL, 0, 0 },  /* Qq */
	{ NULL, 0, 0 },  /* Re */
	{ NULL, 0, 0 },  /* Rs */
	{ NULL, 0, 0 },  /* Sc */
	{ NULL, 0, 0 },  /* So */
	{ NULL, 0, 0 },  /* Sq */
	{ NULL, 0, 0 },  /* Sm */
	{ NULL, 0, 0 },  /* Sx */
	{ NULL, TYPE_Sy, MDOCF_CHILD },  /* Sy */
	{ NULL, TYPE_Tn, MDOCF_CHILD },  /* Tn */
	{ NULL, 0, 0 },  /* Ux */
	{ NULL, 0, 0 },  /* Xc */
	{ NULL, 0, 0 },  /* Xo */
	{ parse_mdoc_head, TYPE_Fn, 0 },  /* Fo */
	{ NULL, 0, 0 },  /* Fc */
	{ NULL, 0, 0 },  /* Oo */
	{ NULL, 0, 0 },  /* Oc */
	{ NULL, 0, 0 },  /* Bk */
	{ NULL, 0, 0 },  /* Ek */
	{ NULL, 0, 0 },  /* Bt */
	{ NULL, 0, 0 },  /* Hf */
	{ NULL, 0, 0 },  /* Fr */
	{ NULL, 0, 0 },  /* Ud */
	{ NULL, TYPE_Lb, MDOCF_CHILD },  /* Lb */
	{ NULL, 0, 0 },  /* Lp */
	{ NULL, TYPE_Lk, MDOCF_CHILD },  /* Lk */
	{ NULL, TYPE_Mt, MDOCF_CHILD },  /* Mt */
	{ NULL, 0, 0 },  /* Brq */
	{ NULL, 0, 0 },  /* Bro */
	{ NULL, 0, 0 },  /* Brc */
	{ NULL, 0, 0 },  /* %C */
	{ NULL, 0, 0 },  /* Es */
	{ NULL, 0, 0 },  /* En */
	{ NULL, TYPE_Dx, MDOCF_CHILD },  /* Dx */
	{ NULL, 0, 0 },  /* %Q */
	{ NULL, 0, 0 },  /* br */
	{ NULL, 0, 0 },  /* sp */
	{ NULL, 0, 0 },  /* %U */
	{ NULL, 0, 0 },  /* Ta */
};

int
main(int argc, char *argv[])
{
	int		  ch, i;
	unsigned int	  index;
	size_t		  j, sz;
	const char	 *path_arg;
	struct str	 *s;
	struct mchars	 *mc;
	struct manpaths	  dirs;
	struct mparse	 *mp;
	struct ohash_info ino_info, filename_info, str_info;

	memset(stmts, 0, STMT__MAX * sizeof(sqlite3_stmt *));
	memset(&dirs, 0, sizeof(struct manpaths));

	ino_info.halloc = filename_info.halloc = 
		str_info.halloc = hash_halloc;
	ino_info.hfree = filename_info.hfree = 
		str_info.hfree = hash_free;
	ino_info.alloc = filename_info.alloc = 
		str_info.alloc = hash_alloc;

	ino_info.key_offset = offsetof(struct of, id);
	filename_info.key_offset = offsetof(struct of, file);
	str_info.key_offset = offsetof(struct str, key);

	progname = strrchr(argv[0], '/');
	if (progname == NULL)
		progname = argv[0];
	else
		++progname;

	/*
	 * We accept a few different invocations.  
	 * The CHECKOP macro makes sure that invocation styles don't
	 * clobber each other.
	 */
#define	CHECKOP(_op, _ch) do \
	if (OP_DEFAULT != (_op)) { \
		fprintf(stderr, "-%c: Conflicting option\n", (_ch)); \
		goto usage; \
	} while (/*CONSTCOND*/0)

	path_arg = NULL;
	op = OP_DEFAULT;

	while (-1 != (ch = getopt(argc, argv, "aC:d:ntu:vW")))
		switch (ch) {
		case ('a'):
			use_all = 1;
			break;
		case ('C'):
			CHECKOP(op, ch);
			path_arg = optarg;
			op = OP_CONFFILE;
			break;
		case ('d'):
			CHECKOP(op, ch);
			path_arg = optarg;
			op = OP_UPDATE;
			break;
		case ('n'):
			nodb = 1;
			break;
		case ('t'):
			CHECKOP(op, ch);
			dup2(STDOUT_FILENO, STDERR_FILENO);
			op = OP_TEST;
			nodb = warnings = 1;
			break;
		case ('u'):
			CHECKOP(op, ch);
			path_arg = optarg;
			op = OP_DELETE;
			break;
		case ('v'):
			verb++;
			break;
		case ('W'):
			warnings = 1;
			break;
		default:
			goto usage;
		}

	argc -= optind;
	argv += optind;

	if (OP_CONFFILE == op && argc > 0) {
		fprintf(stderr, "-C: Too many arguments\n");
		goto usage;
	}

	exitcode = (int)MANDOCLEVEL_OK;
	mp = mparse_alloc(MPARSE_AUTO, 
		MANDOCLEVEL_FATAL, NULL, NULL, NULL);
	mc = mchars_alloc();

	ohash_init(&strings, 6, &str_info);
	ohash_init(&inos, 6, &ino_info);
	ohash_init(&filenames, 6, &filename_info);

	if (OP_UPDATE == op || OP_DELETE == op || OP_TEST == op) {
		/* 
		 * Force processing all files.
		 */
		use_all = 1;

		/*
		 * All of these deal with a specific directory.
		 * Jump into that directory then collect files specified
		 * on the command-line.
		 */
		if (0 == set_basedir(path_arg))
			goto out;
		for (i = 0; i < argc; i++)
			filescan(argv[i]);
		if (0 == dbopen(1))
			goto out;
		if (OP_TEST != op)
			dbprune();
		if (OP_DELETE != op)
			ofmerge(mc, mp);
		dbclose(1);
	} else {
		/*
		 * If we have arguments, use them as our manpaths.
		 * If we don't, grok from manpath(1) or however else
		 * manpath_parse() wants to do it.
		 */
		if (argc > 0) {
			dirs.paths = mandoc_calloc
				(argc, sizeof(char *));
			dirs.sz = (size_t)argc;
			for (i = 0; i < argc; i++)
				dirs.paths[i] = mandoc_strdup(argv[i]);
		} else
			manpath_parse(&dirs, path_arg, NULL, NULL);

		/*
		 * First scan the tree rooted at a base directory.
		 * Then whak its database (if one exists), parse, and
		 * build up the database.
		 * Ignore zero-length directories and strip trailing
		 * slashes.
		 */
		for (j = 0; j < dirs.sz; j++) {
			sz = strlen(dirs.paths[j]);
			if (sz && '/' == dirs.paths[j][sz - 1])
				dirs.paths[j][--sz] = '\0';
			if (0 == sz)
				continue;
			if (0 == set_basedir(dirs.paths[j]))
				goto out;
			if (0 == treescan())
				goto out;
			if (0 == set_basedir(dirs.paths[j]))
				goto out;
			if (0 == dbopen(0))
				goto out;

			/*
			 * Since we're opening up a new database, we can
			 * turn off synchronous mode for much better
			 * performance.
			 */
#ifndef __APPLE__
			SQL_EXEC("PRAGMA synchronous = OFF");
#endif

			ofmerge(mc, mp);
			dbclose(0);
			offree();
			ohash_delete(&inos);
			ohash_init(&inos, 6, &ino_info);
			ohash_delete(&filenames);
			ohash_init(&filenames, 6, &filename_info);
		}
	}
out:
	set_basedir(NULL);
	manpath_free(&dirs);
	mchars_free(mc);
	mparse_free(mp);
	for (s = ohash_first(&strings, &index);
			NULL != s; s = ohash_next(&strings, &index)) {
		if (s->utf8 != s->key)
			free(s->utf8);
		free(s);
	}
	ohash_delete(&strings);
	ohash_delete(&inos);
	ohash_delete(&filenames);
	offree();
	return(exitcode);
usage:
	fprintf(stderr, "usage: %s [-anvW] [-C file]\n"
			"       %s [-anvW] dir ...\n"
			"       %s [-nvW] -d dir [file ...]\n"
			"       %s [-nvW] -u dir [file ...]\n"
			"       %s -t file ...\n",
		       progname, progname, progname, 
		       progname, progname);

	return((int)MANDOCLEVEL_BADARG);
}

/*
 * Scan a directory tree rooted at "basedir" for manpages.
 * We use fts(), scanning directory parts along the way for clues to our
 * section and architecture.
 *
 * If use_all has been specified, grok all files.
 * If not, sanitise paths to the following:
 *
 *   [./]man*[/<arch>]/<name>.<section> 
 *   or
 *   [./]cat<section>[/<arch>]/<name>.0
 *
 * TODO: accomodate for multi-language directories.
 */
static int
treescan(void)
{
	FTS		*f;
	FTSENT		*ff;
	int		 dform;
	char		*sec;
	const char	*dsec, *arch, *cp, *name, *path;
	const char	*argv[2];

	argv[0] = ".";
	argv[1] = (char *)NULL;

	/*
	 * Walk through all components under the directory, using the
	 * logical descent of files.
	 */
	f = fts_open((char * const *)argv, FTS_LOGICAL, NULL);
	if (NULL == f) {
		exitcode = (int)MANDOCLEVEL_SYSERR;
		say("", NULL);
		return(0);
	}

	dsec = arch = NULL;
	dform = FORM_NONE;

	while (NULL != (ff = fts_read(f))) {
		path = ff->fts_path + 2;
		/*
		 * If we're a regular file, add an "of" by using the
		 * stored directory data and handling the filename.
		 * Disallow duplicate (hard-linked) files.
		 */
		if (FTS_F == ff->fts_info) {
			if (0 == strcmp(path, MANDOC_DB))
				continue;
			if ( ! use_all && ff->fts_level < 2) {
				if (warnings)
					say(path, "Extraneous file");
				continue;
			} else if (inocheck(ff->fts_statp)) {
				if (warnings)
					say(path, "Duplicate file");
				continue;
			} else if (NULL == (sec =
					strrchr(ff->fts_name, '.'))) {
				if ( ! use_all) {
					if (warnings)
						say(path,
						    "No filename suffix");
					continue;
				}
			} else if (0 == strcmp(++sec, "html")) {
				if (warnings)
					say(path, "Skip html");
				continue;
			} else if (0 == strcmp(sec, "gz")) {
				if (warnings)
					say(path, "Skip gz");
				continue;
			} else if (0 == strcmp(sec, "ps")) {
				if (warnings)
					say(path, "Skip ps");
				continue;
			} else if (0 == strcmp(sec, "pdf")) {
				if (warnings)
					say(path, "Skip pdf");
				continue;
			} else if ( ! use_all &&
			    ((FORM_SRC == dform && strcmp(sec, dsec)) ||
			     (FORM_CAT == dform && strcmp(sec, "0")))) {
				if (warnings)
					say(path, "Wrong filename suffix");
				continue;
			} else {
				sec[-1] = '\0';
				sec = stradd(sec);
			}
			name = stradd(ff->fts_name);
			ofadd(dform, path, 
				name, dsec, sec, arch, ff->fts_statp);
			continue;
		} else if (FTS_D != ff->fts_info && 
				FTS_DP != ff->fts_info) {
			if (warnings)
				say(path, "Not a regular file");
			continue;
		}

		switch (ff->fts_level) {
		case (0):
			/* Ignore the root directory. */
			break;
		case (1):
			/*
			 * This might contain manX/ or catX/.
			 * Try to infer this from the name.
			 * If we're not in use_all, enforce it.
			 */
			dsec = NULL;
			dform = FORM_NONE;
			cp = ff->fts_name;
			if (FTS_DP == ff->fts_info)
				break;

			if (0 == strncmp(cp, "man", 3)) {
				dform = FORM_SRC;
				dsec = stradd(cp + 3);
			} else if (0 == strncmp(cp, "cat", 3)) {
				dform = FORM_CAT;
				dsec = stradd(cp + 3);
			}

			if (NULL != dsec || use_all) 
				break;

			if (warnings)
				say(path, "Unknown directory part");
			fts_set(f, ff, FTS_SKIP);
			break;
		case (2):
			/*
			 * Possibly our architecture.
			 * If we're descending, keep tabs on it.
			 */
			arch = NULL;
			if (FTS_DP != ff->fts_info && NULL != dsec)
				arch = stradd(ff->fts_name);
			break;
		default:
			if (FTS_DP == ff->fts_info || use_all)
				break;
			if (warnings)
				say(path, "Extraneous directory part");
			fts_set(f, ff, FTS_SKIP);
			break;
		}
	}

	fts_close(f);
	return(1);
}

/*
 * Add a file to the file vector.
 * Do not verify that it's a "valid" looking manpage (we'll do that
 * later).
 *
 * Try to infer the manual section, architecture, and page name from the
 * path, assuming it looks like
 *
 *   [./]man*[/<arch>]/<name>.<section> 
 *   or
 *   [./]cat<section>[/<arch>]/<name>.0
 *
 * Stuff this information directly into the "of" vector.
 * See treescan() for the fts(3) version of this.
 */
static void
filescan(const char *file)
{
	char		 buf[PATH_MAX];
	const char	*sec, *arch, *name, *dsec;
	char		*p, *start;
	int		 dform;
	struct stat	 st;

	assert(use_all);

	if (0 == strncmp(file, "./", 2))
		file += 2;

	if (NULL == realpath(file, buf)) {
		exitcode = (int)MANDOCLEVEL_BADARG;
		say(file, NULL);
		return;
	} else if (strstr(buf, basedir) != buf) {
		exitcode = (int)MANDOCLEVEL_BADARG;
		say("", "%s: outside base directory", buf);
		return;
	} else if (-1 == stat(buf, &st)) {
		exitcode = (int)MANDOCLEVEL_BADARG;
		say(file, NULL);
		return;
	} else if ( ! (S_IFREG & st.st_mode)) {
		exitcode = (int)MANDOCLEVEL_BADARG;
		say(file, "Not a regular file");
		return;
	} else if (inocheck(&st)) {
		if (warnings)
			say(file, "Duplicate file");
		return;
	}
	start = buf + strlen(basedir);
	sec = arch = name = dsec = NULL;
	dform = FORM_NONE;

	/*
	 * First try to guess our directory structure.
	 * If we find a separator, try to look for man* or cat*.
	 * If we find one of these and what's underneath is a directory,
	 * assume it's an architecture.
	 */
	if (NULL != (p = strchr(start, '/'))) {
		*p++ = '\0';
		if (0 == strncmp(start, "man", 3)) {
			dform = FORM_SRC;
			dsec = start + 3;
		} else if (0 == strncmp(start, "cat", 3)) {
			dform = FORM_CAT;
			dsec = start + 3;
		}

		start = p;
		if (NULL != dsec && NULL != (p = strchr(start, '/'))) {
			*p++ = '\0';
			arch = start;
			start = p;
		} 
	}

	/*
	 * Now check the file suffix.
	 * Suffix of `.0' indicates a catpage, `.1-9' is a manpage.
	 */
	p = strrchr(start, '\0');
	while (p-- > start && '/' != *p && '.' != *p)
		/* Loop. */ ;

	if ('.' == *p) {
		*p++ = '\0';
		sec = p;
	}

	/*
	 * Now try to parse the name.
	 * Use the filename portion of the path.
	 */
	name = start;
	if (NULL != (p = strrchr(start, '/'))) {
		name = p + 1;
		*p = '\0';
	} 

	ofadd(dform, file, name, dsec, sec, arch, &st);
}

/*
 * See fileadd(). 
 */
static int
filecheck(const char *name)
{
	unsigned int	 index;

	index = ohash_qlookup(&filenames, name);
	return(NULL != ohash_find(&filenames, index));
}

/*
 * Use the standard hashing mechanism (K&R) to see if the given filename
 * already exists.
 */
static void
fileadd(struct of *of)
{
	unsigned int	 index;

	index = ohash_qlookup(&filenames, of->file);
	assert(NULL == ohash_find(&filenames, index));
	ohash_insert(&filenames, index, of);
}

/*
 * See inoadd().
 */
static int
inocheck(const struct stat *st)
{
	struct id	 id;
	uint32_t	 hash;
	unsigned int	 index;

	memset(&id, 0, sizeof(id));
	id.ino = hash = st->st_ino;
	id.dev = st->st_dev;
	index = ohash_lookup_memory
		(&inos, (char *)&id, sizeof(id), hash);

	return(NULL != ohash_find(&inos, index));
}

/*
 * The hashing function used here is quite simple: simply take the inode
 * and use uint32_t of its bits.
 * Then when we do the lookup, use both the inode and device identifier.
 */
static void
inoadd(const struct stat *st, struct of *of)
{
	uint32_t	 hash;
	unsigned int	 index;

	of->id.ino = hash = st->st_ino;
	of->id.dev = st->st_dev;
	index = ohash_lookup_memory
		(&inos, (char *)&of->id, sizeof(of->id), hash);

	assert(NULL == ohash_find(&inos, index));
	ohash_insert(&inos, index, of);
}

static void
ofadd(int dform, const char *file, const char *name, const char *dsec,
	const char *sec, const char *arch, const struct stat *st)
{
	struct of	*of;
	int		 sform;

	assert(NULL != file);

	if (NULL == name)
		name = "";
	if (NULL == sec)
		sec = "";
	if (NULL == dsec)
		dsec = "";
	if (NULL == arch)
		arch = "";

	sform = FORM_NONE;
	if (NULL != sec && *sec <= '9' && *sec >= '1')
		sform = FORM_SRC;
	else if (NULL != sec && *sec == '0') {
		sec = dsec;
		sform = FORM_CAT;
	}

	of = mandoc_calloc(1, sizeof(struct of));
	strlcpy(of->file, file, PATH_MAX);
	of->name = name;
	of->sec = sec;
	of->dsec = dsec;
	of->arch = arch;
	of->sform = sform;
	of->dform = dform;
	of->next = ofs;
	ofs = of;

	/*
	 * Add to unique identifier hash.
	 * Then if it's a source manual and we're going to use source in
	 * favour of catpages, add it to that hash.
	 */
	inoadd(st, of);
	fileadd(of);
}

static void
offree(void)
{
	struct of	*of;

	while (NULL != (of = ofs)) {
		ofs = of->next;
		free(of);
	}
}

/*
 * Run through the files in the global vector "ofs" and add them to the
 * database specified in "basedir".
 *
 * This handles the parsing scheme itself, using the cues of directory
 * and filename to determine whether the file is parsable or not.
 */
static void
ofmerge(struct mchars *mc, struct mparse *mp)
{
	int		 form;
	size_t		 sz;
	struct mdoc	*mdoc;
	struct man	*man;
	char		 buf[PATH_MAX];
	char		*bufp;
	const char	*msec, *march, *mtitle, *cp;
	struct of	*of;
	enum mandoclevel lvl;

	for (of = ofs; NULL != of; of = of->next) {
		/*
		 * If we're a catpage (as defined by our path), then see
		 * if a manpage exists by the same name (ignoring the
		 * suffix).
		 * If it does, then we want to use it instead of our
		 * own.
		 */
		if ( ! use_all && FORM_CAT == of->dform) {
			sz = strlcpy(buf, of->file, PATH_MAX);
			if (sz >= PATH_MAX) {
				if (warnings)
					say(of->file, "Filename too long");
				continue;
			}
			bufp = strstr(buf, "cat");
			assert(NULL != bufp);
			memcpy(bufp, "man", 3);
			if (NULL != (bufp = strrchr(buf, '.')))
				*++bufp = '\0';
			strlcat(buf, of->dsec, PATH_MAX);
			if (filecheck(buf)) {
				if (warnings)
					say(of->file, "Man "
					    "source exists: %s", buf);
				continue;
			}
		}

		words = NULL;
		mparse_reset(mp);
		mdoc = NULL;
		man = NULL;
		form = 0;
		msec = of->dsec;
		march = of->arch;
		mtitle = of->name;

		/*
		 * Try interpreting the file as mdoc(7) or man(7)
		 * source code, unless it is already known to be
		 * formatted.  Fall back to formatted mode.
		 */
		if (FORM_SRC == of->dform || FORM_SRC == of->sform) {
			lvl = mparse_readfd(mp, -1, of->file);
			if (lvl < MANDOCLEVEL_FATAL)
				mparse_result(mp, &mdoc, &man);
		} 

		if (NULL != mdoc) {
			form = 1;
			msec = mdoc_meta(mdoc)->msec;
			march = mdoc_meta(mdoc)->arch;
			mtitle = mdoc_meta(mdoc)->title;
		} else if (NULL != man) {
			form = 1;
			msec = man_meta(man)->msec;
			march = "";
			mtitle = man_meta(man)->title;
		} 

		if (NULL == msec) 
			msec = "";
		if (NULL == march) 
			march = "";
		if (NULL == mtitle) 
			mtitle = "";

		/*
		 * Check whether the manual section given in a file
		 * agrees with the directory where the file is located.
		 * Some manuals have suffixes like (3p) on their
		 * section number either inside the file or in the
		 * directory name, some are linked into more than one
		 * section, like encrypt(1) = makekey(8).  Do not skip
		 * manuals for such reasons.
		 */
		if (warnings && !use_all && form &&
				strcasecmp(msec, of->dsec))
			say(of->file, "Section \"%s\" "
				"manual in %s directory", 
				msec, of->dsec);

		/*
		 * Manual page directories exist for each kernel
		 * architecture as returned by machine(1).
		 * However, many manuals only depend on the
		 * application architecture as returned by arch(1).
		 * For example, some (2/ARM) manuals are shared
		 * across the "armish" and "zaurus" kernel
		 * architectures.
		 * A few manuals are even shared across completely
		 * different architectures, for example fdformat(1)
		 * on amd64, i386, sparc, and sparc64.
		 * Thus, warn about architecture mismatches,
		 * but don't skip manuals for this reason.
		 */
		if (warnings && !use_all && strcasecmp(march, of->arch))
			say(of->file, "Architecture \"%s\" "
				"manual in \"%s\" directory",
				march, of->arch);

		putkey(of, of->name, TYPE_Nm);

		if (NULL != mdoc) {
			if (NULL != (cp = mdoc_meta(mdoc)->name))
				putkey(of, cp, TYPE_Nm);
			parse_mdoc(of, mdoc_node(mdoc));
		} else if (NULL != man)
			parse_man(of, man_node(man));
		else
			parse_catpage(of);

		dbindex(mc, form, of);
	}
}

static void
parse_catpage(struct of *of)
{
	FILE		*stream;
	char		*line, *p, *title;
	size_t		 len, plen, titlesz;

	if (NULL == (stream = fopen(of->file, "r"))) {
		if (warnings)
			say(of->file, NULL);
		return;
	}

	/* Skip to first blank line. */

	while (NULL != (line = fgetln(stream, &len)))
		if ('\n' == *line)
			break;

	/*
	 * Assume the first line that is not indented
	 * is the first section header.  Skip to it.
	 */

	while (NULL != (line = fgetln(stream, &len)))
		if ('\n' != *line && ' ' != *line)
			break;
	
	/*
	 * Read up until the next section into a buffer.
	 * Strip the leading and trailing newline from each read line,
	 * appending a trailing space.
	 * Ignore empty (whitespace-only) lines.
	 */

	titlesz = 0;
	title = NULL;

	while (NULL != (line = fgetln(stream, &len))) {
		if (' ' != *line || '\n' != line[len - 1])
			break;
		while (len > 0 && isspace((unsigned char)*line)) {
			line++;
			len--;
		}
		if (1 == len)
			continue;
		title = mandoc_realloc(title, titlesz + len);
		memcpy(title + titlesz, line, len);
		titlesz += len;
		title[titlesz - 1] = ' ';
	}

	/*
	 * If no page content can be found, or the input line
	 * is already the next section header, or there is no
	 * trailing newline, reuse the page title as the page
	 * description.
	 */

	if (NULL == title || '\0' == *title) {
		if (warnings)
			say(of->file, "Cannot find NAME section");
		fclose(stream);
		free(title);
		return;
	}

	title = mandoc_realloc(title, titlesz + 1);
	title[titlesz] = '\0';

	/*
	 * Skip to the first dash.
	 * Use the remaining line as the description (no more than 70
	 * bytes).
	 */

	if (NULL != (p = strstr(title, "- "))) {
		for (p += 2; ' ' == *p || '\b' == *p; p++)
			/* Skip to next word. */ ;
	} else {
		if (warnings)
			say(of->file, "No dash in title line");
		p = title;
	}

	plen = strlen(p);

	/* Strip backspace-encoding from line. */

	while (NULL != (line = memchr(p, '\b', plen))) {
		len = line - p;
		if (0 == len) {
			memmove(line, line + 1, plen--);
			continue;
		} 
		memmove(line - 1, line + 1, plen - len);
		plen -= 2;
	}

	of->desc = stradd(p);
	putkey(of, p, TYPE_Nd);
	fclose(stream);
	free(title);
}

/*
 * Put a type/word pair into the word database for this particular file.
 */
static void
putkey(const struct of *of, const char *value, uint64_t type)
{

	assert(NULL != value);
	wordaddbuf(of, value, strlen(value), type);
}

/*
 * Like putkey() but for unterminated strings.
 */
static void
putkeys(const struct of *of, const char *value, int sz, uint64_t type)
{

	wordaddbuf(of, value, sz, type);
}

/*
 * Grok all nodes at or below a certain mdoc node into putkey().
 */
static void
putmdockey(const struct of *of, const struct mdoc_node *n, uint64_t m)
{

	for ( ; NULL != n; n = n->next) {
		if (NULL != n->child)
			putmdockey(of, n->child, m);
		if (MDOC_TEXT == n->type)
			putkey(of, n->string, m);
	}
}

static void
parse_man(struct of *of, const struct man_node *n)
{
	const struct man_node *head, *body;
	char		*start, *sv, *title;
	char		 byte;
	size_t		 sz, titlesz;

	if (NULL == n)
		return;

	/*
	 * We're only searching for one thing: the first text child in
	 * the BODY of a NAME section.  Since we don't keep track of
	 * sections in -man, run some hoops to find out whether we're in
	 * the correct section or not.
	 */

	if (MAN_BODY == n->type && MAN_SH == n->tok) {
		body = n;
		assert(body->parent);
		if (NULL != (head = body->parent->head) &&
				1 == head->nchild &&
				NULL != (head = (head->child)) &&
				MAN_TEXT == head->type &&
				0 == strcmp(head->string, "NAME") &&
				NULL != (body = body->child) &&
				MAN_TEXT == body->type) {

			title = NULL;
			titlesz = 0;

			/*
			 * Suck the entire NAME section into memory.
			 * Yes, we might run away.
			 * But too many manuals have big, spread-out
			 * NAME sections over many lines.
			 */

			for ( ; NULL != body; body = body->next) {
				if (MAN_TEXT != body->type)
					break;
				if (0 == (sz = strlen(body->string)))
					continue;
				title = mandoc_realloc
					(title, titlesz + sz + 1);
				memcpy(title + titlesz, body->string, sz);
				titlesz += sz + 1;
				title[titlesz - 1] = ' ';
			}
			if (NULL == title)
				return;

			title = mandoc_realloc(title, titlesz + 1);
			title[titlesz] = '\0';

			/* Skip leading space.  */

			sv = title;
			while (isspace((unsigned char)*sv))
				sv++;

			if (0 == (sz = strlen(sv))) {
				free(title);
				return;
			}

			/* Erase trailing space. */

			start = &sv[sz - 1];
			while (start > sv && isspace((unsigned char)*start))
				*start-- = '\0';

			if (start == sv) {
				free(title);
				return;
			}

			start = sv;

			/* 
			 * Go through a special heuristic dance here.
			 * Conventionally, one or more manual names are
			 * comma-specified prior to a whitespace, then a
			 * dash, then a description.  Try to puzzle out
			 * the name parts here.
			 */

			for ( ;; ) {
				sz = strcspn(start, " ,");
				if ('\0' == start[sz])
					break;

				byte = start[sz];
				start[sz] = '\0';

				putkey(of, start, TYPE_Nm);

				if (' ' == byte) {
					start += sz + 1;
					break;
				}

				assert(',' == byte);
				start += sz + 1;
				while (' ' == *start)
					start++;
			}

			if (sv == start) {
				putkey(of, start, TYPE_Nm);
				free(title);
				return;
			}

			while (isspace((unsigned char)*start))
				start++;

			if (0 == strncmp(start, "-", 1))
				start += 1;
			else if (0 == strncmp(start, "\\-\\-", 4))
				start += 4;
			else if (0 == strncmp(start, "\\-", 2))
				start += 2;
			else if (0 == strncmp(start, "\\(en", 4))
				start += 4;
			else if (0 == strncmp(start, "\\(em", 4))
				start += 4;

			while (' ' == *start)
				start++;

			assert(NULL == of->desc);
			of->desc = stradd(start);
			putkey(of, start, TYPE_Nd);
			free(title);
			return;
		}
	}

	for (n = n->child; n; n = n->next)
		parse_man(of, n);
}

static void
parse_mdoc(struct of *of, const struct mdoc_node *n)
{

	assert(NULL != n);
	for (n = n->child; NULL != n; n = n->next) {
		switch (n->type) {
		case (MDOC_ELEM):
			/* FALLTHROUGH */
		case (MDOC_BLOCK):
			/* FALLTHROUGH */
		case (MDOC_HEAD):
			/* FALLTHROUGH */
		case (MDOC_BODY):
			/* FALLTHROUGH */
		case (MDOC_TAIL):
			if (NULL != mdocs[n->tok].fp)
			       if (0 == (*mdocs[n->tok].fp)(of, n))
				       break;

			if (MDOCF_CHILD & mdocs[n->tok].flags)
				putmdockey(of, n->child, mdocs[n->tok].mask);
			break;
		default:
			assert(MDOC_ROOT != n->type);
			continue;
		}
		if (NULL != n->child)
			parse_mdoc(of, n);
	}
}

static int
parse_mdoc_Fd(struct of *of, const struct mdoc_node *n)
{
	const char	*start, *end;
	size_t		 sz;

	if (SEC_SYNOPSIS != n->sec ||
			NULL == (n = n->child) || 
			MDOC_TEXT != n->type)
		return(0);

	/*
	 * Only consider those `Fd' macro fields that begin with an
	 * "inclusion" token (versus, e.g., #define).
	 */

	if (strcmp("#include", n->string))
		return(0);

	if (NULL == (n = n->next) || MDOC_TEXT != n->type)
		return(0);

	/*
	 * Strip away the enclosing angle brackets and make sure we're
	 * not zero-length.
	 */

	start = n->string;
	if ('<' == *start || '"' == *start)
		start++;

	if (0 == (sz = strlen(start)))
		return(0);

	end = &start[(int)sz - 1];
	if ('>' == *end || '"' == *end)
		end--;

	if (end > start)
		putkeys(of, start, end - start + 1, TYPE_In);
	return(1);
}

static int
parse_mdoc_In(struct of *of, const struct mdoc_node *n)
{

	if (NULL != n->child && MDOC_TEXT == n->child->type)
		return(0);

	putkey(of, n->child->string, TYPE_In);
	return(1);
}

static int
parse_mdoc_Fn(struct of *of, const struct mdoc_node *n)
{
	const char	*cp;

	if (NULL == (n = n->child) || MDOC_TEXT != n->type)
		return(0);

	/* 
	 * Parse: .Fn "struct type *name" "char *arg".
	 * First strip away pointer symbol. 
	 * Then store the function name, then type.
	 * Finally, store the arguments. 
	 */

	if (NULL == (cp = strrchr(n->string, ' ')))
		cp = n->string;

	while ('*' == *cp)
		cp++;

	putkey(of, cp, TYPE_Fn);

	if (n->string < cp)
		putkeys(of, n->string, cp - n->string, TYPE_Ft);

	for (n = n->next; NULL != n; n = n->next)
		if (MDOC_TEXT == n->type)
			putkey(of, n->string, TYPE_Fa);

	return(0);
}

static int
parse_mdoc_St(struct of *of, const struct mdoc_node *n)
{

	if (NULL == n->child || MDOC_TEXT != n->child->type)
		return(0);

	putkey(of, n->child->string, TYPE_St);
	return(1);
}

static int
parse_mdoc_Xr(struct of *of, const struct mdoc_node *n)
{

	if (NULL == (n = n->child))
		return(0);

	putkey(of, n->string, TYPE_Xr);
	return(1);
}

static int
parse_mdoc_Nd(struct of *of, const struct mdoc_node *n)
{
	size_t		 sz;
	char		*sv, *desc;

	if (MDOC_BODY != n->type)
		return(0);

	/*
	 * Special-case the `Nd' because we need to put the description
	 * into the document table.
	 */

	desc = NULL;
	for (n = n->child; NULL != n; n = n->next) {
		if (MDOC_TEXT == n->type) {
			sz = strlen(n->string) + 1;
			if (NULL != (sv = desc))
				sz += strlen(desc) + 1;
			desc = mandoc_realloc(desc, sz);
			if (NULL != sv)
				strlcat(desc, " ", sz);
			else
				*desc = '\0';
			strlcat(desc, n->string, sz);
		}
		if (NULL != n->child)
			parse_mdoc_Nd(of, n);
	}

	of->desc = NULL != desc ? stradd(desc) : NULL;
	free(desc);
	return(1);
}

static int
parse_mdoc_Nm(struct of *of, const struct mdoc_node *n)
{

	if (SEC_NAME == n->sec)
		return(1);
	else if (SEC_SYNOPSIS != n->sec || MDOC_HEAD != n->type)
		return(0);

	return(1);
}

static int
parse_mdoc_Sh(struct of *of, const struct mdoc_node *n)
{

	return(SEC_CUSTOM == n->sec && MDOC_HEAD == n->type);
}

static int
parse_mdoc_head(struct of *of, const struct mdoc_node *n)
{

	return(MDOC_HEAD == n->type);
}

static int
parse_mdoc_body(struct of *of, const struct mdoc_node *n)
{

	return(MDOC_BODY == n->type);
}

/*
 * See straddbuf().
 */
static char *
stradd(const char *cp)
{

	return(straddbuf(cp, strlen(cp)));
}

/*
 * This looks up or adds a string to the string table.
 * The string table is a table of all strings encountered during parse
 * or file scan.
 * In using it, we avoid having thousands of (e.g.) "cat1" string
 * allocations for the "of" table.
 * We also have a layer atop the string table for keeping track of words
 * in a parse sequence (see wordaddbuf()).
 */
static char *
straddbuf(const char *cp, size_t sz)
{
	struct str	*s;
	unsigned int	 index;
	const char	*end;

	if (NULL != (s = hashget(cp, sz)))
		return(s->key);

	s = mandoc_calloc(sizeof(struct str) + sz + 1, 1);
	memcpy(s->key, cp, sz);

	end = cp + sz;
	index = ohash_qlookupi(&strings, cp, &end);
	assert(NULL == ohash_find(&strings, index));
	ohash_insert(&strings, index, s);
	return(s->key);
}

static struct str *
hashget(const char *cp, size_t sz)
{
	unsigned int	 index;
	const char	*end;

	end = cp + sz;
	index = ohash_qlookupi(&strings, cp, &end);
	return(ohash_find(&strings, index));
}

/*
 * Add a word to the current parse sequence.
 * Within the hashtable of strings, we maintain a list of strings that
 * are currently indexed.
 * Each of these ("words") has a bitmask modified within the parse.
 * When we finish a parse, we'll dump the list, then remove the head
 * entry -- since the next parse will have a new "of", it can keep track
 * of its entries without conflict.
 */
static void
wordaddbuf(const struct of *of, 
		const char *cp, size_t sz, uint64_t v)
{
	struct str	*s;
	unsigned int	 index;
	const char	*end;

	if (0 == sz)
		return;

	s = hashget(cp, sz);

	if (NULL != s && of == s->of) {
		s->mask |= v;
		return;
	} else if (NULL == s) {
		s = mandoc_calloc(sizeof(struct str) + sz + 1, 1);
		memcpy(s->key, cp, sz);
		end = cp + sz;
		index = ohash_qlookupi(&strings, cp, &end);
		assert(NULL == ohash_find(&strings, index));
		ohash_insert(&strings, index, s);
	}

	s->next = words;
	s->of = of;
	s->mask = v;
	words = s;
}

/*
 * Take a Unicode codepoint and produce its UTF-8 encoding.
 * This isn't the best way to do this, but it works.
 * The magic numbers are from the UTF-8 packaging.
 * They're not as scary as they seem: read the UTF-8 spec for details.
 */
static size_t
utf8(unsigned int cp, char out[7])
{
	size_t		 rc;

	rc = 0;
	if (cp <= 0x0000007F) {
		rc = 1;
		out[0] = (char)cp;
	} else if (cp <= 0x000007FF) {
		rc = 2;
		out[0] = (cp >> 6  & 31) | 192;
		out[1] = (cp       & 63) | 128;
	} else if (cp <= 0x0000FFFF) {
		rc = 3;
		out[0] = (cp >> 12 & 15) | 224;
		out[1] = (cp >> 6  & 63) | 128;
		out[2] = (cp       & 63) | 128;
	} else if (cp <= 0x001FFFFF) {
		rc = 4;
		out[0] = (cp >> 18 &  7) | 240;
		out[1] = (cp >> 12 & 63) | 128;
		out[2] = (cp >> 6  & 63) | 128;
		out[3] = (cp       & 63) | 128;
	} else if (cp <= 0x03FFFFFF) {
		rc = 5;
		out[0] = (cp >> 24 &  3) | 248;
		out[1] = (cp >> 18 & 63) | 128;
		out[2] = (cp >> 12 & 63) | 128;
		out[3] = (cp >> 6  & 63) | 128;
		out[4] = (cp       & 63) | 128;
	} else if (cp <= 0x7FFFFFFF) {
		rc = 6;
		out[0] = (cp >> 30 &  1) | 252;
		out[1] = (cp >> 24 & 63) | 128;
		out[2] = (cp >> 18 & 63) | 128;
		out[3] = (cp >> 12 & 63) | 128;
		out[4] = (cp >> 6  & 63) | 128;
		out[5] = (cp       & 63) | 128;
	} else
		return(0);

	out[rc] = '\0';
	return(rc);
}

/*
 * Store the UTF-8 version of a key, or alias the pointer if the key has
 * no UTF-8 transcription marks in it.
 */
static void
utf8key(struct mchars *mc, struct str *key)
{
	size_t		 sz, bsz, pos;
	char		 utfbuf[7], res[5];
	char		*buf;
	const char	*seq, *cpp, *val;
	int		 len, u;
	enum mandoc_esc	 esc;

	assert(NULL == key->utf8);

	res[0] = '\\';
	res[1] = '\t';
	res[2] = ASCII_NBRSP;
	res[3] = ASCII_HYPH;
	res[4] = '\0';

	val = key->key;
	bsz = strlen(val);

	/*
	 * Pre-check: if we have no stop-characters, then set the
	 * pointer as ourselvse and get out of here.
	 */
	if (strcspn(val, res) == bsz) {
		key->utf8 = key->key;
		return;
	} 

	/* Pre-allocate by the length of the input */

	buf = mandoc_malloc(++bsz);
	pos = 0;

	while ('\0' != *val) {
		/*
		 * Halt on the first escape sequence.
		 * This also halts on the end of string, in which case
		 * we just copy, fallthrough, and exit the loop.
		 */
		if ((sz = strcspn(val, res)) > 0) {
			memcpy(&buf[pos], val, sz);
			pos += sz;
			val += sz;
		}

		if (ASCII_HYPH == *val) {
			buf[pos++] = '-';
			val++;
			continue;
		} else if ('\t' == *val || ASCII_NBRSP == *val) {
			buf[pos++] = ' ';
			val++;
			continue;
		} else if ('\\' != *val)
			break;

		/* Read past the slash. */

		val++;
		u = 0;

		/*
		 * Parse the escape sequence and see if it's a
		 * predefined character or special character.
		 */
		esc = mandoc_escape
			((const char **)&val, &seq, &len);
		if (ESCAPE_ERROR == esc)
			break;

		if (ESCAPE_SPECIAL != esc)
			continue;
		if (0 == (u = mchars_spec2cp(mc, seq, len)))
			continue;

		/*
		 * If we have a Unicode codepoint, try to convert that
		 * to a UTF-8 byte string.
		 */
		cpp = utfbuf;
		if (0 == (sz = utf8(u, utfbuf)))
			continue;

		/* Copy the rendered glyph into the stream. */

		sz = strlen(cpp);
		bsz += sz;

		buf = mandoc_realloc(buf, bsz);

		memcpy(&buf[pos], cpp, sz);
		pos += sz;
	}

	buf[pos] = '\0';
	key->utf8 = buf;
}

/*
 * Flush the current page's terms (and their bits) into the database.
 * Wrap the entire set of additions in a transaction to make sqlite be a
 * little faster.
 * Also, UTF-8-encode the description at the last possible moment.
 */
static void
dbindex(struct mchars *mc, int form, const struct of *of)
{
	struct str	*key;
	const char	*desc;
	int64_t		 recno;
	size_t		 i;

	if (verb)
		say(of->file, "Adding to index");

	if (nodb)
		return;

	desc = "";
	if (NULL != of->desc) {
		key = hashget(of->desc, strlen(of->desc));
		assert(NULL != key);
		if (NULL == key->utf8)
			utf8key(mc, key);
		desc = key->utf8;
	}

	SQL_EXEC("BEGIN TRANSACTION");

	i = 1;
	SQL_BIND_TEXT(stmts[STMT_INSERT_DOC], i, of->file);
	SQL_BIND_TEXT(stmts[STMT_INSERT_DOC], i, of->sec);
	SQL_BIND_TEXT(stmts[STMT_INSERT_DOC], i, of->arch);
	SQL_BIND_TEXT(stmts[STMT_INSERT_DOC], i, desc);
	SQL_BIND_INT(stmts[STMT_INSERT_DOC], i, form);
	SQL_STEP(stmts[STMT_INSERT_DOC]);
	recno = sqlite3_last_insert_rowid(db);
	sqlite3_reset(stmts[STMT_INSERT_DOC]);

	for (key = words; NULL != key; key = key->next) {
		assert(key->of == of);
		if (NULL == key->utf8)
			utf8key(mc, key);
		i = 1;
		SQL_BIND_INT64(stmts[STMT_INSERT_KEY], i, key->mask);
		SQL_BIND_TEXT(stmts[STMT_INSERT_KEY], i, key->utf8);
		SQL_BIND_INT64(stmts[STMT_INSERT_KEY], i, recno);
		SQL_STEP(stmts[STMT_INSERT_KEY]);
		sqlite3_reset(stmts[STMT_INSERT_KEY]);
	}

	SQL_EXEC("END TRANSACTION");
}

static void
dbprune(void)
{
	struct of	*of;
	size_t		 i;

	if (nodb)
		return;

	for (of = ofs; NULL != of; of = of->next) {
		i = 1;
		SQL_BIND_TEXT(stmts[STMT_DELETE], i, of->file);
		SQL_STEP(stmts[STMT_DELETE]);
		sqlite3_reset(stmts[STMT_DELETE]);
		if (verb)
			say(of->file, "Deleted from index");
	}
}

/*
 * Close an existing database and its prepared statements.
 * If "real" is not set, rename the temporary file into the real one.
 */
static void
dbclose(int real)
{
	size_t		 i;
	char		 file[PATH_MAX];

	if (nodb)
		return;

	for (i = 0; i < STMT__MAX; i++) {
		sqlite3_finalize(stmts[i]);
		stmts[i] = NULL;
	}

	sqlite3_close(db);
	db = NULL;

	if (real)
		return;

	strlcpy(file, MANDOC_DB, PATH_MAX);
	strlcat(file, "~", PATH_MAX);
	if (-1 == rename(file, MANDOC_DB)) {
		exitcode = (int)MANDOCLEVEL_SYSERR;
		say(MANDOC_DB, NULL);
	}
}

/*
 * This is straightforward stuff.
 * Open a database connection to a "temporary" database, then open a set
 * of prepared statements we'll use over and over again.
 * If "real" is set, we use the existing database; if not, we truncate a
 * temporary one.
 * Must be matched by dbclose().
 */
static int
dbopen(int real)
{
	char		 file[PATH_MAX];
	const char	*sql;
	int		 rc, ofl;
	size_t		 sz;

	if (nodb) 
		return(1);

	sz = strlcpy(file, MANDOC_DB, PATH_MAX);
	if ( ! real)
		sz = strlcat(file, "~", PATH_MAX);

	if (sz >= PATH_MAX) {
		fprintf(stderr, "%s: Path too long\n", file);
		return(0);
	}

	if ( ! real)
		remove(file);

	ofl = SQLITE_OPEN_READWRITE | 
		(0 == real ? SQLITE_OPEN_EXCLUSIVE : 0);

	rc = sqlite3_open_v2(file, &db, ofl, NULL);
	if (SQLITE_OK == rc) 
		goto prepare_statements;
	if (SQLITE_CANTOPEN != rc) {
		exitcode = (int)MANDOCLEVEL_SYSERR;
		say(file, NULL);
		return(0);
	}

	sqlite3_close(db);
	db = NULL;

	if (SQLITE_OK != (rc = sqlite3_open(file, &db))) {
		exitcode = (int)MANDOCLEVEL_SYSERR;
		say(file, NULL);
		return(0);
	}

	sql = "CREATE TABLE \"docs\" (\n"
	      " \"file\" TEXT NOT NULL,\n"
	      " \"sec\" TEXT NOT NULL,\n"
	      " \"arch\" TEXT NOT NULL,\n"
	      " \"desc\" TEXT NOT NULL,\n"
	      " \"form\" INTEGER NOT NULL,\n"
	      " \"id\" INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL\n"
	      ");\n"
	      "\n"
	      "CREATE TABLE \"keys\" (\n"
	      " \"bits\" INTEGER NOT NULL,\n"
	      " \"key\" TEXT NOT NULL,\n"
	      " \"docid\" INTEGER NOT NULL REFERENCES docs(id) "
	      	"ON DELETE CASCADE,\n"
	      " \"id\" INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL\n"
	      ");\n"
	      "\n"
	      "CREATE INDEX \"key_index\" ON keys (key);\n";

	if (SQLITE_OK != sqlite3_exec(db, sql, NULL, NULL, NULL)) {
		exitcode = (int)MANDOCLEVEL_SYSERR;
		say(file, "%s", sqlite3_errmsg(db));
		return(0);
	}

prepare_statements:
	SQL_EXEC("PRAGMA foreign_keys = ON");
	sql = "DELETE FROM docs where file=?";
	sqlite3_prepare_v2(db, sql, -1, &stmts[STMT_DELETE], NULL);
	sql = "INSERT INTO docs "
		"(file,sec,arch,desc,form) VALUES (?,?,?,?,?)";
	sqlite3_prepare_v2(db, sql, -1, &stmts[STMT_INSERT_DOC], NULL);
	sql = "INSERT INTO keys "
		"(bits,key,docid) VALUES (?,?,?)";
	sqlite3_prepare_v2(db, sql, -1, &stmts[STMT_INSERT_KEY], NULL);
	return(1);
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

static int
set_basedir(const char *targetdir)
{
	static char	 startdir[PATH_MAX];
	static int	 fd;

	/*
	 * Remember where we started by keeping a fd open to the origin
	 * path component: throughout this utility, we chdir() a lot to
	 * handle relative paths, and by doing this, we can return to
	 * the starting point.
	 */
	if ('\0' == *startdir) {
		if (NULL == getcwd(startdir, PATH_MAX)) {
			exitcode = (int)MANDOCLEVEL_SYSERR;
			if (NULL != targetdir)
				say(".", NULL);
			return(0);
		}
		if (-1 == (fd = open(startdir, O_RDONLY, 0))) {
			exitcode = (int)MANDOCLEVEL_SYSERR;
			say(startdir, NULL);
			return(0);
		}
		if (NULL == targetdir)
			targetdir = startdir;
	} else {
		if (-1 == fd)
			return(0);
		if (-1 == fchdir(fd)) {
			close(fd);
			basedir[0] = '\0';
			exitcode = (int)MANDOCLEVEL_SYSERR;
			say(startdir, NULL);
			return(0);
		}
		if (NULL == targetdir) {
			close(fd);
			return(1);
		}
	}
	if (NULL == realpath(targetdir, basedir)) {
		basedir[0] = '\0';
		exitcode = (int)MANDOCLEVEL_BADARG;
		say(targetdir, NULL);
		return(0);
	} else if (-1 == chdir(basedir)) {
		exitcode = (int)MANDOCLEVEL_BADARG;
		say("", NULL);
		return(0);
	}
	return(1);
}

static void
say(const char *file, const char *format, ...)
{
	va_list		 ap;

	if ('\0' != *basedir)
		fprintf(stderr, "%s", basedir);
	if ('\0' != *basedir && '\0' != *file)
		fputs("//", stderr);
	if ('\0' != *file)
		fprintf(stderr, "%s", file);
	fputs(": ", stderr);

	if (NULL == format) {
		perror(NULL);
		return;
	}

	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);

	fputc('\n', stderr);
}
