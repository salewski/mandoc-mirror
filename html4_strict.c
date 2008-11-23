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
	ROFF_NONE = 0,
	ROFF_LAYOUT
};

struct rofftree;

#define	ROFFCALL_ARGS	  const struct md_args *arg,		\
			  struct md_mbuf *out,			\
			  const struct md_rbuf *in,		\
			  const char *buf, size_t sz,		\
			  size_t pos, enum roffd type,		\
			  struct rofftree *tree
typedef	int		(*roffcall)(ROFFCALL_ARGS);

static	int		  roff_Dd(ROFFCALL_ARGS);
static	int		  roff_Dt(ROFFCALL_ARGS);
static	int		  roff_Os(ROFFCALL_ARGS);
static	int		  roff_Sh(ROFFCALL_ARGS);

struct	rofftok {
	char		  id;
#define	ROFF___	 	  0
#define	ROFF_Dd		  1
#define	ROFF_Dt		  2
#define	ROFF_Os		  3
#define	ROFF_Sh		  4
#define	ROFF_Max	  5
	char		  name[2];
	roffcall	  cb;
	enum rofftype	  type;
	int		  flags;
#define	ROFF_NESTED	 (1 << 0)
};

static const struct rofftok tokens[ROFF_Max] = {
	{ ROFF___,    "\\\"",	   NULL, ROFF_NONE,	0 },
	{ ROFF_Dd,	"Dd",	roff_Dd, ROFF_NONE,	0 },
	{ ROFF_Dt,	"Dt",	roff_Dt, ROFF_NONE,	0 },
	{ ROFF_Os,	"Os",	roff_Os, ROFF_LAYOUT,	0 },
	{ ROFF_Sh,	"Sh",	roff_Sh, ROFF_LAYOUT,	0 },
};

struct	roffnode {
	int		  tok;	
	struct roffnode	 *parent;
	/* TODO: line number at acquisition. */
};

struct rofftree {
	struct roffnode	 *last;

	time_t		  date;
	char		  title[256];
	char		  section[256];
	char		  volume[256];

	int		  state;
#define	ROFF_PRELUDE	 (1 << 0)
#define	ROFF_PRELUDE_Os	 (1 << 1)
#define	ROFF_PRELUDE_Dt	 (1 << 2)
#define	ROFF_PRELUDE_Dd	 (1 << 3)
};

static int		  rofffind(const char *);
static int 		  roffparse(const struct md_args *, 
				struct md_mbuf *,
				const struct md_rbuf *, 
				const char *, size_t,
				struct rofftree *);
static int		  textparse(struct md_mbuf *, 
				const struct md_rbuf *, 
				const char *, size_t, 
				const struct rofftree *);


int
md_exit_html4_strict(const struct md_args *args, struct md_mbuf *out, 
		const struct md_rbuf *in, void *data)
{
	struct rofftree	*tree;
	int		 error;

	assert(args);
	assert(data);
	tree = (struct rofftree *)data;
	error = 0;

	while (tree->last)
		if ( ! (*tokens[tree->last->tok].cb)
				(args, error ? NULL : out, in, NULL,
				 0, 0, ROFF_EXIT, tree))
			error = 1;

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
		warnx("%s: malformed input (line %zu, col 1)", 
				in->name, in->line);
		return(0);
	} else if (ROFF_Max == (tokid = rofffind(buf + 1))) {
		warnx("%s: unknown token `%c%c' (line %zu, col 1)",
				in->name, *(buf + 1), 
				*(buf + 2), in->line);
		return(0);
	} else if (NULL == tokens[tokid].cb) 
		return(1); /* Skip token. */

	pos = 3;

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

	if (ROFF_LAYOUT == tokens[tokid].type && 
			! (ROFF_NESTED & tokens[tokid].flags)) {
		for (node = tree->last; node; node = node->parent) {
			if (node->tok == tokid)
				break;

			/* Don't break nested scope. */

			if ( ! (ROFF_NESTED & tokens[node->tok].flags))
				continue;
			warnx("%s: scope of %s broken by %s "
					"(line %zu, col %zu)",
					in->name, tokens[tokid].name,
					tokens[node->tok].name,
					in->line, pos);
			return(0);
		}
	}
	if (node) {
		assert(ROFF_LAYOUT == tokens[tokid].type);
		assert( ! (ROFF_NESTED & tokens[tokid].flags));
		assert(node->tok == tokid);

		/* Clear up to last scoped token. */

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
	for (i = 0; i < ROFF_Max; i++)
		if (0 == strncmp(name, tokens[i].name, 2))
			return(i);
	
	return(ROFF_Max);
}


/* ARGUSED */
static	int
roff_Dd(ROFFCALL_ARGS)
{

	assert(in);
	assert(tree);
	assert(arg);
	assert(out);
	assert(buf);
	assert(sz > 0);
	assert(pos > 0);
	assert(type == ROFF_ENTER);

	if (tree->last) {
		warnx("%s: superfluous prelude (line %zu, col %zu)",
				in->name, in->line, pos);
		return(0);
	}

	if (0 != tree->state) {
		warnx("%s: bad manual prelude (line %zu, col %zu)",
				in->name, in->line, pos);
		return(1);
	}

	/* TODO: parse date from buffer. */

	tree->date = time(NULL);
	tree->state |= ROFF_PRELUDE_Dd;
	return(1);
}


static	int
roff_Dt(ROFFCALL_ARGS)
{

	assert(in);
	assert(tree);
	assert(arg);
	assert(out);
	assert(buf);
	assert(sz > 0);
	assert(pos > 0);
	assert(type == ROFF_ENTER);

	if (tree->last) {
		warnx("%s: superfluous prelude (line %zu, col %zu)",
				in->name, in->line, pos);
		return(0);
	}

	if ( ! (ROFF_PRELUDE_Dd & tree->state) ||
			(ROFF_PRELUDE_Os & tree->state) ||
			(ROFF_PRELUDE_Dt & tree->state)) {
		warnx("%s: bad manual prelude (line %zu, col %zu)",
				in->name, in->line, pos);
		return(1);
	}

	/* TODO: parse titles from buffer. */

	tree->state |= ROFF_PRELUDE_Dt;
	return(1);
}


static	int
roff_Os(ROFFCALL_ARGS)
{
	struct roffnode	*node;

	assert(arg);
	assert(tree);
	assert(in);

	if (ROFF_EXIT == type) {
		assert(tree->last);
		assert(tree->last->tok == ROFF_Os);

		/* TODO: flush out ML footer. */

		node = tree->last;
		tree->last = node->parent;
		free(node);

		return(1);
	} 

	assert(out);
	assert(buf);
	assert(sz > 0);
	assert(pos > 0);

	if (tree->last) {
		warnx("%s: superfluous prelude (line %zu, col %zu)",
				in->name, in->line, pos);
		return(0);
	}

	if ((ROFF_PRELUDE_Os & tree->state) ||
			! (ROFF_PRELUDE_Dt & tree->state) ||
			! (ROFF_PRELUDE_Dd & tree->state)) {
		warnx("%s: bad manual prelude (line %zu, col %zu)",
				in->name, in->line, pos);
		return(1);
	}

	node = malloc(sizeof(struct roffnode));
	if (NULL == node) {
		warn("malloc");
		return(0);
	}
	node->tok = ROFF_Os;
	node->parent = NULL;

	tree->state |= ROFF_PRELUDE_Os;
	tree->last = node;

	return(1);
}


static	int
roff_Sh(ROFFCALL_ARGS)
{

	assert(arg);
	/*assert(out);*/(void)out;
	assert(in);
	/*assert(buf);*/(void)buf;
	(void)sz;
	(void)pos;
	(void)type;
	assert(tree);
	return(1);
}

