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

#define	DATESIZ	  32
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

static	int	  concat(struct mdoc *, char *,
			const struct mdoc_node *, size_t);

static	int	  post_dd(POST_ARGS);
static	int	  post_dt(POST_ARGS);
static	int	  post_os(POST_ARGS);
static	int	  post_prol(POST_ARGS);
static	int	  post_std(POST_ARGS);

static	const struct actions mdoc_actions[MDOC_MAX] = {
	{ NULL, NULL }, /* Ap */
	{ NULL, post_dd }, /* Dd */ 
	{ NULL, post_dt }, /* Dt */ 
	{ NULL, post_os }, /* Os */ 
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
	{ NULL, post_std }, /* Ex */
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
	{ NULL, post_std }, /* Rv */
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


/*
 * Concatenate sibling nodes together.  All siblings must be of type
 * MDOC_TEXT or an assertion is raised.  Concatenation is separated by a
 * single whitespace.
 */
static int
concat(struct mdoc *m, char *p, const struct mdoc_node *n, size_t sz)
{

	assert(sz);
	p[0] = '\0';
	for ( ; n; n = n->next) {
		assert(MDOC_TEXT == n->type);
		/*
		 * XXX: yes, these can technically be resized, but it's
		 * highly unlikely that we're going to get here, so let
		 * it slip for now.
		 */
		if (strlcat(p, n->string, sz) >= sz) {
			mdoc_nmsg(m, n, MANDOCERR_MEM);
			return(0);
		}
		if (NULL == n->next)
			continue;
		if (strlcat(p, " ", sz) >= sz) {
			mdoc_nmsg(m, n, MANDOCERR_MEM);
			return(0);
		}
	}

	return(1);
}


/*
 * Macros accepting `-std' as an argument have the name of the current
 * document (`Nm') filled in as the argument if it's not provided.
 */
static int
post_std(POST_ARGS)
{
	struct mdoc_node *nn;

	if (n->child)
		return(1);
	if (NULL == m->meta.name)
		return(1);
	
	nn = n;
	m->next = MDOC_NEXT_CHILD;

	if ( ! mdoc_word_alloc(m, n->line, n->pos, m->meta.name))
		return(0);
	m->last = nn;
	return(1);
}

/*
 * Parse out the contents of `Dt'.  See in-line documentation for how we
 * handle the various fields of this macro.
 */
static int
post_dt(POST_ARGS)
{
	struct mdoc_node *nn;
	const char	 *cp;

	if (m->meta.title)
		free(m->meta.title);
	if (m->meta.vol)
		free(m->meta.vol);
	if (m->meta.arch)
		free(m->meta.arch);

	m->meta.title = m->meta.vol = m->meta.arch = NULL;
	/* Handles: `.Dt' 
	 *   --> title = unknown, volume = local, msec = 0, arch = NULL
	 */

	if (NULL == (nn = n->child)) {
		/* XXX: make these macro values. */
		/* FIXME: warn about missing values. */
		m->meta.title = mandoc_strdup("UNKNOWN");
		m->meta.vol = mandoc_strdup("LOCAL");
		m->meta.msec = mandoc_strdup("1");
		return(post_prol(m, n));
	}

	/* Handles: `.Dt TITLE' 
	 *   --> title = TITLE, volume = local, msec = 0, arch = NULL
	 */

	m->meta.title = mandoc_strdup
		('\0' == nn->string[0] ? "UNKNOWN" : nn->string);

	if (NULL == (nn = nn->next)) {
		/* FIXME: warn about missing msec. */
		/* XXX: make this a macro value. */
		m->meta.vol = mandoc_strdup("LOCAL");
		m->meta.msec = mandoc_strdup("1");
		return(post_prol(m, n));
	}

	/* Handles: `.Dt TITLE SEC'
	 *   --> title = TITLE, volume = SEC is msec ? 
	 *           format(msec) : SEC,
	 *       msec = SEC is msec ? atoi(msec) : 0,
	 *       arch = NULL
	 */

	cp = mdoc_a2msec(nn->string);
	if (cp) {
		m->meta.vol = mandoc_strdup(cp);
		m->meta.msec = mandoc_strdup(nn->string);
	} else if (mdoc_nmsg(m, n, MANDOCERR_BADMSEC)) {
		m->meta.vol = mandoc_strdup(nn->string);
		m->meta.msec = mandoc_strdup(nn->string);
	} else
		return(0);

	if (NULL == (nn = nn->next))
		return(post_prol(m, n));

	/* Handles: `.Dt TITLE SEC VOL'
	 *   --> title = TITLE, volume = VOL is vol ?
	 *       format(VOL) : 
	 *           VOL is arch ? format(arch) : 
	 *               VOL
	 */

	cp = mdoc_a2vol(nn->string);
	if (cp) {
		free(m->meta.vol);
		m->meta.vol = mandoc_strdup(cp);
	} else {
		/* FIXME: warn about bad arch. */
		cp = mdoc_a2arch(nn->string);
		if (NULL == cp) {
			free(m->meta.vol);
			m->meta.vol = mandoc_strdup(nn->string);
		} else 
			m->meta.arch = mandoc_strdup(cp);
	}	

	/* Ignore any subsequent parameters... */
	/* FIXME: warn about subsequent parameters. */

	return(post_prol(m, n));
}


/*
 * Set the operating system by way of the `Os' macro.  Note that if an
 * argument isn't provided and -DOSNAME="\"foo\"" is provided during
 * compilation, this value will be used instead of filling in "sysname
 * release" from uname().
 */
static int
post_os(POST_ARGS)
{
	char		  buf[BUFSIZ];
#ifndef OSNAME
	struct utsname	  utsname;
#endif

	if (m->meta.os)
		free(m->meta.os);

	if ( ! concat(m, buf, n->child, BUFSIZ))
		return(0);

	/* XXX: yes, these can all be dynamically-adjusted buffers, but
	 * it's really not worth the extra hackery.
	 */

	if ('\0' == buf[0]) {
#ifdef OSNAME
		if (strlcat(buf, OSNAME, BUFSIZ) >= BUFSIZ) {
			mdoc_nmsg(m, n, MANDOCERR_MEM);
			return(0);
		}
#else /*!OSNAME */
		if (-1 == uname(&utsname))
			return(mdoc_nmsg(m, n, MANDOCERR_UTSNAME));

		if (strlcat(buf, utsname.sysname, BUFSIZ) >= BUFSIZ) {
			mdoc_nmsg(m, n, MANDOCERR_MEM);
			return(0);
		}
		if (strlcat(buf, " ", 64) >= BUFSIZ) {
			mdoc_nmsg(m, n, MANDOCERR_MEM);
			return(0);
		}
		if (strlcat(buf, utsname.release, BUFSIZ) >= BUFSIZ) {
			mdoc_nmsg(m, n, MANDOCERR_MEM);
			return(0);
		}
#endif /*!OSNAME*/
	}

	m->meta.os = mandoc_strdup(buf);
	return(post_prol(m, n));
}

/*
 * Parse the date field in `Dd'.
 */
static int
post_dd(POST_ARGS)
{
	char		buf[DATESIZ];

	if (NULL == n->child) {
		m->meta.date = time(NULL);
		return(post_prol(m, n));
	}

	if ( ! concat(m, buf, n->child, DATESIZ))
		return(0);

	m->meta.date = mandoc_a2time
		(MTIME_MDOCDATE | MTIME_CANONICAL, buf);

	if (0 == m->meta.date) {
		if ( ! mdoc_nmsg(m, n, MANDOCERR_BADDATE))
			return(0);
		m->meta.date = time(NULL);
	}

	return(post_prol(m, n));
}


/*
 * Remove prologue macros from the document after they're processed.
 * The final document uses mdoc_meta for these values and discards the
 * originals.
 */
static int
post_prol(POST_ARGS)
{

	mdoc_node_delete(m, n);
	if (m->meta.title && m->meta.date && m->meta.os)
		m->flags |= MDOC_PBODY;
	return(1);
}
