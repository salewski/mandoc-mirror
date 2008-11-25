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

/* Whether we're entering or leaving a roff scope. */
enum	roffd { 
	ROFF_ENTER = 0, 
	ROFF_EXIT 
};

/* The type of a macro (see mdoc(7) for more). */
enum	rofftype { 
	ROFF_COMMENT, 
	ROFF_TEXT, 
	ROFF_LAYOUT 
};

/* Arguments passed to a macro callback. */
#define	ROFFCALL_ARGS \
	int tok, struct rofftree *tree, \
	const char *argv[], enum roffd type

struct	rofftree;

/* Describes a roff token (like D1 or Sh). */
struct	rofftok {
	int		(*cb)(ROFFCALL_ARGS);	/* Callback. */
	const int	 *args;			/* Args (or NULL). */
	enum rofftype	  type;			/* Type of macro. */
	int		  symm;			/* FIXME */
	int		  flags;
#define	ROFF_NESTED	 (1 << 0) 		/* Nested-layout. */
#define	ROFF_PARSED	 (1 << 1)		/* "Parsed". */
#define	ROFF_CALLABLE	 (1 << 2)		/* "Callable". */
#define	ROFF_QUOTES	 (1 << 3)		/* Quoted args. */
};

/* An argument to a roff token (like -split or -enum). */
struct	roffarg {
	int		  flags;
#define	ROFF_VALUE	 (1 << 0)		/* Has a value. */
};

/* mdocml remembers only the current parse node and the chain leading to
 * the document root (scopes).
 */
struct	roffnode {
	int		  tok;			/* Token id. */
	struct roffnode	 *parent;		/* Parent (or NULL). */
	size_t		  line;			/* Parsed at line. */
};

/* State of file parse. */
struct	rofftree {
	struct roffnode	 *last;			/* Last parsed node. */
	time_t		  date;			/* `Dd' results. */
	char		  os[64];		/* `Os' results. */
	char		  title[64];		/* `Dt' results. */
	char		  section[64];		/* `Dt' results. */
	char		  volume[64];		/* `Dt' results. */
	int		  state;
#define	ROFF_PRELUDE	 (1 << 1)		/* In roff prelude. */
	/* FIXME: if we had prev ptrs, this wouldn't be necessary. */
#define	ROFF_PRELUDE_Os	 (1 << 2)		/* `Os' is parsed. */
#define	ROFF_PRELUDE_Dt	 (1 << 3)		/* `Dt' is parsed. */
#define	ROFF_PRELUDE_Dd	 (1 << 4)		/* `Dd' is parsed. */
#define	ROFF_BODY	 (1 << 5)		/* In roff body. */
	roffin		 roffin;		/* Text-macro cb. */
	roffblkin	 roffblkin;		/* Block-macro cb. */
	roffout		 roffout;		/* Text-macro cb. */
	roffblkout	 roffblkout;		/* Block-macro cb. */
	struct md_mbuf		*mbuf;		/* Output (or NULL). */
	const struct md_args	*args;		/* Global args. */
	const struct md_rbuf	*rbuf;		/* Input. */
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
static	int		  roffargok(int, int);
static	int		  roffnextopt(int, const char ***, char **);
static	int 		  roffparse(struct rofftree *, char *, size_t);
static	int		  textparse(const struct rofftree *,
				const char *, size_t);

/* Arguments for `An' macro. */
static	const int roffarg_An[] = { 
	ROFF_Split, ROFF_Nosplit, ROFF_ARGMAX };
/* Arguments for `Bd' macro. */
static	const int roffarg_Bd[] = {
	ROFF_Ragged, ROFF_Unfilled, ROFF_Literal, ROFF_File, 
	ROFF_Offset, ROFF_ARGMAX };
/* Arguments for `Bl' macro. */
static 	const int roffarg_Bl[] = {
	ROFF_Bullet, ROFF_Dash, ROFF_Hyphen, ROFF_Item, ROFF_Enum,
	ROFF_Tag, ROFF_Diag, ROFF_Hang, ROFF_Ohang, ROFF_Inset,
	ROFF_Column, ROFF_Offset, ROFF_ARGMAX };

/* FIXME: a big list of fixes that must occur.
 *
 * (1) Distinction not between ROFF_TEXT and ROFF_LAYOUT, but instead
 *     ROFF_ATOM and ROFF_NODE, which designate line spacing.  If
 *     ROFF_ATOM, we need not remember any state.
 *
 * (2) Have a maybe-NULL list of possible subsequent children for each
 *     node.  Bl, e.g., can only have It children (roffparse).
 *
 * (3) Have a maybe-NULL list of possible parents for each node.  It,
 *     e.g., can only have Bl as a parent (roffparse).
 *
 *     (N.B. If (2) were complete, (3) wouldn't be necessary.)
 *
 * (4) Scope rules.  If Pp exists, it closes the scope out from the
 *     previous Pp (if it exists).  Same with Sh and Ss.  If El exists,
 *     it closes out Bl and interim It.
 *
 * (5) Nesting.  Sh cannot be any descendant of Sh.  Bl, however, can be
 *     nested within an It.
 *
 * Once that's done, we're golden.
 */

/* Table of all known tokens. */
static	const struct rofftok tokens[ROFF_MAX] = {
	{        NULL,       NULL, 0, ROFF_COMMENT, 0 },	/* \" */
	{     roff_Dd,       NULL, 0, ROFF_TEXT, 0 },	/* Dd */
	{     roff_Dt,       NULL, 0, ROFF_TEXT, 0 },	/* Dt */
	{     roff_Os,       NULL, 0, ROFF_TEXT, 0 },	/* Os */
	{ roff_layout,       NULL, ROFF_Sh, ROFF_LAYOUT, ROFF_PARSED }, /* Sh */
	{ roff_layout,       NULL, ROFF_Ss, ROFF_LAYOUT, ROFF_PARSED }, /* Ss */ 
	{ roff_layout,       NULL, ROFF_Pp, ROFF_LAYOUT, 0 }, 	/* Pp */
	{ roff_layout,       NULL, 0, ROFF_TEXT, 0 },	 	/* D1 */
	{ roff_layout,       NULL, 0, ROFF_TEXT, 0 }, 		/* Dl */
	{ roff_layout, roffarg_Bd, 0, ROFF_LAYOUT, 0 }, 	/* Bd */
	{ roff_layout,       NULL, ROFF_Bd, ROFF_LAYOUT, 0 }, 	/* Ed */
	{ roff_layout, roffarg_Bl, 0, ROFF_LAYOUT, 0 }, 	/* Bl */
	{ roff_layout,       NULL, ROFF_Bl, ROFF_LAYOUT, 0 }, 	/* El */
	{ roff_layout,       NULL, ROFF_It, ROFF_LAYOUT, 0 }, 	/* It */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ad */
	{   roff_text, roffarg_An, 0, ROFF_TEXT, ROFF_PARSED }, /* An */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ar */
	{   roff_text,       NULL, 0, ROFF_TEXT, 0 },	/* Cd */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Cm */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Dv */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Er */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ev */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ex */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Fa */
	{   roff_text,       NULL, 0, ROFF_TEXT, 0 },	/* Fd */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Fl */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Fn */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ft */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ic */
	{   roff_text,       NULL, 0, ROFF_TEXT, 0 },	/* In */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Li */
	{   roff_text,       NULL, 0, ROFF_TEXT, 0 },	/* Nd */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Nm */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Op */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ot */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Pa */
	{   roff_text,       NULL, 0, ROFF_TEXT, 0 },	/* Rv */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* St */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Va */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Vt */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Xr */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* %A */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE}, /* %B */
	{   roff_text,       NULL, 0, ROFF_TEXT, 0 },	/* %D */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE}, /* %I */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE}, /* %J */
	{   roff_text,       NULL, 0, ROFF_TEXT, 0 },	/* %N */
	{   roff_text,       NULL, 0, ROFF_TEXT, 0 },	/* %O */
	{   roff_text,       NULL, 0, ROFF_TEXT, 0 },	/* %P */
	{   roff_text,       NULL, 0, ROFF_TEXT, 0 },	/* %R */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* %T */
	{   roff_text,       NULL, 0, ROFF_TEXT, 0 },	/* %V */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ac */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ao */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Aq */
	{   roff_text,       NULL, 0, ROFF_TEXT, 0 },	/* At */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Bc */
	{   roff_text,       NULL, 0, ROFF_TEXT, 0 },	/* Bf */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Bo */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Bq */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Bsx */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Bx */
	{   roff_text,       NULL, 0, ROFF_TEXT, 0 },	/* Db */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Dc */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Do */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Dq */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ec */
	{   roff_text,       NULL, 0, ROFF_TEXT, 0 },	/* Ef */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Em */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Eo */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Fx */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Ms */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* No */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ns */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Nx */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Ox */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Pc */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Pf */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Po */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Pq */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Qc */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ql */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Qo */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Qq */
	{   roff_text,       NULL, 0, ROFF_TEXT, 0 },	/* Re */
	{   roff_text,       NULL, 0, ROFF_TEXT, 0 },	/* Rs */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Sc */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* So */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Sq */
	{   roff_text,       NULL, 0, ROFF_TEXT, 0 },	/* Sm */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Sx */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Sy */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Tn */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Ux */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Xc */
	{   roff_text,       NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Xo */
};

/* Table of all known token arguments. */
static	const struct roffarg tokenargs[ROFF_ARGMAX] = {
	{ 0 },						/* split */
	{ 0 },						/* nosplit */
	{ 0 },						/* ragged */
	{ 0 },						/* unfilled */
	{ 0 },						/* literal */
	{ ROFF_VALUE },					/* file */
	{ ROFF_VALUE },					/* offset */
	{ 0 },						/* bullet */
	{ 0 },						/* dash */
	{ 0 },						/* hyphen */
	{ 0 },						/* item */
	{ 0 },						/* enum */
	{ 0 },						/* tag */
	{ 0 },						/* diag */
	{ 0 },						/* hang */
	{ 0 },						/* ohang */
	{ 0 },						/* inset */
	{ 0 },						/* column */
	{ 0 },						/* width */
	{ 0 },						/* compact */
};

const	char *const toknamesp[ROFF_MAX] = 
	{		 
	"\\\"",		 
	"Dd",	/* Title macros. */
	"Dt",		 
	"Os",		 
	"Sh",	/* Layout macros */
	"Ss",		 
	"Pp",		 
	"D1",		 
	"Dl",		 
	"Bd",		 
	"Ed",		 
	"Bl",		 
	"El",		 
	"It",		 
	"Ad",	/* Text macros. */
	"An",		 
	"Ar",		 
	"Cd",		 
	"Cm",		 
	"Dr",		 
	"Er",		 
	"Ev",		 
	"Ex",		 
	"Fa",		 
	"Fd",		 
	"Fl",		 
	"Fn",		 
	"Ft",		 
	"Ex",		 
	"Ic",		 
	"In",		 
	"Li",		 
	"Nd",		 
	"Nm",		 
	"Op",		 
	"Ot",		 
	"Pa",		 
	"Rv",		 
	"St",		 
	"Va",		 
	"Vt",		 
	"Xr",		 
	"\%A",	/* General text macros. */
	"\%B",
	"\%D",
	"\%I",
	"\%J",
	"\%N",
	"\%O",
	"\%P",
	"\%R",
	"\%T",
	"\%V",
	"Ac",
	"Ao",
	"Aq",
	"At",
	"Bc",
	"Bf",
	"Bo",
	"Bq",
	"Bsx",
	"Bx",
	"Db",
	"Dc",
	"Do",
	"Dq",
	"Ec",
	"Ef",
	"Em",
	"Eo",
	"Fx",
	"Ms",
	"No",
	"Ns",
	"Nx",
	"Ox",
	"Pc",
	"Pf",
	"Po",
	"Pq",
	"Qc",
	"Ql",
	"Qo",
	"Qq",
	"Re",
	"Rs",
	"Sc",
	"So",
	"Sq",
	"Sm",
	"Sx",
	"Sy",
	"Tn",
	"Ux",
	"Xc",	/* FIXME: do not support! */
	"Xo",	/* FIXME: do not support! */
	};

const	char *const tokargnamesp[ROFF_ARGMAX] = 
	{		 
	"split",	 
	"nosplit",	 
	"ragged",	 
	"unfilled",	 
	"literal",	 
	"file",		 
	"offset",	 
	"bullet",	 
	"dash",		 
	"hyphen",	 
	"item",		 
	"enum",		 
	"tag",		 
	"diag",		 
	"hang",		 
	"ohang",	 
	"inset",	 
	"column",	 
	"width",	 
	"compact",	 
	};

const	char *const *toknames = toknamesp;
const	char *const *tokargnames = tokargnamesp;


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
	
	/* FIXME: .Bsx is three letters! */
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
roffnextopt(int tok, const char ***in, char **val)
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

	/* FIXME: should we let this slide... ? */

	if (ROFF_ARGMAX == (v = rofffindarg(&arg[1])))
		return(-1);

	/* FIXME: should we let this slide... ? */

	if ( ! roffargok(tok, v))
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
				tree->rbuf->name, 
				toknames[tok], 
				tree->rbuf->line);
		return(0);
	}

	if (ROFF_EXIT == type) {
		roffnode_free(tok, tree);
		return((*tree->roffblkout)(tok));
	} 

	i = 0;

	while (-1 != (c = roffnextopt(tok, &argv, &v))) {
		if (ROFF_ARGMAX == c) {
			warnx("%s: error parsing `%s' args (line %zu)",
					tree->rbuf->name, 
					toknames[tok],
					tree->rbuf->line);
			return(0);
		} else if ( ! roffargok(tok, c)) {
			warnx("%s: arg `%s' not for `%s' (line %zu)",
					tree->rbuf->name, 
					tokargnames[c],
					toknames[tok],
					tree->rbuf->line);
			return(0);
		}
		argcp[i] = c;
		argvp[i] = v;
		i++;
		argv++;
	}

	argcp[i] = ROFF_ARGMAX;
	argvp[i] = NULL;

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
				tree->rbuf->name, 
				toknames[tok],
				tree->rbuf->line);
		return(0);
	}

	i = 0;

	while (-1 != (c = roffnextopt(tok, &argv, &v))) {
		if (ROFF_ARGMAX == c) {
			warnx("%s: error parsing `%s' args (line %zu)",
					tree->rbuf->name, 
					toknames[tok],
					tree->rbuf->line);
			return(0);
		} 
		argcp[i] = c;
		argvp[i] = v;
		i++;
		argv++;
	}

	argcp[i] = ROFF_ARGMAX;
	argvp[i] = NULL;

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
