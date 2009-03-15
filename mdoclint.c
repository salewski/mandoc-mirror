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

int
main(int argc, char *argv[])
{
	struct mmain	*p;
	int		 c;

	p = mmain_alloc();

	c = mmain_getopt(p, argc, argv, NULL, 
			"[infile...]", NULL, NULL, NULL);

	argv += c;
	if (0 == (argc -= c))
		mmain_exit(p, NULL != mmain_mdoc(p, "-"));

	while (c-- > 0) {
		if (NULL == mmain_mdoc(p, *argv++))
			mmain_exit(p, 1);
		mmain_reset(p);
	}

	mmain_exit(p, 0);
	/* NOTREACHED */
}

