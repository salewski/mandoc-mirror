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

#include "html.h"
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


static	int		html_loadcss(struct md_mbuf *, 
				const char *);
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
static	int		html_tputln(struct md_mbuf *, 
				enum ml_scope, int, enum html_tag);
static	int		html_aputln(struct md_mbuf *, enum ml_scope, 
				int, enum html_tag, 
				int, const struct html_pair *);


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
			return(html_stput(mbuf, HTML_TAG_DIV, res));
		case (ROFF_Tag):
			/* FALLTHROUGH */
		case (ROFF_Column): 
			return(html_stput(mbuf, HTML_TAG_TD, res));
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
			return(html_stput(mbuf, HTML_TAG_DIV, res));
		case (ROFF_Tag):
			/* FALLTHROUGH */
		case (ROFF_Column): 
			return(html_stput(mbuf, HTML_TAG_TD, res));
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
			return(html_stput(mbuf, HTML_TAG_OL, res));
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
			return(html_stput(mbuf, HTML_TAG_UL, res));
		case (ROFF_Tag):
			/* FALLTHROUGH */
		case (ROFF_Column): 
			return(html_stput(mbuf, HTML_TAG_TABLE, res));
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
			return(html_stput(mbuf, HTML_TAG_LI, res));
		case (ROFF_Tag):
			/* FALLTHROUGH */
		case (ROFF_Column): 
			return(html_stput(mbuf, HTML_TAG_TR, res));
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


static int
html_tputln(struct md_mbuf *mbuf, enum ml_scope scope,
		int i, enum html_tag tag)
{

	if ( ! ml_putchars(mbuf, ' ', INDENT(i) * INDENT_SZ, NULL))
		return(0);
	if ( ! html_tput(mbuf, scope, tag, NULL))
		return(0);
	return(ml_nputs(mbuf, "\n", 1, NULL));
}


static int
html_aputln(struct md_mbuf *mbuf, enum ml_scope scope, int i, 
		enum html_tag tag, int sz, const struct html_pair *p)
{

	if ( ! ml_putchars(mbuf, ' ', INDENT(i) * INDENT_SZ, NULL))
		return(0);
	if ( ! html_aput(mbuf, scope, tag, NULL, sz, p))
		return(0);
	return(ml_nputs(mbuf, "\n", 1, NULL));
}


/* ARGSUSED */
static int 
html_begin(struct md_mbuf *mbuf, const struct md_args *args,
		const struct tm *tm, const char *os, 
		const char *name, enum roffmsec msec, const char *vol)
{
	struct html_pair attr[4];
	char		 ts[32];
	int		 i;

	(void)snprintf(ts, sizeof(ts), "%s(%s)", 
			name, roff_msecname(msec));

	i = 0;

	if ( ! html_typeput(mbuf, HTML_TYPE_4_01_STRICT, NULL))
		return(0);
	if ( ! html_tputln(mbuf, ML_OPEN, i, HTML_TAG_HTML))
		return(0);
	if ( ! html_tputln(mbuf, ML_OPEN, i++, HTML_TAG_HEAD))
		return(0);

	attr[0].attr = HTML_ATTR_HTTP_EQUIV;
	attr[0].val = "content-type";
	attr[1].attr = HTML_ATTR_CONTENT;
	attr[1].val = "text/html;charset=utf-8";

	if ( ! html_aputln(mbuf, ML_OPEN, i, HTML_TAG_META, 2, attr))
		return(0);

	attr[0].attr = HTML_ATTR_NAME;
	attr[0].val = "resource-type";
	attr[1].attr = HTML_ATTR_CONTENT;
	attr[1].val = "document";

	if ( ! html_aputln(mbuf, ML_OPEN, i, HTML_TAG_META, 2, attr))
		return(0);

	if ( ! html_tputln(mbuf, ML_OPEN, i, HTML_TAG_TITLE))
		return(0);
	if ( ! ml_putstring(mbuf, ts, NULL))
		return(0);
	if ( ! html_tputln(mbuf, ML_CLOSE, i, HTML_TAG_TITLE))
		return(0);

	if (HTML_CSS_EMBED & args->params.html.flags) {
		attr[0].attr = HTML_ATTR_TYPE;
		attr[0].val = "text/css";

		if ( ! html_aputln(mbuf, ML_OPEN, i, 
					HTML_TAG_STYLE, 1, attr))
			return(0);
		if ( ! html_commentput(mbuf, ML_OPEN, NULL))
			return(NULL);

		if ( ! html_loadcss(mbuf, args->params.html.css))
			return(0);

		if ( ! html_commentput(mbuf, ML_CLOSE, NULL))
			return(NULL);
		if ( ! html_tputln(mbuf, ML_CLOSE, i, HTML_TAG_STYLE))
			return(0);
	} else {
		attr[0].attr = HTML_ATTR_REL;
		attr[0].val = "stylesheet";
		attr[1].attr = HTML_ATTR_TYPE;
		attr[1].val = "text/css";
		attr[2].attr = HTML_ATTR_HREF;
		attr[2].val = args->params.html.css;

		if ( ! html_aputln(mbuf, ML_OPEN, i,
					HTML_TAG_LINK, 3, attr))
			return(0);
	}

	if ( ! html_tputln(mbuf, ML_CLOSE, --i, HTML_TAG_HEAD))
		return(0);
	if ( ! html_tputln(mbuf, ML_OPEN, i, HTML_TAG_BODY))
		return(0);

	attr[0].attr = HTML_ATTR_CLASS;
	attr[0].val = "mdoc";

	if ( ! html_aputln(mbuf, ML_OPEN, i, HTML_TAG_DIV, 1, attr))
		return(0);

	attr[0].attr = HTML_ATTR_WIDTH;
	attr[0].val = "100%";

	if ( ! html_aputln(mbuf, ML_OPEN, i++, HTML_TAG_TABLE, 1, attr))
		return(0);
	if ( ! html_tputln(mbuf, ML_OPEN, i++, HTML_TAG_TR))
		return(0);

	if ( ! html_tputln(mbuf, ML_OPEN, i, HTML_TAG_TD))
		return(0);
	if ( ! ml_putstring(mbuf, ts, NULL))
		return(0);
	if ( ! html_tputln(mbuf, ML_CLOSE, i, HTML_TAG_TD))
		return(0);

	if ( ! html_tputln(mbuf, ML_OPEN, i, HTML_TAG_TD))
		return(0);
	/* TODO: middle. */
	if ( ! html_tputln(mbuf, ML_CLOSE, i, HTML_TAG_TD))
		return(0);

	attr[0].attr = HTML_ATTR_ALIGN;
	attr[0].val = "right";

	if ( ! html_aputln(mbuf, ML_OPEN, i, HTML_TAG_TD, 1, attr))
		return(0);
	if ( ! ml_putstring(mbuf, ts, NULL))
		return(0);
	if ( ! html_tputln(mbuf, ML_CLOSE, i, HTML_TAG_TD))
		return(0);

	if ( ! html_tputln(mbuf, ML_CLOSE, --i, HTML_TAG_TR))
		return(0);
	return(html_tputln(mbuf, ML_CLOSE, --i, HTML_TAG_TABLE));
}


/* ARGSUSED */
static int 
html_end(struct md_mbuf *mbuf, const struct md_args *args)
{

	if ( ! html_tputln(mbuf, ML_CLOSE, 0, HTML_TAG_DIV))
		return(0);
	if ( ! html_tputln(mbuf, ML_CLOSE, 0, HTML_TAG_BODY))
		return(0);
	return(html_tputln(mbuf, ML_CLOSE, 0, HTML_TAG_HTML));
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
		return(html_stput(mbuf, HTML_TAG_SPAN, res));
	case (ROFF_It):
		return(html_It_bodytagname(mbuf, q, argc, argv, res));
	case (ROFF_Oo):
		return(html_stput(mbuf, HTML_TAG_SPAN, res));
	default:
		break;
	}

	return(html_stput(mbuf, HTML_TAG_DIV, res));
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
		/* FALLTHROUGH */
	case (ROFF_Oo):
		return(html_stput(mbuf, HTML_TAG_SPAN, res));
	case (ROFF_Sh):
		return(html_stput(mbuf, HTML_TAG_H1, res));
	case (ROFF_Ss):
		return(html_stput(mbuf, HTML_TAG_H2, res));
	default:
		break;
	}

	return(html_stput(mbuf, HTML_TAG_DIV, res));
}


/* ARGSUSED */
static int
html_blocktagname(struct md_mbuf *mbuf, const struct md_args *args, 
		int tok, struct htmlq *q, const int *argc, 
		const char **argv, size_t *res)
{

	switch (tok) {
	case (ROFF_Fo):
		/* FALLTHROUGH */
	case (ROFF_Oo):
		return(html_stput(mbuf, HTML_TAG_SPAN, res));
	case (ROFF_It):
		return(html_It_blocktagname(mbuf, q, argc, argv, res));
	default:
		break;
	}

	return(html_stput(mbuf, HTML_TAG_DIV, res));
}


/* ARGSUSED */
static int
html_printargs(struct md_mbuf *mbuf, int tok, const char *ns,
		const int *argc, const char **argv, size_t *res)
{

	/* FIXME: use API in ml.h. */

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

		/* FIXME: use API in ml.h. */

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
		return(html_stput(mbuf, HTML_TAG_DIV, res));
	case (ROFF_Sx):
		return(html_stput(mbuf, HTML_TAG_A, res));
	default:
		break;
	}

	return(html_stput(mbuf, HTML_TAG_SPAN, res));
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
	struct html_pair pair;

	res = 0;
	pair.attr = HTML_ATTR_HREF;
	pair.val = (char *)buf;

	if ( ! html_aput(mbuf, ML_OPEN, HTML_TAG_A, &res, 1, &pair))
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
	if ( ! html_tput(mbuf, ML_CLOSE, HTML_TAG_A, &res))
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
