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
	const char	*seq;
	size_t		 ssz;

	for (i = 0; i < (int)sz; i++) {
		switch (buf[i]) {

		/* Ampersand ml-escape. */
		case ('&'):
			seq = "&amp;";
			ssz = 5;
			break;

		/* Quotation ml-escape. */
		case ('"'):
			seq = "&quot;";
			ssz = 6;
			break;

		/* Lt ml-escape. */
		case ('<'):
			seq = "&lt;";
			ssz = 4;
			break;

		/* Gt ml-escape. */
		case ('>'):
			seq = "&gt;";
			ssz = 4;
			break;

		default:
			seq = &buf[i];
			ssz = 1;
			break;
		}

		if ( ! ml_nputs(p, seq, ssz, pos))
			return(-1);
	}
	return(1);
}


int
ml_nputs(struct md_mbuf *p, const char *buf, size_t sz, size_t *pos)
{

	if (0 == sz)
		return(1);

	if ( ! md_buf_puts(p, buf, sz))
		return(0);

	*pos += sz;
	return(1);
}


int
ml_puts(struct md_mbuf *p, const char *buf, size_t *pos)
{
	size_t		 sz;

	if (0 == (sz = strlen(buf)))
		return(1);

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
