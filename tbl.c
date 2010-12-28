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

static	const char	 tbl_toks[TBL_TOK__MAX] = {
	'(',	')',	',',	';',	'.',
	' ',	'\t',	'\0'
};

static	void		 tbl_init(struct tbl *);
static	void		 tbl_clear(struct tbl *);
static	enum tbl_tok	 tbl_next_char(char);

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

static enum tbl_tok
tbl_next_char(char c)
{
	int		 i;

	/*
	 * These are delimiting tokens.  They separate out words in the
	 * token stream.
	 *
	 * FIXME: make this into a hashtable for faster lookup.
	 */
	for (i = 0; i < TBL_TOK__MAX; i++)
		if (c == tbl_toks[i])
			return((enum tbl_tok)i);

	return(TBL_TOK__MAX);
}

enum tbl_tok
tbl_next(struct tbl *tbl, const char *p, int *pos)
{
	int		 i;
	enum tbl_tok	 c;

	tbl->buf[0] = '\0';

	if (TBL_TOK__MAX != (c = tbl_next_char(p[*pos]))) {
		if (TBL_TOK_NIL != c) {
			tbl->buf[0] = p[*pos];
			tbl->buf[1] = '\0';
			(*pos)++;
		}
		return(c);
	}

	/*
	 * Copy words into a nil-terminated buffer.  For now, we use a
	 * static buffer.  FIXME: eventually this should be made into a
	 * dynamic one living in struct tbl.
	 */

	for (i = 0; i < BUFSIZ; i++, (*pos)++)
		if (TBL_TOK__MAX == tbl_next_char(p[*pos]))
			tbl->buf[i] = p[*pos];
		else
			break;

	assert(i < BUFSIZ);
	tbl->buf[i] = '\0';

	return(TBL_TOK__MAX);
}


