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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mandoc.h"
#include "libman.h"
#include "libmandoc.h"

struct	actions {
	int	(*post)(struct man *);
};

const	struct actions man_actions[MAN_MAX] = {
	{ NULL }, /* br */
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
	{ NULL }, /* RI */
	{ NULL }, /* na */
	{ NULL }, /* i */
	{ NULL }, /* sp */
	{ NULL }, /* nf */
	{ NULL }, /* fi */
	{ NULL }, /* r */
	{ NULL }, /* RE */
	{ NULL }, /* RS */
	{ NULL }, /* DT */
	{ NULL }, /* UC */
	{ NULL }, /* PD */
	{ NULL }, /* Sp */
	{ NULL }, /* Vb */
	{ NULL }, /* Ve */
	{ NULL }, /* AT */
	{ NULL }, /* in */
};


int
man_action_post(struct man *m)
{

	if (MAN_ACTED & m->last->flags)
		return(1);
	m->last->flags |= MAN_ACTED;

	switch (m->last->type) {
	case (MAN_TEXT):
		/* FALLTHROUGH */
	case (MAN_ROOT):
		return(1);
	default:
		break;
	}

	if (NULL == man_actions[m->last->tok].post)
		return(1);
	return((*man_actions[m->last->tok].post)(m));
}




