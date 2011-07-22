/*	$Id$ */
/*
 * Copyright (c) 2011 Kristaps Dzonsons <kristaps@bsd.lv>
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mandoc.h"
#include "out.h"
#include "term.h"

static void	eqn_box(struct termp *p, const struct eqn_box *);
static void	eqn_box_post(struct termp *p, const struct eqn_box *);
static void	eqn_box_pre(struct termp *p, const struct eqn_box *);
static void	eqn_text(struct termp *p, const struct eqn_box *);

void
term_eqn(struct termp *p, const struct eqn *ep)
{

	p->flags |= TERMP_NONOSPACE;
	eqn_box(p, ep->root);
	p->flags &= ~TERMP_NONOSPACE;
}

static void
eqn_box(struct termp *p, const struct eqn_box *bp)
{

	eqn_box_pre(p, bp);
	eqn_text(p, bp);

	if (bp->first)
		eqn_box(p, bp->first);

	eqn_box_post(p, bp);

	if (bp->next)
		eqn_box(p, bp->next);
}

static void
eqn_box_pre(struct termp *p, const struct eqn_box *bp)
{

	if (EQN_LIST == bp->type)
		term_word(p, "{");
	if (bp->left)
		term_word(p, bp->left);
}

static void
eqn_box_post(struct termp *p, const struct eqn_box *bp)
{

	if (EQN_LIST == bp->type)
		term_word(p, "}");
	if (bp->right)
		term_word(p, bp->right);
	if (bp->above)
		term_word(p, "|");
}

static void
eqn_text(struct termp *p, const struct eqn_box *bp)
{

	if (bp->text)
		term_word(p, bp->text);
}
