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

/* FIXME: some sections should only occur in specific msecs. */
/* FIXME: ignoring Pp. */
/* FIXME: math symbols. */
/* FIXME: make sure prologue is complete. */
/* FIXME: valid character-escape checks. */
/* FIXME: make sure required sections are included (NAME, ...). */

struct	valids {
	v_pre	*pre;
	v_post	*post;
};

/* Utility checks. */

static	int	pre_check_parent(struct mdoc *, struct mdoc_node *, 
			int, enum mdoc_type);
static	int	pre_check_msecs(struct mdoc *, struct mdoc_node *, 
			int, enum mdoc_msec *);
static	int	pre_check_stdarg(struct mdoc *, struct mdoc_node *);
static	int	post_check_children_count(struct mdoc *);
static	int	post_check_children_lt(struct mdoc *, const char *, int);
static	int	post_check_children_gt(struct mdoc *, const char *, int);
static	int	post_check_children_wgt(struct mdoc *, const char *, int);
static	int	post_check_children_eq(struct mdoc *, const char *, int);
static	int	post_check_children_weq(struct mdoc *, const char *, int);

/* Specific pre-child-parse routines. */

static	int	pre_display(struct mdoc *, struct mdoc_node *);
static	int	pre_sh(struct mdoc *, struct mdoc_node *);
static	int	pre_ss(struct mdoc *, struct mdoc_node *);
static	int	pre_bd(struct mdoc *, struct mdoc_node *);
static	int	pre_bl(struct mdoc *, struct mdoc_node *);
static	int	pre_it(struct mdoc *, struct mdoc_node *);
static	int	pre_cd(struct mdoc *, struct mdoc_node *);
static	int	pre_er(struct mdoc *, struct mdoc_node *);
static	int	pre_ex(struct mdoc *, struct mdoc_node *);
static	int	pre_rv(struct mdoc *, struct mdoc_node *);
static	int	pre_an(struct mdoc *, struct mdoc_node *);
static	int	pre_st(struct mdoc *, struct mdoc_node *);
static	int	pre_prologue(struct mdoc *, struct mdoc_node *);
static	int	pre_prologue(struct mdoc *, struct mdoc_node *);
static	int	pre_prologue(struct mdoc *, struct mdoc_node *);

/* Specific post-child-parse routines. */

static	int	herr_ge1(struct mdoc *);
static	int	herr_le1(struct mdoc *);
static	int	hwarn_ge1(struct mdoc *);
static	int	herr_eq0(struct mdoc *);
static	int	eerr_eq0(struct mdoc *);
static	int	eerr_le1(struct mdoc *);
static	int	eerr_le2(struct mdoc *);
static	int	eerr_eq1(struct mdoc *);
static	int	eerr_ge1(struct mdoc *);
static	int	ewarn_eq0(struct mdoc *);
static	int	ewarn_eq1(struct mdoc *);
static	int	bwarn_ge1(struct mdoc *);
static	int	berr_eq0(struct mdoc *);
static	int	ewarn_ge1(struct mdoc *);
static	int	ebool(struct mdoc *);
static	int	post_sh(struct mdoc *);
static	int	post_bl(struct mdoc *);
static	int	post_it(struct mdoc *);
static	int	post_ex(struct mdoc *);
static	int	post_an(struct mdoc *);
static	int	post_at(struct mdoc *);
static	int	post_xr(struct mdoc *);
static	int	post_nm(struct mdoc *);
static	int	post_bf(struct mdoc *);
static	int	post_root(struct mdoc *);

/* Collections of pre-child-parse routines. */

static	v_pre	pres_prologue[] = { pre_prologue, NULL };
static	v_pre	pres_d1[] = { pre_display, NULL };
static	v_pre	pres_bd[] = { pre_display, pre_bd, NULL };
static	v_pre	pres_bl[] = { pre_bl, NULL };
static	v_pre	pres_it[] = { pre_it, NULL };
static	v_pre	pres_ss[] = { pre_ss, NULL };
static	v_pre	pres_sh[] = { pre_sh, NULL };
static	v_pre	pres_cd[] = { pre_cd, NULL };
static	v_pre	pres_er[] = { pre_er, NULL };
static	v_pre	pres_ex[] = { pre_ex, NULL };
static	v_pre	pres_rv[] = { pre_rv, NULL };
static	v_pre	pres_an[] = { pre_an, NULL };
static	v_pre	pres_st[] = { pre_st, NULL };

/* Collections of post-child-parse routines. */

static	v_post	posts_bool[] = { eerr_eq1, ebool, NULL };
static	v_post	posts_bd[] = { herr_eq0, bwarn_ge1, NULL };
static	v_post	posts_text[] = { eerr_ge1, NULL };
static	v_post	posts_wtext[] = { ewarn_ge1, NULL };
static	v_post	posts_notext[] = { eerr_eq0, NULL };
static	v_post	posts_wline[] = { hwarn_ge1, berr_eq0, NULL };
static	v_post	posts_sh[] = { herr_ge1, bwarn_ge1, post_sh, NULL };
static	v_post	posts_bl[] = { herr_eq0, bwarn_ge1, post_bl, NULL };
static	v_post	posts_it[] = { post_it, NULL };
static	v_post	posts_in[] = { ewarn_eq1, NULL };
static	v_post	posts_ss[] = { herr_ge1, NULL };
static	v_post	posts_pp[] = { ewarn_eq0, NULL };
static	v_post	posts_d1[] = { herr_ge1, NULL };
static	v_post	posts_ex[] = { eerr_le1, post_ex, NULL };
static	v_post	posts_an[] = { post_an, NULL };
static	v_post	posts_at[] = { post_at, NULL };
static	v_post	posts_xr[] = { eerr_ge1, eerr_le2, post_xr, NULL };
static	v_post	posts_nm[] = { post_nm, NULL };
static	v_post	posts_bf[] = { herr_le1, post_bf, NULL };

/* Per-macro pre- and post-child-check routine collections. */

const	struct valids mdoc_valids[MDOC_MAX] = {
	{ NULL, NULL }, /* \" */
	{ pres_prologue, posts_text }, /* Dd */
	{ pres_prologue, NULL }, /* Dt */
	{ pres_prologue, NULL }, /* Os */
	/* FIXME: NAME section internal ordering. */
	{ pres_sh, posts_sh }, /* Sh */ 
	{ pres_ss, posts_ss }, /* Ss */ 
	{ NULL, posts_pp }, /* Pp */ 
	{ pres_d1, posts_d1 }, /* D1 */
	{ pres_d1, posts_d1 }, /* Dl */
	{ pres_bd, posts_bd }, /* Bd */
	{ NULL, NULL }, /* Ed */
	{ pres_bl, posts_bl }, /* Bl */ 
	{ NULL, NULL }, /* El */
	{ pres_it, posts_it }, /* It */
	{ NULL, posts_text }, /* Ad */ 
	{ pres_an, posts_an }, /* An */ 
	{ NULL, NULL }, /* Ar */
	{ pres_cd, posts_text }, /* Cd */ 
	{ NULL, NULL }, /* Cm */
	{ NULL, posts_text }, /* Dv */ 
	{ pres_er, posts_text }, /* Er */ 
	{ NULL, posts_text }, /* Ev */ 
	{ pres_ex, posts_ex }, /* Ex */ 
	{ NULL, posts_text }, /* Fa */ 
	/* FIXME: only in SYNOPSIS section. */
	{ NULL, posts_wtext }, /* Fd */
	{ NULL, NULL }, /* Fl */
	{ NULL, posts_text }, /* Fn */ 
	{ NULL, posts_wtext }, /* Ft */ 
	{ NULL, posts_text }, /* Ic */ 
	{ NULL, posts_in }, /* In */ 
	{ NULL, posts_text }, /* Li */
	{ NULL, posts_wtext }, /* Nd */
	{ NULL, posts_nm }, /* Nm */
	{ NULL, posts_wline }, /* Op */
	{ NULL, NULL }, /* Ot */
	{ NULL, NULL }, /* Pa */
	{ pres_rv, posts_notext }, /* Rv */
	{ pres_st, posts_notext }, /* St */ 
	{ NULL, posts_text }, /* Va */
	{ NULL, posts_text }, /* Vt */ 
	{ NULL, posts_xr }, /* Xr */ 
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
	{ NULL, posts_at }, /* At */ 
	{ NULL, NULL }, /* Bc */
	{ NULL, posts_bf }, /* Bf */  /* FIXME */
	{ NULL, NULL }, /* Bo */
	{ NULL, posts_wline }, /* Bq */
	{ NULL, NULL }, /* Bsx */
	{ NULL, NULL }, /* Bx */
	{ NULL, posts_bool }, /* Db */
	{ NULL, NULL }, /* Dc */
	{ NULL, NULL }, /* Do */
	{ NULL, posts_wline }, /* Dq */
	{ NULL, NULL }, /* Ec */
	{ NULL, NULL }, /* Ef */ /* -symbolic, etc. */
	{ NULL, posts_text }, /* Em */ 
	{ NULL, NULL }, /* Eo */
	{ NULL, NULL }, /* Fx */
	{ NULL, posts_text }, /* Ms */ 
	{ NULL, posts_notext }, /* No */
	{ NULL, posts_notext }, /* Ns */
	{ NULL, NULL }, /* Nx */
	{ NULL, NULL }, /* Ox */
	{ NULL, NULL }, /* Pc */
	{ NULL, NULL }, /* Pf */
	{ NULL, NULL }, /* Po */
	{ NULL, posts_wline }, /* Pq */
	{ NULL, NULL }, /* Qc */
	{ NULL, posts_wline }, /* Ql */
	{ NULL, NULL }, /* Qo */
	{ NULL, posts_wline }, /* Qq */
	{ NULL, NULL }, /* Re */
	{ NULL, NULL }, /* Rs */
	{ NULL, NULL }, /* Sc */
	{ NULL, NULL }, /* So */
	{ NULL, posts_wline }, /* Sq */
	{ NULL, posts_bool }, /* Sm */ 
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
post_check_children_count(struct mdoc *mdoc)
{
	struct mdoc_node *n;
	int		  i;

	for (i = 0, n = mdoc->last->child; n; n = n->next, i++)
		/* Do nothing */ ;
	return(i);
}


static int
post_check_children_wgt(struct mdoc *mdoc, const char *p, int sz)
{
	int		  i;

	if ((i = post_check_children_count(mdoc)) > sz)
		return(1);
	return(mdoc_warn(mdoc, WARN_SYNTAX, "macro suggests more "
				"than %d %s (has %d)", sz, p, i));
}


static int
post_check_children_gt(struct mdoc *mdoc, const char *p, int sz)
{
	int		  i;

	if ((i = post_check_children_count(mdoc)) > sz)
		return(1);
	return(mdoc_err(mdoc, "macro requires more than %d "
				"%s (has %d)", sz, p, i));
}


static int
post_check_children_weq(struct mdoc *mdoc, const char *p, int sz)
{
	int		  i;

	if ((i = post_check_children_count(mdoc)) == sz)
		return(1);
	return(mdoc_warn(mdoc, WARN_SYNTAX, "macro suggests %d "
				"%s (has %d)", sz, p, i));
}


static int
post_check_children_eq(struct mdoc *mdoc, const char *p, int sz)
{
	int		  i;

	if ((i = post_check_children_count(mdoc)) == sz)
		return(1);
	return(mdoc_err(mdoc, "macro requires %d %s "
				"(have %d)", sz, p, i));
}


static int
post_check_children_lt(struct mdoc *mdoc, const char *p, int sz)
{
	int		  i;

	if ((i = post_check_children_count(mdoc)) < sz)
		return(1);
	return(mdoc_err(mdoc, "macro requires less than %d "
				"%s (have %d)", sz, p, i));
}


static int
pre_check_stdarg(struct mdoc *mdoc, struct mdoc_node *node)
{

	if (1 == node->data.elem.argc &&
			MDOC_Std == node->data.elem.argv[0].arg)
		return(1);
	return(mdoc_nwarn(mdoc, node, WARN_COMPAT, 
				"macro suggests single `%s' argument",
				mdoc_argnames[MDOC_Std]));
}


static int
pre_check_msecs(struct mdoc *mdoc, struct mdoc_node *node, 
		int sz, enum mdoc_msec *msecs)
{
	int		 i;

	for (i = 0; i < sz; i++)
		if (msecs[i] == mdoc->meta.msec)
			return(1);
	return(mdoc_nwarn(mdoc, node, WARN_COMPAT, "macro not "
				"appropriate for manual section"));
}


static int
pre_check_parent(struct mdoc *mdoc, struct mdoc_node *node, 
		int tok, enum mdoc_type type)
{

	if (type != node->parent->type) 
		return(mdoc_nerr(mdoc, node, "invalid macro parent class %s, expected %s", 
					mdoc_type2a(node->parent->type),
					mdoc_type2a(type)));
	if (MDOC_ROOT != type && tok != node->parent->tok)
		return(mdoc_nerr(mdoc, node, "invalid macro parent `%s', expected `%s'", 
					mdoc_macronames[node->parent->tok],
					mdoc_macronames[tok]));
	return(1);
}


static int
berr_eq0(struct mdoc *mdoc)
{

	if (MDOC_BODY != mdoc->last->type)
		return(1);
	return(post_check_children_eq(mdoc, "body children", 0));
}


static int
bwarn_ge1(struct mdoc *mdoc)
{

	if (MDOC_BODY != mdoc->last->type)
		return(1);
	return(post_check_children_wgt(mdoc, "body children", 0));
}


static int
ewarn_eq1(struct mdoc *mdoc)
{

	assert(MDOC_ELEM == mdoc->last->type);
	return(post_check_children_weq(mdoc, "parameters", 1));
}


static int
ewarn_eq0(struct mdoc *mdoc)
{

	assert(MDOC_ELEM == mdoc->last->type);
	return(post_check_children_weq(mdoc, "parameters", 0));
}


static int
ewarn_ge1(struct mdoc *mdoc)
{

	assert(MDOC_ELEM == mdoc->last->type);
	return(post_check_children_wgt(mdoc, "parameters", 0));
}


static int
eerr_eq1(struct mdoc *mdoc)
{

	assert(MDOC_ELEM == mdoc->last->type);
	return(post_check_children_eq(mdoc, "parameters", 1));
}


static int
eerr_le2(struct mdoc *mdoc)
{

	assert(MDOC_ELEM == mdoc->last->type);
	return(post_check_children_lt(mdoc, "parameters", 3));
}


static int
eerr_le1(struct mdoc *mdoc)
{

	assert(MDOC_ELEM == mdoc->last->type);
	return(post_check_children_lt(mdoc, "parameters", 2));
}


static int
eerr_eq0(struct mdoc *mdoc)
{

	assert(MDOC_ELEM == mdoc->last->type);
	return(post_check_children_eq(mdoc, "parameters", 0));
}


static int
eerr_ge1(struct mdoc *mdoc)
{

	assert(MDOC_ELEM == mdoc->last->type);
	return(post_check_children_gt(mdoc, "parameters", 0));
}


static int
herr_eq0(struct mdoc *mdoc)
{

	if (MDOC_HEAD != mdoc->last->type)
		return(1);
	return(post_check_children_eq(mdoc, "parameters", 0));
}


static int
hwarn_ge1(struct mdoc *mdoc)
{

	if (MDOC_HEAD != mdoc->last->type)
		return(1);
	return(post_check_children_wgt(mdoc, "parameters", 0));
}


static int
herr_le1(struct mdoc *mdoc)
{
	if (MDOC_HEAD != mdoc->last->type)
		return(1);
	return(post_check_children_lt(mdoc, "parameters", 2));
}


static int
herr_ge1(struct mdoc *mdoc)
{

	if (MDOC_HEAD != mdoc->last->type)
		return(1);
	return(post_check_children_gt(mdoc, "parameters", 0));
}


static int
pre_display(struct mdoc *mdoc, struct mdoc_node *node)
{
	struct mdoc_node *n;

	if (MDOC_BLOCK != node->type)
		return(1);

	assert(mdoc->last);
	/* LINTED */
	for (n = mdoc->last->parent; n; n = n->parent) 
		if (MDOC_BLOCK == n->type)
			if (MDOC_Bd == n->tok)
				break;
	if (NULL == n)
		return(1);
	return(mdoc_nerr(mdoc, node, "displays may not be nested"));
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

	/* LINTED */
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
		return(mdoc_err(mdoc, "no list type specified"));
	if (0 == err)
		return(1);
	assert(argv);
	return(mdoc_perr(mdoc, argv->line, 
			argv->pos, "only one list type possible"));
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

	/* LINTED */
	for (err = i = type = 0; 0 == err && i < argc; i++) {
		argv = &node->data.block.argv[(int)i];
		assert(argv);
		switch (argv->arg) {
		case (MDOC_Ragged):
			/* FALLTHROUGH */
		case (MDOC_Unfilled):
			/* FALLTHROUGH */
		case (MDOC_Filled):
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
		return(mdoc_err(mdoc, "no display type specified"));
	if (0 == err)
		return(1);
	assert(argv);
	return(mdoc_perr(mdoc, argv->line, 
			argv->pos, "only one display type possible"));
}


static int
pre_ss(struct mdoc *mdoc, struct mdoc_node *node)
{

	if (MDOC_BLOCK != node->type)
		return(1);
	return(pre_check_parent(mdoc, node, MDOC_Sh, MDOC_BODY));
}


static int
pre_sh(struct mdoc *mdoc, struct mdoc_node *node)
{

	if (MDOC_BLOCK != node->type)
		return(1);
	return(pre_check_parent(mdoc, node, -1, MDOC_ROOT));
}


static int
pre_st(struct mdoc *mdoc, struct mdoc_node *node)
{

	assert(MDOC_ELEM == node->type);
	assert(MDOC_St == node->tok);
	if (1 == node->data.elem.argc)
		return(1);
	return(mdoc_nerr(mdoc, node, "macro must have one argument"));
}


static int
pre_an(struct mdoc *mdoc, struct mdoc_node *node)
{

	assert(MDOC_ELEM == node->type);
	assert(MDOC_An == node->tok);
	if (1 >= node->data.elem.argc)
		return(1);
	return(mdoc_nerr(mdoc, node, "macro may only have one argument"));
}


static int
pre_rv(struct mdoc *mdoc, struct mdoc_node *node)
{
	enum mdoc_msec	 msecs[2];

	assert(MDOC_ELEM == node->type);
	assert(MDOC_Rv == node->tok);

	msecs[0] = MSEC_2;
	msecs[1] = MSEC_3;
	if ( ! pre_check_msecs(mdoc, node, 2, msecs))
		return(0);
	return(pre_check_stdarg(mdoc, node));
}


static int
pre_ex(struct mdoc *mdoc, struct mdoc_node *node)
{
	enum mdoc_msec	 msecs[3];

	assert(MDOC_ELEM == node->type);
	assert(MDOC_Ex == node->tok);

	msecs[0] = MSEC_1;
	msecs[1] = MSEC_6;
	msecs[2] = MSEC_8;
	if ( ! pre_check_msecs(mdoc, node, 3, msecs))
		return(0);
	return(pre_check_stdarg(mdoc, node));
}


static int
pre_er(struct mdoc *mdoc, struct mdoc_node *node)
{
	enum mdoc_msec	 msecs[1];

	msecs[0] = MSEC_2;
	return(pre_check_msecs(mdoc, node, 1, msecs));
}


static int
pre_cd(struct mdoc *mdoc, struct mdoc_node *node)
{
	enum mdoc_msec	 msecs[1];

	msecs[0] = MSEC_4;
	return(pre_check_msecs(mdoc, node, 1, msecs));
}


static int
pre_it(struct mdoc *mdoc, struct mdoc_node *node)
{

	if (MDOC_BLOCK != node->type)
		return(1);
	return(pre_check_parent(mdoc, node, MDOC_Bl, MDOC_BODY));
}


static int
pre_prologue(struct mdoc *mdoc, struct mdoc_node *node)
{

	if (SEC_PROLOGUE != mdoc->sec_lastn)
		return(mdoc_nerr(mdoc, node, "macro may only be invoked in the prologue"));
	assert(MDOC_ELEM == node->type);

	/* Check for ordering. */

	switch (node->tok) {
	case (MDOC_Os):
		if (mdoc->meta.title && mdoc->meta.date)
			break;
		return(mdoc_nerr(mdoc, node, "prologue macro out-of-order"));
	case (MDOC_Dt):
		if (NULL == mdoc->meta.title && mdoc->meta.date)
			break;
		return(mdoc_nerr(mdoc, node, "prologue macro out-of-order"));
	case (MDOC_Dd):
		if (NULL == mdoc->meta.title && 0 == mdoc->meta.date)
			break;
		return(mdoc_nerr(mdoc, node, "prologue macro out-of-order"));
	default:
		abort();
		/* NOTREACHED */
	}

	/* Check for repetition. */

	switch (node->tok) {
	case (MDOC_Os):
		if (NULL == mdoc->meta.os)
			return(1);
		break;
	case (MDOC_Dd):
		if (0 == mdoc->meta.date)
			return(1);
		break;
	case (MDOC_Dt):
		if (NULL == mdoc->meta.title)
			return(1);
		break;
	default:
		abort();
		/* NOTREACHED */
	}

	return(mdoc_nerr(mdoc, node, "prologue macro repeated"));
}


static int
post_bf(struct mdoc *mdoc)
{
	char		 *p;
	struct mdoc_node *head;

	if (MDOC_BLOCK != mdoc->last->type)
		return(1);
	assert(MDOC_Bf == mdoc->last->tok);
	head = mdoc->last->data.block.head;
	assert(head);

	if (0 == mdoc->last->data.block.argc) {
		if (head->child) {
			assert(MDOC_TEXT == head->child->type);
			p = head->child->data.text.string;
			if (xstrcmp(p, "Em"))
				return(1);
			else if (xstrcmp(p, "Li"))
				return(1);
			else if (xstrcmp(p, "Sm"))
				return(1);
			return(mdoc_nerr(mdoc, head->child, "invalid font mode"));
		}
		return(mdoc_err(mdoc, "macro expects an argument or parameter"));
	}
	if (head->child)
		return(mdoc_err(mdoc, "macro expects an argument or parameter"));
	if (1 == mdoc->last->data.block.argc)
		return(1);
	return(mdoc_err(mdoc, "macro expects an argument or parameter"));
}


static int
post_nm(struct mdoc *mdoc)
{

	assert(MDOC_ELEM == mdoc->last->type);
	assert(MDOC_Nm == mdoc->last->tok);
	if (mdoc->last->child)
		return(1);
	if (mdoc->meta.name)
		return(1);
	return(mdoc_err(mdoc, "macro `%s' has not been invoked with a name",
				mdoc_macronames[MDOC_Nm]));
}


static int
post_xr(struct mdoc *mdoc)
{
	struct mdoc_node *n;

	assert(MDOC_ELEM == mdoc->last->type);
	assert(MDOC_Xr == mdoc->last->tok);
	assert(mdoc->last->child);
	assert(MDOC_TEXT == mdoc->last->child->type);

	if (NULL == (n = mdoc->last->child->next))
		return(1);
	assert(MDOC_TEXT == n->type);
	if (MSEC_DEFAULT != mdoc_atomsec(n->data.text.string))
		return(1);
	return(mdoc_nerr(mdoc, n, "invalid manual section"));
}


static int
post_at(struct mdoc *mdoc)
{

	assert(MDOC_ELEM == mdoc->last->type);
	assert(MDOC_At == mdoc->last->tok);

	if (NULL == mdoc->last->child)
		return(1);
	assert(MDOC_TEXT == mdoc->last->child->type);

	if (ATT_DEFAULT != mdoc_atoatt(mdoc->last->child->data.text.string))
		return(1);
	return(mdoc_err(mdoc, "macro expects a valid AT&T version symbol"));
}


static int
post_an(struct mdoc *mdoc)
{

	assert(MDOC_ELEM == mdoc->last->type);
	assert(MDOC_An == mdoc->last->tok);

	if (0 != mdoc->last->data.elem.argc) {
		if (NULL == mdoc->last->child)
			return(1);
		return(mdoc_err(mdoc, "macro expects either argument or parameters"));
	}

	if (mdoc->last->child)
		return(1);
	return(mdoc_err(mdoc, "macro expects either argument or parameters"));
}


static int
post_ex(struct mdoc *mdoc)
{

	assert(MDOC_ELEM == mdoc->last->type);
	assert(MDOC_Ex == mdoc->last->tok);

	if (0 == mdoc->last->data.elem.argc) {
		if (mdoc->last->child)
			return(1);
		return(mdoc_err(mdoc, "macro expects `%s' or a single child",
					mdoc_argnames[MDOC_Std]));
	}
	if (mdoc->last->child)
		return(mdoc_err(mdoc, "macro expects `%s' or a single child",
					mdoc_argnames[MDOC_Std]));
	if (1 != mdoc->last->data.elem.argc)
		return(mdoc_err(mdoc, "macro expects `%s' or a single child",
					mdoc_argnames[MDOC_Std]));
	if (MDOC_Std != mdoc->last->data.elem.argv[0].arg)
		return(mdoc_err(mdoc, "macro expects `%s' or a single child",
					mdoc_argnames[MDOC_Std]));
	return(1);
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
	sv = -1;
	
	/* Some types require block-head, some not. */

	/* LINTED */
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
		n = mdoc->last->data.block.head;
		assert(n);
		if (NULL == n->child)
			if ( ! mdoc_warn(mdoc, WARN_SYNTAX, "macro suggests line parameters"))
				return(0);

		n = mdoc->last->data.block.body;
		assert(n);
		if (NULL == n->child)
			if ( ! mdoc_warn(mdoc, WARN_SYNTAX, "macro suggests body children"))
				return(0);

		return(1);
	}

	assert(TYPE_BODY == type);
	assert(mdoc->last->data.block.head);

	n = mdoc->last->data.block.head;
	assert(n);
	if (n->child)
		if ( ! mdoc_warn(mdoc, WARN_SYNTAX, "macro suggests no line parameters"))
			return(0);

	n = mdoc->last->data.block.body;
	assert(n);
	if (NULL == n->child)
		if ( ! mdoc_warn(mdoc, WARN_SYNTAX, "macro suggests body children"))
			return(0);

	assert(-1 != sv);
	if (MDOC_Column != sv) 
		return(1);

	/* Make sure the number of columns is sane. */

	argc = mdoc->last->parent->parent->data.block.argv->sz;
	n = mdoc->last->data.block.head->child;

	for (i = 0; n; n = n->next)
		i++;

	if (i == argc)
		return(1);
	return(mdoc_err(mdoc, "expected %zu list columns, have %zu", argc, i));
#undef	TYPE_NONE
#undef	TYPE_BODY
#undef	TYPE_HEAD
}


static int
post_bl(struct mdoc *mdoc)
{
	struct mdoc_node *n;

	if (MDOC_BODY != mdoc->last->type)
		return(1);
	assert(MDOC_Bl == mdoc->last->tok);

	/* LINTED */
	for (n = mdoc->last->child; n; n = n->next) {
		if (MDOC_BLOCK == n->type) 
			if (MDOC_It == n->tok)
				continue;
		break;
	}
	if (NULL == n)
		return(1);
	return(mdoc_nerr(mdoc, n, "invalid child of parent macro `Bl'"));
}


static int
ebool(struct mdoc *mdoc)
{
	struct mdoc_node *n;

	assert(MDOC_ELEM == mdoc->last->type);
	/* LINTED */
	for (n = mdoc->last->child; n; n = n->next) {
		if (MDOC_TEXT != n->type)
			break;
		if (xstrcmp(n->data.text.string, "on"))
			continue;
		if (xstrcmp(n->data.text.string, "off"))
			continue;
		break;
	}
	if (NULL == n)
		return(1);
	return(mdoc_nerr(mdoc, n, "expected boolean value"));
}


static int
post_root(struct mdoc *mdoc)
{

	if (NULL == mdoc->last->child)
		return(mdoc_err(mdoc, "document has no data"));
	if (NULL == mdoc->meta.title)
		return(mdoc_err(mdoc, "document has incomplete prologue"));
	if (NULL == mdoc->meta.os)
		return(mdoc_err(mdoc, "document has incomplete prologue"));
	if (0 == mdoc->meta.date)
		return(mdoc_err(mdoc, "document has incomplete prologue"));
	return(1);
}


/* Warn if conventional sections are out of order. */
static int
post_sh(struct mdoc *mdoc)
{
	char		  buf[64];
	enum mdoc_sec	  sec;

	if (MDOC_HEAD != mdoc->last->type)
		return(1);
	assert(MDOC_Sh == mdoc->last->tok);

	if ( ! xstrlcats(buf, mdoc->last->child, 64))
		return(mdoc_err(mdoc, "macro parameters too long"));

	if (SEC_CUSTOM == (sec = mdoc_atosec(buf)))
		return(1);
	if (sec > mdoc->sec_lastn)
		return(1);
	if (sec == mdoc->sec_lastn)
		return(mdoc_warn(mdoc, WARN_SYNTAX, "section repeated"));
	return(mdoc_warn(mdoc, WARN_SYNTAX, "section out of conventional order"));
}


int
mdoc_valid_pre(struct mdoc *mdoc, struct mdoc_node *node)
{
	v_pre		*p;

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

	if (MDOC_VALID & mdoc->last->flags)
		return(1);
	mdoc->last->flags |= MDOC_VALID;

	if (MDOC_TEXT == mdoc->last->type)
		return(1);
	if (MDOC_ROOT == mdoc->last->type)
		return(post_root(mdoc));

	if (NULL == mdoc_valids[mdoc->last->tok].post)
		return(1);
	for (p = mdoc_valids[mdoc->last->tok].post; *p; p++)
		if ( ! (*p)(mdoc)) 
			return(0);

	return(1);
}

