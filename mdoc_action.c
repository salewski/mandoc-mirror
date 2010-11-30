/*	$Id$ */
/*
 * Copyright (c) 2008, 2009, 2010 Kristaps Dzonsons <kristaps@bsd.lv>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef	OSNAME
#include <sys/utsname.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mandoc.h"
#include "libmdoc.h"
#include "libmandoc.h"

/* 
 * FIXME: this file is deprecated.  All future "actions" should be
 * pushed into mdoc_validate.c.
 */

#define	POST_ARGS struct mdoc *m, struct mdoc_node *n
#define	PRE_ARGS  struct mdoc *m, struct mdoc_node *n

struct	actions {
	int	(*pre)(PRE_ARGS);
	int	(*post)(POST_ARGS);
};

static	const struct actions mdoc_actions[MDOC_MAX] = {
	{ NULL, NULL }, /* Ap */
	{ NULL, NULL }, /* Dd */ 
	{ NULL, NULL }, /* Dt */ 
	{ NULL, NULL }, /* Os */ 
	{ NULL, NULL }, /* Sh */ 
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
	{ NULL, NULL }, /* Lb */
	{ NULL, NULL }, /* Lp */
	{ NULL, NULL }, /* Lk */
	{ NULL, NULL }, /* Mt */
	{ NULL, NULL }, /* Brq */
	{ NULL, NULL }, /* Bro */
	{ NULL, NULL }, /* Brc */
	{ NULL, NULL }, /* %C */
	{ NULL, NULL }, /* Es */
	{ NULL, NULL }, /* En */
	{ NULL, NULL }, /* Dx */
	{ NULL, NULL }, /* %Q */
	{ NULL, NULL }, /* br */
	{ NULL, NULL }, /* sp */
	{ NULL, NULL }, /* %U */
	{ NULL, NULL }, /* Ta */
};


int
mdoc_action_pre(struct mdoc *m, struct mdoc_node *n)
{

	switch (n->type) {
	case (MDOC_ROOT):
		/* FALLTHROUGH */
	case (MDOC_TEXT):
		return(1);
	default:
		break;
	}

	if (NULL == mdoc_actions[n->tok].pre)
		return(1);
	return((*mdoc_actions[n->tok].pre)(m, n));
}


int
mdoc_action_post(struct mdoc *m)
{

	if (MDOC_ACTED & m->last->flags)
		return(1);
	m->last->flags |= MDOC_ACTED;

	switch (m->last->type) {
	case (MDOC_TEXT):
		/* FALLTHROUGH */
	case (MDOC_ROOT):
		return(1);
	default:
		break;
	}

	if (NULL == mdoc_actions[m->last->tok].post)
		return(1);
	return((*mdoc_actions[m->last->tok].post)(m, m->last));
}

