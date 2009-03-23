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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "libman.h"

static	int	  in_line_eoln(MACRO_PROT_ARGS);

const	struct man_macro __man_macros[MAN_MAX] = {
	{ in_line_eoln, 0 },	/* MAN___ */
	{ in_line_eoln, 0 },	/* MAN_TH */
	{ in_line_eoln, 0 },	/* MAN_SH */
	{ in_line_eoln, 0 },	/* MAN_SS */
	{ in_line_eoln, 0 },	/* MAN_TP */
	{ in_line_eoln, 0 },	/* MAN_LP */
	{ in_line_eoln, 0 },	/* MAN_PP */
	{ in_line_eoln, 0 },	/* MAN_P */
	{ in_line_eoln, 0 },	/* MAN_IP */
	{ in_line_eoln, 0 },	/* MAN_HP */
	{ in_line_eoln, 0 },	/* MAN_SM */
	{ in_line_eoln, 0 },	/* MAN_SB */
	{ in_line_eoln, 0 },	/* MAN_BI */
	{ in_line_eoln, 0 },	/* MAN_IB */
	{ in_line_eoln, 0 },	/* MAN_BR */
	{ in_line_eoln, 0 },	/* MAN_RB */
	{ in_line_eoln, 0 },	/* MAN_R */
	{ in_line_eoln, 0 },	/* MAN_B */
	{ in_line_eoln, 0 },	/* MAN_I */
};

const	struct man_macro * const man_macros = __man_macros;


/*
 * In-line macro that spans an entire line.  May be callable, but has no
 * subsequent parsed arguments.
 */
static int
in_line_eoln(MACRO_PROT_ARGS)
{
#if 0
	int		  c, w, la;
	char		 *p;

	if ( ! man_elem_alloc(man, line, ppos, tok, arg))
		return(0);
	man->next = MDOC_NEXT_SIBLING;

	for (;;) {
		la = *pos;
		w = man_args(man, line, pos, buf, tok, &p);

		if (ARGS_ERROR == w)
			return(0);
		if (ARGS_EOLN == w)
			break;

		c = ARGS_QWORD == w ? MAN_MAX :
			lookup(man, line, la, tok, p);

		if (MDOC_MAX != c && -1 != c) {
			if ( ! rew_elem(mdoc, tok))
				return(0);
			return(mdoc_macro(mdoc, c, line, la, pos, buf));
		} else if (-1 == c)
			return(0);

		if ( ! mdoc_word_alloc(mdoc, line, la, p))
			return(0);
	}

	return(rew_elem(mdoc, tok));
#endif
	return(1);
}

