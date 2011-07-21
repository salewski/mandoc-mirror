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
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mandoc.h"
#include "libmandoc.h"
#include "libroff.h"

#define	EQN_NEST_MAX	 128 /* maximum nesting of defines */
#define	EQN_MSG(t, x)	 mandoc_msg((t), (x)->parse, (x)->eqn.ln, (x)->eqn.pos, NULL)

enum	eqn_rest {
	EQN_DESCOPE,
	EQN_ERR,
	EQN_OK,
	EQN_EOF
};

struct	eqnstr {
	const char	*name;
	size_t		 sz;
};

struct	eqnpart {
	struct eqnstr	 str;
	int		(*fp)(struct eqn_node *);
};

enum	eqnpartt {
	EQN_DEFINE = 0,
	EQN_SET,
	EQN_UNDEF,
	EQN__MAX
};

static	struct eqn_box	*eqn_box_alloc(struct eqn_box *);
static	void		 eqn_box_free(struct eqn_box *);
static	struct eqn_def	*eqn_def_find(struct eqn_node *, 
				const char *, size_t);
static	int		 eqn_do_define(struct eqn_node *);
static	int		 eqn_do_set(struct eqn_node *);
static	int		 eqn_do_undef(struct eqn_node *);
static	const char	*eqn_nexttok(struct eqn_node *, size_t *);
static	const char	*eqn_nextrawtok(struct eqn_node *, size_t *);
static	const char	*eqn_next(struct eqn_node *, 
				char, size_t *, int);
static	void		 eqn_rewind(struct eqn_node *);
static	enum eqn_rest	 eqn_eqn(struct eqn_node *, struct eqn_box *);
static	enum eqn_rest	 eqn_box(struct eqn_node *, struct eqn_box *);

static	const struct eqnpart eqnparts[EQN__MAX] = {
	{ { "define", 6 }, eqn_do_define }, /* EQN_DEFINE */
	{ { "set", 3 }, eqn_do_set }, /* EQN_SET */
	{ { "undef", 5 }, eqn_do_undef }, /* EQN_UNDEF */
};

static	const struct eqnstr eqnmarks[EQNMARK__MAX] = {
	{ "", 0 }, /* EQNMARK_NONE */
	{ "dot", 3 }, /* EQNMARK_DOT */
	{ "dotdot", 6 }, /* EQNMARK_DOTDOT */
	{ "hat", 3 }, /* EQNMARK_HAT */
	{ "tilde", 5 }, /* EQNMARK_TILDE */
	{ "vec", 3 }, /* EQNMARK_VEC */
	{ "dyad", 4 }, /* EQNMARK_DYAD */
	{ "bar", 3 }, /* EQNMARK_BAR */
	{ "under", 5 }, /* EQNMARK_UNDER */
};

static	const struct eqnstr eqnfonts[EQNFONT__MAX] = {
	{ "", 0 }, /* EQNFONT_NONE */
	{ "roman", 5 }, /* EQNFONT_ROMAN */
	{ "bold", 4 }, /* EQNFONT_BOLD */
	{ "italic", 6 }, /* EQNFONT_ITALIC */
};

static	const struct eqnstr eqnposs[EQNPOS__MAX] = {
	{ "", 0 }, /* EQNPOS_NONE */
	{ "over", 4 }, /* EQNPOS_OVER */
	{ "sup", 3 }, /* EQNPOS_SUP */
	{ "sub", 3 }, /* EQNPOS_SUB */
	{ "to", 2 }, /* EQNPOS_TO */
	{ "from", 4 }, /* EQNPOS_FROM */
};

static	const struct eqnstr eqnpiles[EQNPILE__MAX] = {
	{ "", 0 }, /* EQNPILE_NONE */
	{ "cpile", 5 }, /* EQNPILE_CPILE */
	{ "rpile", 5 }, /* EQNPILE_RPILE */
	{ "lpile", 5 }, /* EQNPILE_LPILE */
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
	struct eqn_box	*root;
	enum eqn_rest	 c;

	ep->eqn.root = mandoc_calloc(1, sizeof(struct eqn_box));

	root = ep->eqn.root;
	root->type = EQN_ROOT;

	if (0 == ep->sz)
		return(ROFF_IGN);

	if (EQN_DESCOPE == (c = eqn_eqn(ep, root))) {
		EQN_MSG(MANDOCERR_EQNNSCOPE, ep);
		c = EQN_ERR;
	}

	return(EQN_EOF == c ? ROFF_EQN : ROFF_IGN);
}

static enum eqn_rest
eqn_eqn(struct eqn_node *ep, struct eqn_box *last)
{
	struct eqn_box	*bp;
	enum eqn_rest	 c;

	bp = eqn_box_alloc(last);
	bp->type = EQN_SUBEXPR;

	while (EQN_OK == (c = eqn_box(ep, bp)))
		/* Spin! */ ;

	return(c);
}

static enum eqn_rest
eqn_box(struct eqn_node *ep, struct eqn_box *last)
{
	size_t		 sz;
	const char	*start;
	char		*left;
	enum eqn_rest	 c;
	int		 i, size;
	struct eqn_box	*bp;

	if (NULL == (start = eqn_nexttok(ep, &sz)))
		return(EQN_EOF);

	if (1 == sz && 0 == strncmp("}", start, 1))
		return(EQN_DESCOPE);
	else if (5 == sz && 0 == strncmp("right", start, 5))
		return(EQN_DESCOPE);
	else if (5 == sz && 0 == strncmp("above", start, 5))
		return(EQN_DESCOPE);

	for (i = 0; i < (int)EQN__MAX; i++) {
		if (eqnparts[i].str.sz != sz)
			continue;
		if (strncmp(eqnparts[i].str.name, start, sz))
			continue;
		return((*eqnparts[i].fp)(ep) ? EQN_OK : EQN_ERR);
	} 

	if (1 == sz && 0 == strncmp("{", start, 1)) {
		if (EQN_DESCOPE != (c = eqn_eqn(ep, last))) {
			if (EQN_ERR != c)
				EQN_MSG(MANDOCERR_EQNSCOPE, ep);
			return(EQN_ERR);
		}
		eqn_rewind(ep);
		start = eqn_nexttok(ep, &sz);
		assert(start);
		if (1 == sz && 0 == strncmp("}", start, 1))
			return(EQN_OK);
		EQN_MSG(MANDOCERR_EQNBADSCOPE, ep);
		return(EQN_ERR);
	} 

	for (i = 0; i < (int)EQNPILE__MAX; i++) {
		if (eqnpiles[i].sz != sz)
			continue;
		if (strncmp(eqnpiles[i].name, start, sz))
			continue;
		if (NULL == (start = eqn_nexttok(ep, &sz))) {
			EQN_MSG(MANDOCERR_EQNEOF, ep);
			return(EQN_ERR);
		}
		if (1 != sz || strncmp("{", start, 1)) {
			EQN_MSG(MANDOCERR_EQNSYNT, ep);
			return(EQN_ERR);
		}
		if (EQN_DESCOPE != (c = eqn_eqn(ep, last))) {
			if (EQN_ERR != c)
				EQN_MSG(MANDOCERR_EQNSCOPE, ep);
			return(EQN_ERR);
		}
		assert(last->last);
		last->last->pile = (enum eqn_pilet)i;
		eqn_rewind(ep);
		start = eqn_nexttok(ep, &sz);
		assert(start);
		if (1 == sz && 0 == strncmp("}", start, 1))
			return(EQN_OK);
		if (5 != sz || strncmp("above", start, 5)) {
			EQN_MSG(MANDOCERR_EQNSYNT, ep);
			return(EQN_ERR);
		}
		last->last->above = 1;
		if (EQN_DESCOPE != (c = eqn_eqn(ep, last))) {
			if (EQN_ERR != c)
				EQN_MSG(MANDOCERR_EQNSCOPE, ep);
			return(EQN_ERR);
		}
		eqn_rewind(ep);
		start = eqn_nexttok(ep, &sz);
		assert(start);
		if (1 == sz && 0 == strncmp("}", start, 1))
			return(EQN_OK);
		EQN_MSG(MANDOCERR_EQNBADSCOPE, ep);
		return(EQN_ERR);
	}

	if (4 == sz && 0 == strncmp("left", start, 4)) {
		if (NULL == (start = eqn_nexttok(ep, &sz))) {
			EQN_MSG(MANDOCERR_EQNEOF, ep);
			return(EQN_ERR);
		}
		left = mandoc_strndup(start, sz);
		if (EQN_DESCOPE != (c = eqn_eqn(ep, last)))
			return(c);
		assert(last->last);
		last->last->left = left;
		eqn_rewind(ep);
		start = eqn_nexttok(ep, &sz);
		assert(start);
		if (5 != sz || strncmp("right", start, 5))
			return(EQN_DESCOPE);
		if (NULL == (start = eqn_nexttok(ep, &sz))) {
			EQN_MSG(MANDOCERR_EQNEOF, ep);
			return(EQN_ERR);
		}
		last->last->right = mandoc_strndup(start, sz);
		return(EQN_OK);
	}

	for (i = 0; i < (int)EQNPOS__MAX; i++) {
		if (eqnposs[i].sz != sz)
			continue;
		if (strncmp(eqnposs[i].name, start, sz))
			continue;
		if (NULL == last->last) {
			EQN_MSG(MANDOCERR_EQNSYNT, ep);
			return(EQN_ERR);
		} 
		last->last->pos = (enum eqn_post)i;
		if (EQN_EOF == (c = eqn_box(ep, last))) {
			EQN_MSG(MANDOCERR_EQNEOF, ep);
			return(EQN_ERR);
		}
		return(c);
	}

	for (i = 0; i < (int)EQNMARK__MAX; i++) {
		if (eqnmarks[i].sz != sz)
			continue;
		if (strncmp(eqnmarks[i].name, start, sz))
			continue;
		if (NULL == last->last) {
			EQN_MSG(MANDOCERR_EQNSYNT, ep);
			return(EQN_ERR);
		} 
		last->last->mark = (enum eqn_markt)i;
		if (EQN_EOF == (c = eqn_box(ep, last))) {
			EQN_MSG(MANDOCERR_EQNEOF, ep);
			return(EQN_ERR);
		}
		return(c);
	}

	for (i = 0; i < (int)EQNFONT__MAX; i++) {
		if (eqnfonts[i].sz != sz)
			continue;
		if (strncmp(eqnfonts[i].name, start, sz))
			continue;
		if (EQN_EOF == (c = eqn_box(ep, last))) {
			EQN_MSG(MANDOCERR_EQNEOF, ep);
			return(EQN_ERR);
		} else if (EQN_OK == c)
			last->last->font = (enum eqn_fontt)i;
		return(c);
	}

	if (4 == sz && 0 == strncmp("size", start, 4)) {
		if (NULL == (start = eqn_nexttok(ep, &sz))) {
			EQN_MSG(MANDOCERR_EQNEOF, ep);
			return(EQN_ERR);
		}
		size = mandoc_strntoi(start, sz, 10);
		if (EQN_EOF == (c = eqn_box(ep, last))) {
			EQN_MSG(MANDOCERR_EQNEOF, ep);
			return(EQN_ERR);
		} else if (EQN_OK != c)
			return(c);
		last->last->size = size;
	}

	bp = eqn_box_alloc(last);
	bp->type = EQN_TEXT;
	bp->text = mandoc_strndup(start, sz);
	return(EQN_OK);
}

void
eqn_free(struct eqn_node *p)
{
	int		 i;

	eqn_box_free(p->eqn.root);

	for (i = 0; i < (int)p->defsz; i++) {
		free(p->defs[i].key);
		free(p->defs[i].val);
	}

	free(p->data);
	free(p->defs);
	free(p);
}

static struct eqn_box *
eqn_box_alloc(struct eqn_box *parent)
{
	struct eqn_box	*bp;

	bp = mandoc_calloc(1, sizeof(struct eqn_box));
	bp->parent = parent;
	bp->size = EQN_DEFSIZE;

	if (NULL == parent->first)
		parent->first = bp;
	else
		parent->last->next = bp;

	parent->last = bp;
	return(bp);
}

static void
eqn_box_free(struct eqn_box *bp)
{

	if (bp->first)
		eqn_box_free(bp->first);
	if (bp->next)
		eqn_box_free(bp->next);

	free(bp->text);
	free(bp->left);
	free(bp->right);
	free(bp);
}

static const char *
eqn_nextrawtok(struct eqn_node *ep, size_t *sz)
{

	return(eqn_next(ep, '"', sz, 0));
}

static const char *
eqn_nexttok(struct eqn_node *ep, size_t *sz)
{

	return(eqn_next(ep, '"', sz, 1));
}

static void
eqn_rewind(struct eqn_node *ep)
{

	ep->cur = ep->rew;
}

static const char *
eqn_next(struct eqn_node *ep, char quote, size_t *sz, int repl)
{
	char		*start, *next;
	int		 q, diff, lim;
	size_t		 ssz;
	struct eqn_def	*def;

	if (NULL == sz)
		sz = &ssz;

	lim = 0;
	ep->rew = ep->cur;
again:
	/* Prevent self-definitions. */

	if (lim >= EQN_NEST_MAX) {
		EQN_MSG(MANDOCERR_EQNNEST, ep);
		return(NULL);
	}

	ep->cur = ep->rew;
	start = &ep->data[(int)ep->cur];
	q = 0;

	if ('\0' == *start)
		return(NULL);

	if (quote == *start) {
		ep->cur++;
		q = 1;
	}

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

	/* Quotes aren't expanded for values. */

	if (q || ! repl)
		return(start);

	if (NULL != (def = eqn_def_find(ep, start, *sz))) {
		diff = def->valsz - *sz;

		if (def->valsz > *sz) {
			ep->sz += diff;
			ep->data = mandoc_realloc(ep->data, ep->sz + 1);
			ep->data[ep->sz] = '\0';
			start = &ep->data[(int)ep->rew];
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
eqn_do_set(struct eqn_node *ep)
{
	const char	*start;

	if (NULL == (start = eqn_nextrawtok(ep, NULL)))
		EQN_MSG(MANDOCERR_EQNARGS, ep);
	else if (NULL == (start = eqn_nextrawtok(ep, NULL)))
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

	if (NULL == (start = eqn_nextrawtok(ep, &sz))) {
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

	start = eqn_next(ep, ep->data[(int)ep->cur], &sz, 0);

	if (NULL == start) {
		EQN_MSG(MANDOCERR_EQNARGS, ep);
		return(0);
	}

	def->valsz = sz;
	def->val = mandoc_realloc(def->val, sz + 1);
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

	if (NULL == (start = eqn_nextrawtok(ep, &sz))) {
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
