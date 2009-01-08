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

typedef int	(*a_pre)(struct mdoc *, struct mdoc_node *);
typedef int	(*a_post)(struct mdoc *);


struct	actions {
	a_pre	 pre;
	a_post	 post;
};


static int	 post_sh(struct mdoc *);
static int	 post_os(struct mdoc *);
static int	 post_dt(struct mdoc *);
static int	 post_dd(struct mdoc *);


const	struct actions mdoc_actions[MDOC_MAX] = {
	{ NULL, NULL }, /* \" */
	{ NULL, post_dd }, /* Dd */ 
	{ NULL, post_dt }, /* Dt */ 
	{ NULL, post_os }, /* Os */ 
	{ NULL, post_sh }, /* Sh */ 
	{ NULL, NULL }, /* Ss */ 
	{ NULL, NULL }, /* Pp */ 
	{ NULL, NULL }, /* D1 */
	{ NULL, NULL }, /* Dl */
	{ NULL, NULL }, /* Bd */ 
	{ NULL, NULL }, /* Ed */
	{ NULL, NULL }, /* Bl */ 
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
	{ NULL, NULL }, /* At */ 
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
	{ NULL, NULL }, /* Ef */
	{ NULL, NULL }, /* Em */ 
	{ NULL, NULL }, /* Eo */
	{ NULL, NULL }, /* Fx */
	{ NULL, NULL }, /* Ms */
	{ NULL, NULL }, /* No */
	{ NULL, NULL }, /* Ns */
	{ NULL, NULL }, /* Nx */
	{ NULL, NULL }, /* Ox */
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
	if (SEC_CUSTOM != sec)
		mdoc->sec_lastn = sec;
	mdoc->sec_last = sec;

	return(1);
}


static int
post_dt(struct mdoc *mdoc)
{
#if 0
	int		  lastarg, j;
	char		 *args[MDOC_LINEARG_MAX];

	if (SEC_PROLOGUE != mdoc->sec_lastn)
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_NPROLOGUE));
	if (0 == mdoc->meta.date)
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_PROLOGUE_OO));
	if (mdoc->meta.title[0])
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_PROLOGUE_REP));

	j = -1;
	lastarg = ppos;

again:
	if (j == MDOC_LINEARG_MAX)
		return(mdoc_err(mdoc, tok, lastarg, ERR_ARGS_MANY));

	lastarg = *pos;

	switch (mdoc_args(mdoc, tok, pos, buf, 0, &args[++j])) {
	case (ARGS_EOLN):
		if (mdoc->meta.title)
			return(1);
		if ( ! mdoc_warn(mdoc, tok, ppos, WARN_ARGS_GE1))
			return(0);
		(void)xstrlcpy(mdoc->meta.title, 
				"UNTITLED", META_TITLE_SZ);
		return(1);
	case (ARGS_ERROR):
		return(0);
	default:
		break;
	}

	if (0 == j) {
		if (xstrlcpy(mdoc->meta.title, args[0], META_TITLE_SZ))
			goto again;
		return(mdoc_err(mdoc, tok, lastarg, ERR_SYNTAX_ARGFORM));

	} else if (1 == j) {
		mdoc->meta.msec = mdoc_atomsec(args[1]);
		if (MSEC_DEFAULT != mdoc->meta.msec)
			goto again;
		return(mdoc_err(mdoc, tok, -1, ERR_SYNTAX_ARGFORM));

	} else if (2 == j) {
		mdoc->meta.vol = mdoc_atovol(args[2]);
		if (VOL_DEFAULT != mdoc->meta.vol)
			goto again;
		mdoc->meta.arch = mdoc_atoarch(args[2]);
		if (ARCH_DEFAULT != mdoc->meta.arch)
			goto again;
		return(mdoc_err(mdoc, tok, lastarg, ERR_SYNTAX_ARGFORM));
	}

	return(mdoc_err(mdoc, tok, lastarg, ERR_ARGS_MANY));
#endif
	return(1);
}


static int
post_os(struct mdoc *mdoc)
{
#if 0
	int		  lastarg, j;
	char		 *args[MDOC_LINEARG_MAX];

	/* FIXME: if we use `Os' again... ? */

	if (SEC_PROLOGUE != mdoc->sec_lastn)
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_NPROLOGUE));
	if (0 == mdoc->meta.title[0])
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_PROLOGUE_OO));
	if (mdoc->meta.os[0])
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_PROLOGUE_REP));

	j = -1;
	lastarg = ppos;

again:
	if (j == MDOC_LINEARG_MAX)
		return(mdoc_err(mdoc, tok, lastarg, ERR_ARGS_MANY));

	lastarg = *pos;

	switch (mdoc_args(mdoc, tok, pos, buf, 
				ARGS_QUOTED, &args[++j])) {
	case (ARGS_EOLN):
		mdoc->sec_lastn = mdoc->sec_last = SEC_BODY;
		return(1);
	case (ARGS_ERROR):
		return(0);
	default:
		break;
	}
	
	if ( ! xstrlcat(mdoc->meta.os, args[j], sizeof(mdoc->meta.os)))
		return(mdoc_err(mdoc, tok, lastarg, ERR_SYNTAX_ARGFORM));
	if ( ! xstrlcat(mdoc->meta.os, " ", sizeof(mdoc->meta.os)))
		return(mdoc_err(mdoc, tok, lastarg, ERR_SYNTAX_ARGFORM));

	goto again;
	/* NOTREACHED */
#endif
	return(1);
}


static int
post_dd(struct mdoc *mdoc)
{
#if 0
	int		  lastarg, j;
	char		 *args[MDOC_LINEARG_MAX], date[64];

	if (SEC_PROLOGUE != mdoc->sec_lastn)
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_NPROLOGUE));
	if (mdoc->meta.title[0])
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_PROLOGUE_OO));
	if (mdoc->meta.date)
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_PROLOGUE_REP));

	j = -1;
	date[0] = 0;
	lastarg = ppos;

again:
	if (j == MDOC_LINEARG_MAX)
		return(mdoc_err(mdoc, tok, lastarg, ERR_ARGS_MANY));

	lastarg = *pos;
	switch (mdoc_args(mdoc, tok, pos, buf, 0, &args[++j])) {
	case (ARGS_EOLN):
		if (mdoc->meta.date)
			return(1);
		mdoc->meta.date = mdoc_atotime(date);
		if (mdoc->meta.date)
			return(1);
		return(mdoc_err(mdoc, tok, ppos, ERR_SYNTAX_ARGFORM));
	case (ARGS_ERROR):
		return(0);
	default:
		break;
	}
	
	if (MDOC_MAX != mdoc_find(mdoc, args[j]) && ! mdoc_warn
			(mdoc, tok, lastarg, WARN_SYNTAX_MACLIKE))
		return(0);
	
	if (0 == j) {
		if (xstrcmp("$Mdocdate$", args[j])) {
			mdoc->meta.date = time(NULL);
			goto again;
		} else if (xstrcmp("$Mdocdate:", args[j])) 
			goto again;
	} else if (4 == j)
		if ( ! xstrcmp("$", args[j]))
			goto again;

	if ( ! xstrlcat(date, args[j], sizeof(date)))
		return(mdoc_err(mdoc, tok, lastarg, ERR_SYNTAX_ARGFORM));
	if ( ! xstrlcat(date, " ", sizeof(date)))
		return(mdoc_err(mdoc, tok, lastarg, ERR_SYNTAX_ARGFORM));

	goto again;
	/* NOTREACHED */
#endif
	return(1);
}


int
mdoc_action_pre(struct mdoc *mdoc, struct mdoc_node *node)
{

	return(1);
}


int
mdoc_action_post(struct mdoc *mdoc)
{
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

	if (NULL == mdoc_actions[t].post)
		return(1);
	/* TODO: MDOC_Nm... ? */
	return((*mdoc_actions[t].post)(mdoc));
}
