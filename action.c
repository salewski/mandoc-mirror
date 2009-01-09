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
#include <time.h>

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
	int		  i;
	char		 *p;
	size_t		  sz;
	struct mdoc_node *n;

	assert(MDOC_ELEM == mdoc->last->type);
	assert(MDOC_Dt == mdoc->last->data.elem.tok);
	assert(0 == mdoc->meta.title[0]);

	sz = META_TITLE_SZ;
	(void)xstrlcpy(mdoc->meta.title, "UNTITLED", sz);

	for (i = 0, n = mdoc->last->child; n; n = n->next, i++) {
		assert(MDOC_TEXT == n->type);
		p = n->data.text.string;

		switch (i) {
		case (0):
			if (xstrlcpy(mdoc->meta.title, p, sz))
				break;
			return(mdoc_err(mdoc, ERR_SYNTAX_ARGFORM));
		case (1):
			mdoc->meta.msec = mdoc_atomsec(p);
			if (MSEC_DEFAULT != mdoc->meta.msec)
				break;
			return(mdoc_err(mdoc, ERR_SYNTAX_ARGFORM));
		case (2):
			mdoc->meta.vol = mdoc_atovol(p);
			if (VOL_DEFAULT != mdoc->meta.vol)
				break;
			mdoc->meta.arch = mdoc_atoarch(p);
			if (ARCH_DEFAULT != mdoc->meta.arch)
				break;
			return(mdoc_err(mdoc, ERR_SYNTAX_ARGFORM));
		default:
			return(mdoc_err(mdoc, ERR_ARGS_MANY));
		}
	}

	mdoc_msg(mdoc, "parsed title: %s", mdoc->meta.title);
	/* TODO: print vol2a functions. */
	return(1);
}


static int
post_os(struct mdoc *mdoc)
{
	char		 *p;
	size_t		  sz;
	struct mdoc_node *n;

	assert(MDOC_ELEM == mdoc->last->type);
	assert(MDOC_Os == mdoc->last->data.elem.tok);
	assert(0 == mdoc->meta.os[0]);

	sz = META_OS_SZ;

	for (n = mdoc->last->child; n; n = n->next) {
		assert(MDOC_TEXT == n->type);
		p = n->data.text.string;

		if ( ! xstrlcat(mdoc->meta.os, p, sz))
			return(mdoc_err(mdoc, ERR_SYNTAX_ARGFORM));
		if ( ! xstrlcat(mdoc->meta.os, " ", sz))
			return(mdoc_err(mdoc, ERR_SYNTAX_ARGFORM));
	}

	if (0 == mdoc->meta.os[0]) 
		(void)xstrlcpy(mdoc->meta.os, "LOCAL", sz);

	mdoc_msg(mdoc, "parsed operating system: %s", mdoc->meta.os);
	mdoc->sec_lastn = mdoc->sec_last = SEC_BODY;
	return(1);
}


static int
post_dd(struct mdoc *mdoc)
{
	char		  date[64];
	size_t		  sz;
	char		 *p;
	struct mdoc_node *n;

	assert(MDOC_ELEM == mdoc->last->type);
	assert(MDOC_Dd == mdoc->last->data.elem.tok);

	n = mdoc->last->child; 
	assert(0 == mdoc->meta.date);
	date[0] = 0;

	sz = 64;

	for ( ; 0 == mdoc->meta.date && n; n = n->next) {
		assert(MDOC_TEXT == n->type);
		p = n->data.text.string;

		if (xstrcmp(p, "$Mdocdate$")) {
			mdoc->meta.date = time(NULL);
			continue;
		} else if (xstrcmp(p, "$")) {
			mdoc->meta.date = mdoc_atotime(date);
			continue;
		} else if (xstrcmp(p, "$Mdocdate:"))
			continue;

		if ( ! xstrlcat(date, n->data.text.string, sz))
			return(mdoc_err(mdoc, ERR_SYNTAX_ARGFORM));
		if ( ! xstrlcat(date, " ", sz))
			return(mdoc_err(mdoc, ERR_SYNTAX_ARGFORM));
	}

	if (mdoc->meta.date && NULL == n) {
		mdoc_msg(mdoc, "parsed time: %u since epoch", 
				mdoc->meta.date);
		return(1);
	}

	return(mdoc_err(mdoc, ERR_SYNTAX_ARGFORM));
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
