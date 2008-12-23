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
#include <err.h>
#include <stdlib.h>
#include <string.h>

#include "private.h"


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
xcalloc(size_t num, size_t sz)
{
	void		*p;

	if (NULL == (p = calloc(num, sz)))
		err(EXIT_FAILURE, "calloc");
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

