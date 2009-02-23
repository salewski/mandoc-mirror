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
#include <err.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "mmain.h"

#define	xprintf (void)printf

static	void	doprint(const struct mdoc_node *, int);

int
main(int argc, char *argv[])
{
	struct mmain	*p;
	const struct mdoc *mdoc;

	extern int	 optreset;
	extern int	 optind;

	p = mmain_alloc();

	if ( ! mmain_getopt(p, argc, argv, NULL, NULL, NULL, NULL))
		mmain_exit(p, 1);

	if (NULL == (mdoc = mmain_mdoc(p)))
		mmain_exit(p, 1);

	doprint(mdoc_node(mdoc), 0);
	mmain_exit(p, 0);
	/* NOTREACHED */
}


static void
doprint(const struct mdoc_node *n, int indent)
{
	const char	 *p, *t;
	int		  i, j;
	size_t		  argc, sz;
	char		**params;
	struct mdoc_arg	 *argv;

	argv = NULL;
	argc = sz = 0;
	params = NULL;

	switch (n->type) {
	case (MDOC_ROOT):
		t = "root";
		break;
	case (MDOC_BLOCK):
		t = "block";
		break;
	case (MDOC_HEAD):
		t = "block-head";
		break;
	case (MDOC_BODY):
		t = "block-body";
		break;
	case (MDOC_TAIL):
		t = "block-tail";
		break;
	case (MDOC_ELEM):
		t = "elem";
		break;
	case (MDOC_TEXT):
		t = "text";
		break;
	default:
		abort();
		/* NOTREACHED */
	}

	switch (n->type) {
	case (MDOC_TEXT):
		p = n->data.text.string;
		break;
	case (MDOC_BODY):
		p = mdoc_macronames[n->tok];
		break;
	case (MDOC_HEAD):
		p = mdoc_macronames[n->tok];
		break;
	case (MDOC_TAIL):
		p = mdoc_macronames[n->tok];
		break;
	case (MDOC_ELEM):
		p = mdoc_macronames[n->tok];
		argv = n->data.elem.argv;
		argc = n->data.elem.argc;
		break;
	case (MDOC_BLOCK):
		p = mdoc_macronames[n->tok];
		argv = n->data.block.argv;
		argc = n->data.block.argc;
		break;
	case (MDOC_ROOT):
		p = "root";
		break;
	default:
		abort();
		/* NOTREACHED */
	}

	for (i = 0; i < indent; i++)
		xprintf("    ");
	xprintf("%s (%s)", p, t);

	for (i = 0; i < (int)argc; i++) {
		xprintf(" -%s", mdoc_argnames[argv[i].arg]);
		if (argv[i].sz > 0)
			xprintf(" [");
		for (j = 0; j < (int)argv[i].sz; j++)
			xprintf(" [%s]", argv[i].value[j]);
		if (argv[i].sz > 0)
			xprintf(" ]");
	}

	for (i = 0; i < (int)sz; i++)
		xprintf(" [%s]", params[i]);

	xprintf(" %d:%d\n", n->line, n->pos);

	if (n->child)
		doprint(n->child, indent + 1);
	if (n->next)
		doprint(n->next, indent);
}
