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

static	int	  rewind_alt(int);
static	int	  rewind_dohalt(int, enum mdoc_type, 
			const struct mdoc_node *);
#define	REWIND_REWIND	(1 << 0)
#define	REWIND_NOHALT	(1 << 1)
#define	REWIND_HALT	(1 << 2)
static	int	  rewind_dobreak(int, enum mdoc_type, 
			const struct mdoc_node *);


static	int	  rewind_elem(struct mdoc *, int);
static	int	  rewind_impblock(struct mdoc *, int, int, int);
static	int	  rewind_expblock(struct mdoc *, int, int, int);
static	int	  rewind_subblock(enum mdoc_type, struct mdoc *, int);
static	int	  rewind_last(int, int, 
			struct mdoc *, struct mdoc_node *);
static	int	  append_delims(struct mdoc *, 
			int, int, int *, char *);
static	int	  lookup(struct mdoc *, int, int, int, const char *);


static int
lookup(struct mdoc *mdoc, int line, int pos, int from, const char *p)
{
	int		 res;

	res = mdoc_find(mdoc, p);
	if (MDOC_PARSED & mdoc_macros[from].flags)
		return(res);
	if (MDOC_MAX == res)
		return(res);

	if ( ! mdoc_pwarn(mdoc, line, pos, WARN_SYNTAX, "macro-like parameter"))
		return(-1);
	return(MDOC_MAX);
}


static int
rewind_last(int tok, int type, struct mdoc *mdoc, struct mdoc_node *to)
{

	assert(to);
	mdoc->next = MDOC_NEXT_SIBLING;
	if (mdoc->last == to) {
		if ( ! mdoc_valid_post(mdoc))
			return(0);
		if ( ! mdoc_action_post(mdoc))
			return(0);
		mdoc_msg(mdoc, "rewound %s %s to %s %s", 
				mdoc_type2a(type),
				mdoc_macronames[tok],
				mdoc_type2a(mdoc->last->type),
				mdoc_macronames[mdoc->last->tok]);
		return(1);
	}

	do {
		mdoc->last = mdoc->last->parent;
		assert(mdoc->last);
		if ( ! mdoc_valid_post(mdoc))
			return(0);
		if ( ! mdoc_action_post(mdoc))
			return(0);
		mdoc_msg(mdoc, "rewound %s %s to %s %s",
				mdoc_type2a(type),
				mdoc_macronames[tok],
				mdoc_type2a(mdoc->last->type),
				mdoc_macronames[mdoc->last->tok]);
	} while (mdoc->last != to);

	return(1);
}


static int
rewind_alt(int tok)
{
	switch (tok) {
	case (MDOC_Ac):
		return(MDOC_Ao);
	case (MDOC_Bc):
		return(MDOC_Bo);
	case (MDOC_Dc):
		return(MDOC_Do);
	case (MDOC_Ec):
		return(MDOC_Eo);
	case (MDOC_Ed):
		return(MDOC_Bd);
	case (MDOC_Ef):
		return(MDOC_Bf);
	case (MDOC_Ek):
		return(MDOC_Bk);
	case (MDOC_El):
		return(MDOC_Bl);
	case (MDOC_Fc):
		return(MDOC_Fo);
	case (MDOC_Oc):
		return(MDOC_Oo);
	case (MDOC_Pc):
		return(MDOC_Po);
	case (MDOC_Qc):
		return(MDOC_Qo);
	case (MDOC_Re):
		return(MDOC_Rs);
	case (MDOC_Sc):
		return(MDOC_So);
	case (MDOC_Xc):
		return(MDOC_Xo);
	default:
		break;
	}
	abort();
	/* NOTREACHED */
}


static int
rewind_dohalt(int tok, enum mdoc_type type, const struct mdoc_node *p)
{

	if (MDOC_ROOT == p->type)
		return(REWIND_HALT);
	if (MDOC_TEXT == p->type) 
		return(REWIND_NOHALT);
	if (MDOC_ELEM == p->type) 
		return(REWIND_NOHALT);

	switch (tok) {
	/* One-liner implicit-scope. */
	case (MDOC_Aq):
		/* FALLTHROUGH */
	case (MDOC_Bq):
		/* FALLTHROUGH */
	case (MDOC_D1):
		/* FALLTHROUGH */
	case (MDOC_Dl):
		/* FALLTHROUGH */
	case (MDOC_Dq):
		/* FALLTHROUGH */
	case (MDOC_Op):
		/* FALLTHROUGH */
	case (MDOC_Pq):
		/* FALLTHROUGH */
	case (MDOC_Ql):
		/* FALLTHROUGH */
	case (MDOC_Qq):
		/* FALLTHROUGH */
	case (MDOC_Sq):
		assert(MDOC_BODY != type);
		assert(MDOC_TAIL != type);
		if (type == p->type && tok == p->tok)
			return(REWIND_REWIND);
		break;

	/* Multi-line implicit-scope. */
	case (MDOC_It):
		assert(MDOC_TAIL != type);
		if (type == p->type && tok == p->tok)
			return(REWIND_REWIND);
		if (MDOC_BODY == p->type && MDOC_Bl == p->tok)
			return(REWIND_HALT);
		break;
	case (MDOC_Sh):
		if (type == p->type && tok == p->tok)
			return(REWIND_REWIND);
		break;
	case (MDOC_Ss):
		assert(MDOC_TAIL != type);
		if (type == p->type && tok == p->tok)
			return(REWIND_REWIND);
		if (MDOC_BODY == p->type && MDOC_Sh == p->tok)
			return(REWIND_HALT);
		break;
	
	/* Multi-line explicit scope start. */
	case (MDOC_Ao):
		/* FALLTHROUGH */
	case (MDOC_Bd):
		/* FALLTHROUGH */
	case (MDOC_Bf):
		/* FALLTHROUGH */
	case (MDOC_Bk):
		/* FALLTHROUGH */
	case (MDOC_Bl):
		/* FALLTHROUGH */
	case (MDOC_Bo):
		/* FALLTHROUGH */
	case (MDOC_Do):
		/* FALLTHROUGH */
	case (MDOC_Eo):
		/* FALLTHROUGH */
	case (MDOC_Fo):
		/* FALLTHROUGH */
	case (MDOC_Oo):
		/* FALLTHROUGH */
	case (MDOC_Po):
		/* FALLTHROUGH */
	case (MDOC_Qo):
		/* FALLTHROUGH */
	case (MDOC_Rs):
		/* FALLTHROUGH */
	case (MDOC_So):
		/* FALLTHROUGH */
	case (MDOC_Xo):
		if (type == p->type && tok == p->tok)
			return(REWIND_REWIND);
		break;

	/* Multi-line explicit scope close. */
	case (MDOC_Ac):
		/* FALLTHROUGH */
	case (MDOC_Bc):
		/* FALLTHROUGH */
	case (MDOC_Dc):
		/* FALLTHROUGH */
	case (MDOC_Ec):
		/* FALLTHROUGH */
	case (MDOC_Ed):
		/* FALLTHROUGH */
	case (MDOC_Ek):
		/* FALLTHROUGH */
	case (MDOC_El):
		/* FALLTHROUGH */
	case (MDOC_Fc):
		/* FALLTHROUGH */
	case (MDOC_Ef):
		/* FALLTHROUGH */
	case (MDOC_Oc):
		/* FALLTHROUGH */
	case (MDOC_Pc):
		/* FALLTHROUGH */
	case (MDOC_Qc):
		/* FALLTHROUGH */
	case (MDOC_Re):
		/* FALLTHROUGH */
	case (MDOC_Sc):
		/* FALLTHROUGH */
	case (MDOC_Xc):
		if (type == p->type && rewind_alt(tok) == p->tok)
			return(REWIND_REWIND);
		break;
	default:
		abort();
		/* NOTREACHED */
	}

	return(REWIND_NOHALT);
}


static int
rewind_dobreak(int tok, enum mdoc_type type, const struct mdoc_node *p)
{

	assert(MDOC_ROOT != p->type);
	if (MDOC_ELEM == p->type)
		return(1);
	if (MDOC_TEXT == p->type)
		return(1);

	switch (tok) {
	/* Implicit rules. */
	case (MDOC_It):
		return(MDOC_It == p->tok);
	case (MDOC_Ss):
		return(MDOC_Ss == p->tok);
	case (MDOC_Sh):
		if (MDOC_Ss == p->tok)
			return(1);
		return(MDOC_Sh == p->tok);

	/* Extra scope rules. */
	case (MDOC_El):
		if (MDOC_It == p->tok)
			return(1);
		break;
	default:
		break;
	}

	if (MDOC_EXPLICIT & mdoc_macros[tok].flags) 
		return(p->tok == rewind_alt(tok));
	else if (MDOC_BLOCK == p->type)
		return(1);

	return(tok == p->tok);
}


static int
rewind_elem(struct mdoc *mdoc, int tok)
{
	struct mdoc_node *n;

	n = mdoc->last;
	if (MDOC_ELEM != n->type)
		n = n->parent;
	assert(MDOC_ELEM == n->type);
	assert(tok == n->tok);

	return(rewind_last(tok, MDOC_ELEM, mdoc, n));
}


static int
rewind_subblock(enum mdoc_type type, struct mdoc *mdoc, int tok)
{
	struct mdoc_node *n;
	int		  c;

	c = rewind_dohalt(tok, type, mdoc->last);
	if (REWIND_HALT == c)
		return(1);
	if (REWIND_REWIND == c)
		return(rewind_last(tok, type, mdoc, mdoc->last));

	/* LINTED */
	for (n = mdoc->last->parent; n; n = n->parent) {
		c = rewind_dohalt(tok, type, n);
		if (REWIND_HALT == c)
			return(1);
		if (REWIND_REWIND == c)
			break;
		else if (rewind_dobreak(tok, type, n))
			continue;
		return(mdoc_nerr(mdoc, n, "body scope broken"));
	}

	assert(n);
	return(rewind_last(tok, type, mdoc, n));
}


static int
rewind_expblock(struct mdoc *mdoc, int tok, int line, int ppos)
{
	struct mdoc_node *n;
	int		  c;

	c = rewind_dohalt(tok, MDOC_BLOCK, mdoc->last);
	if (REWIND_HALT == c)
		return(mdoc_perr(mdoc, line, ppos, "closing macro has no context"));
	if (REWIND_REWIND == c)
		return(rewind_last(tok, MDOC_BLOCK, mdoc, mdoc->last));

	/* LINTED */
	for (n = mdoc->last->parent; n; n = n->parent) {
		c = rewind_dohalt(tok, MDOC_BLOCK, n);
		if (REWIND_HALT == c)
			return(mdoc_perr(mdoc, line, ppos, "closing macro has no context"));
		if (REWIND_REWIND == c)
			break;
		else if (rewind_dobreak(tok, MDOC_BLOCK, n))
			continue;
		return(mdoc_nerr(mdoc, n, "block scope broken"));
	}

	assert(n);
	return(rewind_last(tok, MDOC_BLOCK, mdoc, n));
}


static int
rewind_impblock(struct mdoc *mdoc, int tok, int line, int ppos)
{
	struct mdoc_node *n;
	int		  c;

	c = rewind_dohalt(tok, MDOC_BLOCK, mdoc->last);
	if (REWIND_HALT == c)
		return(1);
	if (REWIND_REWIND == c)
		return(rewind_last(tok, MDOC_BLOCK, mdoc, mdoc->last));

	/* LINTED */
	for (n = mdoc->last->parent; n; n = n->parent) {
		c = rewind_dohalt(tok, MDOC_BLOCK, n);
		if (REWIND_HALT == c)
			return(1);
		else if (REWIND_REWIND == c)
			break;
		else if (rewind_dobreak(tok, MDOC_BLOCK, n))
			continue;
		return(mdoc_nerr(mdoc, n, "block scope broken"));
	}

	assert(n);
	return(rewind_last(tok, MDOC_BLOCK, mdoc, n));
}


static int
append_delims(struct mdoc *mdoc, int tok, 
		int line, int *pos, char *buf)
{
	int		 c, lastarg;
	char		*p;

	if (0 == buf[*pos])
		return(1);

	for (;;) {
		lastarg = *pos;
		c = mdoc_args(mdoc, line, pos, buf, 0, &p);
		if (ARGS_ERROR == c)
			return(0);
		else if (ARGS_EOLN == c)
			break;
		assert(mdoc_isdelim(p));
		if ( ! mdoc_word_alloc(mdoc, line, lastarg, p))
			return(0);
		mdoc->next = MDOC_NEXT_SIBLING;
	}

	return(1);
}


/* ARGSUSED */
int
macro_scoped_close(MACRO_PROT_ARGS)
{
	int	 	 tt, j, c, lastarg, maxargs, flushed;
	char		*p;

	switch (tok) {
	case (MDOC_Ec):
		maxargs = 1;
		break;
	default:
		maxargs = 0;
		break;
	}

	tt = rewind_alt(tok);

	mdoc_msg(mdoc, "parse-quiet: %s closing %s",
			mdoc_macronames[tok], mdoc_macronames[tt]);

	if ( ! (MDOC_CALLABLE & mdoc_macros[tok].flags)) {
		if (0 == buf[*pos]) {
			if ( ! rewind_subblock(MDOC_BODY, mdoc, tok))
				return(0);
			return(rewind_expblock(mdoc, tok, line, ppos));
		}
		return(mdoc_perr(mdoc, line, ppos, "macro expects no parameters"));
	}

	if ( ! rewind_subblock(MDOC_BODY, mdoc, tok))
		return(0);

	lastarg = ppos;
	flushed = 0;

	if (maxargs > 0) {
		if ( ! mdoc_tail_alloc(mdoc, line, ppos, tt))
			return(0);
		mdoc->next = MDOC_NEXT_CHILD;
	}

	for (j = 0; j < MDOC_LINEARG_MAX; j++) {
		lastarg = *pos;

		if (j == maxargs && ! flushed) {
			if ( ! rewind_expblock(mdoc, tok, line, ppos))
				return(0);
			flushed = 1;
		}

		c = mdoc_args(mdoc, line, pos, buf, ARGS_DELIM, &p);
		if (ARGS_ERROR == c)
			return(0);
		if (ARGS_PUNCT == c)
			break;
		if (ARGS_EOLN == c)
			break;

		if (-1 == (c = lookup(mdoc, line, lastarg, tok, p)))
			return(0);
		else if (MDOC_MAX != c) {
			if ( ! flushed) {
				if ( ! rewind_expblock(mdoc, tok, line, ppos))
					return(0);
				flushed = 1;
			}
			if ( ! mdoc_macro(mdoc, c, line, lastarg, pos, buf))
				return(0);
			break;
		} 

		if ( ! mdoc_word_alloc(mdoc, line, lastarg, p))
			return(0);
		mdoc->next = MDOC_NEXT_SIBLING;
	}

	if (MDOC_LINEARG_MAX == j)
		return(mdoc_perr(mdoc, line, ppos, "too many arguments"));

	if ( ! flushed && ! rewind_expblock(mdoc, tok, line, ppos))
		return(0);

	if (ppos > 1)
		return(1);
	return(append_delims(mdoc, tok, line, pos, buf));
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
	int		  la, lastpunct, c, sz, fl, argc;
	struct mdoc_arg	  argv[MDOC_LINEARG_MAX];
	char		 *p;

	la = ppos;
	lastpunct = 0;

	for (argc = 0; argc < MDOC_LINEARG_MAX; argc++) {
		la = *pos;

		c = mdoc_argv(mdoc, line, tok, &argv[argc], pos, buf);
		if (ARGV_EOLN == c)
			break;
		if (ARGV_WORD == c) {
			*pos = la;
			break;
		} else if (ARGV_ARG == c)
			continue;

		mdoc_argv_free(argc, argv);
		return(0);
	}

	if (MDOC_LINEARG_MAX == argc) {
		mdoc_argv_free(argc, argv);
		return(mdoc_perr(mdoc, line, ppos, "too many arguments"));
	}

	c = mdoc_elem_alloc(mdoc, line, la, tok, argc, argv);

	if (0 == c) {
		mdoc_argv_free(argc, argv);
		return(0);
	}

	mdoc->next = MDOC_NEXT_CHILD;

	fl = ARGS_DELIM;
	if (MDOC_QUOTABLE & mdoc_macros[tok].flags)
		fl |= ARGS_QUOTED;

	for (lastpunct = sz = 0; sz + argc < MDOC_LINEARG_MAX; sz++) {
		la = *pos;

		c = mdoc_args(mdoc, line, pos, buf, fl, &p);
		if (ARGS_ERROR == c) {
			mdoc_argv_free(argc, argv);
			return(0);
		}

		if (ARGS_EOLN == c)
			break;
		if (ARGS_PUNCT == c)
			break;

		if (-1 == (c = lookup(mdoc, line, la, tok, p)))
			return(0);
		else if (MDOC_MAX != c) {
			if (0 == lastpunct && ! rewind_elem(mdoc, tok)) {
				mdoc_argv_free(argc, argv);
				return(0);
			}
			mdoc_argv_free(argc, argv);

			c = mdoc_macro(mdoc, c, line, la, pos, buf);
			if (0 == c)
				return(0);
			if (ppos > 1)
				return(1);
			return(append_delims(mdoc, tok, line, pos, buf));
		}

		if (mdoc_isdelim(p)) {
			if (0 == lastpunct && ! rewind_elem(mdoc, tok)) {
				mdoc_argv_free(argc, argv);
				return(0);
			}
			lastpunct = 1;
		} else if (lastpunct) {
			c = mdoc_elem_alloc(mdoc, line, 
					la, tok, argc, argv);
			if (0 == c) {
				mdoc_argv_free(argc, argv);
				return(0);
			}
			mdoc->next = MDOC_NEXT_CHILD;
			lastpunct = 0;
		}

		if ( ! mdoc_word_alloc(mdoc, line, la, p))
			return(0);
		mdoc->next = MDOC_NEXT_SIBLING;
	}

	mdoc_argv_free(argc, argv);

	if (sz == MDOC_LINEARG_MAX)
		return(mdoc_perr(mdoc, line, ppos, "too many arguments"));

	if (0 == lastpunct && ! rewind_elem(mdoc, tok))
		return(0);
	if (ppos > 1)
		return(1);
	return(append_delims(mdoc, tok, line, pos, buf));
}


/*
 * Implicit- or explicit-end multi-line scoped macro.
 */
int
macro_scoped(MACRO_PROT_ARGS)
{
	int		  c, lastarg, argc, j, fl;
	struct mdoc_arg	  argv[MDOC_LINEARG_MAX];
	char		 *p;

	assert ( ! (MDOC_CALLABLE & mdoc_macros[tok].flags));

	if ( ! (MDOC_EXPLICIT & mdoc_macros[tok].flags)) {
		if ( ! rewind_subblock(MDOC_BODY, mdoc, tok))
			return(0);
		if ( ! rewind_impblock(mdoc, tok, line, ppos))
			return(0);
	}

	for (argc = 0; argc < MDOC_LINEARG_MAX; argc++) {
		lastarg = *pos;
		c = mdoc_argv(mdoc, line, tok, &argv[argc], pos, buf);
		if (ARGV_EOLN == c)
			break;
		if (ARGV_WORD == c) {
			*pos = lastarg;
			break;
		} else if (ARGV_ARG == c)
			continue;
		mdoc_argv_free(argc, argv);
		return(0);
	}

	if (MDOC_LINEARG_MAX == argc) {
		mdoc_argv_free(argc, argv);
		return(mdoc_perr(mdoc, line, ppos, "too many arguments"));
	}

	c = mdoc_block_alloc(mdoc, line, ppos, 
			tok, (size_t)argc, argv);
	mdoc_argv_free(argc, argv);

	if (0 == c)
		return(0);

	mdoc->next = MDOC_NEXT_CHILD;

	if (0 == buf[*pos]) {
		if ( ! mdoc_head_alloc(mdoc, line, ppos, tok))
			return(0);
		if ( ! rewind_subblock(MDOC_HEAD, mdoc, tok))
			return(0);
		if ( ! mdoc_body_alloc(mdoc, line, ppos, tok))
			return(0);
		mdoc->next = MDOC_NEXT_CHILD;
		return(1);
	}

	if ( ! mdoc_head_alloc(mdoc, line, ppos, tok))
		return(0);
	mdoc->next = MDOC_NEXT_CHILD;

	fl = ARGS_DELIM;
	if (MDOC_TABSEP & mdoc_macros[tok].flags)
		fl |= ARGS_TABSEP;

	for (j = 0; j < MDOC_LINEARG_MAX; j++) {
		lastarg = *pos;
		c = mdoc_args(mdoc, line, pos, buf, fl, &p);
	
		if (ARGS_ERROR == c)
			return(0);
		if (ARGS_PUNCT == c)
			break;
		if (ARGS_EOLN == c)
			break;
	
		if (-1 == (c = lookup(mdoc, line, lastarg, tok, p)))
			return(0);
		else if (MDOC_MAX == c) {
			if ( ! mdoc_word_alloc(mdoc, line, lastarg, p))
				return(0);
			mdoc->next = MDOC_NEXT_SIBLING;
			continue;
		} 

		if ( ! mdoc_macro(mdoc, c, line, lastarg, pos, buf))
			return(0);
		break;
	}

	if (j == MDOC_LINEARG_MAX)
		return(mdoc_perr(mdoc, line, ppos, "too many arguments"));

	if ( ! rewind_subblock(MDOC_HEAD, mdoc, tok))
		return(0);
	if (1 == ppos && ! append_delims(mdoc, tok, line, pos, buf))
		return(0);

	if ( ! mdoc_body_alloc(mdoc, line, ppos, tok))
		return(0);
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

	if ( ! mdoc_block_alloc(mdoc, line, ppos, tok, 0, NULL))
		return(0);
	mdoc->next = MDOC_NEXT_CHILD;

	if ( ! mdoc_head_alloc(mdoc, line, ppos, tok))
		return(0);
	mdoc->next = MDOC_NEXT_CHILD;

	/* XXX - no known argument macros. */

	for (lastarg = ppos, j = 0; j < MDOC_LINEARG_MAX; j++) {
		lastarg = *pos;
		c = mdoc_args(mdoc, line, pos, buf, ARGS_DELIM, &p);

		if (ARGS_ERROR == c)
			return(0);
		if (ARGS_PUNCT == c)
			break;
		if (ARGS_EOLN == c)
			break;

		if (-1 == (c = lookup(mdoc, line, lastarg, tok, p)))
			return(0);
		else if (MDOC_MAX == c) {
			if ( ! mdoc_word_alloc(mdoc, line, lastarg, p))
				return(0);
			mdoc->next = MDOC_NEXT_SIBLING;
			continue;
		} 

		if ( ! mdoc_macro(mdoc, c, line, lastarg, pos, buf))
			return(0);
		break;
	}

	if (j == MDOC_LINEARG_MAX)
		return(mdoc_perr(mdoc, line, ppos, "too many arguments"));

	if (1 == ppos) {
		if ( ! rewind_subblock(MDOC_HEAD, mdoc, tok))
			return(0);
		if ( ! append_delims(mdoc, tok, line, pos, buf))
			return(0);
	} else if ( ! rewind_subblock(MDOC_HEAD, mdoc, tok))
		return(0);
	return(rewind_impblock(mdoc, tok, line, ppos));
}


/*
 * Constant-scope macros accept a fixed number of arguments and behave
 * like constant macros except that they're scoped across lines.
 */
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

	if ( ! mdoc_block_alloc(mdoc, line, ppos, tok, 0, NULL))
		return(0); 
	mdoc->next = MDOC_NEXT_CHILD;

	if (0 == maxargs) {
		if ( ! mdoc_head_alloc(mdoc, line, ppos, tok))
			return(0);
		if ( ! rewind_subblock(MDOC_HEAD, mdoc, tok))
			return(0);
		if ( ! mdoc_body_alloc(mdoc, line, ppos, tok))
			return(0);
		flushed = 1;
	} else if ( ! mdoc_head_alloc(mdoc, line, ppos, tok))
		return(0);

	mdoc->next = MDOC_NEXT_CHILD;

	for (j = 0; j < MDOC_LINEARG_MAX; j++) {
		lastarg = *pos;

		if (j == maxargs && ! flushed) {
			if ( ! rewind_subblock(MDOC_HEAD, mdoc, tok))
				return(0);
			flushed = 1;
			if ( ! mdoc_body_alloc(mdoc, line, ppos, tok))
				return(0);
			mdoc->next = MDOC_NEXT_CHILD;
		}

		c = mdoc_args(mdoc, line, pos, buf, ARGS_DELIM, &p);
		if (ARGS_ERROR == c)
			return(0);
		if (ARGS_PUNCT == c)
			break;
		if (ARGS_EOLN == c)
			break;

		if (-1 == (c = lookup(mdoc, line, lastarg, tok, p)))
			return(0);
		else if (MDOC_MAX != c) {
			if ( ! flushed) {
				if ( ! rewind_subblock(MDOC_HEAD, mdoc, tok))
					return(0);
				flushed = 1;
				if ( ! mdoc_body_alloc(mdoc, line, ppos, tok))
					return(0);
				mdoc->next = MDOC_NEXT_CHILD;
			}
			if ( ! mdoc_macro(mdoc, c, line, lastarg, pos, buf))
				return(0);
			break;
		}

		if ( ! flushed && mdoc_isdelim(p)) {
			if ( ! rewind_subblock(MDOC_HEAD, mdoc, tok))
				return(0);
			flushed = 1;
			if ( ! mdoc_body_alloc(mdoc, line, ppos, tok))
				return(0);
			mdoc->next = MDOC_NEXT_CHILD;
		}
	
		if ( ! mdoc_word_alloc(mdoc, line, lastarg, p))
			return(0);
		mdoc->next = MDOC_NEXT_SIBLING;
	}

	if (MDOC_LINEARG_MAX == j)
		return(mdoc_perr(mdoc, line, ppos, "too many arguments"));

	if ( ! flushed) {
		if ( ! rewind_subblock(MDOC_HEAD, mdoc, tok))
			return(0);
		if ( ! mdoc_body_alloc(mdoc, line, ppos, tok))
			return(0);
		mdoc->next = MDOC_NEXT_CHILD;
	}

	if (ppos > 1)
		return(1);
	return(append_delims(mdoc, tok, line, pos, buf));
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
		c = mdoc_argv(mdoc, line, tok, &argv[argc], pos, buf);
		if (ARGV_EOLN == c)
			break;
		if (ARGV_WORD == c) {
			*pos = lastarg;
			break;
		} else if (ARGV_ARG == c)
			continue;
		mdoc_argv_free(argc, argv);
		return(0);
	}

	c = mdoc_elem_alloc(mdoc, line, lastarg, tok, argc, argv);
	mdoc_argv_free(argc, argv);

	if (0 == c)
		return(0);

	mdoc->next = MDOC_NEXT_CHILD;

	for (j = 0; j < MDOC_LINEARG_MAX; j++) {
		lastarg = *pos;

		if (j == maxargs && ! flushed) {
			if ( ! rewind_elem(mdoc, tok))
				return(0);
			flushed = 1;
		}

		c = mdoc_args(mdoc, line, pos, buf, ARGS_DELIM, &p);
		if (ARGS_ERROR == c)
			return(0);
		if (ARGS_PUNCT == c)
			break;
		if (ARGS_EOLN == c)
			break;

		if (-1 == (c = lookup(mdoc, line, lastarg, tok, p)))
			return(0);
		else if (MDOC_MAX != c) {
			if ( ! flushed && ! rewind_elem(mdoc, tok))
				return(0);
			flushed = 1;
			if ( ! mdoc_macro(mdoc, c, line, lastarg, pos, buf))
				return(0);
			break;
		}

		if ( ! flushed && mdoc_isdelim(p)) {
			if ( ! rewind_elem(mdoc, tok))
				return(0);
			flushed = 1;
		}
	
		if ( ! mdoc_word_alloc(mdoc, line, lastarg, p))
			return(0);
		mdoc->next = MDOC_NEXT_SIBLING;
	}

	if (MDOC_LINEARG_MAX == j)
		return(mdoc_perr(mdoc, line, ppos, "too many arguments"));

	if ( ! flushed && rewind_elem(mdoc, tok))
		return(0);

	if (ppos > 1)
		return(1);
	return(append_delims(mdoc, tok, line, pos, buf));
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

	/* FIXME: parsing macros! */

	fl = 0;
	if (MDOC_QUOTABLE & mdoc_macros[tok].flags)
		fl = ARGS_QUOTED;

	for (argc = 0; argc < MDOC_LINEARG_MAX; argc++) {
		lastarg = *pos;
		c = mdoc_argv(mdoc, line, tok, &argv[argc], pos, buf);
		if (ARGV_EOLN == c) 
			break;
		if (ARGV_WORD == c) {
			*pos = lastarg;
			break;
		} else if (ARGV_ARG == c)
			continue;

		mdoc_argv_free(argc, argv);
		return(0);
	}

	c = mdoc_elem_alloc(mdoc, line, ppos, tok, argc, argv);
	mdoc_argv_free(argc, argv);

	if (0 == c)
		return(0);

	mdoc->next = MDOC_NEXT_CHILD;

	if (MDOC_LINEARG_MAX == argc)
		return(mdoc_perr(mdoc, line, ppos, "too many arguments"));

	for (sz = 0; sz + argc < MDOC_LINEARG_MAX; sz++) {
		lastarg = *pos;
		c = mdoc_args(mdoc, line, pos, buf, fl, &p);
		if (ARGS_ERROR == c)
			return(0);
		if (ARGS_EOLN == c)
			break;

		if ( ! mdoc_word_alloc(mdoc, line, lastarg, p))
			return(0);
		mdoc->next = MDOC_NEXT_SIBLING;
	}

	if (MDOC_LINEARG_MAX == sz + argc)
		return(mdoc_perr(mdoc, line, ppos, "too many arguments"));

	return(rewind_elem(mdoc, tok));
}


/* ARGSUSED */
int
macro_obsolete(MACRO_PROT_ARGS)
{

	return(mdoc_pwarn(mdoc, line, ppos, WARN_SYNTAX, "macro is obsolete"));
}


int
macro_end(struct mdoc *mdoc)
{

	assert(mdoc->first);
	assert(mdoc->last);
	return(rewind_last(mdoc->last->tok, mdoc->last->type,
				mdoc, mdoc->first));
}
