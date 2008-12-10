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
#include <stdlib.h>
#include <string.h>

#include "ml.h"

#ifdef __linux__
extern	size_t		  strlcat(char *, const char *, size_t);
extern	size_t		  strlcpy(char *, const char *, size_t);
#endif


int
ml_putstring(struct md_mbuf *p, const char *buf, size_t *pos)
{

	return(ml_nputstring(p, buf, strlen(buf), pos));
}


int
ml_nputstring(struct md_mbuf *p, 
		const char *buf, size_t sz, size_t *pos)
{
	int		 i, v;
	const char	*seq;
	size_t		 ssz;

	for (i = 0; i < (int)sz; i++) {
		switch (buf[i]) {

		/* Escaped value. */
		case ('\\'):
			if (-1 == (v = rofftok_scan(buf, &i)))
				return(0);

			switch (v) {
			case (ROFFTok_Sp_A):
				seq = "\\a";
				ssz = 2;
				break;
			case (ROFFTok_Sp_B):
				seq = "\\b";
				ssz = 2;
				break;
			case (ROFFTok_Sp_F):
				seq = "\\f";
				ssz = 2;
				break;
			case (ROFFTok_Sp_N):
				seq = "\\n";
				ssz = 2;
				break;
			case (ROFFTok_Sp_R):
				seq = "\\r";
				ssz = 2;
				break;
			case (ROFFTok_Sp_T):
				seq = "\\t";
				ssz = 2;
				break;
			case (ROFFTok_Sp_V):
				seq = "\\v";
				ssz = 2;
				break;
			case (ROFFTok_Sp_0):
				seq = "\\0";
				ssz = 2;
				break;
			case (ROFFTok_Space):
				seq = "&nbsp;";
				ssz = 6;
				break;
			case (ROFFTok_Hyphen):
				seq = "&#8208;";
				ssz = 7;
				break;
			case (ROFFTok_Em):
				seq = "&#8212;";
				ssz = 7;
				break;
			case (ROFFTok_En):
				seq = "&#8211;";
				ssz = 7;
				break;
			case (ROFFTok_Ge):
				seq = "&#8805;";
				ssz = 7;
				break;
			case (ROFFTok_Le):
				seq = "&#8804;";
				ssz = 7;
				break;
			case (ROFFTok_Rquote):
				seq = "&#8221;";
				ssz = 7;
				break;
			case (ROFFTok_Lquote):
				seq = "&#8220;";
				ssz = 7;
				break;
			case (ROFFTok_Uparrow):
				seq = "&#8593;";
				ssz = 7;
				break;
			case (ROFFTok_Acute):
				seq = "&#180;";
				ssz = 6;
				break;
			case (ROFFTok_Grave):
				seq = "&#96;";
				ssz = 5;
				break;
			case (ROFFTok_Pi):
				seq = "&#960;";
				ssz = 6;
				break;
			case (ROFFTok_Ne):
				seq = "&#8800;";
				ssz = 7;
				break;
			case (ROFFTok_Lt):
				seq = "&lt;";
				ssz = 4;
				break;
			case (ROFFTok_Gt):
				seq = "&gt;";
				ssz = 4;
				break;
			case (ROFFTok_Plusmin):
				seq = "&#177;";
				ssz = 6;
				break;
			case (ROFFTok_Infty):
				seq = "&#8734;";
				ssz = 7;
				break;
			case (ROFFTok_Bar):
				seq = "&#124;";
				ssz = 6;
				break;
			case (ROFFTok_Nan):
				seq = "Nan";
				ssz = 3;
				break;
			case (ROFFTok_Quote):
				seq = "&quot;";
				ssz = 6;
				break;
			case (ROFFTok_Slash):
				seq = "\\";
				ssz = 1;
				break;
			case (ROFFTok_Null):
				seq = "";
				ssz = 0;
				break;
			default:
				return(0);
			}
			break;

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

		if (ssz > 0 && ! ml_nputs(p, seq, ssz, pos))
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

	if (pos)
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

	if (pos)
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
