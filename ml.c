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
#include <stdlib.h>
#include <string.h>

#include "libmdocml.h"
#include "private.h"


#define	MAXINDENT	 8

static ssize_t		 ml_puts(struct md_mbuf *, const char *);
static ssize_t		 ml_putchar(struct md_mbuf *, char);
static ssize_t		 ml_putstring(struct md_mbuf *, const char *);


static ssize_t
ml_puts(struct md_mbuf *p, const char *buf)
{

	return(ml_nputs(p, buf, strlen(buf)));
}


static ssize_t
ml_putchar(struct md_mbuf *p, char buf)
{

	return(ml_nputs(p, &buf, 1));
}


static ssize_t
ml_putstring(struct md_mbuf *p, const char *buf)
{
	
	return(ml_nputstring(p, buf, strlen(buf)));
}


ssize_t
ml_begintag(struct md_mbuf *p, const char *name, 
		int *argc, char **argv)
{
	int		 i;
	ssize_t		 res, sz;

	res = 0;

	if (-1 == (sz = ml_nputs(p, "<", 1)))
		return(-1);
	res += sz;

	if (-1 == (sz = ml_puts(p, name)))
		return(-1);
	res += sz;

	for (i = 0; ROFF_ARGMAX != argc[i]; i++) {
		if (-1 == (sz = ml_nputs(p, " ", 1)))
			return(-1);
		res += sz;

		if (-1 == (sz = ml_puts(p, tokargnames[argc[i]])))
			return(-1);
		res += sz;

		if (-1 == (sz = ml_nputs(p, "=\"", 2)))
			return(-1);
		res += sz;

		if (-1 == (sz = ml_putstring(p, argv[i] ? 
						argv[i] : "true")))
			return(-1);
		res += sz;

		if (-1 == (sz = ml_nputs(p, "\"", 1)))
			return(-1);
		res += sz;
	}
	
	if (-1 == (sz = ml_nputs(p, ">", 1)))
		return(-1);

	return(res + sz);
}


ssize_t
ml_endtag(struct md_mbuf *p, const char *tag)
{
	ssize_t		 res, sz;

	res = 0;

	if (-1 == (sz = ml_nputs(p, "</", 2)))
		return(-1);
	res += sz;

	if (-1 == (sz = ml_puts(p, tag)))
		return(-1);
	res += sz;

	if (-1 == (sz = ml_nputs(p, ">", 1)))
		return(-1);

	return(res + sz);
}


ssize_t
ml_nputstring(struct md_mbuf *p, const char *buf, size_t bufsz)
{
	int		 i;
	ssize_t		 res, sz;

	res = 0;

	for (i = 0; i < (int)bufsz; i++) {
		switch (buf[i]) {
		case ('&'):
			if (-1 == (sz = ml_nputs(p, "&amp;", 5)))
				return(-1);
			break;
		case ('"'):
			if (-1 == (sz = ml_nputs(p, "&quot;", 6)))
				return(-1);
			break;
		case ('<'):
			if (-1 == (sz = ml_nputs(p, "&lt;", 4)))
				return(-1);
			break;
		case ('>'):
			if (-1 == (sz = ml_nputs(p, "&gt;", 4)))
				return(-1);
			break;
		default:
			if (-1 == (sz = ml_putchar(p, buf[i])))
				return(-1);
			break;
		}
		res += sz;
	}
	return(res);
}


ssize_t
ml_nputs(struct md_mbuf *p, const char *buf, size_t sz)
{

	return(0 == md_buf_puts(p, buf, sz) ? -1 : (ssize_t)sz);
}


ssize_t
ml_indent(struct md_mbuf *p, int indent)
{
	size_t		 i;
	ssize_t		 res, sz;

	res = sz  0;

	/* LINTED */
	for (i = 0; i < MIN(indent, MAXINDENT); i++, res += sz)
		if (-1 == (sz = ml_nputs(p, "    ", 4)))
			return(-1);
	return(res);
}
