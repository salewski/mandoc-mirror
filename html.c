/* $Id$ */
/*
 * Copyright (c) 2008 Kristaps Dzonsons <kristaps@kth.se>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
#include <sys/param.h>
#include <sys/stat.h>

#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "libmdocml.h"
#include "private.h"
#include "ml.h"


/* TODO: allow head/tail-less invocations (just "div" start). */

struct	htmlnode {
	int		 tok;
	enum md_ns	 ns;
	int		 argc[ROFF_MAXLINEARG];
	char		*argv[ROFF_MAXLINEARG];
	struct htmlnode	*parent;
};


struct	htmlq {
	struct htmlnode	*last;
};


static	int		html_loadcss(struct md_mbuf *, const char *);

static	int		html_alloc(void **);
static	void		html_free(void *);
static	ssize_t		html_endtag(struct md_mbuf *, void *,
				const struct md_args *, 
				enum md_ns, int);
static	ssize_t		html_beginstring(struct md_mbuf *, 
				const struct md_args *, 
				const char *, size_t);
static	ssize_t		html_beginhttp(struct md_mbuf *, 
				const struct md_args *, 
				const char *, size_t);
static	ssize_t		html_endstring(struct md_mbuf *, 
				const struct md_args *, 
				const char *, size_t);
static	ssize_t		html_endhttp(struct md_mbuf *, 
				const struct md_args *, 
				const char *, size_t);
static	ssize_t		html_begintag(struct md_mbuf *, void *,
				const struct md_args *, 
				enum md_ns, int, 
				const int *, const char **);
static	int		html_begin(struct md_mbuf *,
	       			const struct md_args *, 
				const struct tm *, 
				const char *, const char *, 
				enum roffmsec, const char *);
static	int		html_printargs(struct md_mbuf *, int, 
				const char *, const int *, 
				const char **, size_t *);
static	int		html_end(struct md_mbuf *, 
				const struct md_args *);
static	int		html_blocktagname(struct md_mbuf *,
				const struct md_args *, int, 
				struct htmlq *, const int *, 
				const char **, size_t *);
static	int		html_blocktagargs(struct md_mbuf *,
				const struct md_args *, int,
				const int *, const char **, size_t *);
static	int		html_headtagname(struct md_mbuf *,
				const struct md_args *, int, 
				struct htmlq *, const int *, 
				const char **, size_t *);
static	int		html_headtagargs(struct md_mbuf *,
				const struct md_args *, int,
				const int *, const char **, size_t *);
static	int		html_bodytagname(struct md_mbuf *,
				const struct md_args *, 
				int, struct htmlq *, const int *, 
				const char **, size_t *);
static	int		html_bodytagargs(struct md_mbuf *,
				const struct md_args *, int,
				const int *, const char **, size_t *);
static	int		html_inlinetagname(struct md_mbuf *,
				const struct md_args *, int, size_t *);
static	int		html_inlinetagargs(struct md_mbuf *,
				const struct md_args *, int,
				const int *, const char **, size_t *);
static	int		html_Bl_bodytagname(struct md_mbuf *, 
				struct htmlq *, const int *, 
				const char **, size_t *);
static	int		html_It_blocktagname(struct md_mbuf *, 
				struct htmlq *, const int *, 
				const char **, size_t *);
static	int		html_It_headtagname(struct md_mbuf *, 
				struct htmlq *, const int *, 
				const char **, size_t *);
static	int		html_It_bodytagname(struct md_mbuf *, 
				struct htmlq *, const int *, 
				const char **, size_t *);


/* ARGSUSED */
static int
html_It_headtagname(struct md_mbuf *mbuf, struct htmlq *q, 
		const int *argc, const char **argv, size_t *res)
{
	struct htmlnode	*n;
	int		 i;

	for (n = q->last; n; n = n->parent)
		if (n->tok == ROFF_Bl)
			break;

	assert(n);

	/* LINTED */
	for (i = 0; ROFF_ARGMAX != n->argc[i] &&
			i < ROFF_MAXLINEARG; i++) {
		switch (n->argc[i]) {
		case (ROFF_Ohang):
			return(ml_nputs(mbuf, "div", 3, res));
		case (ROFF_Tag):
			/* FALLTHROUGH */
		case (ROFF_Column): 
			return(ml_nputs(mbuf, "td", 2, res));
		default:
			break;
		}
	}

	return(0);
}


/* ARGSUSED */
static int
html_It_bodytagname(struct md_mbuf *mbuf, struct htmlq *q, 
		const int *argc, const char **argv, size_t *res)
{
	struct htmlnode	*n;
	int		 i;

	for (n = q->last; n; n = n->parent)
		if (n->tok == ROFF_Bl)
			break;

	assert(n);

	/* LINTED */
	for (i = 0; ROFF_ARGMAX != n->argc[i] &&
			i < ROFF_MAXLINEARG; i++) {
		switch (n->argc[i]) {
		case (ROFF_Enum):
			/* FALLTHROUGH */
		case (ROFF_Bullet):
			/* FALLTHROUGH */
		case (ROFF_Dash):
			/* FALLTHROUGH */
		case (ROFF_Hyphen):
			/* FALLTHROUGH */
		case (ROFF_Item):
			/* FALLTHROUGH */
		case (ROFF_Diag):
			/* FALLTHROUGH */
		case (ROFF_Hang):
			/* FALLTHROUGH */
		case (ROFF_Ohang): 
			/* FALLTHROUGH */
		case (ROFF_Inset):
			return(ml_nputs(mbuf, "div", 3, res));
		case (ROFF_Tag):
			/* FALLTHROUGH */
		case (ROFF_Column): 
			return(ml_nputs(mbuf, "td", 2, res));
		default:
			break;
		}
	}

	assert(i != ROFF_MAXLINEARG);
	return(0);
}


/* ARGSUSED */
static int
html_Bl_bodytagname(struct md_mbuf *mbuf, struct htmlq *q, 
		const int *argc, const char **argv, size_t *res)
{
	int		 i;

	for (i = 0; ROFF_ARGMAX != argc[i]
			&& i < ROFF_MAXLINEARG; i++) {
		switch (argc[i]) {
		case (ROFF_Enum):
			return(ml_nputs(mbuf, "ol", 2, res));
		case (ROFF_Bullet):
			/* FALLTHROUGH */
		case (ROFF_Dash):
			/* FALLTHROUGH */
		case (ROFF_Hyphen):
			/* FALLTHROUGH */
		case (ROFF_Item):
			/* FALLTHROUGH */
		case (ROFF_Diag):
			/* FALLTHROUGH */
		case (ROFF_Hang):
			/* FALLTHROUGH */
		case (ROFF_Ohang): 
			/* FALLTHROUGH */
		case (ROFF_Inset):
			return(ml_nputs(mbuf, "ul", 2, res));
		case (ROFF_Tag):
			/* FALLTHROUGH */
		case (ROFF_Column): 
			return(ml_nputs(mbuf, "table", 5, res));
		default:
			break;
		}
	}

	assert(i != ROFF_MAXLINEARG);
	return(0);
}


/* ARGSUSED */
static int
html_It_blocktagname(struct md_mbuf *mbuf, struct htmlq *q, 
		const int *argc, const char **argv, size_t *res)
{
	struct htmlnode	*n;
	int		 i;

	for (n = q->last; n; n = n->parent)
		if (n->tok == ROFF_Bl)
			break;

	assert(n);

	/* LINTED */
	for (i = 0; ROFF_ARGMAX != n->argc[i] &&
			i < ROFF_MAXLINEARG; i++) {
		switch (n->argc[i]) {
		case (ROFF_Enum):
			/* FALLTHROUGH */
		case (ROFF_Bullet):
			/* FALLTHROUGH */
		case (ROFF_Dash):
			/* FALLTHROUGH */
		case (ROFF_Hyphen):
			/* FALLTHROUGH */
		case (ROFF_Item):
			/* FALLTHROUGH */
		case (ROFF_Diag):
			/* FALLTHROUGH */
		case (ROFF_Hang):
			/* FALLTHROUGH */
		case (ROFF_Ohang): 
			/* FALLTHROUGH */
		case (ROFF_Inset):
			return(ml_nputs(mbuf, "li", 2, res));
		case (ROFF_Tag):
			/* FALLTHROUGH */
		case (ROFF_Column): 
			return(ml_nputs(mbuf, "tr", 2, res));
		default:
			break;
		}
	}

	assert(i != ROFF_MAXLINEARG);
	return(0);
}


static int
html_loadcss(struct md_mbuf *mbuf, const char *css)
{
	size_t		 res, bufsz;
	char		*buf;
	struct stat	 st;
	int		 fd, c;
	ssize_t		 ssz;

	c = 0;
	res = 0;
	buf = NULL;

	if (-1 == (fd = open(css, O_RDONLY, 0))) {
		warn("%s", css);
		return(0);
	} 
	
	if (-1 == fstat(fd, &st)) {
		warn("%s", css);
		goto out;
	}

	bufsz = MAX(st.st_blksize, BUFSIZ);
	if (NULL == (buf = malloc(bufsz))) {
		warn("malloc");
		goto out;
	}

	for (;;) {
		if (-1 == (ssz = read(fd, buf, bufsz))) {
			warn("%s", css);
			goto out;
		} else if (0 == ssz)
			break;
		if ( ! ml_nputs(mbuf, buf, (size_t)ssz, &res))
			goto out;
	}

	c = 1;

out:
	if (-1 == close(fd)) {
		warn("%s", css);
		c = 0;
	}

	if (buf)
		free(buf);

	return(c);
}


/* ARGSUSED */
static int 
html_begin(struct md_mbuf *mbuf, const struct md_args *args,
		const struct tm *tm, const char *os, 
		const char *title, enum roffmsec section, 
		const char *vol)
{
	const char	*preamble, *css, *trail;
	char		 buf[512];
	size_t		 res;

	preamble =
	"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\"\n"
	"    \"http://www.w3.org/TR/html4/strict.dtd\">\n"
	"<html>\n"
	"<head>\n"
	"    <meta http-equiv=\"Content-Type\"\n"
	"         content=\"text/html;charset=utf-8\">\n"
	"    <meta name=\"resource-type\" content=\"document\">\n"
	"    <title>Manual Page for %s(%s)</title>\n";

	css = 
	"    <link rel=\"stylesheet\" type=\"text/css\"\n"
	"         href=\"%s\">\n";
	trail = 
	"</head>\n"
	"<body>\n"
	"<div class=\"mdoc\">";

	res = 0;

	(void)snprintf(buf, sizeof(buf) - 1,
			preamble, title, ml_section(section));

	if ( ! ml_puts(mbuf, buf, &res))
		return(0);

	assert(args->params.html.css);
	if (HTML_CSS_EMBED & args->params.html.flags) {
		if ( ! ml_puts(mbuf, "    <style type=\"text/css\"><!--\n", &res))
			return(0);
		if ( ! html_loadcss(mbuf, args->params.html.css))
			return(0);
		if ( ! ml_puts(mbuf, "    --!></style>\n", &res))
			return(0);
	} else {
		(void)snprintf(buf, sizeof(buf) - 1, css, 
				args->params.html.css);
		if ( ! ml_puts(mbuf, buf, &res))
			return(0);
	}

	if ( ! ml_puts(mbuf, trail, &res))
		return(0);

	return(1);
}


/* ARGSUSED */
static int 
html_end(struct md_mbuf *mbuf, const struct md_args *args)
{

	return(ml_puts(mbuf, "</div></body>\n</html>", NULL));
}


/* ARGSUSED */
static int
html_bodytagname(struct md_mbuf *mbuf, 
		const struct md_args *args, int tok, struct htmlq *q, 
		const int *argc, const char **argv, size_t *res)
{

	switch (tok) {
	case (ROFF_Bl):
		return(html_Bl_bodytagname(mbuf, q, argc, argv, res));
	case (ROFF_Fo):
		return(ml_nputs(mbuf, "span", 4, res));
	case (ROFF_It):
		return(html_It_bodytagname(mbuf, q, argc, argv, res));
	case (ROFF_Oo):
		return(ml_nputs(mbuf, "span", 4, res));
	default:
		break;
	}

	return(ml_puts(mbuf, "div", res));
}


/* ARGSUSED */
static int
html_headtagname(struct md_mbuf *mbuf, 
		const struct md_args *args, int tok, struct htmlq *q, 
		const int *argc, const char **argv, size_t *res)
{

	switch (tok) {
	case (ROFF_It):
		return(html_It_headtagname(mbuf, q, argc, argv, res));
	case (ROFF_Fo):
		return(ml_nputs(mbuf, "span", 4, res));
	case (ROFF_Oo):
		return(ml_nputs(mbuf, "span", 4, res));
	case (ROFF_Sh):
		return(ml_nputs(mbuf, "h1", 2, res));
	case (ROFF_Ss):
		return(ml_nputs(mbuf, "h2", 2, res));
	default:
		break;
	}

	return(ml_nputs(mbuf, "div", 3, res));
}


/* ARGSUSED */
static int
html_blocktagname(struct md_mbuf *mbuf, const struct md_args *args, 
		int tok, struct htmlq *q, const int *argc, 
		const char **argv, size_t *res)
{

	switch (tok) {
	case (ROFF_Fo):
		return(ml_nputs(mbuf, "span", 4, res));
	case (ROFF_Oo):
		return(ml_nputs(mbuf, "span", 4, res));
	case (ROFF_It):
		return(html_It_blocktagname(mbuf, q, argc, argv, res));
	default:
		break;
	}

	return(ml_puts(mbuf, "div", res));
}


/* ARGSUSED */
static int
html_printargs(struct md_mbuf *mbuf, int tok, const char *ns,
		const int *argc, const char **argv, size_t *res)
{

	if ( ! ml_puts(mbuf, " class=\"", res))
		return(0);
	if ( ! ml_puts(mbuf, ns, res))
		return(0);
	if ( ! ml_puts(mbuf, "-", res))
		return(0);
	if ( ! ml_puts(mbuf, toknames[tok], res))
		return(0);
	return(ml_puts(mbuf, "\"", res));
}


/* ARGSUSED */
static int
html_headtagargs(struct md_mbuf *mbuf, 
		const struct md_args *args, int tok, 
		const int *argc, const char **argv, size_t *res)
{

	return(html_printargs(mbuf, tok, "head", argc, argv, res));
}


/* ARGSUSED */
static int
html_bodytagargs(struct md_mbuf *mbuf, 
		const struct md_args *args, int tok, 
		const int *argc, const char **argv, size_t *res)
{

	return(html_printargs(mbuf, tok, "body", argc, argv, res));
}


/* ARGSUSED */
static int
html_blocktagargs(struct md_mbuf *mbuf, 
		const struct md_args *args, int tok, 
		const int *argc, const char **argv, size_t *res)
{

	return(html_printargs(mbuf, tok, "block", argc, argv, res));
}


/* ARGSUSED */
static int
html_inlinetagargs(struct md_mbuf *mbuf, 
		const struct md_args *args, int tok, 
		const int *argc, const char **argv, size_t *res)
{

	if ( ! html_printargs(mbuf, tok, "inline", argc, argv, res))
		return(0);

	switch (tok) {
	case (ROFF_Sx):
		assert(*argv);
		if ( ! ml_nputs(mbuf, " href=\"#", 8, res))
			return(0);
		if ( ! ml_putstring(mbuf, *argv, res))
			return(0);
		if ( ! ml_nputs(mbuf, "\"", 1, res))
			return(0);
		break;
	default:
		break;
	}
	
	return(1);
}


/* ARGSUSED */
static int
html_inlinetagname(struct md_mbuf *mbuf, 
		const struct md_args *args, int tok, size_t *res)
{

	switch (tok) {
	case (ROFF_Pp):
		return(ml_nputs(mbuf, "div", 3, res));
	case (ROFF_Sx):
		return(ml_nputs(mbuf, "a", 1, res));
	default:
		break;
	}

	return(ml_puts(mbuf, "span", res));
}


static ssize_t 
html_begintag(struct md_mbuf *mbuf, void *data,
		const struct md_args *args, enum md_ns ns, 
		int tok, const int *argc, const char **argv)
{
	size_t		 res;
	struct htmlq	*q;
	struct htmlnode	*node;
	int		 i;

	assert(ns != MD_NS_DEFAULT);
	res = 0;

	assert(data);
	q = (struct htmlq *)data;

	if (NULL == (node = calloc(1, sizeof(struct htmlnode)))) {
		warn("calloc");
		return(-1);
	}

	node->parent = q->last;
	node->tok = tok;
	node->ns = ns;

	if (argc)  {
		/* TODO: argv. */

		assert(argv);
		/* LINTED */
		for (i = 0; ROFF_ARGMAX != argc[i]
				&& i < ROFF_MAXLINEARG; i++) 
			node->argc[i] = argc[i];
		assert(i != ROFF_MAXLINEARG);
	} 


	q->last = node;

	switch (ns) {
	case (MD_NS_BLOCK):
		if ( ! html_blocktagname(mbuf, args, tok, 
					q, argc, argv, &res))
			return(-1);
		if ( ! html_blocktagargs(mbuf, args, tok, 
					argc, argv, &res))
			return(-1);
		break;
	case (MD_NS_BODY):
		if ( ! html_bodytagname(mbuf, args, tok, 
					q, argc, argv, &res))
			return(-1);
		if ( ! html_bodytagargs(mbuf, args, tok, 
					argc, argv, &res))
			return(-1);
		break;
	case (MD_NS_HEAD):
		if ( ! html_headtagname(mbuf, args, tok, q,
					argc, argv, &res))
			return(-1);
		if ( ! html_headtagargs(mbuf, args, tok, 
					argc, argv, &res))
			return(-1);
		break;
	default:
		if ( ! html_inlinetagname(mbuf, args, tok, &res))
			return(-1);
		if ( ! html_inlinetagargs(mbuf, args, tok, 
					argc, argv, &res))
			return(-1);
		break;
	}

	return((ssize_t)res);
}


static ssize_t 
html_endtag(struct md_mbuf *mbuf, void *data,
		const struct md_args *args, enum md_ns ns, int tok)
{
	size_t		 res;
	struct htmlq	*q;
	struct htmlnode	*node;

	assert(ns != MD_NS_DEFAULT);
	res = 0;

	assert(data);
	q = (struct htmlq *)data;
	node = q->last;

	switch (ns) {
	case (MD_NS_BLOCK):
		if ( ! html_blocktagname(mbuf, args, tok, 
					q, node->argc, 
					(const char **)node->argv, &res))
			return(-1);
		break;
	case (MD_NS_BODY):
		if ( ! html_bodytagname(mbuf, args, tok, 
					q, node->argc, 
					(const char **)node->argv, &res))
			return(-1);
		break;
	case (MD_NS_HEAD):
		if ( ! html_headtagname(mbuf, args, tok, 
					q, node->argc,
					(const char **)node->argv, &res))
			return(-1);
		break;
	default:
		if ( ! html_inlinetagname(mbuf, args, tok, &res))
			return(-1);
		break;
	}

	q->last = node->parent;

	free(node);

	return((ssize_t)res);
}


static int
html_alloc(void **p)
{

	if (NULL == (*p = calloc(1, sizeof(struct htmlq)))) {
		warn("calloc");
		return(0);
	}
	return(1);
}


static void
html_free(void *p)
{
	struct htmlq	*q;
	struct htmlnode	*n;

	assert(p);
	q = (struct htmlq *)p;

	/* LINTED */
	while ((n = q->last)) {
		q->last = n->parent;
		free(n);
	}

	free(q);
}


static ssize_t 
html_beginhttp(struct md_mbuf *mbuf, 
		const struct md_args *args, 
		const char *buf, size_t sz)
{
	size_t		 res;

	res = 0;

	if ( ! ml_puts(mbuf, "<a href=\"", &res))
		return(-1);
	if (1 != ml_nputstring(mbuf, buf, sz, &res))
		return(-1);
	if ( ! ml_puts(mbuf, "\">", &res))
		return(-1);

	return((ssize_t)res);
}


static ssize_t 
html_endhttp(struct md_mbuf *mbuf, 
		const struct md_args *args, 
		const char *buf, size_t sz)
{
	size_t		 res;

	res = 0;

	if ( ! ml_puts(mbuf, "</a>", &res))
		return(-1);

	return((ssize_t)res);
}


/* ARGSUSED */
static ssize_t 
html_beginstring(struct md_mbuf *mbuf, 
		const struct md_args *args, 
		const char *buf, size_t sz)
{

	if (0 == strncmp(buf, "http://", 7))
		return(html_beginhttp(mbuf, args, buf, sz));

	return(0);
}


/* ARGSUSED */
static ssize_t 
html_endstring(struct md_mbuf *mbuf, 
		const struct md_args *args, 
		const char *buf, size_t sz)
{
	
	if (0 == strncmp(buf, "http://", 7))
		return(html_endhttp(mbuf, args, buf, sz));

	return(0);
}


int
md_line_html(void *data, char *buf)
{

	return(mlg_line((struct md_mlg *)data, buf));
}


int
md_exit_html(void *data, int flush)
{

	return(mlg_exit((struct md_mlg *)data, flush));
}


void *
md_init_html(const struct md_args *args,
		struct md_mbuf *mbuf, const struct md_rbuf *rbuf)
{
	struct ml_cbs	 cbs;

	cbs.ml_alloc = html_alloc;
	cbs.ml_free = html_free;
	cbs.ml_begintag = html_begintag;
	cbs.ml_endtag = html_endtag;
	cbs.ml_begin = html_begin;
	cbs.ml_end = html_end;
	cbs.ml_beginstring = html_beginstring;
	cbs.ml_endstring = html_endstring;

	return(mlg_alloc(args, rbuf, mbuf, &cbs));
}
