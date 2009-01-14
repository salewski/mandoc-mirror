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
static	int	headchild_warn_ge1(struct mdoc *);
static	int	headchild_err_eq0(struct mdoc *);
static	int	elemchild_err_eq0(struct mdoc *);
static	int	elemchild_err_ge1(struct mdoc *);
static	int	elemchild_warn_eq0(struct mdoc *);
static	int	bodychild_warn_ge1(struct mdoc *);
static	int	bodychild_err_eq0(struct mdoc *);
static	int	elemchild_warn_ge1(struct mdoc *);
static	int	post_sh(struct mdoc *);
static	int	post_bl(struct mdoc *);
static	int	post_it(struct mdoc *);

static	v_pre	pres_prologue[] = { pre_prologue, NULL };
static	v_pre	pres_d1[] = { pre_display, NULL };
static	v_pre	pres_bd[] = { pre_display, pre_bd, NULL };
static	v_pre	pres_bl[] = { pre_bl, NULL };
static	v_pre	pres_it[] = { pre_it, NULL };

static	v_post	posts_bd[] = { headchild_err_eq0, bodychild_warn_ge1, NULL };
static	v_post	posts_text[] = { elemchild_err_ge1, NULL };
static	v_post	posts_wtext[] = { elemchild_warn_ge1, NULL };
static	v_post	posts_notext[] = { elemchild_err_eq0, NULL };
static	v_post	posts_wline[] = { headchild_warn_ge1, bodychild_err_eq0, NULL };
static	v_post	posts_sh[] = { headchild_err_ge1, bodychild_warn_ge1, post_sh, NULL };
static	v_post	posts_bl[] = { headchild_err_eq0, bodychild_warn_ge1, post_bl, NULL };
static	v_post	posts_it[] = { post_it, NULL };
static	v_post	posts_ss[] = { headchild_err_ge1, NULL };
static	v_post	posts_pp[] = { elemchild_warn_eq0, NULL };
static	v_post	posts_d1[] = { headchild_err_ge1, NULL };


const	struct valids mdoc_valids[MDOC_MAX] = {
	{ NULL, NULL }, /* \" */
	{ pres_prologue, posts_text }, /* Dd */
	{ pres_prologue, NULL }, /* Dt */
	{ pres_prologue, NULL }, /* Os */
	/* FIXME: preceding Pp. */ 
	/* FIXME: NAME section internal ordering. */
	/* FIXME: can only be a child of root. */
	{ NULL, posts_sh }, /* Sh */ 
	/* FIXME: preceding Pp. */
	/* FIXME: can only be a child of Sh. */
	{ NULL, posts_ss }, /* Ss */ 
	/* FIXME: proceeding... */
	{ NULL, posts_pp }, /* Pp */ 
	{ pres_d1, posts_d1 }, /* D1 */
	{ pres_d1, posts_d1 }, /* Dl */
	 /* FIXME: preceding Pp. */
	{ pres_bd, posts_bd }, /* Bd */
	{ NULL, NULL }, /* Ed */
	/* FIXME: preceding Pp. */
	{ pres_bl, posts_bl }, /* Bl */ 
	{ NULL, NULL }, /* El */
	{ pres_it, posts_it }, /* It */
	{ NULL, posts_text }, /* Ad */ 
	/* FIXME */
	{ NULL, NULL }, /* An */ 
	{ NULL, NULL }, /* Ar */

	{ NULL, posts_text }, /* Cd */ /* FIXME: section 4 only. */
	{ NULL, NULL }, /* Cm */
	{ NULL, posts_text }, /* Dv */ 
	{ NULL, posts_text }, /* Er */ /* FIXME: section 2 only. */
	{ NULL, posts_text }, /* Ev */ 
	{ NULL, posts_notext }, /* Ex */ /* FIXME: sections 1,6,8 only. */ /* -std required */
	{ NULL, posts_text }, /* Fa */ 
	{ NULL, NULL }, /* Fd */ /* FIXME: SYNOPSIS section. */
	{ NULL, NULL }, /* Fl */
	{ NULL, posts_text }, /* Fn */ 
	{ NULL, NULL }, /* Ft */ 
	{ NULL, posts_text }, /* Ic */ 
	{ NULL, posts_wtext }, /* In */ 
	{ NULL, posts_text }, /* Li */
	{ NULL, posts_wtext }, /* Nd */
	{ NULL, NULL }, /* Nm */  /* FIXME: If name not set? */
	{ NULL, posts_wline }, /* Op */
	{ NULL, NULL }, /* Ot */
	{ NULL, NULL }, /* Pa */
	{ NULL, posts_notext }, /* Rv */ /* -std required */
	{ NULL, posts_notext }, /* St */ /* arg required */
	{ NULL, posts_text }, /* Va */
	{ NULL, posts_text }, /* Vt */ 
	{ NULL, NULL }, /* Xr */ /* FIXME */
	{ NULL, posts_text }, /* %A */
	{ NULL, posts_text }, /* %B */
	{ NULL, posts_text }, /* %D */
	{ NULL, posts_text }, /* %I */
	{ NULL, posts_text }, /* %J */
	{ NULL, posts_text }, /* %N */
	{ NULL, posts_text }, /* %O */
	{ NULL, posts_text }, /* %P */
	{ NULL, posts_text }, /* %R */
	{ NULL, posts_text }, /* %T */
	{ NULL, posts_text }, /* %V */
	{ NULL, NULL }, /* Ac */
	{ NULL, NULL }, /* Ao */
	{ NULL, posts_wline }, /* Aq */
	{ NULL, NULL }, /* At */ /* FIXME */
	{ NULL, NULL }, /* Bc */
	{ NULL, NULL }, /* Bf */ 
	{ NULL, NULL }, /* Bo */
	{ NULL, posts_wline }, /* Bq */
	{ NULL, NULL }, /* Bsx */
	{ NULL, NULL }, /* Bx */
	{ NULL, NULL }, /* Db */ /* FIXME: boolean */
	{ NULL, NULL }, /* Dc */
	{ NULL, NULL }, /* Do */
	{ NULL, posts_wline }, /* Dq */
	{ NULL, NULL }, /* Ec */
	{ NULL, NULL }, /* Ef */ /* -symbolic, etc. */
	{ NULL, posts_text }, /* Em */ 
	{ NULL, NULL }, /* Eo */
	{ NULL, NULL }, /* Fx */
	{ NULL, posts_text }, /* Ms */ /* FIXME: which symbols? */
	{ NULL, posts_notext }, /* No */
	{ NULL, posts_notext }, /* Ns */
	{ NULL, NULL }, /* Nx */
	{ NULL, NULL }, /* Ox */
	{ NULL, NULL }, /* Pc */
	{ NULL, NULL }, /* Pf */ /* FIXME: 2 or more arguments */
	{ NULL, NULL }, /* Po */
	{ NULL, posts_wline }, /* Pq */ /* FIXME: ignore following Sh/Ss */
	{ NULL, NULL }, /* Qc */
	{ NULL, posts_wline }, /* Ql */
	{ NULL, NULL }, /* Qo */
	{ NULL, posts_wline }, /* Qq */
	{ NULL, NULL }, /* Re */
	{ NULL, NULL }, /* Rs */
	{ NULL, NULL }, /* Sc */
	{ NULL, NULL }, /* So */
	{ NULL, posts_wline }, /* Sq */
	{ NULL, NULL }, /* Sm */ /* FIXME: boolean */
	{ NULL, posts_text }, /* Sx */
	{ NULL, posts_text }, /* Sy */
	{ NULL, posts_text }, /* Tn */
	{ NULL, NULL }, /* Ux */
	{ NULL, NULL }, /* Xc */
	{ NULL, NULL }, /* Xo */
	{ NULL, NULL }, /* Fo */ 
	{ NULL, NULL }, /* Fc */ 
	{ NULL, NULL }, /* Oo */
	{ NULL, NULL }, /* Oc */
	{ NULL, NULL }, /* Bk */
	{ NULL, NULL }, /* Ek */
	{ NULL, posts_notext }, /* Bt */
	{ NULL, NULL }, /* Hf */
	{ NULL, NULL }, /* Fr */
	{ NULL, posts_notext }, /* Ud */
};


static int
bodychild_err_eq0(struct mdoc *mdoc)
{

	if (MDOC_BODY != mdoc->last->type)
		return(1);
	if (NULL == mdoc->last->child)
		return(1);
	return(mdoc_warn(mdoc, WARN_ARGS_EQ0));
}


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
elemchild_warn_ge1(struct mdoc *mdoc)
{

	assert(MDOC_ELEM == mdoc->last->type);
	if (mdoc->last->child)
		return(1);
	return(mdoc_warn(mdoc, WARN_ARGS_GE1));
}


static int
elemchild_err_eq0(struct mdoc *mdoc)
{

	assert(MDOC_ELEM == mdoc->last->type);
	if (NULL == mdoc->last->child)
		return(1);
	return(mdoc_err(mdoc, ERR_ARGS_EQ0));
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
headchild_warn_ge1(struct mdoc *mdoc)
{

	if (MDOC_HEAD != mdoc->last->type)
		return(1);
	if (mdoc->last->child)
		return(1);
	return(mdoc_warn(mdoc, WARN_ARGS_GE1));
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
			/* FALLTHROUGH */
		case (MDOC_Column):
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
	
	/* Some types require block-head, some not. */

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

	if (MDOC_Column != sv) 
		return(1);

	/* Make sure the number of columns is sane. */

	sv = mdoc->last->parent->parent->data.block.argv->sz;
	n = mdoc->last->data.block.head->child;

	for (i = 0; n; n = n->next)
		i++;

	if (i == (size_t)sv)
		return(1);
	return(mdoc_err(mdoc, ERR_SYNTAX_ARGFORM));

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

