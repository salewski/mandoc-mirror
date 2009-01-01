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

#define	_CC(p)	((const char **)p)

static	int	  scope_rewind_imp(struct mdoc *, int, int);
static	int	  scope_rewind_exp(struct mdoc *, int, int, int);
static	int	  scope_rewind_line(struct mdoc *, int, int);
static	int	  append_text(struct mdoc *, int, 
			int, int, char *[]);
static	int	  append_text_argv(struct mdoc *, int, int, 
			int, char *[], int, const struct mdoc_arg *);
static	int	  append_delims(struct mdoc *, int, int *, char *);


static int
scope_rewind_line(struct mdoc *mdoc, int ppos, int tok)
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
	return(1);
}


static int
scope_rewind_exp(struct mdoc *mdoc, int ppos, int tok, int tt)
{
	struct mdoc_node *n;

	assert(mdoc->last);

	/* LINTED */
	for (n = mdoc->last->parent; n; n = n->parent) {
		if (MDOC_BLOCK != n->type) 
			continue;
		if (tt == n->data.block.tok)
			break;
		return(mdoc_err(mdoc, tok, ppos, ERR_SCOPE_BREAK));
	}

	if (NULL == (mdoc->last = n))
		return(mdoc_err(mdoc, tok, ppos, ERR_SCOPE_NOCTX));

	mdoc->next = MDOC_NEXT_SIBLING;
	return(mdoc_valid_post(mdoc, tok, ppos));
}


static int
scope_rewind_imp(struct mdoc *mdoc, int ppos, int tok)
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
		return(mdoc_err(mdoc, tok, ppos, ERR_SCOPE_BREAK));
	}

	mdoc->last = n ? n : mdoc->last;
	mdoc->next = MDOC_NEXT_SIBLING;
	return(mdoc_valid_post(mdoc, tok, ppos));
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


static int
append_text_argv(struct mdoc *mdoc, int tok, int pos, 
		int sz, char *args[],
		int argc, const struct mdoc_arg *argv)
{

	if ( ! mdoc_valid_pre(mdoc, tok, pos, 0, NULL, argc, argv))
		return(0);
	mdoc_elem_alloc(mdoc, pos, tok, (size_t)argc, 
			argv, (size_t)sz, _CC(args));
	mdoc->next = MDOC_NEXT_SIBLING;
	return(1);
}


static int
append_text(struct mdoc *mdoc, int tok, 
		int pos, int sz, char *args[])
{

	if ( ! mdoc_valid_pre(mdoc, tok, pos, sz, _CC(args), 0, NULL))
		return(0);

	switch (tok) {
	case (MDOC_At):
		if (0 == sz)
			break;
		if (ATT_DEFAULT == mdoc_atoatt(args[0])) {
			mdoc_elem_alloc(mdoc, pos, tok, 
					0, NULL, 0, NULL);
			mdoc->next = MDOC_NEXT_SIBLING;
			mdoc_word_alloc(mdoc, pos, args[0]);
			mdoc->next = MDOC_NEXT_SIBLING;
		} else {
			mdoc_elem_alloc(mdoc, pos, tok, 0, 
					NULL, 1, _CC(&args[0]));
			mdoc->next = MDOC_NEXT_SIBLING;
		}

		if (sz > 1)
			mdoc_word_alloc(mdoc, pos, args[1]);
		mdoc->next = MDOC_NEXT_SIBLING;
		return(1);

	default:
		break;
	}

	mdoc_elem_alloc(mdoc, pos, tok, 0, NULL, (size_t)sz, _CC(args));
	mdoc->next = MDOC_NEXT_SIBLING;
	return(1);
}


int
macro_text(MACRO_PROT_ARGS)
{
	int		  lastarg, lastpunct, c, j, fl;
	char		 *args[MDOC_LINEARG_MAX];

	/* Token pre-processing.  */

	switch (tok) {
	case (MDOC_Pp):
		/* `.Pp' ignored when following `.Sh' or `.Ss'. */
		assert(mdoc->last);
		if (MDOC_BODY != mdoc->last->type)
			break;
		switch (mdoc->last->data.body.tok) {
		case (MDOC_Ss):
			/* FALLTHROUGH */
		case (MDOC_Sh):
			if ( ! mdoc_warn(mdoc, tok, ppos, WARN_IGN_AFTER_BLK))
				return(0);
			return(1);
		default:
			break;
		}
		break;
	default:
		break;
	}

	/* Process line parameters. */

	j = 0;
	lastarg = ppos;
	lastpunct = 0;
	fl = ARGS_DELIM;

	if (MDOC_QUOTABLE & mdoc_macros[tok].flags)
		fl |= ARGS_QUOTED;

again:
	if (j == MDOC_LINEARG_MAX)
		return(mdoc_err(mdoc, tok, lastarg, ERR_ARGS_MANY));

	/* 
	 * Parse out the next argument, unquoted and unescaped.   If
	 * we're a word (which may be punctuation followed eventually by
	 * a real word), then fall into checking for callables.  If
	 * only punctuation remains and we're the first, then flush
	 * arguments, punctuation and exit; else, return to the caller.
	 */

	lastarg = *pos;

	switch (mdoc_args(mdoc, tok, pos, buf, fl, &args[j])) {
	case (ARGS_ERROR):
		return(0);
	case (ARGS_WORD):
		break;
	case (ARGS_PUNCT):
		if ( ! lastpunct && ! append_text(mdoc, tok, ppos, j, args))
			return(0);
		if (ppos > 1)
			return(1);
		return(append_delims(mdoc, tok, pos, buf));
	case (ARGS_EOLN):
		if (lastpunct)
			return(1);
		return(append_text(mdoc, tok, ppos, j, args));
	default:
		abort();
		/* NOTREACHED */
	}

	/* 
	 * Command found.  First flush out arguments, then call the
	 * command.  If we're the line macro when it exits, flush
	 * terminal punctuation.
	 */

	if (MDOC_MAX != (c = mdoc_find(mdoc, args[j]))) {
		if ( ! lastpunct && ! append_text(mdoc, tok, ppos, j, args))
			return(0);
		if ( ! mdoc_macro(mdoc, c, lastarg, pos, buf))
			return(0);
		if (ppos > 1)
			return(1);
		return(append_delims(mdoc, tok, pos, buf));
	}

	/* Word/non-term-punctuation found. */

	if ( ! mdoc_isdelim(args[j])) {
		/* Words are appended to the array of arguments. */
		j++;
		lastpunct = 0;
		goto again;
	}

	/* 
	 * For punctuation, flush all collected words, then flush
	 * punctuation, then start collecting again.   Of course, this
	 * is non-terminal punctuation.
	 */

	if ( ! lastpunct && ! append_text(mdoc, tok, ppos, j, args))
		return(0);

	mdoc_word_alloc(mdoc, lastarg, args[j]);
	mdoc->next = MDOC_NEXT_SIBLING;
	j = 0;
	lastpunct = 1;

	goto again;
	/* NOTREACHED */
}



/* ARGSUSED */
int
macro_close_explicit(MACRO_PROT_ARGS)
{
	int		 tt;

	/*
	 * First close out the explicit scope.  The `end' tags (such as
	 * `.El' to `.Bl' don't cause anything to happen: we merely
	 * readjust our last parse point.
	 */

	switch (tok) {
	case (MDOC_El):
		tt = MDOC_Bl;
		break;
	case (MDOC_Ed):
		tt = MDOC_Bd;
		break;
	case (MDOC_Re):
		tt = MDOC_Rs;
		break;
	default:
		abort();
		/* NOTREACHED */
	}

	return(scope_rewind_exp(mdoc, ppos, tok, tt));
}


int
macro_scoped(MACRO_PROT_ARGS)
{
	int		  i, c, lastarg, argc, sz;
	char		 *args[MDOC_LINEARG_MAX];
	struct mdoc_arg	  argv[MDOC_LINEARG_MAX];
	enum mdoc_sec	  sec;
	struct mdoc_node *n;

	assert ( ! (MDOC_CALLABLE & mdoc_macros[tok].flags));

	/* Token pre-processing. */

	switch (tok) {
	case (MDOC_Bl):
		/* FALLTHROUGH */
	case (MDOC_Bd):
		/* `.Pp' ignored when preceding `.Bl' or `.Bd'. */
		assert(mdoc->last);
		if (MDOC_ELEM != mdoc->last->type)
			break;
		if (MDOC_Pp != mdoc->last->data.elem.tok)
			break;
		if ( ! mdoc_warn(mdoc, tok, ppos, WARN_IGN_BEFORE_BLK))
			return(0);
		assert(mdoc->last->prev);
		n = mdoc->last;
		mdoc->last = mdoc->last->prev;
		mdoc->last->next = NULL;
		mdoc_node_free(n);
		break;
	case (MDOC_Sh):
		/* FALLTHROUGH */
	case (MDOC_Ss):
		if ( ! scope_rewind_imp(mdoc, ppos, tok))
			return(0);
		/* `.Pp' ignored when preceding `.Ss' or `.Sh'. */
		if (NULL == mdoc->last)
			break;
		if (MDOC_ELEM != mdoc->last->type)
			break;
		if (MDOC_Pp != mdoc->last->data.elem.tok)
			break;
		if ( ! mdoc_warn(mdoc, tok, ppos, WARN_IGN_BEFORE_BLK))
			return(0);
		assert(mdoc->last->prev);
		n = mdoc->last;
		mdoc_msg(mdoc, ppos, "removing prior `Pp' macro");
		mdoc->last = mdoc->last->prev;
		mdoc->last->next = NULL;
		mdoc_node_free(n);
		break;
	default:
		break;
	}

	/* Argument processing. */

	lastarg = *pos;

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

	/* Parameter processing. */

	for (sz = 0; argc + sz < MDOC_LINEARG_MAX; sz++) {
		lastarg = *pos;
		c = mdoc_args(mdoc, tok, pos, buf, 0, &args[sz]);
		if (ARGS_EOLN == c)
			break;
		if (ARGS_WORD == c)
			continue;
		mdoc_argv_free(argc, argv);
		return(0);
	}

	if (MDOC_LINEARG_MAX == (argc + sz)) {
		mdoc_argv_free(argc, argv);
		return(mdoc_err(mdoc, tok, lastarg, ERR_ARGS_MANY));
	}

	/* Post-processing. */

	if ( ! mdoc_valid_pre(mdoc, tok, ppos, sz, _CC(args), argc, argv)) {
		mdoc_argv_free(argc, argv);
		return(0);
	}

	switch (tok) {
	case (MDOC_Sh):
		sec = mdoc_atosec((size_t)sz, _CC(args));
		if (SEC_CUSTOM != sec)
			mdoc->sec_lastn = sec;
		mdoc->sec_last = sec;
		break;
	default:
		break;
	}

	mdoc_block_alloc(mdoc, ppos, tok, (size_t)argc, argv);
	mdoc->next = MDOC_NEXT_CHILD;

	mdoc_argv_free(argc, argv);

	mdoc_head_alloc(mdoc, ppos, tok);
	mdoc->next = MDOC_NEXT_CHILD;

	for (i = 0; i < sz; i++) {
		mdoc_word_alloc(mdoc, ppos, args[i]);
		mdoc->next = MDOC_NEXT_SIBLING;
	}

	if ( ! scope_rewind_line(mdoc, ppos, tok))
		return(0);

	mdoc_body_alloc(mdoc, ppos, tok);
	mdoc->next = MDOC_NEXT_CHILD;

	return(1);
}


int
macro_scoped_line(MACRO_PROT_ARGS)
{
	int		  lastarg, c, j;
	char		  *p;

	assert(1 == ppos);
	
	mdoc_block_alloc(mdoc, ppos, tok, 0, NULL);
	mdoc->next = MDOC_NEXT_CHILD;

	mdoc_head_alloc(mdoc, ppos, tok);
	mdoc->next = MDOC_NEXT_CHILD;

	if ( ! mdoc_valid_pre(mdoc, tok, ppos, 0, NULL, 0, NULL))
		return(0);

	/* Process line parameters. */

	j = 0;
	lastarg = ppos;

again:
	if (j == MDOC_LINEARG_MAX)
		return(mdoc_err(mdoc, tok, lastarg, ERR_ARGS_MANY));

	lastarg = *pos;
	c = mdoc_args(mdoc, tok, pos, buf, ARGS_DELIM, &p);

	switch (c) {
	case (ARGS_ERROR):
		return(0);
	case (ARGS_WORD):
		break;
	case (ARGS_PUNCT):
		if (ppos > 1)
			return(scope_rewind_imp(mdoc, ppos, tok));
		if ( ! scope_rewind_line(mdoc, ppos, tok))
			return(0);
		if ( ! append_delims(mdoc, tok, pos, buf))
			return(0);
		return(scope_rewind_imp(mdoc, ppos, tok));
	case (ARGS_EOLN):
		return(scope_rewind_imp(mdoc, ppos, tok));
	default:
		abort();
		/* NOTREACHED */
	}

	if (MDOC_MAX != (c = mdoc_find(mdoc, p))) {
		if ( ! mdoc_macro(mdoc, c, lastarg, pos, buf))
			return(0);
		if (ppos > 1)
			return(scope_rewind_imp(mdoc, ppos, tok));
		if ( ! scope_rewind_line(mdoc, ppos, tok))
			return(0);
		if ( ! append_delims(mdoc, tok, pos, buf))
			return(0);
		return(scope_rewind_imp(mdoc, ppos, tok));
	}

	if (mdoc_isdelim(p))
		j = 0;

	mdoc_word_alloc(mdoc, lastarg, p);
	mdoc->next = MDOC_NEXT_SIBLING;
	goto again;
	/* NOTREACHED */
}


int
macro_constant_delimited(MACRO_PROT_ARGS)
{
	int		  lastarg, flushed, c, maxargs;
	char		 *p;

	/* Process line parameters. */

	lastarg = ppos;
	flushed = 0;

	/* Token pre-processing. */

	switch (tok) {
	case (MDOC_No):
		/* FALLTHROUGH */
	case (MDOC_Ns):
		/* FALLTHROUGH */
	case (MDOC_Ux):
		maxargs = 0;
		break;
	default:
		maxargs = 1;
		break;
	}

again:
	lastarg = *pos;

	switch (mdoc_args(mdoc, tok, pos, buf, ARGS_DELIM, &p)) {
	case (ARGS_ERROR):
		return(0);
	case (ARGS_WORD):
		break;
	case (ARGS_PUNCT):
		if ( ! flushed && ! append_text(mdoc, tok, ppos, 0, &p))
			return(0);
		if (ppos > 1)
			return(1);
		return(append_delims(mdoc, tok, pos, buf));
	case (ARGS_EOLN):
		if (flushed)
			return(1);
		return(append_text(mdoc, tok, ppos, 0, &p));
	default:
		abort();
		/* NOTREACHED */
	}

	/* Accepts no arguments: flush out symbol and continue. */

	if ( ! flushed && 0 == maxargs) {
		if ( ! append_text(mdoc, tok, ppos, 0, &p))
			return(0);
		flushed = 1;
	}

	if (MDOC_MAX != (c = mdoc_find(mdoc, p))) {
		if ( ! flushed && ! append_text(mdoc, tok, ppos, 0, &p))
			return(0);
		if ( ! mdoc_macro(mdoc, c, lastarg, pos, buf))
			return(0);
		if (ppos > 1)
			return(1);
		return(append_delims(mdoc, tok, pos, buf));
	}

	/* 
	 * We only accept one argument; subsequent tokens are considered
	 * as literal words (until a macro).
	 */

	if ( ! flushed && ! mdoc_isdelim(p)) {
	       if ( ! append_text(mdoc, tok, ppos, 1, &p))
			return(0);
		flushed = 1;
		goto again;
	} else if ( ! flushed) {
		if ( ! append_text(mdoc, tok, ppos, 0, &p))
			return(0);
		flushed = 1;
	}

	mdoc_word_alloc(mdoc, lastarg, p);
	mdoc->next = MDOC_NEXT_SIBLING;
	goto again;
	/* NOTREACHED */
}


int
macro_constant(MACRO_PROT_ARGS)
{
	int		  c, lastarg, argc, sz, fl;
	char		 *args[MDOC_LINEARG_MAX];
	struct mdoc_arg	  argv[MDOC_LINEARG_MAX];

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

	for (sz = 0; sz + argc < MDOC_LINEARG_MAX; sz++) {
		lastarg = *pos;
		c = mdoc_args(mdoc, tok, pos, buf, fl, &args[sz]);
		if (ARGS_ERROR == c)
			return(0);
		if (ARGS_EOLN == c)
			break;
	}

	if (MDOC_LINEARG_MAX == sz + argc) {
		mdoc_argv_free(argc, argv);
		return(mdoc_err(mdoc, tok, lastarg, ERR_ARGS_MANY));
	}

	c = append_text_argv(mdoc, tok, ppos, sz, args, argc, argv);
	mdoc_argv_free(argc, argv);
	return(c);
}


/* ARGSUSED */
int
macro_obsolete(MACRO_PROT_ARGS)
{

	return(mdoc_warn(mdoc, tok, ppos, WARN_IGN_OBSOLETE));
}
