/*	$Id$ */
/*
 * Copyright (c) 2009 Kristaps Dzonsons <kristaps@kth.se>
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libroff.h"

enum	tbl_ident {
	KEY_CENTRE = 0,
	KEY_DELIM,
	KEY_EXPAND,
	KEY_BOX,
	KEY_DBOX,
	KEY_ALLBOX,
	KEY_TAB,
	KEY_LINESIZE,
	KEY_NOKEEP,
	KEY_DPOINT,
	KEY_NOSPACE,
	KEY_FRAME,
	KEY_DFRAME,
	KEY_MAX
};

struct	tbl_phrase {
	const char	*name;
	int		 key;
	enum tbl_ident	 ident;
};

/* Handle Commonwealth/American spellings. */
#define	KEY_MAXKEYS	 14

static	const struct tbl_phrase keys[KEY_MAXKEYS] = {
	{ "center",	 TBL_OPT_CENTRE,	KEY_CENTRE},
	{ "centre",	 TBL_OPT_CENTRE,	KEY_CENTRE},
	{ "delim",	 0,	       		KEY_DELIM},
	{ "expand",	 TBL_OPT_EXPAND,	KEY_EXPAND},
	{ "box",	 TBL_OPT_BOX,   	KEY_BOX},
	{ "doublebox",	 TBL_OPT_DBOX,  	KEY_DBOX},
	{ "allbox",	 TBL_OPT_ALLBOX,	KEY_ALLBOX},
	{ "frame",	 TBL_OPT_BOX,		KEY_FRAME},
	{ "doubleframe", TBL_OPT_DBOX,		KEY_DFRAME},
	{ "tab",	 0,			KEY_TAB},
	{ "linesize",	 0,			KEY_LINESIZE},
	{ "nokeep",	 TBL_OPT_NOKEEP,	KEY_NOKEEP},
	{ "decimalpoint", 0,			KEY_DPOINT},
	{ "nospaces",	 TBL_OPT_NOSPACE,	KEY_NOSPACE},
};

static	int		 arg(struct tbl *, int, const char *, int *, int);
static	int		 opt(struct tbl *, int, const char *, int *);

static int
arg(struct tbl *tbl, int ln, const char *p, int *pos, int key)
{
	int		 sv;

again:
	sv = *pos;

	switch (tbl_next(tbl, p, pos)) {
	case (TBL_TOK_OPENPAREN):
		break;
	case (TBL_TOK_SPACE):
		/* FALLTHROUGH */
	case (TBL_TOK_TAB):
		goto again;
	default:
		return(0);
	}

	sv = *pos;

	switch (tbl_next(tbl, p, pos)) {
	case (TBL_TOK__MAX):
		break;
	default:
		return(0);
	}

	switch (key) {
	case (KEY_DELIM):
		/* FIXME: cache this value. */
		if (2 != strlen(tbl->buf))
			return(0);
		tbl->delims[0] = tbl->buf[0];
		tbl->delims[1] = tbl->buf[1];
		break;
	case (KEY_TAB):
		/* FIXME: cache this value. */
		if (1 != strlen(tbl->buf))
			return(0);
		tbl->tab = tbl->buf[0];
		break;
	case (KEY_LINESIZE):
		if ((tbl->linesize = atoi(tbl->buf)) <= 0)
			return(0);
		break;
	case (KEY_DPOINT):
		/* FIXME: cache this value. */
		if (1 != strlen(tbl->buf))
			return(0);
		tbl->decimal = tbl->buf[0];
		break;
	default:
		abort();
	}

	sv = *pos;

	switch (tbl_next(tbl, p, pos)) {
	case (TBL_TOK_CLOSEPAREN):
		break;
	default:
		return(0);
	}

	return(1);
}


static int
opt(struct tbl *tbl, int ln, const char *p, int *pos)
{
	int		 i, sv;

again:
	sv = *pos;

	/*
	 * EBNF describing this section:
	 *
	 * options	::= option_list [:space:]* [;][\n]
	 * option_list	::= option option_tail
	 * option_tail	::= [:space:]+ option_list |
	 * 		::= epsilon
	 * option	::= [:alpha:]+ args
	 * args		::= [:space:]* [(] [:alpha:]+ [)]
	 */

	switch (tbl_next(tbl, p, pos)) {
	case (TBL_TOK__MAX):
		break;
	case (TBL_TOK_SPACE):
		/* FALLTHROUGH */
	case (TBL_TOK_TAB):
		goto again;
	case (TBL_TOK_SEMICOLON):
		tbl->part = TBL_PART_LAYOUT;
		return(1);
	default:
		return(0);
	}

	for (i = 0; i < KEY_MAXKEYS; i++) {
		/* FIXME: hashtable this? */
		if (strcasecmp(tbl->buf, keys[i].name))
			continue;
		if (keys[i].key) 
			tbl->opts |= keys[i].key;
		else if ( ! arg(tbl, ln, p, pos, keys[i].ident))
			return(0);

		break;
	}

	if (KEY_MAXKEYS == i)
		return(0);

	return(opt(tbl, ln, p, pos));
}

int
tbl_option(struct tbl *tbl, int ln, const char *p)
{
	int		 pos;

	pos = 0;
	return(opt(tbl, ln, p, &pos));
}
