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

#include "mandoc.h"
#include "roff.h"

enum	rofft {
	ROFF_de,
	ROFF_dei,
	ROFF_am,
	ROFF_ami,
	ROFF_ig,
	ROFF_close,
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
	int		 line; /* parse line */
	int		 col; /* parse col */
};

#define	ROFF_ARGS	 struct roff *r, /* parse ctx */ \
		 	 char **bufp, /* input buffer */ \
			 size_t *szp, /* size of input buffer */ \
			 int ln, /* parse line */ \
			 int ppos /* current pos in buffer */

typedef	enum rofferr (*roffproc)(ROFF_ARGS);

struct	roffmac {
	const char	*name; /* macro name */
	roffproc	 sub; /* child of control black */
	roffproc	 new; /* root of stack (type = ROFF_MAX) */
};

static	enum rofferr	 roff_ignore(ROFF_ARGS);
static	enum rofferr	 roff_new_close(ROFF_ARGS);
static	enum rofferr	 roff_new_ig(ROFF_ARGS);
static	enum rofferr	 roff_sub_ig(ROFF_ARGS);

const	struct roffmac	 roffs[ROFF_MAX] = {
	{ "de", NULL, roff_ignore },
	{ "dei", NULL, roff_ignore },
	{ "am", NULL, roff_ignore },
	{ "ami", NULL, roff_ignore },
	{ "ig", roff_sub_ig, roff_new_ig },
	{ ".", NULL, roff_new_close },
};

static	void		 roff_alloc1(struct roff *);
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

	if (NULL == (p = r->last))
		return;
	r->last = p->parent;
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


static void
roff_alloc1(struct roff *r)
{

	memset(r, 0, sizeof(struct roff));
}


void
roff_reset(struct roff *r)
{

	roff_free1(r);
	roff_alloc1(r);
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
roff_parseln(struct roff *r, int ln, char **bufp, size_t *szp)
{
	enum rofft	 t;
	int		 ppos;

	if (NULL != r->last) {
		/*
		 * If there's a node on the stack, then jump directly
		 * into its processing function.
		 */
		t = r->last->tok;
		assert(roffs[t].sub);
		return((*roffs[t].sub)(r, bufp, szp, ln, 0));
	} else if ('.' != (*bufp)[0] && NULL == r->last)
		/* Return when in free text without a context. */
		return(ROFF_CONT);

	/* There's nothing on the stack: make us anew. */

	if (ROFF_MAX == (t = roff_parse(*bufp, &ppos)))
		return(ROFF_CONT);

	assert(roffs[t].new);
	return((*roffs[t].new)(r, bufp, szp, ln, ppos));
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

	assert('.' == buf[0]);
	*pos = 1;

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


/* ARGSUSED */
static enum rofferr
roff_ignore(ROFF_ARGS)
{

	return(ROFF_IGN);
}


/* ARGSUSED */
static enum rofferr
roff_sub_ig(ROFF_ARGS)
{
	enum rofft	 t;
	int		 pos;

	/* Ignore free-text lines. */

	if ('.' != (*bufp)[ppos])
		return(ROFF_IGN);

	/* Ignore macros unless it's a closing macro. */

	t = roff_parse(*bufp, &pos);
	if (ROFF_close != t)
		return(ROFF_IGN);

	roffnode_pop(r);
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


/* ARGSUSED */
static enum rofferr
roff_new_ig(ROFF_ARGS)
{

	return(roffnode_push(r, ROFF_ig, ln, ppos) ? 
			ROFF_IGN : ROFF_ERR);
}


int
roff_endparse(struct roff *r)
{

	if (NULL == r->last)
		return(1);
	return((*r->msg)(MANDOCERR_SCOPEEXIT, r->data, 
				r->last->line, r->last->col, NULL));
}
