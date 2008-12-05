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
/* FIXME: free htmlq. */

struct	htmlnode {
	int		 tok;
	enum md_ns	 ns;
	int		*argc[ROFF_MAXLINEARG];
	char		*argv[ROFF_MAXLINEARG];
	struct htmlnode	*parent;
};


struct	htmlq {
	struct htmlnode	*last;
};


static	void		htmlnode_free(struct htmlnode *);
static	void		htmlnode_free(struct htmlnode *);

static	int		html_loadcss(struct md_mbuf *, const char *);

static	ssize_t		html_endtag(struct md_mbuf *, void *,
				const struct md_args *, 
				enum md_ns, int);
static	ssize_t		html_begintag(struct md_mbuf *, void *,
				const struct md_args *, 
				enum md_ns, int, 
				const int *, const char **);
static	int		html_begin(struct md_mbuf *,
	       			const struct md_args *, 
				const struct tm *, 
				const char *, const char *, 
				const char *, const char *);
static	int		html_printargs(struct md_mbuf *, int, 
				const char *, const int *, 
				const char **, size_t *);
static	int		html_end(struct md_mbuf *, 
				const struct md_args *);
static	int		html_blocktagname(struct md_mbuf *,
				const struct md_args *, int, size_t *);
static	int		html_blocktagargs(struct md_mbuf *,
				const struct md_args *, int,
				const int *, const char **, size_t *);
static	int		html_blockheadtagname(struct md_mbuf *,
				const struct md_args *, int, size_t *);
static	int		html_blockheadtagargs(struct md_mbuf *,
				const struct md_args *, int,
				const int *, const char **, size_t *);
static	int		html_blockbodytagname(struct md_mbuf *,
				const struct md_args *, int, size_t *);
static	int		html_blockbodytagargs(struct md_mbuf *,
				const struct md_args *, int,
				const int *, const char **, size_t *);
static	int		html_inlinetagname(struct md_mbuf *,
				const struct md_args *, int, size_t *);
static	int		html_inlinetagargs(struct md_mbuf *,
				const struct md_args *, int,
				const int *, const char **, size_t *);


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
		const char *title, const char *section, 
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
	"<div class=\"mdoc\">\n";

	res = 0;

	(void)snprintf(buf, sizeof(buf) - 1,
			preamble, title, section);

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
	size_t		 res;

	res = 0;
	if ( ! ml_puts(mbuf, "</div></body>\n</html>", &res))
		return(0);

	return(1);
}


/* ARGSUSED */
static int
html_blockbodytagname(struct md_mbuf *mbuf, 
		const struct md_args *args, int tok, size_t *res)
{

	return(ml_puts(mbuf, "div", res));
}




/* ARGSUSED */
static int
html_blockheadtagname(struct md_mbuf *mbuf, 
		const struct md_args *args, int tok, size_t *res)
{

	return(ml_puts(mbuf, "div", res));
}


/* ARGSUSED */
static int
html_blocktagname(struct md_mbuf *mbuf, 
		const struct md_args *args, int tok, size_t *res)
{

	return(ml_puts(mbuf, "div", res));
}


static int
html_printargs(struct md_mbuf *mbuf, int tok, const char *ns,
		const int *argc, const char **argv, size_t *res)
{
	int		 i, c;

	if ( ! ml_puts(mbuf, " class=\"", res))
		return(0);
	if ( ! ml_puts(mbuf, ns, res))
		return(0);
	if ( ! ml_puts(mbuf, "-", res))
		return(0);
	if ( ! ml_puts(mbuf, toknames[tok], res))
		return(0);
	if ( ! ml_puts(mbuf, "\"", res))
		return(0);

	if (NULL == argv || NULL == argc)
		return(1);
	assert(argv && argc);
	
	/* FIXME: ignores values. */

	for (i = 0; ROFF_ARGMAX != (c = argc[i]); i++) {
		if (argv[i])
			continue;
		if ( ! ml_puts(mbuf, " class=\"", res))
			return(0);
		if ( ! ml_puts(mbuf, ns, res))
			return(0);
		if ( ! ml_puts(mbuf, "-", res))
			return(0);
		if ( ! ml_puts(mbuf, toknames[tok], res))
			return(0);
		if ( ! ml_puts(mbuf, "-", res))
			return(0);
		if ( ! ml_puts(mbuf, tokargnames[c], res))
			return(0);
		if ( ! ml_puts(mbuf, "\"", res))
			return(0);
	}

	return(1);
}


/* ARGSUSED */
static int
html_blockheadtagargs(struct md_mbuf *mbuf, 
		const struct md_args *args, int tok, 
		const int *argc, const char **argv, size_t *res)
{

	return(html_printargs(mbuf, tok, "head", argc, argv, res));
}


/* ARGSUSED */
static int
html_blockbodytagargs(struct md_mbuf *mbuf, 
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

	return(html_printargs(mbuf, tok, "inline", argc, argv, res));
}


/* ARGSUSED */
static int
html_inlinetagname(struct md_mbuf *mbuf, 
		const struct md_args *args, int tok, size_t *res)
{

	switch (tok) {
	case (ROFF_Pp):
		return(ml_puts(mbuf, "div", res));
	default:
		return(ml_puts(mbuf, "span", res));
	}
	return(1);
}


static ssize_t 
html_begintag(struct md_mbuf *mbuf, void *data,
		const struct md_args *args, enum md_ns ns, 
		int tok, const int *argc, const char **argv)
{
	size_t		 res;
	struct htmlq	*q;
	struct htmlnode	*node;

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

	q->last = node;

	switch (ns) {
	case (MD_NS_BLOCK):
		if ( ! html_blocktagname(mbuf, args, tok, &res))
			return(-1);
		if ( ! html_blocktagargs(mbuf, args, tok, 
					argc, argv, &res))
			return(-1);
		break;
	case (MD_NS_BODY):
		if ( ! html_blockbodytagname(mbuf, args, tok, &res))
			return(-1);
		if ( ! html_blockbodytagargs(mbuf, args, tok, 
					argc, argv, &res))
			return(-1);
		break;
	case (MD_NS_HEAD):
		if ( ! html_blockheadtagname(mbuf, args, tok, &res))
			return(-1);
		if ( ! html_blockheadtagargs(mbuf, args, tok, 
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

	switch (ns) {
	case (MD_NS_BLOCK):
		if ( ! html_blocktagname(mbuf, args, tok, &res))
			return(-1);
		break;
	case (MD_NS_BODY):
		if ( ! html_blockbodytagname(mbuf, args, tok, &res))
			return(-1);
		break;
	case (MD_NS_HEAD):
		if ( ! html_blockheadtagname(mbuf, args, tok, &res))
			return(-1);
		break;
	default:
		if ( ! html_inlinetagname(mbuf, args, tok, &res))
			return(-1);
		break;
	}

	node = q->last;
	q->last = node->parent;

	htmlnode_free(node);

	return((ssize_t)res);
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
	struct htmlq	*q;

	if (NULL == (q = calloc(1, sizeof(struct htmlq)))) {
		warn("calloc");
		return(NULL);
	}

	return(mlg_alloc(args, q, rbuf, mbuf, html_begintag, 
				html_endtag, html_begin, html_end));
}

