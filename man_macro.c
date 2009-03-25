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
#include <ctype.h>
#include <err.h> /* XXX */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "libman.h"

static	int		 man_args(struct man *, int, 
				int *, char *, char **);


int
man_macro(struct man *man, int tok, int line, 
		int ppos, int *pos, char *buf)
{
	int		 w, la;
	char		*p;
	struct man_node	*n;

	if ( ! man_elem_alloc(man, line, ppos, tok))
		return(0);
	n = man->last;
	man->next = MAN_NEXT_CHILD;

	for (;;) {
		la = *pos;
		w = man_args(man, line, pos, buf, &p);

		if (-1 == w)
			return(0);
		if (0 == w)
			break;

		if ( ! man_word_alloc(man, line, la, p))
			return(0);
		man->next = MAN_NEXT_SIBLING;
	}

	for ( ; man->last && man->last != n; 
			man->last = man->last->parent)
		if ( ! man_valid_post(man))
			return(0);

	assert(man->last);
	if ( ! man_valid_post(man))
		return(0);
	man->next = MAN_NEXT_SIBLING;

	return(1);
}


int
man_macroend(struct man *m)
{

	/* TODO: validate & actions. */
	return(1);
}


/* ARGSUSED */
static int
man_args(struct man *man, int line, 
		int *pos, char *buf, char **v)
{

	if (0 == buf[*pos])
		return(0);

	/* First parse non-quoted strings. */

	if ('\"' != buf[*pos]) {
		*v = &buf[*pos];

		while (buf[*pos]) {
			if (' ' == buf[*pos])
				if ('\\' != buf[*pos - 1])
					break;
			(*pos)++;
		}

		if (0 == buf[*pos])
			return(1);

		buf[(*pos)++] = 0;

		if (0 == buf[*pos])
			return(1);

		while (buf[*pos] && ' ' == buf[*pos])
			(*pos)++;

		if (buf[*pos])
			return(1);

		warnx("tail whitespace");
		return(-1);
	}

	/*
	 * If we're a quoted string (and quoted strings are allowed),
	 * then parse ahead to the next quote.  If none's found, it's an
	 * error.  After, parse to the next word.  
	 */

	*v = &buf[++(*pos)];

	while (buf[*pos] && '\"' != buf[*pos])
		(*pos)++;

	if (0 == buf[*pos]) {
		warnx("unterminated quotation");
		return(-1);
	}

	buf[(*pos)++] = 0;
	if (0 == buf[*pos])
		return(1);

	while (buf[*pos] && ' ' == buf[*pos])
		(*pos)++;

	if (buf[*pos])
		return(1);

	warnx("tail whitespace");
	return(-1);
}
