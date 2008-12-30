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


typedef int	(*v_sz)(struct mdoc *, int, int, int);
typedef int	(*v_extra)(struct mdoc *, int, int, int, 
			const char *[], int, const struct mdoc_arg *);

static int	  assert_eq0(struct mdoc *, int, int, int);
static int	  assert_le1(struct mdoc *, int, int, int);
static int	  need_eq0(struct mdoc *, int, int, int);
static int	  need_eq1(struct mdoc *, int, int, int);
static int	  need_ge1(struct mdoc *, int, int, int);
static int	  need_le2(struct mdoc *, int, int, int);
static int	  want_eq0(struct mdoc *, int, int, int);
static int	  want_ge1(struct mdoc *, int, int, int);
static int	  v_Sh(struct mdoc *, int, int, int, 
			const char *[], int, const struct mdoc_arg *);
static int	  v_Bd(struct mdoc *, int, int, int, 
			const char *[], int, const struct mdoc_arg *);
static int	  v_At(struct mdoc *, int, int, int, 
			const char *[], int, const struct mdoc_arg *);

struct	valids {
	v_sz	  sz;
	v_extra	  extra;
};


const	struct valids mdoc_valids[MDOC_MAX] = {
	{ NULL, NULL }, /* \" */
	{ NULL, NULL }, /* Dd */
	{ NULL, NULL }, /* Dt */
	{ NULL, NULL }, /* Os */
	{ need_ge1, v_Sh }, /* Sh */
	{ need_ge1, NULL }, /* Ss */ 
	{ want_eq0, NULL }, /* Pp */ 
	{ NULL, NULL }, /* D1 */
	{ NULL, NULL }, /* Dl */
	{ NULL, v_Bd }, /* Bd */
	{ NULL, NULL }, /* Ed */
	{ NULL, NULL }, /* Bl */
	{ NULL, NULL }, /* El */
	{ NULL, NULL }, /* It */
	{ need_ge1, NULL }, /* Ad */ 
	{ NULL, NULL }, /* An */ 
	{ NULL, NULL }, /* Ar */
	{ NULL, NULL }, /* Cd */
	{ NULL, NULL }, /* Cm */
	{ need_ge1, NULL }, /* Dv */ 
	{ need_ge1, NULL }, /* Er */ 
	{ need_ge1, NULL }, /* Ev */ 
	{ NULL, NULL }, /* Ex */
	{ need_ge1, NULL }, /* Fa */ 
	{ NULL, NULL }, /* Fd */ 
	{ NULL, NULL }, /* Fl */
	{ NULL, NULL }, /* Fn */ 
	{ want_ge1, NULL }, /* Ft */ 
	{ need_ge1, NULL }, /* Ic */ 
	{ NULL, NULL }, /* In */ 
	{ want_ge1, NULL }, /* Li */
	{ want_ge1, NULL }, /* Nd */ 
	{ NULL, NULL }, /* Nm */ 
	{ NULL, NULL }, /* Op */
	{ NULL, NULL }, /* Ot */
	{ want_ge1, NULL }, /* Pa */
	{ NULL, NULL }, /* Rv */
	{ NULL, NULL }, /* St */
	{ need_ge1, NULL }, /* Va */
	{ need_ge1, NULL }, /* Vt */ 
	{ NULL, NULL }, /* Xr */
	{ NULL, NULL }, /* %A */
	{ NULL, NULL }, /* %B */
	{ NULL, NULL }, /* %D */
	{ NULL, NULL }, /* %I */
	{ NULL, NULL }, /* %J */
	{ NULL, NULL }, /* %N */
	{ NULL, NULL }, /* %O */
	{ NULL, NULL }, /* %P */
	{ NULL, NULL }, /* %R */
	{ NULL, NULL }, /* %T */
	{ NULL, NULL }, /* %V */
	{ NULL, NULL }, /* Ac */
	{ NULL, NULL }, /* Ao */
	{ NULL, NULL }, /* Aq */
	{ need_le2, v_At }, /* At */
	{ NULL, NULL }, /* Bc */
	{ NULL, NULL }, /* Bf */ 
	{ NULL, NULL }, /* Bo */
	{ NULL, NULL }, /* Bq */
	{ assert_le1, NULL }, /* Bsx */
	{ assert_le1, NULL }, /* Bx */
	{ NULL, NULL }, /* Db */
	{ NULL, NULL }, /* Dc */
	{ NULL, NULL }, /* Do */
	{ NULL, NULL }, /* Dq */
	{ NULL, NULL }, /* Ec */
	{ NULL, NULL }, /* Ef */
	{ need_ge1, NULL }, /* Em */ 
	{ NULL, NULL }, /* Eo */
	{ assert_le1, NULL }, /* Fx */
	{ want_ge1, NULL }, /* Ms */
	{ NULL, NULL }, /* No */
	{ NULL, NULL }, /* Ns */
	{ assert_le1, NULL }, /* Nx */
	{ assert_le1, NULL }, /* Ox */
	{ NULL, NULL }, /* Pc */
	{ NULL, NULL }, /* Pf */
	{ NULL, NULL }, /* Po */
	{ NULL, NULL }, /* Pq */
	{ NULL, NULL }, /* Qc */
	{ NULL, NULL }, /* Ql */
	{ NULL, NULL }, /* Qo */
	{ NULL, NULL }, /* Qq */
	{ NULL, NULL }, /* Re */
	{ NULL, NULL }, /* Rs */
	{ NULL, NULL }, /* Sc */
	{ NULL, NULL }, /* So */
	{ NULL, NULL }, /* Sq */
	{ NULL, NULL }, /* Sm */
	{ need_ge1, NULL }, /* Sx */
	{ need_ge1, NULL }, /* Sy */
	{ want_ge1, NULL }, /* Tn */
	{ assert_eq0, NULL }, /* Ux */
	{ NULL, NULL }, /* Xc */
	{ NULL, NULL }, /* Xo */
	{ NULL, NULL }, /* Fo */ 
	{ NULL, NULL }, /* Fc */ 
	{ NULL, NULL }, /* Oo */
	{ NULL, NULL }, /* Oc */
	{ NULL, NULL }, /* Bk */
	{ NULL, NULL }, /* Ek */
	{ need_eq0, NULL }, /* Bt */
	{ need_eq1, NULL }, /* Hf */
	{ NULL, NULL }, /* Fr */
	{ need_eq0, NULL }, /* Ud */
};


static int
need_le2(struct mdoc *mdoc, int tok, int pos, int sz)
{
	if (sz > 2)
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
v_Sh(struct mdoc *mdoc, int tok, int pos, 
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
v_At(struct mdoc *mdoc, int tok, int pos, 
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
v_Bd(struct mdoc *mdoc, int tok, int pos, 
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
	for (node = mdoc->last->parent ; node; node = node->parent) {
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
mdoc_valid(struct mdoc *mdoc, int tok, int pos, 
		int sz, const char *args[], 
		int argc, const struct mdoc_arg *argv)
{

	assert(tok < MDOC_MAX);
	if (mdoc_valids[tok].sz) 
		if ( ! (*mdoc_valids[tok].sz)(mdoc, tok, pos, sz))
			return(0);

	if (NULL == mdoc_valids[tok].extra) 
		return(1);
	return(*mdoc_valids[tok].extra)(mdoc, 
			tok, pos, sz, args, argc, argv);
}

