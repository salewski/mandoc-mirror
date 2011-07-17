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
#include <time.h>

#include "mandoc.h"
#include "libmandoc.h"
#include "libroff.h"

#define	EQN_ARGS	 struct eqn_node *ep, \
			 int ln, \
			 int pos, \
			 const char **end

struct	eqnpart {
	const char	*name;
	size_t		 sz;
	int		(*fp)(EQN_ARGS);
};

enum	eqnpartt {
	EQN_DEFINE = 0,
	EQN_SET,
	EQN_UNDEF,
	EQN__MAX
};

static	int		 eqn_do_define(EQN_ARGS);
static	int		 eqn_do_set(EQN_ARGS);
static	int		 eqn_do_undef(EQN_ARGS);
static	const char	*eqn_nexttok(struct mparse *, int, int,
				const char **, size_t *);

static	const struct eqnpart eqnparts[EQN__MAX] = {
	{ "define", 6, eqn_do_define }, /* EQN_DEFINE */
	{ "set", 3, eqn_do_set }, /* EQN_SET */
	{ "undef", 5, eqn_do_undef }, /* EQN_UNDEF */
};

/* ARGSUSED */
enum rofferr
eqn_read(struct eqn_node **epp, int ln, 
		const char *p, int pos, int *offs)
{
	size_t		 sz;
	struct eqn_node	*ep;
	struct mparse	*mp;
	const char	*start, *end;
	int		 i, c;

	if (0 == strcmp(p, ".EN")) {
		*epp = NULL;
		return(ROFF_EQN);
	}

	ep = *epp;
	mp = ep->parse;
	end = p + pos;

	if (NULL == (start = eqn_nexttok(mp, ln, pos, &end, &sz)))
		return(ROFF_IGN);

	for (i = 0; i < (int)EQN__MAX; i++) {
		if (eqnparts[i].sz != sz)
			continue;
		if (strncmp(eqnparts[i].name, start, sz))
			continue;

		if ((c = (*eqnparts[i].fp)(ep, ln, pos, &end)) < 0)
			return(ROFF_ERR);
		else if (0 == c || '\0' == *end)
			return(ROFF_IGN);

		/* 
		 * Re-calculate offset and rerun, if trailing text.
		 * This allows multiple definitions (say) on each line.
		 */

		*offs = end - (p + pos);
		return(ROFF_RERUN);
	} 

	end = p + pos;
	while (NULL != (start = eqn_nexttok(mp, ln, pos, &end, &sz))) {
		if (0 == sz)
			continue;

		for (i = 0; i < (int)ep->defsz; i++) {
			if (0 == ep->defs[i].keysz)
				continue;
			if (ep->defs[i].keysz != sz)
				continue;
			if (strncmp(ep->defs[i].key, start, sz))
				continue;
			start = ep->defs[i].val;
			sz = ep->defs[i].valsz;
			break;
		}

		ep->eqn.data = mandoc_realloc
			(ep->eqn.data, ep->eqn.sz + sz + 1);

		if (0 == ep->eqn.sz)
			*ep->eqn.data = '\0';

		ep->eqn.sz += sz;
		strlcat(ep->eqn.data, start, ep->eqn.sz + 1);
	}

	return(ROFF_IGN);
}

struct eqn_node *
eqn_alloc(int pos, int line, struct mparse *parse)
{
	struct eqn_node	*p;

	p = mandoc_calloc(1, sizeof(struct eqn_node));
	p->parse = parse;
	p->eqn.line = line;
	p->eqn.pos = pos;

	return(p);
}

/* ARGSUSED */
void
eqn_end(struct eqn_node *e)
{

	/* Nothing to do. */
}

void
eqn_free(struct eqn_node *p)
{
	int		 i;

	free(p->eqn.data);

	for (i = 0; i < (int)p->defsz; i++) {
		free(p->defs[i].key);
		free(p->defs[i].val);
	}

	free(p->defs);
	free(p);
}

/*
 * Return the current equation token setting "next" on the next one,
 * setting the token size in "sz".
 * This does the Right Thing for quoted strings, too.
 * Returns NULL if no more tokens exist.
 */
static const char *
eqn_nexttok(struct mparse *mp, int ln, int pos,
		const char **next, size_t *sz)
{
	const char	*start;
	int		 q;

	start = *next;
	q = 0;

	if ('\0' == *start)
		return(NULL);

	if ('"' == *start) {
		start++;
		q = 1;
	}

	*next = q ? strchr(start, '"') : strchr(start, ' ');

	if (NULL != *next) {
		*sz = (size_t)(*next - start);
		if (q)
			(*next)++;
		while (' ' == **next)
			(*next)++;
	} else {
		/*
		 * XXX: groff gets confused by this and doesn't always
		 * do the "right thing" (just terminate it and warn
		 * about it).
		 */
		if (q)
			mandoc_msg(MANDOCERR_BADQUOTE, 
					mp, ln, pos, NULL);
		*next = strchr(start, '\0');
		*sz = (size_t)(*next - start);
	}

	return(start);
}

static int
eqn_do_set(struct eqn_node *ep, int ln, int pos, const char **end)
{
	const char	*start;
	struct mparse	*mp;
	size_t		 sz;

	mp = ep->parse;

	start = eqn_nexttok(ep->parse, ln, pos, end, &sz);
	if (NULL == start || 0 == sz) {
		mandoc_msg(MANDOCERR_EQNARGS, mp, ln, pos, NULL); 
		return(0);
	}

	start = eqn_nexttok(ep->parse, ln, pos, end, &sz);
	if (NULL == start || 0 == sz) {
		mandoc_msg(MANDOCERR_EQNARGS, mp, ln, pos, NULL); 
		return(0);
	}

	return(1);
}

static int
eqn_do_define(struct eqn_node *ep, int ln, int pos, const char **end)
{
	const char	*start;
	struct mparse	*mp;
	size_t		 sz;
	int		 i;

	mp = ep->parse;

	start = eqn_nexttok(mp, ln, pos, end, &sz);
	if (NULL == start || 0 == sz) {
		mandoc_msg(MANDOCERR_EQNARGS, mp, ln, pos, NULL); 
		return(0);
	}

	/* TODO: merge this code with roff_getstr(). */

	/* 
	 * Search for a key that already exists. 
	 * Note that the string array can have "holes" (null key).
	 */

	for (i = 0; i < (int)ep->defsz; i++)  {
		if (0 == ep->defs[i].keysz || ep->defs[i].keysz != sz)
			continue;
		if (0 == strncmp(ep->defs[i].key, start, sz))
			break;
	}

	/* Create a new key. */

	if (i == (int)ep->defsz) {
		/* Find holes in string array. */
		for (i = 0; i < (int)ep->defsz; i++)
			if (0 == ep->defs[i].keysz)
				break;

		if (i == (int)ep->defsz) {
			ep->defsz++;
			ep->defs = mandoc_realloc
				(ep->defs, ep->defsz * 
				 sizeof(struct eqn_def));
			ep->defs[i].key = ep->defs[i].val = NULL;
		}

		ep->defs[i].keysz = sz;
		ep->defs[i].key = mandoc_realloc
			(ep->defs[i].key, sz + 1);

		memcpy(ep->defs[i].key, start, sz);
		ep->defs[i].key[(int)sz] = '\0';
	}

	start = eqn_nexttok(mp, ln, pos, end, &sz);

	if (NULL == start || 0 == sz) {
		ep->defs[i].keysz = 0;
		mandoc_msg(MANDOCERR_EQNARGS, mp, ln, pos, NULL); 
		return(0);
	}

	ep->defs[i].valsz = sz;
	ep->defs[i].val = mandoc_realloc
		(ep->defs[i].val, sz + 1);
	memcpy(ep->defs[i].val, start, sz);
	ep->defs[i].val[(int)sz] = '\0';

	return(sz ? 1 : 0);
}

static int
eqn_do_undef(struct eqn_node *ep, int ln, int pos, const char **end)
{
	const char	*start;
	struct mparse	*mp;
	size_t		 sz;
	int		 i;

	mp = ep->parse;

	start = eqn_nexttok(mp, ln, pos, end, &sz);
	if (NULL == start || 0 == sz) {
		mandoc_msg(MANDOCERR_EQNARGS, mp, ln, pos, NULL); 
		return(0);
	}

	for (i = 0; i < (int)ep->defsz; i++)  {
		if (0 == ep->defs[i].keysz || ep->defs[i].keysz != sz)
			continue;
		if (strncmp(ep->defs[i].key, start, sz))
			continue;
		ep->defs[i].keysz = 0;
		break;
	}

	return(1);
}
