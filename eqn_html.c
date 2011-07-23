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
#include "html.h"

static void	eqn_box(struct html *, const struct eqn_box *);
static void	eqn_box_post(struct html *, const struct eqn_box *);
static void	eqn_box_pre(struct html *, const struct eqn_box *);
static void	eqn_text(struct html *, const struct eqn_box *);

void
print_eqn(struct html *p, const struct eqn *ep)
{
	struct htmlpair	 tag;
	struct tag	*t;

	PAIR_CLASS_INIT(&tag, "eqn");
	t = print_otag(p, TAG_SPAN, 1, &tag);

	p->flags |= HTML_NONOSPACE;
	eqn_box(p, ep->root);
	p->flags &= ~HTML_NONOSPACE;

	print_tagq(p, t);
}

static void
eqn_box(struct html *p, const struct eqn_box *bp)
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
eqn_box_pre(struct html *p, const struct eqn_box *bp)
{

	if (bp->left)
		print_text(p, bp->left);
}

static void
eqn_box_post(struct html *p, const struct eqn_box *bp)
{

	if (bp->right)
		print_text(p, bp->right);
}

static void
eqn_text(struct html *p, const struct eqn_box *bp)
{

	if (bp->text)
		print_text(p, bp->text);
}
