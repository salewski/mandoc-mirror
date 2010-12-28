/*	$Id$ */
/*
 * Copyright (c) 2009, 2010 Kristaps Dzonsons <kristaps@kth.se>
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
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mandoc.h"
#include "roff.h"
#include "libmandoc.h"
#include "libroff.h"

enum	tbl_part {
	TBL_PART_OPTS, /* in options (first line) */
	TBL_PART_LAYOUT, /* describing layout */
	TBL_PART_DATA  /* creating data rows */
};


struct	tbl {
	enum tbl_part	 part;
};

static	void		 tbl_init(struct tbl *);
static	void		 tbl_clear(struct tbl *);

static void
tbl_clear(struct tbl *tbl)
{

}

static void
tbl_init(struct tbl *tbl)
{

	tbl->part = TBL_PART_OPTS;
}

enum rofferr
tbl_read(struct tbl *tbl, int ln, const char *p, int offs)
{
	int		 len;
	const char	*cp;

	cp = &p[offs];
	len = (int)strlen(cp);

	if (len && TBL_PART_OPTS == tbl->part)
		if (';' != cp[len - 1])
			tbl->part = TBL_PART_LAYOUT;
	
	return(ROFF_CONT);
}

struct tbl *
tbl_alloc(void)
{
	struct tbl	*p;

	p = mandoc_malloc(sizeof(struct tbl));
	tbl_init(p);
	return(p);
}

void
tbl_free(struct tbl *p)
{

	tbl_clear(p);
	free(p);
}

void
tbl_reset(struct tbl *tbl)
{

	tbl_clear(tbl);
	tbl_init(tbl);
}
