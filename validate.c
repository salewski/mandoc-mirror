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
typedef int	(*v_args)(struct mdoc *, int, int, 
			int, const char *[],
			int, const struct mdoc_arg *);
typedef int	(*v_tree)(struct mdoc *, int, int);


struct	valids {
	v_args_sz sz;
	v_args	  args;
	v_tree	  tree_pre;
	v_tree	  tree_post;
};


static	int	  assert_eq0(struct mdoc *, int, int, int);
static	int	  assert_le1(struct mdoc *, int, int, int);
static	int	  need_eq0(struct mdoc *, int, int, int);
static	int	  need_eq1(struct mdoc *, int, int, int);
static	int	  need_ge1(struct mdoc *, int, int, int);
static	int	  need_le2(struct mdoc *, int, int, int);
static	int	  want_eq0(struct mdoc *, int, int, int);
static	int	  want_ge1(struct mdoc *, int, int, int);

static	int	  tree_pre_ref(struct mdoc *, int, int);
static	int	  tree_pre_display(struct mdoc *, int, int);

static	int	  tree_post_onlyhead(struct mdoc *, int, int);
static	int	  tree_post_onlybody(struct mdoc *, int, int);
static	int	  tree_post_warnemptybody(struct mdoc *, int, int);

static	int	  args_bool(struct mdoc *, int, int, 
			int, const char *[], 
			int, const struct mdoc_arg *);
static	int	  args_sh(struct mdoc *, int, int, 
			int, const char *[], 
			int, const struct mdoc_arg *);
static	int	  args_an(struct mdoc *, int, int, 
			int, const char *[], 
			int, const struct mdoc_arg *);
static	int	  args_nopunct(struct mdoc *, int, int, 
			int, const char *[], 
			int, const struct mdoc_arg *);
static	int	  args_xr(struct mdoc *, int, int, 
			int, const char *[], 
			int, const struct mdoc_arg *);


const	struct valids mdoc_valids[MDOC_MAX] = {
	{ NULL, NULL, NULL, NULL }, /* \" */
	{ NULL, NULL, NULL, NULL }, /* Dd */ /* TODO */
	{ NULL, NULL, NULL, NULL }, /* Dt */ /* TODO */
	{ NULL, NULL, NULL, NULL }, /* Os */ /* TODO */
	{ want_ge1, args_sh, NULL, NULL }, /* Sh */
	{ want_ge1, NULL, NULL, NULL }, /* Ss */ 
	{ want_eq0, NULL, NULL, NULL }, /* Pp */ 
	{ assert_eq0, NULL, tree_pre_display, tree_post_onlyhead }, /* D1 */
	{ assert_eq0, NULL, tree_pre_display, tree_post_onlyhead }, /* Dl */
	{ want_eq0, NULL, tree_pre_display, tree_post_warnemptybody }, /* Bd */
	{ assert_eq0, NULL, NULL, tree_post_onlybody }, /* Ed */
	{ want_eq0, NULL, NULL, NULL }, /* Bl */
	{ assert_eq0, NULL, NULL, tree_post_onlybody }, /* El */
	{ NULL, NULL, NULL, NULL }, /* It */
	{ need_ge1, NULL, NULL, NULL }, /* Ad */ 
	{ NULL, args_an, NULL, NULL }, /* An */
	{ NULL, NULL, NULL, NULL }, /* Ar */
	{ need_ge1, NULL, NULL, NULL }, /* Cd */
	{ NULL, NULL, NULL, NULL }, /* Cm */
	{ need_ge1, NULL, NULL, NULL }, /* Dv */ 
	{ need_ge1, NULL, NULL, NULL }, /* Er */ 
	{ need_ge1, NULL, NULL, NULL }, /* Ev */ 
	{ NULL, NULL, NULL, NULL }, /* Ex */
	{ need_ge1, NULL, NULL, NULL }, /* Fa */ 
	{ NULL, NULL, NULL, NULL }, /* Fd */ 
	{ NULL, NULL, NULL, NULL }, /* Fl */
	{ need_ge1, NULL, NULL, NULL }, /* Fn */ 
	{ want_ge1, NULL, NULL, NULL }, /* Ft */ 
	{ need_ge1, NULL, NULL, NULL }, /* Ic */ 
	{ need_eq1, NULL, NULL, NULL }, /* In */ 
	{ want_ge1, NULL, NULL, NULL }, /* Li */
	{ want_ge1, NULL, NULL, NULL }, /* Nd */ 
	{ NULL, NULL, NULL, NULL }, /* Nm */ 
	{ NULL, NULL, NULL, NULL }, /* Op */
	{ NULL, NULL, NULL, NULL }, /* Ot */
	{ want_ge1, NULL, NULL, NULL }, /* Pa */
	{ NULL, NULL, NULL, NULL }, /* Rv */
	{ NULL, NULL, NULL, NULL }, /* St */
	{ need_ge1, NULL, NULL, NULL }, /* Va */
	{ need_ge1, NULL, NULL, NULL }, /* Vt */ 
	{ need_le2, args_xr, NULL, NULL }, /* Xr */
	{ need_ge1, NULL, tree_pre_ref, NULL }, /* %A */
	{ need_ge1, NULL, tree_pre_ref, NULL }, /* %B */
	{ need_ge1, NULL, tree_pre_ref, NULL }, /* %D */
	{ need_ge1, NULL, tree_pre_ref, NULL }, /* %I */
	{ need_ge1, NULL, tree_pre_ref, NULL }, /* %J */
	{ need_ge1, NULL, tree_pre_ref, NULL }, /* %N */
	{ need_ge1, NULL, tree_pre_ref, NULL }, /* %O */
	{ need_ge1, NULL, tree_pre_ref, NULL }, /* %P */
	{ need_ge1, NULL, tree_pre_ref, NULL }, /* %R */
	{ need_ge1, NULL, tree_pre_ref, NULL }, /* %T */
	{ need_ge1, NULL, tree_pre_ref, NULL }, /* %V */
	{ NULL, NULL, NULL, NULL }, /* Ac */
	{ NULL, NULL, NULL, NULL }, /* Ao */
	{ NULL, NULL, NULL, NULL }, /* Aq */
	{ need_le2, args_nopunct, NULL, NULL }, /* At */
	{ NULL, NULL, NULL, NULL }, /* Bc */
	{ NULL, NULL, NULL, NULL }, /* Bf */ 
	{ NULL, NULL, NULL, NULL }, /* Bo */
	{ NULL, NULL, NULL, NULL }, /* Bq */
	{ assert_le1, NULL, NULL, NULL }, /* Bsx */
	{ assert_le1, NULL, NULL, NULL }, /* Bx */
	{ need_eq1, args_bool, NULL, NULL }, /* Db */
	{ NULL, NULL, NULL, NULL }, /* Dc */
	{ NULL, NULL, NULL, NULL }, /* Do */
	{ NULL, NULL, NULL, NULL }, /* Dq */
	{ NULL, NULL, NULL, NULL }, /* Ec */
	{ NULL, NULL, NULL, NULL }, /* Ef */ /* -symbolic, etc. */
	{ need_ge1, NULL, NULL, NULL }, /* Em */ 
	{ NULL, NULL, NULL, NULL }, /* Eo */
	{ assert_le1, NULL, NULL, NULL }, /* Fx */
	{ want_ge1, NULL, NULL, NULL }, /* Ms */
	{ NULL, NULL, NULL, NULL }, /* No */
	{ NULL, NULL, NULL, NULL }, /* Ns */
	{ assert_le1, NULL, NULL, NULL }, /* Nx */
	{ assert_le1, NULL, NULL, NULL }, /* Ox */
	{ NULL, NULL, NULL, NULL }, /* Pc */
	{ NULL, NULL, NULL, NULL }, /* Pf */ /* 2 or more arguments */
	{ NULL, NULL, NULL, NULL }, /* Po */
	{ NULL, NULL, NULL, NULL }, /* Pq */
	{ NULL, NULL, NULL, NULL }, /* Qc */
	{ NULL, NULL, NULL, NULL }, /* Ql */
	{ NULL, NULL, NULL, NULL }, /* Qo */
	{ NULL, NULL, NULL, NULL }, /* Qq */
	{ NULL, NULL, NULL, NULL }, /* Re */
	{ NULL, NULL, NULL, NULL }, /* Rs */
	{ NULL, NULL, NULL, NULL }, /* Sc */
	{ NULL, NULL, NULL, NULL }, /* So */
	{ NULL, NULL, NULL, NULL }, /* Sq */
	{ need_eq1, args_bool, NULL, NULL }, /* Sm */
	{ need_ge1, NULL, NULL, NULL }, /* Sx */
	{ need_ge1, NULL, NULL, NULL }, /* Sy */
	{ want_ge1, NULL, NULL, NULL }, /* Tn */
	{ assert_eq0, NULL, NULL, NULL }, /* Ux */
	{ NULL, NULL, NULL, NULL }, /* Xc */
	{ NULL, NULL, NULL, NULL }, /* Xo */
	{ NULL, NULL, NULL, NULL }, /* Fo */ 
	{ NULL, NULL, NULL, NULL }, /* Fc */ 
	{ NULL, NULL, NULL, NULL }, /* Oo */
	{ NULL, NULL, NULL, NULL }, /* Oc */
	{ NULL, NULL, NULL, NULL }, /* Bk */
	{ NULL, NULL, NULL, NULL }, /* Ek */
	{ need_eq0, NULL, NULL, NULL }, /* Bt */
	{ need_eq1, NULL, NULL, NULL }, /* Hf */
	{ NULL, NULL, NULL, NULL }, /* Fr */
	{ need_eq0, NULL, NULL, NULL }, /* Ud */
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
tree_post_onlybody(struct mdoc *mdoc, int tok, int pos)
{
	struct mdoc_node *n;

	assert(mdoc->last);
	n = mdoc->last;

	assert(MDOC_BLOCK == n->type);
	assert(n->child);

	if (MDOC_BODY == n->child->type) {
		if (n->child->child)
			return(1);
		return(mdoc_err(mdoc, tok, pos, ERR_SYNTAX_EMPTYBODY));
	}

	return(mdoc_err(mdoc, tok, pos, ERR_SYNTAX_CHILDBODY));
}


static int
tree_post_warnemptybody(struct mdoc *mdoc, int tok, int pos)
{
	struct mdoc_node *n;

	assert(mdoc->last);
	n = mdoc->last;

	assert(MDOC_BLOCK == n->type);
	assert(n->child);

	for (n = n->child; n; n = n->next)
		if (MDOC_BODY == n->type)
			break;

	if (n && n->child)
		return(1);
	return(mdoc_warn(mdoc, tok, pos, WARN_SYNTAX_EMPTYBODY));
}


static int
tree_post_onlyhead(struct mdoc *mdoc, int tok, int pos)
{
	struct mdoc_node *n;

	assert(mdoc->last);
	n = mdoc->last;

	assert(MDOC_BLOCK == n->type);
	assert(n->child);

	n = n->child;

	if (MDOC_HEAD != n->type) 
		return(mdoc_err(mdoc, tok, pos, ERR_SYNTAX_CHILDHEAD));
	if (n->child)
		return(1);
	return(mdoc_err(mdoc, tok, pos, ERR_SYNTAX_EMPTYHEAD));
}


static int
args_an(struct mdoc *mdoc, int tok, int pos, 
		int sz, const char *args[],
		int argc, const struct mdoc_arg *argv)
{

	printf("argc=%d, sz=%d\n", argc, sz);
	if (0 != argc && 0 != sz) 
		return(mdoc_warn(mdoc, tok, pos, WARN_ARGS_EQ0));
	return(1);
}


static int
args_sh(struct mdoc *mdoc, int tok, int pos, 
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
	int	 	 i;

	for (i = 0; i < sz; i++) {
		if (xstrcmp(args[i], "on"))
			continue;
		if (xstrcmp(args[i], "off"))
			continue;
		return(mdoc_err(mdoc, tok, pos, ERR_SYNTAX_ARGBAD));
	}
	return(1);
}


static int
tree_pre_ref(struct mdoc *mdoc, int tok, int pos)
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
args_xr(struct mdoc *mdoc, int tok, int pos, 
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
args_nopunct(struct mdoc *mdoc, int tok, int pos, 
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
tree_pre_display(struct mdoc *mdoc, int tok, int pos)
{
	struct mdoc_node *node;

	assert(mdoc->last);

	/* Displays may not be nested in other displays. */

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
	if (mdoc_valids[tok].args) 
		if ( ! (*mdoc_valids[tok].args)(mdoc, tok, pos, 
					sz, args, argc, argv))
			return(0);
	if (mdoc_valids[tok].tree_pre) 
		if ( ! (*mdoc_valids[tok].tree_pre)(mdoc, tok, pos))
			return(0);
	return(1);
}


int
mdoc_valid_post(struct mdoc *mdoc, int tok, int pos)
{

	if (mdoc_valids[tok].tree_post)
		return((*mdoc_valids[tok].tree_post)(mdoc, tok, pos));
	return(1);
}

