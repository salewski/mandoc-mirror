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
#include "ml.h"

#ifdef __linux__
extern	size_t		  strlcat(char *, const char *, size_t);
extern	size_t		  strlcpy(char *, const char *, size_t);
#endif


int
ml_nputstring(struct md_mbuf *p, 
		const char *buf, size_t sz, size_t *pos)
{
	int		 i;

	for (i = 0; i < (int)sz; i++) {
		switch (buf[i]) {
		case ('&'):
			if ( ! ml_nputs(p, "&amp;", 5, pos))
				return(0);
			break;
		case ('"'):
			if ( ! ml_nputs(p, "&quot;", 6, pos))
				return(0);
			break;
		case ('<'):
			if ( ! ml_nputs(p, "&lt;", 4, pos))
				return(0);
			break;
		case ('>'):
			if ( ! ml_nputs(p, "&gt;", 4, pos))
				return(0);
			break;
		default:
			if ( ! ml_nputs(p, &buf[i], 1, pos))
				return(0);
			break;
		}
	}
	return(1);
}


int
ml_nputs(struct md_mbuf *p, const char *buf, size_t sz, size_t *pos)
{

	if ( ! md_buf_puts(p, buf, sz))
		return(0);

	*pos += sz;
	return(1);
}


int
ml_putchars(struct md_mbuf *p, char buf, size_t count, size_t *pos)
{
	size_t		 i;

	for (i = 0; i < count; i++)
		if ( ! ml_nputs(p, &buf, 1, pos))
			return(0);

	return(1);
}
