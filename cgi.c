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
static	void		 catman(const char *);
static	void		 format(const char *);
static	void		 html_print(const char *);
static	void		 html_putchar(char);
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
static	void		 resp_error400(void);
static	void		 resp_error404(const char *);
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

static void
html_putchar(char c)
{

	switch (c) {
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

/*
 * Print a word, escaping HTML along the way.
 * This will pass non-ASCII straight to output: be warned!
 */
static void
html_print(const char *p)
{
	
	if (NULL == p)
		return;
	while ('\0' != *p)
		html_putchar(*p++);
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
	     "   <META HTTP-EQUIV=\"Content-Type\" "		"\n"
	     "         CONTENT=\"text/html; charset=utf-8\">"	"\n"
	     "  <LINK REL=\"stylesheet\" HREF=\"/man.cgi.css\""	"\n"
	     "        TYPE=\"text/css\" media=\"all\">"		"\n"
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
		else if (0 == strcmp(req->fields[i].key, "query"))
			expr = req->fields[i].val;
		else if (0 == strcmp(req->fields[i].key, "sec"))
			sec = req->fields[i].val;
		else if (0 == strcmp(req->fields[i].key, "sektion"))
			sec = req->fields[i].val;
		else if (0 == strcmp(req->fields[i].key, "arch"))
			arch = req->fields[i].val;

	if (NULL != sec && 0 == strcmp(sec, "0"))
		sec = NULL;

	puts("<!-- Begin search form. //-->");
	printf("<FORM ACTION=\"");
	html_print(progname);
	printf("/search.html\" METHOD=\"get\">\n");
	puts("<FIELDSET>\n"
	     "<INPUT TYPE=\"submit\" NAME=\"op\" "
	      "VALUE=\"Whatis\"> or \n"
	     "<INPUT TYPE=\"submit\" NAME=\"op\" "
	      "VALUE=\"apropos\"> for manuals satisfying \n"
	     "<INPUT TYPE=\"text\" SIZE=\"40\" "
	      "NAME=\"expr\" VALUE=\"");
	html_print(expr);
	puts("\">, section "
	     "<INPUT TYPE=\"text\" "
	      "SIZE=\"4\" NAME=\"sec\" VALUE=\"");
	html_print(sec);
	puts("\">, arch "
	     "<INPUT TYPE=\"text\" "
	      "SIZE=\"8\" NAME=\"arch\" VALUE=\"");
	html_print(arch);
	puts("\">.\n"
	     "<INPUT TYPE=\"reset\" VALUE=\"Reset\">\n"
	     "</FIELDSET>\n"
	     "</FORM>\n"
	     "<!-- End search form. //-->");
}

static void
resp_index(const struct req *req)
{

	resp_begin_html(200, NULL);
	resp_searchform(req);
	resp_end_html();
}

static void
resp_error400(void)
{

	resp_begin_html(400, "Query Malformed");
	printf("<H1>Malformed Query</H1>\n"
	       "<P>\n"
	       "  The query your entered was malformed.\n"
	       "  Try again from the\n"
	       "  <A HREF=\"%s/index.html\">main page</A>\n"
	       "</P>", progname);
	resp_end_html();
}

static void
resp_error404(const char *page)
{

	resp_begin_html(404, "Not Found");
	puts("<H1>Page Not Found</H1>\n"
	     "<P>\n"
	     "  The page you're looking for, ");
	printf("  <B>");
	html_print(page);
	printf("</B>,\n"
	       "  could not be found.\n"
	       "  Try searching from the\n"
	       "  <A HREF=\"%s/index.html\">main page</A>\n"
	       "</P>", progname);
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
catman(const char *file)
{
	FILE		*f;
	size_t		 len;
	int		 i;
	char		*p;
	int		 italic, bold;

	if (NULL == (f = fopen(file, "r"))) {
		resp_baddb();
		return;
	}

	resp_begin_http(200, NULL);
	puts("<!DOCTYPE HTML PUBLIC "				"\n"
	     " \"-//W3C//DTD HTML 4.01//EN\""			"\n"
	     " \"http://www.w3.org/TR/html4/strict.dtd\">"	"\n"
	     "<HTML>"						"\n"
	     " <HEAD>"						"\n"
	     "  <META HTTP-EQUIV=\"Content-Type\" "		"\n"
	     "        CONTENT=\"text/html; charset=utf-8\">"	"\n"
	     "  <LINK REL=\"stylesheet\" HREF=\"/catman.css\""	"\n"
	     "        TYPE=\"text/css\" media=\"all\">"		"\n"
	     "  <TITLE>System Manpage Reference</TITLE>"	"\n"
	     " </HEAD>"						"\n"
	     " <BODY>"						"\n"
	     "<!-- Begin page content. //-->");

	puts("<PRE>");
	while (NULL != (p = fgetln(f, &len))) {
		bold = italic = 0;
		for (i = 0; i < (int)len - 1; i++) {
			/* 
			 * This means that the catpage is out of state.
			 * Ignore it and keep going (although the
			 * catpage is bogus).
			 */

			if ('\b' == p[i] || '\n' == p[i])
				continue;

			/*
			 * Print a regular character.
			 * Close out any bold/italic scopes.
			 * If we're in back-space mode, make sure we'll
			 * have something to enter when we backspace.
			 */

			if ('\b' != p[i + 1]) {
				if (italic)
					printf("</I>");
				if (bold)
					printf("</B>");
				italic = bold = 0;
				html_putchar(p[i]);
				continue;
			} else if (i + 2 >= (int)len)
				continue;

			/* Italic mode. */

			if ('_' == p[i]) {
				if (bold)
					printf("</B>");
				if ( ! italic)
					printf("<I>");
				bold = 0;
				italic = 1;
				i += 2;
				html_putchar(p[i]);
				continue;
			}

			/* 
			 * Handle funny behaviour troff-isms.
			 * These grok'd from the original man2html.c.
			 */

			if (('+' == p[i] && 'o' == p[i + 2]) ||
					('o' == p[i] && '+' == p[i + 2]) ||
					('|' == p[i] && '=' == p[i + 2]) ||
					('=' == p[i] && '|' == p[i + 2]) ||
					('*' == p[i] && '=' == p[i + 2]) ||
					('=' == p[i] && '*' == p[i + 2]) ||
					('*' == p[i] && '|' == p[i + 2]) ||
					('|' == p[i] && '*' == p[i + 2]))  {
				if (italic)
					printf("</I>");
				if (bold)
					printf("</B>");
				italic = bold = 0;
				putchar('*');
				i += 2;
				continue;
			} else if (('|' == p[i] && '-' == p[i + 2]) ||
					('-' == p[i] && '|' == p[i + 1]) ||
					('+' == p[i] && '-' == p[i + 1]) ||
					('-' == p[i] && '+' == p[i + 1]) ||
					('+' == p[i] && '|' == p[i + 1]) ||
					('|' == p[i] && '+' == p[i + 1]))  {
				if (italic)
					printf("</I>");
				if (bold)
					printf("</B>");
				italic = bold = 0;
				putchar('+');
				i += 2;
				continue;
			}

			/* Bold mode. */
			
			if (italic)
				printf("</I>");
			if ( ! bold)
				printf("<B>");
			bold = 1;
			italic = 0;
			i += 2;
			html_putchar(p[i]);
		}

		/* 
		 * Clean up the last character.
		 * We can get to a newline; don't print that. 
		 */

		if (italic)
			printf("</I>");
		if (bold)
			printf("</B>");

		if (i == (int)len - 1 && '\n' != p[i])
			html_putchar(p[i]);

		putchar('\n');
	}

	puts("</PRE>\n"
	     "</BODY>\n"
	     "</HTML>");

	fclose(f);
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
	char		 opts[MAXPATHLEN + 128];

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

	snprintf(opts, sizeof(opts), "style=/man.css,"
			"man=%s/search.html?sec=%%S&expr=%%N,"
			/*"includes=/cgi-bin/man.cgi/usr/include/%%I"*/,
			progname);

	mparse_result(mp, &mdoc, &man);
	vp = html_alloc(opts);

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
	const char	*fn, *cp;
	int		 rc;
	unsigned int	 vol, rec;
	DB		*idx;
	DBT		 key, val;

	if (NULL == path) {
		resp_error400();
		return;
	} else if (NULL == (sub = strrchr(path, '/'))) {
		resp_error400();
		return;
	} else
		*sub++ = '\0';

	if ( ! (atou(path, &vol) && atou(sub, &rec))) {
		resp_error400();
		return;
	} else if (vol >= (unsigned int)ps->sz) {
		resp_error400();
		return;
	}

	strlcpy(file, ps->paths[vol], MAXPATHLEN);
	strlcat(file, "/mandoc.index", MAXPATHLEN);

	/* Open the index recno(3) database. */

	idx = dbopen(file, O_RDONLY, 0, DB_RECNO, NULL);
	if (NULL == idx) {
		resp_baddb();
		return;
	}

	key.data = &rec;
	key.size = 4;

	if (0 != (rc = (*idx->get)(idx, &key, &val, 0))) {
		rc < 0 ? resp_baddb() : resp_error400();
		goto out;
	} 

	cp = (char *)val.data;

	if (NULL == (fn = memchr(cp, '\0', val.size)))
		resp_baddb();
	else if (++fn - cp >= (int)val.size)
		resp_baddb();
	else if (NULL == memchr(fn, '\0', val.size - (fn - cp)))
		resp_baddb();
	else {
		strlcpy(file, ps->paths[vol], MAXPATHLEN);
		strlcat(file, "/", MAXPATHLEN);
		strlcat(file, fn, MAXPATHLEN);
		if (0 == strcmp(cp, "cat"))
			catman(file);
		else
			format(file);
	}
out:
	(*idx->close)(idx);
}

static void
pg_search(const struct manpaths *ps, const struct req *req, char *path)
{
	size_t		  tt;
	int		  i, sz, rc, whatis;
	const char	 *ep, *start;
	char		**cp;
	struct opts	  opt;
	struct expr	 *expr;

	expr = NULL;
	cp = NULL;
	ep = NULL;
	sz = 0;
	whatis = 0;

	memset(&opt, 0, sizeof(struct opts));

	for (sz = i = 0; i < (int)req->fieldsz; i++)
		if (0 == strcmp(req->fields[i].key, "expr"))
			ep = req->fields[i].val;
		else if (0 == strcmp(req->fields[i].key, "query"))
			ep = req->fields[i].val;
		else if (0 == strcmp(req->fields[i].key, "sec"))
			opt.cat = req->fields[i].val;
		else if (0 == strcmp(req->fields[i].key, "sektion"))
			opt.cat = req->fields[i].val;
		else if (0 == strcmp(req->fields[i].key, "arch"))
			opt.arch = req->fields[i].val;
		else if (0 == strcmp(req->fields[i].key, "apropos"))
			whatis = 0 == strcmp
				(req->fields[i].val, "0");
		else if (0 == strcmp(req->fields[i].key, "op"))
			whatis = 0 == strcasecmp
				(req->fields[i].val, "whatis");

	if (NULL != opt.cat && 0 == strcmp(opt.cat, "0"))
		opt.cat = NULL;

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

	expr = whatis ? termcomp(sz, cp, &tt) :
		        exprcomp(sz, cp, &tt);

	if (NULL != expr)
		rc = apropos_search
			(ps->sz, ps->paths, &opt,
			 expr, tt, (void *)req, resp_search);

	/* ...unless errors occured. */

	if (0 == rc)
		resp_baddb();
	else if (-1 == rc)
		resp_search(NULL, 0, (void *)req);

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
	manpath_manconf("etc/catman.conf", &paths);

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
		resp_error404(path);
		break;
	}

	manpath_free(&paths);
	kval_free(req.fields, req.fieldsz);

	return(EXIT_SUCCESS);
}
