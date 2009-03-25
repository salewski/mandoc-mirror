/* $Id$ */
/*
 * Copyright (c) 2008, 2009 Kristaps Dzonsons <kristaps@openbsd.org>
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
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libman.h"


struct	actions {
	int	(*post)(struct man *);
};


const	struct actions man_actions[MAN_MAX] = {
	{ NULL }, /* __ */
	{ NULL }, /* TH */
	{ NULL }, /* SH */
	{ NULL }, /* SS */
	{ NULL }, /* TP */
	{ NULL }, /* LP */
	{ NULL }, /* PP */
	{ NULL }, /* P */
	{ NULL }, /* IP */
	{ NULL }, /* HP */
	{ NULL }, /* SM */
	{ NULL }, /* SB */
	{ NULL }, /* BI */
	{ NULL }, /* IB */
	{ NULL }, /* BR */
	{ NULL }, /* RB */
	{ NULL }, /* R */
	{ NULL }, /* B */
	{ NULL }, /* I */
	{ NULL }, /* IR */
};


int
man_action_post(struct man *m)
{

	if (MAN_ACTED & m->last->flags)
		return(1);
	m->last->flags |= MAN_ACTED;

	switch (m->last->type) {
	case (MAN_TEXT):
		break;
	case (MAN_ROOT):
		break;
	default:
		if (NULL == man_actions[m->last->tok].post)
			break;
		return((*man_actions[m->last->tok].post)(m));
	}
	return(1);
}

#if 0
static int
post_dt(POST_ARGS)
{
	struct mdoc_node *n;
	const char	 *cp;
	char		 *ep;
	long		  lval;

	if (m->meta.title)
		free(m->meta.title);
	if (m->meta.vol)
		free(m->meta.vol);
	if (m->meta.arch)
		free(m->meta.arch);

	m->meta.title = m->meta.vol = m->meta.arch = NULL;
	m->meta.msec = 0;

	/* Handles: `.Dt' 
	 *   --> title = unknown, volume = local, msec = 0, arch = NULL
	 */

	if (NULL == (n = m->last->child)) {
		m->meta.title = xstrdup("unknown");
		m->meta.vol = xstrdup("local");
		return(post_prol(m));
	}

	/* Handles: `.Dt TITLE' 
	 *   --> title = TITLE, volume = local, msec = 0, arch = NULL
	 */

	m->meta.title = xstrdup(n->string);

	if (NULL == (n = n->next)) {
		m->meta.vol = xstrdup("local");
		return(post_prol(m));
	}

	/* Handles: `.Dt TITLE SEC'
	 *   --> title = TITLE, volume = SEC is msec ? 
	 *           format(msec) : SEC,
	 *       msec = SEC is msec ? atoi(msec) : 0,
	 *       arch = NULL
	 */

	cp = mdoc_a2msec(n->string);
	if (cp) {
		m->meta.vol = xstrdup(cp);
		errno = 0;
		lval = strtol(n->string, &ep, 10);
		if (n->string[0] != '\0' && *ep == '\0')
			m->meta.msec = (int)lval;
	} else 
		m->meta.vol = xstrdup(n->string);

	if (NULL == (n = n->next))
		return(post_prol(m));

	/* Handles: `.Dt TITLE SEC VOL'
	 *   --> title = TITLE, volume = VOL is vol ?
	 *       format(VOL) : 
	 *           VOL is arch ? format(arch) : 
	 *               VOL
	 */

	cp = mdoc_a2vol(n->string);
	if (cp) {
		free(m->meta.vol);
		m->meta.vol = xstrdup(cp);
		n = n->next;
	} else {
		cp = mdoc_a2arch(n->string);
		if (NULL == cp) {
			free(m->meta.vol);
			m->meta.vol = xstrdup(n->string);
		} else
			m->meta.arch = xstrdup(cp);
	}	

	/* Ignore any subsequent parameters... */

	return(post_prol(m));
}

static int
post_prol(POST_ARGS)
{
	struct mdoc_node *n;

	/* 
	 * The end document shouldn't have the prologue macros as part
	 * of the syntax tree (they encompass only meta-data).  
	 */

	if (m->last->parent->child == m->last)
		m->last->parent->child = m->last->prev;
	if (m->last->prev)
		m->last->prev->next = NULL;

	n = m->last;
	assert(NULL == m->last->next);

	if (m->last->prev) {
		m->last = m->last->prev;
		m->next = MDOC_NEXT_SIBLING;
	} else {
		m->last = m->last->parent;
		m->next = MDOC_NEXT_CHILD;
	}

	mdoc_node_freelist(n);
	return(1);
}
#endif
