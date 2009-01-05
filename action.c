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

typedef int	(*a_act)(struct mdoc *, int, int, 
			int, const char *[],
			int, const struct mdoc_arg *);


struct	actions {
	a_act	 action;
};


const	struct actions mdoc_actions[MDOC_MAX] = {
	{ NULL }, /* \" */
	{ NULL }, /* Dd */ 
	{ NULL }, /* Dt */ 
	{ NULL }, /* Os */ 
	{ NULL }, /* Sh */ 
	{ NULL }, /* Ss */ 
	{ NULL }, /* Pp */ 
	{ NULL }, /* D1 */
	{ NULL }, /* Dl */
	{ NULL }, /* Bd */ 
	{ NULL }, /* Ed */
	{ NULL }, /* Bl */ 
	{ NULL }, /* El */
	{ NULL }, /* It */
	{ NULL }, /* Ad */ 
	{ NULL }, /* An */
	{ NULL }, /* Ar */
	{ NULL }, /* Cd */
	{ NULL }, /* Cm */
	{ NULL }, /* Dv */ 
	{ NULL }, /* Er */ 
	{ NULL }, /* Ev */ 
	{ NULL }, /* Ex */
	{ NULL }, /* Fa */ 
	{ NULL }, /* Fd */ 
	{ NULL }, /* Fl */
	{ NULL }, /* Fn */ 
	{ NULL }, /* Ft */ 
	{ NULL }, /* Ic */ 
	{ NULL }, /* In */ 
	{ NULL }, /* Li */
	{ NULL }, /* Nd */ 
	{ NULL }, /* Nm */ 
	{ NULL }, /* Op */
	{ NULL }, /* Ot */
	{ NULL }, /* Pa */
	{ NULL }, /* Rv */
	{ NULL }, /* St */
	{ NULL }, /* Va */
	{ NULL }, /* Vt */ 
	{ NULL }, /* Xr */
	{ NULL }, /* %A */
	{ NULL }, /* %B */
	{ NULL }, /* %D */
	{ NULL }, /* %I */
	{ NULL }, /* %J */
	{ NULL }, /* %N */
	{ NULL }, /* %O */
	{ NULL }, /* %P */
	{ NULL }, /* %R */
	{ NULL }, /* %T */
	{ NULL }, /* %V */
	{ NULL }, /* Ac */
	{ NULL }, /* Ao */
	{ NULL }, /* Aq */
	{ NULL }, /* At */ 
	{ NULL }, /* Bc */
	{ NULL }, /* Bf */ 
	{ NULL }, /* Bo */
	{ NULL }, /* Bq */
	{ NULL }, /* Bsx */
	{ NULL }, /* Bx */
	{ NULL }, /* Db */
	{ NULL }, /* Dc */
	{ NULL }, /* Do */
	{ NULL }, /* Dq */
	{ NULL }, /* Ec */
	{ NULL }, /* Ef */
	{ NULL }, /* Em */ 
	{ NULL }, /* Eo */
	{ NULL }, /* Fx */
	{ NULL }, /* Ms */
	{ NULL }, /* No */
	{ NULL }, /* Ns */
	{ NULL }, /* Nx */
	{ NULL }, /* Ox */
	{ NULL }, /* Pc */
	{ NULL }, /* Pf */
	{ NULL }, /* Po */
	{ NULL }, /* Pq */
	{ NULL }, /* Qc */
	{ NULL }, /* Ql */
	{ NULL }, /* Qo */
	{ NULL }, /* Qq */
	{ NULL }, /* Re */
	{ NULL }, /* Rs */
	{ NULL }, /* Sc */
	{ NULL }, /* So */
	{ NULL }, /* Sq */
	{ NULL }, /* Sm */
	{ NULL }, /* Sx */
	{ NULL }, /* Sy */
	{ NULL }, /* Tn */
	{ NULL }, /* Ux */
	{ NULL }, /* Xc */
	{ NULL }, /* Xo */
	{ NULL }, /* Fo */ 
	{ NULL }, /* Fc */ 
	{ NULL }, /* Oo */
	{ NULL }, /* Oc */
	{ NULL }, /* Bk */
	{ NULL }, /* Ek */
	{ NULL }, /* Bt */
	{ NULL }, /* Hf */
	{ NULL }, /* Fr */
	{ NULL }, /* Ud */
};


int
mdoc_action(struct mdoc *mdoc, int tok, int pos)
{

	return(1);
}

#if 0
	/* Post-processing. */
	switch (tok) {
	case (MDOC_Sh):
		sec = mdoc_atosec((size_t)sz, _CC(args));
		if (SEC_CUSTOM != sec)
			mdoc->sec_lastn = sec;
		mdoc->sec_last = sec;
		break;
	default:
		break;
	}
#endif

