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
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "libmdocml.h"
#include "private.h"

/* FIXME: warn if Pp occurs before/after Sh etc. (see mdoc.samples). */

/* FIXME: warn about "X section only" macros. */

/* FIXME: warn about empty lists. */

/* FIXME: roff_layout and roff_text have identical-ish lower bodies. */

/* FIXME: NAME section needs specific elements. */

#define	ROFF_MAXARG	  32

enum	roffd { 
	ROFF_ENTER = 0, 
	ROFF_EXIT 
};

enum	rofftype { 
	ROFF_COMMENT, 
	ROFF_TEXT, 
	ROFF_LAYOUT,
	ROFF_SPECIAL
};

#define	ROFFCALL_ARGS \
	int tok, struct rofftree *tree, \
	char *argv[], enum roffd type

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
#define	ROFF_SHALLOW	 (1 << 2)		/* Nesting block. */
};

struct	roffarg {
	int		  flags;
#define	ROFF_VALUE	 (1 << 0)		/* Has a value. */
};

struct	roffnode {
	int		  tok;			/* Token id. */
	struct roffnode	 *parent;		/* Parent (or NULL). */
};

struct	rofftree {
	struct roffnode	 *last;			/* Last parsed node. */
	char		 *cur;

	time_t		  date;			/* `Dd' results. */
	char		  os[64];		/* `Os' results. */
	char		  title[64];		/* `Dt' results. */
	char		  section[64];		/* `Dt' results. */
	char		  volume[64];		/* `Dt' results. */

	int		  state;
#define	ROFF_PRELUDE	 (1 << 1)		/* In roff prelude. */
#define	ROFF_PRELUDE_Os	 (1 << 2)		/* `Os' is parsed. */
#define	ROFF_PRELUDE_Dt	 (1 << 3)		/* `Dt' is parsed. */
#define	ROFF_PRELUDE_Dd	 (1 << 4)		/* `Dd' is parsed. */
#define	ROFF_BODY	 (1 << 5)		/* In roff body. */

	struct roffcb	  cb;
	void		 *arg;
};

static	int		  roff_Dd(ROFFCALL_ARGS);
static	int		  roff_Dt(ROFFCALL_ARGS);
static	int		  roff_Os(ROFFCALL_ARGS);

static	int		  roff_layout(ROFFCALL_ARGS);
static	int		  roff_text(ROFFCALL_ARGS);
static	int		  roff_comment(ROFFCALL_ARGS);
static	int		  roff_close(ROFFCALL_ARGS);
static	int		  roff_special(ROFFCALL_ARGS);

static	struct roffnode	 *roffnode_new(int, struct rofftree *);
static	void		  roffnode_free(struct rofftree *);

static	void		  roff_warn(const struct rofftree *, 
				const char *, char *, ...);
static	void		  roff_err(const struct rofftree *, 
				const char *, char *, ...);

static	int		  roffscan(int, const int *);
static	int		  rofffindtok(const char *);
static	int		  rofffindarg(const char *);
static	int		  rofffindcallable(const char *);
static	int		  roffargs(const struct rofftree *,
				int, char *, char **);
static	int		  roffargok(int, int);
static	int		  roffnextopt(const struct rofftree *,
				int, char ***, char **);
static	int 		  roffparse(struct rofftree *, char *);
static	int		  textparse(const struct rofftree *, char *);


static	const int roffarg_An[] = { ROFF_Split, ROFF_Nosplit, 
	ROFF_ARGMAX };
static	const int roffarg_Bd[] = { ROFF_Ragged, ROFF_Unfilled, 
	ROFF_Literal, ROFF_File, ROFF_Offset, ROFF_Filled,
	ROFF_Compact, ROFF_ARGMAX };
static	const int roffarg_Bk[] = { ROFF_Words, ROFF_ARGMAX };
static	const int roffarg_Ex[] = { ROFF_Std, ROFF_ARGMAX };
static	const int roffarg_Rv[] = { ROFF_Std, ROFF_ARGMAX };
static 	const int roffarg_Bl[] = { ROFF_Bullet, ROFF_Dash, 
	ROFF_Hyphen, ROFF_Item, ROFF_Enum, ROFF_Tag, ROFF_Diag, 
	ROFF_Hang, ROFF_Ohang, ROFF_Inset, ROFF_Column, ROFF_Offset, 
	ROFF_Width, ROFF_Compact, ROFF_ARGMAX };
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

static	const int roffchild_Bl[] = { ROFF_It, ROFF_El, ROFF_MAX };
static	const int roffchild_Fo[] = { ROFF_Fa, ROFF_Fc, ROFF_MAX };
static	const int roffchild_Oo[] = { ROFF_Op, ROFF_Oc, ROFF_MAX };
static	const int roffchild_Rs[] = { ROFF_Re, ROFF__A, ROFF__B,
	ROFF__D, ROFF__I, ROFF__J, ROFF__N, ROFF__O, ROFF__P,
	ROFF__R, ROFF__T, ROFF__V, ROFF_MAX };

static	const int roffparent_El[] = { ROFF_Bl, ROFF_It, ROFF_MAX };
static	const int roffparent_Fc[] = { ROFF_Fo, ROFF_Fa, ROFF_MAX };
static	const int roffparent_Oc[] = { ROFF_Oo, ROFF_Oc, ROFF_MAX };
static	const int roffparent_It[] = { ROFF_Bl, ROFF_It, ROFF_MAX };
static	const int roffparent_Re[] = { ROFF_Rs, ROFF_MAX };

/* Table of all known tokens. */
static	const struct rofftok tokens[ROFF_MAX] = {
	{roff_comment, NULL, NULL, NULL, 0, ROFF_COMMENT, 0 }, /* \" */
	{     roff_Dd, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* Dd */
	{     roff_Dt, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* Dt */
	{     roff_Os, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* Os */
	{ roff_layout, NULL, NULL, NULL, ROFF_Sh, ROFF_LAYOUT, 0 }, /* Sh */
	{ roff_layout, NULL, NULL, NULL, ROFF_Ss, ROFF_LAYOUT, 0 }, /* Ss */ 
	{   roff_text, NULL, NULL, NULL, ROFF_Pp, ROFF_TEXT, 0 }, /* Pp */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* D1 */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Dl */
	{ roff_layout, roffarg_Bd, NULL, NULL, 0, ROFF_LAYOUT, 0 }, 	/* Bd */
	{  roff_close, NULL, NULL, NULL, ROFF_Bd, ROFF_LAYOUT, 0 }, /* Ed */
	{ roff_layout, roffarg_Bl, NULL, roffchild_Bl, 0, ROFF_LAYOUT, 0 }, /* Bl */
	{  roff_close, NULL, roffparent_El, NULL, ROFF_Bl, ROFF_LAYOUT, 0 }, /* El */
	{ roff_layout, NULL, roffparent_It, NULL, ROFF_It, ROFF_LAYOUT, ROFF_PARSED | ROFF_SHALLOW }, /* It */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ad */ /* FIXME */
	{   roff_text, roffarg_An, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* An */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ar */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* Cd */ /* XXX man.4 only */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Cm */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Dv */ /* XXX needs arg */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Er */ /* XXX needs arg */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ev */ /* XXX needs arg */
	{   roff_text, roffarg_Ex, NULL, NULL, 0, ROFF_TEXT, 0 }, /* Ex */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Fa */ /* XXX needs arg */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* Fd */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Fl */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Fn */ /* XXX needs arg */ /* FIXME */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Ft */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ic */ /* XXX needs arg */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* In */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Li */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* Nd */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Nm */ /* FIXME */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Op */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* Ot */ /* XXX deprecated */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Pa */
	{   roff_text, roffarg_Rv, NULL, NULL, 0, ROFF_TEXT, 0 }, /* Rv */
	{   roff_text, roffarg_St, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* St */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Va */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Vt */ /* XXX needs arg */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Xr */ /* XXX needs arg */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* %A */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE}, /* %B */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* %D */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE}, /* %I */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE}, /* %J */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* %N */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* %O */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* %P */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* %R */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* %T */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* %V */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ac */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ao */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Aq */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* At */ /* XXX at most 2 args */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Bc */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, 0 },	/* Bf */ /* FIXME */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Bo */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Bq */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Bsx */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Bx */
	{roff_special, NULL, NULL, NULL, 0, ROFF_SPECIAL, 0 },	/* Db */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Dc */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Do */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Dq */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ec */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, 0 },	/* Ef */ /* FIXME */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Em */ /* XXX needs arg */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Eo */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Fx */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Ms */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* No */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ns */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Nx */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Ox */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Pc */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Pf */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_LAYOUT, ROFF_PARSED | ROFF_CALLABLE }, /* Po */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Pq */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Qc */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ql */
	{ roff_layout, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Qo */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Qq */
	{  roff_close, NULL, roffparent_Re, NULL, ROFF_Rs, ROFF_LAYOUT, 0 }, /* Re */
	{ roff_layout, NULL, NULL, roffchild_Rs, 0, ROFF_LAYOUT, 0 },	/* Rs */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Sc */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* So */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Sq */
	{roff_special, NULL, NULL, NULL, 0, ROFF_SPECIAL, 0 }, /* Sm */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Sx */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Sy */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Tn */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Ux */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Xc */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Xo */
	{ roff_layout, NULL, NULL, roffchild_Fo, 0, ROFF_LAYOUT, 0 }, /* Fo */
	{  roff_close, NULL, roffparent_Fc, NULL, ROFF_Fo, ROFF_LAYOUT, 0 }, /* Fc */
	{ roff_layout, NULL, NULL, roffchild_Oo, 0, ROFF_LAYOUT, 0 }, /* Oo */
	{  roff_close, NULL, roffparent_Oc, NULL, ROFF_Oo, ROFF_LAYOUT, 0 }, /* Oc */
	{ roff_layout, roffarg_Bk, NULL, NULL, 0, ROFF_LAYOUT, 0 }, /* Bk */
	{  roff_close, NULL, NULL, NULL, ROFF_Bk, ROFF_LAYOUT, 0 }, /* Ek */
	};

/* Table of all known token arguments. */
static	const int tokenargs[ROFF_ARGMAX] = {
	0,		0,		0,		0,
	0,		ROFF_VALUE,	ROFF_VALUE,	0,
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		ROFF_VALUE,	0,
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		0,		0,
	};

const	char *const toknamesp[ROFF_MAX] = {		 
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
	/* LINTED */
	"Xr",		"\%A",		"\%B",		"\%D",
	/* LINTED */
	"\%I",		"\%J",		"\%N",		"\%O",
	/* LINTED */
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
	"Fo",		"Fc",		"Oo",		"Oc",
	"Bk",		"Ek",
	};

const	char *const tokargnamesp[ROFF_ARGMAX] = {		 
	"split",		"nosplit",		"ragged",
	"unfilled",		"literal",		"file",		 
	"offset",		"bullet",		"dash",		 
	"hyphen",		"item",			"enum",		 
	"tag",			"diag",			"hang",		 
	"ohang",		"inset",		"column",	 
	"width",		"compact",		"std",	 
	"p1003.1-88",		"p1003.1-90",		"p1003.1-96",
	"p1003.1-2001",		"p1003.1-2004",		"p1003.1",
	"p1003.1b",		"p1003.1b-93",		"p1003.1c-95",
	"p1003.1g-2000",	"p1003.2-92",		"p1387.2-95",
	"p1003.2",		"p1387.2",		"isoC-90",
	"isoC-amd1",		"isoC-tcor1",		"isoC-tcor2",
	"isoC-99",		"ansiC",		"ansiC-89",
	"ansiC-99",		"ieee754",		"iso8802-3",
	"xpg3",			"xpg4",			"xpg4.2",
	"xpg4.3",		"xbd5",			"xcu5",
	"xsh5",			"xns5",			"xns5.2d2.0",
	"xcurses4.2",		"susv2",		"susv3",
	"svid4",		"filled",		"words",
	};

const	char *const *toknames = toknamesp;
const	char *const *tokargnames = tokargnamesp;


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
		roff_warn(tree, NULL, "prelude never finished");
		goto end;
	} 

	for (n = tree->last; n->parent; n = n->parent) {
		if (0 != tokens[n->tok].ctx) 
			continue;
		roff_warn(tree, NULL, "closing explicit scope `%s'", 
				toknames[n->tok]);
		goto end;
	}

	while (tree->last) {
		t = tree->last->tok;
		if ( ! (*tokens[t].cb)(t, tree, NULL, ROFF_EXIT))
			goto end;
	}

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

	(void)memcpy(&tree->cb, cb, sizeof(struct roffcb));

	return(tree);
}


int
roff_engine(struct rofftree *tree, char *buf)
{

	tree->cur = buf;
	assert(buf);

	if (0 == *buf) {
		roff_warn(tree, buf, "blank line");
		return(0);
	} else if ('.' != *buf)
		return(textparse(tree, buf));

	return(roffparse(tree, buf));
}


static int
textparse(const struct rofftree *tree, char *buf)
{

	return((*tree->cb.roffdata)(tree->arg, 1, buf));
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

	/* LINTED */
	for (i = 0; *buf && i < ROFF_MAXARG; i++) {
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
			while (*buf && ! isspace(*buf))
				buf++;
			if (0 == *buf)
				continue;
		}
		*buf++ = 0;
		while (*buf && isspace(*buf))
			buf++;
	}
	
	assert(i > 0);
	if (ROFF_MAXARG == i && *buf) {
		roff_err(tree, p, "too many arguments for `%s'", toknames
				[tok]);
		return(0);
	}

	argv[i] = NULL;
	return(1);
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
roffparse(struct rofftree *tree, char *buf)
{
	int		  tok, t;
	struct roffnode	 *n;
	char		 *argv[ROFF_MAXARG];
	char		**argvp;

	if (ROFF_MAX == (tok = rofffindtok(buf + 1))) {
		roff_err(tree, buf + 1, "bogus line macro");
		return(0);
	} else if (NULL == tokens[tok].cb) {
		roff_err(tree, buf + 1, "unsupported macro `%s'", 
				toknames[tok]);
		return(0);
	} else if (ROFF_COMMENT == tokens[tok].type)
		return(1);
	
	if ( ! roffargs(tree, tok, buf, argv)) 
		return(0);

	argvp = (char **)argv;

	/* 
	 * Prelude macros break some assumptions, so branch now. 
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
		roff_err(tree, *argvp, "`%s' has invalid parent `%s'",
				toknames[tok], 
				toknames[tree->last->tok]);
		return(0);
	} 

	if ( ! roffscan(tok, tokens[tree->last->tok].children)) {
		roff_err(tree, *argvp, "`%s' is invalid child of `%s'",
				toknames[tok],
				toknames[tree->last->tok]);
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
		}

		/*
		 * Create a new scope, as no previous one exists to
		 * close out.
		 */

		if (NULL == n)
			return((*tokens[tok].cb)(tok, tree, argvp, ROFF_ENTER));

		/* 
		 * Close out all intermediary scoped blocks, then hang
		 * the current scope from our predecessor's parent.
		 */

		do {
			t = tree->last->tok;
			if ( ! (*tokens[t].cb)(t, tree, NULL, ROFF_EXIT))
				return(0);
		} while (t != tok);

		return((*tokens[tok].cb)(tok, tree, argvp, ROFF_ENTER));
	}

	/*
	 * Now consider explicit-end tags, where we want to close back
	 * to a specific tag.  Example:
	 * 	.Bl
	 * 	.It Item.
	 * 	.El
	 * In this, the `El' tag closes out the scope of `Bl'.
	 */

	assert(tree->last);
	assert(tok != tokens[tok].ctx && 0 != tokens[tok].ctx);

	/* LINTED */
	do {
		t = tree->last->tok;
		if ( ! (*tokens[t].cb)(t, tree, NULL, ROFF_EXIT))
			return(0);
	} while (t != tokens[tok].ctx);

	assert(tree->last);
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
				"`%s'", &arg[1], toknames[tok]);
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
		roff_err(tree, *argv, "repeated `Dd' in prelude");
		return(0);
	} else if (ROFF_PRELUDE_Dt & tree->state) {
		roff_err(tree, *argv, "out-of-order `Dd' in prelude");
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
		roff_err(tree, *argv, "out-of-order `Dt' in prelude");
		return(0);
	} else if (ROFF_PRELUDE_Dt & tree->state) {
		roff_err(tree, *argv, "repeated `Dt' in prelude");
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
		roffnode_free(tree);
		return((*tree->cb.rofftail)(tree->arg));
	} else if (ROFF_BODY & tree->state) {
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

	/* TODO: extract OS. */

	tree->state |= ROFF_PRELUDE_Os;
	tree->state &= ~ROFF_PRELUDE;
	tree->state |= ROFF_BODY;

	assert(NULL == tree->last);

	if (NULL == roffnode_new(tok, tree))
		return(0);

	return((*tree->cb.roffhead)(tree->arg));
}


/* ARGSUSED */
static int
roff_layout(ROFFCALL_ARGS) 
{
	int		 i, c, argcp[ROFF_MAXARG];
	char		*v, *argvp[ROFF_MAXARG];

	if (ROFF_PRELUDE & tree->state) {
		roff_err(tree, *argv, "`%s' disallowed in prelude", 
				toknames[tok]);
		return(0);
	}

	if (ROFF_EXIT == type) {
		roffnode_free(tree);
		return((*tree->cb.roffblkout)(tree->arg, tok));
	} 

	i = 0;
	argv++;

	while (-1 != (c = roffnextopt(tree, tok, &argv, &v))) {
		if (ROFF_ARGMAX == c)
			return(0);

		argcp[i] = c;
		argvp[i] = v;
		i++;
		argv++;
	}

	argcp[i] = ROFF_ARGMAX;
	argvp[i] = NULL;

	if (NULL == roffnode_new(tok, tree))
		return(0);

	if ( ! (*tree->cb.roffblkin)(tree->arg, tok, argcp, argvp))
		return(0);

	if (NULL == *argv)
		return(1);

	if ( ! (*tree->cb.roffin)(tree->arg, tok, 0, argcp, argvp))
		return(0);

	if ( ! (ROFF_PARSED & tokens[tok].flags)) {
		i = 0;
		while (*argv) {
			if ( ! (*tree->cb.roffdata)(tree->arg, i, *argv++))
				return(0);
			i = 1;
		}
		return((*tree->cb.roffout)(tree->arg, tok));
	}

	i = 0;
	while (*argv) {
		if (ROFF_MAX != (c = rofffindcallable(*argv))) {
			if (NULL == tokens[c].cb) {
				roff_err(tree, *argv, "unsupported "
						"macro `%s'",
						toknames[c]);
				return(0);
			}
			if ( ! (*tokens[c].cb)(c, tree, argv, ROFF_ENTER))
				return(0);
			break;
		}

		assert(tree->arg);
		if ( ! (*tree->cb.roffdata)(tree->arg, i, *argv++))
			return(0);
		i = 1;
	}

	/* 
	 * If we're the first parser (*argv == tree->cur) then purge out
	 * any additional punctuation, should there be any remaining at
	 * the end of line. 
	 */

	if (ROFF_PARSED & tokens[tok].flags && *argv) {
		i = 0;
		while (argv[i])
			i++;

		assert(i > 0);
		if ( ! roffispunct(argv[--i]))
			return((*tree->cb.roffout)(tree->arg, tok));

		while (i >= 0 && roffispunct(argv[i]))
			i--;

		assert(0 != i);
		i++;

		/* LINTED */
		while (argv[i])
			if ( ! (*tree->cb.roffdata)(tree->arg, 0, argv[i++]))
				return(0);
	}

	return((*tree->cb.roffout)(tree->arg, tok));
}


/* ARGSUSED */
static int
roff_text(ROFFCALL_ARGS) 
{
	int		 i, j, first, c, argcp[ROFF_MAXARG];
	char		*v, *argvp[ROFF_MAXARG];

	if (ROFF_PRELUDE & tree->state) {
		roff_err(tree, *argv, "`%s' disallowed in prelude", 
				toknames[tok]);
		return(0);
	}

	/* FIXME: breaks if passed from roff_layout. */
	first = *argv == tree->cur;

	i = 0;
	argv++;

	while (-1 != (c = roffnextopt(tree, tok, &argv, &v))) {
		if (ROFF_ARGMAX == c) 
			return(0);

		argcp[i] = c;
		argvp[i] = v;
		i++;
		argv++;
	}

	argcp[i] = ROFF_ARGMAX;
	argvp[i] = NULL;

	if ( ! (*tree->cb.roffin)(tree->arg, tok, 1, argcp, argvp))
		return(0);

	if ( ! (ROFF_PARSED & tokens[tok].flags)) {
		i = 0;
		while (*argv) {
			if ( ! (*tree->cb.roffdata)(tree->arg, i, *argv++))
				return(0);
			i = 1;
		}
		return((*tree->cb.roffout)(tree->arg, tok));
	}

	i = 0;
	while (*argv) {
		if (ROFF_MAX == (c = rofffindcallable(*argv))) {
			/* 
			 * If all that remains is roff punctuation, then
			 * close out our scope and return.
			 */
			if (roffispunct(*argv)) {
				for (j = 0; argv[j]; j++)
					if ( ! roffispunct(argv[j]))
						break;
				if (NULL == argv[j])
					break;
				i = 1;
			}
			
			if ( ! (*tree->cb.roffdata)(tree->arg, i, *argv++))
				return(0);

			i = 1;
			continue;
		}

		/*
		 * A sub-command has been found.  Execute it and
		 * discontinue parsing for arguments.
		 */

		if (NULL == tokens[c].cb) {
			roff_err(tree, *argv, "unsupported macro `%s'",
					toknames[c]);
			return(0);
		} 
		
		if ( ! (*tokens[c].cb)(c, tree, argv, ROFF_ENTER))
			return(0);

		break;
	}

	if ( ! (*tree->cb.roffout)(tree->arg, tok))
		return(0);

	/* 
	 * If we're the first parser (*argv == tree->cur) then purge out
	 * any additional punctuation, should there be any remaining at
	 * the end of line. 
	 */

	if (first && *argv) {
		i = 0;
		while (argv[i])
			i++;

		assert(i > 0);
		if ( ! roffispunct(argv[--i]))
			return(1);

		while (i >= 0 && roffispunct(argv[i]))
			i--;

		assert(0 != i);
		i++;

		/* LINTED */
		while (argv[i])
			if ( ! (*tree->cb.roffdata)(tree->arg, 0, argv[i++]))
				return(0);
	}

	return(1);
}


/* ARGSUSED */
static int
roff_comment(ROFFCALL_ARGS)
{

	return(1);
}


/* ARGSUSED */
static int
roff_close(ROFFCALL_ARGS)
{

	return(1);
}


/* ARGSUSED */
static int
roff_special(ROFFCALL_ARGS)
{

	return((*tree->cb.roffspecial)(tree->arg, tok));
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
