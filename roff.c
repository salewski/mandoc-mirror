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

/* FIXME: prolog roffs can be text roffs, too. */

enum	rofftype { 
	ROFF_COMMENT, 
	ROFF_TEXT, 
	ROFF_LAYOUT 
};

#define	ROFFCALL_ARGS \
	int tok, struct rofftree *tree, \
	const char *argv[], enum roffd type

struct	rofftree;

struct	rofftok {
	int		(*cb)(ROFFCALL_ARGS);
	enum rofftype	  type;
	int		  flags;
#define	ROFF_NESTED	 (1 << 0) 
#define	ROFF_PARSED	 (1 << 1)
#define	ROFF_CALLABLE	 (1 << 2)
#define	ROFF_QUOTES	 (1 << 3)
};

struct	roffarg {
	int 		  tok;
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
	char		  os[64];
	char		  title[64];
	char		  section[64];
	char		  volume[64];
	int		  state;
#define	ROFF_PRELUDE	 (1 << 1)
#define	ROFF_PRELUDE_Os	 (1 << 2)
#define	ROFF_PRELUDE_Dt	 (1 << 3)
#define	ROFF_PRELUDE_Dd	 (1 << 4)
#define	ROFF_BODY	 (1 << 5)

	roffin		 roffin;
	roffblkin	 roffblkin;
	roffout		 roffout;
	roffblkout	 roffblkout;

	struct md_mbuf		*mbuf; /* NULL if !flush. */
	const struct md_args	*args;
	const struct md_rbuf	*rbuf;
};

static	int		  roff_Dd(ROFFCALL_ARGS);
static	int		  roff_Dt(ROFFCALL_ARGS);
static	int		  roff_Os(ROFFCALL_ARGS);

static	int		  roff_layout(ROFFCALL_ARGS);
static	int		  roff_text(ROFFCALL_ARGS);

static	struct roffnode	 *roffnode_new(int, struct rofftree *);
static	void		  roffnode_free(int, struct rofftree *);

static	int		  rofffindtok(const char *);
static	int		  rofffindarg(const char *);
static	int		  rofffindcallable(const char *);
static	int		  roffargs(int, char *, char **);
static	int 		  roffparse(struct rofftree *, char *, size_t);
static	int		  textparse(const struct rofftree *,
				const char *, size_t);


static	const struct rofftok tokens[ROFF_MAX] = {
	{        NULL, ROFF_COMMENT, 0 },
	{     roff_Dd, ROFF_TEXT, 0 },			/* Dd */
	{     roff_Dt, ROFF_TEXT, 0 },			/* Dt */
	{     roff_Os, ROFF_TEXT, 0 },			/* Os */
	{ roff_layout, ROFF_LAYOUT, ROFF_PARSED },	/* Sh */
	{ roff_layout, ROFF_LAYOUT, ROFF_PARSED }, 	/* Ss XXX */ 
	{ roff_layout, ROFF_LAYOUT, 0 }, 		/* Pp */
	{ roff_layout, ROFF_LAYOUT, 0 }, 		/* D1 */
	{ roff_layout, ROFF_LAYOUT, 0 }, 		/* Dl */
	{ roff_layout, ROFF_LAYOUT, 0 }, 		/* Bd */
	{ roff_layout, ROFF_LAYOUT, 0 }, 		/* Ed */
	{ roff_layout, ROFF_LAYOUT, 0 }, 		/* Bl */
	{ roff_layout, ROFF_LAYOUT, 0 }, 		/* El */
	{ roff_layout, ROFF_LAYOUT, 0 }, 		/* It */
	{   roff_text, ROFF_TEXT, ROFF_PARSED },	/* An */
	{   roff_text, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Li */
};

/* FIXME: multiple owners? */

static	const struct roffarg tokenargs[ROFF_ARGMAX] = {
	{ ROFF_An, 0 },				/* split */
	{ ROFF_An, 0 },				/* nosplit */
	{ ROFF_Bd, 0 },				/* ragged */
	{ ROFF_Bd, 0 },				/* unfilled */
	{ ROFF_Bd, 0 },				/* literal */
	{ ROFF_Bd, ROFF_VALUE },		/* file */
	{ ROFF_Bd, ROFF_VALUE },		/* offset */
	{ ROFF_Bl, 0 },				/* bullet */
	{ ROFF_Bl, 0 },				/* dash */
	{ ROFF_Bl, 0 },				/* hyphen */
	{ ROFF_Bl, 0 },				/* item */
	{ ROFF_Bl, 0 },				/* enum */
	{ ROFF_Bl, 0 },				/* tag */
	{ ROFF_Bl, 0 },				/* diag */
	{ ROFF_Bl, 0 },				/* hang */
	{ ROFF_Bl, 0 },				/* ohang */
	{ ROFF_Bl, 0 },				/* inset */
	{ ROFF_Bl, 0 },				/* column */
};

static	const char *const toknames[ROFF_MAX] = ROFF_NAMES;
static 	const char *const tokargnames[ROFF_ARGMAX] = ROFF_ARGNAMES;


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
				(tree->last->tok, tree, NULL, ROFF_EXIT))
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
		const struct md_rbuf *in, roffin textin, 
		roffout textout, roffblkin blkin, roffblkout blkout)
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
	tree->roffin = textin;
	tree->roffout = textout;
	tree->roffblkin = blkin;
	tree->roffblkout = blkout;

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
				tree->rbuf->name, toknames[tok], 
				tree->rbuf->line);
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
					toknames[tok], node->line, 
					toknames[node->tok],
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
					(tree->last->tok, tree, NULL, ROFF_EXIT))
				return(0);
		} while (t != tok);
	}

	/* Proceed with actual token processing. */

	argvp = (const char **)&argv[1];
	return((*tokens[tok].cb)(tok, tree, argvp, ROFF_ENTER));
}


static int
rofffindarg(const char *name)
{
	size_t		 i;

	/* FIXME: use a table, this is slow but ok for now. */

	/* LINTED */
	for (i = 0; i < ROFF_ARGMAX; i++)
		/* LINTED */
		if (0 == strcmp(name, tokargnames[i]))
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
		if (0 == strncmp(name, toknames[i], 2))
			return((int)i);
	
	return(ROFF_MAX);
}


static int
rofffindcallable(const char *name)
{
	int		 c;

	if (ROFF_MAX == (c = rofffindtok(name)))
		return(ROFF_MAX);
	return(ROFF_CALLABLE & tokens[c].flags ? c : ROFF_MAX);
}


static struct roffnode *
roffnode_new(int tokid, struct rofftree *tree)
{
	struct roffnode	*p;
	
	if (NULL == (p = malloc(sizeof(struct roffnode)))) {
		warn("malloc");
		return(NULL);
	}

	p->line = tree->rbuf->line;
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


/* ARGSUSED */
static	int
roff_Dd(ROFFCALL_ARGS)
{

	if (ROFF_BODY & tree->state) {
		assert( ! (ROFF_PRELUDE & tree->state));
		assert(ROFF_PRELUDE_Dd & tree->state);
		return(roff_text(tok, tree, argv, type));
	}

	assert(ROFF_PRELUDE & tree->state);
	assert( ! (ROFF_BODY & tree->state));

	if (ROFF_PRELUDE_Dd & tree->state ||
			ROFF_PRELUDE_Dt & tree->state) {
		warnx("%s: prelude `Dd' out-of-order (line %zu)",
				tree->rbuf->name, tree->rbuf->line);
		return(0);
	}

	/* TODO: parse date. */

	assert(NULL == tree->last);
	tree->state |= ROFF_PRELUDE_Dd;

	return(1);
}


/* ARGSUSED */
static	int
roff_Dt(ROFFCALL_ARGS)
{

	if (ROFF_BODY & tree->state) {
		assert( ! (ROFF_PRELUDE & tree->state));
		assert(ROFF_PRELUDE_Dt & tree->state);
		return(roff_text(tok, tree, argv, type));
	}

	assert(ROFF_PRELUDE & tree->state);
	assert( ! (ROFF_BODY & tree->state));

	if ( ! (ROFF_PRELUDE_Dd & tree->state) ||
			(ROFF_PRELUDE_Dt & tree->state)) {
		warnx("%s: prelude `Dt' out-of-order (line %zu)",
				tree->rbuf->name, tree->rbuf->line);
		return(0);
	}

	/* TODO: parse date. */

	assert(NULL == tree->last);
	tree->state |= ROFF_PRELUDE_Dt;

	return(1);
}


/* ARGSUSED */
static	int
roff_Os(ROFFCALL_ARGS)
{

	if (ROFF_EXIT == type) {
		assert(ROFF_PRELUDE_Os & tree->state);
		return(roff_layout(tok, tree, argv, type));
	} else if (ROFF_BODY & tree->state) {
		assert( ! (ROFF_PRELUDE & tree->state));
		assert(ROFF_PRELUDE_Os & tree->state);
		return(roff_text(tok, tree, argv, type));
	}

	assert(ROFF_PRELUDE & tree->state);
	if ( ! (ROFF_PRELUDE_Dt & tree->state) ||
			! (ROFF_PRELUDE_Dd & tree->state)) {
		warnx("%s: prelude `Os' out-of-order (line %zu)",
				tree->rbuf->name, tree->rbuf->line);
		return(0);
	}

	/* TODO: extract OS. */

	tree->state |= ROFF_PRELUDE_Os;
	tree->state &= ~ROFF_PRELUDE;
	tree->state |= ROFF_BODY;

	assert(NULL == tree->last);

	return(roff_layout(tok, tree, argv, type));
}


/* ARGUSED */
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
roff_layout(ROFFCALL_ARGS) 
{
	int		 i, c, argcp[ROFF_MAXARG];
	char		*v, *argvp[ROFF_MAXARG];

	if (ROFF_PRELUDE & tree->state) {
		warnx("%s: macro `%s' called in prelude (line %zu)",
				tree->rbuf->name, toknames[tok],
				tree->rbuf->line);
		return(0);
	}

	if (ROFF_EXIT == type) {
		roffnode_free(tok, tree);
		return((*tree->roffblkout)(tok));
	} 

	i = 0;
	while (-1 != (c = roffnextopt(&argv, &v))) {
		if (ROFF_ARGMAX == c) {
			warnx("%s: error parsing `%s' args (line %zu)", 
					tree->rbuf->name, 
					toknames[tok],
					tree->rbuf->line);
			return(0);
		}
		argcp[i] = c;
		argvp[i] = v;
		argv++;
	}

	if (NULL == roffnode_new(tok, tree))
		return(0);

	if ( ! (*tree->roffin)(tok, argcp, argvp))
		return(0);

	if ( ! (ROFF_PARSED & tokens[tok].flags)) {
		/* TODO: print all tokens. */

		if ( ! ((*tree->roffout)(tok)))
			return(0);
		return((*tree->roffblkin)(tok));
	}

	while (*argv) {
		if (2 >= strlen(*argv) && ROFF_MAX != 
				(c = rofffindcallable(*argv)))
			if ( ! (*tokens[c].cb)(c, tree, 
						argv + 1, ROFF_ENTER))
				return(0);

		/* TODO: print token. */
		argv++;
	}

	if ( ! ((*tree->roffout)(tok)))
		return(0);

	return((*tree->roffblkin)(tok));
}


/* ARGSUSED */
static int
roff_text(ROFFCALL_ARGS) 
{
	int		 i, c, argcp[ROFF_MAXARG];
	char		*v, *argvp[ROFF_MAXARG];

	if (ROFF_PRELUDE & tree->state) {
		warnx("%s: macro `%s' called in prelude (line %zu)",
				tree->rbuf->name, toknames[tok],
				tree->rbuf->line);
		return(0);
	}

	i = 0;
	while (-1 != (c = roffnextopt(&argv, &v))) {
		if (ROFF_ARGMAX == c) {
			warnx("%s: error parsing `%s' args (line %zu)", 
					tree->rbuf->name, 
					toknames[tok],
					tree->rbuf->line);
			return(0);
		}
		argcp[i] = c;
		argvp[i] = v;
		argv++;
	}

	if ( ! (*tree->roffin)(tok, argcp, argvp))
		return(0);

	if ( ! (ROFF_PARSED & tokens[tok].flags)) {
		/* TODO: print all tokens. */
		return((*tree->roffout)(tok));
	}

	while (*argv) {
		if (2 >= strlen(*argv) && ROFF_MAX != 
				(c = rofffindcallable(*argv)))
			if ( ! (*tokens[c].cb)(c, tree, 
						argv + 1, ROFF_ENTER))
				return(0);

		/* TODO: print token. */
		argv++;
	}

	return((*tree->roffout)(tok));
}
