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
#include <stdlib.h>
#include <string.h>

#include "term.h"

#define	INDENT		  4

/*
 * Performs actions on nodes of the abstract syntax tree.  Both pre- and
 * post-fix operations are defined here.
 */

/* FIXME: indent/tab. */
/* FIXME: handle nested lists. */

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
#define	TTYPE_NMAX	  18

/* 
 * These define "styles" for element types, like command arguments or
 * executable names.  This is useful when multiple macros must decorate
 * the same thing (like .Ex -std cmd and .Nm cmd). 
 */

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
	TERMP_BOLD	 	/* TTYPE_SYMBOL */
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
	const struct mdoc_meta *meta, \
	const struct mdoc_node *node

#define	DECL_PRE(name) \
static	int	 	  name##_pre(DECL_ARGS)
#define	DECL_POST(name) \
static	void	 	  name##_post(DECL_ARGS)
#define	DECL_PREPOST(name) \
DECL_PRE(name); \
DECL_POST(name);

DECL_PREPOST(termp_aq);
DECL_PREPOST(termp_ar);
DECL_PREPOST(termp_bf);
DECL_PREPOST(termp_bd);
DECL_PREPOST(termp_bq);
DECL_PREPOST(termp_cd);
DECL_PREPOST(termp_cm);
DECL_PREPOST(termp_d1);
DECL_PREPOST(termp_dq);
DECL_PREPOST(termp_em);
DECL_PREPOST(termp_fa);
DECL_PREPOST(termp_fd);
DECL_PREPOST(termp_fl);
DECL_PREPOST(termp_fn);
DECL_PREPOST(termp_fo);
DECL_PREPOST(termp_ft);
DECL_PREPOST(termp_ic);
DECL_PREPOST(termp_in);
DECL_PREPOST(termp_it);
DECL_PREPOST(termp_ms);
DECL_PREPOST(termp_nm);
DECL_PREPOST(termp_op);
DECL_PREPOST(termp_pa);
DECL_PREPOST(termp_pf);
DECL_PREPOST(termp_pq);
DECL_PREPOST(termp_qq);
DECL_PREPOST(termp_sh);
DECL_PREPOST(termp_ss);
DECL_PREPOST(termp_sq);
DECL_PREPOST(termp_sx);
DECL_PREPOST(termp_sy);
DECL_PREPOST(termp_va);
DECL_PREPOST(termp_vt);

DECL_PRE(termp_at);
DECL_PRE(termp_bsx);
DECL_PRE(termp_bt);
DECL_PRE(termp_bx);
DECL_PRE(termp_ex);
DECL_PRE(termp_fx);
DECL_PRE(termp_nd);
DECL_PRE(termp_ns);
DECL_PRE(termp_nx);
DECL_PRE(termp_ox);
DECL_PRE(termp_pp);
DECL_PRE(termp_rv);
DECL_PRE(termp_st);
DECL_PRE(termp_ud);
DECL_PRE(termp_ux);
DECL_PRE(termp_xr);

DECL_POST(termp_bl);

const	struct termact __termacts[MDOC_MAX] = {
	{ NULL, NULL }, /* \" */
	{ NULL, NULL }, /* Dd */
	{ NULL, NULL }, /* Dt */
	{ NULL, NULL }, /* Os */
	{ termp_sh_pre, termp_sh_post }, /* Sh */
	{ termp_ss_pre, termp_ss_post }, /* Ss */ 
	{ termp_pp_pre, NULL }, /* Pp */ 
	{ termp_d1_pre, termp_d1_post }, /* D1 */
	{ NULL, NULL }, /* Dl */
	{ termp_bd_pre, termp_bd_post }, /* Bd */
	{ NULL, NULL }, /* Ed */
	{ NULL, termp_bl_post }, /* Bl */
	{ NULL, NULL }, /* El */
	{ termp_it_pre, termp_it_post }, /* It */
	{ NULL, NULL }, /* Ad */ 
	{ NULL, NULL }, /* An */
	{ termp_ar_pre, termp_ar_post }, /* Ar */
	{ termp_cd_pre, termp_cd_post }, /* Cd */
	{ termp_cm_pre, termp_cm_post }, /* Cm */
	{ NULL, NULL }, /* Dv */ 
	{ NULL, NULL }, /* Er */ 
	{ NULL, NULL }, /* Ev */ 
	{ termp_ex_pre, NULL }, /* Ex */
	{ termp_fa_pre, termp_fa_post }, /* Fa */ 
	{ termp_fd_pre, termp_fd_post }, /* Fd */ 
	{ termp_fl_pre, termp_fl_post }, /* Fl */
	{ termp_fn_pre, termp_fn_post }, /* Fn */ 
	{ termp_ft_pre, termp_ft_post }, /* Ft */ 
	{ termp_ic_pre, termp_ic_post }, /* Ic */ 
	{ termp_in_pre, termp_in_post }, /* In */ 
	{ NULL, NULL }, /* Li */
	{ termp_nd_pre, NULL }, /* Nd */ 
	{ termp_nm_pre, termp_nm_post }, /* Nm */ 
	{ termp_op_pre, termp_op_post }, /* Op */
	{ NULL, NULL }, /* Ot */
	{ termp_pa_pre, termp_pa_post }, /* Pa */
	{ termp_rv_pre, NULL }, /* Rv */
	{ termp_st_pre, NULL }, /* St */ 
	{ termp_va_pre, termp_va_post }, /* Va */
	{ termp_vt_pre, termp_vt_post }, /* Vt */ 
	{ termp_xr_pre, NULL }, /* Xr */
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
	{ termp_aq_pre, termp_aq_post }, /* Ao */
	{ termp_aq_pre, termp_aq_post }, /* Aq */
	{ termp_at_pre, NULL }, /* At */
	{ NULL, NULL }, /* Bc */
	{ termp_bf_pre, termp_bf_post }, /* Bf */ 
	{ termp_bq_pre, termp_bq_post }, /* Bo */
	{ termp_bq_pre, termp_bq_post }, /* Bq */
	{ termp_bsx_pre, NULL }, /* Bsx */
	{ termp_bx_pre, NULL }, /* Bx */
	{ NULL, NULL }, /* Db */
	{ NULL, NULL }, /* Dc */
	{ termp_dq_pre, termp_dq_post }, /* Do */
	{ termp_dq_pre, termp_dq_post }, /* Dq */
	{ NULL, NULL }, /* Ec */
	{ NULL, NULL }, /* Ef */
	{ termp_em_pre, termp_em_post }, /* Em */ 
	{ NULL, NULL }, /* Eo */
	{ termp_fx_pre, NULL }, /* Fx */
	{ termp_ms_pre, termp_ms_post }, /* Ms */
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
	{ NULL, NULL }, /* Rs */
	{ NULL, NULL }, /* Sc */
	{ termp_sq_pre, termp_sq_post }, /* So */
	{ termp_sq_pre, termp_sq_post }, /* Sq */
	{ NULL, NULL }, /* Sm */
	{ termp_sx_pre, termp_sx_post }, /* Sx */
	{ termp_sy_pre, termp_sy_post }, /* Sy */
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

	/* TODO */
	assert(*arg->value);
	return(strlen(*arg->value));
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
static void
termp_it_post(DECL_ARGS)
{
	const struct mdoc_node *n, *it;
	const struct mdoc_block *bl;
	int		 i;
	size_t		 width, offset;

	/*
	 * This (and termp_it_pre()) are the most complicated functions
	 * here.  They must account for a considerable number of
	 * switches that completely change the output behaviour, like
	 * -tag versus -column.  Yech.
	 */

	switch (node->type) {
	case (MDOC_BODY):
		/* FALLTHROUGH */
	case (MDOC_HEAD):
		break;
	default:
		return;
	}

	it = node->parent;
	assert(MDOC_BLOCK == it->type);
	assert(MDOC_It == it->tok);

	n = it->parent;
	assert(MDOC_BODY == n->type);
	assert(MDOC_Bl == n->tok);
	n = n->parent;
	bl = &n->data.block;

	/* If `-tag', adjust our margins accordingly. */

	if (arg_hasattr(MDOC_Tag, bl->argc, bl->argv)) {
		flushln(p);

		/* FIXME: this should auto-size. */
		i = arg_getattr(MDOC_Width, bl->argc, bl->argv);
		width = i >= 0 ? arg_width(&bl->argv[i]) : 10;

		/* FIXME: nesting!  Should happen at block. */
		i = arg_getattr(MDOC_Offset, bl->argc, bl->argv);
		offset = i >= 0 ? arg_width(&bl->argv[i]) : 0;

		if (MDOC_HEAD == node->type) {
			p->rmargin = p->maxrmargin;
			p->offset -= offset;
			p->flags &= ~TERMP_NOBREAK;
		} else {
			p->offset -= width;
			p->flags &= ~TERMP_NOLPAD;
		}
	}

	if (arg_hasattr(MDOC_Ohang, bl->argc, bl->argv)) {
		i = arg_getattr(MDOC_Offset, bl->argc, bl->argv);
		offset = i >= 0 ? arg_offset(&bl->argv[i]) : 0;

		flushln(p);
		p->offset -= offset;
		return;
	}
}


/* ARGSUSED */
static int
termp_it_pre(DECL_ARGS)
{
	const struct mdoc_node *n, *it;
	const struct mdoc_block *bl;
	int		 i;
	size_t		 width, offset;

	/*
	 * Also see termp_it_post() for general comments.
	 */

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

	assert(MDOC_BLOCK == it->type);
	assert(MDOC_It == it->tok);

	n = it->parent;
	assert(MDOC_BODY == n->type);
	assert(MDOC_Bl == n->tok);
	n = n->parent;
	bl = &n->data.block;

	/* If `-compact', don't assert vertical space. */

	if (MDOC_BLOCK == node->type) {
		if (arg_hasattr(MDOC_Compact, bl->argc, bl->argv))
			newln(p);
		else
			vspace(p);
		return(1);
	}

	assert(MDOC_HEAD == node->type 
			|| MDOC_BODY == node->type);

	/* FIXME: see termp_it_post(). */

	/* If `-tag', adjust our margins accordingly. */

	if (arg_hasattr(MDOC_Tag, bl->argc, bl->argv)) {
		p->flags |= TERMP_NOSPACE;

		i = arg_getattr(MDOC_Width, bl->argc, bl->argv);
		width = i >= 0 ? arg_width(&bl->argv[i]) : 10;

		i = arg_getattr(MDOC_Offset, bl->argc, bl->argv);
		offset = i >= 0 ? arg_offset(&bl->argv[i]) : 0;

		if (MDOC_HEAD == node->type) {
			p->flags |= TERMP_NOBREAK;
			p->offset += offset;
			p->rmargin = p->offset + width;
		} else {
			p->flags |= TERMP_NOSPACE;
			p->flags |= TERMP_NOLPAD;
			p->offset += width;
		}
		return(1);
	}

	/* If `-ohang', adjust left-margin. */

	if (arg_hasattr(MDOC_Ohang, bl->argc, bl->argv)) {
		i = arg_getattr(MDOC_Offset, bl->argc, bl->argv);
		offset = i >= 0 ? arg_offset(&bl->argv[i]) : 0;

		p->flags |= TERMP_NOSPACE;
		p->offset += offset;
		return(1);
	}

	return(1);
}


/* ARGSUSED */
static void
termp_nm_post(DECL_ARGS)
{

	p->flags &= ~ttypes[TTYPE_PROG];
}


/* ARGSUSED */
static void
termp_fl_post(DECL_ARGS)
{

	p->flags &= ~ttypes[TTYPE_CMD_FLAG];
}


/* ARGSUSED */
static int
termp_ar_pre(DECL_ARGS)
{

	p->flags |= ttypes[TTYPE_CMD_ARG];
	if (NULL == node->child)
		word(p, "...");
	return(1);
}


/* ARGSUSED */
static int
termp_nm_pre(DECL_ARGS)
{

	p->flags |= ttypes[TTYPE_PROG];
	if (NULL == node->child)
		word(p, meta->name);
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
static void
termp_ar_post(DECL_ARGS)
{

	p->flags &= ~ttypes[TTYPE_CMD_ARG];
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
static void
termp_sh_post(DECL_ARGS)
{

	switch (node->type) {
	case (MDOC_HEAD):
		p->flags &= ~ttypes[TTYPE_SECTION];
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
	p->flags |= ttypes[TTYPE_VAR_DECL];
	return(1);
}


/* ARGSUSED */
static void
termp_vt_post(DECL_ARGS)
{

	p->flags &= ~ttypes[TTYPE_VAR_DECL];
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
	p->flags |= ttypes[TTYPE_FUNC_DECL];
	return(1);
}


/* ARGSUSED */
static void
termp_fd_post(DECL_ARGS)
{

	p->flags &= ~ttypes[TTYPE_FUNC_DECL];
	if (node->sec == SEC_SYNOPSIS)
		vspace(p);

}


/* ARGSUSED */
static int
termp_sh_pre(DECL_ARGS)
{

	switch (node->type) {
	case (MDOC_HEAD):
		vspace(p);
		p->flags |= ttypes[TTYPE_SECTION];
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
termp_fl_pre(DECL_ARGS)
{

	p->flags |= ttypes[TTYPE_CMD_FLAG];
	word(p, "\\-");
	p->flags |= TERMP_NOSPACE;
	return(1);
}


/* ARGSUSED */
static int
termp_d1_pre(DECL_ARGS)
{

	if (MDOC_BODY != node->type)
		return(1);
	newln(p);
	p->offset += INDENT;
	return(1);
}


/* ARGSUSED */
static void
termp_d1_post(DECL_ARGS)
{

	if (MDOC_BODY != node->type) 
		return;
	newln(p);
	p->offset -= INDENT;
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

	p->flags |= ttypes[TTYPE_FUNC_TYPE];
	return(1);
}


/* ARGSUSED */
static void
termp_ft_post(DECL_ARGS)
{

	p->flags &= ~ttypes[TTYPE_FUNC_TYPE];
	if (node->sec == SEC_SYNOPSIS)
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

	if (node->sec == SEC_SYNOPSIS)
		vspace(p);

}


/* ARGSUSED */
static int
termp_sx_pre(DECL_ARGS)
{

	p->flags |= ttypes[TTYPE_LINK];
	return(1);
}


/* ARGSUSED */
static void
termp_sx_post(DECL_ARGS)
{

	p->flags &= ~ttypes[TTYPE_LINK];
}


/* ARGSUSED */
static int
termp_fa_pre(DECL_ARGS)
{
	struct mdoc_node *n;

	if (node->parent->tok != MDOC_Fo) {
		p->flags |= ttypes[TTYPE_FUNC_ARG];
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
static void
termp_fa_post(DECL_ARGS)
{

	p->flags &= ~ttypes[TTYPE_FUNC_ARG];
}


/* ARGSUSED */
static int
termp_va_pre(DECL_ARGS)
{

	p->flags |= ttypes[TTYPE_VAR_DECL];
	return(1);
}


/* ARGSUSED */
static void
termp_va_post(DECL_ARGS)
{

	p->flags &= ~ttypes[TTYPE_VAR_DECL];
}


/* ARGSUSED */
static int
termp_bd_pre(DECL_ARGS)
{
	const struct mdoc_block *bl;
	const struct mdoc_node *n;
	int		 i;

	if (MDOC_BLOCK == node->type) {
		vspace(p);
		return(1);
	} else if (MDOC_BODY != node->type)
		return(1);

	assert(MDOC_BLOCK == node->parent->type);

	bl = &node->parent->data.block;

	i = arg_getattr(MDOC_Offset, bl->argc, bl->argv);
	if (-1 != i) {
		assert(1 == bl->argv[i].sz);
		p->offset += arg_offset(&bl->argv[i]);
	}

	if ( ! arg_hasattr(MDOC_Literal, bl->argc, bl->argv))
		return(1);

	p->flags |= TERMP_LITERAL;

	for (n = node->child; n; n = n->next) {
		assert(MDOC_TEXT == n->type); /* FIXME */
		if ((*n->data.text.string)) {
			word(p, n->data.text.string);
			flushln(p);
		} else
			vspace(p);

	}

	p->flags &= ~TERMP_LITERAL;
	return(0);
}


/* ARGSUSED */
static void
termp_bd_post(DECL_ARGS)
{
	int		 i;
	const struct mdoc_block *bl;

	if (MDOC_BODY != node->type)
		return;

	assert(MDOC_BLOCK == node->parent->type);
	bl = &node->parent->data.block;

	i = arg_getattr(MDOC_Offset, bl->argc, bl->argv);
	if (-1 != i) {
		assert(1 == bl->argv[i].sz);
		p->offset -= arg_offset(&bl->argv[i]);
	}
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
static int
termp_bx_pre(DECL_ARGS)
{

	word(p, "BSD");
	return(1);
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
	word(p, "\'");
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
		p->flags |= ttypes[TTYPE_SSECTION];
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
		p->flags &= ~ttypes[TTYPE_SSECTION];
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

	p->flags |= ttypes[TTYPE_FILE];
	return(1);
}


/* ARGSUSED */
static void
termp_pa_post(DECL_ARGS)
{

	p->flags &= ~ttypes[TTYPE_FILE];
}


/* ARGSUSED */
static int
termp_em_pre(DECL_ARGS)
{

	p->flags |= ttypes[TTYPE_EMPH];
	return(1);
}


/* ARGSUSED */
static void
termp_em_post(DECL_ARGS)
{

	p->flags &= ~ttypes[TTYPE_EMPH];
}


/* ARGSUSED */
static int
termp_cd_pre(DECL_ARGS)
{

	p->flags |= ttypes[TTYPE_CONFIG];
	return(1);
}


/* ARGSUSED */
static void
termp_cd_post(DECL_ARGS)
{

	p->flags &= ~ttypes[TTYPE_CONFIG];
}


/* ARGSUSED */
static int
termp_cm_pre(DECL_ARGS)
{

	p->flags |= ttypes[TTYPE_CMD_FLAG];
	return(1);
}


/* ARGSUSED */
static void
termp_cm_post(DECL_ARGS)
{

	p->flags &= ~ttypes[TTYPE_CMD_FLAG];
}


/* ARGSUSED */
static int
termp_ic_pre(DECL_ARGS)
{

	p->flags |= ttypes[TTYPE_CMD];
	return(1);
}


/* ARGSUSED */
static void
termp_ic_post(DECL_ARGS)
{

	p->flags &= ~ttypes[TTYPE_CMD];
}


/* ARGSUSED */
static int
termp_in_pre(DECL_ARGS)
{

	p->flags |= ttypes[TTYPE_INCLUDE];
	return(1);
}


/* ARGSUSED */
static void
termp_in_post(DECL_ARGS)
{

	p->flags &= ~ttypes[TTYPE_INCLUDE];
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
	word(p, "(");
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
			p->flags |= ttypes[TTYPE_EMPH];
		else if (arg_hasattr(MDOC_Symbolic, b->argc, b->argv))
			p->flags |= ttypes[TTYPE_SYMB];

		return(1);
	} 

	assert(MDOC_TEXT == n->type);

	if (0 == strcmp("Em", n->data.text.string))
		p->flags |= ttypes[TTYPE_EMPH];
	else if (0 == strcmp("Sy", n->data.text.string))
		p->flags |= ttypes[TTYPE_SYMB];

	return(1);
}


/* ARGSUSED */
static void
termp_bf_post(DECL_ARGS)
{
	const struct mdoc_node	*n;
	const struct mdoc_block	*b;

	if (MDOC_BLOCK != node->type)
		return;

	b = &node->data.block;

	if (NULL == (n = b->head->child)) {
		if (arg_hasattr(MDOC_Emphasis, b->argc, b->argv))
			p->flags &= ~ttypes[TTYPE_EMPH];
		else if (arg_hasattr(MDOC_Symbolic, b->argc, b->argv))
			p->flags &= ~ttypes[TTYPE_SYMB];

		return;
	} 

	assert(MDOC_TEXT == n->type);

	if (0 == strcmp("Emphasis", n->data.text.string))
		p->flags &= ~ttypes[TTYPE_EMPH];
	else if (0 == strcmp("Symbolic", n->data.text.string))
		p->flags &= ~ttypes[TTYPE_SYMB];

	return;
}


/* ARGSUSED */
static int
termp_sy_pre(DECL_ARGS)
{

	p->flags |= ttypes[TTYPE_SYMB];
	return(1);
}


/* ARGSUSED */
static void
termp_sy_post(DECL_ARGS)
{

	p->flags &= ~ttypes[TTYPE_SYMB];
}


/* ARGSUSED */
static int
termp_ms_pre(DECL_ARGS)
{

	p->flags |= ttypes[TTYPE_SYMBOL];
	return(1);
}


/* ARGSUSED */
static void
termp_ms_post(DECL_ARGS)
{

	p->flags &= ~ttypes[TTYPE_SYMBOL];
}
