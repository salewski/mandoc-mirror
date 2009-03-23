/* $Id$ */
/*
 * Copyright (c) 2008, 2009 Kristaps Dzonsons <kristaps@openbsd.org>
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
#include <err.h>
#include <stdlib.h>
#include <string.h>

#include "libmdoc.h"

#ifdef __linux__
extern	size_t			strlcpy(char *, const char *, size_t);
extern	size_t			strlcat(char *, const char *, size_t);
#endif

/*
 * Contains wrappers for common functions to simplify their general
 * usage throughout this codebase.
 */

int
xstrcmp(const char *p1, const char *p2)
{

	return(0 == strcmp(p1, p2));
}

int
xstrlcat(char *dst, const char *src, size_t sz)
{

	return(strlcat(dst, src, sz) < sz);
}

int
xstrlcpy(char *dst, const char *src, size_t sz)
{

	return(strlcpy(dst, src, sz) < sz);
}

void *
xrealloc(void *ptr, size_t sz)
{
	void		*p;

	if (NULL == (p = realloc(ptr, sz)))
		err(EXIT_FAILURE, "realloc");
	return(p);
}


char *
xstrdup(const char *p)
{
	char		*pp;

	if (NULL == (pp = strdup(p)))
		err(EXIT_FAILURE, "strdup");
	return(pp);
}

int
xstrlcpys(char *buf, const struct mdoc_node *n, size_t sz)
{
	char		 *p;

	assert(sz > 0);
	assert(buf);
	*buf = 0;

	for ( ; n; n = n->next) {
		assert(MDOC_TEXT == n->type);
		p = n->string;
		if ( ! xstrlcat(buf, p, sz))
			return(0);
		if (n->next && ! xstrlcat(buf, " ", sz))
			return(0);
	}

	return(1);
}
