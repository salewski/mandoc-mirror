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

/* FIXME: warn if Pp occurs before/after Sh etc. (see mdoc.samples). */

/* FIXME: warn about "X section only" macros. */

/* FIXME: warn about empty lists. */

/* FIXME: ; : } ) (etc.) after text macros? */

#define	ROFF_MAXARG	  10

enum	roffd { 
	ROFF_ENTER = 0, 
	ROFF_EXIT 
};

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
	int		(*cb)(ROFFCALL_ARGS);	/* Callback. */
	const int	 *args;			/* Args (or NULL). */
	const int	 *parents;
	const int	 *children;
	int		  ctx;
	enum rofftype	  type;			/* Type of macro. */
	int		  flags;
#define	ROFF_PARSED	 (1 << 0)		/* "Parsed". */
#define	ROFF_CALLABLE	 (1 << 1)		/* "Callable". */
#define	ROFF_QUOTES	 (1 << 2)		/* Quoted args. */
};

struct	roffarg {
	int		  flags;
#define	ROFF_VALUE	 (1 << 0)		/* Has a value. */
};

struct	roffnode {
	int		  tok;			/* Token id. */
	struct roffnode	 *parent;		/* Parent (or NULL). */
	size_t		  line;			/* Parsed at line. */
};

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
	struct md_mbuf		*mbuf;		/* Output (or NULL). */
	const struct md_args	*args;		/* Global args. */
	const struct md_rbuf	*rbuf;		/* Input. */
	const struct roffcb	*cb;
};

static	int		  roff_Dd(ROFFCALL_ARGS);
static	int		  roff_Dt(ROFFCALL_ARGS);
static	int		  roff_Os(ROFFCALL_ARGS);

static	int		  roff_layout(ROFFCALL_ARGS);
static	int		  roff_text(ROFFCALL_ARGS);
static	int		  roff_comment(ROFFCALL_ARGS);

static	struct roffnode	 *roffnode_new(int, struct rofftree *);
static	void		  roffnode_free(int, struct rofftree *);

static	int		  roffscan(int, const int *);
static	int		  rofffindtok(const char *);
static	int		  rofffindarg(const char *);
static	int		  rofffindcallable(const char *);
static	int		  roffargs(int, char *, char **);
static	int		  roffargok(int, int);
static	int		  roffnextopt(int, const char ***, char **);
static	int 		  roffparse(struct rofftree *, char *, size_t);
static	int		  textparse(const struct rofftree *,
				const char *, size_t);


static	const int roffarg_An[] = { ROFF_Split, ROFF_Nosplit, 
	ROFF_ARGMAX };
static	const int roffarg_Bd[] = { ROFF_Ragged, ROFF_Unfilled, 
	ROFF_Literal, ROFF_File, ROFF_Offset, ROFF_ARGMAX };
static	const int roffarg_Ex[] = { ROFF_Std, ROFF_ARGMAX };
static	const int roffarg_Rv[] = { ROFF_Std, ROFF_ARGMAX };
static 	const int roffarg_Bl[] = { ROFF_Bullet, ROFF_Dash, 
	ROFF_Hyphen, ROFF_Item, ROFF_Enum, ROFF_Tag, ROFF_Diag, 
	ROFF_Hang, ROFF_Ohang, ROFF_Inset, ROFF_Column, ROFF_Offset, 
	ROFF_ARGMAX };
static 	const int roffarg_St[] = {
	ROFF_p1003_1_88, ROFF_p1003_1_90, ROFF_p1003_1_96,
	ROFF_p1003_1_2001, ROFF_p1003_1_2004, ROFF_p1003_1,
	ROFF_p1003_1b, ROFF_p1003_1b_93, ROFF_p1003_1c_95,
	ROFF_p1003_1g_2000, ROFF_p1003_2_92, ROFF_p1387_2_95,
	ROFF_p1003_2, ROFF_p1387_2, ROFF_isoC_90, ROFF_isoC_amd1,
	ROFF_isoC_tcor1, ROFF_isoC_tcor2, ROFF_isoC_99, ROFF_ansiC,
	ROFF_ansiC_89, ROFF_ansiC_99, ROFF_ieee754, ROFF_iso8802_3,
	ROFF_xpg3, ROFF_xpg4, ROFF_xpg4_2, ROFF_xpg4_3, ROFF_xbd5,
	ROFF_xcu5, ROFF_xsh5, ROFF_xns5, ROFF_xns5_2d2_0,
	ROFF_xcurses4_2, ROFF_susv2, ROFF_susv3, ROFF_svid4,
	ROFF_ARGMAX };

static	const int roffchild_Bd[] = { ROFF_Ed, ROFF_MAX };
static	const int roffchild_Bl[] = { ROFF_It, ROFF_El, ROFF_MAX };
static	const int roffchild_Fo[] = { ROFF_Fa, ROFF_Fc, ROFF_MAX };
static	const int roffchild_Oo[] = { ROFF_Op, ROFF_Oc, ROFF_MAX };

static	const int roffparent_Ed[] = { ROFF_Bd, ROFF_MAX };
static	const int roffparent_El[] = { ROFF_Bl, ROFF_It, ROFF_MAX };
static	const int roffparent_Fc[] = { ROFF_Fo, ROFF_Fa, ROFF_MAX };
static	const int roffparent_Oc[] = { ROFF_Oo, ROFF_Oc, ROFF_MAX };
static	const int roffparent_It[] = { ROFF_Bl, ROFF_It, ROFF_MAX };

/* Table of all known tokens. */
static	const struct rofftok tokens[ROFF_MAX] = {
	{roff_comment, NULL, NULL, NULL, 0, ROFF_COMMENT, 0 },	/* \" */
	{     roff_Dd, NULL, NULL, NULL, 0, ROFF_TEXT, 0 },	/* Dd */
	{     roff_Dt, NULL, NULL, NULL, 0, ROFF_TEXT, 0 },	/* Dt */
	{     roff_Os, NULL, NULL, NULL, 0, ROFF_TEXT, 0 },	/* Os */
	{ roff_layout, NULL, NULL, NULL, ROFF_Sh, ROFF_LAYOUT, ROFF_PARSED }, /* Sh */
	{ roff_layout, NULL, NULL, NULL, ROFF_Ss, ROFF_LAYOUT, ROFF_PARSED }, /* Ss */ 
	{   roff_text, NULL, NULL, NULL, ROFF_Pp, ROFF_TEXT, 0 }, /* Pp */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* D1 */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Dl */
	{ roff_layout, NULL, NULL, roffchild_Bd, 0, ROFF_LAYOUT, 0 }, 	/* Bd */
	{ roff_layout, NULL, roffparent_Ed, NULL, ROFF_Bd, ROFF_LAYOUT, 0 }, 	/* Ed */
	{ roff_layout, roffarg_Bl, NULL, roffchild_Bl, 0, ROFF_LAYOUT, 0 }, /* Bl */
	{ roff_layout, NULL, roffparent_El, NULL, ROFF_Bl, ROFF_LAYOUT, 0 }, /* El */
	{ roff_layout, NULL, roffparent_It, NULL, ROFF_It, ROFF_LAYOUT, 0 }, /* It */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ad */
	{   roff_text, roffarg_An, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* An */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ar */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_QUOTES }, /* Cd */ /* XXX man.4 only */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Cm */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Dv */ /* XXX needs arg */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Er */ /* XXX needs arg */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ev */ /* XXX needs arg */
	{   roff_text, roffarg_Ex, NULL, NULL, 0, ROFF_TEXT, 0 }, /* Ex */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Fa */ /* XXX needs arg */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, 0 },	/* Fd */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Fl */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Fn */ /* XXX needs arg */ /* FIXME */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Ft */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ic */ /* XXX needs arg */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, 0 },	/* In */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Li */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, 0 },	/* Nd */ /* FIXME */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Nm */ /* FIXME */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Op */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* Ot */ /* XXX deprecated */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Pa */
	{   roff_text, roffarg_Rv, NULL, NULL, 0, ROFF_TEXT, 0 },	/* Rv */
	{   roff_text, roffarg_St, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* St */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Va */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Vt */ /* XXX needs arg */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Xr */ /* XXX needs arg */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* %A */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE}, /* %B */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, 0 },	/* %D */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE}, /* %I */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE}, /* %J */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, 0 },	/* %N */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, 0 },	/* %O */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, 0 },	/* %P */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, 0 },	/* %R */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* %T */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, 0 },	/* %V */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ac */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ao */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Aq */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, 0 },	/* At */ /* XXX at most 2 args */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Bc */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, 0 },	/* Bf */ /* FIXME */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Bo */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Bq */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Bsx */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Bx */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, 0 },	/* Db */ /* XXX */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Dc */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Do */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Dq */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ec */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, 0 },	/* Ef */ /* FIXME */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Em */ /* XXX needs arg */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Eo */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Fx */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Ms */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* No */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ns */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Nx */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Ox */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Pc */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Pf */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Po */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Pq */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Qc */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ql */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Qo */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Qq */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, 0 },	/* Re */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, 0 },	/* Rs */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Sc */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* So */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Sq */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, 0 },	/* Sm */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Sx */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE | ROFF_QUOTES }, /* Sy */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Tn */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Ux */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Xc */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Xo */
	{ roff_layout, NULL, NULL, NULL, 0, ROFF_LAYOUT, 0 }, /* Fo */
	{ roff_layout, NULL, roffparent_Fc, NULL, ROFF_Fo, ROFF_LAYOUT, 0 }, /* Fc */
	{ roff_layout, NULL, NULL, NULL, 0, ROFF_LAYOUT, 0 }, /* Oo */
	{ roff_layout, NULL, roffparent_Oc, NULL, ROFF_Oo, ROFF_LAYOUT, 0 }, /* Oc */
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
	"\\\"",		"Dd",		"Dt",		"Os",
	"Sh",		"Ss",		"Pp",		"D1",
	"Dl",		"Bd",		"Ed",		"Bl",
	"El",		"It",		"Ad",		"An",
	"Ar",		"Cd",		"Cm",		"Dv",
	"Er",		"Ev",		"Ex",		"Fa",
	"Fd",		"Fl",		"Fn",		"Ft",
	"Ic",		"In",		"Li",		"Nd",
	"Nm",		"Op",		"Ot",		"Pa",
	"Rv",		"St",		"Va",		"Vt",
	"Xr",		"\%A",		"\%B",		"\%D",
	"\%I",		"\%J",		"\%N",		"\%O",
	"\%P",		"\%R",		"\%T",		"\%V",
	"Ac",		"Ao",		"Aq",		"At",
	"Bc",		"Bf",		"Bo",		"Bq",
	"Bsx",		"Bx",		"Db",		"Dc",
	"Do",		"Dq",		"Ec",		"Ef",
	"Em",		"Eo",		"Fx",		"Ms",
	"No",		"Ns",		"Nx",		"Ox",
	"Pc",		"Pf",		"Po",		"Pq",
	"Qc",		"Ql",		"Qo",		"Qq",
	"Re",		"Rs",		"Sc",		"So",
	"Sq",		"Sm",		"Sx",		"Sy",
	"Tn",		"Ux",		"Xc",		"Xo",
	"Fo",		"Fc",		"Oo",		"Oc"
	};

const	char *const tokargnamesp[ROFF_ARGMAX] = 
	{		 
	"split",	"nosplit",	"ragged",	 
	"unfilled",	"literal",	"file",		 
	"offset",	"bullet",	"dash",		 
	"hyphen",	"item",		"enum",		 
	"tag",		"diag",		"hang",		 
	"ohang",	"inset",	"column",	 
	"width",	"compact",	"std",	 
	"-p1003.1-88",	"-p1003.1-90",	"-p1003.1-96",
	"-p1003.1-2001", "-p1003.1-2004", "-p1003.1",
	"-p1003.1b",	"-p1003.1b-93",	"-p1003.1c-95",
	"-p1003.1g-2000", "-p1003.2-92", "-p1387.2-95",
	"-p1003.2",	"-p1387.2",	"-isoC-90",
	"-isoC-amd1",	"-isoC-tcor1",	"-isoC-tcor2",
	"-isoC-99",	"-ansiC",	"-ansiC-89",
	"-ansiC-99",	"-ieee754",	"-iso8802-3",
	"-xpg3",	"-xpg4",	"-xpg4.2",
	"-xpg4.3",	"-xbd5",	"-xcu5",
	"-xsh5",	"-xns5",	"-xns5.2d2.0",
	"-xcurses4.2",	"-susv2",	"-susv3",
	"-svid4"
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
		const struct md_rbuf *in, const struct roffcb *cb)
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
	tree->cb = cb;

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


/* XXX */
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
roffparse(struct rofftree *tree, char *buf, size_t sz)
{
	int		  tok, t;
	struct roffnode	 *n;
	char		 *argv[ROFF_MAXARG];
	const char	**argvp;

	assert(sz > 0);

	if (ROFF_MAX == (tok = rofffindtok(buf + 1))) {
		warnx("%s: unknown line macro (line %zu)",
				tree->rbuf->name, tree->rbuf->line);
		return(0);
	} else if (NULL == tokens[tok].cb) {
		warnx("%s: macro `%s' not supported (line %zu)",
				tree->rbuf->name, toknames[tok],
				tree->rbuf->line);
		return(0);
	} else if (ROFF_COMMENT == tokens[tok].type)
		return(1);
	
	if ( ! roffargs(tok, buf, argv)) {
		warnx("%s: too many args to `%s' (line %zu)",
				tree->rbuf->name, toknames[tok], 
				tree->rbuf->line);
		return(0);
	} else
		argvp = (const char **)argv + 1;

	/* 
	 * Prelude macros break some assumptions: branch now. 
	 */
	
	if (ROFF_PRELUDE & tree->state) {
		assert(NULL == tree->last);
		return((*tokens[tok].cb)(tok, tree, argvp, ROFF_ENTER));
	} else 
		assert(tree->last);

	assert(ROFF_BODY & tree->state);

	/* 
	 * First check that our possible parents and parent's possible
	 * children are satisfied.  
	 */

	if ( ! roffscan(tree->last->tok, tokens[tok].parents)) {
		warnx("%s: invalid parent `%s' for `%s' (line %zu)",
				tree->rbuf->name, 
				toknames[tree->last->tok],
				toknames[tok], tree->rbuf->line);
		return(0);
	} 

	if ( ! roffscan(tok, tokens[tree->last->tok].children)) {
		warnx("%s: invalid child `%s' for `%s' (line %zu)",
				tree->rbuf->name, toknames[tok], 
				toknames[tree->last->tok],
				tree->rbuf->line);
		return(0);
	}

	/*
	 * Branch if we're not a layout token.
	 */

	if (ROFF_LAYOUT != tokens[tok].type)
		return((*tokens[tok].cb)(tok, tree, argvp, ROFF_ENTER));

	/* 
	 * Check our scope rules. 
	 */

	if (0 == tokens[tok].ctx)
		return((*tokens[tok].cb)(tok, tree, argvp, ROFF_ENTER));

	if (tok == tokens[tok].ctx) {
		for (n = tree->last; n; n = n->parent) {
			assert(0 == tokens[n->tok].ctx ||
					n->tok == tokens[n->tok].ctx);
			if (n->tok == tok)
				break;
		}
		if (NULL == n) {
#ifdef DEBUG
			(void)printf("scope: new `%s'\n",
					toknames[tok]);
#endif
			return((*tokens[tok].cb)(tok, tree, argvp, ROFF_ENTER));
		}
		do {
			t = tree->last->tok;
#ifdef DEBUG
		(void)printf("scope: closing `%s'\n", 
				toknames[t]);
#endif
			if ( ! (*tokens[t].cb)(t, tree, NULL, ROFF_EXIT))
				return(0);
		} while (t != tok);

		return((*tokens[tok].cb)(tok, tree, argvp, ROFF_ENTER));
	}

	assert(tok != tokens[tok].ctx && 0 != tokens[tok].ctx);

	do {
		t = tree->last->tok;
#ifdef DEBUG
		(void)printf("scope: closing `%s'\n", toknames[t]);
#endif
		if ( ! (*tokens[t].cb)(t, tree, NULL, ROFF_EXIT))
			return(0);
	} while (t != tokens[tok].ctx);

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
rofffindtok(const char *buf)
{
	char		 token[4];
	size_t		 i;

	for (i = 0; *buf && ! isspace(*buf) && i < 3; i++, buf++)
		token[i] = *buf;

	if (i == 3) {
#ifdef DEBUG
		(void)printf("lookup: macro too long: `%s'\n", buf);
#endif
		return(ROFF_MAX);
	}

	token[i] = 0;

	/* FIXME: use a table, this is slow but ok for now. */

	/* LINTED */
	for (i = 0; i < ROFF_MAX; i++)
		/* LINTED */
		if (0 == strcmp(toknames[i], token)) {
#ifdef DEBUG
			(void)printf("lookup (good): `%s' (%d)\n", 
					token, (int)i);
#endif
			return((int)i);
		}

#ifdef DEBUG
	(void)printf("lookup (bad): `%s'\n", token);
#endif
	
	return(ROFF_MAX);
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

	if (ROFF_PRELUDE_Dd & tree->state) {
		warnx("%s: prelude `Dd' repeated (line %zu)",
				tree->rbuf->name, tree->rbuf->line);
		return(0);
	} else if (ROFF_PRELUDE_Dt & tree->state) {
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

	if ( ! (ROFF_PRELUDE_Dd & tree->state)) {
		warnx("%s: prelude `Dt' out-of-order (line %zu)",
				tree->rbuf->name, tree->rbuf->line);
		return(0);
	} else if (ROFF_PRELUDE_Dt & tree->state) {
		warnx("%s: prelude `Dt' repeated (line %zu)",
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
		return((*tree->cb->roffblkout)(tok));
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

	if ( ! (*tree->cb->roffin)(tok, argcp, argvp))
		return(0);

	if ( ! (ROFF_PARSED & tokens[tok].flags)) {
		/* TODO: print all tokens. */

		if ( ! ((*tree->cb->roffout)(tok)))
			return(0);
		return((*tree->cb->roffblkin)(tok));
	}

	while (*argv) {
		if (ROFF_MAX != (c = rofffindcallable(*argv))) {
			if (NULL == tokens[c].cb) {
				warnx("%s: macro `%s' not supported "
						"(line %zu)",
						tree->rbuf->name, 
						toknames[c],
						tree->rbuf->line);
				return(0);
			}
			if ( ! (*tokens[c].cb)(c, tree, 
						argv + 1, ROFF_ENTER))
				return(0);
		}
		/* TODO: print token. */
		argv++;
	}

	if ( ! ((*tree->cb->roffout)(tok)))
		return(0);

	return((*tree->cb->roffblkin)(tok));
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

	if ( ! (*tree->cb->roffin)(tok, argcp, argvp))
		return(0);

	if ( ! (ROFF_PARSED & tokens[tok].flags)) {
		/* TODO: print all tokens. */
		return((*tree->cb->roffout)(tok));
	}

	while (*argv) {
		if (ROFF_MAX != (c = rofffindcallable(*argv))) {
			if (NULL == tokens[c].cb) {
				warnx("%s: macro `%s' not supported "
						"(line %zu)",
						tree->rbuf->name, 
						toknames[c],
						tree->rbuf->line);
				return(0);
			}
			if ( ! (*tokens[c].cb)(c, tree, 
						argv + 1, ROFF_ENTER))
				return(0);
		}
		/* TODO: print token. */
		argv++;
	}

	return((*tree->cb->roffout)(tok));
}


/* ARGUSED */
static int
roff_comment(ROFFCALL_ARGS)
{

	return(1);
}
