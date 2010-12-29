/*	$Id$ */
/*
 * Copyright (c) 2009, 2010 Kristaps Dzonsons <kristaps@bsd.lv>
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mandoc.h"
#include "roff.h"
#include "libmandoc.h"
#include "libroff.h"

static	void		 tbl_init(struct tbl *);
static	void		 tbl_clear(struct tbl *);

static void
tbl_clear(struct tbl *tbl)
{
	struct tbl_row	*rp;
	struct tbl_cell	*cp;

	while (tbl->first) {
		rp = tbl->first;
		tbl->first = rp->next;
		while (rp->first) {
			cp = rp->first;
			rp->first = cp->next;
			free(cp);
		}
		free(rp);
	}

	tbl->last = NULL;
}

static void
tbl_init(struct tbl *tbl)
{

	tbl->part = TBL_PART_OPTS;
	tbl->tab = '\t';
	tbl->linesize = 12;
	tbl->decimal = '.';
}

enum rofferr
tbl_read(struct tbl *tbl, int ln, const char *p, int offs)
{
	int		 len;
	const char	*cp;

	cp = &p[offs];
	len = (int)strlen(cp);

	/*
	 * If we're in the options section and we don't have a
	 * terminating semicolon, assume we've moved directly into the
	 * layout section.  No need to report a warning: this is,
	 * apparently, standard behaviour.
	 */

	if (TBL_PART_OPTS == tbl->part && len)
		if (';' != cp[len - 1])
			tbl->part = TBL_PART_LAYOUT;

	/* Now process each logical section of the table.  */

	switch (tbl->part) {
	case (TBL_PART_OPTS):
		return(tbl_option(tbl, ln, p) ? ROFF_IGN : ROFF_ERR);
	case (TBL_PART_LAYOUT):
		return(tbl_layout(tbl, ln, p) ? ROFF_IGN : ROFF_ERR);
	default:
		break;
	}
	
	return(ROFF_CONT);
}

struct tbl *
tbl_alloc(void *data, const mandocmsg msg)
{
	struct tbl	*p;

	p = mandoc_calloc(1, sizeof(struct tbl));
	p->data = data;
	p->msg = msg;
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

