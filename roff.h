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
#ifndef ROFF_H
#define ROFF_H

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

struct	rofftree;

#define	ROFFCALL_ARGS \
	int tok, struct rofftree *tree, \
	char *argv[], enum roffd type

struct	rofftok {
	int		(*cb)(ROFFCALL_ARGS);	/* Callback. */
	const int	 *args;			/* Args (or NULL). */
	const int	 *parents;		/* Limit to parents. */
	const int	 *children;		/* Limit to kids. */
	int		  ctx;			/* Blk-close node. */
	enum rofftype	  type;			/* Type of macro. */
	int	  	  flags;
#define	ROFF_PARSED	 (1 << 0)		/* "Parsed". */
#define	ROFF_CALLABLE	 (1 << 1)		/* "Callable". */
#define	ROFF_SHALLOW	 (1 << 2)		/* Nesting block. */
#define	ROFF_LSCOPE	 (1 << 3)		/* Line scope. */
};

__BEGIN_DECLS

static	int		  roff_Dd(ROFFCALL_ARGS); /* FIXME: deprecate. */
static	int		  roff_Dt(ROFFCALL_ARGS); /* FIXME: deprecate. */
static	int		  roff_Os(ROFFCALL_ARGS); /* FIXME: deprecate. */
static	int		  roff_Ns(ROFFCALL_ARGS); /* FIXME: deprecate. */
static	int		  roff_layout(ROFFCALL_ARGS);
static	int		  roff_text(ROFFCALL_ARGS);
static	int		  roff_noop(ROFFCALL_ARGS);
static	int		  roff_depr(ROFFCALL_ARGS);
static	int		  roff_ordered(ROFFCALL_ARGS);

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
	{roff_ordered, roffarg_Ex, NULL, NULL, 0, ROFF_TEXT, 0 }, /* Ex */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Fa */ /* XXX needs arg */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* Fd */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Fl */
	{roff_ordered, NULL, NULL, NULL, 0, ROFF_TEXT, /*XXX*/ -1 }, /* Fn */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Ft */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ic */ /* XXX needs arg */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* In */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Li */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* Nd */
	{roff_ordered, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Nm */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE | ROFF_LSCOPE }, /* Op */
	{   roff_depr, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* Ot */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Pa */
/*Ok*/	{roff_ordered, roffarg_Rv, NULL, NULL, 0, ROFF_TEXT, 0 }, /* Rv */
/*Ok*/	{roff_ordered, roffarg_St, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* St */
/*Ok*/	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Va */
/*Ok*/	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Vt */ /* FIXME: section/linebreak. */
/*Ok*/	{roff_ordered, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Xr */
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
/*Ok*/	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE | ROFF_LSCOPE }, /* Aq */
/*Ok*/	{roff_ordered, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* At */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Bc */
	{ roff_layout, NULL, NULL, NULL, 0, ROFF_LAYOUT, 0 }, /* Bf */ /* FIXME */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Bo */
/*Ok*/	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE | ROFF_LSCOPE }, /* Bq */
/*Ok*/	{roff_ordered, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Bsx */
/*Ok*/	{roff_ordered, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Bx */
	{        NULL, NULL, NULL, NULL, 0, ROFF_SPECIAL, 0 },	/* Db */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Dc */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Do */
/*Ok*/	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE | ROFF_LSCOPE }, /* Dq */
	{        NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ec */
	{   roff_noop, NULL, NULL, NULL, ROFF_Bf, ROFF_LAYOUT, 0 }, /* Ef */
/*Ok*/	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Em */ /* XXX needs arg */
	{        NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Eo */
/*Ok*/	{roff_ordered, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Fx */
	{        NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Ms */
	{        NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* No */
/*Ok*/	{     roff_Ns, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ns */
/*Ok*/	{roff_ordered, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Nx */
/*Ok*/	{roff_ordered, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Ox */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Pc */
	{        NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Pf */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Po */
/*Ok*/	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE | ROFF_LSCOPE }, /* Pq */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Qc */
/*Ok*/	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Ql */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Qo */
/*Ok*/	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE | ROFF_LSCOPE }, /* Qq */
	{   roff_noop, NULL, roffparent_Re, NULL, ROFF_Rs, ROFF_LAYOUT, 0 }, /* Re */
	{ roff_layout, NULL, NULL, roffchild_Rs, 0, ROFF_LAYOUT, 0 },	/* Rs */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Sc */
	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* So */
/*Ok*/	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE | ROFF_LSCOPE }, /* Sq */
/*Ok*/	{roff_ordered, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* Sm */
/*Ok*/	{roff_ordered, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Sx */
/*Ok*/	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Sy */
/*Ok*/	{   roff_text, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Tn */
/*Ok*/	{roff_ordered, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED }, /* Ux */
	{        NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Xc */
	{        NULL, NULL, NULL, NULL, 0, ROFF_TEXT, ROFF_PARSED | ROFF_CALLABLE }, /* Xo */
/*Ok*/	{ roff_layout, NULL, NULL, roffchild_Fo, 0, ROFF_LAYOUT, 0 }, /* Fo */ /* FIXME: section/linebreak. */
/*Ok*/	{   roff_noop, NULL, roffparent_Fc, NULL, ROFF_Fo, ROFF_LAYOUT, 0 }, /* Fc */ /* FIXME: section/linebreak. */
/*Ok*/	{ roff_layout, NULL, NULL, NULL, 0, ROFF_LAYOUT, 0 }, /* Oo */
/*Ok*/	{   roff_noop, NULL, roffparent_Oc, NULL, ROFF_Oo, ROFF_LAYOUT, 0 }, /* Oc */
	{        NULL, roffarg_Bk, NULL, NULL, 0, ROFF_LAYOUT, 0 }, /* Bk */
	{        NULL, NULL, NULL, NULL, ROFF_Bk, ROFF_LAYOUT, 0 }, /* Ek */
/*Ok*/	{roff_ordered, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* Bt */
	{        NULL, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* Hf */
/*Ok*/	{   roff_depr, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* Fr */
/*Ok*/	{roff_ordered, NULL, NULL, NULL, 0, ROFF_TEXT, 0 }, /* Ud */
	};

#define	ROFF_VALUE	(1 << 0)

static	const int tokenargs[ROFF_ARGMAX] = {
	0,		0,		0,		0,
	0,		ROFF_VALUE,	ROFF_VALUE,	0,
	0,		0,		0,		0,
	0,		0,		0,		0,
	0,		0,		ROFF_VALUE,	0,
	0,		ROFF_VALUE,	0,		0,
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

__END_DECLS

#endif /*!ROFF_H*/
