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
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>

#include "html.h"

static	int	html_tstart(struct md_mbuf *, enum ml_scope, 
			enum html_tag, size_t *);
static	int	html_tclose(struct md_mbuf *, size_t *);

static	const char * const tagnames[] = {
	"span",		"html",		"head",		"meta",
	"title",	"style",	"link",		"body",
	"div",		"table",	"td",		"tr",
	"ol",		"ul",		"li",		"h1",
	"h2",		"a",
	};

static	const char * const attrnames[] = {
	"class",	"http-equiv",	"content",	"name",
	"type",		"rel",		"href",		"width",
	"align",	"valign",	"nowrap",
	};

static	const char * const typenames[] = {
	"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\""
		"\"http://www.w3.org/TR/html4/strict.dtd\">",
	};


/* FIXME: move into ml.c. */
static int
html_tstart(struct md_mbuf *mbuf, enum ml_scope scope, 
		enum html_tag tag, size_t *res)
{

	switch (scope) {
	case (ML_OPEN):
		if ( ! ml_nputs(mbuf, "<", 1, res))
			return(0);
		break;
	case (ML_CLOSE):
		if ( ! ml_nputs(mbuf, "</", 2, res))
			return(0);
		break;
	default:
		abort();
		/* NOTREACHED */
	}

	return(ml_puts(mbuf, tagnames[tag], res));
}


/* FIXME: move into ml.c. */
static int
html_tclose(struct md_mbuf *mbuf, size_t *res)
{

	return(ml_nputs(mbuf, ">", 1, res));
}



int 
html_stput(struct md_mbuf *mbuf, enum html_tag tag, size_t *res)
{

	return(ml_puts(mbuf, tagnames[tag], res));
}


int 
html_saput(struct md_mbuf *mbuf, enum html_tag tag,
		size_t *res, int sz, const struct html_pair *p)
{
	int		 i;
	
	if ( ! ml_puts(mbuf, tagnames[tag], res))
		return(0);

	assert(sz >= 0);
	for (i = 0; i < sz; i++) {

		/* FIXME: move into ml.c. */

		if ( ! ml_nputs(mbuf, " ", 1, res))
			return(0);
		if ( ! ml_puts(mbuf, attrnames[p[i].attr], res))
			return(0);
		if ( ! ml_nputs(mbuf, "=\"", 2, res))
			return(0);
		if ( ! ml_putstring(mbuf, p[i].val, res))
			return(0);
		if ( ! ml_nputs(mbuf, "\"", 1, res))
			return(0);
	}

	return(1);
}


int
html_tput(struct md_mbuf *mbuf, enum ml_scope scope,
		enum html_tag tag, size_t *res)
{

	if ( ! html_tstart(mbuf, scope, tag, res))
		return(0);
	return(html_tclose(mbuf, res));
}


int
html_aput(struct md_mbuf *mbuf, enum ml_scope scope,
		enum html_tag tag, size_t *res, 
		int sz, const struct html_pair *p)
{
	int		 i;

	if ( ! html_tstart(mbuf, scope, tag, res))
		return(0);

	assert(sz >= 0);
	for (i = 0; i < sz; i++) {

		/* FIXME: move into ml.c. */

		if ( ! ml_nputs(mbuf, " ", 1, res))
			return(0);
		if ( ! ml_puts(mbuf, attrnames[p[i].attr], res))
			return(0);
		if ( ! ml_nputs(mbuf, "=\"", 2, res))
			return(0);
		if ( ! ml_putstring(mbuf, p[i].val, res))
			return(0);
		if ( ! ml_nputs(mbuf, "\"", 1, res))
			return(0);
	}

	return(html_tclose(mbuf, res));
}


int
html_typeput(struct md_mbuf *mbuf, 
		enum html_type type, size_t *res)
{

	return(ml_puts(mbuf, typenames[type], res));
}


int
html_commentput(struct md_mbuf *mbuf, 
		enum ml_scope scope, size_t *res)
{

	switch (scope) {
	case (ML_OPEN):
		return(ml_nputs(mbuf, "<!--", 4, res));
	case (ML_CLOSE):
		return(ml_nputs(mbuf, "-->", 3, res));
	default:
		break;
	}

	abort();
	/* NOTREACHED */
}
