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

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "mdoc.h"
#include "man.h"
#include "out.h"

int
out_a2list(const struct mdoc_node *n)
{
	int		 i;

	assert(MDOC_BLOCK == n->type && MDOC_Bl == n->tok);
	assert(n->args);

	for (i = 0; i < (int)n->args->argc; i++) 
		switch (n->args->argv[i].arg) {
		case (MDOC_Enum):
			/* FALLTHROUGH */
		case (MDOC_Dash):
			/* FALLTHROUGH */
		case (MDOC_Hyphen):
			/* FALLTHROUGH */
		case (MDOC_Bullet):
			/* FALLTHROUGH */
		case (MDOC_Tag):
			/* FALLTHROUGH */
		case (MDOC_Hang):
			/* FALLTHROUGH */
		case (MDOC_Inset):
			/* FALLTHROUGH */
		case (MDOC_Diag):
			/* FALLTHROUGH */
		case (MDOC_Item):
			/* FALLTHROUGH */
		case (MDOC_Column):
			/* FALLTHROUGH */
		case (MDOC_Ohang):
			return(n->args->argv[i].arg);
		default:
			break;
		}

	abort();
	/* NOTREACHED */
}


int
out_a2width(const char *p)
{
	int		 i, len;

	if (0 == (len = (int)strlen(p)))
		return(0);
	for (i = 0; i < len - 1; i++) 
		if ( ! isdigit((u_char)p[i]))
			break;

	if (i == len - 1) 
		if ('n' == p[len - 1] || 'm' == p[len - 1])
			return(atoi(p) + 2);

	return(len + 2);
}


int
out_a2offs(const char *p, int indent)
{
	int		 len, i;

	if (0 == strcmp(p, "left"))
		return(0);
	if (0 == strcmp(p, "indent"))
		return(indent + 1);
	if (0 == strcmp(p, "indent-two"))
		return((indent + 1) * 2);

	if (0 == (len = (int)strlen(p)))
		return(0);

	for (i = 0; i < len - 1; i++) 
		if ( ! isdigit((u_char)p[i]))
			break;

	if (i == len - 1) 
		if ('n' == p[len - 1] || 'm' == p[len - 1])
			return(atoi(p));

	return(len);
}

