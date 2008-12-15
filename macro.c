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
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#include "private.h"

#define	_C(p)	((const char **)p)

static	int	  isdelim(const char *);
static	int 	  args_next(struct mdoc *, int, int *, char *, char **);
static	int	  append_text(struct mdoc *, int, int, int, char *[]);
static	int	  append_scoped(struct mdoc *, int, int, int, char *[]);


static int
isdelim(const char *p)
{

	if (0 == *p)
		return(0);
	if (0 != *(p + 1))
		return(0);

	switch (*p) {
	case('{'):
		/* FALLTHROUGH */
	case('.'):
		/* FALLTHROUGH */
	case(','):
		/* FALLTHROUGH */
	case(';'):
		/* FALLTHROUGH */
	case(':'):
		/* FALLTHROUGH */
	case('?'):
		/* FALLTHROUGH */
	case('!'):
		/* FALLTHROUGH */
	case('('):
		/* FALLTHROUGH */
	case(')'):
		/* FALLTHROUGH */
	case('['):
		/* FALLTHROUGH */
	case(']'):
		/* FALLTHROUGH */
	case('}'):
		return(1);
	default:
		break;
	}

	return(0);
}


static int
args_next(struct mdoc *mdoc, int tok, 
		int *pos, char *buf, char **v)
{

	if (0 == buf[*pos])
		return(0);

	assert( ! isspace(buf[*pos]));

	if ('\"' == buf[*pos]) {
		(void)mdoc_err(mdoc, tok, *pos, ERR_SYNTAX_QUOTE);
		return(-1);
	}

	*v = &buf[*pos];

	/* Scan ahead to end of token. */

	while (buf[*pos] && ! isspace(buf[*pos]))
		(*pos)++;

	if (buf[*pos] && buf[*pos + 1] && '\\' == buf[*pos]) {
		(void)mdoc_err(mdoc, tok, *pos, ERR_SYNTAX_WS);
		return(-1);
	}

	if (0 == buf[*pos])
		return(1);

	/* Scan ahead over trailing whitespace. */

	buf[(*pos)++] = 0;
	while (buf[*pos] && isspace(buf[*pos]))
		(*pos)++;

	if (0 == buf[*pos]) 
		if ( ! mdoc_warn(mdoc, tok, *pos, WARN_SYNTAX_WS_EOLN))
			return(-1);

	return(1);
}


static int
append_scoped(struct mdoc *mdoc, int tok, int pos, int sz, char *args[])
{

	args[sz] = NULL;
	mdoc_block_alloc(mdoc, pos, tok, 0, NULL);
	mdoc_head_alloc(mdoc, pos, tok, sz, _C(args));
	mdoc_body_alloc(mdoc, pos, tok);
	return(1);
}


static int
append_text(struct mdoc *mdoc, int tok, int pos, int sz, char *args[])
{

	args[sz] = NULL;

	switch (tok) {
	 /* ======= ADD MORE MACRO ARGUMENT-LIMITS BELOW. ======= */

	case (MDOC_Ft):
		/* FALLTHROUGH */
	case (MDOC_Li):
		/* FALLTHROUGH */
	case (MDOC_Ms):
		/* FALLTHROUGH */
	case (MDOC_Pa):
		/* FALLTHROUGH */
	case (MDOC_Tn):
		if (0 == sz && ! mdoc_warn(mdoc, tok, pos, WARN_ARGS_GE1))
			return(0);
		mdoc_elem_alloc(mdoc, pos, tok, 0, NULL, sz, _C(args));
		return(1);

	case (MDOC_Ar):
		/* FALLTHROUGH */
	case (MDOC_Cm):
		/* FALLTHROUGH */
	case (MDOC_Fl):
		mdoc_elem_alloc(mdoc, pos, tok, 0, NULL, sz, _C(args));
		return(1);

	case (MDOC_Ad):
		/* FALLTHROUGH */
	case (MDOC_Em):
		/* FALLTHROUGH */
	case (MDOC_Er):
		/* FALLTHROUGH */
	case (MDOC_Ev):
		/* FALLTHROUGH */
	case (MDOC_Fa):
		/* FALLTHROUGH */
	case (MDOC_Dv):
		/* FALLTHROUGH */
	case (MDOC_Ic):
		/* FALLTHROUGH */
	case (MDOC_Va):
		/* FALLTHROUGH */
	case (MDOC_Vt):
		if (0 == sz) 
			return(mdoc_err(mdoc, tok, pos, ERR_ARGS_GE1));
		mdoc_elem_alloc(mdoc, pos, tok, 0, NULL, sz, _C(args));
		return(1);

	 /* ======= ADD MORE MACRO ARGUMENT-LIMITS ABOVE. ======= */
	default:
		break;
	}

	abort();
	/* NOTREACHED */
}


int
macro_text(struct mdoc *mdoc, int tok, int ppos, int *pos, char *buf)
{
	int		  lastarg, j, c, lasttok, lastpunct;
	char		 *args[MDOC_LINEARG_MAX], *p;

	lasttok = ppos;
	lastpunct = 0;
	j = 0;

again:

	lastarg = *pos;
	c = args_next(mdoc, tok, pos, buf, &args[j]);
	
	if (-1 == c) 
		return(0);
	if (0 == c && ! lastpunct)
		return(append_text(mdoc, tok, lasttok, j, args));
	else if (0 == c)
		return(1);

	/* Command found. */

	if (MDOC_MAX != (c = mdoc_find(mdoc, args[j]))) {
		if ( ! lastpunct)
			if ( ! append_text(mdoc, tok, lasttok, j, args))
				return(0);
		return(mdoc_macro(mdoc, c, lastarg, pos, buf));
	}

	/* Word found. */

	if ( ! isdelim(args[j])) {
		j++;
		goto again;
	}

	/* Punctuation found.  */

	p = args[j]; /* Save argument (NULL-ified in append). */

	if ( ! lastpunct)
		if ( ! append_text(mdoc, tok, lasttok, j, args))
			return(0);

	args[j] = p;

	mdoc_word_alloc(mdoc, lastarg, args[j]);
	lastpunct = 1;
	j = 0;

	goto again;

	/* NOTREACHED */
}


int
macro_scoped_implicit(struct mdoc *mdoc, 
		int tok, int ppos, int *pos, char *buf)
{
	int		  j, c, lastarg, t;
	char		 *args[MDOC_LINEARG_MAX];
	struct mdoc_node *n;

	/*
	 * Look for an implicit parent.
	 */

	assert( ! (MDOC_EXPLICIT & mdoc_macros[tok].flags));

	for (n = mdoc->last; n; n = n->parent) {
		if (MDOC_BLOCK != n->type) 
			continue;
		if (tok == (t = n->data.block.tok))
			break;
		if ( ! (MDOC_EXPLICIT & mdoc_macros[t].flags))
			continue;
		return(mdoc_err(mdoc, tok, ppos, ERR_SCOPE_BREAK));
	}

	if (n) {
		mdoc->last = n;
		mdoc_msg(mdoc, ppos, "scope: rewound `%s'",
				mdoc_macronames[tok]);
	} else
		mdoc_msg(mdoc, ppos, "scope: new `%s'",
				mdoc_macronames[tok]);

	j = 0;

again:

	lastarg = *pos;
	c = args_next(mdoc, tok, pos, buf, &args[j]);
	
	if (-1 == c) 
		return(0);
	if (0 == c)
		return(append_scoped(mdoc, tok, ppos, j, args));

	/* Command found. */

	if (MDOC_MAX != (c = mdoc_find(mdoc, args[j])))
		if ( ! mdoc_warn(mdoc, tok, *pos, WARN_SYNTAX_MACLIKE))
			return(0);

	/* Word found. */

	j++;
	goto again;

	/* NOTREACHED */
}
