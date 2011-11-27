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

#include <sys/param.h>
#include <sys/wait.h>

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <regex.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "apropos_db.h"
#include "mandoc.h"
#include "mdoc.h"
#include "man.h"
#include "main.h"
#include "manpath.h"

#ifdef __linux__
# include <db_185.h>
#else
# include <db.h>
#endif

enum	page {
	PAGE_INDEX,
	PAGE_SEARCH,
	PAGE_SHOW,
	PAGE__MAX
};

struct	kval {
	char		*key;
	char		*val;
};

struct	req {
	struct kval	*fields;
	size_t		 fieldsz;
	enum page	 page;
};

static	int		 atou(const char *, unsigned *);
static	void		 format(const char *);
static	void		 html_print(const char *);
static	int 		 kval_decode(char *);
static	void		 kval_parse(struct kval **, size_t *, char *);
static	void		 kval_free(struct kval *, size_t);
static	void		 pg_index(const struct manpaths *,
				const struct req *, char *);
static	void		 pg_search(const struct manpaths *,
				const struct req *, char *);
static	void		 pg_show(const struct manpaths *,
				const struct req *, char *);
static	void		 resp_bad(void);
static	void		 resp_baddb(void);
static	void		 resp_badexpr(const struct req *);
static	void		 resp_badmanual(void);
static	void		 resp_begin_html(int, const char *);
static	void		 resp_begin_http(int, const char *);
static	void		 resp_end_html(void);
static	void		 resp_index(const struct req *);
static	void		 resp_search(struct res *, size_t, void *);
static	void		 resp_searchform(const struct req *);

static	const char	 *progname;
static	const char	 *cache;
static	const char	 *host;

static	const char * const pages[PAGE__MAX] = {
	"index", /* PAGE_INDEX */ 
	"search", /* PAGE_SEARCH */
	"show", /* PAGE_SHOW */
};

/*
 * This is just OpenBSD's strtol(3) suggestion.
 * I use it instead of strtonum(3) for portability's sake.
 */
static int
atou(const char *buf, unsigned *v)
{
	char		*ep;
	long		 lval;

	errno = 0;
	lval = strtol(buf, &ep, 10);
	if (buf[0] == '\0' || *ep != '\0')
		return(0);
	if ((errno == ERANGE && (lval == LONG_MAX || 
					lval == LONG_MIN)) ||
			(lval > UINT_MAX || lval < 0))
		return(0);

	*v = (unsigned int)lval;
	return(1);
}

/*
 * Print a word, escaping HTML along the way.
 * This will pass non-ASCII straight to output: be warned!
 */
static void
html_print(const char *p)
{
	char		 c;
	
	if (NULL == p)
		return;

	while ('\0' != *p)
		switch ((c = *p++)) {
		case ('"'):
			printf("&quote;");
			break;
		case ('&'):
			printf("&amp;");
			break;
		case ('>'):
			printf("&gt;");
			break;
		case ('<'):
			printf("&lt;");
			break;
		default:
			putchar((unsigned char)c);
			break;
		}
}

static void
kval_free(struct kval *p, size_t sz)
{
	int		 i;

	for (i = 0; i < (int)sz; i++) {
		free(p[i].key);
		free(p[i].val);
	}
	free(p);
}

/*
 * Parse out key-value pairs from an HTTP request variable.
 * This can be either a cookie or a POST/GET string, although man.cgi
 * uses only GET for simplicity.
 */
static void
kval_parse(struct kval **kv, size_t *kvsz, char *p)
{
	char            *key, *val;
	size_t           sz, cur;

	cur = 0;

	while (p && '\0' != *p) {
		while (' ' == *p)
			p++;

		key = p;
		val = NULL;

		if (NULL != (p = strchr(p, '='))) {
			*p++ = '\0';
			val = p;

			sz = strcspn(p, ";&");
			/* LINTED */
			p += sz;

			if ('\0' != *p)
				*p++ = '\0';
		} else {
			p = key;
			sz = strcspn(p, ";&");
			/* LINTED */
			p += sz;

			if ('\0' != *p)
				p++;
			continue;
		}

		if ('\0' == *key || '\0' == *val)
			continue;

		/* Just abort handling. */

		if ( ! kval_decode(key))
			return;
		if ( ! kval_decode(val))
			return;

		if (*kvsz + 1 >= cur) {
			cur++;
			*kv = mandoc_realloc
				(*kv, cur * sizeof(struct kval));
		}

		(*kv)[(int)*kvsz].key = mandoc_strdup(key);
		(*kv)[(int)*kvsz].val = mandoc_strdup(val);
		(*kvsz)++;
	}
}

/*
 * HTTP-decode a string.  The standard explanation is that this turns
 * "%4e+foo" into "n foo" in the regular way.  This is done in-place
 * over the allocated string.
 */
static int
kval_decode(char *p)
{
	char             hex[3];
	int              c;

	hex[2] = '\0';

	for ( ; '\0' != *p; p++) {
		if ('%' == *p) {
			if ('\0' == (hex[0] = *(p + 1)))
				return(0);
			if ('\0' == (hex[1] = *(p + 2)))
				return(0);
			if (1 != sscanf(hex, "%x", &c))
				return(0);
			if ('\0' == c)
				return(0);

			*p = (char)c;
			memmove(p + 1, p + 3, strlen(p + 3) + 1);
		} else
			*p = '+' == *p ? ' ' : *p;
	}

	*p = '\0';
	return(1);
}

static void
resp_begin_http(int code, const char *msg)
{

	if (200 != code)
		printf("Status: %d %s\n", code, msg);

	puts("Content-Type: text/html; charset=utf-8"		"\n"
	     "Cache-Control: no-cache"				"\n"
	     "Pragma: no-cache"					"\n"
	     "");

	fflush(stdout);
}

static void
resp_begin_html(int code, const char *msg)
{

	resp_begin_http(code, msg);

	puts("<!DOCTYPE HTML PUBLIC "				"\n"
	     " \"-//W3C//DTD HTML 4.01//EN\""			"\n"
	     " \"http://www.w3.org/TR/html4/strict.dtd\">"	"\n"
	     "<HTML>"						"\n"
	     " <HEAD>"						"\n"
	     "  <TITLE>System Manpage Reference</TITLE>"	"\n"
	     " </HEAD>"						"\n"
	     " <BODY>"						"\n"
	     "<!-- Begin page content. //-->");
}

static void
resp_end_html(void)
{

	puts(" </BODY>\n</HTML>");
}

static void
resp_searchform(const struct req *req)
{
	int	 	 i;
	const char	*expr, *sec, *arch;

	expr = sec = arch = "";

	for (i = 0; i < (int)req->fieldsz; i++)
		if (0 == strcmp(req->fields[i].key, "expr"))
			expr = req->fields[i].val;
		else if (0 == strcmp(req->fields[i].key, "sec"))
			sec = req->fields[i].val;
		else if (0 == strcmp(req->fields[i].key, "arch"))
			arch = req->fields[i].val;

	puts("<!-- Begin search form. //-->");
	printf("<FORM ACTION=\"");
	html_print(progname);
	printf("/search\" METHOD=\"get\">\n");
	puts(" <FIELDSET>"					"\n"
	     "  <INPUT TYPE=\"submit\" VALUE=\"Search:\">");
	printf("  Terms: <INPUT TYPE=\"text\" "
			"SIZE=\"60\" NAME=\"expr\" VALUE=\"");
	html_print(expr);
	puts("\">");
	printf("  Section: <INPUT TYPE=\"text\" "
			"SIZE=\"4\" NAME=\"sec\" VALUE=\"");
	html_print(sec);
	puts("\">");
	printf("  Arch: <INPUT TYPE=\"text\" "
			"SIZE=\"8\" NAME=\"arch\" VALUE=\"");
	html_print(arch);
	puts("\">");
	puts(" </FIELDSET>\n</FORM>\n<!-- End search form. //-->");
}

static void
resp_index(const struct req *req)
{

	resp_begin_html(200, NULL);
	resp_searchform(req);
	resp_end_html();
}

static void
resp_badmanual(void)
{

	resp_begin_html(404, "Not Found");
	puts("<P>Requested manual not found.</P>");
	resp_end_html();
}

static void
resp_badexpr(const struct req *req)
{

	resp_begin_html(200, NULL);
	resp_searchform(req);
	puts("<P>Your search didn't work.</P>");
	resp_end_html();
}

static void
resp_bad(void)
{
	resp_begin_html(500, "Internal Server Error");
	puts("<P>Generic badness happened.</P>");
	resp_end_html();
}

static void
resp_baddb(void)
{

	resp_begin_html(500, "Internal Server Error");
	puts("<P>Your database is broken.</P>");
	resp_end_html();
}

static void
resp_search(struct res *r, size_t sz, void *arg)
{
	int		 i;

	if (1 == sz) {
		/*
		 * If we have just one result, then jump there now
		 * without any delay.
		 */
		puts("Status: 303 See Other");
		printf("Location: http://%s%s/show/%u/%u.html\n",
				host, progname,
				r[0].volume, r[0].rec);
		puts("Content-Type: text/html; charset=utf-8\n");
		return;
	}

	resp_begin_html(200, NULL);
	resp_searchform((const struct req *)arg);

	if (0 == sz)
		puts("<P>No results found.</P>");

	for (i = 0; i < (int)sz; i++) {
		printf("<P><A HREF=\"");
		html_print(progname);
		printf("/show/%u/%u.html\">", r[i].volume, r[i].rec);
		html_print(r[i].title);
		putchar('(');
		html_print(r[i].cat);
		if (r[i].arch && '\0' != *r[i].arch) {
			putchar('/');
			html_print(r[i].arch);
		}
		printf(")</A> ");
		html_print(r[i].desc);
		puts("</P>");
	}

	resp_end_html();
}

/* ARGSUSED */
static void
pg_index(const struct manpaths *ps, const struct req *req, char *path)
{

	resp_index(req);
}

static void
format(const char *file)
{
	struct mparse	*mp;
	int		 fd;
	struct mdoc	*mdoc;
	struct man	*man;
	void		*vp;
	enum mandoclevel rc;

	if (-1 == (fd = open(file, O_RDONLY, 0))) {
		resp_baddb();
		return;
	}

	mp = mparse_alloc(MPARSE_AUTO, MANDOCLEVEL_FATAL, NULL, NULL);
	rc = mparse_readfd(mp, fd, file);
	close(fd);

	if (rc >= MANDOCLEVEL_FATAL) {
		resp_baddb();
		return;
	}

	mparse_result(mp, &mdoc, &man);
	vp = html_alloc(NULL);

	if (NULL != mdoc) {
		resp_begin_http(200, NULL);
		html_mdoc(vp, mdoc);
	} else if (NULL != man) {
		resp_begin_http(200, NULL);
		html_man(vp, man);
	} else
		resp_baddb();

	html_free(vp);
	mparse_free(mp);
}

static void
pg_show(const struct manpaths *ps, const struct req *req, char *path)
{
	char		*sub;
	char		 file[MAXPATHLEN];
	int		 rc;
	unsigned int	 vol, rec;
	DB		*db;
	DBT		 key, val;

	if (NULL == path) {
		resp_badmanual();
		return;
	} else if (NULL == (sub = strrchr(path, '/'))) {
		resp_badmanual();
		return;
	} else
		*sub++ = '\0';

	if ( ! (atou(path, &vol) && atou(sub, &rec))) {
		resp_badmanual();
		return;
	} else if (vol >= (unsigned int)ps->sz) {
		resp_badmanual();
		return;
	}

	strlcpy(file, ps->paths[vol], MAXPATHLEN);
	strlcat(file, "/mandoc.index", MAXPATHLEN);

	/* Open the index recno(3) database. */

	db = dbopen(file, O_RDONLY, 0, DB_RECNO, NULL);
	if (NULL == db) {
		resp_baddb();
		return;
	}

	key.data = &rec;
	key.size = 4;

	if (0 != (rc = (*db->get)(db, &key, &val, 0))) {
		rc < 0 ? resp_baddb() : resp_badmanual();
		(*db->close)(db);
		return;
	} 

	/* Extra filename: the first nil-terminated entry. */

	strlcpy(file, ps->paths[vol], MAXPATHLEN);
	strlcat(file, "/", MAXPATHLEN);
	strlcat(file, (char *)val.data, MAXPATHLEN);

	(*db->close)(db);

	format(file);
}

static void
pg_search(const struct manpaths *ps, const struct req *req, char *path)
{
	size_t		  tt;
	int		  i, sz, rc;
	const char	 *ep, *start;
	char		**cp;
	struct opts	  opt;
	struct expr	 *expr;

	expr = NULL;
	cp = NULL;
	ep = NULL;
	sz = 0;

	memset(&opt, 0, sizeof(struct opts));

	for (sz = i = 0; i < (int)req->fieldsz; i++)
		if (0 == strcmp(req->fields[i].key, "expr"))
			ep = req->fields[i].val;
		else if (0 == strcmp(req->fields[i].key, "sec"))
			opt.cat = req->fields[i].val;
		else if (0 == strcmp(req->fields[i].key, "arch"))
			opt.arch = req->fields[i].val;

	/*
	 * Poor man's tokenisation.
	 * Just break apart by spaces.
	 * Yes, this is half-ass.  But it works for now.
	 */

	while (ep && isspace((unsigned char)*ep))
		ep++;

	while (ep && '\0' != *ep) {
		cp = mandoc_realloc(cp, (sz + 1) * sizeof(char *));
		start = ep;
		while ('\0' != *ep && ! isspace((unsigned char)*ep))
			ep++;
		cp[sz] = mandoc_malloc((ep - start) + 1);
		memcpy(cp[sz], start, ep - start);
		cp[sz++][ep - start] = '\0';
		while (isspace((unsigned char)*ep))
			ep++;
	}

	rc = -1;

	/*
	 * Pump down into apropos backend.
	 * The resp_search() function is called with the results.
	 */

	if (NULL != (expr = exprcomp(sz, cp, &tt)))
		rc = apropos_search
			(ps->sz, ps->paths, &opt,
			 expr, tt, (void *)req, resp_search);

	/* ...unless errors occured. */

	if (0 == rc)
		resp_baddb();
	else if (-1 == rc)
		resp_badexpr(req);

	for (i = 0; i < sz; i++)
		free(cp[i]);

	free(cp);
	exprfree(expr);
}

int
main(void)
{
	int		 i;
	struct req	 req;
	char		*p, *path, *subpath;
	struct manpaths	 paths;

	/* HTTP init: read and parse the query string. */

	progname = getenv("SCRIPT_NAME");
	if (NULL == progname)
		progname = "";

	cache = getenv("CACHE_DIR");
	if (NULL == cache)
		cache = "/cache/man.cgi";

	if (-1 == chdir(cache)) {
		resp_bad();
		return(EXIT_FAILURE);
	}

	host = getenv("HTTP_HOST");
	if (NULL == host)
		host = "localhost";

	memset(&req, 0, sizeof(struct req));

	if (NULL != (p = getenv("QUERY_STRING")))
		kval_parse(&req.fields, &req.fieldsz, p);

	/* Resolve leading subpath component. */

	subpath = path = NULL;
	req.page = PAGE__MAX;

	if (NULL == (path = getenv("PATH_INFO")) || '\0' == *path)
		req.page = PAGE_INDEX;

	if (NULL != path && '/' == *path && '\0' == *++path)
		req.page = PAGE_INDEX;

	/* Strip file suffix. */

	if (NULL != path && NULL != (p = strrchr(path, '.')))
		if (NULL != p && NULL == strchr(p, '/'))
			*p++ = '\0';

	/* Resolve subpath component. */

	if (NULL != path && NULL != (subpath = strchr(path, '/')))
		*subpath++ = '\0';

	/* Map path into one we recognise. */

	if (NULL != path && '\0' != *path)
		for (i = 0; i < (int)PAGE__MAX; i++) 
			if (0 == strcmp(pages[i], path)) {
				req.page = (enum page)i;
				break;
			}

	/* Initialise MANPATH. */

	memset(&paths, 0, sizeof(struct manpaths));
	manpath_manconf("etc/man.conf", &paths);

	/* Route pages. */

	switch (req.page) {
	case (PAGE_INDEX):
		pg_index(&paths, &req, subpath);
		break;
	case (PAGE_SEARCH):
		pg_search(&paths, &req, subpath);
		break;
	case (PAGE_SHOW):
		pg_show(&paths, &req, subpath);
		break;
	default:
		break;
	}

	manpath_free(&paths);
	kval_free(req.fields, req.fieldsz);

	return(EXIT_SUCCESS);
}
