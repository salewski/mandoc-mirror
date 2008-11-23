/* $Id$ */
/*
 * Copyright (c) 2008 Kristaps Dzonsons <kristaps@kth.se>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "libmdocml.h"
#include "private.h"

enum	roffd { 
	ROFF_ENTER = 0, 
	ROFF_EXIT 
};

enum	rofftype { 
	ROFF_TITLE, 
	ROFF_COMMENT, 
	ROFF_TEXT, 
	ROFF_LAYOUT 
};

#define	ROFFCALL_ARGS	  const struct md_args *arg,		\
			  struct md_mbuf *out,			\
			  const struct md_rbuf *in,		\
			  const char *buf, size_t sz,		\
			  size_t pos, enum roffd type,		\
			  struct rofftree *tree

struct	rofftree;

struct	rofftok {
	int		  id;
	char		  name[2];
	int		(*cb)(ROFFCALL_ARGS);
	enum rofftype	  type;
	int		  flags;
#define	ROFF_NESTED	 (1 << 0) /* FIXME: test. */
#define	ROFF_PARSED	 (1 << 1) /* FIXME: test. */
#define	ROFF_CALLABLE	 (1 << 2) /* FIXME: test. */
};

struct	roffnode {
	int		  tok;	
	struct roffnode	 *parent;
	size_t		  line;
};

struct	rofftree {
	struct roffnode	 *last;
	time_t		  date;
	char		  title[256];
	char		  section[256];
	char		  volume[256];
	int		  state;
#define	ROFF_PRELUDE	 (1 << 1)
#define	ROFF_PRELUDE_Os	 (1 << 2)
#define	ROFF_PRELUDE_Dt	 (1 << 3)
#define	ROFF_PRELUDE_Dd	 (1 << 4)
#define	ROFF_BODY	 (1 << 5)
};

#define	ROFF___	 	  0
#define	ROFF_Dd		  1
#define	ROFF_Dt		  2
#define	ROFF_Os		  3
#define	ROFF_Sh		  4
#define	ROFF_An		  5
#define	ROFF_Li		  6
#define	ROFF_Max	  7

static	int		  roff_Dd(ROFFCALL_ARGS);
static	int		  roff_Dt(ROFFCALL_ARGS);
static	int		  roff_Os(ROFFCALL_ARGS);
static	int		  roff_Sh(ROFFCALL_ARGS);
static	int		  roff_An(ROFFCALL_ARGS);
static	int		  roff_Li(ROFFCALL_ARGS);

static	struct roffnode	 *roffnode_new(int, size_t, 
				struct rofftree *);
static	void		  roffnode_free(int, struct rofftree *);

static	int		  rofffind(const char *);
static	int 		  roffparse(const struct md_args *, 
				struct md_mbuf *,
				const struct md_rbuf *, 
				const char *, size_t,
				struct rofftree *);
static	int		  textparse(struct md_mbuf *, 
				const struct md_rbuf *, 
				const char *, size_t, 
				const struct rofftree *);

static	void		  dbg_enter(const struct md_args *, int);
static	void		  dbg_leave(const struct md_args *, int);


static const struct rofftok tokens[ROFF_Max] = 
{
{ ROFF___, "\\\"",     NULL, ROFF_COMMENT,			  0 },
{ ROFF_Dd,   "Dd",  roff_Dd, ROFF_TITLE,			  0 },
{ ROFF_Dt,   "Dt",  roff_Dt, ROFF_TITLE,			  0 },
{ ROFF_Os,   "Os",  roff_Os, ROFF_TITLE,			  0 },
{ ROFF_Sh,   "Sh",  roff_Sh, ROFF_LAYOUT,			  0 },
{ ROFF_An,   "An",  roff_An, ROFF_TEXT,	ROFF_PARSED 		    },
{ ROFF_Li,   "Li",  roff_Li, ROFF_TEXT,	ROFF_PARSED | ROFF_CALLABLE },
};


int
md_exit_html4_strict(const struct md_args *args, struct md_mbuf *out, 
		const struct md_rbuf *in, int error, void *data)
{
	struct rofftree	*tree;

	assert(args);
	assert(data);
	tree = (struct rofftree *)data;

	if (-1 == error)
		out = NULL;

	/* LINTED */
	while (tree->last)
		if ( ! (*tokens[tree->last->tok].cb)(args, out, in, 
					NULL, 0, 0, ROFF_EXIT, tree))
			out = NULL;

	if (out && (ROFF_PRELUDE & tree->state)) {
		warnx("%s: prelude never finished", in->name);
		error = 1;
	}

	free(tree);

	return(error ? 0 : 1);
}


int
md_init_html4_strict(const struct md_args *args, struct md_mbuf *out, 
		const struct md_rbuf *in, void **data) 
{
	struct rofftree	*tree;

	assert(args);
	assert(in);
	assert(out);
	assert(data);

	/* TODO: write HTML-DTD header. */

	if (NULL == (tree = calloc(1, sizeof(struct rofftree)))) {
		warn("malloc");
		return(0);
	}

	tree->state = ROFF_PRELUDE;

	*data = tree;
	return(1);
}


int
md_line_html4_strict(const struct md_args *args, struct md_mbuf *out, 
		const struct md_rbuf *in, const char *buf, 
		size_t sz, void *data)
{
	struct rofftree	*tree;

	assert(args);
	assert(in);
	assert(data);

	tree = (struct rofftree *)data;

	if (0 == sz) {
		warnx("%s: blank line (line %zu)", in->name, in->line);
		return(0);
	} else if ('.' != *buf)
		return(textparse(out, in, buf, sz, tree));

	return(roffparse(args, out, in, buf, sz, tree));
}


static int
textparse(struct md_mbuf *out, const struct md_rbuf *in, 
		const char *buf, size_t sz, 
		const struct rofftree *tree)
{
	
	assert(tree);
	assert(out);
	assert(in);
	assert(buf);
	assert(sz > 0);

	if (NULL == tree->last) {
		warnx("%s: unexpected text (line %zu)",
				in->name, in->line);
		return(0);
	} else if (NULL == tree->last->parent) {
		warnx("%s: disallowed text (line %zu)",
				in->name, in->line);
		return(0);
	}

	if ( ! md_buf_puts(out, buf, sz))
		return(0);
	return(md_buf_putstring(out, " "));
}


static int
roffparse(const struct md_args *args, struct md_mbuf *out,
		const struct md_rbuf *in, const char *buf,
		size_t sz, struct rofftree *tree)
{
	int		 tokid, t;
	size_t		 pos;
	struct roffnode	*node;

	assert(args);
	assert(out);
	assert(in);
	assert(buf);
	assert(sz > 0);
	assert(tree);

	/*
	 * Extract the token identifier from the buffer.  If there's no
	 * callback for the token (comment, etc.) then exit immediately.
	 * We don't do any error handling (yet), so if the token doesn't
	 * exist, die.
	 */

	if (3 > sz) {
		warnx("%s: malformed line (line %zu)", 
				in->name, in->line);
		return(0);
	} else if (ROFF_Max == (tokid = rofffind(buf + 1))) {
		warnx("%s: unknown line token `%c%c' (line %zu)",
				in->name, *(buf + 1), 
				*(buf + 2), in->line);
		return(0);
	} 

	/* Domain cross-contamination (and sanity) checks. */

	switch (tokens[tokid].type) {
	case (ROFF_TITLE):
		if (ROFF_PRELUDE & tree->state) {
			assert( ! (ROFF_BODY & tree->state));
			break;
		}
		assert(ROFF_BODY & tree->state);
		warnx("%s: prelude token `%s' in body (line %zu)",
				in->name, tokens[tokid].name, in->line);
		return(0);
	case (ROFF_LAYOUT):
		/* FALLTHROUGH */
	case (ROFF_TEXT):
		if (ROFF_BODY & tree->state) {
			assert( ! (ROFF_PRELUDE & tree->state));
			break;
		}
		assert(ROFF_PRELUDE & tree->state);
		warnx("%s: text token `%s' in prelude (line %zu)",
				in->name, tokens[tokid].name, in->line);
		return(0);
	default:
		return(1);
	}

	/*
	 * Text-domain checks.
	 */

	if (ROFF_TEXT == tokens[tokid].type &&
			! (ROFF_PARSED & tokens[tokid].flags)) {
		warnx("%s: text token `%s' not callable (line %zu)",
				in->name, tokens[tokid].name, in->line);
		return(0);
	}

	/*
	 * If this is a non-nestable layout token and we're below a
	 * token of the same type, then recurse upward to the token,
	 * closing out the interim scopes.
	 *
	 * If there's a nested token on the chain, then raise an error
	 * as nested tokens have corresponding "ending" tokens and we're
	 * breaking their scope.
	 */

	node = NULL;
	pos = 3;

	if (ROFF_LAYOUT == tokens[tokid].type && 
			! (ROFF_NESTED & tokens[tokid].flags)) {
		for (node = tree->last; node; node = node->parent) {
			if (node->tok == tokid)
				break;

			/* Don't break nested scope. */

			if ( ! (ROFF_NESTED & tokens[node->tok].flags))
				continue;
			warnx("%s: scope of %s (line %zu) broken by "
					"%s (line %zu)", in->name, 
					tokens[tokid].name,
					node->line, 
					tokens[node->tok].name,
					in->line);
			return(0);
		}
	}

	if (node) {
		assert(ROFF_LAYOUT == tokens[tokid].type);
		assert( ! (ROFF_NESTED & tokens[tokid].flags));
		assert(node->tok == tokid);

		/* Clear up to last scoped token. */

		/* LINTED */
		do {
			t = tree->last->tok;
			if ( ! (*tokens[tree->last->tok].cb)
					(args, out, in, NULL,
					 0, 0, ROFF_EXIT, tree))
				return(0);
		} while (t != tokid);
	}

	/* Proceed with actual token processing. */

	return((*tokens[tokid].cb)(args, out, in, buf, sz, 
				pos, ROFF_ENTER, tree));
}


static int
rofffind(const char *name)
{
	size_t		 i;

	assert(name);
	/* FIXME: use a table, this is slow but ok for now. */

	/* LINTED */
	for (i = 0; i < ROFF_Max; i++)
		/* LINTED */
		if (0 == strncmp(name, tokens[i].name, 2))
			return((int)i);
	
	return(ROFF_Max);
}


static struct roffnode *
roffnode_new(int tokid, size_t line, struct rofftree *tree)
{
	struct roffnode	*p;
	
	if (NULL == (p = malloc(sizeof(struct roffnode)))) {
		warn("malloc");
		return(NULL);
	}

	p->line = line;
	p->tok = tokid;
	p->parent = tree->last;
	tree->last = p;
	return(p);
}


static void
roffnode_free(int tokid, struct rofftree *tree)
{
	struct roffnode	*p;

	assert(tree->last);
	assert(tree->last->tok == tokid);

	p = tree->last;
	tree->last = tree->last->parent;
	free(p);
}


static int dbg_lvl = 0; /* FIXME: de-globalise. */


static void
dbg_enter(const struct md_args *args, int tokid)
{
	int		 i;

	assert(args);
	if ( ! (args->dbg & MD_DBG_TREE))
		return;

	assert(tokid >= 0 && tokid <= ROFF_Max);

	/* LINTED */
	for (i = 0; i < dbg_lvl; i++)
		(void)printf("    ");

	(void)printf("%s\n", tokens[tokid].name);

	if (ROFF_LAYOUT == tokens[tokid].type)
		dbg_lvl++;
}


static void
dbg_leave(const struct md_args *args, int tokid)
{
	int		 i;

	assert(args);
	if ( ! (args->dbg & MD_DBG_TREE))
		return;
	if (ROFF_LAYOUT != tokens[tokid].type)
		return;

	assert(tokid >= 0 && tokid <= ROFF_Max);
	assert(dbg_lvl > 0);

	dbg_lvl--;

	/* LINTED */
	for (i = 0; i < dbg_lvl; i++) 
		(void)printf("    ");

	(void)printf("%s\n", tokens[tokid].name);
}


static	int
roff_Dd(ROFFCALL_ARGS)
{

	assert(ROFF_PRELUDE & tree->state);
	if (ROFF_PRELUDE_Dt & tree->state ||
			ROFF_PRELUDE_Dd & tree->state) {
		warnx("%s: bad prelude ordering (line %zu)",
				in->name, in->line);
		return(0);
	}

	assert(NULL == tree->last);
	tree->state |= ROFF_PRELUDE_Dd;

	dbg_enter(arg, ROFF_Dd);
	return(1);
}


static	int
roff_Dt(ROFFCALL_ARGS)
{

	assert(ROFF_PRELUDE & tree->state);
	if ( ! (ROFF_PRELUDE_Dd & tree->state) ||
			(ROFF_PRELUDE_Dt & tree->state)) {
		warnx("%s: bad prelude ordering (line %zu)",
				in->name, in->line);
		return(0);
	}

	assert(NULL == tree->last);
	tree->state |= ROFF_PRELUDE_Dt;

	dbg_enter(arg, ROFF_Dt);
	return(1);
}


static	int
roff_Os(ROFFCALL_ARGS)
{

	if (ROFF_EXIT == type) {
		roffnode_free(ROFF_Os, tree);
		dbg_leave(arg, ROFF_Os);
		return(1);
	} 

	assert(ROFF_PRELUDE & tree->state);
	if ( ! (ROFF_PRELUDE_Dt & tree->state) ||
			! (ROFF_PRELUDE_Dd & tree->state)) {
		warnx("%s: bad prelude ordering (line %zu)",
				in->name, in->line);
		return(0);
	}

	assert(NULL == tree->last);
	if (NULL == roffnode_new(ROFF_Os, in->line, tree))
		return(0);

	tree->state |= ROFF_PRELUDE_Os;
	tree->state &= ~ROFF_PRELUDE;
	tree->state |= ROFF_BODY;

	dbg_enter(arg, ROFF_Os);
	return(1);
}


static int
roff_Sh(ROFFCALL_ARGS)
{

	if (ROFF_EXIT == type) {
		roffnode_free(ROFF_Sh, tree);
		dbg_leave(arg, ROFF_Sh);
		return(1);
	} 

	if (NULL == roffnode_new(ROFF_Sh, in->line, tree))
		return(0);

	dbg_enter(arg, ROFF_Sh);
	return(1);
}


static int
roff_Li(ROFFCALL_ARGS) 
{

	return(1);
}


static int
roff_An(ROFFCALL_ARGS) 
{

	return(1);
}
