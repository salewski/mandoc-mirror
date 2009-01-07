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


typedef	int	(*v_pre)(struct mdoc *, int, int, 
			int, const struct mdoc_arg *);
typedef	int	(*v_post)(struct mdoc *, int, int);


struct	valids {
	v_pre	 pre;
	v_post	*post;
};


static	int	pre_sh(struct mdoc *, int, int, 
			int, const struct mdoc_arg *);
static	int	post_headchild_err_ge1(struct mdoc *, int, int);
static	int	post_headchild_err_le8(struct mdoc *, int, int);
static	int	post_bodychild_warn_ge1(struct mdoc *, int, int);

static v_post	posts_sh[] = { post_headchild_err_ge1, 
			post_bodychild_warn_ge1, 
			post_headchild_err_le8, NULL };


const	struct valids mdoc_valids[MDOC_MAX] = {
	{ NULL, NULL }, /* \" */
	{ NULL, NULL }, /* Dd */ /* TODO */
	{ NULL, NULL }, /* Dt */ /* TODO */
	{ NULL, NULL }, /* Os */ /* TODO */
	{ pre_sh, posts_sh }, /* Sh */ /* FIXME: preceding Pp. */
	{ NULL, NULL }, /* Ss */ /* FIXME: preceding Pp. */
	{ NULL, NULL }, /* Pp */ 
	{ NULL, NULL }, /* D1 */
	{ NULL, NULL }, /* Dl */
	{ NULL, NULL }, /* Bd */ /* FIXME: preceding Pp. */
	{ NULL, NULL }, /* Ed */
	{ NULL, NULL }, /* Bl */ /* FIXME: preceding Pp. */
	{ NULL, NULL }, /* El */
	{ NULL, NULL }, /* It */
	{ NULL, NULL }, /* Ad */ 
	{ NULL, NULL }, /* An */
	{ NULL, NULL }, /* Ar */
	{ NULL, NULL }, /* Cd */
	{ NULL, NULL }, /* Cm */
	{ NULL, NULL }, /* Dv */ 
	{ NULL, NULL }, /* Er */ 
	{ NULL, NULL }, /* Ev */ 
	{ NULL, NULL }, /* Ex */
	{ NULL, NULL }, /* Fa */ 
	{ NULL, NULL }, /* Fd */ 
	{ NULL, NULL }, /* Fl */
	{ NULL, NULL }, /* Fn */ 
	{ NULL, NULL }, /* Ft */ 
	{ NULL, NULL }, /* Ic */ 
	{ NULL, NULL }, /* In */ 
	{ NULL, NULL }, /* Li */
	{ NULL, NULL }, /* Nd */ 
	{ NULL, NULL }, /* Nm */ 
	{ NULL, NULL }, /* Op */
	{ NULL, NULL }, /* Ot */
	{ NULL, NULL }, /* Pa */
	{ NULL, NULL }, /* Rv */
	{ NULL, NULL }, /* St */
	{ NULL, NULL }, /* Va */
	{ NULL, NULL }, /* Vt */ 
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
	{ NULL, NULL }, /* At */ /* FIXME */
	{ NULL, NULL }, /* Bc */
	{ NULL, NULL }, /* Bf */ 
	{ NULL, NULL }, /* Bo */
	{ NULL, NULL }, /* Bq */
	{ NULL, NULL }, /* Bsx */
	{ NULL, NULL }, /* Bx */
	{ NULL, NULL }, /* Db */
	{ NULL, NULL }, /* Dc */
	{ NULL, NULL }, /* Do */
	{ NULL, NULL }, /* Dq */
	{ NULL, NULL }, /* Ec */
	{ NULL, NULL }, /* Ef */ /* -symbolic, etc. */
	{ NULL, NULL }, /* Em */ 
	{ NULL, NULL }, /* Eo */
	{ NULL, NULL }, /* Fx */
	{ NULL, NULL }, /* Ms */
	{ NULL, NULL }, /* No */
	{ NULL, NULL }, /* Ns */
	{ NULL, NULL }, /* Nx */
	{ NULL, NULL }, /* Ox */
	{ NULL, NULL }, /* Pc */
	{ NULL, NULL }, /* Pf */ /* 2 or more arguments */
	{ NULL, NULL }, /* Po */
	{ NULL, NULL }, /* Pq */ /* FIXME: ignore following Sh/Ss */
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
	{ NULL, NULL }, /* Sx */
	{ NULL, NULL }, /* Sy */
	{ NULL, NULL }, /* Tn */
	{ NULL, NULL }, /* Ux */
	{ NULL, NULL }, /* Xc */
	{ NULL, NULL }, /* Xo */
	{ NULL, NULL }, /* Fo */ 
	{ NULL, NULL }, /* Fc */ 
	{ NULL, NULL }, /* Oo */
	{ NULL, NULL }, /* Oc */
	{ NULL, NULL }, /* Bk */
	{ NULL, NULL }, /* Ek */
	{ NULL, NULL }, /* Bt */
	{ NULL, NULL }, /* Hf */
	{ NULL, NULL }, /* Fr */
	{ NULL, NULL }, /* Ud */
};


static int
post_bodychild_warn_ge1(struct mdoc *mdoc, int tok, int pos)
{

	if (MDOC_BODY != mdoc->last->type)
		return(1);
	if (mdoc->last->child)
		return(1);

	return(mdoc_warn(mdoc, tok, pos, WARN_ARGS_GE1));
}


static int
post_headchild_err_ge1(struct mdoc *mdoc, int tok, int pos)
{

	if (MDOC_HEAD != mdoc->last->type)
		return(1);
	if (mdoc->last->child)
		return(1);
	return(mdoc_err(mdoc, tok, pos, ERR_ARGS_GE1));
}


static int
post_headchild_err_le8(struct mdoc *mdoc, int tok, int pos)
{
	int		  i;
	struct mdoc_node *n;

	if (MDOC_HEAD != mdoc->last->type)
		return(1);
	for (i = 0, n = mdoc->last->child; n; n = n->next, i++)
		/* Do nothing. */ ;
	if (i <= 8)
		return(1);
	return(mdoc_err(mdoc, tok, pos, ERR_ARGS_LE8));
}


static int
pre_sh(struct mdoc *mdoc, int tok, int pos,
		int argc, const struct mdoc_arg *argv)
{

	return(1);
}


int
mdoc_valid_pre(struct mdoc *mdoc, int tok, int pos, 
		int argc, const struct mdoc_arg *argv)
{

	if (NULL == mdoc_valids[tok].pre)
		return(1);
	return((*mdoc_valids[tok].pre)(mdoc, tok, pos, argc, argv));
}


int
mdoc_valid_post(struct mdoc *mdoc, int pos)
{
	v_post		*p;
	int		 t;

	switch (mdoc->last->type) {
	case (MDOC_BODY):
		t = mdoc->last->data.body.tok;
		break;
	case (MDOC_ELEM):
		t = mdoc->last->data.elem.tok;
		break;
	case (MDOC_BLOCK):
		t = mdoc->last->data.block.tok;
		break;
	case (MDOC_HEAD):
		t = mdoc->last->data.head.tok;
		break;
	default:
		return(1);
	}

	if (NULL == mdoc_valids[t].post)
		return(1);

	for (p = mdoc_valids[t].post; *p; p++)
		if ( ! (*p)(mdoc, t, pos)) 
			return(0);

	return(1);
}

