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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "private.h"

/*
 * Actions are executed on macros after they've been post-validated: in
 * other words, a macro will not be "acted upon" until all of its
 * children have been filled in (post-fix order).
 */

struct	actions {
	int	(*post)(struct mdoc *);
};

/* Per-macro action routines. */

static	int	 post_bl(struct mdoc *);
static	int	 post_sh(struct mdoc *);
static	int	 post_os(struct mdoc *);
static	int	 post_dt(struct mdoc *);
static	int	 post_dd(struct mdoc *);
static	int	 post_nm(struct mdoc *);

static	int	 post_prologue(struct mdoc *);

/* Array of macro action routines. */

const	struct actions mdoc_actions[MDOC_MAX] = {
	{ NULL }, /* \" */
	{ post_dd }, /* Dd */ 
	{ post_dt }, /* Dt */ 
	{ post_os }, /* Os */ 
	{ post_sh }, /* Sh */ 
	{ NULL }, /* Ss */ 
	{ NULL }, /* Pp */ 
	{ NULL }, /* D1 */
	{ NULL }, /* Dl */
	{ NULL }, /* Bd */ 
	{ NULL }, /* Ed */
	{ post_bl }, /* Bl */ 
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
	{ post_nm }, /* Nm */ 
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


/*
 * The `Nm' macro sets the document's name when used the first time with
 * an argument.  Subsequent calls without a value will result in the
 * name value being used.
 */
static int
post_nm(struct mdoc *mdoc)
{
	char		 buf[64];

	assert(MDOC_ELEM == mdoc->last->type);
	assert(MDOC_Nm == mdoc->last->tok);

	if (mdoc->meta.name)
		return(1);

	if (xstrlcats(buf, mdoc->last->child, 64)) {
		mdoc->meta.name = xstrdup(buf);
		return(1);
	}

	return(mdoc_err(mdoc, "macro parameters too long"));
}


/*
 * We keep track of the current section in order to provide warnings on
 * section ordering, per-section macros, and so on.
 */
static int
post_sh(struct mdoc *mdoc)
{
	enum mdoc_sec	 sec;
	char		 buf[64];

	if (MDOC_HEAD != mdoc->last->type)
		return(1);
	if (xstrlcats(buf, mdoc->last->child, 64)) {
		if (SEC_CUSTOM != (sec = mdoc_atosec(buf)))
			mdoc->lastnamed = sec;
		mdoc->lastsec = sec;
		return(1);
	}

	return(mdoc_err(mdoc, "macro parameters too long"));
}


/* 
 * Prologue title must be parsed into document meta-data.
 */
static int
post_dt(struct mdoc *mdoc)
{
	int		  i;
	char		 *p;
	struct mdoc_node *n;

	assert(MDOC_ELEM == mdoc->last->type);
	assert(MDOC_Dt == mdoc->last->tok);

	assert(NULL == mdoc->meta.title);

	/* LINTED */
	for (i = 0, n = mdoc->last->child; n; n = n->next, i++) {
		assert(MDOC_TEXT == n->type);
		p = n->data.text.string;

		switch (i) {
		case (0):
			mdoc->meta.title = xstrdup(p);
			break;
		case (1):
			mdoc->meta.msec = mdoc_atomsec(p);
			if (MSEC_DEFAULT != mdoc->meta.msec)
				break;
			return(mdoc_nerr(mdoc, n, "invalid parameter syntax"));
		case (2):
			mdoc->meta.vol = mdoc_atovol(p);
			if (VOL_DEFAULT != mdoc->meta.vol)
				break;
			mdoc->meta.arch = mdoc_atoarch(p);
			if (ARCH_DEFAULT != mdoc->meta.arch)
				break;
			return(mdoc_nerr(mdoc, n, "invalid parameter syntax"));
		default:
			return(mdoc_nerr(mdoc, n, "too many parameters"));
		}
	}

	if (NULL == mdoc->meta.title)
		mdoc->meta.title = xstrdup("UNTITLED");

	mdoc_msg(mdoc, "title: %s", mdoc->meta.title);

	return(post_prologue(mdoc));
}


/* 
 * Prologue operating system must be parsed into document meta-data.
 */
static int
post_os(struct mdoc *mdoc)
{
	char		  buf[64];

	assert(MDOC_ELEM == mdoc->last->type);
	assert(MDOC_Os == mdoc->last->tok);
	assert(NULL == mdoc->meta.os);

	if ( ! xstrlcats(buf, mdoc->last->child, 64))
		return(mdoc_err(mdoc, "macro parameters too long")); 

	mdoc->meta.os = xstrdup(buf[0] ? buf : "local");
	mdoc->lastnamed = SEC_BODY;

	return(post_prologue(mdoc));
}


/* 
 * Transform -width MACRO values into real widths. 
 */
static int
post_bl(struct mdoc *mdoc)
{
	struct mdoc_block *bl;
	size_t		   width;
	int		   tok, i;
	char		   buf[32];

	if (MDOC_BLOCK != mdoc->last->type)
		return(1);

	bl = &mdoc->last->data.block;

	for (i = 0; i < (int)bl->argc; i++)
		if (MDOC_Width == bl->argv[i].arg)
			break;

	if (i == (int)bl->argc)
		return(1);

	assert(1 == bl->argv[i].sz);
	if (MDOC_MAX == (tok = mdoc_find(mdoc, *bl->argv[i].value)))
		return(1);

	if (0 == (width = mdoc_macro2len(tok))) 
		return(mdoc_warn(mdoc, WARN_SYNTAX,
					"-%s macro has no length", 
					mdoc_argnames[MDOC_Width]));

	mdoc_msg(mdoc, "re-writing %s argument: %s -> %zun", 
			mdoc_argnames[MDOC_Width],
			*bl->argv[i].value, width);

	/* FIXME: silently truncates. */
	(void)snprintf(buf, sizeof(buf), "%zun", width);

	free(*bl->argv[i].value);
	*bl->argv[i].value = strdup(buf);

	return(1);
}


/* 
 * Prologue date must be parsed into document meta-data.
 */
static int
post_dd(struct mdoc *mdoc)
{
	char		  buf[64];

	assert(MDOC_ELEM == mdoc->last->type);
	assert(MDOC_Dd == mdoc->last->tok);

	assert(0 == mdoc->meta.date);

	if ( ! xstrlcats(buf, mdoc->last->child, 64))
		return(mdoc_err(mdoc, "macro parameters too long"));
	if (0 == (mdoc->meta.date = mdoc_atotime(buf)))
		return(mdoc_err(mdoc, "invalid parameter syntax"));

	mdoc_msg(mdoc, "date: %u", mdoc->meta.date);

	return(post_prologue(mdoc));
}


/*
 * The end document shouldn't have the prologue macros as part of the
 * syntax tree (they encompass only meta-data). 
 */
static int
post_prologue(struct mdoc *mdoc)
{
	struct mdoc_node *n;

	if (mdoc->last->parent->child == mdoc->last)
		mdoc->last->parent->child = mdoc->last->prev;
	if (mdoc->last->prev)
		mdoc->last->prev->next = NULL;

	n = mdoc->last;
	assert(NULL == mdoc->last->next);

	if (mdoc->last->prev) {
		mdoc->last = mdoc->last->prev;
		mdoc->next = MDOC_NEXT_SIBLING;
	} else {
		mdoc->last = mdoc->last->parent;
		mdoc->next = MDOC_NEXT_CHILD;
	}

	mdoc_node_freelist(n);
	return(1);
}


int
mdoc_action_post(struct mdoc *mdoc)
{

	if (MDOC_ACTED & mdoc->last->flags)
		return(1);
	mdoc->last->flags |= MDOC_ACTED;

	if (MDOC_TEXT == mdoc->last->type)
		return(1);
	if (MDOC_ROOT == mdoc->last->type)
		return(1);
	if (NULL == mdoc_actions[mdoc->last->tok].post)
		return(1);
	return((*mdoc_actions[mdoc->last->tok].post)(mdoc));
}
