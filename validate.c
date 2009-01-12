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
static	int	pre_it(struct mdoc *, struct mdoc_node *);
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
static	int	post_it(struct mdoc *);

static	v_pre	pres_prologue[] = { pre_prologue, NULL };
static	v_pre	pres_d1[] = { pre_display, NULL };
static	v_pre	pres_bd[] = { pre_display, pre_bd, NULL };
static	v_pre	pres_bl[] = { pre_bl, NULL };
static	v_pre	pres_it[] = { pre_it, NULL };
static	v_post	posts_bd[] = { headchild_err_eq0, 
			bodychild_warn_ge1, NULL };

static	v_post	posts_sh[] = { headchild_err_ge1, 
			bodychild_warn_ge1, post_sh, NULL };
static	v_post	posts_bl[] = { headchild_err_eq0, 
			bodychild_warn_ge1, post_bl, NULL };
static	v_post	posts_it[] = { post_it, NULL };
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
	{ pres_it, posts_it }, /* It */
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
			if (MDOC_Bd == n->tok)
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
	assert(MDOC_Bl == node->tok);

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
	assert(MDOC_Bd == node->tok);

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
pre_it(struct mdoc *mdoc, struct mdoc_node *node)
{

	if (MDOC_BLOCK != mdoc->last->type)
		return(1);
	assert(MDOC_It == mdoc->last->tok);

	if (MDOC_BODY != mdoc->last->parent->type) 
		return(mdoc_verr(mdoc, node, ERR_SYNTAX_PARENTBAD));
	if (MDOC_Bl != mdoc->last->parent->tok)
		return(mdoc_verr(mdoc, node, ERR_SYNTAX_PARENTBAD));

	return(1);
}


static int
pre_prologue(struct mdoc *mdoc, struct mdoc_node *node)
{

	if (SEC_PROLOGUE != mdoc->sec_lastn)
		return(mdoc_verr(mdoc, node, ERR_SEC_NPROLOGUE));
	assert(MDOC_ELEM == node->type);

	/* Check for ordering. */

	switch (node->tok) {
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

	switch (node->tok) {
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


/* Warn if `Bl' type-specific syntax isn't reflected in items. */
static int
post_it(struct mdoc *mdoc)
{
	int		  type, sv;
#define	TYPE_NONE	 (0)
#define	TYPE_BODY	 (1)
#define	TYPE_HEAD	 (2)
	size_t		  i, argc;
	struct mdoc_node *n;

	if (MDOC_BLOCK != mdoc->last->type)
		return(1);

	assert(MDOC_It == mdoc->last->tok);

	n = mdoc->last->parent;
	assert(n);
	assert(MDOC_Bl == n->tok);

	n = n->parent;
	assert(MDOC_BLOCK == n->type);
	assert(MDOC_Bl == n->tok);

	argc = n->data.block.argc;
	type = TYPE_NONE;

	for (i = 0; TYPE_NONE == type && i < argc; i++)
		switch (n->data.block.argv[(int)i].arg) {
		case (MDOC_Tag):
			/* FALLTHROUGH */
		case (MDOC_Diag):
			/* FALLTHROUGH */
		case (MDOC_Hang):
			/* FALLTHROUGH */
		case (MDOC_Ohang):
			/* FALLTHROUGH */
		case (MDOC_Inset):
			type = TYPE_HEAD;
			sv = n->data.block.argv[(int)i].arg;
			break;
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
		case (MDOC_Column):
			type = TYPE_BODY;
			sv = n->data.block.argv[(int)i].arg;
			break;
		default:
			break;
		}

	assert(TYPE_NONE != type);

	if (TYPE_HEAD == type) {
		if (NULL == (n = mdoc->last->data.block.head)) {
			if ( ! mdoc_warn(mdoc, WARN_SYNTAX_EMPTYHEAD))
				return(0);
		} else if (NULL == n->child)
			if ( ! mdoc_warn(mdoc, WARN_SYNTAX_EMPTYHEAD))
				return(0);

		if (NULL == (n = mdoc->last->data.block.body)) {
			if ( ! mdoc_warn(mdoc, WARN_SYNTAX_EMPTYBODY))
				return(0);
		} else if (NULL == n->child)
			if ( ! mdoc_warn(mdoc, WARN_SYNTAX_EMPTYBODY))
				return(0);

		return(1);
	}

	if (NULL == (n = mdoc->last->data.block.head)) {
		if ( ! mdoc_warn(mdoc, WARN_SYNTAX_EMPTYHEAD))
			return(0);
	} else if (NULL == n->child)
		if ( ! mdoc_warn(mdoc, WARN_SYNTAX_EMPTYHEAD))
			return(0);

	if ((n = mdoc->last->data.block.body) && n->child)
		if ( ! mdoc_warn(mdoc, WARN_SYNTAX_NOBODY))
			return(0);

	/* TODO: make sure columns are aligned. */
	assert(MDOC_Column != sv);

	return(1);

#undef	TYPE_NONE
#undef	TYPE_BODY
#undef	TYPE_HEAD
}


/* Make sure that only `It' macros are our body-children. */
static int
post_bl(struct mdoc *mdoc)
{
	struct mdoc_node *n;

	if (MDOC_BODY != mdoc->last->type)
		return(1);
	assert(MDOC_Bl == mdoc->last->tok);

	for (n = mdoc->last->child; n; n = n->next) {
		if (MDOC_BLOCK == n->type) 
			if (MDOC_It == n->tok)
				continue;
		break;
	}
	if (NULL == n)
		return(1);
	return(mdoc_verr(mdoc, n, ERR_SYNTAX_CHILDBAD));
}


/* Warn if conventional sections are out of order. */
static int
post_sh(struct mdoc *mdoc)
{
	enum mdoc_sec	  sec;
	int		  i;
	struct mdoc_node *n;
	char		 *args[MDOC_LINEARG_MAX];

	if (MDOC_HEAD != mdoc->last->type)
		return(1);
	
	assert(MDOC_Sh == mdoc->last->tok);

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

	/* TODO: character-escape checks. */

	if (MDOC_TEXT == node->type)
		return(1);
	assert(MDOC_ROOT != node->type);

	if (NULL == mdoc_valids[node->tok].pre)
		return(1);
	for (p = mdoc_valids[node->tok].pre; *p; p++)
		if ( ! (*p)(mdoc, node)) 
			return(0);
	return(1);
}


int
mdoc_valid_post(struct mdoc *mdoc)
{
	v_post		*p;

	if (MDOC_TEXT == mdoc->last->type)
		return(1);
	if (MDOC_ROOT == mdoc->last->type)
		return(1);

	if (NULL == mdoc_valids[mdoc->last->tok].post)
		return(1);
	for (p = mdoc_valids[mdoc->last->tok].post; *p; p++)
		if ( ! (*p)(mdoc)) 
			return(0);

	return(1);
}

