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

static const char	*eqn_nexttok(struct mparse *, int, int,
				const char **, size_t *);

/* ARGSUSED */
enum rofferr
eqn_read(struct eqn_node **epp, int ln, 
		const char *p, int pos, int *offs)
{
	size_t		  sz;
	struct eqn_node	 *ep;
	const char	 *start, *end;
	int		  i;

	if (0 == strcmp(p, ".EN")) {
		*epp = NULL;
		return(ROFF_EQN);
	}

	ep = *epp;
	end = p + pos;
	start = eqn_nexttok(ep->parse, ln, pos, &end, &sz);

	if (NULL == start)
		return(ROFF_IGN);

	if (6 == sz && 0 == strncmp("define", start, 6)) {
		if (end && '"' == *end)
			mandoc_msg(MANDOCERR_EQNQUOTE, 
					ep->parse, ln, pos, NULL);

		start = eqn_nexttok(ep->parse, ln, pos, &end, &sz);

		for (i = 0; i < (int)ep->defsz; i++)  {
			if (ep->defs[i].keysz != sz)
				continue;
			if (0 == strncmp(ep->defs[i].key, start, sz))
				break;
		}

		/*
		 * TODO: merge this code with roff_getstr().
		 */

		if (i == (int)ep->defsz) {
			ep->defsz++;
			ep->defs = mandoc_realloc
				(ep->defs, ep->defsz * 
				 sizeof(struct eqn_def));
			ep->defs[i].keysz = sz;
			ep->defs[i].key = mandoc_malloc(sz + 1);
			memcpy(ep->defs[i].key, start, sz);
			ep->defs[i].key[(int)sz] = '\0';
			ep->defs[i].val = NULL;
			ep->defs[i].valsz = 0;
		}

		start = eqn_nexttok(ep->parse, ln, pos, &end, &sz);

		ep->defs[i].valsz = sz;
		ep->defs[i].val = mandoc_realloc
			(ep->defs[i].val, sz + 1);
		memcpy(ep->defs[i].val, start, sz);
		ep->defs[i].val[(int)sz] = '\0';

		if ('\0' == *end)
			return(ROFF_IGN);

		*offs = end - (p + pos);
		assert(*offs > 0);

		return(ROFF_RERUN);
	}  else
		end = p + pos;

	if (0 == (sz = strlen(end)))
		return(ROFF_IGN);

	ep->eqn.data = mandoc_realloc(ep->eqn.data, ep->eqn.sz + sz + 1);
	if (0 == ep->eqn.sz)
		*ep->eqn.data = '\0';

	ep->eqn.sz += sz;
	strlcat(ep->eqn.data, end, ep->eqn.sz + 1);
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
