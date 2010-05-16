/*	$Id$ */
/*
 * Copyright (c) 2010 Kristaps Dzonsons <kristaps@bsd.lv>
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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "mandoc.h"
#include "roff.h"

#define	ROFF_CTL(c) \
	('.' == (c) || '\'' == (c))
#ifdef	ROFF_DEBUG
#define	ROFF_MDEBUG(p, str) \
	fprintf(stderr, "%s: %s (%d:%d)\n", (str), \
		roffs[(p)->last->tok].name, \
	       	(p)->last->line, (p)->last->col)
#else
#define	ROFF_MDEBUG(p, str) while (/* CONSTCOND */ 0)
#endif

enum	rofft {
	ROFF_if,
	ROFF_ccond,
#if 0
	ROFF_am,
	ROFF_ami,
	ROFF_de,
	ROFF_dei,
	ROFF_ig,
	ROFF_close,
#endif
	ROFF_MAX
};

struct	roff {
	struct roffnode	*last; /* leaf of stack */
	mandocmsg	 msg; /* err/warn/fatal messages */
	void		*data; /* privdata for messages */
};

struct	roffnode {
	enum rofft	 tok; /* type of node */
	struct roffnode	*parent; /* up one in stack */
	char		*end; /* end-token: custom */
	int		 line; /* parse line */
	int		 col; /* parse col */
	int		 endspan;
};

#define	ROFF_ARGS	 struct roff *r, /* parse ctx */ \
			 enum rofft tok, /* tok of macro */ \
		 	 char **bufp, /* input buffer */ \
			 size_t *szp, /* size of input buffer */ \
			 int ln, /* parse line */ \
			 int ppos, /* original pos in buffer */ \
			 int pos, /* current pos in buffer */ \
			 int *offs /* reset offset of buffer data */

typedef	enum rofferr (*roffproc)(ROFF_ARGS);

struct	roffmac {
	const char	*name; /* macro name */
	roffproc	 proc;
};

static	enum rofferr	 roff_if(ROFF_ARGS);
static	enum rofferr	 roff_ccond(ROFF_ARGS);
#if 0
static	enum rofferr	 roff_new_close(ROFF_ARGS);
static	enum rofferr	 roff_new_ig(ROFF_ARGS);
static	enum rofferr	 roff_sub_ig(ROFF_ARGS);
#endif

const	struct roffmac	 roffs[ROFF_MAX] = {
	{ "if", roff_if },
	{ "\\}", roff_ccond },
#if 0
	{ "am", roff_sub_ig, roff_new_ig },
	{ "ami", roff_sub_ig, roff_new_ig },
	{ "de", roff_sub_ig, roff_new_ig },
	{ "dei", roff_sub_ig, roff_new_ig },
	{ "ig", roff_sub_ig, roff_new_ig },
	{ ".", NULL, roff_new_close },
#endif
};

static	void		 roff_free1(struct roff *);
static	enum rofft	 roff_hash_find(const char *);
static	int		 roffnode_push(struct roff *, 
				enum rofft, int, int);
static	void		 roffnode_pop(struct roff *);
static	enum rofft	 roff_parse(const char *, int *);


/*
 * Look up a roff token by its name.  Returns ROFF_MAX if no macro by
 * the nil-terminated string name could be found.
 */
static enum rofft
roff_hash_find(const char *p)
{
	int		 i;

	/* FIXME: make this be fast and efficient. */

	for (i = 0; i < (int)ROFF_MAX; i++)
		if (0 == strcmp(roffs[i].name, p))
			return((enum rofft)i);

	return(ROFF_MAX);
}


/*
 * Pop the current node off of the stack of roff instructions currently
 * pending.
 */
static void
roffnode_pop(struct roff *r)
{
	struct roffnode	*p;

	assert(r->last);
	p = r->last; 
	r->last = r->last->parent;
	if (p->end)
		free(p->end);
	free(p);
}


/*
 * Push a roff node onto the instruction stack.  This must later be
 * removed with roffnode_pop().
 */
static int
roffnode_push(struct roff *r, enum rofft tok, int line, int col)
{
	struct roffnode	*p;

	if (NULL == (p = calloc(1, sizeof(struct roffnode)))) {
		(*r->msg)(MANDOCERR_MEM, r->data, line, col, NULL);
		return(0);
	}

	p->tok = tok;
	p->parent = r->last;
	p->line = line;
	p->col = col;

	r->last = p;
	return(1);
}


static void
roff_free1(struct roff *r)
{

	while (r->last)
		roffnode_pop(r);
}


void
roff_reset(struct roff *r)
{

	roff_free1(r);
}


void
roff_free(struct roff *r)
{

	roff_free1(r);
	free(r);
}


struct roff *
roff_alloc(const mandocmsg msg, void *data)
{
	struct roff	*r;

	if (NULL == (r = calloc(1, sizeof(struct roff)))) {
		(*msg)(MANDOCERR_MEM, data, 0, 0, NULL);
		return(0);
	}

	r->msg = msg;
	r->data = data;
	return(r);
}


enum rofferr
roff_parseln(struct roff *r, int ln, 
		char **bufp, size_t *szp, int pos, int *offs)
{
	enum rofft	 t;
	int		 ppos;

	/* Return when in free text without a context. */

	if (r->last && ! ROFF_CTL((*bufp)[pos])) {
		/* XXX: this assumes we're just discarding. */
		while (r->last) {
			if (r->last->endspan-- < 0)
				break;
			ROFF_MDEBUG(r, "closing implicit scope");
			roffnode_pop(r);
		}
		return(ROFF_IGN);
	} else if ( ! ROFF_CTL((*bufp)[pos]))
		return(ROFF_CONT);

	/* There's nothing on the stack: make us anew. */

	ppos = pos;
	if (ROFF_MAX == (t = roff_parse(*bufp, &pos)))
		return(ROFF_CONT);

	assert(roffs[t].proc);
	return((*roffs[t].proc)(r, t, bufp, szp, ln, ppos, pos, offs));
}


int
roff_endparse(struct roff *r)
{

	if (NULL == r->last)
		return(1);
	return((*r->msg)(MANDOCERR_SCOPEEXIT, r->data, r->last->line, 
				r->last->col, NULL));
}


/*
 * Parse a roff node's type from the input buffer.  This must be in the
 * form of ".foo xxx" in the usual way.
 */
static enum rofft
roff_parse(const char *buf, int *pos)
{
	int		 j;
	char		 mac[5];
	enum rofft	 t;

	assert(ROFF_CTL(buf[*pos]));
	(*pos)++;

	while (buf[*pos] && (' ' == buf[*pos] || '\t' == buf[*pos]))
		(*pos)++;

	if ('\0' == buf[*pos])
		return(ROFF_MAX);

	for (j = 0; j < 4; j++, (*pos)++)
		if ('\0' == (mac[j] = buf[*pos]))
			break;
		else if (' ' == buf[*pos])
			break;

	if (j == 4 || j < 1)
		return(ROFF_MAX);

	mac[j] = '\0';

	if (ROFF_MAX == (t = roff_hash_find(mac)))
		return(t);

	while (buf[*pos] && ' ' == buf[*pos])
		(*pos)++;

	return(t);
}


#if 0
/* ARGSUSED */
static enum rofferr
roff_sub_ig(ROFF_ARGS)
{
	int		 i, j;

	/* Ignore free-text lines. */

	if ('.' != (*bufp)[ppos] && '\'' != (*bufp)[ppos])
		return(ROFF_IGN);

	if (r->last->end) {
		i = ppos + 1;

		while ((*bufp)[i] && ' ' == (*bufp)[i])
			i++;

		for (j = 0; r->last->end[j]; i++, j++)
			if ((*bufp)[i] != r->last->end[j])
				return(ROFF_IGN);

		if (r->last->end[j])
			return(ROFF_IGN);
		if ((*bufp)[i] && ' ' != (*bufp)[i])
			return(ROFF_IGN);

		while (' ' == (*bufp)[i])
			i++;

	} else if (ROFF_close != roff_parse(*bufp, &i))
		return(ROFF_IGN);

	roffnode_pop(r);

	if ('\0' == (*bufp)[i])
		return(ROFF_IGN);
	if ( ! (*r->msg)(MANDOCERR_ARGSLOST, r->data, ln, i, NULL))
		return(ROFF_ERR);

	return(ROFF_IGN);
}


/* ARGSUSED */
static enum rofferr
roff_new_close(ROFF_ARGS)
{

	if ( ! (*r->msg)(MANDOCERR_NOSCOPE, r->data, ln, ppos, NULL))
		return(ROFF_ERR);

	return(ROFF_IGN);
}
#endif


/* ARGSUSED */
static enum rofferr
roff_ccond(ROFF_ARGS)
{

	if (NULL == r->last || ROFF_if != r->last->tok || r->last->endspan > -1) {
		if ( ! (*r->msg)(MANDOCERR_NOSCOPE, r->data, ln, ppos, NULL))
			return(ROFF_ERR);
		return(ROFF_IGN);
	}

	ROFF_MDEBUG(r, "closing explicit scope");
	roffnode_pop(r);

	while (r->last) {
		if (--r->last->endspan < 0)
			break;

		ROFF_MDEBUG(r, "closing implicit scope");
		roffnode_pop(r);
	}

	return(ROFF_IGN);
}


/* ARGSUSED */
static enum rofferr
roff_if(ROFF_ARGS)
{

	/*
	 * Read ahead past the conditional.
	 * FIXME: this does not work, as conditionals don't end on
	 * whitespace, but are parsed according to a formal grammar.
	 * It's good enough for now, however.
	 */

	if ( ! roffnode_push(r, tok, ln, ppos))
		return(ROFF_ERR);

	while ((*bufp)[pos] && ' ' != (*bufp)[pos])
		pos++;
	while (' ' == (*bufp)[pos])
		pos++;

	/* Don't evaluate: just assume NO. */

	r->last->endspan = 1;

	if ('\\' == (*bufp)[pos] && '{' == (*bufp)[pos + 1]) {
		ROFF_MDEBUG(r, "opening explicit scope");
		r->last->endspan = -1;
		pos += 2;
	} else
		ROFF_MDEBUG(r, "opening implicit scope");

	if ('\0' == (*bufp)[pos])
		return(ROFF_IGN);

	*offs = pos;
	return(ROFF_RERUN);
}


#if 0
static enum rofferr
roff_new_ig(ROFF_ARGS)
{
	int		 i;

	if ( ! roffnode_push(r, tok, ln, ppos))
		return(ROFF_ERR);
	
	/*
	 * Other macros (not `ig') using this routine have additional
	 * crap here that we discard. 
	 */

	if (ROFF_ig != tok) {
		while ((*bufp)[ppos] && ' ' != (*bufp)[ppos])
			ppos++;
		while (' ' == (*bufp)[ppos])
			ppos++;
	}

	i = (int)ppos;

	while ((*bufp)[i] && ' ' != (*bufp)[i])
		i++;

	if (i == (int)ppos)
		return(ROFF_IGN);

	if ((*bufp)[i])
		if ( ! (*r->msg)(MANDOCERR_ARGSLOST, r->data, ln, i, NULL))
			return(ROFF_ERR);

	/*
	 * If the macro has arguments, the first argument (up to the
	 * next whitespace) is interpreted as an argument marking the
	 * macro close.  Thus, `.ig foo' will close at `.foo'.
	 *
	 * NOTE: the closing macro `.foo' in the above case is not
	 * allowed to have leading spaces with old groff!  Thus `.foo'
	 * != `. foo'.  Oh yeah, everything after the `.foo' is lost.
	 * Merry fucking Christmas.
	 */

	r->last->end = malloc((size_t)(i - ppos) + 1);
	if (NULL == r->last->end) {
		(*r->msg)(MANDOCERR_MEM, r->data, ln, ppos, NULL);
		return(ROFF_ERR);
	}

	memcpy(r->last->end, &(*bufp)[ppos], (size_t)(i - ppos));
	r->last->end[i - ppos] = '\0';

	return(ROFF_IGN);
}
#endif
