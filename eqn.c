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

#define	EQN_NEST_MAX	 128 /* maximum nesting of defines */
#define	EQN_MSG(t, x)	 mandoc_msg((t), (x)->parse, (x)->eqn.ln, (x)->eqn.pos, NULL)

struct	eqnpart {
	const char	*name;
	size_t		 sz;
	int		(*fp)(struct eqn_node *);
};

enum	eqnpartt {
	EQN_DEFINE = 0,
	EQN_SET,
	EQN_UNDEF,
	EQN__MAX
};

static	struct eqn_def	*eqn_def_find(struct eqn_node *, 
				const char *, size_t);
static	int		 eqn_do_define(struct eqn_node *);
static	int		 eqn_do_ign2(struct eqn_node *);
static	int		 eqn_do_undef(struct eqn_node *);
static	const char	*eqn_nexttok(struct eqn_node *, size_t *);
static	const char	*eqn_next(struct eqn_node *, char, size_t *);
static	int		 eqn_box(struct eqn_node *);

static	const struct eqnpart eqnparts[EQN__MAX] = {
	{ "define", 6, eqn_do_define }, /* EQN_DEFINE */
	{ "set", 3, eqn_do_ign2 }, /* EQN_SET */
	{ "undef", 5, eqn_do_undef }, /* EQN_UNDEF */
};

/* ARGSUSED */
enum rofferr
eqn_read(struct eqn_node **epp, int ln, 
		const char *p, int pos, int *offs)
{
	size_t		 sz;
	struct eqn_node	*ep;
	enum rofferr	 er;

	ep = *epp;

	/*
	 * If we're the terminating mark, unset our equation status and
	 * validate the full equation.
	 */

	if (0 == strcmp(p, ".EN")) {
		er = eqn_end(ep);
		*epp = NULL;
		return(er);
	}

	/*
	 * Build up the full string, replacing all newlines with regular
	 * whitespace.
	 */

	sz = strlen(p + pos) + 1;
	ep->data = mandoc_realloc(ep->data, ep->sz + sz + 1);

	/* First invocation: nil terminate the string. */

	if (0 == ep->sz)
		*ep->data = '\0';

	ep->sz += sz;
	strlcat(ep->data, p + pos, ep->sz + 1);
	strlcat(ep->data, " ", ep->sz + 1);
	return(ROFF_IGN);
}

struct eqn_node *
eqn_alloc(int pos, int line, struct mparse *parse)
{
	struct eqn_node	*p;

	p = mandoc_calloc(1, sizeof(struct eqn_node));
	p->parse = parse;
	p->eqn.ln = line;
	p->eqn.pos = pos;

	return(p);
}

enum rofferr
eqn_end(struct eqn_node *ep)
{
	int		 c;

	/*
	 * Validate the expression.
	 * Use the grammar found in the literature.
	 */

	if (0 == ep->sz)
		return(ROFF_IGN);

	while (1 == (c = eqn_box(ep)))
		/* Keep parsing. */ ;

	return(c < 0 ? ROFF_IGN : ROFF_EQN);
}

static int
eqn_box(struct eqn_node *ep)
{
	size_t		 sz;
	const char	*start;
	int		 i;

	if (NULL == (start = eqn_nexttok(ep, &sz)))
		return(0);

	for (i = 0; i < (int)EQN__MAX; i++) {
		if (eqnparts[i].sz != sz)
			continue;
		if (strncmp(eqnparts[i].name, start, sz))
			continue;
		if ( ! (*eqnparts[i].fp)(ep))
			return(-1);

		return(1);
	} 

	ep->eqn.data = mandoc_realloc
		(ep->eqn.data, ep->eqn.sz + sz + 1);

	if (0 == ep->eqn.sz)
		*ep->eqn.data = '\0';

	ep->eqn.sz += sz;
	strlcat(ep->eqn.data, start, ep->eqn.sz + 1);
	return(1);
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

	free(p->data);
	free(p->defs);
	free(p);
}

static const char *
eqn_nexttok(struct eqn_node *ep, size_t *sz)
{

	return(eqn_next(ep, '"', sz));
}

static const char *
eqn_next(struct eqn_node *ep, char quote, size_t *sz)
{
	char		*start, *next;
	int		 q, diff, lim;
	size_t		 sv, ssz;
	struct eqn_def	*def;

	if (NULL == sz)
		sz = &ssz;

	start = &ep->data[(int)ep->cur];
	q = 0;

	if ('\0' == *start)
		return(NULL);

	if (quote == *start) {
		ep->cur++;
		q = 1;
	}

	lim = 0;

	sv = ep->cur;
again:
	if (lim >= EQN_NEST_MAX) {
		EQN_MSG(MANDOCERR_EQNNEST, ep);
		return(NULL);
	}

	ep->cur = sv;
	start = &ep->data[(int)ep->cur];
	next = q ? strchr(start, quote) : strchr(start, ' ');

	if (NULL != next) {
		*sz = (size_t)(next - start);
		ep->cur += *sz;
		if (q)
			ep->cur++;
		while (' ' == ep->data[(int)ep->cur])
			ep->cur++;
	} else {
		if (q)
			EQN_MSG(MANDOCERR_BADQUOTE, ep);
		next = strchr(start, '\0');
		*sz = (size_t)(next - start);
		ep->cur += *sz;
	}

	if (NULL != (def = eqn_def_find(ep, start, *sz))) {
		diff = def->valsz - *sz;

		if (def->valsz > *sz) {
			ep->sz += diff;
			ep->data = mandoc_realloc(ep->data, ep->sz + 1);
			ep->data[ep->sz] = '\0';
			start = &ep->data[(int)sv];
		}

		diff = def->valsz - *sz;
		memmove(start + *sz + diff, start + *sz, 
				(strlen(start) - *sz) + 1);
		memcpy(start, def->val, def->valsz);
		goto again;
	}

	return(start);
}

static int
eqn_do_ign2(struct eqn_node *ep)
{
	const char	*start;

	if (NULL == (start = eqn_nexttok(ep, NULL)))
		EQN_MSG(MANDOCERR_EQNARGS, ep);
	else if (NULL == (start = eqn_nexttok(ep, NULL)))
		EQN_MSG(MANDOCERR_EQNARGS, ep);
	else
		return(1);

	return(0);
}

static int
eqn_do_define(struct eqn_node *ep)
{
	const char	*start;
	size_t		 sz;
	struct eqn_def	*def;
	int		 i;

	if (NULL == (start = eqn_nexttok(ep, &sz))) {
		EQN_MSG(MANDOCERR_EQNARGS, ep);
		return(0);
	}

	/* 
	 * Search for a key that already exists. 
	 * Create a new key if none is found.
	 */

	if (NULL == (def = eqn_def_find(ep, start, sz))) {
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
		def = &ep->defs[i];
	}

	start = eqn_next(ep, ep->data[(int)ep->cur], &sz);

	if (NULL == start) {
		EQN_MSG(MANDOCERR_EQNARGS, ep);
		return(0);
	}

	def->valsz = sz;
	def->val = mandoc_realloc(ep->defs[i].val, sz + 1);
	memcpy(def->val, start, sz);
	def->val[(int)sz] = '\0';
	return(1);
}

static int
eqn_do_undef(struct eqn_node *ep)
{
	const char	*start;
	struct eqn_def	*def;
	size_t		 sz;

	if (NULL == (start = eqn_nexttok(ep, &sz))) {
		EQN_MSG(MANDOCERR_EQNARGS, ep);
		return(0);
	} else if (NULL != (def = eqn_def_find(ep, start, sz)))
		def->keysz = 0;

	return(1);
}

static struct eqn_def *
eqn_def_find(struct eqn_node *ep, const char *key, size_t sz)
{
	int		 i;

	for (i = 0; i < (int)ep->defsz; i++) 
		if (ep->defs[i].keysz && ep->defs[i].keysz == sz &&
				0 == strncmp(ep->defs[i].key, key, sz))
			return(&ep->defs[i]);

	return(NULL);
}
