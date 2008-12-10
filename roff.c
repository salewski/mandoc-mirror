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

#include "private.h"
#include "roff.h"

/* FIXME: First letters of quoted-text interpreted in rofffindtok. */
/* FIXME: `No' not implemented. */
/* TODO: warn if Pp occurs before/after Sh etc. (see mdoc.samples). */
/* TODO: warn about empty lists. */
/* TODO: (warn) some sections need specific elements. */
/* TODO: (warn) NAME section has particular order. */
/* TODO: macros with a set number of arguments? */
/* TODO: validate Dt macro arguments. */
/* FIXME: Bl -diag supposed to ignore callable children. */

struct	roffnode {
	int		  tok;		/* Token id. */
	struct roffnode	 *parent;	/* Parent (or NULL). */
};

enum	rofferr {
	ERR_ARGEQ1,			/* Macro requires arg == 1. */
	ERR_ARGEQ0,			/* Macro requires arg == 0. */
	ERR_ARGGE1,			/* Macro requires arg >= 1. */
	ERR_ARGGE2,			/* Macro requires arg >= 2. */
	ERR_ARGLEN,			/* Macro argument too long. */
	ERR_BADARG,			/* Macro has bad arg. */
	ERR_ARGMNY,			/* Too many macro arguments. */
	ERR_NOTSUP,			/* Macro not supported. */
	ERR_DEPREC,			/* Macro deprecated. */
	ERR_PR_OOO,			/* Prelude macro bad order. */
	ERR_PR_REP,			/* Prelude macro repeated. */
	ERR_NOT_PR,			/* Not allowed in prelude. */
	WRN_SECORD,			/* Sections out-of-order. */
};

struct	rofftree {
	struct roffnode	 *last;			/* Last parsed node. */
	char		 *cur;			/* Line start. */
	struct tm	  tm;			/* `Dd' results. */
	char		  name[64];		/* `Nm' results. */
	char		  os[64];		/* `Os' results. */
	char		  title[64];		/* `Dt' results. */
	enum roffmsec	  section;
	enum roffvol	  volume;
	int		  state;
#define	ROFF_PRELUDE	 (1 << 1)		/* In roff prelude. */ /* FIXME: put into asec. */
#define	ROFF_PRELUDE_Os	 (1 << 2)		/* `Os' is parsed. */
#define	ROFF_PRELUDE_Dt	 (1 << 3)		/* `Dt' is parsed. */
#define	ROFF_PRELUDE_Dd	 (1 << 4)		/* `Dd' is parsed. */
#define	ROFF_BODY	 (1 << 5)		/* In roff body. */
	struct roffcb	  cb;			/* Callbacks. */
	void		 *arg;			/* Callbacks' arg. */
	int		  csec;			/* Current section. */
	int		  asec;			/* Thus-far sections. */
};

static	struct roffnode	 *roffnode_new(int, struct rofftree *);
static	void		  roffnode_free(struct rofftree *);
static	int		  roff_warn(const struct rofftree *, 
				const char *, char *, ...);
static	int		  roff_warnp(const struct rofftree *, 
				const char *, int, enum rofferr);
static	int		  roff_err(const struct rofftree *, 
				const char *, char *, ...);
static	int		  roff_errp(const struct rofftree *, 
				const char *, int, enum rofferr);
static	int		  roffpurgepunct(struct rofftree *, char **);
static	int		  roffscan(int, const int *);
static	int		  rofffindtok(const char *);
static	int		  rofffindarg(const char *);
static	int		  rofffindcallable(const char *);
static	int		  roffispunct(const char *);
static	int		  roffchecksec(struct rofftree *, 
				const char *, int);
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
		(void)roff_err(tree, NULL, "prelude never finished");
		goto end;
	} else if ( ! (ROFFSec_NAME & tree->asec)) {
		(void)roff_err(tree, NULL, "missing `NAME' section");
		goto end;
	} else if ( ! (ROFFSec_NMASK & tree->asec))
		(void)roff_warn(tree, NULL, "missing suggested `NAME', "
				"`SYNOPSIS', `DESCRIPTION' sections");

	for (n = tree->last; n; n = n->parent) {
		if (0 != tokens[n->tok].ctx) 
			continue;
		(void)roff_err(tree, NULL, "closing explicit scope "
				"`%s'", toknames[n->tok]);
		goto end;
	}

	while (tree->last) {
		t = tree->last->tok;
		if ( ! roffexit(tree, t))
			goto end;
	}

	if ( ! (*tree->cb.rofftail)(tree->arg, &tree->tm,
				tree->os, tree->title, 
				tree->section, tree->volume))
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

	if (0 == *buf) 
		return(roff_err(tree, buf, "blank line"));
	else if ('.' != *buf)
		return(textparse(tree, buf));

	return(roffparse(tree, buf));
}


static int
textparse(struct rofftree *tree, char *buf)
{
	char		*bufp;

	/* TODO: literal parsing. */

	if ( ! (ROFF_BODY & tree->state))
		return(roff_err(tree, buf, "data not in body"));

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
			if (0 == *buf)
				return(roff_err(tree, argv[i], 
					"unclosed quote in arg list"));
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
	if (ROFF_MAXLINEARG == i && *buf)
		return(roff_err(tree, p, "too many args"));

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

	if (ROFF_MAX == (tok = rofffindtok(buf + 1))) 
		return(roff_err(tree, buf, "bogus line macro"));
	else if ( ! roffargs(tree, tok, buf, argv)) 
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
			(tree->last->tok, tokens[tok].parents))
		return(roff_err(tree, *argvp, "`%s' has invalid "
					"parent `%s'", toknames[tok], 
					toknames[tree->last->tok]));

	if (tree->last && ! roffscan
			(tok, tokens[tree->last->tok].children))
		return(roff_err(tree, *argvp, "`%s' has invalid "
					"child `%s'", toknames[tok],
					toknames[tree->last->tok]));

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
			return(roff_err(tree, *argv, "`%s' breaks "
						"scope of prior`%s'",
						toknames[tok], 
						toknames[n->tok]));
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
			return(roff_err(tree, *argv, "`%s' breaks "
						"scope of prior `%s'",
						toknames[tok], 
						toknames[n->tok]));
		} else
			break;

	if (NULL == n)
		return(roff_err(tree, *argv, "`%s' has no starting "
					"tag `%s'", toknames[tok], 
					toknames[tokens[tok].ctx]));

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
roffchecksec(struct rofftree *tree, const char *start, int sec)
{
	int		 prior;

	switch (sec) {
	case(ROFFSec_SYNOP):
		if ((prior = ROFFSec_NAME) & tree->asec)
			return(1);
		break;
	case(ROFFSec_DESC):
		if ((prior = ROFFSec_SYNOP) & tree->asec)
			return(1);
		break;
	case(ROFFSec_RETVAL):
		if ((prior = ROFFSec_DESC) & tree->asec)
			return(1);
		break;
	case(ROFFSec_ENV):
		if ((prior = ROFFSec_RETVAL) & tree->asec)
			return(1);
		break;
	case(ROFFSec_FILES):
		if ((prior = ROFFSec_ENV) & tree->asec)
			return(1);
		break;
	case(ROFFSec_EX):
		if ((prior = ROFFSec_FILES) & tree->asec)
			return(1);
		break;
	case(ROFFSec_DIAG):
		if ((prior = ROFFSec_EX) & tree->asec)
			return(1);
		break;
	case(ROFFSec_ERRS):
		if ((prior = ROFFSec_DIAG) & tree->asec)
			return(1);
		break;
	case(ROFFSec_SEEALSO):
		if ((prior = ROFFSec_ERRS) & tree->asec)
			return(1);
		break;
	case(ROFFSec_STAND):
		if ((prior = ROFFSec_SEEALSO) & tree->asec)
			return(1);
		break;
	case(ROFFSec_HIST):
		if ((prior = ROFFSec_STAND) & tree->asec)
			return(1);
		break;
	case(ROFFSec_AUTH):
		if ((prior = ROFFSec_HIST) & tree->asec)
			return(1);
		break;
	case(ROFFSec_CAVEATS):
		if ((prior = ROFFSec_AUTH) & tree->asec)
			return(1);
		break;
	case(ROFFSec_BUGS):
		if ((prior = ROFFSec_CAVEATS) & tree->asec)
			return(1);
		break;
	default:
		return(1);
	}

	return(roff_warnp(tree, start, ROFF_Sh, WRN_SECORD));
}


/* FIXME: move this into literals.c (or similar). */
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
		if (ROFF_ATT_MAX != roff_att(*ordp))
			break;
		return(roff_errp(tree, *ordp, tok, ERR_BADARG));
	
	case (ROFF_Xr):
		if (2 == sz) {
			assert(ordp[1]);
			if (ROFF_MSEC_MAX != roff_msec(ordp[1]))
				break;
			if ( ! roff_warn(tree, start, "invalid `%s' manual "
						"section", toknames[tok]))
				return(0);
		}
		/* FALLTHROUGH */

	case (ROFF_Sx):
		/* FALLTHROUGH*/
	case (ROFF_Fn):
		if (0 != sz) 
			break;
		return(roff_errp(tree, start, tok, ERR_ARGGE1));

	case (ROFF_Nm):
		if (0 == sz) {
			if (0 != tree->name[0]) {
				ordp[0] = tree->name;
				ordp[1] = NULL;
				break;
			}
			return(roff_err(tree, start, "`Nm' not set"));
		} else if ( ! roffsetname(tree, ordp))
			return(0);
		break;

	case (ROFF_Rv):
		/* FALLTHROUGH*/
	case (ROFF_Ex):
		if (1 == sz) 
			break;
		return(roff_errp(tree, start, tok, ERR_ARGEQ1));

	case (ROFF_Sm):
		if (1 != sz)
			return(roff_errp(tree, start, tok, ERR_ARGEQ1));
		else if (0 == strcmp(ordp[0], "on") || 
				0 == strcmp(ordp[0], "off"))
			break;
		return(roff_errp(tree, *ordp, tok, ERR_BADARG));
	
	case (ROFF_Ud):
		/* FALLTHROUGH */
	case (ROFF_Ux):
		/* FALLTHROUGH */
	case (ROFF_Bt):
		if (0 == sz) 
			break;
		return(roff_errp(tree, start, tok, ERR_ARGEQ0));
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

	if (NULL == tokens[tok].cb)
		return(roff_errp(tree, *argv, tok, ERR_NOTSUP));

	if (tokens[tok].sections && ROFF_MSEC_MAX != tree->section) {
		i = 0;
		while (ROFF_MSEC_MAX != 
				(c = tokens[tok].sections[i++]))
			if (c == tree->section)
				break;
		if (ROFF_MSEC_MAX == c) {
			if ( ! roff_warn(tree, *argv, "`%s' is not a valid "
					"macro in this manual section",
					toknames[tok]))
				return(0);
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
		if ( ! roff_warn(tree, arg, "argument-like parameter `%s' to "
					"`%s'", arg, toknames[tok]))
			return(ROFF_ARGMAX);
		return(-1);
	} 
	
	if ( ! roffargok(tok, v)) {
		if ( ! roff_warn(tree, arg, "invalid argument parameter `%s' to "
				"`%s'", tokargnames[v], toknames[tok]))
			return(ROFF_ARGMAX);
		return(-1);
	} 
	
	if ( ! (ROFF_VALUE & tokenargs[v]))
		return(v);

	*in = ++argv;

	if (NULL == *argv) {
		(void)roff_err(tree, arg, "empty value of `%s' for `%s'",
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
	size_t		 sz;

	if (ROFF_BODY & tree->state) {
		assert( ! (ROFF_PRELUDE & tree->state));
		assert(ROFF_PRELUDE_Dd & tree->state);
		return(roff_text(tok, tree, argv, type));
	}

	assert(ROFF_PRELUDE & tree->state);
	assert( ! (ROFF_BODY & tree->state));

	if (ROFF_PRELUDE_Dd & tree->state)
		return(roff_errp(tree, *argv, tok, ERR_PR_REP));
	if (ROFF_PRELUDE_Dt & tree->state)
		return(roff_errp(tree, *argv, tok, ERR_PR_OOO));

	assert(NULL == tree->last);

	argv++;

	/*
	 * This is a bit complex because there are many forms the date
	 * can be in:  it can be simply $Mdocdate$, $Mdocdate <date>$,
	 * or a raw date.  Process accordingly.
	 */

	if (0 == strcmp(*argv, "$Mdocdate$")) {
		t = time(NULL);
		if (NULL == localtime_r(&t, &tree->tm))
			err(1, "localtime_r");
		tree->state |= ROFF_PRELUDE_Dd;
		return(1);
	} 

	buf[0] = 0;
	p = *argv;
	sz = sizeof(buf);

	if (0 != strcmp(*argv, "$Mdocdate:")) {
		while (*argv) {
			if (strlcat(buf, *argv++, sz) < sz)
				continue;
			return(roff_errp(tree, p, tok, ERR_BADARG));
		}
		if (strptime(buf, "%b%d,%Y", &tree->tm)) {
			tree->state |= ROFF_PRELUDE_Dd;
			return(1);
		}
		return(roff_errp(tree, p, tok, ERR_BADARG));
	}

	argv++;

	while (*argv && **argv != '$') {
		if (strlcat(buf, *argv++, sz) >= sz)
			return(roff_errp(tree, p, tok, ERR_BADARG));
		if (strlcat(buf, " ", sz) >= sz) 
			return(roff_errp(tree, p, tok, ERR_BADARG));
	}

	if (NULL == *argv) 
		return(roff_errp(tree, p, tok, ERR_BADARG));
	if (NULL == strptime(buf, "%b %d %Y", &tree->tm)) 
		return(roff_errp(tree, p, tok, ERR_BADARG));

	tree->state |= ROFF_PRELUDE_Dd;
	return(1);
}


/* ARGSUSED */
static	int
roff_Dt(ROFFCALL_ARGS)
{
	size_t		 sz;

	if (ROFF_BODY & tree->state) {
		assert( ! (ROFF_PRELUDE & tree->state));
		assert(ROFF_PRELUDE_Dt & tree->state);
		return(roff_text(tok, tree, argv, type));
	}

	assert(ROFF_PRELUDE & tree->state);
	assert( ! (ROFF_BODY & tree->state));

	if ( ! (ROFF_PRELUDE_Dd & tree->state))
		return(roff_errp(tree, *argv, tok, ERR_PR_OOO));
	if (ROFF_PRELUDE_Dt & tree->state)
		return(roff_errp(tree, *argv, tok, ERR_PR_REP));

	argv++;
	sz = sizeof(tree->title);

	if (NULL == *argv) 
		return(roff_errp(tree, *argv, tok, ERR_ARGGE2));
	if (strlcpy(tree->title, *argv, sz) >= sz)
		return(roff_errp(tree, *argv, tok, ERR_ARGLEN));

	argv++;
	if (NULL == *argv)
		return(roff_errp(tree, *argv, tok, ERR_ARGGE2));

	if (ROFF_MSEC_MAX == (tree->section = roff_msec(*argv)))
		return(roff_errp(tree, *argv, tok, ERR_BADARG));

	argv++;

	if (NULL == *argv) {
		switch (tree->section) {
		case(ROFF_MSEC_1):
			/* FALLTHROUGH */
		case(ROFF_MSEC_6):
			/* FALLTHROUGH */
		case(ROFF_MSEC_7):
			tree->volume = ROFF_VOL_URM;
			break;
		case(ROFF_MSEC_2):
			/* FALLTHROUGH */
		case(ROFF_MSEC_3):
			/* FALLTHROUGH */
		case(ROFF_MSEC_3p):
			/* FALLTHROUGH */
		case(ROFF_MSEC_4):
			/* FALLTHROUGH */
		case(ROFF_MSEC_5):
			tree->volume = ROFF_VOL_PRM;
			break;
		case(ROFF_MSEC_8):
			tree->volume = ROFF_VOL_PRM;
			break;
		case(ROFF_MSEC_9):
			tree->volume = ROFF_VOL_KM;
			break;
		case(ROFF_MSEC_UNASS):
			/* FALLTHROUGH */
		case(ROFF_MSEC_DRAFT):
			/* FALLTHROUGH */
		case(ROFF_MSEC_PAPER):
			tree->volume = ROFF_VOL_NONE;
			break;
		default:
			abort();
			/* NOTREACHED */
		}
	} else if (ROFF_VOL_MAX == (tree->volume = roff_vol(*argv)))
		return(roff_errp(tree, *argv, tok, ERR_BADARG));

	assert(NULL == tree->last);
	tree->state |= ROFF_PRELUDE_Dt;

	return(1);
}


static int
roffsetname(struct rofftree *tree, char **ordp)
{
	size_t		 sz;
	
	assert(*ordp);

	/* FIXME: not all sections can set this. */

	if (NULL != *(ordp + 1))
		return(roff_errp(tree, *ordp, ROFF_Nm, ERR_ARGMNY));
	
	sz = sizeof(tree->name);
	if (strlcpy(tree->name, *ordp, sz) >= sz)
		return(roff_errp(tree, *ordp, ROFF_Nm, ERR_ARGLEN));

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
	size_t		 sz;

	if (ROFF_BODY & tree->state) {
		assert( ! (ROFF_PRELUDE & tree->state));
		assert(ROFF_PRELUDE_Os & tree->state);
		return(roff_text(tok, tree, argv, type));
	}

	assert(ROFF_PRELUDE & tree->state);
	if ( ! (ROFF_PRELUDE_Dt & tree->state) ||
			! (ROFF_PRELUDE_Dd & tree->state)) 
		return(roff_errp(tree, *argv, tok, ERR_PR_OOO));

	tree->os[0] = 0;

	p = *++argv;
	sz = sizeof(tree->os);

	while (*argv)
		if (strlcat(tree->os, *argv++, sz) >= sz)
			return(roff_errp(tree, p, tok, ERR_ARGLEN));

	if (0 == tree->os[0])
		if (strlcpy(tree->os, "LOCAL", sz) >= sz)
			return(roff_errp(tree, p, tok, ERR_ARGLEN));

	tree->state |= ROFF_PRELUDE_Os;
	tree->state &= ~ROFF_PRELUDE;
	tree->state |= ROFF_BODY;

	assert(ROFF_MSEC_MAX != tree->section);
	assert(0 != tree->title[0]);
	assert(0 != tree->os[0]);

	assert(NULL == tree->last);

	return((*tree->cb.roffhead)(tree->arg, &tree->tm,
				tree->os, tree->title, 
				tree->section, tree->volume));
}


/* ARGSUSED */
static int
roff_layout(ROFFCALL_ARGS) 
{
	int		 i, c, argcp[ROFF_MAXLINEARG];
	char		*argvp[ROFF_MAXLINEARG];

	/*
	 * The roff_layout function is for multi-line macros.  A layout
	 * has a start and end point, which is either declared
	 * explicitly or implicitly.  An explicit start and end is
	 * embodied by `.Bl' and `.El', with the former being the start
	 * and the latter being an end.  The `.Sh' and `.Ss' tags, on
	 * the other hand, are implicit.  The scope of a layout is the
	 * space between start and end.  Explicit layouts may not close
	 * out implicit ones and vice versa; implicit layouts may close
	 * out other implicit layouts.
	 */

	assert( ! (ROFF_CALLABLE & tokens[tok].flags));

	if (ROFF_PRELUDE & tree->state)
		return(roff_errp(tree, *argv, tok, ERR_NOT_PR));

	if (ROFF_EXIT == type) {
		roffnode_free(tree);
		if ( ! (*tree->cb.roffblkbodyout)(tree->arg, tok))
			return(0);
		return((*tree->cb.roffblkout)(tree->arg, tok));
	} 

	argv++;
	assert( ! (ROFF_CALLABLE & tokens[tok].flags));

	if ( ! roffparseopts(tree, tok, &argv, argcp, argvp))
		return(0);
	if (NULL == roffnode_new(tok, tree))
		return(0);

	/*
	 * Layouts have two parts: the layout body and header.  The
	 * layout header is the trailing text of the line macro, while
	 * the layout body is everything following until termination.
	 * Example:
	 *
	 * .It Fl f ) ;
	 * Bar.
	 *
	 * ...Produces...
	 *
	 * <block>
	 * 	<head>
	 * 		<!Fl f!> ;
	 * 	</head>
	 * 	
	 *	<body>
	 *		Bar.
	 *	</body>
	 * </block>
	 */

	if ( ! (*tree->cb.roffblkin)(tree->arg, tok, argcp, 
				(const char **)argvp))
		return(0);

	/* +++ Begin run macro-specific hooks over argv. */

	switch (tok) {
	case (ROFF_Sh):
		if (NULL == *argv) {
			argv--;
			return(roff_errp(tree, *argv, tok, ERR_ARGGE1));
		}

		tree->csec = roff_sec((const char **)argv);

		if ( ! (ROFFSec_OTHER & tree->csec) &&
				tree->asec & tree->csec) 
			if ( ! roff_warn(tree, *argv, "section repeated"))
				return(0);

		if (0 == tree->asec && ! (ROFFSec_NAME & tree->csec))
			return(roff_err(tree, *argv, "`NAME' section "
						"must be first"));
		if ( ! roffchecksec(tree, *argv, tree->csec))
			return(0);

		tree->asec |= tree->csec;
		break;
	default:
		break;
	}

	/* --- End run macro-specific hooks over argv. */

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
		return((*tree->cb.roffblkbodyin)(tree->arg, tok, argcp,
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
	return((*tree->cb.roffblkbodyin)(tree->arg, 
				tok, argcp, (const char **)argvp));
}


/* ARGSUSED */
static int
roff_ordered(ROFFCALL_ARGS) 
{
	int		 i, first, c, argcp[ROFF_MAXLINEARG];
	char		*ordp[ROFF_MAXLINEARG], *p,
			*argvp[ROFF_MAXLINEARG];

	/*
	 * Ordered macros pass their arguments directly to handlers,
	 * instead of considering it free-form text.  Thus, the
	 * following macro looks as follows:
	 *
	 * .Xr foo 1 ) ,
	 *
	 * .Xr arg1 arg2 punctuation
	 */

	if (ROFF_PRELUDE & tree->state)
		return(roff_errp(tree, *argv, tok, ERR_NOT_PR));

	first = (*argv == tree->cur);
	p = *argv++;
	ordp[0] = NULL;

	if ( ! roffparseopts(tree, tok, &argv, argcp, argvp))
		return(0);

	if (NULL == *argv)
		return(roffspecial(tree, tok, p, argcp, 
					(const char **)argvp, 0, ordp));

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

	/*
	 * Text macros are similar to special tokens, except that
	 * arguments are instead flushed as pure data: we're only
	 * concerned with the macro and its arguments.  Example:
	 * 
	 * .Fl v W f ;
	 *
	 * ...Produces...
	 *
	 * <fl> v W f </fl> ;
	 */

	if (ROFF_PRELUDE & tree->state)
		return(roff_errp(tree, *argv, tok, ERR_NOT_PR));

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

	return(roff_errp(tree, *argv, tok, ERR_DEPREC));
}


static int
roff_warnp(const struct rofftree *tree, const char *pos,
		int tok, enum rofferr type)
{
	char		 *p;

	switch (type) {
	case (WRN_SECORD):
		p = "section at `%s' out of order";
		break;
	default:
		abort();
		/* NOTREACHED */
	}

	return(roff_warn(tree, pos, p, toknames[tok]));
}


static int
roff_warn(const struct rofftree *tree, const char *pos, char *fmt, ...)
{
	va_list		 ap;
	char		 buf[128];

	va_start(ap, fmt);
	(void)vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	return((*tree->cb.roffmsg)(tree->arg, 
				ROFF_WARN, tree->cur, pos, buf));
}


static int
roff_errp(const struct rofftree *tree, const char *pos, 
		int tok, enum rofferr type)
{
	char		 *p;

	switch (type) {
	case (ERR_ARGEQ1):
		p = "`%s' expects exactly one argument";
		break;
	case (ERR_ARGEQ0):
		p = "`%s' expects exactly zero arguments";
		break;
	case (ERR_ARGGE1):
		p = "`%s' expects one or more arguments";
		break;
	case (ERR_ARGGE2):
		p = "`%s' expects two or more arguments";
		break;
	case (ERR_BADARG):
		p = "invalid argument for `%s'";
		break;
	case (ERR_NOTSUP):
		p = "macro `%s' is not supported";
		break;
	case(ERR_PR_OOO):
		p = "prelude macro `%s' is out of order";
		break;
	case(ERR_PR_REP):
		p = "prelude macro `%s' repeated";
		break;
	case(ERR_ARGLEN):
		p = "macro argument for `%s' is too long";
		break;
	case(ERR_DEPREC):
		p = "macro `%s' is deprecated";
		break;
	case(ERR_NOT_PR):
		p = "macro `%s' disallowed in prelude";
		break;
	case(ERR_ARGMNY):
		p = "too many arguments for macro `%s'";
		break;
	default:
		abort();
		/* NOTREACHED */
	}

	return(roff_err(tree, pos, p, toknames[tok]));
}


static int
roff_err(const struct rofftree *tree, const char *pos, char *fmt, ...)
{
	va_list		 ap;
	char		 buf[128];

	va_start(ap, fmt);
	if (-1 == vsnprintf(buf, sizeof(buf), fmt, ap))
		err(1, "vsnprintf");
	va_end(ap);

	return((*tree->cb.roffmsg)
			(tree->arg, ROFF_ERROR, tree->cur, pos, buf));
}

