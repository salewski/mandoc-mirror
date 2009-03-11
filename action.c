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
#include <sys/utsname.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "private.h"

/*
 * Actions are executed on macros after they've been post-validated: in
 * other words, a macro will not be "acted upon" until all of its
 * children have been filled in (post-fix order).
 */

enum	merr {
	ENOWIDTH
};

enum	mwarn {
	WBADSEC,
	WNOWIDTH,
	WBADDATE
};

struct	actions {
	int	(*post)(struct mdoc *);
};

static	int	 nwarn(struct mdoc *, 
			const struct mdoc_node *, enum mwarn);
static	int	 nerr(struct mdoc *, 
			const struct mdoc_node *, enum merr);
static	int	 post_ar(struct mdoc *);
static	int	 post_bl(struct mdoc *);
static	int	 post_bl_width(struct mdoc *);
static	int	 post_bl_tagwidth(struct mdoc *);
static	int	 post_dd(struct mdoc *);
static	int	 post_dt(struct mdoc *);
static	int	 post_nm(struct mdoc *);
static	int	 post_os(struct mdoc *);
static	int	 post_sh(struct mdoc *);
static	int	 post_ex(struct mdoc *);
static	int	 post_prologue(struct mdoc *);

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
	{ post_ar }, /* Ar */
	{ NULL }, /* Cd */
	{ NULL }, /* Cm */
	{ NULL }, /* Dv */ 
	{ NULL }, /* Er */ 
	{ NULL }, /* Ev */ 
	{ post_ex }, /* Ex */
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
	{ NULL }, /* Lb */
	{ NULL }, /* Ap */
	{ NULL }, /* Lp */
	{ NULL }, /* Lk */
	{ NULL }, /* Mt */
	{ NULL }, /* Brq */
	{ NULL }, /* Bro */
	{ NULL }, /* Brc */
};


#define	merr(m, t) nerr((m), (m)->last, (t))
static int
nerr(struct mdoc *m, const struct mdoc_node *n, enum merr type)
{
	char		*p;

	p = NULL;

	switch (type) {
	case (ENOWIDTH):
		p = "missing width argument";
		break;
	}

	assert(p);
	return(mdoc_nerr(m, n, p));
}


#define	mwarn(m, t) nwarn((m), (m)->last, (t))
static int
nwarn(struct mdoc *m, const struct mdoc_node *n, enum mwarn type)
{
	char		*p;
	int		 c;

	p = NULL;
	c = WARN_SYNTAX;

	switch (type) {
	case (WBADSEC):
		p = "inappropriate document section in manual section";
		c = WARN_COMPAT;
		break;
	case (WNOWIDTH):
		p = "cannot determine default width";
		break;
	case (WBADDATE):
		p = "malformed date syntax";
		break;
	}

	assert(p);
	return(mdoc_nwarn(m, n, c, p));
}


static int
post_ex(struct mdoc *mdoc)
{

	/*
	 * If `.Ex -std' is invoked without an argument, fill it in with
	 * our name (if it's been set).
	 */

	if (NULL == mdoc->last->args)
		return(1);
	if (mdoc->last->args->argv[0].sz)
		return(1);

	assert(mdoc->meta.name);

	mdoc_msg(mdoc, "writing %s argument: %s", 
			mdoc_argnames[MDOC_Std], 
			mdoc->meta.name);

	mdoc->last->args->argv[0].value = xcalloc(1, sizeof(char *));
	mdoc->last->args->argv[0].sz = 1;
	mdoc->last->args->argv[0].value[0] = xstrdup(mdoc->meta.name);
	return(1);
}


static int
post_nm(struct mdoc *mdoc)
{
	char		 buf[64];

	if (mdoc->meta.name)
		return(1);

	(void)xstrlcpys(buf, mdoc->last->child, sizeof(buf));
	mdoc->meta.name = xstrdup(buf);
	mdoc_msg(mdoc, "name: %s", mdoc->meta.name);

	return(1);
}


static int
post_sh(struct mdoc *mdoc)
{
	enum mdoc_sec	 sec;
	char		 buf[64];

	/*
	 * We keep track of the current section /and/ the "named"
	 * section, which is one of the conventional ones, in order to
	 * check ordering.
	 */

	if (MDOC_HEAD != mdoc->last->type)
		return(1);

	(void)xstrlcpys(buf, mdoc->last->child, sizeof(buf));
	if (SEC_CUSTOM != (sec = mdoc_atosec(buf)))
		mdoc->lastnamed = sec;

	mdoc->lastsec = sec;

	switch (mdoc->lastsec) {
	case (SEC_RETURN_VALUES):
		/* FALLTHROUGH */
	case (SEC_ERRORS):
		switch (mdoc->meta.msec) {
		case (2):
			/* FALLTHROUGH */
		case (3):
			/* FALLTHROUGH */
		case (9):
			break;
		default:
			return(mwarn(mdoc, WBADSEC));
		}
		break;
	default:
		break;
	}
	return(1);
}


static int
post_dt(struct mdoc *mdoc)
{
	struct mdoc_node *n;
	const char	 *cp;
	char		 *ep;
	long		  lval;

	if (mdoc->meta.title)
		free(mdoc->meta.title);
	if (mdoc->meta.vol)
		free(mdoc->meta.vol);
	if (mdoc->meta.arch)
		free(mdoc->meta.arch);

	mdoc->meta.title = mdoc->meta.vol = mdoc->meta.arch = NULL;
	mdoc->meta.msec = 0;

	/* Handles: `.Dt' 
	 *   --> title = unknown, volume = local, msec = 0, arch = NULL
	 */

	if (NULL == (n = mdoc->last->child)) {
		mdoc->meta.title = xstrdup("unknown");
		mdoc->meta.vol = xstrdup("local");
		mdoc_msg(mdoc, "title: %s", mdoc->meta.title);
		mdoc_msg(mdoc, "volume: %s", mdoc->meta.vol);
		mdoc_msg(mdoc, "arch: <unset>");
		mdoc_msg(mdoc, "msec: <unset>");
		return(post_prologue(mdoc));
	}

	/* Handles: `.Dt TITLE' 
	 *   --> title = TITLE, volume = local, msec = 0, arch = NULL
	 */

	mdoc->meta.title = xstrdup(n->string);
	mdoc_msg(mdoc, "title: %s", mdoc->meta.title);

	if (NULL == (n = n->next)) {
		mdoc->meta.vol = xstrdup("local");
		mdoc_msg(mdoc, "volume: %s", mdoc->meta.vol);
		mdoc_msg(mdoc, "arch: <unset>");
		mdoc_msg(mdoc, "msec: %d", mdoc->meta.msec);
		return(post_prologue(mdoc));
	}

	/* Handles: `.Dt TITLE SEC'
	 *   --> title = TITLE, volume = SEC is msec ? 
	 *           format(msec) : SEC,
	 *       msec = SEC is msec ? atoi(msec) : 0,
	 *       arch = NULL
	 */

	if ((cp = mdoc_a2msec(n->string))) {
		mdoc->meta.vol = xstrdup(cp);
		errno = 0;
		lval = strtol(n->string, &ep, 10);
		if (n->string[0] != '\0' && *ep == '\0')
			mdoc->meta.msec = (int)lval;
	} else 
		mdoc->meta.vol = xstrdup(n->string);

	if (NULL == (n = n->next)) {
		mdoc_msg(mdoc, "volume: %s", mdoc->meta.vol);
		mdoc_msg(mdoc, "arch: <unset>");
		mdoc_msg(mdoc, "msec: %d", mdoc->meta.msec);
		return(post_prologue(mdoc));
	}

	/* Handles: `.Dt TITLE SEC VOL'
	 *   --> title = TITLE, volume = VOL is vol ?
	 *       format(VOL) : 
	 *           VOL is arch ? format(arch) : 
	 *               VOL
	 */

	if ((cp = mdoc_a2vol(n->string))) {
		free(mdoc->meta.vol);
		mdoc->meta.vol = xstrdup(cp);
		n = n->next;
	} else {
		cp = mdoc_a2arch(n->string);
		if (NULL == cp) {
			free(mdoc->meta.vol);
			mdoc->meta.vol = xstrdup(n->string);
		} else
			mdoc->meta.arch = xstrdup(cp);
	}	

	mdoc_msg(mdoc, "volume: %s", mdoc->meta.vol);
	mdoc_msg(mdoc, "arch: %s", mdoc->meta.arch ?
			mdoc->meta.arch : "<unset>");
	mdoc_msg(mdoc, "msec: %d", mdoc->meta.msec);

	/* Ignore any subsequent parameters... */

	return(post_prologue(mdoc));
}


static int
post_os(struct mdoc *mdoc)
{
	char		  buf[64];
	struct utsname	  utsname;

	if (mdoc->meta.os)
		free(mdoc->meta.os);

	(void)xstrlcpys(buf, mdoc->last->child, sizeof(buf));

	if (0 == buf[0]) {
		if (-1 == uname(&utsname))
			return(mdoc_err(mdoc, "utsname"));
		(void)xstrlcpy(buf, utsname.sysname, sizeof(buf));
		(void)xstrlcat(buf, " ", sizeof(buf));
		(void)xstrlcat(buf, utsname.release, sizeof(buf));
	}

	mdoc->meta.os = xstrdup(buf);
	mdoc_msg(mdoc, "system: %s", mdoc->meta.os);

	mdoc->lastnamed = mdoc->lastsec = SEC_BODY;

	return(post_prologue(mdoc));
}


static int
post_bl_tagwidth(struct mdoc *mdoc)
{
	struct mdoc_node  *n;
	int		   sz;
	char		   buf[32];

	/*
	 * If -tag has been specified and -width has not been, then try
	 * to intuit our width from the first body element.  
	 */

	if (NULL == (n = mdoc->last->body->child))
		return(1);

	/*
	 * Use the text width, if a text node, or the default macro
	 * width if a macro.
	 */

	if ((n = n->head->child)) {
		if (MDOC_TEXT != n->type) {
			if (0 == (sz = (int)mdoc_macro2len(n->tok)))
				sz = -1;
		} else
			sz = (int)strlen(n->string) + 1;
	} else
		sz = -1;

	if (-1 == sz) {
		if ( ! mwarn(mdoc, WNOWIDTH))
			return(0);
		sz = 10;
	}

	(void)snprintf(buf, sizeof(buf), "%dn", sz);

	/*
	 * We have to dynamically add this to the macro's argument list.
	 * We're guaranteed that a MDOC_Width doesn't already exist.
	 */

	if (NULL == mdoc->last->args) {
		mdoc->last->args = xcalloc
			(1, sizeof(struct mdoc_arg));
		mdoc->last->args->refcnt = 1;
	}

	n = mdoc->last;
	sz = (int)n->args->argc;
	
	(n->args->argc)++;

	n->args->argv = xrealloc(n->args->argv, 
			n->args->argc * sizeof(struct mdoc_arg));

	n->args->argv[sz - 1].arg = MDOC_Width;
	n->args->argv[sz - 1].line = mdoc->last->line;
	n->args->argv[sz - 1].pos = mdoc->last->pos;
	n->args->argv[sz - 1].sz = 1;
	n->args->argv[sz - 1].value = xcalloc(1, sizeof(char *));
	n->args->argv[sz - 1].value[0] = xstrdup(buf);

	mdoc_msg(mdoc, "adding %s argument: %s", 
			mdoc_argnames[MDOC_Width], buf);

	return(1);
}


static int
post_bl_width(struct mdoc *m)
{
	size_t		  width;
	int		  i, tok;
	char		  buf[32];
	char		 *p;

	if (NULL == m->last->args)
		return(merr(m, ENOWIDTH));

	for (i = 0; i < (int)m->last->args->argc; i++)
		if (MDOC_Width == m->last->args->argv[i].arg)
			break;

	if (i == (int)m->last->args->argc)
		return(merr(m, ENOWIDTH));

	p = m->last->args->argv[i].value[0];

	/*
	 * If the value to -width is a macro, then we re-write it to be
	 * the macro's width as set in share/tmac/mdoc/doc-common.
	 */

	if (xstrcmp(p, "Ds"))
		width = 8;
	else if (MDOC_MAX == (tok = mdoc_tokhash_find(m->htab, p)))
		return(1);
	else if (0 == (width = mdoc_macro2len(tok))) 
		return(mwarn(m, WNOWIDTH));

	mdoc_msg(m, "re-writing %s argument: %s -> %zun", 
			mdoc_argnames[MDOC_Width], p, width);

	/* The value already exists: free and reallocate it. */

	(void)snprintf(buf, sizeof(buf), "%zun", width);

	free(m->last->args->argv[i].value[0]);
	m->last->args->argv[i].value[0] = xstrdup(buf);

	return(1);
}


static int
post_bl(struct mdoc *mdoc)
{
	int		  i, r, len;

	if (MDOC_BLOCK != mdoc->last->type)
		return(1);

	/*
	 * These are fairly complicated, so we've broken them into two
	 * functions.  post_bl_tagwidth() is called when a -tag is
	 * specified, but no -width (it must be guessed).  The second
	 * when a -width is specified (macro indicators must be
	 * rewritten into real lengths).
	 */

	len = (int)(mdoc->last->args ? mdoc->last->args->argc : 0);

	for (r = i = 0; i < len; i++) {
		if (MDOC_Tag == mdoc->last->args->argv[i].arg)
			r |= 1 << 0;
		if (MDOC_Width == mdoc->last->args->argv[i].arg)
			r |= 1 << 1;
	}

	if (r & (1 << 0) && ! (r & (1 << 1))) {
		if ( ! post_bl_tagwidth(mdoc))
			return(0);
	} else if (r & (1 << 1))
		if ( ! post_bl_width(mdoc))
			return(0);

	return(1);
}


static int
post_ar(struct mdoc *mdoc)
{
	struct mdoc_node *n;

	if (mdoc->last->child)
		return(1);
	
	n = mdoc->last;

	mdoc->next = MDOC_NEXT_CHILD;
	if ( ! mdoc_word_alloc(mdoc, mdoc->last->line,
				mdoc->last->pos, "file"))
		return(0);
	mdoc->next = MDOC_NEXT_SIBLING;
	if ( ! mdoc_word_alloc(mdoc, mdoc->last->line, 
				mdoc->last->pos, "..."))
		return(0);

	mdoc->last = n;
	mdoc->next = MDOC_NEXT_SIBLING;
	return(1);
}


static int
post_dd(struct mdoc *mdoc)
{
	char		  buf[64];

	(void)xstrlcpys(buf, mdoc->last->child, sizeof(buf));

	if (0 == (mdoc->meta.date = mdoc_atotime(buf))) {
		if ( ! mwarn(mdoc, WBADDATE))
			return(0);
		mdoc->meta.date = time(NULL);
	}

	mdoc_msg(mdoc, "date: %u", mdoc->meta.date);
	return(post_prologue(mdoc));
}


static int
post_prologue(struct mdoc *mdoc)
{
	struct mdoc_node *n;

	/* 
	 * The end document shouldn't have the prologue macros as part
	 * of the syntax tree (they encompass only meta-data).  
	 */

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
