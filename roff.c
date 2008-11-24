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

#define	ROFF_MAXARG	  10

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

#define	ROFFCALL_ARGS \
	struct rofftree *tree, const char *argv[], enum roffd type

struct	rofftree;

struct	rofftok {
	char		 *name;
	int		(*cb)(ROFFCALL_ARGS);
	enum rofftype	  type;
	int		  flags;
#define	ROFF_NESTED	 (1 << 0) 
#define	ROFF_PARSED	 (1 << 1)
#define	ROFF_CALLABLE	 (1 << 2)
#define	ROFF_QUOTES	 (1 << 3)
};

struct	roffarg {
	char		 *name;
	int		  flags;
#define	ROFF_VALUE	 (1 << 0)
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
	struct md_mbuf	*mbuf; /* NULL if ROFF_EXIT and error. */

	const struct md_args	*args;
	const struct md_rbuf	*rbuf;
};

#define	ROFF___	 	  0
#define	ROFF_Dd		  1
#define	ROFF_Dt		  2
#define	ROFF_Os		  3
#define	ROFF_Sh		  4
#define	ROFF_An		  5
#define	ROFF_Li		  6
#define	ROFF_MAX	  7

static	int		  roff_Dd(ROFFCALL_ARGS);
static	int		  roff_Dt(ROFFCALL_ARGS);
static	int		  roff_Os(ROFFCALL_ARGS);
static	int		  roff_Sh(ROFFCALL_ARGS);
static	int		  roff_An(ROFFCALL_ARGS);
static	int		  roff_Li(ROFFCALL_ARGS);

static	struct roffnode	 *roffnode_new(int, size_t, 
				struct rofftree *);
static	void		  roffnode_free(int, struct rofftree *);

static	int		  rofffindtok(const char *);
static	int		  rofffindarg(const char *);
static	int		  roffargs(int, char *, char **);
static	int 		  roffparse(struct rofftree *, char *, size_t);
static	int		  textparse(const struct rofftree *,
				const char *, size_t);

static	void		  dbg_enter(const struct md_args *, int);
static	void		  dbg_leave(const struct md_args *, int);


static const struct rofftok tokens[ROFF_MAX] = {
	{ "\\\"",    NULL, ROFF_COMMENT, 0 },
	{   "Dd", roff_Dd, ROFF_TITLE, 0 },
	{   "Dt", roff_Dt, ROFF_TITLE, 0 },
	{   "Os", roff_Os, ROFF_TITLE, 0 },
	{   "Sh", roff_Sh, ROFF_LAYOUT, 0 },
	{   "An", roff_An, ROFF_TEXT, ROFF_PARSED },
	{   "Li", roff_Li, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE },
};

#define	ROFF_Split	  0
#define	ROFF_Nosplit	  1
#define	ROFF_ARGMAX	  2

static const struct roffarg tokenargs[ROFF_ARGMAX] = {
	{  "split", 	0 },
	{  "nosplit",	0 },
};


int
roff_free(struct rofftree *tree, int flush)
{
	int		 error;

	assert(tree->mbuf);
	if ( ! flush)
		tree->mbuf = NULL;

	/* LINTED */
	while (tree->last)
		if ( ! (*tokens[tree->last->tok].cb)
				(tree, NULL, ROFF_EXIT))
			/* Disallow flushing. */
			tree->mbuf = NULL;

	error = tree->mbuf ? 0 : 1;

	if (tree->mbuf && (ROFF_PRELUDE & tree->state)) {
		warnx("%s: prelude never finished", 
				tree->rbuf->name);
		error = 1;
	}

	free(tree);
	return(error ? 0 : 1);
}


struct rofftree *
roff_alloc(const struct md_args *args, struct md_mbuf *out, 
		const struct md_rbuf *in)
{
	struct rofftree	*tree;

	if (NULL == (tree = calloc(1, sizeof(struct rofftree)))) {
		warn("malloc");
		return(NULL);
	}

	tree->state = ROFF_PRELUDE;
	tree->args = args;
	tree->mbuf = out;
	tree->rbuf = in;

	return(tree);
}


int
roff_engine(struct rofftree *tree, char *buf, size_t sz)
{

	if (0 == sz) {
		warnx("%s: blank line (line %zu)", 
				tree->rbuf->name, 
				tree->rbuf->line);
		return(0);
	} else if ('.' != *buf)
		return(textparse(tree, buf, sz));

	return(roffparse(tree, buf, sz));
}


static int
textparse(const struct rofftree *tree, const char *buf, size_t sz)
{
	
	if (NULL == tree->last) {
		warnx("%s: unexpected text (line %zu)",
				tree->rbuf->name, 
				tree->rbuf->line);
		return(0);
	} else if (NULL == tree->last->parent) {
		warnx("%s: disallowed text (line %zu)",
				tree->rbuf->name, 
				tree->rbuf->line);
		return(0);
	}

	/* Print text. */

	return(1);
}


static int
roffargs(int tok, char *buf, char **argv)
{
	int		 i;

	(void)tok;/* FIXME: quotable strings? */

	assert(tok >= 0 && tok < ROFF_MAX);
	assert('.' == *buf);

	/* LINTED */
	for (i = 0; *buf && i < ROFF_MAXARG; i++) {
		argv[i] = buf++;
		while (*buf && ! isspace(*buf))
			buf++;
		if (0 == *buf) {
			continue;
		}
		*buf++ = 0;
		while (*buf && isspace(*buf))
			buf++;
	}
	
	assert(i > 0);
	if (i < ROFF_MAXARG)
		argv[i] = NULL;

	return(ROFF_MAXARG > i);
}


static int
roffparse(struct rofftree *tree, char *buf, size_t sz)
{
	int		  tok, t;
	struct roffnode	 *node;
	char		 *argv[ROFF_MAXARG];
	const char	**argvp;

	assert(sz > 0);

	/*
	 * Extract the token identifier from the buffer.  If there's no
	 * callback for the token (comment, etc.) then exit immediately.
	 * We don't do any error handling (yet), so if the token doesn't
	 * exist, die.
	 */

	if (3 > sz) {
		warnx("%s: malformed line (line %zu)", 
				tree->rbuf->name, 
				tree->rbuf->line);
		return(0);
	} else if (ROFF_MAX == (tok = rofffindtok(buf + 1))) {
		warnx("%s: unknown line token `%c%c' (line %zu)",
				tree->rbuf->name, 
				*(buf + 1), *(buf + 2), 
				tree->rbuf->line);
		return(0);
	} else if (ROFF_COMMENT == tokens[tok].type) 
		/* Ignore comment tokens. */
		return(1);
	
	if ( ! roffargs(tok, buf, argv)) {
		warnx("%s: too many arguments to `%s' (line %zu)",
				tree->rbuf->name, tokens[tok].name, 
				tree->rbuf->line);
		return(0);
	}

	/* Domain cross-contamination (and sanity) checks. */

	switch (tokens[tok].type) {
	case (ROFF_TITLE):
		if (ROFF_PRELUDE & tree->state) {
			assert( ! (ROFF_BODY & tree->state));
			break;
		}
		assert(ROFF_BODY & tree->state);
		warnx("%s: prelude token `%s' in body (line %zu)",
				tree->rbuf->name, tokens[tok].name, 
				tree->rbuf->line);
		return(0);
	case (ROFF_LAYOUT):
		/* FALLTHROUGH */
	case (ROFF_TEXT):
		if (ROFF_BODY & tree->state) {
			assert( ! (ROFF_PRELUDE & tree->state));
			break;
		}
		assert(ROFF_PRELUDE & tree->state);
		warnx("%s: body token `%s' in prelude (line %zu)",
				tree->rbuf->name, tokens[tok].name, 
				tree->rbuf->line);
		return(0);
	case (ROFF_COMMENT):
		return(1);
	default:
		abort();
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

	if (ROFF_LAYOUT == tokens[tok].type && 
			! (ROFF_NESTED & tokens[tok].flags)) {
		for (node = tree->last; node; node = node->parent) {
			if (node->tok == tok)
				break;

			/* Don't break nested scope. */

			if ( ! (ROFF_NESTED & tokens[node->tok].flags))
				continue;
			warnx("%s: scope of %s (line %zu) broken by "
					"%s (line %zu)", 
					tree->rbuf->name, 
					tokens[tok].name,
					node->line, 
					tokens[node->tok].name,
					tree->rbuf->line);
			return(0);
		}
	}

	if (node) {
		assert(ROFF_LAYOUT == tokens[tok].type);
		assert( ! (ROFF_NESTED & tokens[tok].flags));
		assert(node->tok == tok);

		/* Clear up to last scoped token. */

		/* LINTED */
		do {
			t = tree->last->tok;
			if ( ! (*tokens[tree->last->tok].cb)
					(tree, NULL, ROFF_EXIT))
				return(0);
		} while (t != tok);
	}

	/* Proceed with actual token processing. */

	argvp = (const char **)&argv[1];
	return((*tokens[tok].cb)(tree, argvp, ROFF_ENTER));
}


static int
rofffindarg(const char *name)
{
	size_t		 i;

	/* FIXME: use a table, this is slow but ok for now. */

	/* LINTED */
	for (i = 0; i < ROFF_ARGMAX; i++)
		/* LINTED */
		if (0 == strcmp(name, tokenargs[i].name))
			return((int)i);
	
	return(ROFF_ARGMAX);
}


static int
rofffindtok(const char *name)
{
	size_t		 i;

	/* FIXME: use a table, this is slow but ok for now. */

	/* LINTED */
	for (i = 0; i < ROFF_MAX; i++)
		/* LINTED */
		if (0 == strncmp(name, tokens[i].name, 2))
			return((int)i);
	
	return(ROFF_MAX);
}


/* FIXME: accept only struct rofftree *. */
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


static int dbg_lvl = 0;


static void
dbg_enter(const struct md_args *args, int tokid)
{
	int		 i;
	static char	 buf[72];

	assert(args);
	if ( ! (args->dbg & MD_DBG_TREE))
		return;
	assert(tokid >= 0 && tokid <= ROFF_MAX);

	buf[0] = buf[71] = 0;

	switch (tokens[tokid].type) {
	case (ROFF_LAYOUT):
		(void)strncat(buf, "[body-layout] ", sizeof(buf) - 1);
		break;
	case (ROFF_TEXT):
		(void)strncat(buf, "[  body-text] ", sizeof(buf) - 1);
		break;
	case (ROFF_TITLE):
		(void)strncat(buf, "[    prelude] ", sizeof(buf) - 1);
		break;
	default:
		abort();
	}

	/* LINTED */
	for (i = 0; i < dbg_lvl; i++)
		(void)strncat(buf, "  ", sizeof(buf) - 1);

	(void)strncat(buf, tokens[tokid].name, sizeof(buf) - 1);

	(void)printf("%s\n", buf);

	dbg_lvl++;
}


/* FIXME: accept only struct rofftree *. */
static void
dbg_leave(const struct md_args *args, int tokid)
{
	assert(args);
	if ( ! (args->dbg & MD_DBG_TREE))
		return;

	assert(tokid >= 0 && tokid <= ROFF_MAX);
	assert(dbg_lvl > 0);
	dbg_lvl--;
}


/* FIXME: accept only struct rofftree *. */
/* ARGSUSED */
static	int
roff_Dd(ROFFCALL_ARGS)
{

	dbg_enter(tree->args, ROFF_Dd);

	assert(ROFF_PRELUDE & tree->state);
	if (ROFF_PRELUDE_Dt & tree->state ||
			ROFF_PRELUDE_Dd & tree->state) {
		warnx("%s: prelude `Dd' out-of-order (line %zu)",
				tree->rbuf->name, tree->rbuf->line);
		return(0);
	}

	assert(NULL == tree->last);
	tree->state |= ROFF_PRELUDE_Dd;

	dbg_leave(tree->args, ROFF_Dd);

	return(1);
}


/* ARGSUSED */
static	int
roff_Dt(ROFFCALL_ARGS)
{

	dbg_enter(tree->args, ROFF_Dt);

	assert(ROFF_PRELUDE & tree->state);
	if ( ! (ROFF_PRELUDE_Dd & tree->state) ||
			(ROFF_PRELUDE_Dt & tree->state)) {
		warnx("%s: prelude `Dt' out-of-order (line %zu)",
				tree->rbuf->name, tree->rbuf->line);
		return(0);
	}

	assert(NULL == tree->last);
	tree->state |= ROFF_PRELUDE_Dt;

	dbg_leave(tree->args, ROFF_Dt);

	return(1);
}


/* ARGSUSED */
static	int
roff_Os(ROFFCALL_ARGS)
{

	if (ROFF_EXIT == type) {
		roffnode_free(ROFF_Os, tree);
		dbg_leave(tree->args, ROFF_Os);
		return(1);
	} 

	dbg_enter(tree->args, ROFF_Os);

	assert(ROFF_PRELUDE & tree->state);
	if ( ! (ROFF_PRELUDE_Dt & tree->state) ||
			! (ROFF_PRELUDE_Dd & tree->state)) {
		warnx("%s: prelude `Os' out-of-order (line %zu)",
				tree->rbuf->name, tree->rbuf->line);
		return(0);
	}

	assert(NULL == tree->last);
	if (NULL == roffnode_new(ROFF_Os, tree->rbuf->line, tree))
		return(0);

	tree->state |= ROFF_PRELUDE_Os;
	tree->state &= ~ROFF_PRELUDE;
	tree->state |= ROFF_BODY;

	return(1);
}


/* ARGSUSED */
static int
roff_Sh(ROFFCALL_ARGS)
{

	if (ROFF_EXIT == type) {
		roffnode_free(ROFF_Sh, tree);
		dbg_leave(tree->args, ROFF_Sh);
		return(1);
	} 

	dbg_enter(tree->args, ROFF_Sh);

	if (NULL == roffnode_new(ROFF_Sh, tree->rbuf->line, tree))
		return(0);

	return(1);
}


/* ARGSUSED */
static int
roff_Li(ROFFCALL_ARGS) 
{

	dbg_enter(tree->args, ROFF_Li);
	dbg_leave(tree->args, ROFF_Li);

	return(1);
}


static int
roffnextopt(const char ***in, char **val)
{
	const char	*arg, **argv;
	int		 v;

	*val = NULL;
	argv = *in;
	assert(argv);

	if (NULL == (arg = *argv))
		return(-1);
	if ('-' != *arg)
		return(-1);
	if (ROFF_ARGMAX == (v = rofffindarg(&arg[1])))
		return(-1);
	if ( ! (ROFF_VALUE & tokenargs[v].flags))
		return(v);

	*in = ++argv;

	/* FIXME: what if this looks like a roff token or argument? */

	return(*argv ? v : ROFF_ARGMAX);
}


/* ARGSUSED */
static int
roff_An(ROFFCALL_ARGS) 
{
	int		 c;
	char		*val;

	dbg_enter(tree->args, ROFF_An);

	while (-1 != (c = roffnextopt(&argv, &val))) {
		switch (c) {
		case (ROFF_Split):
			/* Process argument. */
			break;
		case (ROFF_Nosplit):
			/* Process argument. */
			break;
		default:
			warnx("%s: error parsing `An' args (line %zu)", 
					tree->rbuf->name, 
					tree->rbuf->line);
			return(0);
		}
		argv++;
	}

	/* Print header. */
  
	while (*argv) {
		if (/* is_parsable && */ 2 >= strlen(*argv)) {
			if (ROFF_MAX != (c = rofffindtok(*argv))) {
				if (ROFF_CALLABLE & tokens[c].flags) {
					/* Call to token. */
					if ( ! (*tokens[c].cb)(tree, (const char **)argv + 1, ROFF_ENTER))
						return(0);
				} 
				/* Print token. */
			} else {
				/* Print token. */
			}
		} else {
			/* Print token. */
		}
		argv++;
	}

	/* Print footer. */

	dbg_leave(tree->args, ROFF_An);

	return(1);
}

