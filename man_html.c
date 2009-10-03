/*	$Id$ */
/*
 * Copyright (c) 2008, 2009 Kristaps Dzonsons <kristaps@kth.se>
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
#include <sys/types.h>
#include <sys/queue.h>

#include <stdio.h>
#include <stdlib.h>

#include "html.h"
#include "man.h"

#define	MAN_ARGS	  const struct man_meta *m, \
			  const struct man_node *n, \
			  struct html *h

struct	htmlman {
	int		(*pre)(MAN_ARGS);
	int		(*post)(MAN_ARGS);
};


static	void		  print_man(MAN_ARGS);
static	void		  print_man_head(MAN_ARGS);


static	const struct htmlman mans[MAN_MAX] = {
	{ NULL, NULL }, /* br */
	{ NULL, NULL }, /* TH */
	{ NULL, NULL }, /* SH */
	{ NULL, NULL }, /* SS */
	{ NULL, NULL }, /* TP */
	{ NULL, NULL }, /* LP */
	{ NULL, NULL }, /* PP */
	{ NULL, NULL }, /* P */
	{ NULL, NULL }, /* IP */
	{ NULL, NULL }, /* HP */ 
	{ NULL, NULL }, /* SM */
	{ NULL, NULL }, /* SB */
	{ NULL, NULL }, /* BI */
	{ NULL, NULL }, /* IB */
	{ NULL, NULL }, /* BR */
	{ NULL, NULL }, /* RB */
	{ NULL, NULL }, /* R */
	{ NULL, NULL }, /* B */
	{ NULL, NULL }, /* I */
	{ NULL, NULL }, /* IR */
	{ NULL, NULL }, /* RI */
	{ NULL, NULL }, /* na */
	{ NULL, NULL }, /* i */
	{ NULL, NULL }, /* sp */
	{ NULL, NULL }, /* nf */
	{ NULL, NULL }, /* fi */
	{ NULL, NULL }, /* r */
	{ NULL, NULL }, /* RE */
	{ NULL, NULL }, /* RS */
	{ NULL, NULL }, /* DT */
	{ NULL, NULL }, /* UC */
};


void
html_man(void *arg, const struct man *m)
{
	struct html	*h;
	struct tag	*t;

	h = (struct html *)arg;

	print_gen_doctype(h);

	t = print_otag(h, TAG_HTML, 0, NULL);
	print_man(man_meta(m), man_node(m), h);
	print_tagq(h, t);

	printf("\n");
}


static void
print_man(MAN_ARGS) 
{
	struct tag	*t;
	struct htmlpair	 tag;

	t = print_otag(h, TAG_HEAD, 0, NULL);

	print_man_head(m, n, h);
	print_tagq(h, t);
	t = print_otag(h, TAG_BODY, 0, NULL);

	tag.key = ATTR_CLASS;
	tag.val = "body";
	print_otag(h, TAG_DIV, 1, &tag);

	/*print_man_nodelist(m, n, h);*/

	print_tagq(h, t);
}


/* ARGSUSED */
static void
print_man_head(MAN_ARGS)
{

	print_gen_head(h);
	bufinit(h);
	buffmt(h, "%s(%d)", m->title, m->msec);

	print_otag(h, TAG_TITLE, 0, NULL);
	print_text(h, h->buf);
}
