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
#include <assert.h>
#include <fcntl.h>
#include <regex.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#ifdef __linux__
# include <db_185.h>
#else
# include <db.h>
#endif

#include "mandocdb.h"
#include "apropos_db.h"
#include "mandoc.h"

struct	rec {
	struct res	 res; /* resulting record info */
	/*
	 * Maintain a binary tree for checking the uniqueness of `rec'
	 * when adding elements to the results array.
	 * Since the results array is dynamic, use offset in the array
	 * instead of a pointer to the structure.
	 */
	int		 lhs;
	int		 rhs;
	int		 matched; /* expression is true */
	int		*matches; /* partial truth evaluations */
};

struct	expr {
	int		 regex; /* is regex? */
	int		 index; /* index in match array */
	int	 	 mask; /* type-mask */
	int		 cs; /* is case-sensitive? */
	int		 and; /* is rhs of logical AND? */
	char		*v; /* search value */
	regex_t	 	 re; /* compiled re, if regex */
	struct expr	*next; /* next in sequence */
	struct expr	*subexpr;
};

struct	type {
	int		 mask;
	const char	*name;
};

static	const struct type types[] = {
	{ TYPE_An, "An" },
	{ TYPE_Cd, "Cd" },
	{ TYPE_Er, "Er" },
	{ TYPE_Ev, "Ev" },
	{ TYPE_Fn, "Fn" },
	{ TYPE_Fn, "Fo" },
	{ TYPE_In, "In" },
	{ TYPE_Nd, "Nd" },
	{ TYPE_Nm, "Nm" },
	{ TYPE_Pa, "Pa" },
	{ TYPE_St, "St" },
	{ TYPE_Va, "Va" },
	{ TYPE_Va, "Vt" },
	{ TYPE_Xr, "Xr" },
	{ INT_MAX, "any" },
	{ 0, NULL }
};

static	DB	*btree_open(void);
static	int	 btree_read(const DBT *, 
			const struct mchars *, char **);
static	int	 expreval(const struct expr *, int *);
static	void	 exprexec(const struct expr *, 
			const char *, int, struct rec *);
static	int	 exprmark(const struct expr *, 
			const char *, int, int *);
static	struct expr *exprexpr(int, char *[], int *, int *, size_t *);
static	struct expr *exprterm(char *, int);
static	DB	*index_open(void);
static	int	 index_read(const DBT *, const DBT *, 
			const struct mchars *, struct rec *);
static	void	 norm_string(const char *,
			const struct mchars *, char **);
static	size_t	 norm_utf8(unsigned int, char[7]);
static	void	 recfree(struct rec *);

/*
 * Open the keyword mandoc-db database.
 */
static DB *
btree_open(void)
{
	BTREEINFO	 info;
	DB		*db;

	memset(&info, 0, sizeof(BTREEINFO));
	info.flags = R_DUP;

	db = dbopen(MANDOC_DB, O_RDONLY, 0, DB_BTREE, &info);
	if (NULL != db) 
		return(db);

	return(NULL);
}

/*
 * Read a keyword from the database and normalise it.
 * Return 0 if the database is insane, else 1.
 */
static int
btree_read(const DBT *v, const struct mchars *mc, char **buf)
{

	/* Sanity: are we nil-terminated? */

	assert(v->size > 0);
	if ('\0' != ((char *)v->data)[(int)v->size - 1])
		return(0);

	norm_string((char *)v->data, mc, buf);
	return(1);
}

/*
 * Take a Unicode codepoint and produce its UTF-8 encoding.
 * This isn't the best way to do this, but it works.
 * The magic numbers are from the UTF-8 packaging.  
 * They're not as scary as they seem: read the UTF-8 spec for details.
 */
static size_t
norm_utf8(unsigned int cp, char out[7])
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
		out[0] = (cp >> 18 & 7) | 240;
		out[1] = (cp >> 12 & 63) | 128;
		out[2] = (cp >> 6  & 63) | 128;
		out[3] = (cp       & 63) | 128;
	} else if (cp <= 0x03FFFFFF) {
		rc = 5;
		out[0] = (cp >> 24 & 3) | 248;
		out[1] = (cp >> 18 & 63) | 128;
		out[2] = (cp >> 12 & 63) | 128;
		out[3] = (cp >> 6  & 63) | 128;
		out[4] = (cp       & 63) | 128;
	} else if (cp <= 0x7FFFFFFF) {
		rc = 6;
		out[0] = (cp >> 30 & 1) | 252;
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
 * Normalise strings from the index and database.
 * These strings are escaped as defined by mandoc_char(7) along with
 * other goop in mandoc.h (e.g., soft hyphens).
 * This function normalises these into a nice UTF-8 string.
 * Returns 0 if the database is fucked.
 */
static void
norm_string(const char *val, const struct mchars *mc, char **buf)
{
	size_t		  sz, bsz;
	char		  utfbuf[7];
	const char	 *seq, *cpp;
	int		  len, u, pos;
	enum mandoc_esc	  esc;
	static const char res[] = { '\\', '\t', 
				ASCII_NBRSP, ASCII_HYPH, '\0' };

	/* Pre-allocate by the length of the input */

	bsz = strlen(val) + 1;
	*buf = mandoc_realloc(*buf, bsz);
	pos = 0;

	while ('\0' != *val) {
		/*
		 * Halt on the first escape sequence.
		 * This also halts on the end of string, in which case
		 * we just copy, fallthrough, and exit the loop.
		 */
		if ((sz = strcspn(val, res)) > 0) {
			memcpy(&(*buf)[pos], val, sz);
			pos += (int)sz;
			val += (int)sz;
		}

		if (ASCII_HYPH == *val) {
			(*buf)[pos++] = '-';
			val++;
			continue;
		} else if ('\t' == *val || ASCII_NBRSP == *val) {
			(*buf)[pos++] = ' ';
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

		esc = mandoc_escape(&val, &seq, &len);
		if (ESCAPE_ERROR == esc)
			break;

		/* 
		 * XXX - this just does UTF-8, but we need to know
		 * beforehand whether we should do text substitution.
		 */

		switch (esc) {
		case (ESCAPE_SPECIAL):
			if (0 != (u = mchars_spec2cp(mc, seq, len)))
				break;
			/* FALLTHROUGH */
		default:
			continue;
		}

		/*
		 * If we have a Unicode codepoint, try to convert that
		 * to a UTF-8 byte string.
		 */

		cpp = utfbuf;
		if (0 == (sz = norm_utf8(u, utfbuf)))
			continue;

		/* Copy the rendered glyph into the stream. */

		sz = strlen(cpp);
		bsz += sz;

		*buf = mandoc_realloc(*buf, bsz);

		memcpy(&(*buf)[pos], cpp, sz);
		pos += (int)sz;
	}

	(*buf)[pos] = '\0';
}

/*
 * Open the filename-index mandoc-db database.
 * Returns NULL if opening failed.
 */
static DB *
index_open(void)
{
	DB		*db;

	db = dbopen(MANDOC_IDX, O_RDONLY, 0, DB_RECNO, NULL);
	if (NULL != db)
		return(db);

	return(NULL);
}

/*
 * Safely unpack from an index file record into the structure.
 * Returns 1 if an entry was unpacked, 0 if the database is insane.
 */
static int
index_read(const DBT *key, const DBT *val, 
		const struct mchars *mc, struct rec *rec)
{
	size_t		 left;
	char		*np, *cp;

#define	INDEX_BREAD(_dst) \
	do { \
		if (NULL == (np = memchr(cp, '\0', left))) \
			return(0); \
		norm_string(cp, mc, &(_dst)); \
		left -= (np - cp) + 1; \
		cp = np + 1; \
	} while (/* CONSTCOND */ 0)

	left = val->size;
	cp = (char *)val->data;

	rec->res.rec = *(recno_t *)key->data;

	INDEX_BREAD(rec->res.file);
	INDEX_BREAD(rec->res.cat);
	INDEX_BREAD(rec->res.title);
	INDEX_BREAD(rec->res.arch);
	INDEX_BREAD(rec->res.desc);
	return(1);
}

/*
 * Search the mandocdb database for the expression "expr".
 * Filter out by "opts".
 * Call "res" with the results, which may be zero.
 * Return 0 if there was a database error, else return 1.
 */
int
apropos_search(const struct opts *opts, const struct expr *expr,
		size_t terms, void *arg, 
		void (*res)(struct res *, size_t, void *))
{
	int		 i, rsz, root, leaf, mask, mlen, rc, ch;
	DBT		 key, val;
	DB		*btree, *idx;
	struct mchars	*mc;
	char		*buf;
	recno_t		 rec;
	struct rec	*rs;
	struct res	*ress;
	struct rec	 r;

	rc	= 0;
	root	= -1;
	leaf	= -1;
	btree	= NULL;
	idx	= NULL;
	mc	= NULL;
	buf	= NULL;
	rs	= NULL;
	rsz	= 0;

	memset(&r, 0, sizeof(struct rec));

	mc = mchars_alloc();

	if (NULL == (btree = btree_open())) 
		goto out;
	if (NULL == (idx = index_open())) 
		goto out;

	while (0 == (ch = (*btree->seq)(btree, &key, &val, R_NEXT))) {
		/* 
		 * Low-water mark for key and value.
		 * The key must have something in it, and the value must
		 * have the correct tags/recno mix.
		 */
		if (key.size < 2 || 8 != val.size) 
			break;
		if ( ! btree_read(&key, mc, &buf))
			break;

		mask = *(int *)val.data;

		/*
		 * See if this keyword record matches any of the
		 * expressions we have stored.
		 */
		if ( ! exprmark(expr, buf, mask, NULL))
			continue;

		memcpy(&rec, val.data + 4, sizeof(recno_t));

		/*
		 * O(log n) scan for prior records.  Since a record
		 * number is unbounded, this has decent performance over
		 * a complex hash function.
		 */

		for (leaf = root; leaf >= 0; )
			if (rec > rs[leaf].res.rec && 
					rs[leaf].rhs >= 0)
				leaf = rs[leaf].rhs;
			else if (rec < rs[leaf].res.rec && 
					rs[leaf].lhs >= 0)
				leaf = rs[leaf].lhs;
			else 
				break;

		/*
		 * If we find a record, see if it has already evaluated
		 * to true.  If it has, great, just keep going.  If not,
		 * try to evaluate it now and continue anyway.
		 */

		if (leaf >= 0 && rs[leaf].res.rec == rec) {
			if (0 == rs[leaf].matched)
				exprexec(expr, buf, mask, &rs[leaf]);
			continue;
		}

		/*
		 * We have a new file to examine.
		 * Extract the manpage's metadata from the index
		 * database, then begin partial evaluation.
		 */

		key.data = &rec;
		key.size = sizeof(recno_t);

		if (0 != (*idx->get)(idx, &key, &val, 0))
			break;

		r.lhs = r.rhs = -1;
		if ( ! index_read(&key, &val, mc, &r))
			break;

		/* XXX: this should be elsewhere, I guess? */

		if (opts->cat && strcasecmp(opts->cat, r.res.cat))
			continue;
		if (opts->arch && strcasecmp(opts->arch, r.res.arch))
			continue;

		rs = mandoc_realloc
			(rs, (rsz + 1) * sizeof(struct rec));

		memcpy(&rs[rsz], &r, sizeof(struct rec));
		rs[rsz].matches = mandoc_calloc(terms, sizeof(int));

		exprexec(expr, buf, mask, &rs[rsz]); 
		/* Append to our tree. */

		if (leaf >= 0) {
			if (rec > rs[leaf].res.rec)
				rs[leaf].rhs = rsz;
			else
				rs[leaf].lhs = rsz;
		} else
			root = rsz;
		
		memset(&r, 0, sizeof(struct rec));
		rsz++;
	}
	
	/*
	 * If we haven't encountered any database errors, then construct
	 * an array of results and push them to the caller.
	 */

	if (1 == ch) {
		for (mlen = i = 0; i < rsz; i++)
			if (rs[i].matched)
				mlen++;
		ress = mandoc_malloc(mlen * sizeof(struct res));
		for (mlen = i = 0; i < rsz; i++)
			if (rs[i].matched)
				memcpy(&ress[mlen++], &rs[i].res, 
						sizeof(struct res));
		(*res)(ress, mlen, arg);
		free(ress);
		rc = 1;
	}

out:
	for (i = 0; i < rsz; i++)
		recfree(&rs[i]);

	recfree(&r);

	if (mc)
		mchars_free(mc);
	if (btree)
		(*btree->close)(btree);
	if (idx)
		(*idx->close)(idx);

	free(buf);
	free(rs);
	return(rc);
}

static void
recfree(struct rec *rec)
{

	free(rec->res.file);
	free(rec->res.cat);
	free(rec->res.title);
	free(rec->res.arch);
	free(rec->res.desc);

	free(rec->matches);
}

struct expr *
exprcomp(int argc, char *argv[], size_t *tt)
{
	int		 pos, lvl;
	struct expr	*e;

	pos = lvl = 0;
	*tt = 0;

	e = exprexpr(argc, argv, &pos, &lvl, tt);

	if (0 == lvl && pos >= argc)
		return(e);

	exprfree(e);
	return(NULL);
}

/*
 * Compile an array of tokens into an expression.
 * An informal expression grammar is defined in apropos(1).
 * Return NULL if we fail doing so.  All memory will be cleaned up.
 * Return the root of the expression sequence if alright.
 */
static struct expr *
exprexpr(int argc, char *argv[], int *pos, int *lvl, size_t *tt)
{
	struct expr	*e, *first, *next;
	int		 log;

	first = next = NULL;

	for ( ; *pos < argc; (*pos)++) {
		e = next;

		/*
		 * Close out a subexpression.
		 */

		if (NULL != e && 0 == strcmp(")", argv[*pos])) {
			if (--(*lvl) < 0)
				goto err;
			break;
		}

		/*
		 * Small note: if we're just starting, don't let "-a"
		 * and "-o" be considered logical operators: they're
		 * just tokens unless pairwise joining, in which case we
		 * record their existence (or assume "OR").
		 */
		log = 0;

		if (NULL != e && 0 == strcmp("-a", argv[*pos]))
			log = 1;			
		else if (NULL != e && 0 == strcmp("-o", argv[*pos]))
			log = 2;

		if (log > 0 && ++(*pos) >= argc)
			goto err;

		/*
		 * Now we parse the term part.  This can begin with
		 * "-i", in which case the expression is case
		 * insensitive.
		 */

		if (0 == strcmp("(", argv[*pos])) {
			++(*pos);
			++(*lvl);
			next = mandoc_calloc(1, sizeof(struct expr));
			next->cs = 1;
			next->subexpr = exprexpr(argc, argv, pos, lvl, tt);
			if (NULL == next->subexpr) {
				free(next);
				next = NULL;
			}
		} else if (0 == strcmp("-i", argv[*pos])) {
			if (++(*pos) >= argc)
				goto err;
			next = exprterm(argv[*pos], 0);
		} else
			next = exprterm(argv[*pos], 1);

		if (NULL == next)
			goto err;

		next->and = log == 1;
		next->index = (int)(*tt)++;

		/* Append to our chain of expressions. */

		if (NULL == first) {
			assert(NULL == e);
			first = next;
		} else {
			assert(NULL != e);
			e->next = next;
		}
	}

	return(first);
err:
	exprfree(first);
	return(NULL);
}

/*
 * Parse a terminal expression with the grammar as defined in
 * apropos(1).
 * Return NULL if we fail the parse.
 */
static struct expr *
exprterm(char *buf, int cs)
{
	struct expr	 e;
	struct expr	*p;
	char		*key;
	int		 i;

	memset(&e, 0, sizeof(struct expr));

	e.cs = cs;

	/* Choose regex or substring match. */

	if (NULL == (e.v = strpbrk(buf, "=~"))) {
		e.regex = 0;
		e.v = buf;
	} else {
		e.regex = '~' == *e.v;
		*e.v++ = '\0';
	}

	/* Determine the record types to search for. */

	e.mask = 0;
	if (buf < e.v) {
		while (NULL != (key = strsep(&buf, ","))) {
			i = 0;
			while (types[i].mask &&
					strcmp(types[i].name, key))
				i++;
			e.mask |= types[i].mask;
		}
	}
	if (0 == e.mask)
		e.mask = TYPE_Nm | TYPE_Nd;

	if (e.regex) {
		i = REG_EXTENDED | REG_NOSUB | cs ? 0 : REG_ICASE;
		if (regcomp(&e.re, e.v, i))
			return(NULL);
	}

	e.v = mandoc_strdup(e.v);

	p = mandoc_calloc(1, sizeof(struct expr));
	memcpy(p, &e, sizeof(struct expr));
	return(p);
}

void
exprfree(struct expr *p)
{
	struct expr	*pp;
	
	while (NULL != p) {
		if (p->subexpr)
			exprfree(p->subexpr);
		if (p->regex)
			regfree(&p->re);
		free(p->v);
		pp = p->next;
		free(p);
		p = pp;
	}
}

static int
exprmark(const struct expr *p, const char *cp, int mask, int *ms)
{

	for ( ; p; p = p->next) {
		if (p->subexpr) {
			if (exprmark(p->subexpr, cp, mask, ms))
				return(1);
			continue;
		} else if ( ! (mask & p->mask))
			continue;

		if (p->regex) {
			if (regexec(&p->re, cp, 0, NULL, 0))
				continue;
		} else if (p->cs) {
			if (NULL == strstr(cp, p->v))
				continue;
		} else {
			if (NULL == strcasestr(cp, p->v))
				continue;
		}

		if (NULL == ms)
			return(1);
		else
			ms[p->index] = 1;
	}

	return(0);
}

static int
expreval(const struct expr *p, int *ms)
{
	int		 match;

	/*
	 * AND has precedence over OR.  Analysis is left-right, though
	 * it doesn't matter because there are no side-effects.
	 * Thus, step through pairwise ANDs and accumulate their Boolean
	 * evaluation.  If we encounter a single true AND collection or
	 * standalone term, the whole expression is true (by definition
	 * of OR).
	 */

	for (match = 0; p && ! match; p = p->next) {
		/* Evaluate a subexpression, if applicable. */
		if (p->subexpr && ! ms[p->index])
			ms[p->index] = expreval(p->subexpr, ms);

		match = ms[p->index];
		for ( ; p->next && p->next->and; p = p->next) {
			/* Evaluate a subexpression, if applicable. */
			if (p->next->subexpr && ! ms[p->next->index])
				ms[p->next->index] = 
					expreval(p->next->subexpr, ms);
			match = match && ms[p->next->index];
		}
	}

	return(match);
}

/*
 * First, update the array of terms for which this expression evaluates
 * to true.
 * Second, logically evaluate all terms over the updated array of truth
 * values.
 * If this evaluates to true, mark the expression as satisfied.
 */
static void
exprexec(const struct expr *p, const char *cp, int mask, struct rec *r)
{

	assert(0 == r->matched);
	exprmark(p, cp, mask, r->matches);
	r->matched = expreval(p, r->matches);
}