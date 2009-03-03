/* $Id$ */
/*
 * Copyright (c) 2009 Kristaps Dzonsons <kristaps@kth.se>
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
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "term.h"

/*
 * Performs actions on nodes of the abstract syntax tree.  Both pre- and
 * post-fix operations are defined here.
 */

/* FIXME: macro arguments can be escaped. */

#define	TTYPE_PROG	  0
#define	TTYPE_CMD_FLAG	  1
#define	TTYPE_CMD_ARG	  2
#define	TTYPE_SECTION	  3
#define	TTYPE_FUNC_DECL	  4
#define	TTYPE_VAR_DECL	  5
#define	TTYPE_FUNC_TYPE	  6
#define	TTYPE_FUNC_NAME	  7
#define	TTYPE_FUNC_ARG	  8
#define	TTYPE_LINK	  9
#define	TTYPE_SSECTION	  10
#define	TTYPE_FILE	  11
#define	TTYPE_EMPH	  12
#define	TTYPE_CONFIG	  13
#define	TTYPE_CMD	  14
#define	TTYPE_INCLUDE	  15
#define	TTYPE_SYMB	  16
#define	TTYPE_SYMBOL	  17
#define	TTYPE_DIAG	  18
#define	TTYPE_NMAX	  19

/* 
 * These define "styles" for element types, like command arguments or
 * executable names.  This is useful when multiple macros must decorate
 * the same thing (like .Ex -std cmd and .Nm cmd). 
 */

/* TODO: abstract this into mdocterm.c. */

const	int ttypes[TTYPE_NMAX] = {
	TERMP_BOLD,		/* TTYPE_PROG */
	TERMP_BOLD,		/* TTYPE_CMD_FLAG */
	TERMP_UNDERLINE, 	/* TTYPE_CMD_ARG */
	TERMP_BOLD, 		/* TTYPE_SECTION */
	TERMP_BOLD,		/* TTYPE_FUNC_DECL */
	TERMP_UNDERLINE,	/* TTYPE_VAR_DECL */
	TERMP_UNDERLINE,	/* TTYPE_FUNC_TYPE */
	TERMP_BOLD, 		/* TTYPE_FUNC_NAME */
	TERMP_UNDERLINE, 	/* TTYPE_FUNC_ARG */
	TERMP_UNDERLINE, 	/* TTYPE_LINK */
	TERMP_BOLD,	 	/* TTYPE_SSECTION */
	TERMP_UNDERLINE, 	/* TTYPE_FILE */
	TERMP_UNDERLINE, 	/* TTYPE_EMPH */
	TERMP_BOLD,	 	/* TTYPE_CONFIG */
	TERMP_BOLD,	 	/* TTYPE_CMD */
	TERMP_BOLD,	 	/* TTYPE_INCLUDE */
	TERMP_BOLD,	 	/* TTYPE_SYMB */
	TERMP_BOLD,	 	/* TTYPE_SYMBOL */
	TERMP_BOLD	 	/* TTYPE_DIAG */
};

static	int		  arg_hasattr(int, size_t, 
				const struct mdoc_arg *);
static	int		  arg_getattr(int, size_t, 
				const struct mdoc_arg *);
static	size_t		  arg_offset(const struct mdoc_arg *);
static	size_t		  arg_width(const struct mdoc_arg *);

/*
 * What follows describes prefix and postfix operations for the abstract
 * syntax tree descent.
 */

#define	DECL_ARGS \
	struct termp *p, \
	struct termpair *pair, \
	const struct mdoc_meta *meta, \
	const struct mdoc_node *node

#define	DECL_PRE(name) \
static	int	 	  name##_pre(DECL_ARGS)
#define	DECL_POST(name) \
static	void	 	  name##_post(DECL_ARGS)
#define	DECL_PREPOST(name) \
DECL_PRE(name); \
DECL_POST(name);

DECL_PREPOST(termp__t);
DECL_PREPOST(termp_aq);
DECL_PREPOST(termp_bd);
DECL_PREPOST(termp_bq);
DECL_PREPOST(termp_d1);
DECL_PREPOST(termp_dq);
DECL_PREPOST(termp_fd);
DECL_PREPOST(termp_fn);
DECL_PREPOST(termp_fo);
DECL_PREPOST(termp_ft);
DECL_PREPOST(termp_in);
DECL_PREPOST(termp_it);
DECL_PREPOST(termp_op);
DECL_PREPOST(termp_pf);
DECL_PREPOST(termp_pq);
DECL_PREPOST(termp_qq);
DECL_PREPOST(termp_sh);
DECL_PREPOST(termp_ss);
DECL_PREPOST(termp_sq);
DECL_PREPOST(termp_vt);

DECL_PRE(termp_ar);
DECL_PRE(termp_at);
DECL_PRE(termp_bf);
DECL_PRE(termp_bsx);
DECL_PRE(termp_bt);
DECL_PRE(termp_cd);
DECL_PRE(termp_cm);
DECL_PRE(termp_em);
DECL_PRE(termp_ex);
DECL_PRE(termp_fa);
DECL_PRE(termp_fl);
DECL_PRE(termp_fx);
DECL_PRE(termp_ic);
DECL_PRE(termp_ms);
DECL_PRE(termp_nd);
DECL_PRE(termp_nm);
DECL_PRE(termp_ns);
DECL_PRE(termp_nx);
DECL_PRE(termp_ox);
DECL_PRE(termp_pa);
DECL_PRE(termp_pp);
DECL_PRE(termp_rs);
DECL_PRE(termp_rv);
DECL_PRE(termp_sm);
DECL_PRE(termp_st);
DECL_PRE(termp_sx);
DECL_PRE(termp_sy);
DECL_PRE(termp_ud);
DECL_PRE(termp_ux);
DECL_PRE(termp_va);
DECL_PRE(termp_xr);

DECL_POST(termp___);
DECL_POST(termp_bl);
DECL_POST(termp_bx);

const	struct termact __termacts[MDOC_MAX] = {
	{ NULL, NULL }, /* \" */
	{ NULL, NULL }, /* Dd */
	{ NULL, NULL }, /* Dt */
	{ NULL, NULL }, /* Os */
	{ termp_sh_pre, termp_sh_post }, /* Sh */
	{ termp_ss_pre, termp_ss_post }, /* Ss */ 
	{ termp_pp_pre, NULL }, /* Pp */ 
	{ termp_d1_pre, termp_d1_post }, /* D1 */
	{ termp_d1_pre, termp_d1_post }, /* Dl */
	{ termp_bd_pre, termp_bd_post }, /* Bd */
	{ NULL, NULL }, /* Ed */
	{ NULL, termp_bl_post }, /* Bl */
	{ NULL, NULL }, /* El */
	{ termp_it_pre, termp_it_post }, /* It */
	{ NULL, NULL }, /* Ad */ 
	{ NULL, NULL }, /* An */
	{ termp_ar_pre, NULL }, /* Ar */
	{ termp_cd_pre, NULL }, /* Cd */
	{ termp_cm_pre, NULL }, /* Cm */
	{ NULL, NULL }, /* Dv */ 
	{ NULL, NULL }, /* Er */ 
	{ NULL, NULL }, /* Ev */ 
	{ termp_ex_pre, NULL }, /* Ex */
	{ termp_fa_pre, NULL }, /* Fa */ 
	{ termp_fd_pre, termp_fd_post }, /* Fd */ 
	{ termp_fl_pre, NULL }, /* Fl */
	{ termp_fn_pre, termp_fn_post }, /* Fn */ 
	{ termp_ft_pre, termp_ft_post }, /* Ft */ 
	{ termp_ic_pre, NULL }, /* Ic */ 
	{ termp_in_pre, termp_in_post }, /* In */ 
	{ NULL, NULL }, /* Li */
	{ termp_nd_pre, NULL }, /* Nd */ 
	{ termp_nm_pre, NULL }, /* Nm */ 
	{ termp_op_pre, termp_op_post }, /* Op */
	{ NULL, NULL }, /* Ot */
	{ termp_pa_pre, NULL }, /* Pa */
	{ termp_rv_pre, NULL }, /* Rv */
	{ termp_st_pre, NULL }, /* St */ 
	{ termp_va_pre, NULL }, /* Va */
	{ termp_vt_pre, termp_vt_post }, /* Vt */ 
	{ termp_xr_pre, NULL }, /* Xr */
	{ NULL, termp____post }, /* %A */
	{ NULL, termp____post }, /* %B */
	{ NULL, termp____post }, /* %D */
	{ NULL, termp____post }, /* %I */
	{ NULL, termp____post }, /* %J */
	{ NULL, termp____post }, /* %N */
	{ NULL, termp____post }, /* %O */
	{ NULL, termp____post }, /* %P */
	{ NULL, termp____post }, /* %R */
	{ termp__t_pre, termp__t_post }, /* %T */
	{ NULL, termp____post }, /* %V */
	{ NULL, NULL }, /* Ac */
	{ termp_aq_pre, termp_aq_post }, /* Ao */
	{ termp_aq_pre, termp_aq_post }, /* Aq */
	{ termp_at_pre, NULL }, /* At */
	{ NULL, NULL }, /* Bc */
	{ termp_bf_pre, NULL }, /* Bf */ 
	{ termp_bq_pre, termp_bq_post }, /* Bo */
	{ termp_bq_pre, termp_bq_post }, /* Bq */
	{ termp_bsx_pre, NULL }, /* Bsx */
	{ NULL, termp_bx_post }, /* Bx */
	{ NULL, NULL }, /* Db */
	{ NULL, NULL }, /* Dc */
	{ termp_dq_pre, termp_dq_post }, /* Do */
	{ termp_dq_pre, termp_dq_post }, /* Dq */
	{ NULL, NULL }, /* Ec */
	{ NULL, NULL }, /* Ef */
	{ termp_em_pre, NULL }, /* Em */ 
	{ NULL, NULL }, /* Eo */
	{ termp_fx_pre, NULL }, /* Fx */
	{ termp_ms_pre, NULL }, /* Ms */
	{ NULL, NULL }, /* No */
	{ termp_ns_pre, NULL }, /* Ns */
	{ termp_nx_pre, NULL }, /* Nx */
	{ termp_ox_pre, NULL }, /* Ox */
	{ NULL, NULL }, /* Pc */
	{ termp_pf_pre, termp_pf_post }, /* Pf */
	{ termp_pq_pre, termp_pq_post }, /* Po */
	{ termp_pq_pre, termp_pq_post }, /* Pq */
	{ NULL, NULL }, /* Qc */
	{ termp_sq_pre, termp_sq_post }, /* Ql */
	{ termp_qq_pre, termp_qq_post }, /* Qo */
	{ termp_qq_pre, termp_qq_post }, /* Qq */
	{ NULL, NULL }, /* Re */
	{ termp_rs_pre, NULL }, /* Rs */
	{ NULL, NULL }, /* Sc */
	{ termp_sq_pre, termp_sq_post }, /* So */
	{ termp_sq_pre, termp_sq_post }, /* Sq */
	{ termp_sm_pre, NULL }, /* Sm */
	{ termp_sx_pre, NULL }, /* Sx */
	{ termp_sy_pre, NULL }, /* Sy */
	{ NULL, NULL }, /* Tn */
	{ termp_ux_pre, NULL }, /* Ux */
	{ NULL, NULL }, /* Xc */
	{ NULL, NULL }, /* Xo */
	{ termp_fo_pre, termp_fo_post }, /* Fo */ 
	{ NULL, NULL }, /* Fc */ 
	{ termp_op_pre, termp_op_post }, /* Oo */
	{ NULL, NULL }, /* Oc */
	{ NULL, NULL }, /* Bk */
	{ NULL, NULL }, /* Ek */
	{ termp_bt_pre, NULL }, /* Bt */
	{ NULL, NULL }, /* Hf */
	{ NULL, NULL }, /* Fr */
	{ termp_ud_pre, NULL }, /* Ud */
};

const struct termact *termacts = __termacts;


static size_t
arg_width(const struct mdoc_arg *arg)
{
	size_t		 v;
	int		 i, len;

	assert(*arg->value);
	if (0 == strcmp(*arg->value, "indent"))
		return(INDENT);
	if (0 == strcmp(*arg->value, "indent-two"))
		return(INDENT * 2);

	len = (int)strlen(*arg->value);
	assert(len > 0);

	for (i = 0; i < len - 1; i++) 
		if ( ! isdigit((int)(*arg->value)[i]))
			break;

	if (i == len - 1) {
		if ('n' == (*arg->value)[len - 1]) {
			v = (size_t)atoi(*arg->value);
			return(v);
		}

	}
	return(strlen(*arg->value) + 1);
}


static size_t
arg_offset(const struct mdoc_arg *arg)
{

	/* TODO */
	assert(*arg->value);
	if (0 == strcmp(*arg->value, "indent"))
		return(INDENT);
	if (0 == strcmp(*arg->value, "indent-two"))
		return(INDENT * 2);
	return(strlen(*arg->value));
}


static int
arg_hasattr(int arg, size_t argc, const struct mdoc_arg *argv)
{

	return(-1 != arg_getattr(arg, argc, argv));
}


static int
arg_getattr(int arg, size_t argc, const struct mdoc_arg *argv)
{
	int		 i;

	for (i = 0; i < (int)argc; i++) 
		if (argv[i].arg == arg)
			return(i);
	return(-1);
}


/* ARGSUSED */
static int
termp_dq_pre(DECL_ARGS)
{

	if (MDOC_BODY != node->type)
		return(1);

	word(p, "``");
	p->flags |= TERMP_NOSPACE;
	return(1);
}


/* ARGSUSED */
static void
termp_dq_post(DECL_ARGS)
{

	if (MDOC_BODY != node->type)
		return;

	p->flags |= TERMP_NOSPACE;
	word(p, "''");
}


/* ARGSUSED */
static int
termp_it_pre(DECL_ARGS)
{
	const struct mdoc_node *n, *it;
	const struct mdoc_block *bl;
	char		 buf[7], *tp;
	int		 i, type;
	size_t		 width, offset;

	switch (node->type) {
	case (MDOC_BODY):
		/* FALLTHROUGH */
	case (MDOC_HEAD):
		it = node->parent;
		break;
	case (MDOC_BLOCK):
		it = node;
		break;
	default:
		return(1);
	}

	n = it->parent->parent;
	bl = &n->data.block;

	if (MDOC_BLOCK == node->type) {
		newln(p);
		if ( ! arg_hasattr(MDOC_Compact, bl->argc, bl->argv))
			if (node->prev || n->prev)
				vspace(p);
		return(1);
	}

	/* Get our list type. */

	for (type = -1, i = 0; i < (int)bl->argc; i++) 
		switch (bl->argv[i].arg) {
		case (MDOC_Bullet):
			/* FALLTHROUGH */
		case (MDOC_Dash):
			/* FALLTHROUGH */
		case (MDOC_Enum):
			/* FALLTHROUGH */
		case (MDOC_Hyphen):
			/* FALLTHROUGH */
		case (MDOC_Tag):
			/* FALLTHROUGH */
		case (MDOC_Inset):
			/* FALLTHROUGH */
		case (MDOC_Diag):
			/* FALLTHROUGH */
		case (MDOC_Ohang):
			type = bl->argv[i].arg;
			i = (int)bl->argc;
			break;
		default:
			errx(1, "list type not supported");
			/* NOTREACHED */
		}

	assert(-1 != type);

	/* Save our existing (inherited) margin and offset. */

	pair->offset = p->offset;
	pair->rmargin = p->rmargin;

	/* Get list width and offset. */

	i = arg_getattr(MDOC_Width, bl->argc, bl->argv);
	width = i >= 0 ? arg_width(&bl->argv[i]) : 0;

	i = arg_getattr(MDOC_Offset, bl->argc, bl->argv);
	offset = i >= 0 ? arg_offset(&bl->argv[i]) : 0;

	/* Override the width. */

	switch (type) {
	case (MDOC_Bullet):
		/* FALLTHROUGH */
	case (MDOC_Dash):
		/* FALLTHROUGH */
	case (MDOC_Enum):
		/* FALLTHROUGH */
	case (MDOC_Hyphen):
		width = width > 6 ? width : 6;
		break;
	case (MDOC_Tag):
		if (0 == width)
			errx(1, "need non-zero -width");
		break;
	default:
		break;
	}

	/* Word-wrap control. */

	switch (type) {
	case (MDOC_Diag):
		if (MDOC_HEAD == node->type)
			TERMPAIR_SETFLAG(p, pair, ttypes[TTYPE_DIAG]);
		/* FALLTHROUGH */
	case (MDOC_Inset):
		if (MDOC_HEAD == node->type)
			p->flags |= TERMP_NOSPACE;
		break;
	case (MDOC_Bullet):
		/* FALLTHROUGH */
	case (MDOC_Dash):
		/* FALLTHROUGH */
	case (MDOC_Enum):
		/* FALLTHROUGH */
	case (MDOC_Hyphen):
		/* FALLTHROUGH */
	case (MDOC_Tag):
		p->flags |= TERMP_NOSPACE;
		if (MDOC_HEAD == node->type)
			p->flags |= TERMP_NOBREAK;
		else if (MDOC_BODY == node->type)
			p->flags |= TERMP_NOLPAD;
		break;
	default:
		break;
	}

	/* 
	 * Get a token to use as the HEAD lead-in.  If NULL, we use the
	 * HEAD child. 
	 */

	tp = NULL;

	if (MDOC_HEAD == node->type) {
		if (arg_hasattr(MDOC_Bullet, bl->argc, bl->argv))
			tp = "\\[bu]";
		if (arg_hasattr(MDOC_Dash, bl->argc, bl->argv))
			tp = "\\-";
		if (arg_hasattr(MDOC_Enum, bl->argc, bl->argv)) {
			(pair->ppair->ppair->count)++;
			(void)snprintf(buf, sizeof(buf), "%d.", 
					pair->ppair->ppair->count);
			tp = buf;
		}
		if (arg_hasattr(MDOC_Hyphen, bl->argc, bl->argv))
			tp = "\\-";
	}

	/* Margin control. */

	p->offset += offset;

	switch (type) {
	case (MDOC_Bullet):
		/* FALLTHROUGH */
	case (MDOC_Dash):
		/* FALLTHROUGH */
	case (MDOC_Enum):
		/* FALLTHROUGH */
	case (MDOC_Hyphen):
		/* FALLTHROUGH */
	case (MDOC_Tag):
		if (MDOC_HEAD == node->type)
			p->rmargin = p->offset + width;
		else if (MDOC_BODY == node->type) 
			p->offset += width;
		break;
	default:
		break;
	}

	if (NULL == tp)
		return(1);

	word(p, tp);
	return(0);
}


/* ARGSUSED */
static void
termp_it_post(DECL_ARGS)
{
	int		   type, i;
	struct mdoc_block *bl;

	if (MDOC_BODY != node->type && MDOC_HEAD != node->type)
		return;

	assert(MDOC_BLOCK == node->parent->parent->parent->type);
	assert(MDOC_Bl == node->parent->parent->parent->tok);
	bl = &node->parent->parent->parent->data.block;

	for (type = -1, i = 0; i < (int)bl->argc; i++) 
		switch (bl->argv[i].arg) {
		case (MDOC_Bullet):
			/* FALLTHROUGH */
		case (MDOC_Dash):
			/* FALLTHROUGH */
		case (MDOC_Enum):
			/* FALLTHROUGH */
		case (MDOC_Hyphen):
			/* FALLTHROUGH */
		case (MDOC_Tag):
			/* FALLTHROUGH */
		case (MDOC_Diag):
			/* FALLTHROUGH */
		case (MDOC_Inset):
			/* FALLTHROUGH */
		case (MDOC_Ohang):
			type = bl->argv[i].arg;
			i = (int)bl->argc;
			break;
		default:
			errx(1, "list type not supported");
			/* NOTREACHED */
		}


	switch (type) {
	case (MDOC_Diag):
		/* FALLTHROUGH */
	case (MDOC_Inset):
		break;
	default:
		flushln(p);
		break;
	}

	p->offset = pair->offset;
	p->rmargin = pair->rmargin;

	switch (type) {
	case (MDOC_Inset):
		break;
	default:
		if (MDOC_HEAD == node->type)
			p->flags &= ~TERMP_NOBREAK;
		else if (MDOC_BODY == node->type)
			p->flags &= ~TERMP_NOLPAD;
		break;
	}
}


/* ARGSUSED */
static int
termp_nm_pre(DECL_ARGS)
{

	if (SEC_SYNOPSIS == node->sec)
		newln(p);

	TERMPAIR_SETFLAG(p, pair, ttypes[TTYPE_PROG]);
	if (NULL == node->child)
		word(p, meta->name);

	return(1);
}


/* ARGSUSED */
static int
termp_fl_pre(DECL_ARGS)
{

	TERMPAIR_SETFLAG(p, pair, ttypes[TTYPE_CMD_FLAG]);
	word(p, "\\-");
	p->flags |= TERMP_NOSPACE;
	return(1);
}


/* ARGSUSED */
static int
termp_ar_pre(DECL_ARGS)
{

	TERMPAIR_SETFLAG(p, pair, ttypes[TTYPE_CMD_ARG]);
	if (NULL == node->child) {
		word(p, "file");
		word(p, "...");
	}
	return(1);
}


/* ARGSUSED */
static int
termp_ns_pre(DECL_ARGS)
{

	p->flags |= TERMP_NOSPACE;
	return(1);
}


/* ARGSUSED */
static int
termp_pp_pre(DECL_ARGS)
{

	vspace(p);
	return(1);
}


/* ARGSUSED */
static int
termp_st_pre(DECL_ARGS)
{
	const char 	*tp;

	assert(1 == node->data.elem.argc);

	tp = mdoc_st2a(node->data.elem.argv[0].arg);
	word(p, tp);

	return(1);
}


/* ARGSUSED */
static int
termp_rs_pre(DECL_ARGS)
{

	if (MDOC_BLOCK == node->type && node->prev)
		vspace(p);
	return(1);
}


/* ARGSUSED */
static int
termp_rv_pre(DECL_ARGS)
{
	int		 i;

	i = arg_getattr(MDOC_Std, node->data.elem.argc, 
			node->data.elem.argv);
	assert(i >= 0);

	newln(p);
	word(p, "The");

	p->flags |= ttypes[TTYPE_FUNC_NAME];
	word(p, *node->data.elem.argv[i].value);
	p->flags &= ~ttypes[TTYPE_FUNC_NAME];

       	word(p, "() function returns the value 0 if successful;");
       	word(p, "otherwise the value -1 is returned and the");
       	word(p, "global variable");

	p->flags |= ttypes[TTYPE_VAR_DECL];
	word(p, "errno");
	p->flags &= ~ttypes[TTYPE_VAR_DECL];

       	word(p, "is set to indicate the error.");

	return(1);
}


/* ARGSUSED */
static int
termp_ex_pre(DECL_ARGS)
{
	int		 i;

	i = arg_getattr(MDOC_Std, node->data.elem.argc, 
			node->data.elem.argv);
	assert(i >= 0);

	word(p, "The");
	p->flags |= ttypes[TTYPE_PROG];
	word(p, *node->data.elem.argv[i].value);
	p->flags &= ~ttypes[TTYPE_PROG];
       	word(p, "utility exits 0 on success, and >0 if an error occurs.");

	return(1);
}


/* ARGSUSED */
static int
termp_nd_pre(DECL_ARGS)
{

	word(p, "\\-");
	return(1);
}


/* ARGSUSED */
static void
termp_bl_post(DECL_ARGS)
{

	if (MDOC_BLOCK == node->type)
		newln(p);
}


/* ARGSUSED */
static void
termp_op_post(DECL_ARGS)
{

	if (MDOC_BODY != node->type) 
		return;
	p->flags |= TERMP_NOSPACE;
	word(p, "\\(rB");
}


/* ARGSUSED */
static int
termp_xr_pre(DECL_ARGS)
{
	const struct mdoc_node *n;

	n = node->child;
	assert(n);

	assert(MDOC_TEXT == n->type);
	word(p, n->data.text.string);

	if (NULL == (n = n->next)) 
		return(0);

	assert(MDOC_TEXT == n->type);
	p->flags |= TERMP_NOSPACE;
	word(p, "(");
	p->flags |= TERMP_NOSPACE;
	word(p, n->data.text.string);
	p->flags |= TERMP_NOSPACE;
	word(p, ")");

	return(0);
}


/* ARGSUSED */
static int
termp_vt_pre(DECL_ARGS)
{

	/* FIXME: this can be "type name". */
	TERMPAIR_SETFLAG(p, pair, ttypes[TTYPE_VAR_DECL]);
	return(1);
}


/* ARGSUSED */
static void
termp_vt_post(DECL_ARGS)
{

	if (node->sec == SEC_SYNOPSIS)
		vspace(p);
}


/* ARGSUSED */
static int
termp_fd_pre(DECL_ARGS)
{

	/* 
	 * FIXME: this naming is bad.  This value is used, in general,
	 * for the #include header or other preprocessor statement.
	 */
	TERMPAIR_SETFLAG(p, pair, ttypes[TTYPE_FUNC_DECL]);
	return(1);
}


/* ARGSUSED */
static void
termp_fd_post(DECL_ARGS)
{

	if (node->sec != SEC_SYNOPSIS)
		return;
	newln(p);
	if (node->next && MDOC_Fd != node->next->tok)
		vspace(p);
}


/* ARGSUSED */
static int
termp_sh_pre(DECL_ARGS)
{

	switch (node->type) {
	case (MDOC_HEAD):
		vspace(p);
		TERMPAIR_SETFLAG(p, pair, ttypes[TTYPE_SECTION]);
		break;
	case (MDOC_BODY):
		p->offset = INDENT;
		break;
	default:
		break;
	}
	return(1);
}


/* ARGSUSED */
static void
termp_sh_post(DECL_ARGS)
{

	switch (node->type) {
	case (MDOC_HEAD):
		newln(p);
		break;
	case (MDOC_BODY):
		newln(p);
		p->offset = 0;
		break;
	default:
		break;
	}
}


/* ARGSUSED */
static int
termp_op_pre(DECL_ARGS)
{

	switch (node->type) {
	case (MDOC_BODY):
		word(p, "\\(lB");
		p->flags |= TERMP_NOSPACE;
		break;
	default:
		break;
	}
	return(1);
}


/* ARGSUSED */
static int
termp_bt_pre(DECL_ARGS)
{

	word(p, "is currently in beta test.");
	return(1);
}


/* ARGSUSED */
static int
termp_ud_pre(DECL_ARGS)
{

	word(p, "currently under development.");
	return(1);
}


/* ARGSUSED */
static int
termp_d1_pre(DECL_ARGS)
{

	if (MDOC_BODY != node->type)
		return(1);
	newln(p);
	p->offset += (pair->offset = INDENT);
	return(1);
}


/* ARGSUSED */
static void
termp_d1_post(DECL_ARGS)
{

	if (MDOC_BODY != node->type) 
		return;
	newln(p);
	p->offset -= pair->offset;
}


/* ARGSUSED */
static int
termp_aq_pre(DECL_ARGS)
{

	if (MDOC_BODY != node->type)
		return(1);
	word(p, "<");
	p->flags |= TERMP_NOSPACE;
	return(1);
}


/* ARGSUSED */
static void
termp_aq_post(DECL_ARGS)
{

	if (MDOC_BODY != node->type)
		return;
	p->flags |= TERMP_NOSPACE;
	word(p, ">");
}


/* ARGSUSED */
static int
termp_ft_pre(DECL_ARGS)
{

	if (SEC_SYNOPSIS == node->sec)
		if (node->prev && MDOC_Fo == node->prev->tok)
			vspace(p);
	TERMPAIR_SETFLAG(p, pair, ttypes[TTYPE_FUNC_TYPE]);
	return(1);
}


/* ARGSUSED */
static void
termp_ft_post(DECL_ARGS)
{

	if (SEC_SYNOPSIS == node->sec)
		newln(p);
}


/* ARGSUSED */
static int
termp_fn_pre(DECL_ARGS)
{
	const struct mdoc_node *n;

	assert(node->child);
	assert(MDOC_TEXT == node->child->type);

	/* FIXME: can be "type funcname" "type varname"... */

	p->flags |= ttypes[TTYPE_FUNC_NAME];
	word(p, node->child->data.text.string);
	p->flags &= ~ttypes[TTYPE_FUNC_NAME];

	word(p, "(");

	p->flags |= TERMP_NOSPACE;
	for (n = node->child->next; n; n = n->next) {
		assert(MDOC_TEXT == n->type);
		p->flags |= ttypes[TTYPE_FUNC_ARG];
		word(p, n->data.text.string);
		p->flags &= ~ttypes[TTYPE_FUNC_ARG];
		if (n->next)
			word(p, ",");
	}

	word(p, ")");

	if (SEC_SYNOPSIS == node->sec)
		word(p, ";");

	return(0);
}


/* ARGSUSED */
static void
termp_fn_post(DECL_ARGS)
{

	if (node->sec == SEC_SYNOPSIS && node->next)
		vspace(p);

}


/* ARGSUSED */
static int
termp_sx_pre(DECL_ARGS)
{

	TERMPAIR_SETFLAG(p, pair, ttypes[TTYPE_LINK]);
	return(1);
}


/* ARGSUSED */
static int
termp_fa_pre(DECL_ARGS)
{
	struct mdoc_node *n;

	if (node->parent->tok != MDOC_Fo) {
		TERMPAIR_SETFLAG(p, pair, ttypes[TTYPE_FUNC_ARG]);
		return(1);
	}

	for (n = node->child; n; n = n->next) {
		assert(MDOC_TEXT == n->type);

		p->flags |= ttypes[TTYPE_FUNC_ARG];
		word(p, n->data.text.string);
		p->flags &= ~ttypes[TTYPE_FUNC_ARG];

		if (n->next)
			word(p, ",");
	}

	if (node->next && node->next->tok == MDOC_Fa)
		word(p, ",");

	return(0);
}


/* ARGSUSED */
static int
termp_va_pre(DECL_ARGS)
{

	TERMPAIR_SETFLAG(p, pair, ttypes[TTYPE_VAR_DECL]);
	return(1);
}


/* ARGSUSED */
static int
termp_bd_pre(DECL_ARGS)
{
	const struct mdoc_block *bl;
	const struct mdoc_node  *n;
	int		 i, type;

	if (MDOC_BLOCK == node->type) {
		if (node->prev)
			vspace(p);
		return(1);
	} else if (MDOC_BODY != node->type)
		return(1);

	pair->offset = p->offset;
	bl = &node->parent->data.block;

	for (type = -1, i = 0; i < (int)bl->argc; i++) {
		switch (bl->argv[i].arg) {
		case (MDOC_Ragged):
			/* FALLTHROUGH */
		case (MDOC_Filled):
			/* FALLTHROUGH */
		case (MDOC_Unfilled):
			/* FALLTHROUGH */
		case (MDOC_Literal):
			type = bl->argv[i].arg;
			i = (int)bl->argc;
			break;
		default:
			errx(1, "display type not supported");
		}
	}

	assert(-1 != type);

	i = arg_getattr(MDOC_Offset, bl->argc, bl->argv);
	if (-1 != i) {
		assert(1 == bl->argv[i].sz);
		p->offset += arg_offset(&bl->argv[i]);
	}


	switch (type) {
	case (MDOC_Literal):
		/* FALLTHROUGH */
	case (MDOC_Unfilled):
		break;
	default:
		return(1);
	}

	p->flags |= TERMP_LITERAL;

	for (n = node->child; n; n = n->next) {
		if (MDOC_TEXT != n->type) {
			warnx("non-text children not yet allowed");
			continue;
		}
		word(p, n->data.text.string);
		flushln(p);
	}

	return(0);
}


/* ARGSUSED */
static void
termp_bd_post(DECL_ARGS)
{

	if (MDOC_BODY != node->type) 
		return;

	if ( ! (p->flags & TERMP_LITERAL))
		flushln(p);

	p->flags &= ~TERMP_LITERAL;
	p->offset = pair->offset;
}


/* ARGSUSED */
static int
termp_qq_pre(DECL_ARGS)
{

	if (MDOC_BODY != node->type)
		return(1);
	word(p, "\"");
	p->flags |= TERMP_NOSPACE;
	return(1);
}


/* ARGSUSED */
static void
termp_qq_post(DECL_ARGS)
{

	if (MDOC_BODY != node->type)
		return;
	p->flags |= TERMP_NOSPACE;
	word(p, "\"");
}


/* ARGSUSED */
static int
termp_bsx_pre(DECL_ARGS)
{

	word(p, "BSDI BSD/OS");
	return(1);
}


/* ARGSUSED */
static void
termp_bx_post(DECL_ARGS)
{

	if (node->child)
		p->flags |= TERMP_NOSPACE;
	word(p, "BSD");
}


/* ARGSUSED */
static int
termp_ox_pre(DECL_ARGS)
{

	word(p, "OpenBSD");
	return(1);
}


/* ARGSUSED */
static int
termp_ux_pre(DECL_ARGS)
{

	word(p, "UNIX");
	return(1);
}


/* ARGSUSED */
static int
termp_fx_pre(DECL_ARGS)
{

	word(p, "FreeBSD");
	return(1);
}


/* ARGSUSED */
static int
termp_nx_pre(DECL_ARGS)
{

	word(p, "NetBSD");
	return(1);
}


/* ARGSUSED */
static int
termp_sq_pre(DECL_ARGS)
{

	if (MDOC_BODY != node->type)
		return(1);
	word(p, "`");
	p->flags |= TERMP_NOSPACE;
	return(1);
}


/* ARGSUSED */
static void
termp_sq_post(DECL_ARGS)
{

	if (MDOC_BODY != node->type)
		return;
	p->flags |= TERMP_NOSPACE;
	word(p, "\'");
}


/* ARGSUSED */
static int
termp_pf_pre(DECL_ARGS)
{

	p->flags |= TERMP_IGNDELIM;
	return(1);
}


/* ARGSUSED */
static void
termp_pf_post(DECL_ARGS)
{

	p->flags &= ~TERMP_IGNDELIM;
	p->flags |= TERMP_NOSPACE;
}


/* ARGSUSED */
static int
termp_ss_pre(DECL_ARGS)
{

	switch (node->type) {
	case (MDOC_HEAD):
		vspace(p);
		TERMPAIR_SETFLAG(p, pair, ttypes[TTYPE_SSECTION]);
		p->offset = INDENT / 2;
		break;
	default:
		break;
	}

	return(1);
}


/* ARGSUSED */
static void
termp_ss_post(DECL_ARGS)
{

	switch (node->type) {
	case (MDOC_HEAD):
		newln(p);
		p->offset = INDENT;
		break;
	default:
		break;
	}
}


/* ARGSUSED */
static int
termp_pa_pre(DECL_ARGS)
{

	TERMPAIR_SETFLAG(p, pair, ttypes[TTYPE_FILE]);
	return(1);
}


/* ARGSUSED */
static int
termp_em_pre(DECL_ARGS)
{

	TERMPAIR_SETFLAG(p, pair, ttypes[TTYPE_EMPH]);
	return(1);
}


/* ARGSUSED */
static int
termp_cd_pre(DECL_ARGS)
{

	TERMPAIR_SETFLAG(p, pair, ttypes[TTYPE_CONFIG]);
	newln(p);
	return(1);
}


/* ARGSUSED */
static int
termp_cm_pre(DECL_ARGS)
{

	TERMPAIR_SETFLAG(p, pair, ttypes[TTYPE_CMD_FLAG]);
	return(1);
}


/* ARGSUSED */
static int
termp_ic_pre(DECL_ARGS)
{

	TERMPAIR_SETFLAG(p, pair, ttypes[TTYPE_CMD]);
	return(1);
}


/* ARGSUSED */
static int
termp_in_pre(DECL_ARGS)
{

	TERMPAIR_SETFLAG(p, pair, ttypes[TTYPE_INCLUDE]);
	word(p, "#include");
	word(p, "<");
	p->flags |= TERMP_NOSPACE;
	return(1);
}


/* ARGSUSED */
static void
termp_in_post(DECL_ARGS)
{

	p->flags |= TERMP_NOSPACE;
	word(p, ">");

	newln(p);
	if (SEC_SYNOPSIS != node->sec)
		return;
	if (node->next && MDOC_In != node->next->tok)
		vspace(p);
}


/* ARGSUSED */
static int
termp_at_pre(DECL_ARGS)
{
	enum mdoc_att	 c;

	c = ATT_DEFAULT;
	if (node->child) {
		assert(MDOC_TEXT == node->child->type);
		c = mdoc_atoatt(node->child->data.text.string);
	}

	word(p, mdoc_att2a(c));
	return(0);
}


/* ARGSUSED */
static int
termp_bq_pre(DECL_ARGS)
{

	if (MDOC_BODY != node->type)
		return(1);
	word(p, "[");
	p->flags |= TERMP_NOSPACE;
	return(1);
}


/* ARGSUSED */
static void
termp_bq_post(DECL_ARGS)
{

	if (MDOC_BODY != node->type)
		return;
	word(p, "]");
}


/* ARGSUSED */
static int
termp_pq_pre(DECL_ARGS)
{

	if (MDOC_BODY != node->type)
		return(1);
	word(p, "\\&(");
	p->flags |= TERMP_NOSPACE;
	return(1);
}


/* ARGSUSED */
static void
termp_pq_post(DECL_ARGS)
{

	if (MDOC_BODY != node->type)
		return;
	word(p, ")");
}


/* ARGSUSED */
static int
termp_fo_pre(DECL_ARGS)
{
	const struct mdoc_node *n;

	if (MDOC_BODY == node->type) {
		word(p, "(");
		p->flags |= TERMP_NOSPACE;
		return(1);
	} else if (MDOC_HEAD != node->type) 
		return(1);

	/* XXX - groff shows only first parameter */

	p->flags |= ttypes[TTYPE_FUNC_NAME];
	for (n = node->child; n; n = n->next) {
		assert(MDOC_TEXT == n->type);
		word(p, n->data.text.string);
	}
	p->flags &= ~ttypes[TTYPE_FUNC_NAME];

	return(0);
}


/* ARGSUSED */
static void
termp_fo_post(DECL_ARGS)
{

	if (MDOC_BODY != node->type)
		return;
	word(p, ")");
	word(p, ";");
	newln(p);
}


/* ARGSUSED */
static int
termp_bf_pre(DECL_ARGS)
{
	const struct mdoc_node	*n;
	const struct mdoc_block	*b;

	/* XXX - we skip over possible trailing HEAD tokens. */

	if (MDOC_HEAD == node->type)
		return(0);
	else if (MDOC_BLOCK != node->type)
		return(1);

	b = &node->data.block;

	if (NULL == (n = b->head->child)) {
		if (arg_hasattr(MDOC_Emphasis, b->argc, b->argv))
			TERMPAIR_SETFLAG(p, pair, ttypes[TTYPE_EMPH]);
		else if (arg_hasattr(MDOC_Symbolic, b->argc, b->argv))
			TERMPAIR_SETFLAG(p, pair, ttypes[TTYPE_SYMB]);

		return(1);
	} 

	assert(MDOC_TEXT == n->type);

	if (0 == strcmp("Em", n->data.text.string))
		TERMPAIR_SETFLAG(p, pair, ttypes[TTYPE_EMPH]);
	else if (0 == strcmp("Sy", n->data.text.string))
		TERMPAIR_SETFLAG(p, pair, ttypes[TTYPE_EMPH]);

	return(1);
}


/* ARGSUSED */
static int
termp_sy_pre(DECL_ARGS)
{

	TERMPAIR_SETFLAG(p, pair, ttypes[TTYPE_SYMB]);
	return(1);
}


/* ARGSUSED */
static int
termp_ms_pre(DECL_ARGS)
{

	TERMPAIR_SETFLAG(p, pair, ttypes[TTYPE_SYMBOL]);
	return(1);
}



/* ARGSUSED */
static int
termp_sm_pre(DECL_ARGS)
{

#if notyet
	assert(node->child);
	if (0 == strcmp("off", node->child->data.text.string)) {
		p->flags &= ~TERMP_NONOSPACE;
		p->flags &= ~TERMP_NOSPACE;
	} else {
		p->flags |= TERMP_NONOSPACE;
		p->flags |= TERMP_NOSPACE;
	}
#endif

	return(0);
}


/* ARGSUSED */
static int
termp__t_pre(DECL_ARGS)
{

	word(p, "\"");
	p->flags |= TERMP_NOSPACE;
	return(1);
}


/* ARGSUSED */
static void
termp__t_post(DECL_ARGS)
{

	p->flags |= TERMP_NOSPACE;
	word(p, "\"");
	word(p, node->next ? "," : ".");
}


/* ARGSUSED */
static void
termp____post(DECL_ARGS)
{

	p->flags |= TERMP_NOSPACE;
	word(p, node->next ? "," : ".");
}
