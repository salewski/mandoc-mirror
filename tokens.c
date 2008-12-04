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


static	int		 rofftok_dashes(const char *);
static	int		 rofftok_special(const char *);
static	int		 rofftok_predef(const char *);
static	int		 rofftok_defined(const char *);


static int
rofftok_defined(const char *buf)
{
	if (0 == *buf)
		return(-1);
	if (0 == *(buf + 1))
		return(-1);
	if (0 != *(buf + 2))
		return(-1);

	if (0 == strcmp(buf, ">="))
		return(ROFFTok_Ge);
	else if (0 == strcmp(buf, "<="))
		return(ROFFTok_Le);
	else if (0 == strcmp(buf, "Rq"))
		return(ROFFTok_Rquote);
	else if (0 == strcmp(buf, "Lq"))
		return(ROFFTok_Lquote);
	else if (0 == strcmp(buf, "ua"))
		return(ROFFTok_Uparrow);
	else if (0 == strcmp(buf, "aa"))
		return(ROFFTok_Acute);
	else if (0 == strcmp(buf, "ga"))
		return(ROFFTok_Grave);
	else if (0 == strcmp(buf, "Pi"))
		return(ROFFTok_Pi);
	else if (0 == strcmp(buf, "Ne"))
		return(ROFFTok_Ne);
	else if (0 == strcmp(buf, "Le"))
		return(ROFFTok_Le);
	else if (0 == strcmp(buf, "Ge"))
		return(ROFFTok_Ge);
	else if (0 == strcmp(buf, "Lt"))
		return(ROFFTok_Lt);
	else if (0 == strcmp(buf, "Gt"))
		return(ROFFTok_Gt);
	else if (0 == strcmp(buf, "Pm"))
		return(ROFFTok_Plusmin);
	else if (0 == strcmp(buf, "If"))
		return(ROFFTok_Infty);
	else if (0 == strcmp(buf, "Na"))
		return(ROFFTok_Nan);
	else if (0 == strcmp(buf, "Ba"))
		return(ROFFTok_Bar);

	return(-1);
}


static int
rofftok_predef(const char *buf)
{
	if (0 == *buf)
		return(-1);

	if ('(' == *buf)
		return(rofftok_defined(++buf));

	/* TODO */

	return(-1);
}


static int
rofftok_dashes(const char *buf)
{

	if (0 == *buf)
		return(-1);
	else if (*buf++ != 'e')
		return(-1);

	if (0 == *buf)
		return(-1);
	else if (0 != *(buf + 1))
		return(-1);

	switch (*buf) {
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
rofftok_special(const char *buf)
{

	if (0 == *buf)
		return(-1);
	else if (0 != *(buf + 1))
		return(-1);

	switch (*buf) {
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
	default:
		break;
	}
	return(-1);
}


int
rofftok_scan(const char *buf)
{

	assert(*buf);
	if ('\\' != *buf++)
		return(ROFFTok_MAX);

	for ( ; *buf; buf++) {
		switch (*buf) {
		case ('e'):
			return(rofftok_special(++buf));
		case ('('):
			return(rofftok_dashes(++buf));
		case (' '):
			return(ROFFTok_Space);
		case ('&'):
			return(ROFFTok_Null);
		case ('-'):
			return(ROFFTok_Hyphen);
		case ('*'):
			return(rofftok_predef(++buf));
		default:
			break;
		}
	}

	return(-1);
}


