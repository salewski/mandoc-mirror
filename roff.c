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
#include <sys/param.h>
#include <sys/types.h>

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "libmdocml.h"
#include "private.h"
#include "roff.h"

/* FIXME: First letters of quoted-text interpreted in rofffindtok. */
/* FIXME: `No' not implemented. */
/* TODO: warn if Pp occurs before/after Sh etc. (see mdoc.samples). */
/* TODO: warn about "X section only" macros. */
/* TODO: warn about empty lists. */
/* TODO: (warn) some sections need specific elements. */
/* TODO: (warn) NAME section has particular order. */
/* TODO: unify empty-content tags a la <br />. */
/* TODO: macros with a set number of arguments? */
/* TODO: validate Dt macro arguments. */
/* FIXME: Bl -diag supposed to ignore callable children. */
/* FIXME: Nm has newline when used in NAME section. */

struct	roffnode {
	int		  tok;			/* Token id. */
	struct roffnode	 *parent;		/* Parent (or NULL). */
};

struct	rofftree {
	struct roffnode	 *last;			/* Last parsed node. */
	char		 *cur;			/* Line start. */
	struct tm	  tm;			/* `Dd' results. */
	char		  name[64];		/* `Nm' results. */
	char		  os[64];		/* `Os' results. */
	char		  title[64];		/* `Dt' results. */
	enum roffmsec	  section;
	char		  volume[64];		/* `Dt' results. */
	int		  state;
#define	ROFF_PRELUDE	 (1 << 1)		/* In roff prelude. */
#define	ROFF_PRELUDE_Os	 (1 << 2)		/* `Os' is parsed. */
#define	ROFF_PRELUDE_Dt	 (1 << 3)		/* `Dt' is parsed. */
#define	ROFF_PRELUDE_Dd	 (1 << 4)		/* `Dd' is parsed. */
#define	ROFF_BODY	 (1 << 5)		/* In roff body. */
	struct roffcb	  cb;			/* Callbacks. */
	void		 *arg;			/* Callbacks' arg. */
};

static	struct roffnode	 *roffnode_new(int, struct rofftree *);
static	void		  roffnode_free(struct rofftree *);
static	void		  roff_warn(const struct rofftree *, 
				const char *, char *, ...);
static	void		  roff_err(const struct rofftree *, 
				const char *, char *, ...);
static	int		  roffpurgepunct(struct rofftree *, char **);
static	int		  roffscan(int, const int *);
static	int		  rofffindtok(const char *);
static	int		  rofffindarg(const char *);
static	int		  rofffindcallable(const char *);
static	int		  roffismsec(const char *);
static	int		  roffispunct(const char *);
static	int		  roffargs(const struct rofftree *,
				int, char *, char **);
static	int		  roffargok(int, int);
static	int		  roffnextopt(const struct rofftree *,
				int, char ***, char **);
static	int		  roffparseopts(struct rofftree *, int, 
				char ***, int *, char **);
static	int		  roffcall(struct rofftree *, int, char **);
static	int		  roffexit(struct rofftree *, int);
static	int 		  roffparse(struct rofftree *, char *);
static	int		  textparse(struct rofftree *, char *);
static	int		  roffdata(struct rofftree *, int, char *);
static	int		  roffspecial(struct rofftree *, int, 
				const char *, const int *,
				const char **, size_t, char **);
static	int		  roffsetname(struct rofftree *, char **);

#ifdef __linux__ 
extern	size_t		  strlcat(char *, const char *, size_t);
extern	size_t		  strlcpy(char *, const char *, size_t);
extern	int		  vsnprintf(char *, size_t, 
				const char *, va_list);
extern	char		 *strptime(const char *, const char *,
				struct tm *);
#endif

int
roff_free(struct rofftree *tree, int flush)
{
	int		 error, t;
	struct roffnode	*n;

	error = 0;

	if ( ! flush)
		goto end;

	error = 1;

	if (ROFF_PRELUDE & tree->state) {
		roff_err(tree, NULL, "prelude never finished");
		goto end;
	} 

	for (n = tree->last; n; n = n->parent) {
		if (0 != tokens[n->tok].ctx) 
			continue;
		roff_err(tree, NULL, "closing explicit scope `%s'", 
				toknames[n->tok]);
		goto end;
	}

	while (tree->last) {
		t = tree->last->tok;
		if ( ! roffexit(tree, t))
			goto end;
	}

	if ( ! (*tree->cb.rofftail)(tree->arg))
		goto end;

	error = 0;

end:

	while (tree->last) 
		roffnode_free(tree);

	free(tree);

	return(error ? 0 : 1);
}


struct rofftree *
roff_alloc(const struct roffcb *cb, void *args)
{
	struct rofftree	*tree;

	assert(args);
	assert(cb);

	if (NULL == (tree = calloc(1, sizeof(struct rofftree))))
		err(1, "calloc");

	tree->state = ROFF_PRELUDE;
	tree->arg = args;
	tree->section = ROFF_MSEC_MAX;

	(void)memcpy(&tree->cb, cb, sizeof(struct roffcb));

	return(tree);
}


int
roff_engine(struct rofftree *tree, char *buf)
{

	tree->cur = buf;
	assert(buf);

	if (0 == *buf) {
		roff_err(tree, buf, "blank line");
		return(0);
	} else if ('.' != *buf)
		return(textparse(tree, buf));

	return(roffparse(tree, buf));
}


static int
textparse(struct rofftree *tree, char *buf)
{
	char		*bufp;

	/* TODO: literal parsing. */

	if ( ! (ROFF_BODY & tree->state)) {
		roff_err(tree, buf, "data not in body");
		return(0);
	}

	/* LINTED */
	while (*buf) {
		while (*buf && isspace(*buf))
			buf++;

		if (0 == *buf)
			break;

		bufp = buf++;

		while (*buf && ! isspace(*buf))
			buf++;

		if (0 != *buf) {
			*buf++ = 0;
			if ( ! roffdata(tree, 1, bufp))
				return(0);
			continue;
		}

		if ( ! roffdata(tree, 1, bufp))
			return(0);
		break;
	}

	return(1);
}


static int
roffargs(const struct rofftree *tree, 
		int tok, char *buf, char **argv)
{
	int		 i;
	char		*p;

	assert(tok >= 0 && tok < ROFF_MAX);
	assert('.' == *buf);

	p = buf;

	/*
	 * This is an ugly little loop.  It parses a line into
	 * space-delimited tokens.  If a quote mark is encountered, a
	 * token is alloted the entire quoted text.  If whitespace is
	 * escaped, it's included in the prior alloted token.
	 */

	/* LINTED */
	for (i = 0; *buf && i < ROFF_MAXLINEARG; i++) {
		if ('\"' == *buf) {
			argv[i] = ++buf;
			while (*buf && '\"' != *buf)
				buf++;
			if (0 == *buf) {
				roff_err(tree, argv[i], "unclosed "
						"quote in argument "
						"list for `%s'", 
						toknames[tok]);
				return(0);
			}
		} else { 
			argv[i] = buf++;
			while (*buf) {
				if ( ! isspace(*buf)) {
					buf++;
					continue;
				}
				if (*(buf - 1) == '\\') {
					buf++;
					continue;
				}
				break;
			}
			if (0 == *buf)
				continue;
		}
		*buf++ = 0;
		while (*buf && isspace(*buf))
			buf++;
	}

	assert(i > 0);
	if (ROFF_MAXLINEARG == i && *buf) {
		roff_err(tree, p, "too many arguments for `%s'", toknames
				[tok]);
		return(0);
	}

	argv[i] = NULL;
	return(1);
}


static int
roffscan(int tok, const int *tokv)
{

	if (NULL == tokv)
		return(1);

	for ( ; ROFF_MAX != *tokv; tokv++) 
		if (tok == *tokv)
			return(1);

	return(0);
}


static int
roffparse(struct rofftree *tree, char *buf)
{
	int		  tok, t;
	struct roffnode	 *n;
	char		 *argv[ROFF_MAXLINEARG];
	char		**argvp;

	if (0 != *buf && 0 != *(buf + 1) && 0 != *(buf + 2))
		if (0 == strncmp(buf, ".\\\"", 3))
			return(1);

	if (ROFF_MAX == (tok = rofffindtok(buf + 1))) {
		roff_err(tree, buf + 1, "bogus line macro");
		return(0);
	} else if ( ! roffargs(tree, tok, buf, argv)) 
		return(0);

	argvp = (char **)argv;

	/* 
	 * Prelude macros break some assumptions, so branch now. 
	 */
	
	if (ROFF_PRELUDE & tree->state) {
		assert(NULL == tree->last);
		return(roffcall(tree, tok, argvp));
	} 

	assert(ROFF_BODY & tree->state);

	/* 
	 * First check that our possible parents and parent's possible
	 * children are satisfied.  
	 */

	if (tree->last && ! roffscan
			(tree->last->tok, tokens[tok].parents)) {
		roff_err(tree, *argvp, "`%s' has invalid parent `%s'",
				toknames[tok], 
				toknames[tree->last->tok]);
		return(0);
	} 

	if (tree->last && ! roffscan
			(tok, tokens[tree->last->tok].children)) {
		roff_err(tree, *argvp, "`%s' is invalid child of `%s'",
				toknames[tok],
				toknames[tree->last->tok]);
		return(0);
	}

	/*
	 * Branch if we're not a layout token.
	 */

	if (ROFF_LAYOUT != tokens[tok].type)
		return(roffcall(tree, tok, argvp));
	if (0 == tokens[tok].ctx)
		return(roffcall(tree, tok, argvp));

	/*
	 * First consider implicit-end tags, like as follows:
	 *	.Sh SECTION 1
	 *	.Sh SECTION 2
	 * In this, we want to close the scope of the NAME section.  If
	 * there's an intermediary implicit-end tag, such as
	 *	.Sh SECTION 1
	 *	.Ss Subsection 1
	 *	.Sh SECTION 2
	 * then it must be closed as well.
	 */

	if (tok == tokens[tok].ctx) {
		/* 
		 * First search up to the point where we must close.
		 * If one doesn't exist, then we can open a new scope.
		 */

		for (n = tree->last; n; n = n->parent) {
			assert(0 == tokens[n->tok].ctx ||
					n->tok == tokens[n->tok].ctx);
			if (n->tok == tok)
				break;
			if (ROFF_SHALLOW & tokens[tok].flags) {
				n = NULL;
				break;
			}
			if (tokens[n->tok].ctx == n->tok)
				continue;
			roff_err(tree, *argv, "`%s' breaks `%s' scope",
					toknames[tok], toknames[n->tok]);
			return(0);
		}

		/*
		 * Create a new scope, as no previous one exists to
		 * close out.
		 */

		if (NULL == n)
			return(roffcall(tree, tok, argvp));

		/* 
		 * Close out all intermediary scoped blocks, then hang
		 * the current scope from our predecessor's parent.
		 */

		do {
			t = tree->last->tok;
			if ( ! roffexit(tree, t))
				return(0);
		} while (t != tok);

		return(roffcall(tree, tok, argvp));
	}

	/*
	 * Now consider explicit-end tags, where we want to close back
	 * to a specific tag.  Example:
	 * 	.Bl
	 * 	.It Item.
	 * 	.El
	 * In this, the `El' tag closes out the scope of `Bl'.
	 */

	assert(tok != tokens[tok].ctx && 0 != tokens[tok].ctx);

	/* LINTED */
	for (n = tree->last; n; n = n->parent)
		if (n->tok != tokens[tok].ctx) {
			if (n->tok == tokens[n->tok].ctx)
				continue;
			roff_err(tree, *argv, "`%s' breaks `%s' scope",
					toknames[tok], toknames[n->tok]);
			return(0);
		} else
			break;


	if (NULL == n) {
		roff_err(tree, *argv, "`%s' has no starting tag `%s'",
				toknames[tok], 
				toknames[tokens[tok].ctx]);
		return(0);
	}

	/* LINTED */
	do {
		t = tree->last->tok;
		if ( ! roffexit(tree, t))
			return(0);
	} while (t != tokens[tok].ctx);

	return(1);
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
rofffindtok(const char *buf)
{
	char		 token[4];
	int		 i;

	for (i = 0; *buf && ! isspace(*buf) && i < 3; i++, buf++)
		token[i] = *buf;

	if (i == 3) 
		return(ROFF_MAX);

	token[i] = 0;

	/* FIXME: use a table, this is slow but ok for now. */

	/* LINTED */
	for (i = 0; i < ROFF_MAX; i++)
		/* LINTED */
		if (0 == strcmp(toknames[i], token))
			return((int)i);

	return(ROFF_MAX);
}


static int
roffismsec(const char *p)
{

	if (0 == strcmp(p, "1"))
		return(ROFF_MSEC_1);
	else if (0 == strcmp(p, "2"))
		return(ROFF_MSEC_2);
	else if (0 == strcmp(p, "3"))
		return(ROFF_MSEC_3);
	else if (0 == strcmp(p, "3p"))
		return(ROFF_MSEC_3p);
	else if (0 == strcmp(p, "4"))
		return(ROFF_MSEC_4);
	else if (0 == strcmp(p, "5"))
		return(ROFF_MSEC_5);
	else if (0 == strcmp(p, "6"))
		return(ROFF_MSEC_6);
	else if (0 == strcmp(p, "7"))
		return(ROFF_MSEC_7);
	else if (0 == strcmp(p, "8"))
		return(ROFF_MSEC_8);
	else if (0 == strcmp(p, "9"))
		return(ROFF_MSEC_9);
	else if (0 == strcmp(p, "unass"))
		return(ROFF_MSEC_UNASS);
	else if (0 == strcmp(p, "draft"))
		return(ROFF_MSEC_DRAFT);
	else if (0 == strcmp(p, "paper"))
		return(ROFF_MSEC_PAPER);

	return(ROFF_MSEC_MAX);
}


static int
roffispunct(const char *p)
{

	if (0 == *p)
		return(0);
	if (0 != *(p + 1))
		return(0);

	switch (*p) {
	case('{'):
		/* FALLTHROUGH */
	case('.'):
		/* FALLTHROUGH */
	case(','):
		/* FALLTHROUGH */
	case(';'):
		/* FALLTHROUGH */
	case(':'):
		/* FALLTHROUGH */
	case('?'):
		/* FALLTHROUGH */
	case('!'):
		/* FALLTHROUGH */
	case('('):
		/* FALLTHROUGH */
	case(')'):
		/* FALLTHROUGH */
	case('['):
		/* FALLTHROUGH */
	case(']'):
		/* FALLTHROUGH */
	case('}'):
		return(1);
	default:
		break;
	}

	return(0);
}


static int
rofffindcallable(const char *name)
{
	int		 c;

	if (ROFF_MAX == (c = rofffindtok(name)))
		return(ROFF_MAX);
	assert(c >= 0 && c < ROFF_MAX);
	return(ROFF_CALLABLE & tokens[c].flags ? c : ROFF_MAX);
}


static struct roffnode *
roffnode_new(int tokid, struct rofftree *tree)
{
	struct roffnode	*p;
	
	if (NULL == (p = malloc(sizeof(struct roffnode))))
		err(1, "malloc");

	p->tok = tokid;
	p->parent = tree->last;
	tree->last = p;

	return(p);
}


static int
roffargok(int tokid, int argid)
{
	const int	*c;

	if (NULL == (c = tokens[tokid].args))
		return(0);

	for ( ; ROFF_ARGMAX != *c; c++) 
		if (argid == *c)
			return(1);

	return(0);
}


static void
roffnode_free(struct rofftree *tree)
{
	struct roffnode	*p;

	assert(tree->last);

	p = tree->last;
	tree->last = tree->last->parent;
	free(p);
}


static int
roffspecial(struct rofftree *tree, int tok, const char *start, 
		const int *argc, const char **argv, 
		size_t sz, char **ordp)
{

	switch (tok) {
	case (ROFF_At):
		if (0 == sz)
			break;
		if (0 == strcmp(*ordp, "v6"))
			break;
		else if (0 == strcmp(*ordp, "v7")) 
			break;
		else if (0 == strcmp(*ordp, "32v"))
			break;
		else if (0 == strcmp(*ordp, "V.1"))
			break;
		else if (0 == strcmp(*ordp, "V.4"))
			break;
		roff_err(tree, start, "invalid `At' arg");
		return(0);
	
	case (ROFF_Xr):
		if (2 == sz) {
			assert(ordp[1]);
			if (ROFF_MSEC_MAX != roffismsec(ordp[1]))
				break;
			roff_warn(tree, start, "invalid `%s' manual "
					"section", toknames[tok]);
		}
		/* FALLTHROUGH */

	case (ROFF_Fn):
		if (0 != sz) 
			break;
		roff_err(tree, start, "`%s' expects at least "
				"one arg", toknames[tok]);
		return(0);

	case (ROFF_Nm):
		if (0 == sz) {
			if (0 == tree->name[0]) {
				roff_err(tree, start, "`Nm' not set");
				return(0);
			}
			ordp[0] = tree->name;
			ordp[1] = NULL;
		} else if ( ! roffsetname(tree, ordp))
			return(0);
		break;

	case (ROFF_Rv):
		/* FALLTHROUGH*/
	case (ROFF_Sx):
		/* FALLTHROUGH*/
	case (ROFF_Ex):
		if (1 == sz) 
			break;
		roff_err(tree, start, "`%s' expects one arg", 
				toknames[tok]);
		return(0);

	case (ROFF_Sm):
		if (1 != sz) {
			roff_err(tree, start, "`Sm' expects one arg");
			return(0);
		} 
		
		if (0 != strcmp(ordp[0], "on") &&
				0 != strcmp(ordp[0], "off")) {
			roff_err(tree, start, "`Sm' has invalid argument");
			return(0);
		}
		break;
	
	case (ROFF_Ud):
		/* FALLTHROUGH */
	case (ROFF_Ux):
		/* FALLTHROUGH */
	case (ROFF_Bt):
		if (0 != sz) {
			roff_err(tree, start, "`%s' expects no args",
					toknames[tok]);
			return(0);
		}
		break;
	default:
		break;
	}

	return((*tree->cb.roffspecial)(tree->arg, tok, tree->cur, 
				argc, argv, (const char **)ordp));
}


static int
roffexit(struct rofftree *tree, int tok)
{

	assert(tokens[tok].cb);
	return((*tokens[tok].cb)(tok, tree, NULL, ROFF_EXIT));
}


static int
roffcall(struct rofftree *tree, int tok, char **argv)
{
	int		 i;
	enum roffmsec	 c;

	if (NULL == tokens[tok].cb) {
		roff_err(tree, *argv, "`%s' is unsupported", 
				toknames[tok]);
		return(0);
	}
	if (tokens[tok].sections && ROFF_MSEC_MAX != tree->section) {
		i = 0;
		while (ROFF_MSEC_MAX != 
				(c = tokens[tok].sections[i++]))
			if (c == tree->section)
				break;
		if (ROFF_MSEC_MAX == c) {
			roff_warn(tree, *argv, "`%s' is not a valid "
					"macro in this manual section",
					toknames[tok]);
		}
	}

	return((*tokens[tok].cb)(tok, tree, argv, ROFF_ENTER));
}


static int
roffnextopt(const struct rofftree *tree, int tok, 
		char ***in, char **val)
{
	char		*arg, **argv;
	int		 v;

	*val = NULL;
	argv = *in;
	assert(argv);

	if (NULL == (arg = *argv))
		return(-1);
	if ('-' != *arg)
		return(-1);

	if (ROFF_ARGMAX == (v = rofffindarg(arg + 1))) {
		roff_warn(tree, arg, "argument-like parameter `%s' to "
				"`%s'", arg, toknames[tok]);
		return(-1);
	} 
	
	if ( ! roffargok(tok, v)) {
		roff_warn(tree, arg, "invalid argument parameter `%s' to "
				"`%s'", tokargnames[v], toknames[tok]);
		return(-1);
	} 
	
	if ( ! (ROFF_VALUE & tokenargs[v]))
		return(v);

	*in = ++argv;

	if (NULL == *argv) {
		roff_err(tree, arg, "empty value of `%s' for `%s'",
				tokargnames[v], toknames[tok]);
		return(ROFF_ARGMAX);
	}

	return(v);
}


static int
roffpurgepunct(struct rofftree *tree, char **argv)
{
	int		 i;

	i = 0;
	while (argv[i])
		i++;
	assert(i > 0);
	if ( ! roffispunct(argv[--i]))
		return(1);
	while (i >= 0 && roffispunct(argv[i]))
		i--;
	i++;

	/* LINTED */
	while (argv[i])
		if ( ! roffdata(tree, 0, argv[i++]))
			return(0);
	return(1);
}


static int
roffparseopts(struct rofftree *tree, int tok, 
		char ***args, int *argc, char **argv)
{
	int		 i, c;
	char		*v;

	i = 0;

	while (-1 != (c = roffnextopt(tree, tok, args, &v))) {
		if (ROFF_ARGMAX == c) 
			return(0);

		argc[i] = c;
		argv[i] = v;
		i++;
		*args = *args + 1;
	}

	argc[i] = ROFF_ARGMAX;
	argv[i] = NULL;
	return(1);
}


static int
roffdata(struct rofftree *tree, int space, char *buf)
{

	if (0 == *buf)
		return(1);
	return((*tree->cb.roffdata)(tree->arg, 
				space != 0, tree->cur, buf));
}


/* ARGSUSED */
static	int
roff_Dd(ROFFCALL_ARGS)
{
	time_t		 t;
	char		*p, buf[32];

	if (ROFF_BODY & tree->state) {
		assert( ! (ROFF_PRELUDE & tree->state));
		assert(ROFF_PRELUDE_Dd & tree->state);
		return(roff_text(tok, tree, argv, type));
	}

	assert(ROFF_PRELUDE & tree->state);
	assert( ! (ROFF_BODY & tree->state));

	if (ROFF_PRELUDE_Dd & tree->state) {
		roff_err(tree, *argv, "repeated `Dd' in prelude");
		return(0);
	} else if (ROFF_PRELUDE_Dt & tree->state) {
		roff_err(tree, *argv, "out-of-order `Dd' in prelude");
		return(0);
	}

	assert(NULL == tree->last);

	argv++;

	if (0 == strcmp(*argv, "$Mdocdate$")) {
		t = time(NULL);
		if (NULL == localtime_r(&t, &tree->tm))
			err(1, "localtime_r");
		tree->state |= ROFF_PRELUDE_Dd;
		return(1);
	} 

	/* Build this from Mdocdate or raw date. */
	
	buf[0] = 0;
	p = *argv;

	if (0 != strcmp(*argv, "$Mdocdate:")) {
		while (*argv) {
			if (strlcat(buf, *argv++, sizeof(buf))
					< sizeof(buf)) 
				continue;
			roff_err(tree, p, "bad `Dd' date");
			return(0);
		}
		if (strptime(buf, "%b%d,%Y", &tree->tm)) {
			tree->state |= ROFF_PRELUDE_Dd;
			return(1);
		}
		roff_err(tree, *argv, "bad `Dd' date");
		return(0);
	}

	argv++;
	while (*argv && **argv != '$') {
		if (strlcat(buf, *argv++, sizeof(buf))
				>= sizeof(buf)) {
			roff_err(tree, p, "bad `Dd' Mdocdate");
			return(0);
		} 
		if (strlcat(buf, " ", sizeof(buf))
				>= sizeof(buf)) {
			roff_err(tree, p, "bad `Dd' Mdocdate");
			return(0);
		}
	}
	if (NULL == *argv) {
		roff_err(tree, p, "bad `Dd' Mdocdate");
		return(0);
	}

	if (NULL == strptime(buf, "%b %d %Y", &tree->tm)) {
		roff_err(tree, *argv, "bad `Dd' Mdocdate");
		return(0);
	}

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

	if ( ! (ROFF_PRELUDE_Dd & tree->state)) {
		roff_err(tree, *argv, "out-of-order `Dt' in prelude");
		return(0);
	} else if (ROFF_PRELUDE_Dt & tree->state) {
		roff_err(tree, *argv, "repeated `Dt' in prelude");
		return(0);
	}

	argv++;
	if (NULL == *argv) {
		roff_err(tree, *argv, "`Dt' needs document title");
		return(0);
	} else if (strlcpy(tree->title, *argv, sizeof(tree->title))
			>= sizeof(tree->title)) {
		roff_err(tree, *argv, "`Dt' document title too long");
		return(0);
	}

	argv++;
	if (NULL == *argv) {
		roff_err(tree, *argv, "`Dt' needs section");
		return(0);
	} 

	if (ROFF_MSEC_MAX == (tree->section = roffismsec(*argv))) {
		roff_err(tree, *argv, "bad `Dt' section");
		return(0);
	}

	argv++;
	if (NULL == *argv) {
		tree->volume[0] = 0;
	} else if (strlcpy(tree->volume, *argv, sizeof(tree->volume))
			>= sizeof(tree->volume)) {
		roff_err(tree, *argv, "`Dt' volume too long");
		return(0);
	}

	assert(NULL == tree->last);
	tree->state |= ROFF_PRELUDE_Dt;

	return(1);
}


static int
roffsetname(struct rofftree *tree, char **ordp)
{
	
	assert(*ordp);

	/* FIXME: not all sections can set this. */

	if (NULL != *(ordp + 1)) {
		roff_err(tree, *ordp, "too many `Nm' args");
		return(0);
	} 
	
	if (strlcpy(tree->name, *ordp, sizeof(tree->name)) 
			>= sizeof(tree->name)) {
		roff_err(tree, *ordp, "`Nm' arg too long");
		return(0);
	}

	return(1);
}


/* ARGSUSED */
static	int
roff_Ns(ROFFCALL_ARGS)
{
	int		 j, c, first;
	char		*morep[1];

	first = (*argv++ == tree->cur);
	morep[0] = NULL;

	if ( ! roffspecial(tree, tok, *argv, NULL, NULL, 0, morep))
		return(0);

	while (*argv) {
		if (ROFF_MAX != (c = rofffindcallable(*argv))) {
			if ( ! roffcall(tree, c, argv))
				return(0);
			break;
		}

		if ( ! roffispunct(*argv)) {
			if ( ! roffdata(tree, 1, *argv++))
				return(0);
			continue;
		}

		for (j = 0; argv[j]; j++)
			if ( ! roffispunct(argv[j]))
				break;

		if (argv[j]) {
			if ( ! roffdata(tree, 0, *argv++))
				return(0);
			continue;
		}

		break;
	}

	if ( ! first)
		return(1);

	return(roffpurgepunct(tree, argv));
}


/* ARGSUSED */
static	int
roff_Os(ROFFCALL_ARGS)
{
	char		*p;

	if (ROFF_BODY & tree->state) {
		assert( ! (ROFF_PRELUDE & tree->state));
		assert(ROFF_PRELUDE_Os & tree->state);
		return(roff_text(tok, tree, argv, type));
	}

	assert(ROFF_PRELUDE & tree->state);
	if ( ! (ROFF_PRELUDE_Dt & tree->state) ||
			! (ROFF_PRELUDE_Dd & tree->state)) {
		roff_err(tree, *argv, "out-of-order `Os' in prelude");
		return(0);
	}

	tree->os[0] = 0;

	p = *++argv;

	while (*argv) {
		if (strlcat(tree->os, *argv++, sizeof(tree->os))
				< sizeof(tree->os)) 
			continue;
		roff_err(tree, p, "`Os' value too long");
		return(0);
	}

	if (0 == tree->os[0])
		if (strlcpy(tree->os, "LOCAL", sizeof(tree->os))
				>= sizeof(tree->os)) {
			roff_err(tree, p, "`Os' value too long");
			return(0);
		}

	tree->state |= ROFF_PRELUDE_Os;
	tree->state &= ~ROFF_PRELUDE;
	tree->state |= ROFF_BODY;

	assert(ROFF_MSEC_MAX != tree->section);
	assert(0 != tree->title[0]);
	assert(0 != tree->os[0]);

	assert(NULL == tree->last);

	return((*tree->cb.roffhead)(tree->arg, &tree->tm,
				tree->os, tree->title, tree->section,
				tree->volume));
}


/* ARGSUSED */
static int
roff_layout(ROFFCALL_ARGS) 
{
	int		 i, c, argcp[ROFF_MAXLINEARG];
	char		*argvp[ROFF_MAXLINEARG];

	if (ROFF_PRELUDE & tree->state) {
		roff_err(tree, *argv, "bad `%s' in prelude", 
				toknames[tok]);
		return(0);
	} else if (ROFF_EXIT == type) {
		roffnode_free(tree);
		if ( ! (*tree->cb.roffblkbodyout)(tree->arg, tok))
			return(0);
		return((*tree->cb.roffblkout)(tree->arg, tok));
	} 

	assert( ! (ROFF_CALLABLE & tokens[tok].flags));

	++argv;

	if ( ! roffparseopts(tree, tok, &argv, argcp, argvp))
		return(0);
	if (NULL == roffnode_new(tok, tree))
		return(0);

	/*
	 * Layouts have two parts: the layout body and header.  The
	 * layout header is the trailing text of the line macro, while
	 * the layout body is everything following until termination.
	 */

	if ( ! (*tree->cb.roffblkin)(tree->arg, tok, argcp, 
				(const char **)argvp))
		return(0);
	if (NULL == *argv)
		return((*tree->cb.roffblkbodyin)
				(tree->arg, tok, argcp, 
				 (const char **)argvp));

	if ( ! (*tree->cb.roffblkheadin)(tree->arg, tok, argcp, 
				(const char **)argvp))
		return(0);

	/*
	 * If there are no parsable parts, then write remaining tokens
	 * into the layout header and exit.
	 */

	if ( ! (ROFF_PARSED & tokens[tok].flags)) {
		i = 0;
		while (*argv)
			if ( ! roffdata(tree, i++, *argv++))
				return(0);

		if ( ! (*tree->cb.roffblkheadout)(tree->arg, tok))
			return(0);
		return((*tree->cb.roffblkbodyin)
				(tree->arg, tok, argcp, 
				 (const char **)argvp));
	}

	/*
	 * Parsable elements may be in the header (or be the header, for
	 * that matter).  Follow the regular parsing rules for these.
	 */

	i = 0;
	while (*argv) {
		if (ROFF_MAX == (c = rofffindcallable(*argv))) {
			assert(tree->arg);
			if ( ! roffdata(tree, i++, *argv++))
				return(0);
			continue;
		}
		if ( ! roffcall(tree, c, argv))
			return(0);
		break;
	}

	/* 
	 * If there's trailing punctuation in the header, then write it
	 * out now.  Here we mimic the behaviour of a line-dominant text
	 * macro.
	 */

	if (NULL == *argv) {
		if ( ! (*tree->cb.roffblkheadout)(tree->arg, tok))
			return(0);
		return((*tree->cb.roffblkbodyin)
				(tree->arg, tok, argcp, 
				 (const char **)argvp));
	}

	/*
	 * Expensive.  Scan to the end of line then work backwards until
	 * a token isn't punctuation.
	 */

	if ( ! roffpurgepunct(tree, argv))
		return(0);

	if ( ! (*tree->cb.roffblkheadout)(tree->arg, tok))
		return(0);
	return((*tree->cb.roffblkbodyin)
			(tree->arg, tok, argcp, 
			 (const char **)argvp));
}


/* ARGSUSED */
static int
roff_ordered(ROFFCALL_ARGS) 
{
	int		 i, first, c, argcp[ROFF_MAXLINEARG];
	char		*ordp[ROFF_MAXLINEARG], *p,
			*argvp[ROFF_MAXLINEARG];

	if (ROFF_PRELUDE & tree->state) {
		roff_err(tree, *argv, "`%s' disallowed in prelude", 
				toknames[tok]);
		return(0);
	}

	first = (*argv == tree->cur);
	p = *argv++;

	if ( ! roffparseopts(tree, tok, &argv, argcp, argvp))
		return(0);

	if (NULL == *argv) {
		ordp[0] = NULL;
		return(roffspecial(tree, tok, p, argcp, 
					(const char **)argvp, 0, ordp));
	}

	i = 0;
	while (*argv && i < ROFF_MAXLINEARG) {
		c = ROFF_PARSED & tokens[tok].flags ?
			rofffindcallable(*argv) : ROFF_MAX;

		if (ROFF_MAX == c && ! roffispunct(*argv)) {
			ordp[i++] = *argv++;
			continue;
		}
		ordp[i] = NULL;

		if (ROFF_MAX == c)
			break;

		if ( ! roffspecial(tree, tok, p, argcp, 
					(const char **)argvp,
					(size_t)i, ordp))
			return(0);

		return(roffcall(tree, c, argv));
	}

	assert(i != ROFF_MAXLINEARG);
	ordp[i] = NULL;

	if ( ! roffspecial(tree, tok, p, argcp, 
				(const char**)argvp,
				(size_t)i, ordp))
		return(0);

	/* FIXME: error if there's stuff after the punctuation. */

	if ( ! first || NULL == *argv)
		return(1);

	return(roffpurgepunct(tree, argv));
}


/* ARGSUSED */
static int
roff_text(ROFFCALL_ARGS) 
{
	int		 i, j, first, c, argcp[ROFF_MAXLINEARG];
	char		*argvp[ROFF_MAXLINEARG];

	if (ROFF_PRELUDE & tree->state) {
		roff_err(tree, *argv, "`%s' disallowed in prelude", 
				toknames[tok]);
		return(0);
	}

	first = (*argv == tree->cur);
	argv++;

	if ( ! roffparseopts(tree, tok, &argv, argcp, argvp))
		return(0);
	if ( ! (*tree->cb.roffin)(tree->arg, tok, argcp, 
				(const char **)argvp))
		return(0);
	if (NULL == *argv)
		return((*tree->cb.roffout)(tree->arg, tok));

	if ( ! (ROFF_PARSED & tokens[tok].flags)) {
		i = 0;
		while (*argv)
			if ( ! roffdata(tree, i++, *argv++))
				return(0);

		return((*tree->cb.roffout)(tree->arg, tok));
	}

	/*
	 * Deal with punctuation.  Ugly.  Work ahead until we encounter
	 * terminating punctuation.  If we encounter it and all
	 * subsequent tokens are punctuation, then stop processing (the
	 * line-dominant macro will print these tokens after closure).
	 * If the punctuation is followed by non-punctuation, then close
	 * and re-open our scope, then continue.
	 */

	i = 0;
	while (*argv) {
		if (ROFF_MAX != (c = rofffindcallable(*argv))) {
			if ( ! (ROFF_LSCOPE & tokens[tok].flags))
				if ( ! (*tree->cb.roffout)(tree->arg, tok))
					return(0);
	
			if ( ! roffcall(tree, c, argv))
				return(0);
	
			if (ROFF_LSCOPE & tokens[tok].flags)
				if ( ! (*tree->cb.roffout)(tree->arg, tok))
					return(0);
	
			break;
		}

		if ( ! roffispunct(*argv)) {
			if ( ! roffdata(tree, i++, *argv++))
				return(0);
			continue;
		}

		i = 1;
		for (j = 0; argv[j]; j++)
			if ( ! roffispunct(argv[j]))
				break;

		if (argv[j]) {
			if (ROFF_LSCOPE & tokens[tok].flags) {
				if ( ! roffdata(tree, 0, *argv++))
					return(0);
				continue;
			}
			if ( ! (*tree->cb.roffout)(tree->arg, tok))
				return(0);
			if ( ! roffdata(tree, 0, *argv++))
				return(0);
			if ( ! (*tree->cb.roffin)(tree->arg, tok, 
						argcp, 
						(const char **)argvp))
				return(0);

			i = 0;
			continue;
		}

		if ( ! (*tree->cb.roffout)(tree->arg, tok))
			return(0);
		break;
	}

	if (NULL == *argv)
		return((*tree->cb.roffout)(tree->arg, tok));
	if ( ! first)
		return(1);

	return(roffpurgepunct(tree, argv));
}


/* ARGSUSED */
static int
roff_noop(ROFFCALL_ARGS)
{

	return(1);
}


/* ARGSUSED */
static int
roff_depr(ROFFCALL_ARGS)
{

	roff_err(tree, *argv, "`%s' is deprecated", toknames[tok]);
	return(0);
}


static void
roff_warn(const struct rofftree *tree, const char *pos, char *fmt, ...)
{
	va_list		 ap;
	char		 buf[128];

	va_start(ap, fmt);
	(void)vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	(*tree->cb.roffmsg)(tree->arg, 
			ROFF_WARN, tree->cur, pos, buf);
}


static void
roff_err(const struct rofftree *tree, const char *pos, char *fmt, ...)
{
	va_list		 ap;
	char		 buf[128];

	va_start(ap, fmt);
	(void)vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	(*tree->cb.roffmsg)(tree->arg, 
			ROFF_ERROR, tree->cur, pos, buf);
}

