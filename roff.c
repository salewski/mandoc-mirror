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
	const int	 *parents;		/* Limit to parents. */
	const int	 *children;		/* Limit to kids. */
	int		  ctx;			/* Blk-close node. */
	enum rofftype	  type;			/* Type of macro. */
	int		  flags;
#define	ROFF_PARSED	 (1 << 0)		/* "Parsed". */
#define	ROFF_CALLABLE	 (1 << 1)		/* "Callable". */
#define	ROFF_SHALLOW	 (1 << 2)		/* Nesting block. */
#define	ROFF_LSCOPE	 (1 << 3)		/* Line scope. */
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
	char		 *cur;			/* Line start. */
	struct tm	  tm;			/* `Dd' results. */
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
	struct roffcb	  cb;			/* Callbacks. */
	void		 *arg;			/* Callbacks' arg. */
};

static	int		  roff_Dd(ROFFCALL_ARGS);
static	int		  roff_Dt(ROFFCALL_ARGS);
static	int		  roff_Os(ROFFCALL_ARGS);
static	int		  roff_Ns(ROFFCALL_ARGS);
static	int		  roff_Sm(ROFFCALL_ARGS);
static	int		  roff_layout(ROFFCALL_ARGS);
static	int		  roff_text(ROFFCALL_ARGS);
static	int		  roff_noop(ROFFCALL_ARGS);
static	int		  roff_depr(ROFFCALL_ARGS);
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
static	int		  roffargs(const struct rofftree *,
				int, char *, char **);
static	int		  roffargok(int, int);
static	int		  roffnextopt(const struct rofftree *,
				int, char ***, char **);
static	int		  roffparseopts(struct rofftree *, int, 
				char ***, int *, char **);
static	int		  roffcall(struct rofftree *, int, char **);
static	int 		  roffparse(struct rofftree *, char *);
static	int		  textparse(struct rofftree *, char *);
static	int		  roffdata(struct rofftree *, int, char *);

#ifdef __linux__ 
extern	size_t		  strlcat(char *, const char *, size_t);
extern	size_t		  strlcpy(char *, const char *, size_t);
extern	int		  vsnprintf(char *, size_t, 
				const char *, va_list);
extern	char		 *strptime(const char *, const char *,
				struct tm *);
#endif


static	const int roffarg_An[] = { ROFF_Split, ROFF_Nosplit, ROFF_ARGMAX };
static	const int roffarg_Bd[] = { ROFF_Ragged, ROFF_Unfilled, ROFF_Literal,
	ROFF_File, ROFF_Offset, ROFF_Filled, ROFF_Compact, ROFF_ARGMAX };
static	const int roffarg_Bk[] = { ROFF_Words, ROFF_ARGMAX };
static	const int roffarg_Ex[] = { ROFF_Std, ROFF_ARGMAX };
static	const int roffarg_Rv[] = { ROFF_Std, ROFF_ARGMAX };
static 	const int roffarg_Bl[] = { ROFF_Bullet, ROFF_Dash, ROFF_Hyphen,
	ROFF_Item, ROFF_Enum, ROFF_Tag, ROFF_Diag, ROFF_Hang, ROFF_Ohang,
	ROFF_Inset, ROFF_Column, ROFF_Offset, ROFF_Width, ROFF_Compact,
	ROFF_ARGMAX };
static 	const int roffarg_St[] = { ROFF_p1003_1_88, ROFF_p1003_1_90,
	ROFF_p1003_1_96, ROFF_p1003_1_2001, ROFF_p1003_1_2004, ROFF_p1003_1,
	ROFF_p1003_1b, ROFF_p1003_1b_93, ROFF_p1003_1c_95, ROFF_p1003_1g_2000,
	ROFF_p1003_2_92, ROFF_p1387_2_95, ROFF_p1003_2, ROFF_p1387_2,
	ROFF_isoC_90, ROFF_isoC_amd1, ROFF_isoC_tcor1, ROFF_isoC_tcor2,
	ROFF_isoC_99, ROFF_ansiC, ROFF_ansiC_89, ROFF_ansiC_99, ROFF_ieee754,
	ROFF_iso8802_3, ROFF_xpg3, ROFF_xpg4, ROFF_xpg4_2, ROFF_xpg4_3,
	ROFF_xbd5, ROFF_xcu5, ROFF_xsh5, ROFF_xns5, ROFF_xns5_2d2_0,
	ROFF_xcurses4_2, ROFF_susv2, ROFF_susv3, ROFF_svid4, ROFF_ARGMAX };

static	const int roffchild_Bl[] = { ROFF_It, ROFF_El, ROFF_MAX };
static	const int roffchild_Fo[] = { ROFF_Fa, ROFF_Fc, ROFF_MAX };
static	const int roffchild_Rs[] = { ROFF_Re, ROFF__A, ROFF__B, ROFF__D,
	ROFF__I, ROFF__J, ROFF__N, ROFF__O, ROFF__P, ROFF__R, ROFF__T, ROFF__V,
	ROFF_MAX };

static	const int roffparent_El[] = { ROFF_Bl, ROFF_It, ROFF_MAX };
static	const int roffparent_Fc[] = { ROFF_Fo, ROFF_Fa, ROFF_MAX };
static	const int roffparent_Oc[] = { ROFF_Oo, ROFF_MAX };
static	const int roffparent_It[] = { ROFF_Bl, ROFF_It, ROFF_MAX };
static	const int roffparent_Re[] = { ROFF_Rs, ROFF_MAX };

static	const struct rofftok tokens[ROFF_MAX] = {
	{   roff_noop, NULL, NULL, NULL, 0, ROFF_COMMENT, 0 }, /* \" */
	{     roff_Dd, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* Dd */
	{     roff_Dt, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* Dt */
	{     roff_Os, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* Os */
	{ roff_layout, NULL, NULL, NULL, ROFF_Sh, ROFF_LAYOUT, 0 }, /* Sh */
	{ roff_layout, NULL, NULL, NULL, ROFF_Ss, ROFF_LAYOUT, 0 }, /* Ss */ 
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* Pp */ /* XXX 0 args */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_LSCOPE }, /* D1 */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_LSCOPE }, /* Dl */
	{ roff_layout, roffarg_Bd, NULL, NULL, 0, ROFF_LAYOUT, 0 }, 	/* Bd */
	{   roff_noop, NULL, NULL, NULL, ROFF_Bd, ROFF_LAYOUT, 0 }, /* Ed */
	{ roff_layout, roffarg_Bl, NULL, roffchild_Bl, 0, ROFF_LAYOUT, 0 }, /* Bl */
	{   roff_noop, NULL, roffparent_El, NULL, ROFF_Bl, ROFF_LAYOUT, 0 }, /* El */
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
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE | ROFF_LSCOPE }, /* Op */
	{   roff_depr, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* Ot */
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
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE | ROFF_LSCOPE }, /* Aq */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* At */ /* XXX at most 2 args */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Bc */
	{ roff_layout, NULL, NULL, NULL, 0, ROFF_LAYOUT, 0 }, /* Bf */ /* FIXME */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Bo */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE | ROFF_LSCOPE }, /* Bq */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Bsx */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Bx */
	{        NULL, NULL, NULL, NULL, 0, ROFF_SPECIAL, 0 },	/* Db */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Dc */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Do */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE | ROFF_LSCOPE }, /* Dq */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ec */
	{   roff_noop, NULL, NULL, NULL, ROFF_Bf, ROFF_LAYOUT, 0 }, /* Ef */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Em */ /* XXX needs arg */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Eo */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Fx */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Ms */
	{   NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* No */
	{     roff_Ns, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ns */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Nx */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Ox */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Pc */
	{        NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Pf */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Po */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE | ROFF_LSCOPE }, /* Pq */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Qc */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ql */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Qo */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE | ROFF_LSCOPE }, /* Qq */
	{   roff_noop, NULL, roffparent_Re, NULL, ROFF_Rs, ROFF_LAYOUT, 0 }, /* Re */
	{ roff_layout, NULL, NULL, roffchild_Rs, 0, ROFF_LAYOUT, 0 },	/* Rs */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Sc */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* So */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE | ROFF_LSCOPE }, /* Sq */
	{     roff_Sm, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* Sm */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Sx */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Sy */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Tn */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Ux */
	{  NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Xc */
	{  NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Xo */
	{ roff_layout, NULL, NULL, roffchild_Fo, 0, ROFF_LAYOUT, 0 }, /* Fo */
	{   roff_noop, NULL, roffparent_Fc, NULL, ROFF_Fo, ROFF_LAYOUT, 0 }, /* Fc */
	{ roff_layout, NULL, NULL, NULL, 0, ROFF_LAYOUT, 0 }, /* Oo */
	{   roff_noop, NULL, roffparent_Oc, NULL, ROFF_Oo, ROFF_LAYOUT, 0 }, /* Oc */
	{ roff_layout, roffarg_Bk, NULL, NULL, 0, ROFF_LAYOUT, 0 }, /* Bk */
	{   roff_noop, NULL, NULL, NULL, ROFF_Bk, ROFF_LAYOUT, 0 }, /* Ek */
	{        NULL, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* Bt */
	{        NULL, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* Hf */
	{   roff_depr, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* Fr */
	{        NULL, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* Ud */
	};

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
	"Bk",		"Ek",		"Bt",		"Hf",
	"Fr",		"Ud",
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
		if ( ! (*tokens[t].cb)(t, tree, NULL, ROFF_EXIT))
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
	} else if (NULL == tokens[tok].cb) {
		roff_err(tree, buf + 1, "unsupported macro `%s'", 
				toknames[tok]);
		return(0);
	}

	assert(ROFF___ != tok);
	if ( ! roffargs(tree, tok, buf, argv)) 
		return(0);

	argvp = (char **)argv;

	/* 
	 * Prelude macros break some assumptions, so branch now. 
	 */
	
	if (ROFF_PRELUDE & tree->state) {
		assert(NULL == tree->last);
		return((*tokens[tok].cb)(tok, tree, argvp, ROFF_ENTER));
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
		return((*tokens[tok].cb)(tok, tree, argvp, ROFF_ENTER));
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
		if ( ! (*tokens[t].cb)(t, tree, NULL, ROFF_EXIT))
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
roffcall(struct rofftree *tree, int tok, char **argv)
{

	if (NULL == tokens[tok].cb) {
		roff_err(tree, *argv, "unsupported macro `%s'", 
				toknames[tok]);
		return(0);
	}
	if ( ! (*tokens[tok].cb)(tok, tree, argv, ROFF_ENTER))
		return(0);
	return(1);
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
	int		 tok;

	if (0 == *buf)
		return(1);

	if (-1 == (tok = rofftok_scan(buf))) {
		roff_err(tree, buf, "invalid character sequence");
		return(0);
	} else if (ROFFTok_MAX != tok) {
		if (ROFFTok_Null == tok) { /* FIXME */
			buf += 2;
			return(roffdata(tree, space, buf));
		}
		return((*tree->cb.rofftoken)
				(tree->arg, space != 0, tok));
	}

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
	} else if (strlcpy(tree->section, *argv, sizeof(tree->section))
			>= sizeof(tree->section)) {
		roff_err(tree, *argv, "`Dt' section too long");
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


/* ARGSUSED */
static	int
roff_Sm(ROFFCALL_ARGS)
{
	int		 argcp[1];
	char		*argvp[1], *morep[1], *p;

	p = *argv++;

	argcp[0] = ROFF_ARGMAX;
	argvp[0] = NULL;
	if (NULL == (morep[0] = *argv++)) {
		roff_err(tree, p, "`Sm' expects an argument");
		return(0);
	} else if (0 != strcmp(morep[0], "on") && 
			0 != strcmp(morep[0], "off")) {
		roff_err(tree, p, "`Sm' has invalid argument");
		return(0);
	}

	if (*argv) 
		roff_warn(tree, *argv, "`Sm' shouldn't have arguments");

	if ( ! (*tree->cb.roffspecial)(tree->arg, 
				tok, argcp, argvp, morep))
		return(0);

	while (*argv)
		if ( ! roffdata(tree, 1, *argv++))
			return(0);

	return(1);
}


/* ARGSUSED */
static	int
roff_Ns(ROFFCALL_ARGS)
{
	int		 j, c, first;
	int		 argcp[1];
	char		*argvp[1], *morep[1];

	first = (*argv++ == tree->cur);

	argcp[0] = ROFF_ARGMAX;
	argvp[0] = morep[0] = NULL;

	if ( ! (*tree->cb.roffspecial)(tree->arg, 
				tok, argcp, argvp, morep))
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

	if ( ! (*tree->cb.roffblkin)(tree->arg, tok, argcp, argvp))
		return(0);
	if (NULL == *argv)
		return((*tree->cb.roffblkbodyin)
				(tree->arg, tok, argcp, argvp));

	if ( ! (*tree->cb.roffblkheadin)(tree->arg, tok, argcp, argvp))
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
				(tree->arg, tok, argcp, argvp));
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
				(tree->arg, tok, argcp, argvp));
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
			(tree->arg, tok, argcp, argvp));
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
	if ( ! (*tree->cb.roffin)(tree->arg, tok, argcp, argvp))
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
			if ( ! roffdata(tree, 0, *argv++))
				return(0);
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

