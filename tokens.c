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

#include "libmdocml.h"
#include "private.h"


static	int		 rofftok_dashes(const char *, int *);
static	int		 rofftok_special(const char *, int *);
static	int		 rofftok_predef(const char *, int *);
static	int		 rofftok_defined(const char *, int *);


static int
rofftok_defined(const char *buf, int *i)
{
	const char	 *p;

	if (0 == buf[*i])
		return(-1);
	if (0 == buf[*i + 1])
		return(-1);

	(*i)++;
	p = &buf[(*i)++];

	if (0 == memcmp(p, ">=", 2))
		return(ROFFTok_Ge);
	else if (0 == memcmp(p, "<=", 2))
		return(ROFFTok_Le);
	else if (0 == memcmp(p, "Rq", 2))
		return(ROFFTok_Rquote);
	else if (0 == memcmp(p, "Lq", 2))
		return(ROFFTok_Lquote);
	else if (0 == memcmp(p, "ua", 2))
		return(ROFFTok_Uparrow);
	else if (0 == memcmp(p, "aa", 2))
		return(ROFFTok_Acute);
	else if (0 == memcmp(p, "ga", 2))
		return(ROFFTok_Grave);
	else if (0 == memcmp(p, "Pi", 2))
		return(ROFFTok_Pi);
	else if (0 == memcmp(p, "Ne", 2))
		return(ROFFTok_Ne);
	else if (0 == memcmp(p, "Le", 2))
		return(ROFFTok_Le);
	else if (0 == memcmp(p, "Ge", 2))
		return(ROFFTok_Ge);
	else if (0 == memcmp(p, "Lt", 2))
		return(ROFFTok_Lt);
	else if (0 == memcmp(p, "Gt", 2))
		return(ROFFTok_Gt);
	else if (0 == memcmp(p, "Pm", 2))
		return(ROFFTok_Plusmin);
	else if (0 == memcmp(p, "If", 2))
		return(ROFFTok_Infty);
	else if (0 == memcmp(p, "Na", 2))
		return(ROFFTok_Nan);
	else if (0 == memcmp(p, "Ba", 2))
		return(ROFFTok_Bar);

	return(-1);
}


static int
rofftok_predef(const char *buf, int *i)
{
	if (0 == buf[*i])
		return(-1);
	if ('(' == buf[*i])
		return(rofftok_defined(buf, i));

	switch (buf[*i]) {
	case ('q'):
		return(ROFFTok_Quote);
	default:
		break;
	}

	return(-1);
}


static int
rofftok_dashes(const char *buf, int *i)
{

	if (0 == buf[*i])
		return(-1);
	else if (buf[(*i)++] != 'e')
		return(-1);
	if (0 == buf[*i])
		return(-1);

	switch (buf[*i]) {
	case ('m'):
		return(ROFFTok_Em);
	case ('n'):
		return(ROFFTok_En);
	default:
		break;
	}
	return(-1);
}


static int
rofftok_special(const char *buf, int *i)
{

	if (0 == buf[*i])
		return(ROFFTok_Slash);

	switch (buf[*i]) {
	case ('a'):
		return(ROFFTok_Sp_A);
	case ('b'):
		return(ROFFTok_Sp_B);
	case ('f'):
		return(ROFFTok_Sp_F);
	case ('n'):
		return(ROFFTok_Sp_N);
	case ('r'):
		return(ROFFTok_Sp_R);
	case ('t'):
		return(ROFFTok_Sp_T);
	case ('v'):
		return(ROFFTok_Sp_V);
	case ('0'):
		return(ROFFTok_Sp_0);
	default:
		break;
	}
	return(-1);
}


int
rofftok_scan(const char *buf, int *i)
{

	assert(*buf);
	assert(buf[*i] == '\\');

	(*i)++;

	for ( ; buf[*i]; (*i)++) {
		switch (buf[*i]) {
		case ('e'):
			(*i)++;
			return(rofftok_special(buf, i));
		case ('('):
			(*i)++;
			return(rofftok_dashes(buf, i));
		case (' '):
			return(ROFFTok_Space);
		case ('&'):
			return(ROFFTok_Null);
		case ('-'):
			return(ROFFTok_Hyphen);
		case ('*'):
			(*i)++;
			return(rofftok_predef(buf, i));
		case ('\\'):
			return(ROFFTok_Slash);
		default:
			break;
		}
	}

	return(-1);
}


