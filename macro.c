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

#define	_CC(p)	((const char **)p)

static	int	  scope_rewind_exp(struct mdoc *, int, int, int);
static	int	  append_text(struct mdoc *, int, 
			int, int, char *[]);
static	int	  append_scoped(struct mdoc *, int, int, int, 
			const char *[], int, const struct mdoc_arg *);
static	int 	  args_next(struct mdoc *, int, 
			int *, char *, char **);
static	int 	  argv_next(struct mdoc *, int, 
			int *, char *, struct mdoc_arg *);
static	int 	  args_next_quoted(struct mdoc *, int, 
			int *, char *, char **);


static int
args_next_quoted(struct mdoc *mdoc, int tok, 
		int *pos, char *buf, char **v)
{

	if (0 == buf[*pos])
		return(0);

	assert( ! isspace(buf[*pos]));

	if ('\"' != buf[*pos])
		return(args_next(mdoc, tok, pos, buf, v));

	*v = &buf[++(*pos)];

	while (buf[*pos] && '\"' != buf[*pos])
		(*pos)++;

	if (0 == buf[*pos]) {
		(void)mdoc_err(mdoc, tok, *pos, ERR_SYNTAX_UNQUOTE);
		return(-1);
	}

	buf[(*pos)++] = 0;
	if (0 == buf[*pos])
		return(1);

	while (buf[*pos] && isspace(buf[*pos]))
		(*pos)++;

	if (0 == buf[*pos]) 
		if ( ! mdoc_warn(mdoc, tok, *pos, WARN_SYNTAX_WS_EOLN))
			return(-1);

	return(1);
}


static int
scope_rewind_exp(struct mdoc *mdoc, int ppos, int tok, int dst)
{
	struct mdoc_node *n;

	/* LINTED */
	for (n = mdoc->last; n; n = n->parent) {
		if (MDOC_BLOCK != n->type) 
			continue;
		if (dst == n->data.block.tok)
			break;
		return(mdoc_err(mdoc, tok, ppos, ERR_SCOPE_BREAK));
	}

	if (NULL == (mdoc->last = n))
		return(mdoc_err(mdoc, tok, ppos, ERR_SCOPE_NOCTX));

	mdoc_msg(mdoc, ppos, "scope: rewound `%s' to `%s'",
			mdoc_macronames[tok], mdoc_macronames[dst]);

	return(1);
}


static int
argv_next(struct mdoc *mdoc, int tok, 
		int *pos, char *buf, struct mdoc_arg *v)
{
	char		*argv;
	int		 i, val;

	if (0 == buf[*pos])
		return(0);

	assert( ! isspace(buf[*pos]));

	if ('-' != buf[*pos]) {
		(void)mdoc_err(mdoc, tok, *pos, ERR_SYNTAX_ARGS);
		return(-1);
	}

	i = *pos;
	argv = &buf[++(*pos)];

	while (buf[*pos] && ! isspace(buf[*pos]))
		(*pos)++;

	if (buf[*pos])
		buf[(*pos)++] = 0;

	/*
	 * XXX This is a little bit ugly.  The mdoc_argv structure
	 * points to a pointer array, which we allocate on-the-fly in
	 * this function.  If there's any failure, we need to release
	 * this memory, which is done by the caller of this function
	 * with mdoc_argv_free.  Ew.  This should be simpler.
	 */

	if (MDOC_ARG_MAX == (val = mdoc_argv_lookup(tok, argv))) {
		(void)mdoc_err(mdoc, tok, i, ERR_SYNTAX_BADARG);
		return(-1);
	}

	while (buf[*pos] && isspace(buf[*pos]))
		(*pos)++;

	if ( ! mdoc_argv_parse(mdoc, tok, val, v, pos, buf))
		return(-1);

	return(1);
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
append_scoped(struct mdoc *mdoc, int tok, int pos, 
		int sz, const char *args[], 
		int argc, const struct mdoc_arg *argv)
{
	enum mdoc_sec	 sec;

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
append_text(struct mdoc *mdoc, int tok, 
		int pos, int sz, char *args[])
{

	assert(sz >= 0);
	args[sz] = NULL;

	switch (tok) {
	 /* ======= ADD MORE MACRO CHECKS BELOW. ======= */
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

	mdoc_elem_alloc(mdoc, pos, tok, 0, 
			NULL, (size_t)sz, _CC(args));
	return(1);
}


int
macro_text(MACRO_PROT_ARGS)
{
	int		  lastarg, c, lasttok, lastpunct, j;
	char		 *args[MDOC_LINEARG_MAX], *p;

	lasttok = ppos;
	lastpunct = 0;
	j = 0;

	if (SEC_PROLOGUE == mdoc->sec_lastn)
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_PROLOGUE));

again:
	lastarg = *pos;

	if (j == MDOC_LINEARG_MAX)
		return(mdoc_err(mdoc, tok, lastarg, ERR_ARGS_MANY));
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

	if ( ! mdoc_isdelim(args[j])) {
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
macro_prologue_dtitle(MACRO_PROT_ARGS)
{
	int		  c, lastarg, j;
	char		 *args[MDOC_LINEARG_MAX];

	if (SEC_PROLOGUE != mdoc->sec_lastn)
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_NPROLOGUE));
	if (0 == mdoc->meta.date)
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_PROLOGUE_OO));
	if (mdoc->meta.title[0])
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_PROLOGUE_REP));

	j = -1;

again:
	lastarg = *pos;

	if (j == MDOC_LINEARG_MAX)
		return(mdoc_err(mdoc, tok, lastarg, ERR_ARGS_MANY));
	c = args_next(mdoc, tok, pos, buf, &args[++j]);

	if (0 == c) {
		if (mdoc->meta.title)
			return(1);
		if ( ! mdoc_warn(mdoc, tok, ppos, WARN_ARGS_GE1))
			return(0);
		(void)xstrlcpy(mdoc->meta.title, 
				"UNTITLED", META_TITLE_SZ);
		return(1);
	} else if (-1 == c)
		return(0);
	
	if (MDOC_MAX != mdoc_find(mdoc, args[j]) && ! mdoc_warn
			(mdoc, tok, lastarg, WARN_SYNTAX_MACLIKE))
		return(0);

	if (0 == j) {
		if (xstrlcpy(mdoc->meta.title, args[0], META_TITLE_SZ))
			goto again;
		return(mdoc_err(mdoc, tok, lastarg, ERR_SYNTAX_ARGS));

	} else if (1 == j) {
		mdoc->meta.msec = mdoc_atomsec(args[1]);
		if (MSEC_DEFAULT != mdoc->meta.msec)
			goto again;
		return(mdoc_err(mdoc, tok, -1, ERR_SYNTAX_ARGS));

	} else if (2 == j) {
		mdoc->meta.vol = mdoc_atovol(args[2]);
		if (VOL_DEFAULT != mdoc->meta.vol)
			goto again;
		mdoc->meta.arch = mdoc_atoarch(args[2]);
		if (ARCH_DEFAULT != mdoc->meta.arch)
			goto again;
		return(mdoc_err(mdoc, tok, lastarg, ERR_SYNTAX_ARGS));
	}

	return(mdoc_err(mdoc, tok, lastarg, ERR_ARGS_MANY));
}


int
macro_prologue_os(MACRO_PROT_ARGS)
{
	int		  c, lastarg, j;
	char		 *args[MDOC_LINEARG_MAX];

	if (SEC_PROLOGUE != mdoc->sec_lastn)
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_NPROLOGUE));
	if (0 == mdoc->meta.title[0])
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_PROLOGUE_OO));
	if (mdoc->meta.os[0])
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_PROLOGUE_REP));

	j = -1;

again:
	lastarg = *pos;

	if (j == MDOC_LINEARG_MAX)
		return(mdoc_err(mdoc, tok, lastarg, ERR_ARGS_MANY));
	c = args_next_quoted(mdoc, tok, pos, buf, &args[++j]);

	if (0 == c) {
		mdoc->sec_lastn = mdoc->sec_last = SEC_BODY;
		return(1);
	} else if (-1 == c) 
		return(0);
	
	if ( ! xstrlcat(mdoc->meta.os, args[j], sizeof(mdoc->meta.os)))
		return(mdoc_err(mdoc, tok, lastarg, ERR_SYNTAX_ARGS));
	if ( ! xstrlcat(mdoc->meta.os, " ", sizeof(mdoc->meta.os)))
		return(mdoc_err(mdoc, tok, lastarg, ERR_SYNTAX_ARGS));

	goto again;
	/* NOTREACHED */
}


int
macro_prologue_ddate(MACRO_PROT_ARGS)
{
	int		  c, lastarg, j;
	char		 *args[MDOC_LINEARG_MAX], date[64];

	if (SEC_PROLOGUE != mdoc->sec_lastn)
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_NPROLOGUE));
	if (mdoc->meta.title[0])
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_PROLOGUE_OO));
	if (mdoc->meta.date)
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_PROLOGUE_REP));

	j = -1;
	date[0] = 0;

again:
	lastarg = *pos;

	if (j == MDOC_LINEARG_MAX)
		return(mdoc_err(mdoc, tok, lastarg, ERR_ARGS_MANY));
	c = args_next(mdoc, tok, pos, buf, &args[++j]);

	if (0 == c) {
		if (mdoc->meta.date)
			return(1);
		mdoc->meta.date = mdoc_atotime(date);
		if (mdoc->meta.date)
			return(1);
		return(mdoc_err(mdoc, tok, ppos, ERR_SYNTAX_ARGS));
	} else if (-1 == c) 
		return(0);
	
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
		return(mdoc_err(mdoc, tok, lastarg, ERR_SYNTAX_ARGS));
	if ( ! xstrlcat(date, " ", sizeof(date)))
		return(mdoc_err(mdoc, tok, lastarg, ERR_SYNTAX_ARGS));

	goto again;
	/* NOTREACHED */
}


int
macro_scoped_explicit(MACRO_PROT_ARGS)
{
	int		  c, lastarg, j;
	struct mdoc_arg	  argv[MDOC_LINEARG_MAX];

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
	default:
		break;
	}

	assert(MDOC_EXPLICIT & mdoc_macros[tok].flags);

	lastarg = *pos;

	for (j = 0; j < MDOC_LINEARG_MAX; j++) {
		lastarg = *pos;
		c = argv_next(mdoc, tok, pos, buf, &argv[j]);
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


int
macro_scoped_implicit(MACRO_PROT_ARGS)
{
	int		  t, c, lastarg, j;
	char		 *args[MDOC_LINEARG_MAX];
	struct mdoc_node *n;

	assert( ! (MDOC_EXPLICIT & mdoc_macros[tok].flags));

	if (SEC_PROLOGUE == mdoc->sec_lastn)
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_PROLOGUE));

	/* FIXME: put into scope_rewind_imp(). */

	/* LINTED */
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

	if (j == MDOC_LINEARG_MAX)
		return(mdoc_err(mdoc, tok, lastarg, ERR_ARGS_MANY));
	c = args_next(mdoc, tok, pos, buf, &args[j]);
	
	if (-1 == c) 
		return(0);
	if (0 == c)
		return(append_scoped(mdoc, tok, ppos, 
					j, _CC(args), 0, NULL));

	/* Command found. */

	if (MDOC_MAX != (c = mdoc_find(mdoc, args[j])))
		if ( ! mdoc_warn(mdoc, tok, lastarg, WARN_SYNTAX_MACLIKE))
			return(0);

	/* Word found. */

	j++;
	goto again;

	/* NOTREACHED */
}

