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


typedef	int	(*v_pre)(struct mdoc *, struct mdoc_node *);
typedef	int	(*v_post)(struct mdoc *);


struct	valids {
	v_pre	*pre;
	v_post	*post;
};


static	int	pre_display(struct mdoc *, struct mdoc_node *);
static	int	pre_bd(struct mdoc *, struct mdoc_node *);
static	int	pre_bl(struct mdoc *, struct mdoc_node *);
static	int	pre_prologue(struct mdoc *, struct mdoc_node *);
static	int	pre_prologue(struct mdoc *, struct mdoc_node *);
static	int	pre_prologue(struct mdoc *, struct mdoc_node *);

static	int	headchild_err_ge1(struct mdoc *);
static	int	headchild_err_eq0(struct mdoc *);
static	int	elemchild_err_ge1(struct mdoc *);
static	int	elemchild_warn_eq0(struct mdoc *);
static	int	bodychild_warn_ge1(struct mdoc *);
static	int	post_sh(struct mdoc *);
static	int	post_bl(struct mdoc *);

static	v_pre	pres_prologue[] = { pre_prologue, NULL };
static	v_pre	pres_d1[] = { pre_display, NULL };
static	v_pre	pres_bd[] = { pre_display, pre_bd, NULL };
static	v_pre	pres_bl[] = { pre_bl, NULL };
static	v_post	posts_bd[] = { headchild_err_eq0, 
			bodychild_warn_ge1, NULL };

static	v_post	posts_sh[] = { headchild_err_ge1, 
			bodychild_warn_ge1, post_sh, NULL };
static	v_post	posts_bl[] = { headchild_err_eq0, 
			bodychild_warn_ge1, post_bl, NULL };
static	v_post	posts_ss[] = { headchild_err_ge1, NULL };
static	v_post	posts_pp[] = { elemchild_warn_eq0, NULL };
static	v_post	posts_dd[] = { elemchild_err_ge1, NULL };
static	v_post	posts_d1[] = { headchild_err_ge1, NULL };


const	struct valids mdoc_valids[MDOC_MAX] = {
	{ NULL, NULL }, /* \" */
	{ pres_prologue, posts_dd }, /* Dd */
	{ pres_prologue, NULL }, /* Dt */
	{ pres_prologue, NULL }, /* Os */
	{ NULL, posts_sh }, /* Sh */ /* FIXME: preceding Pp. */
	{ NULL, posts_ss }, /* Ss */ /* FIXME: preceding Pp. */
	{ NULL, posts_pp }, /* Pp */ /* FIXME: proceeding... */
	{ pres_d1, posts_d1 }, /* D1 */
	{ pres_d1, posts_d1 }, /* Dl */
	{ pres_bd, posts_bd }, /* Bd */ /* FIXME: preceding Pp. */
	{ NULL, NULL }, /* Ed */
	{ pres_bl, posts_bl }, /* Bl */ /* FIXME: preceding Pp. */
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
bodychild_warn_ge1(struct mdoc *mdoc)
{

	if (MDOC_BODY != mdoc->last->type)
		return(1);
	if (mdoc->last->child)
		return(1);
	return(mdoc_warn(mdoc, WARN_ARGS_GE1));
}


static int
elemchild_warn_eq0(struct mdoc *mdoc)
{

	assert(MDOC_ELEM == mdoc->last->type);
	if (NULL == mdoc->last->child)
		return(1);
	return(mdoc_pwarn(mdoc, mdoc->last->child->line,
			mdoc->last->child->pos, WARN_ARGS_EQ0));
}


static int
elemchild_err_ge1(struct mdoc *mdoc)
{

	assert(MDOC_ELEM == mdoc->last->type);
	if (mdoc->last->child)
		return(1);
	return(mdoc_err(mdoc, ERR_ARGS_GE1));
}


static int
headchild_err_eq0(struct mdoc *mdoc)
{

	if (MDOC_HEAD != mdoc->last->type)
		return(1);
	if (NULL == mdoc->last->child)
		return(1);
	return(mdoc_perr(mdoc, mdoc->last->child->line,
			mdoc->last->child->pos, ERR_ARGS_EQ0));
}


static int
headchild_err_ge1(struct mdoc *mdoc)
{

	if (MDOC_HEAD != mdoc->last->type)
		return(1);
	if (mdoc->last->child)
		return(1);
	return(mdoc_err(mdoc, ERR_ARGS_GE1));
}


static int
pre_display(struct mdoc *mdoc, struct mdoc_node *node)
{
	struct mdoc_node *n;

	if (MDOC_BLOCK != node->type)
		return(1);

	for (n = mdoc->last; n; n = n->parent) 
		if (MDOC_BLOCK == n->type)
			if (MDOC_Bd == n->data.block.tok)
				break;
	if (NULL == n)
		return(1);
	return(mdoc_verr(mdoc, node, ERR_SCOPE_NONEST));
}


static int
pre_bl(struct mdoc *mdoc, struct mdoc_node *node)
{
	int		 type, err;
	struct mdoc_arg	*argv;
	size_t		 i, argc;

	if (MDOC_BLOCK != node->type)
		return(1);
	assert(MDOC_Bl == node->data.block.tok);

	argv = NULL;
	argc = node->data.block.argc; 

	for (i = type = err = 0; i < argc; i++) {
		argv = &node->data.block.argv[(int)i];
		assert(argv);
		switch (argv->arg) {
		case (MDOC_Bullet):
			/* FALLTHROUGH */
		case (MDOC_Dash):
			/* FALLTHROUGH */
		case (MDOC_Enum):
			/* FALLTHROUGH */
		case (MDOC_Hyphen):
			/* FALLTHROUGH */
		case (MDOC_Item):
			/* FALLTHROUGH */
		case (MDOC_Tag):
			/* FALLTHROUGH */
		case (MDOC_Diag):
			/* FALLTHROUGH */
		case (MDOC_Hang):
			/* FALLTHROUGH */
		case (MDOC_Ohang):
			/* FALLTHROUGH */
		case (MDOC_Inset):
			if (type)
				err++;
			type++;
			break;
		default:
			break;
		}
	}
	if (0 == type)
		return(mdoc_err(mdoc, ERR_SYNTAX_ARGMISS));
	if (0 == err)
		return(1);
	assert(argv);
	return(mdoc_perr(mdoc, argv->line, 
			argv->pos, ERR_SYNTAX_ARGBAD));
}


static int
pre_bd(struct mdoc *mdoc, struct mdoc_node *node)
{
	int		 type, err;
	struct mdoc_arg	*argv;
	size_t		 i, argc;

	if (MDOC_BLOCK != node->type)
		return(1);
	assert(MDOC_Bd == node->data.block.tok);

	argv = NULL;
	argc = node->data.block.argc;

	for (err = i = type = 0; 0 == err && i < argc; i++) {
		argv = &node->data.block.argv[(int)i];
		assert(argv);
		switch (argv->arg) {
		case (MDOC_Ragged):
			/* FALLTHROUGH */
		case (MDOC_Unfilled):
			/* FALLTHROUGH */
		case (MDOC_Literal):
			/* FALLTHROUGH */
		case (MDOC_File):
			if (type)
				err++;
			type++;
			break;
		default:
			break;
		}
	}
	if (0 == type)
		return(mdoc_err(mdoc, ERR_SYNTAX_ARGMISS));
	if (0 == err)
		return(1);
	assert(argv);
	return(mdoc_perr(mdoc, argv->line, 
			argv->pos, ERR_SYNTAX_ARGBAD));
}


static int
pre_prologue(struct mdoc *mdoc, struct mdoc_node *node)
{

	if (SEC_PROLOGUE != mdoc->sec_lastn)
		return(mdoc_verr(mdoc, node, ERR_SEC_NPROLOGUE));
	assert(MDOC_ELEM == node->type);

	/* Check for ordering. */

	switch (node->data.elem.tok) {
	case (MDOC_Os):
		if (mdoc->meta.title[0] && mdoc->meta.date)
			break;
		return(mdoc_verr(mdoc, node, ERR_SEC_PROLOGUE_OO));
	case (MDOC_Dt):
		if (0 == mdoc->meta.title[0] && mdoc->meta.date)
			break;
		return(mdoc_verr(mdoc, node, ERR_SEC_PROLOGUE_OO));
	case (MDOC_Dd):
		if (0 == mdoc->meta.title[0] && 0 == mdoc->meta.date)
			break;
		return(mdoc_verr(mdoc, node, ERR_SEC_PROLOGUE_OO));
	default:
		abort();
		/* NOTREACHED */
	}

	/* Check for repetition. */

	switch (node->data.elem.tok) {
	case (MDOC_Os):
		if (0 == mdoc->meta.os[0])
			return(1);
		break;
	case (MDOC_Dd):
		if (0 == mdoc->meta.date)
			return(1);
		break;
	case (MDOC_Dt):
		if (0 == mdoc->meta.title[0])
			return(1);
		break;
	default:
		abort();
		/* NOTREACHED */
	}

	return(mdoc_verr(mdoc, node, ERR_SEC_PROLOGUE_REP));
}


static int
post_bl(struct mdoc *mdoc)
{
	struct mdoc_node *n;

	if (MDOC_BODY != mdoc->last->type)
		return(1);
	assert(MDOC_Bl == mdoc->last->data.body.tok);

	for (n = mdoc->last->child; n; n = n->next) {
		if (MDOC_BLOCK == n->type) 
			if (MDOC_It == n->data.block.tok)
				continue;
		break;
	}
	if (NULL == n)
		return(1);
	return(mdoc_verr(mdoc, n, ERR_SYNTAX_CHILDBAD));
}


/*
 * Warn if sections (those that are with a known title, such as NAME,
 * DESCRIPTION, and so forth) are out of the conventional order.
 */
static int
post_sh(struct mdoc *mdoc)
{
	enum mdoc_sec	  sec;
	int		  i;
	struct mdoc_node *n;
	char		 *args[MDOC_LINEARG_MAX];

	if (MDOC_HEAD != mdoc->last->type)
		return(1);
	
	assert(MDOC_Sh == mdoc->last->data.head.tok);

	n = mdoc->last->child;
	assert(n);

	for (i = 0; n && i < MDOC_LINEARG_MAX; n = n->next, i++) {
		assert(MDOC_TEXT == n->type);
		assert(NULL == n->child);
		assert(n->data.text.string);
		args[i] = n->data.text.string;
	}

	sec = mdoc_atosec((size_t)i, (const char **)args);
	if (SEC_CUSTOM == sec)
		return(1);
	if (sec > mdoc->sec_lastn)
		return(1);

	if (sec == mdoc->sec_lastn)
		return(mdoc_warn(mdoc, WARN_SEC_REP));
	return(mdoc_warn(mdoc, WARN_SEC_OO));
}


int
mdoc_valid_pre(struct mdoc *mdoc, struct mdoc_node *node)
{
	v_pre		*p;
	int		 t;

	switch (node->type) {
	case (MDOC_BODY):
		t = node->data.body.tok;
		break;
	case (MDOC_ELEM):
		t = node->data.elem.tok;
		break;
	case (MDOC_BLOCK):
		t = node->data.block.tok;
		break;
	case (MDOC_HEAD):
		t = node->data.head.tok;
		break;
	default:
		return(1);
	}

	if (NULL == mdoc_valids[t].pre)
		return(1);
	for (p = mdoc_valids[t].pre; *p; p++)
		if ( ! (*p)(mdoc, node)) 
			return(0);
	return(1);
}


int
mdoc_valid_post(struct mdoc *mdoc)
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
		if ( ! (*p)(mdoc)) 
			return(0);

	return(1);
}

