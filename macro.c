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
#include <string.h>
#ifdef	__linux__
#include <time.h>
#endif

#include "private.h"

/* FIXME: maxlineargs should be per LINE, no per TOKEN. */

static	int	  rewind_elem(struct mdoc *, int, int);
static	int	  rewind_impblock(struct mdoc *, int, int);
static	int	  rewind_expblock(struct mdoc *, int, int, int);
static	int	  rewind_head(struct mdoc *, int, int);
static	int	  rewind_body(struct mdoc *, int, int, int);
static	int	  append_delims(struct mdoc *, int, int *, char *);
static	int	  lookup(struct mdoc *, int, const char *);


static int
lookup(struct mdoc *mdoc, int from, const char *p)
{

	if ( ! (MDOC_PARSED & mdoc_macros[from].flags))
		return(MDOC_MAX);
	return(mdoc_find(mdoc, p));
}


static int
rewind_elem(struct mdoc *mdoc, int ppos, int tok)
{
	struct mdoc_node *n;

	n = mdoc->last;
	if (MDOC_ELEM != n->type)
		n = n->parent;
	assert(MDOC_ELEM == n->type);
	assert(tok == n->data.elem.tok);

	mdoc->last = n;
	mdoc->next = MDOC_NEXT_SIBLING;
	if ( ! mdoc_valid_post(mdoc, tok, ppos))
		return(0);
	return(mdoc_action(mdoc, tok, ppos));
}


static int
rewind_body(struct mdoc *mdoc, int ppos, int tok, int tt)
{
	struct mdoc_node *n;

	/* LINTED */
	for (n = mdoc->last; n; n = n->parent) {
		if (MDOC_BODY != n->type) 
			continue;
		if (tt == n->data.head.tok)
			break;
		if ( ! (MDOC_EXPLICIT & mdoc_macros[tt].flags))
			continue;
		return(mdoc_err(mdoc, tok, ppos, ERR_SCOPE_BREAK));
	}

	mdoc->last = n ? n : mdoc->last;
	mdoc->next = MDOC_NEXT_SIBLING;
	/* XXX - no validation, we do this only for blocks/elements. */
	return(1);
}


static int
rewind_head(struct mdoc *mdoc, int ppos, int tok)
{
	struct mdoc_node *n;
	int		  t;

	/* LINTED */
	for (n = mdoc->last; n; n = n->parent) {
		if (MDOC_HEAD != n->type) 
			continue;
		if (tok == (t = n->data.head.tok))
			break;
		if ( ! (MDOC_EXPLICIT & mdoc_macros[t].flags))
			continue;
		return(mdoc_err(mdoc, tok, ppos, ERR_SCOPE_BREAK));
	}

	mdoc->last = n ? n : mdoc->last;
	mdoc->next = MDOC_NEXT_SIBLING;
	/* XXX - no validation, we do this only for blocks/elements. */
	return(1);
}


static int
rewind_expblock(struct mdoc *mdoc, int ppos, int tok, int tt)
{
	struct mdoc_node *n;
	int		  t;

	assert(mdoc->last);

	/* LINTED */
	for (n = mdoc->last->parent; n; n = n->parent) {
		if (MDOC_BLOCK != n->type) 
			continue;
		if (tt == (t = n->data.block.tok))
			break;
		if (MDOC_NESTED & mdoc_macros[t].flags)
			continue;
		return(mdoc_err(mdoc, tok, ppos, ERR_SCOPE_BREAK));
	}

	if (NULL == (mdoc->last = n))
		return(mdoc_err(mdoc, tok, ppos, ERR_SCOPE_NOCTX));

	mdoc->next = MDOC_NEXT_SIBLING;
	if ( ! mdoc_valid_post(mdoc, tok, ppos))
		return(0);
	return(mdoc_action(mdoc, tok, ppos));
}


static int
rewind_impblock(struct mdoc *mdoc, int ppos, int tok)
{
	struct mdoc_node *n;
	int		  t;

	n = mdoc->last ? mdoc->last->parent : NULL;

	/* LINTED */
	for ( ; n; n = n->parent) {
		if (MDOC_BLOCK != n->type) 
			continue;
		if (tok == (t = n->data.block.tok))
			break;
		if ( ! (MDOC_EXPLICIT & mdoc_macros[t].flags))
			continue;
		if (MDOC_NESTED & mdoc_macros[tok].flags)
			return(1);
		return(mdoc_err(mdoc, tok, ppos, ERR_SCOPE_BREAK));
	}

	if (NULL == n)
		return(1);

	mdoc->next = MDOC_NEXT_SIBLING;
	mdoc->last = n;
	if ( ! mdoc_valid_post(mdoc, tok, ppos))
		return(0);
	return(mdoc_action(mdoc, tok, ppos));
}


static int
append_delims(struct mdoc *mdoc, int tok, int *pos, char *buf)
{
	int		 c, lastarg;
	char		*p;

	if (0 == buf[*pos])
		return(1);

	for (;;) {
		lastarg = *pos;
		c = mdoc_args(mdoc, tok, pos, buf, 0, &p);
		if (ARGS_ERROR == c)
			return(0);
		else if (ARGS_EOLN == c)
			break;
		assert(mdoc_isdelim(p));
		mdoc_word_alloc(mdoc, lastarg, p);
		mdoc->next = MDOC_NEXT_SIBLING;
	}

	return(1);
}


/* ARGSUSED */
int
macro_close_explicit(MACRO_PROT_ARGS)
{
	int		 tt, j, c, lastarg, maxargs, flushed;
	char		*p;

	switch (tok) {
	case (MDOC_Ac):
		tt = MDOC_Ao;
		break;
	case (MDOC_Bc):
		tt = MDOC_Bo;
		break;
	case (MDOC_Dc):
		tt = MDOC_Do;
		break;
	case (MDOC_Ec):
		tt = MDOC_Eo;
		break;
	case (MDOC_Ed):
		tt = MDOC_Bd;
		break;
	case (MDOC_Ef):
		tt = MDOC_Bf;
		break;
	case (MDOC_Ek):
		tt = MDOC_Bk;
		break;
	case (MDOC_El):
		tt = MDOC_Bl;
		break;
	case (MDOC_Fc):
		tt = MDOC_Fo;
		break;
	case (MDOC_Oc):
		tt = MDOC_Oo;
		break;
	case (MDOC_Pc):
		tt = MDOC_Po;
		break;
	case (MDOC_Qc):
		tt = MDOC_Qo;
		break;
	case (MDOC_Re):
		tt = MDOC_Rs;
		break;
	case (MDOC_Sc):
		tt = MDOC_So;
		break;
	case (MDOC_Xc):
		tt = MDOC_Xo;
		break;
	default:
		abort();
		/* NOTREACHED */
	}

	switch (tok) {
	case (MDOC_Ec):
		maxargs = 1;
		break;
	default:
		maxargs = 0;
		break;
	}

	if ( ! (MDOC_CALLABLE & mdoc_macros[tok].flags)) {
		if ( ! rewind_expblock(mdoc, ppos, tok, tt))
			return(0);
		if (0 != buf[*pos])
			return(mdoc_err(mdoc, tok, *pos, ERR_ARGS_EQ0));
		return(1);
	}

	if ( ! rewind_body(mdoc, ppos, tok, tt))
		return(0);

	lastarg = ppos;
	flushed = 0;

	if (maxargs > 0) {
		mdoc_tail_alloc(mdoc, ppos, tt);
		mdoc->next = MDOC_NEXT_CHILD;
	}

	for (j = 0; j < MDOC_LINEARG_MAX; j++) {
		lastarg = *pos;

		if (j == maxargs && ! flushed) {
			if ( ! rewind_expblock(mdoc, ppos, tok, tt))
				return(0);
			flushed = 1;
		}

		c = mdoc_args(mdoc, tok, pos, buf, ARGS_DELIM, &p);
		if (ARGS_ERROR == c)
			return(0);
		if (ARGS_PUNCT == c)
			break;
		if (ARGS_EOLN == c)
			break;

		if (MDOC_MAX != (c = lookup(mdoc, tok, p))) {
			if ( ! flushed) {
				if ( ! rewind_expblock(mdoc, ppos, tok, tt))
					return(0);
				flushed = 1;
			}
			if ( ! mdoc_macro(mdoc, c, lastarg, pos, buf))
				return(0);
			break;
		}

		mdoc_word_alloc(mdoc, lastarg, p);
		mdoc->next = MDOC_NEXT_SIBLING;
	}

	if (MDOC_LINEARG_MAX == j)
		return(mdoc_err(mdoc, tok, lastarg, ERR_ARGS_MANY));

	if ( ! flushed) 
		if ( ! rewind_expblock(mdoc, ppos, tok, tt))
			return(0);

	if (ppos > 1)
		return(1);
	return(append_delims(mdoc, tok, pos, buf));
}


/*
 * A general text domain macro.  When invoked, this opens a scope that
 * accepts words until either end-of-line, only-punctuation, or a
 * callable macro.  If the word is punctuation (not only-punctuation),
 * then the scope is closed out, the punctuation appended, then the
 * scope opened again.  If any terminating conditions are met, the scope
 * is closed out.  If this is the first macro in the line and
 * only-punctuation remains, this punctuation is flushed.
 */
int
macro_text(MACRO_PROT_ARGS)
{
	int		  lastarg, lastpunct, c, sz, fl, argc;
	struct mdoc_arg	  argv[MDOC_LINEARG_MAX];
	char		 *p;

	lastarg = ppos;
	lastpunct = 0;

	for (argc = 0; argc < MDOC_LINEARG_MAX; argc++) {
		lastarg = *pos;

		c = mdoc_argv(mdoc, tok, &argv[argc], pos, buf);
		if (ARGV_EOLN == c || ARGV_WORD == c)
			break;
		else if (ARGV_ARG == c)
			continue;
		mdoc_argv_free(argc, argv);
		return(0);
	}

	if ( ! mdoc_valid_pre(mdoc, tok, ppos, argc, argv)) {
		mdoc_argv_free(argc, argv);
		return(0);
	}

	fl = ARGS_DELIM;
	if (MDOC_QUOTABLE & mdoc_macros[tok].flags)
		fl |= ARGS_QUOTED;

	mdoc_elem_alloc(mdoc, lastarg, tok, argc, argv);
	mdoc->next = MDOC_NEXT_CHILD;

	for (lastpunct = sz = 0; sz + argc < MDOC_LINEARG_MAX; sz++) {
		lastarg = *pos;

		if (lastpunct) {
			mdoc_elem_alloc(mdoc, lastarg, tok, argc, argv);
			mdoc->next = MDOC_NEXT_CHILD;
			lastpunct = 0;
		}

		c = mdoc_args(mdoc, tok, pos, buf, fl, &p);
		if (ARGS_ERROR == c) {
			mdoc_argv_free(argc, argv);
			return(0);
		}

		if (ARGS_EOLN == c)
			break;
		if (ARGS_PUNCT == c)
			break;

		if (MDOC_MAX != (c = lookup(mdoc, tok, p))) {
			if ( ! rewind_elem(mdoc, ppos, tok)) {
				mdoc_argv_free(argc, argv);
				return(0);
			}
			mdoc_argv_free(argc, argv);
			if ( ! mdoc_macro(mdoc, c, lastarg, pos, buf))
				return(0);
			if (ppos > 1)
				return(1);
			return(append_delims(mdoc, tok, pos, buf));
		}

		if (mdoc_isdelim(p)) {
			if ( ! rewind_elem(mdoc, ppos, tok)) {
				mdoc_argv_free(argc, argv);
				return(0);
			}
			lastpunct = 1;
		}
		mdoc_word_alloc(mdoc, lastarg, p);
		mdoc->next = MDOC_NEXT_SIBLING;
	}

	mdoc_argv_free(argc, argv);

	if (sz == MDOC_LINEARG_MAX)
		return(mdoc_err(mdoc, tok, lastarg, ERR_ARGS_MANY));

	if ( ! rewind_elem(mdoc, ppos, tok))
		return(0);
	if (ppos > 1)
		return(1);
	return(append_delims(mdoc, tok, pos, buf));
}


/*
 * Multi-line-scoped macro.
 */
int
macro_scoped(MACRO_PROT_ARGS)
{
	int		  c, lastarg, argc, j;
	struct mdoc_arg	  argv[MDOC_LINEARG_MAX];
	char		 *p;

	assert ( ! (MDOC_CALLABLE & mdoc_macros[tok].flags));

	if ( ! (MDOC_EXPLICIT & mdoc_macros[tok].flags)) 
		if ( ! rewind_impblock(mdoc, ppos, tok))
			return(0);

	lastarg = ppos;

	for (argc = 0; argc < MDOC_LINEARG_MAX; argc++) {
		lastarg = *pos;
		c = mdoc_argv(mdoc, tok, &argv[argc], pos, buf);
		if (ARGV_EOLN == c || ARGV_WORD == c)
			break;
		else if (ARGV_ARG == c)
			continue;
		mdoc_argv_free(argc, argv);
		return(0);
	}

	if ( ! mdoc_valid_pre(mdoc, tok, ppos, argc, argv)) {
		mdoc_argv_free(argc, argv);
		return(0);
	}

	mdoc_block_alloc(mdoc, ppos, tok, (size_t)argc, argv);
	mdoc->next = MDOC_NEXT_CHILD;

	mdoc_argv_free(argc, argv);

	if (0 == buf[*pos]) {
		mdoc_body_alloc(mdoc, ppos, tok);
		mdoc->next = MDOC_NEXT_CHILD;
		return(1);
	}

	mdoc_head_alloc(mdoc, ppos, tok);
	mdoc->next = MDOC_NEXT_CHILD;

	for (j = 0; j < MDOC_LINEARG_MAX; j++) {
		lastarg = *pos;
		c = mdoc_args(mdoc, tok, pos, buf, ARGS_DELIM, &p);
	
		if (ARGS_ERROR == c)
			return(0);
		if (ARGS_PUNCT == c)
			break;
		if (ARGS_EOLN == c)
			break;
	
		if (MDOC_MAX == (c = lookup(mdoc, tok, p))) {
			mdoc_word_alloc(mdoc, lastarg, p);
			mdoc->next = MDOC_NEXT_SIBLING;
			continue;
		}

		if ( ! mdoc_macro(mdoc, c, lastarg, pos, buf))
			return(0);
		break;
	}

	if (j == MDOC_LINEARG_MAX)
		return(mdoc_err(mdoc, tok, lastarg, ERR_ARGS_MANY));

	if ( ! rewind_head(mdoc, ppos, tok))
		return(0);
	if (1 == ppos && ! append_delims(mdoc, tok, pos, buf))
		return(0);

	mdoc_body_alloc(mdoc, ppos, tok);
	mdoc->next = MDOC_NEXT_CHILD;

	return(1);
}


/*
 * When scoped to a line, a macro encompasses all of the contents.  This
 * differs from constants or text macros, where a new macro will
 * terminate the existing context.
 */
int
macro_scoped_line(MACRO_PROT_ARGS)
{
	int		  lastarg, c, j;
	char		  *p;

	mdoc_block_alloc(mdoc, ppos, tok, 0, NULL);
	mdoc->next = MDOC_NEXT_CHILD;

	mdoc_head_alloc(mdoc, ppos, tok);
	mdoc->next = MDOC_NEXT_CHILD;

	/* XXX - no known argument macros. */

	if ( ! mdoc_valid_pre(mdoc, tok, ppos, 0, NULL))
		return(0);

	/* Process line parameters. */

	for (lastarg = ppos, j = 0; j < MDOC_LINEARG_MAX; j++) {
		lastarg = *pos;
		c = mdoc_args(mdoc, tok, pos, buf, ARGS_DELIM, &p);

		if (ARGS_ERROR == c)
			return(0);
		if (ARGS_PUNCT == c)
			break;
		if (ARGS_EOLN == c)
			break;

		if (MDOC_MAX == (c = lookup(mdoc, tok, p))) {
			mdoc_word_alloc(mdoc, lastarg, p);
			mdoc->next = MDOC_NEXT_SIBLING;
			continue;
		}

		if ( ! mdoc_macro(mdoc, c, lastarg, pos, buf))
			return(0);
		break;
	}

	if (j == MDOC_LINEARG_MAX)
		return(mdoc_err(mdoc, tok, lastarg, ERR_ARGS_MANY));

	if (1 == ppos) {
		if ( ! rewind_head(mdoc, ppos, tok))
			return(0);
		if ( ! append_delims(mdoc, tok, pos, buf))
			return(0);
	}
	return(rewind_impblock(mdoc, ppos, tok));
}


int
macro_constant_scoped(MACRO_PROT_ARGS)
{
	int		  lastarg, flushed, j, c, maxargs;
	char		 *p;

	lastarg = ppos;
	flushed = 0;

	switch (tok) {
	case (MDOC_Eo):
		maxargs = 1;
		break;
	default:
		maxargs = 0;
		break;
	}

	if ( ! mdoc_valid_pre(mdoc, tok, ppos, 0, NULL))
		return(0);

	mdoc_block_alloc(mdoc, ppos, tok, 0, NULL);
	mdoc->next = MDOC_NEXT_CHILD;

	if (0 == maxargs) {
		mdoc_body_alloc(mdoc, ppos, tok);
		flushed = 1;
	} else
		mdoc_head_alloc(mdoc, ppos, tok);

	mdoc->next = MDOC_NEXT_CHILD;

	for (j = 0; j < MDOC_LINEARG_MAX; j++) {
		lastarg = *pos;

		if (j == maxargs && ! flushed) {
			if ( ! rewind_head(mdoc, ppos, tok))
				return(0);
			flushed = 1;
			mdoc_body_alloc(mdoc, ppos, tok);
			mdoc->next = MDOC_NEXT_CHILD;
		}

		c = mdoc_args(mdoc, tok, pos, buf, ARGS_DELIM, &p);
		if (ARGS_ERROR == c)
			return(0);
		if (ARGS_PUNCT == c)
			break;
		if (ARGS_EOLN == c)
			break;

		if (MDOC_MAX != (c = lookup(mdoc, tok, p))) {
			if ( ! flushed) {
				if ( ! rewind_head(mdoc, ppos, tok))
					return(0);
				flushed = 1;
				mdoc_body_alloc(mdoc, ppos, tok);
				mdoc->next = MDOC_NEXT_CHILD;
			}
			if ( ! mdoc_macro(mdoc, c, lastarg, pos, buf))
				return(0);
			break;
		}

		if ( ! flushed && mdoc_isdelim(p)) {
			if ( ! rewind_head(mdoc, ppos, tok))
				return(0);
			flushed = 1;
			mdoc_body_alloc(mdoc, ppos, tok);
			mdoc->next = MDOC_NEXT_CHILD;
		}
	
		mdoc_word_alloc(mdoc, lastarg, p);
		mdoc->next = MDOC_NEXT_SIBLING;
	}

	if (MDOC_LINEARG_MAX == j)
		return(mdoc_err(mdoc, tok, lastarg, ERR_ARGS_MANY));

	if ( ! flushed) {
		if ( ! rewind_head(mdoc, ppos, tok))
			return(0);
		mdoc_body_alloc(mdoc, ppos, tok);
		mdoc->next = MDOC_NEXT_CHILD;
	}

	if (ppos > 1)
		return(1);
	return(append_delims(mdoc, tok, pos, buf));
}


/*
 * Delimited macros are like text macros except that, should punctuation
 * be encountered, the macro isn't re-started with remaining tokens
 * (it's only emitted once).  Delimited macros can have a maximum number
 * of arguments.
 */
int
macro_constant_delimited(MACRO_PROT_ARGS)
{
	int		  lastarg, flushed, j, c, maxargs, argc;
	struct mdoc_arg	  argv[MDOC_LINEARG_MAX];
	char		 *p;

	lastarg = ppos;
	flushed = 0;

	switch (tok) {
	case (MDOC_No):
		/* FALLTHROUGH */
	case (MDOC_Ns):
		/* FALLTHROUGH */
	case (MDOC_Ux):
		/* FALLTHROUGH */
	case (MDOC_St):
		maxargs = 0;
		break;
	default:
		maxargs = 1;
		break;
	}

	for (argc = 0; argc < MDOC_LINEARG_MAX; argc++) {
		lastarg = *pos;
		c = mdoc_argv(mdoc, tok, &argv[argc], pos, buf);
		if (ARGV_EOLN == c || ARGV_WORD == c)
			break;
		else if (ARGV_ARG == c)
			continue;
		mdoc_argv_free(argc, argv);
		return(0);
	}

	if ( ! mdoc_valid_pre(mdoc, tok, ppos, argc, argv)) {
		mdoc_argv_free(argc, argv);
		return(0);
	}

	mdoc_elem_alloc(mdoc, lastarg, tok, argc, argv);
	mdoc->next = MDOC_NEXT_CHILD;

	mdoc_argv_free(argc, argv);

	for (j = 0; j < MDOC_LINEARG_MAX; j++) {
		lastarg = *pos;

		if (j == maxargs && ! flushed) {
			if ( ! rewind_elem(mdoc, ppos, tok))
				return(0);
			flushed = 1;
		}

		c = mdoc_args(mdoc, tok, pos, buf, ARGS_DELIM, &p);
		if (ARGS_ERROR == c)
			return(0);
		if (ARGS_PUNCT == c)
			break;
		if (ARGS_EOLN == c)
			break;

		if (MDOC_MAX != (c = lookup(mdoc, tok, p))) {
			if ( ! flushed && ! rewind_elem(mdoc, ppos, tok))
				return(0);
			flushed = 1;
			if ( ! mdoc_macro(mdoc, c, lastarg, pos, buf))
				return(0);
			break;
		}

		if ( ! flushed && mdoc_isdelim(p)) {
			if ( ! rewind_elem(mdoc, ppos, tok))
				return(0);
			flushed = 1;
		}
	
		mdoc_word_alloc(mdoc, lastarg, p);
		mdoc->next = MDOC_NEXT_SIBLING;
	}

	if (MDOC_LINEARG_MAX == j)
		return(mdoc_err(mdoc, tok, lastarg, ERR_ARGS_MANY));

	if ( ! flushed && rewind_elem(mdoc, ppos, tok))
		return(0);

	if (ppos > 1)
		return(1);
	return(append_delims(mdoc, tok, pos, buf));
}


/*
 * Constant macros span an entire line:  they constitute a macro and all
 * of its arguments and child data.
 */
int
macro_constant(MACRO_PROT_ARGS)
{
	int		 c, lastarg, argc, sz, fl;
	struct mdoc_arg	 argv[MDOC_LINEARG_MAX];
	char		*p;

	/*assert( ! (MDOC_PARSED & mdoc_macros[tok].flags));*/
	/*FIXME*/

	fl = 0;
	if (MDOC_QUOTABLE & mdoc_macros[tok].flags)
		fl = ARGS_QUOTED;

	for (argc = 0; argc < MDOC_LINEARG_MAX; argc++) {
		lastarg = *pos;
		c = mdoc_argv(mdoc, tok, &argv[argc], pos, buf);
		if (ARGV_EOLN == c) 
			break;
		else if (ARGV_ARG == c)
			continue;
		else if (ARGV_WORD == c)
			break;

		mdoc_argv_free(argc, argv);
		return(0);
	}

	if (MDOC_LINEARG_MAX == argc) {
		mdoc_argv_free(argc, argv);
		return(mdoc_err(mdoc, tok, lastarg, ERR_ARGS_MANY));
	}

	mdoc_elem_alloc(mdoc, ppos, tok, argc, argv);
	mdoc->next = MDOC_NEXT_CHILD;

	mdoc_argv_free(argc, argv);

	for (sz = 0; sz + argc < MDOC_LINEARG_MAX; sz++) {
		lastarg = *pos;
		c = mdoc_args(mdoc, tok, pos, buf, fl, &p);
		if (ARGS_ERROR == c)
			return(0);
		if (ARGS_EOLN == c)
			break;

		mdoc_word_alloc(mdoc, lastarg, p);
		mdoc->next = MDOC_NEXT_SIBLING;
	}

	if (MDOC_LINEARG_MAX == sz + argc)
		return(mdoc_err(mdoc, tok, lastarg, ERR_ARGS_MANY));

	return(rewind_elem(mdoc, ppos, tok));
}


/* ARGSUSED */
int
macro_obsolete(MACRO_PROT_ARGS)
{

	return(mdoc_warn(mdoc, tok, ppos, WARN_IGN_OBSOLETE));
}
