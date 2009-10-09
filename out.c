/*	$Id$ */
/*
 * Copyright (c) 2009 Kristaps Dzonsons <kristaps@kth.se>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <sys/types.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "out.h"


/* 
 * Convert a `scaling unit' to a consistent form, or fail.  Scaling
 * units are documented in groff.7, which stipulates the following:
 *
 *  (1) A scaling unit is a signed/unsigned integer/float with or
 *      without a unit.
 *
 *  (2) The following units exist:
 *	c	Centimeter
 *	i	Inch
 *	P	Pica = 1/6 inch
 *	p	Point = 1/72 inch
 *	m	Em = the font size in points (width of letter m)
 *	M	100th of an Em 
 *	n	En = Em/2 
 *	u	Basic unit for actual output device
 *	v	Vertical line space in basic units scaled point =
 *		1/sizescale of a point (defined in font DESC file)
 *	f	Scale by 65536.
 */
int
a2roffsu(const char *src, struct roffsu *dst)
{
	char		 buf[BUFSIZ], hasd;
	int		 i;
	enum roffscale	 unit;

	i = hasd = 0;

	switch (*src) {
	case ('+'):
		src++;
		break;
	case ('-'):
		buf[i++] = *src++;
		break;
	default:
		break;
	}

	while (i < BUFSIZ) {
		if ( ! isdigit((u_char)*src)) {
			if ('.' != *src)
				break;
			else if (hasd)
				break;
			else
				hasd = 1;
		}
		buf[i++] = *src++;
	}

	if (BUFSIZ == i || (*src && *(src + 1)))
		return(0);

	buf[i] = '\0';

	switch (*src) {
	case ('c'):
		unit = SCALE_CM;
		break;
	case ('i'):
		unit = SCALE_IN;
		break;
	case ('P'):
		unit = SCALE_PC;
		break;
	case ('p'):
		unit = SCALE_PT;
		break;
	case ('f'):
		unit = SCALE_FS;
		break;
	case ('v'):
		unit = SCALE_VS;
		break;
	case ('m'):
		unit = SCALE_EM;
		break;
	case ('\0'):
		/* FALLTHROUGH */
	case ('u'):
		unit = SCALE_BU;
		break;
	case ('M'):
		unit = SCALE_MM;
		break;
	case ('n'):
		unit = SCALE_EN;
		break;
	default:
		return(0);
	}

	if ((dst->scale = atof(buf)) < 0)
		dst->scale = 0;
	dst->unit = unit;
	dst->pt = hasd;

	return(1);
}
