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
#include <stdlib.h>

#include "private.h"

/* FIXME: `.St' can only have one argument set. */

typedef int	(*v_args_sz)(struct mdoc *, int, int, int);
typedef int	(*v_args)(struct mdoc *, int, int, int, 
			const char *[], int, const struct mdoc_arg *);
typedef int	(*v_post)(struct mdoc *, int, int);

static	int	  need_head_child(struct mdoc *, int, int);

static	int	  assert_eq0(struct mdoc *, int, int, int);
static	int	  assert_le1(struct mdoc *, int, int, int);
static	int	  need_eq0(struct mdoc *, int, int, int);
static	int	  need_eq1(struct mdoc *, int, int, int);
static	int	  need_ge1(struct mdoc *, int, int, int);
static	int	  need_le2(struct mdoc *, int, int, int);
static	int	  want_eq0(struct mdoc *, int, int, int);
static	int	  want_ge1(struct mdoc *, int, int, int);

static	int	  args_ref(struct mdoc *, int, int, int, 
			const char *[], int, const struct mdoc_arg *);
static	int	  args_bool(struct mdoc *, int, int, int, 
			const char *[], int, const struct mdoc_arg *);
static	int	  args_Sh(struct mdoc *, int, int, int, 
			const char *[], int, const struct mdoc_arg *);
static	int	  args_blocknest(struct mdoc *, int, int, int, 
			const char *[], int, const struct mdoc_arg *);
static	int	  args_At(struct mdoc *, int, int, int, 
			const char *[], int, const struct mdoc_arg *);
static	int	  args_Xr(struct mdoc *, int, int, int, 
			const char *[], int, const struct mdoc_arg *);

struct	valids {
	v_args_sz sz;
	v_args	  args;
	v_post	  post;
};


const	struct valids mdoc_valids[MDOC_MAX] = {
	{ NULL, NULL, NULL }, /* \" */
	{ NULL, NULL, NULL }, /* Dd */ /* TODO */
	{ NULL, NULL, NULL }, /* Dt */ /* TODO */
	{ NULL, NULL, NULL }, /* Os */ /* TODO */
	{ NULL, args_Sh, NULL }, /* Sh */
	{ NULL, NULL, NULL }, /* Ss */ 
	{ want_eq0, NULL, NULL }, /* Pp */ 
	{ NULL, args_blocknest, need_head_child }, /* D1 */
	{ NULL, args_blocknest, need_head_child }, /* Dl */
	{ NULL, args_blocknest, NULL }, /* Bd */
	{ NULL, NULL, NULL }, /* Ed */
	{ NULL, NULL, NULL }, /* Bl */
	{ NULL, NULL, NULL }, /* El */
	{ NULL, NULL, NULL }, /* It */
	{ need_ge1, NULL, NULL }, /* Ad */ 
	{ NULL, NULL, NULL }, /* An */ 
	{ NULL, NULL, NULL }, /* Ar */
	{ need_ge1, NULL, NULL }, /* Cd */
	{ NULL, NULL, NULL }, /* Cm */
	{ need_ge1, NULL, NULL }, /* Dv */ 
	{ need_ge1, NULL, NULL }, /* Er */ 
	{ need_ge1, NULL, NULL }, /* Ev */ 
	{ NULL, NULL, NULL }, /* Ex */
	{ need_ge1, NULL, NULL }, /* Fa */ 
	{ NULL, NULL, NULL }, /* Fd */ 
	{ NULL, NULL, NULL }, /* Fl */
	{ need_ge1, NULL, NULL }, /* Fn */ 
	{ want_ge1, NULL, NULL }, /* Ft */ 
	{ need_ge1, NULL, NULL }, /* Ic */ 
	{ need_eq1, NULL, NULL }, /* In */ 
	{ want_ge1, NULL, NULL }, /* Li */
	{ want_ge1, NULL, NULL }, /* Nd */ 
	{ NULL, NULL, NULL }, /* Nm */ 
	{ NULL, NULL, NULL }, /* Op */
	{ NULL, NULL, NULL }, /* Ot */
	{ want_ge1, NULL, NULL }, /* Pa */
	{ NULL, NULL, NULL }, /* Rv */
	{ NULL, NULL, NULL }, /* St */
	{ need_ge1, NULL, NULL }, /* Va */
	{ need_ge1, NULL, NULL }, /* Vt */ 
	{ need_le2, args_Xr, NULL }, /* Xr */
	{ need_ge1, args_ref, NULL }, /* %A */
	{ need_ge1, args_ref, NULL }, /* %B */
	{ need_ge1, args_ref, NULL }, /* %D */
	{ need_ge1, args_ref, NULL }, /* %I */
	{ need_ge1, args_ref, NULL }, /* %J */
	{ need_ge1, args_ref, NULL }, /* %N */
	{ need_ge1, args_ref, NULL }, /* %O */
	{ need_ge1, args_ref, NULL }, /* %P */
	{ need_ge1, args_ref, NULL }, /* %R */
	{ need_ge1, args_ref, NULL }, /* %T */
	{ need_ge1, args_ref, NULL }, /* %V */
	{ NULL, NULL, NULL }, /* Ac */
	{ NULL, NULL, NULL }, /* Ao */
	{ NULL, NULL, NULL }, /* Aq */
	{ need_le2, args_At, NULL }, /* At */
	{ NULL, NULL, NULL }, /* Bc */
	{ NULL, NULL, NULL }, /* Bf */ 
	{ NULL, NULL, NULL }, /* Bo */
	{ NULL, NULL, NULL }, /* Bq */
	{ assert_le1, NULL, NULL }, /* Bsx */
	{ assert_le1, NULL, NULL }, /* Bx */
	{ need_eq1, args_bool, NULL }, /* Db */
	{ NULL, NULL, NULL }, /* Dc */
	{ NULL, NULL, NULL }, /* Do */
	{ NULL, NULL, NULL }, /* Dq */
	{ NULL, NULL, NULL }, /* Ec */
	{ NULL, NULL, NULL }, /* Ef */
	{ need_ge1, NULL, NULL }, /* Em */ 
	{ NULL, NULL, NULL }, /* Eo */
	{ assert_le1, NULL, NULL }, /* Fx */
	{ want_ge1, NULL, NULL }, /* Ms */
	{ NULL, NULL, NULL }, /* No */
	{ NULL, NULL, NULL }, /* Ns */
	{ assert_le1, NULL, NULL }, /* Nx */
	{ assert_le1, NULL, NULL }, /* Ox */
	{ NULL, NULL, NULL }, /* Pc */
	{ NULL, NULL, NULL }, /* Pf */
	{ NULL, NULL, NULL }, /* Po */
	{ NULL, NULL, NULL }, /* Pq */
	{ NULL, NULL, NULL }, /* Qc */
	{ NULL, NULL, NULL }, /* Ql */
	{ NULL, NULL, NULL }, /* Qo */
	{ NULL, NULL, NULL }, /* Qq */
	{ NULL, NULL, NULL }, /* Re */
	{ NULL, NULL, NULL }, /* Rs */
	{ NULL, NULL, NULL }, /* Sc */
	{ NULL, NULL, NULL }, /* So */
	{ NULL, NULL, NULL }, /* Sq */
	{ need_eq1, args_bool, NULL }, /* Sm */
	{ need_ge1, NULL, NULL }, /* Sx */
	{ need_ge1, NULL, NULL }, /* Sy */
	{ want_ge1, NULL, NULL }, /* Tn */
	{ assert_eq0, NULL, NULL }, /* Ux */
	{ NULL, NULL, NULL }, /* Xc */
	{ NULL, NULL, NULL }, /* Xo */
	{ NULL, NULL, NULL }, /* Fo */ 
	{ NULL, NULL, NULL }, /* Fc */ 
	{ NULL, NULL, NULL }, /* Oo */
	{ NULL, NULL, NULL }, /* Oc */
	{ NULL, NULL, NULL }, /* Bk */
	{ NULL, NULL, NULL }, /* Ek */
	{ need_eq0, NULL, NULL }, /* Bt */
	{ need_eq1, NULL, NULL }, /* Hf */
	{ NULL, NULL, NULL }, /* Fr */
	{ need_eq0, NULL, NULL }, /* Ud */
};


static int
need_le2(struct mdoc *mdoc, int tok, int pos, int sz)
{
	if (sz <= 2)
		return(1);
	return(mdoc_err(mdoc, tok, pos, ERR_ARGS_LE2));
}


static int
want_ge1(struct mdoc *mdoc, int tok, int pos, int sz)
{
	if (sz > 0)
		return(1);
	return(mdoc_warn(mdoc, tok, pos, WARN_ARGS_GE1));
}


static int
want_eq0(struct mdoc *mdoc, int tok, int pos, int sz)
{
	if (sz == 0)
		return(1);
	return(mdoc_warn(mdoc, tok, pos, WARN_ARGS_EQ0));
}


static int
need_eq0(struct mdoc *mdoc, int tok, int pos, int sz)
{
	if (sz == 0)
		return(1);
	return(mdoc_err(mdoc, tok, pos, ERR_ARGS_EQ0));
}


static int
assert_le1(struct mdoc *mdoc, int tok, int pos, int sz)
{

	assert(sz <= 1);
	return(1);
}


static int
assert_eq0(struct mdoc *mdoc, int tok, int pos, int sz)
{

	assert(sz == 0);
	return(1);
}


static int
need_eq1(struct mdoc *mdoc, int tok, int pos, int sz)
{
	if (sz == 1)
		return(1);
	return(mdoc_err(mdoc, tok, pos, ERR_ARGS_EQ1));
}


static int
need_ge1(struct mdoc *mdoc, int tok, int pos, int sz)
{
	if (sz > 0)
		return(1);
	return(mdoc_err(mdoc, tok, pos, ERR_ARGS_GE1));
}


static int
need_head_child(struct mdoc *mdoc, int tok, int pos)
{
	struct mdoc_node *n;

	assert(mdoc->last);
	n = mdoc->last;

	assert(MDOC_BLOCK == n->type);
	assert(n->child);

	n = n->child;

	if (MDOC_HEAD != n->type) 
		return(mdoc_err(mdoc, tok, pos, ERR_CHILD_HEAD));
	if (n->child)
		return(1);
	return(mdoc_err(mdoc, tok, pos, ERR_CHILD_HEAD));
}


static int
args_Sh(struct mdoc *mdoc, int tok, int pos, 
		int sz, const char *args[], 
		int argc, const struct mdoc_arg *argv)
{
	enum mdoc_sec	 sec;

	sec = mdoc_atosec((size_t)sz, args);
	if (SEC_CUSTOM != sec && sec < mdoc->sec_lastn)
		if ( ! mdoc_warn(mdoc, tok, pos, WARN_SEC_OO))
			return(0);
	if (SEC_BODY == mdoc->sec_last && SEC_NAME != sec)
		return(mdoc_err(mdoc, tok, pos, ERR_SEC_NAME));

	return(1);
}


static int
args_bool(struct mdoc *mdoc, int tok, int pos, 
		int sz, const char *args[], 
		int argc, const struct mdoc_arg *argv)
{

	assert(1 == sz);
	if (xstrcmp(args[0], "on"))
		return(1);	
	if (xstrcmp(args[0], "off"))
		return(1);
	return(mdoc_err(mdoc, tok, pos, ERR_SYNTAX_ARGBAD));
}


static int
args_ref(struct mdoc *mdoc, int tok, int pos, 
		int sz, const char *args[], 
		int argc, const struct mdoc_arg *argv)
{
	struct mdoc_node *n;

	assert(mdoc->last);
	for (n = mdoc->last ; n; n = n->parent) {
		if (MDOC_BLOCK != n->type)
			continue;
		if (MDOC_Rs != n->data.block.tok)
			break;
		return(1);
	}

	return(mdoc_err(mdoc, tok, pos, ERR_SCOPE_NOCTX));
}


static int
args_Xr(struct mdoc *mdoc, int tok, int pos, 
		int sz, const char *args[], 
		int argc, const struct mdoc_arg *argv)
{

	if (1 == sz)
		return(1);
	if (0 == sz)
		return(mdoc_err(mdoc, tok, pos, ERR_ARGS_GE1));

	if (MSEC_DEFAULT == mdoc_atomsec(args[1]))
		return(mdoc_err(mdoc, tok, pos, ERR_SYNTAX_ARGFORM));
	return(1);
}


static int
args_At(struct mdoc *mdoc, int tok, int pos, 
		int sz, const char *args[], 
		int argc, const struct mdoc_arg *argv)
{
	int		 i;

	if (0 == sz)
		return(1);

	i = 0;
	if (ATT_DEFAULT == mdoc_atoatt(args[i]))
		i++;

	for ( ; i < sz; i++) {
		if ( ! mdoc_isdelim(args[i]))
			continue;
		return(mdoc_err(mdoc, tok, pos, ERR_SYNTAX_NOPUNCT));
	}
	return(1);
}


static int
args_blocknest(struct mdoc *mdoc, int tok, int pos, 
		int sz, const char *args[], 
		int argc, const struct mdoc_arg *argv)
{
	struct mdoc_node *node;

	/*
	 * We can't be nested within any other block displays (or really
	 * any other kind of display, although Bd is the only multi-line
	 * one that will show up). 
	 */
	assert(mdoc->last);

	/* LINTED */
	for (node = mdoc->last; node; node = node->parent) {
		if (node->type != MDOC_BLOCK)
			continue;
		if (node->data.block.tok != MDOC_Bd)
			continue;
		break;
	}
	if (NULL == node)
		return(1);
	return(mdoc_err(mdoc, tok, pos, ERR_SCOPE_NONEST));
}


int
mdoc_valid_pre(struct mdoc *mdoc, int tok, int pos, 
		int sz, const char *args[], 
		int argc, const struct mdoc_arg *argv)
{

	assert(tok < MDOC_MAX);
	if (mdoc_valids[tok].sz) 
		if ( ! (*mdoc_valids[tok].sz)(mdoc, tok, pos, sz))
			return(0);

	if (NULL == mdoc_valids[tok].args) 
		return(1);
	return((*mdoc_valids[tok].args)(mdoc, 
			tok, pos, sz, args, argc, argv));
}


int
mdoc_valid_post(struct mdoc *mdoc, int tok, int pos)
{

	if (NULL == mdoc_valids[tok].post)
		return(1);
	return((*mdoc_valids[tok].post)(mdoc, tok, pos));
}
