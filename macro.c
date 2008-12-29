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

#include "private.h"

/* FIXME: maxlineargs should be per LINE, no per TOKEN. */

#define	_CC(p)	((const char **)p)

static	int	  scope_rewind_exp(struct mdoc *, int, int, int);
static	int	  scope_rewind_imp(struct mdoc *, int, int);
static	int	  append_text(struct mdoc *, int, 
			int, int, char *[]);
static	int	  append_const(struct mdoc *, int, int, int, char *[]);
static	int	  append_scoped(struct mdoc *, int, int, int, 
			const char *[], int, const struct mdoc_arg *);
static	int	  append_delims(struct mdoc *, int, int *, char *);


static int
append_delims(struct mdoc *mdoc, int tok, int *pos, char *buf)
{
	int		 c, lastarg;
	char		*p;

	if (0 == buf[*pos])
		return(1);

	mdoc_msg(mdoc, *pos, "`%s' flushing punctuation",
			mdoc_macronames[tok]);

	for (;;) {
		lastarg = *pos;
		c = mdoc_args(mdoc, tok, pos, buf, 0, &p);
		if (ARGS_ERROR == c)
			return(0);
		else if (ARGS_EOLN == c)
			break;
		assert(mdoc_isdelim(p));
		mdoc_word_alloc(mdoc, lastarg, p);
	}

	return(1);
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

	if (n) {
		mdoc->last = n;
		mdoc_msg(mdoc, ppos, "scope: rewound implicit `%s'",
				mdoc_macronames[tok]);
		return(1);
	} 

	mdoc_msg(mdoc, ppos, "scope: new implicit `%s'", 
			mdoc_macronames[tok]);
	return(1);
}


static int
scope_rewind_exp(struct mdoc *mdoc, int ppos, int tok, int dst)
{
	struct mdoc_node *n;

	assert(mdoc->last);

	/* LINTED */
	for (n = mdoc->last->parent; n; n = n->parent) {
		if (MDOC_BLOCK != n->type) 
			continue;
		if (dst == n->data.block.tok)
			break;
		return(mdoc_err(mdoc, tok, ppos, ERR_SCOPE_BREAK));
	}

	if (NULL == (mdoc->last = n))
		return(mdoc_err(mdoc, tok, ppos, ERR_SCOPE_NOCTX));

	mdoc_msg(mdoc, ppos, "scope: rewound explicit `%s' to `%s'",
			mdoc_macronames[tok], mdoc_macronames[dst]);

	return(1);
}


static int
append_scoped(struct mdoc *mdoc, int tok, int pos, 
		int sz, const char *args[], 
		int argc, const struct mdoc_arg *argv)
{
	enum mdoc_sec	  sec;
	struct mdoc_node *node;

	switch (tok) {
	 /* ======= ADD MORE MACRO CHECKS BELOW. ======= */
	case (MDOC_Sh):
		if (0 == sz) 
			return(mdoc_err(mdoc, tok, pos, ERR_ARGS_GE1));

		sec = mdoc_atosec((size_t)sz, _CC(args));
		if (SEC_CUSTOM != sec && sec < mdoc->sec_lastn)
			if ( ! mdoc_warn(mdoc, tok, pos, WARN_SEC_OO))
				return(0);

		if (SEC_BODY == mdoc->sec_last && SEC_NAME != sec)
			return(mdoc_err(mdoc, tok, pos, ERR_SEC_NAME));

		if (SEC_CUSTOM != sec)
			mdoc->sec_lastn = sec;
		mdoc->sec_last = sec;
		break;

	case (MDOC_Ss):
		if (0 == sz) 
			return(mdoc_err(mdoc, tok, pos, ERR_ARGS_GE1));
		break;
	
	case (MDOC_Bd):
		assert(mdoc->last);
		node = mdoc->last->parent; 
		/* LINTED */
		for ( ; node; node = node->parent) {
			if (node->type != MDOC_BLOCK)
				continue;
			if (node->data.block.tok != MDOC_Bd)
				continue;
			return(mdoc_err(mdoc, tok, pos, ERR_SCOPE_NONEST));
		}
		break;

	case (MDOC_Bl):
		break;

	 /* ======= ADD MORE MACRO CHECKS ABOVE. ======= */
	default:
		abort();
		/* NOTREACHED */
	}

	mdoc_block_alloc(mdoc, pos, tok, (size_t)argc, argv);
	mdoc_head_alloc(mdoc, pos, tok, (size_t)sz, _CC(args));
	mdoc_body_alloc(mdoc, pos, tok);
	return(1);
}


static int
append_const(struct mdoc *mdoc, int tok, 
		int pos, int sz, char *args[])
{

	assert(sz >= 0);
	args[sz] = NULL;

	switch (tok) {
	 /* ======= ADD MORE MACRO CHECKS BELOW. ======= */
	case (MDOC_Bx):
		/* FALLTHROUGH */
	case (MDOC_Bsx):
		/* FALLTHROUGH */
	case (MDOC_Os):
		/* FALLTHROUGH */
	case (MDOC_Fx):
		/* FALLTHROUGH */
	case (MDOC_Nx):
		assert(sz <= 1);
		break;

	case (MDOC_Ux):
		assert(0 == sz);
		break;

	 /* ======= ADD MORE MACRO CHECKS ABOVE. ======= */
	default:
		abort();
		/* NOTREACHED */
	}

	mdoc_elem_alloc(mdoc, pos, tok, 0, NULL, (size_t)sz, _CC(args));
	return(1);
}


static int
append_text(struct mdoc *mdoc, int tok, 
		int pos, int sz, char *args[])
{

	assert(sz >= 0);
	args[sz] = NULL;

	switch (tok) {
	 /* ======= ADD MORE MACRO CHECKS BELOW. ======= */
	case (MDOC_Pp):
		if (0 == sz)
			break;
		if ( ! mdoc_warn(mdoc, tok, pos, WARN_ARGS_EQ0))
			return(0);
		break;

	case (MDOC_Ft):
		/* FALLTHROUGH */
	case (MDOC_Li):
		/* FALLTHROUGH */
	case (MDOC_Ms):
		/* FALLTHROUGH */
	case (MDOC_Pa):
		/* FALLTHROUGH */
	case (MDOC_Tn):
		if (0 < sz)
			break;
		if ( ! mdoc_warn(mdoc, tok, pos, WARN_ARGS_GE1))
			return(0);
		break;

	case (MDOC_Ar):
		/* FALLTHROUGH */
	case (MDOC_Cm):
		/* FALLTHROUGH */
	case (MDOC_Fl):
		/* These can have no arguments. */
		break;

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
		if (0 < sz) 
			break;
		return(mdoc_err(mdoc, tok, pos, ERR_ARGS_GE1));
	 /* ======= ADD MORE MACRO CHECKS ABOVE. ======= */
	default:
		abort();
		/* NOTREACHED */
	}

	mdoc_elem_alloc(mdoc, pos, tok, 0, NULL, (size_t)sz, _CC(args));
	return(1);
}


int
macro_text(MACRO_PROT_ARGS)
{
	int		  lastarg, lastpunct, c, j;
	char		 *args[MDOC_LINEARG_MAX], *p;

	if (SEC_PROLOGUE == mdoc->sec_lastn)
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_PROLOGUE));

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

	switch (mdoc_args(mdoc, tok, pos, buf, ARGS_DELIM, &args[j])) {
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

	p = args[j];
	if ( ! lastpunct && ! append_text(mdoc, tok, ppos, j, args))
		return(0);

	mdoc_word_alloc(mdoc, lastarg, p);
	j = 0;
	lastpunct = 1;

	goto again;
	/* NOTREACHED */
}


int
macro_prologue_dtitle(MACRO_PROT_ARGS)
{
	int		  lastarg, j;
	char		 *args[MDOC_LINEARG_MAX];

	if (SEC_PROLOGUE != mdoc->sec_lastn)
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_NPROLOGUE));
	if (0 == mdoc->meta.date)
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_PROLOGUE_OO));
	if (mdoc->meta.title[0])
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_PROLOGUE_REP));

	j = -1;
	lastarg = ppos;

again:
	if (j == MDOC_LINEARG_MAX)
		return(mdoc_err(mdoc, tok, lastarg, ERR_ARGS_MANY));

	lastarg = *pos;

	switch (mdoc_args(mdoc, tok, pos, buf, 0, &args[++j])) {
	case (ARGS_EOLN):
		if (mdoc->meta.title)
			return(1);
		if ( ! mdoc_warn(mdoc, tok, ppos, WARN_ARGS_GE1))
			return(0);
		(void)xstrlcpy(mdoc->meta.title, 
				"UNTITLED", META_TITLE_SZ);
		return(1);
	case (ARGS_ERROR):
		return(0);
	default:
		break;
	}

	if (MDOC_MAX != mdoc_find(mdoc, args[j]) && ! mdoc_warn
			(mdoc, tok, lastarg, WARN_SYNTAX_MACLIKE))
		return(0);

	if (0 == j) {
		if (xstrlcpy(mdoc->meta.title, args[0], META_TITLE_SZ))
			goto again;
		return(mdoc_err(mdoc, tok, lastarg, ERR_SYNTAX_ARGFORM));

	} else if (1 == j) {
		mdoc->meta.msec = mdoc_atomsec(args[1]);
		if (MSEC_DEFAULT != mdoc->meta.msec)
			goto again;
		return(mdoc_err(mdoc, tok, -1, ERR_SYNTAX_ARGFORM));

	} else if (2 == j) {
		mdoc->meta.vol = mdoc_atovol(args[2]);
		if (VOL_DEFAULT != mdoc->meta.vol)
			goto again;
		mdoc->meta.arch = mdoc_atoarch(args[2]);
		if (ARCH_DEFAULT != mdoc->meta.arch)
			goto again;
		return(mdoc_err(mdoc, tok, lastarg, ERR_SYNTAX_ARGFORM));
	}

	return(mdoc_err(mdoc, tok, lastarg, ERR_ARGS_MANY));
}


int
macro_prologue_os(MACRO_PROT_ARGS)
{
	int		  lastarg, j;
	char		 *args[MDOC_LINEARG_MAX];

	if (SEC_PROLOGUE != mdoc->sec_lastn)
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_NPROLOGUE));
	if (0 == mdoc->meta.title[0])
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_PROLOGUE_OO));
	if (mdoc->meta.os[0])
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_PROLOGUE_REP));

	j = -1;
	lastarg = ppos;

again:
	if (j == MDOC_LINEARG_MAX)
		return(mdoc_err(mdoc, tok, lastarg, ERR_ARGS_MANY));

	lastarg = *pos;

	switch (mdoc_args(mdoc, tok, pos, buf, 
				ARGS_QUOTED, &args[++j])) {
	case (ARGS_EOLN):
		mdoc->sec_lastn = mdoc->sec_last = SEC_BODY;
		return(1);
	case (ARGS_ERROR):
		return(0);
	default:
		break;
	}
	
	if ( ! xstrlcat(mdoc->meta.os, args[j], sizeof(mdoc->meta.os)))
		return(mdoc_err(mdoc, tok, lastarg, ERR_SYNTAX_ARGFORM));
	if ( ! xstrlcat(mdoc->meta.os, " ", sizeof(mdoc->meta.os)))
		return(mdoc_err(mdoc, tok, lastarg, ERR_SYNTAX_ARGFORM));

	goto again;
	/* NOTREACHED */
}


int
macro_prologue_ddate(MACRO_PROT_ARGS)
{
	int		  lastarg, j;
	char		 *args[MDOC_LINEARG_MAX], date[64];

	if (SEC_PROLOGUE != mdoc->sec_lastn)
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_NPROLOGUE));
	if (mdoc->meta.title[0])
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_PROLOGUE_OO));
	if (mdoc->meta.date)
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_PROLOGUE_REP));

	j = -1;
	date[0] = 0;
	lastarg = ppos;

again:
	if (j == MDOC_LINEARG_MAX)
		return(mdoc_err(mdoc, tok, lastarg, ERR_ARGS_MANY));

	lastarg = *pos;
	switch (mdoc_args(mdoc, tok, pos, buf, 0, &args[++j])) {
	case (ARGS_EOLN):
		if (mdoc->meta.date)
			return(1);
		mdoc->meta.date = mdoc_atotime(date);
		if (mdoc->meta.date)
			return(1);
		return(mdoc_err(mdoc, tok, ppos, ERR_SYNTAX_ARGFORM));
	case (ARGS_ERROR):
		return(0);
	default:
		break;
	}
	
	if (MDOC_MAX != mdoc_find(mdoc, args[j]) && ! mdoc_warn
			(mdoc, tok, lastarg, WARN_SYNTAX_MACLIKE))
		return(0);
	
	if (0 == j) {
		if (xstrcmp("$Mdocdate$", args[j])) {
			mdoc->meta.date = time(NULL);
			goto again;
		} else if (xstrcmp("$Mdocdate:", args[j])) 
			goto again;
	} else if (4 == j)
		if ( ! xstrcmp("$", args[j]))
			goto again;

	if ( ! xstrlcat(date, args[j], sizeof(date)))
		return(mdoc_err(mdoc, tok, lastarg, ERR_SYNTAX_ARGFORM));
	if ( ! xstrlcat(date, " ", sizeof(date)))
		return(mdoc_err(mdoc, tok, lastarg, ERR_SYNTAX_ARGFORM));

	goto again;
	/* NOTREACHED */
}


int
macro_scoped_explicit(MACRO_PROT_ARGS)
{
	int		  c, lastarg, j;
	struct mdoc_arg	  argv[MDOC_LINEARG_MAX];
	struct mdoc_node *n;

	if (SEC_PROLOGUE == mdoc->sec_lastn)
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_PROLOGUE));

	/*
	 * First close out the explicit scope.  The `end' tags (such as
	 * `.El' to `.Bl' don't cause anything to happen: we merely
	 * readjust our last parse point.
	 */

	switch (tok) {
	case (MDOC_El):
		return(scope_rewind_exp(mdoc, ppos, tok, MDOC_Bl));
	case (MDOC_Ed):
		return(scope_rewind_exp(mdoc, ppos, tok, MDOC_Bd));
	default:
		break;
	}

	assert(MDOC_EXPLICIT & mdoc_macros[tok].flags);

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
	default:
		break;
	}

	lastarg = *pos;

	for (j = 0; j < MDOC_LINEARG_MAX; j++) {
		lastarg = *pos;
		c = mdoc_argv(mdoc, tok, &argv[j], pos, buf);
		if (0 == c)
			break;
		else if (1 == c)
			continue;

		mdoc_argv_free(j, argv);
		return(0);
	}

	if (MDOC_LINEARG_MAX == j) {
		mdoc_argv_free(j, argv);
		return(mdoc_err(mdoc, tok, lastarg, ERR_ARGS_MANY));
	}

	c = append_scoped(mdoc, tok, ppos, 0, NULL, j, argv);
	mdoc_argv_free(j, argv);
	return(c);
}


/*
 * Implicity-scoped macros, like `.Ss', have a scope that terminates
 * with a subsequent call to the same macro.  Implicit macros cannot
 * break the scope of explicitly-scoped macros; however, they can break
 * the scope of other implicit macros (so `.Sh' can break `.Ss').  This
 * is ok with macros like `.It' because they exist only within an
 * explicit context.
 *
 * These macros put line arguments (which it's allowed to have) into the
 * HEAD section and open a BODY scope to be used until the macro scope
 * closes.
 */
int
macro_scoped_implicit(MACRO_PROT_ARGS)
{
	int		  lastarg, j;
	char		 *args[MDOC_LINEARG_MAX];
	struct mdoc_node *n;

	assert( ! (MDOC_EXPLICIT & mdoc_macros[tok].flags));

	if (SEC_PROLOGUE == mdoc->sec_lastn)
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_PROLOGUE));

	/* Token pre-processing. */

	switch (tok) {
	case (MDOC_Ss):
		/* FALLTHROUGH */
	case (MDOC_Sh):
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

	/* Rewind our scope. */

	if ( ! scope_rewind_imp(mdoc, ppos, tok))
		return(0);

	j = 0;
	lastarg = ppos;

	/*
	 * Process until we hit a line.  Note that current implicit
	 * macros don't have any arguments, so we don't need to do any
	 * argument processing.
	 */

again:
	if (j == MDOC_LINEARG_MAX)
		return(mdoc_err(mdoc, tok, lastarg, ERR_ARGS_MANY));

	lastarg = *pos;

	switch (mdoc_args(mdoc, tok, pos, buf, 0, &args[j])) {
	case (ARGS_ERROR):
		return(0);
	case (ARGS_EOLN):
		return(append_scoped(mdoc, tok, ppos, j, _CC(args), 0, NULL));
	default:
		break;
	}

	if (MDOC_MAX != mdoc_find(mdoc, args[j]))
		if ( ! mdoc_warn(mdoc, tok, lastarg, WARN_SYNTAX_MACLIKE))
			return(0);

	j++;
	goto again;
	/* NOTREACHED */
}


/*
 * A line-scoped macro opens a scope for the contents of its line, which
 * are placed under the HEAD node.  Punctuation trailing the line is put
 * as a sibling to the HEAD node, under the BLOCK node.  
 */
int
macro_scoped_line(MACRO_PROT_ARGS)
{
	int		  lastarg, c, j;
	char		  *p;
	struct mdoc_node  *n;

	if (SEC_PROLOGUE == mdoc->sec_lastn)
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_PROLOGUE));

	assert(1 == ppos);
	
	/* Token pre-processing.  */

	switch (tok) {
	case (MDOC_D1):
		/* FALLTHROUGH */
	case (MDOC_Dl):
		/* These can't be nested in a display block. */
		assert(mdoc->last);
		for (n = mdoc->last->parent ; n; n = n->parent)
			if (MDOC_BLOCK != n->type) 
				continue;
			else if (MDOC_Bd == n->data.block.tok)
				break;
		if (NULL == n)
			break;
		return(mdoc_err(mdoc, tok, ppos, ERR_SCOPE_NONEST));
	default:
		break;
	}

	/*
	 * All line-scoped macros have a HEAD and optionally a BODY
	 * section.  We open our scope here; when we exit this function,
	 * we'll rewind our scope appropriately.
	 */

	mdoc_block_alloc(mdoc, ppos, tok, 0, NULL);
	mdoc_head_alloc(mdoc, ppos, tok, 0, NULL);

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
		if ( ! append_delims(mdoc, tok, pos, buf))
			return(0);
		return(scope_rewind_imp(mdoc, ppos, tok));
	}

	if (mdoc_isdelim(p))
		j = 0;

	mdoc_word_alloc(mdoc, lastarg, p);
	goto again;
	/* NOTREACHED */
}


/* 
 * Partial-line scope is identical to line scope (macro_scoped_line())
 * except that trailing punctuation is appended to the BLOCK, instead of
 * contained within the HEAD.
 */
int
macro_scoped_pline(MACRO_PROT_ARGS)
{
	int		  lastarg, c, j;
	char		  *p;

	if (SEC_PROLOGUE == mdoc->sec_lastn)
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_PROLOGUE));

	/* Token pre-processing.  */

	switch (tok) {
	case (MDOC_Ql):
		if ( ! mdoc_warn(mdoc, tok, ppos, WARN_COMPAT_TROFF))
			return(0);
		break;
	default:
		break;
	}

	mdoc_block_alloc(mdoc, ppos, tok, 0, NULL);
	mdoc_head_alloc(mdoc, ppos, tok, 0, NULL);

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
		if ( ! scope_rewind_imp(mdoc, ppos, tok))
			return(0);
		if (ppos > 1)
			return(1);
		return(append_delims(mdoc, tok, pos, buf));
	case (ARGS_EOLN):
		return(scope_rewind_imp(mdoc, ppos, tok));
	default:
		abort();
		/* NOTREACHED */
	}

	if (MDOC_MAX != (c = mdoc_find(mdoc, p))) {
		if ( ! mdoc_macro(mdoc, c, lastarg, pos, buf))
			return(0);
		if ( ! scope_rewind_imp(mdoc, ppos, tok))
			return(0);
		if (ppos > 1)
			return(1);
		return(append_delims(mdoc, tok, pos, buf));
	}

	if (mdoc_isdelim(p))
		j = 0;

	mdoc_word_alloc(mdoc, lastarg, p);
	goto again;
	/* NOTREACHED */
}


/*
 * A delimited-constant macro is similar to a general text macro: the
 * macro is followed by a 0 or 1 arguments (possibly-unspecified) then
 * terminating punctuation, other words, or another callable macro.
 */
int
macro_constant_delimited(MACRO_PROT_ARGS)
{
	int		  lastarg, flushed, c, maxargs;
	char		 *p, *pp;

	if (SEC_PROLOGUE == mdoc->sec_lastn)
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_PROLOGUE));

	/* Process line parameters. */

	lastarg = ppos;
	flushed = 0;

	switch (tok) {
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
		if ( ! flushed && ! append_const(mdoc, tok, ppos, 0, &p))
			return(0);
		if (ppos > 1)
			return(1);
		return(append_delims(mdoc, tok, pos, buf));
	case (ARGS_EOLN):
		if (flushed)
			return(1);
		return(append_const(mdoc, tok, ppos, 0, &p));
	default:
		abort();
		/* NOTREACHED */
	}

	if (0 == maxargs) {
		pp = p;
		if ( ! append_const(mdoc, tok, ppos, 0, &p))
			return(0);
		p = pp;
		flushed = 1;
	}

	if (MDOC_MAX != (c = mdoc_find(mdoc, p))) {
		if ( ! flushed && ! append_const(mdoc, tok, ppos, 0, &p))
			return(0);
		if ( ! mdoc_macro(mdoc, c, lastarg, pos, buf))
			return(0);
		if (ppos > 1)
			return(1);
		return(append_delims(mdoc, tok, pos, buf));
	}

	if ( ! flushed && ! mdoc_isdelim(p)) {
	       if ( ! append_const(mdoc, tok, ppos, 1, &p))
			return(0);
		flushed = 1;
		goto again;
	} else if ( ! flushed) {
		pp = p;
		if ( ! append_const(mdoc, tok, ppos, 0, &p))
			return(0);
		p = pp;
		flushed = 1;
	}

	mdoc_word_alloc(mdoc, lastarg, p);
	goto again;
	/* NOTREACHED */
}
