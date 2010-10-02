/*	$Id$ */
/*
 * Copyright (c) 2010 Kristaps Dzonsons <kristaps@bsd.lv>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef MANDOC_H
#define MANDOC_H

#define ASCII_NBRSP	 31  /* non-breaking space */
#define	ASCII_HYPH	 30  /* breakable hyphen */

__BEGIN_DECLS

enum	mandoclevel {
	MANDOCLEVEL_OK = 0,
	MANDOCLEVEL_RESERVED,
	MANDOCLEVEL_WARNING,
	MANDOCLEVEL_ERROR,
	MANDOCLEVEL_FATAL,
	MANDOCLEVEL_BADARG,
	MANDOCLEVEL_SYSERR,
	MANDOCLEVEL_MAX
};

enum	mandocerr {
	MANDOCERR_OK,

	MANDOCERR_WARNING, /* ===== start of warnings ===== */
	MANDOCERR_UPPERCASE, /* text should be uppercase */
	MANDOCERR_SECOOO, /* sections out of conventional order */
	MANDOCERR_SECREP, /* section name repeats */
	MANDOCERR_PROLOGOOO, /* out of order prologue */
	MANDOCERR_PROLOGREP, /* repeated prologue entry */
	MANDOCERR_LISTFIRST, /* list type must come first */
	MANDOCERR_BADSTANDARD, /* bad standard */
	MANDOCERR_BADLIB, /* bad library */
	MANDOCERR_BADTAB, /* tab in non-literal context */
	MANDOCERR_BADESCAPE, /* bad escape sequence */
	MANDOCERR_BADQUOTE, /* unterminated quoted string */
	MANDOCERR_NOWIDTHARG, /* argument requires the width argument */
	/* FIXME: merge with MANDOCERR_IGNARGV. */
	MANDOCERR_WIDTHARG, /* superfluous width argument */
	MANDOCERR_IGNARGV, /* ignoring argument */
	MANDOCERR_BADDATE, /* bad date argument */
	MANDOCERR_BADWIDTH, /* bad width argument */
	MANDOCERR_BADMSEC, /* unknown manual section */
	MANDOCERR_SECMSEC, /* section not in conventional manual section */
	MANDOCERR_EOLNSPACE, /* end of line whitespace */
	MANDOCERR_SCOPENEST, /* blocks badly nested */

	MANDOCERR_ERROR, /* ===== start of errors ===== */
	MANDOCERR_NAMESECFIRST, /* NAME section must come first */
	MANDOCERR_BADBOOL, /* bad Boolean value */
	MANDOCERR_CHILD, /* child violates parent syntax */
	MANDOCERR_BADATT, /* bad AT&T symbol */
	MANDOCERR_LISTREP, /* list type repeated */
	MANDOCERR_DISPREP, /* display type repeated */
	MANDOCERR_ARGVREP, /* argument repeated */
	MANDOCERR_NONAME, /* manual name not yet set */
	MANDOCERR_MACROOBS, /* obsolete macro ignored */
	MANDOCERR_MACROEMPTY, /* empty macro ignored */
	MANDOCERR_BADBODY, /* macro not allowed in body */
	MANDOCERR_BADPROLOG, /* macro not allowed in prologue */
	MANDOCERR_BADCHAR, /* bad character */
	MANDOCERR_BADNAMESEC, /* bad NAME section contents */
	MANDOCERR_NOBLANKLN, /* no blank lines */
	MANDOCERR_NOTEXT, /* no text in this context */
	MANDOCERR_BADCOMMENT, /* bad comment style */
	MANDOCERR_MACRO, /* unknown macro will be lost */
	MANDOCERR_LINESCOPE, /* line scope broken */
	MANDOCERR_ARGCOUNT, /* argument count wrong */
	MANDOCERR_NOSCOPE, /* no such block is open */
	MANDOCERR_SCOPEREP, /* scope already open */
	MANDOCERR_SCOPEEXIT, /* scope open on exit */
	/* FIXME: merge following with MANDOCERR_ARGCOUNT */
	MANDOCERR_NOARGS, /* macro requires line argument(s) */
	MANDOCERR_NOBODY, /* macro requires body argument(s) */
	MANDOCERR_NOARGV, /* macro requires argument(s) */
	MANDOCERR_NOTITLE, /* no title in document */
	MANDOCERR_LISTTYPE, /* missing list type */
	MANDOCERR_DISPTYPE, /* missing display type */
	MANDOCERR_FONTTYPE, /* missing font type */
	MANDOCERR_ARGSLOST, /* line argument(s) will be lost */
	MANDOCERR_BODYLOST, /* body argument(s) will be lost */
	MANDOCERR_IGNPAR, /* paragraph macro ignored */

	MANDOCERR_FATAL, /* ===== start of fatal errors ===== */
	MANDOCERR_COLUMNS, /* column syntax is inconsistent */
	/* FIXME: this should be a MANDOCERR_ERROR */
	MANDOCERR_NESTEDDISP, /* displays may not be nested */
	MANDOCERR_BADDISP, /* unsupported display type */
	MANDOCERR_SCOPEFATAL, /* blocks badly nested */
	MANDOCERR_SYNTNOSCOPE, /* no scope to rewind: syntax violated */
	MANDOCERR_SYNTLINESCOPE, /* line scope broken, syntax violated */
	MANDOCERR_SYNTARGVCOUNT, /* argument count wrong, violates syntax */
	MANDOCERR_SYNTCHILD, /* child violates parent syntax */
	MANDOCERR_SYNTARGCOUNT, /* argument count wrong, violates syntax */
	MANDOCERR_NODOCBODY, /* no document body */
	MANDOCERR_NODOCPROLOG, /* no document prologue */
	MANDOCERR_UTSNAME, /* utsname system call failed */
	MANDOCERR_MEM, /* static buffer exhausted */
	MANDOCERR_MAX
};

enum	regs {
	REG_nS = 0,	/* register: nS */
	REG__MAX
};

enum	mant {
	MAN_br = 0,
	MAN_TH,
	MAN_SH,
	MAN_SS,
	MAN_TP,
	MAN_LP,
	MAN_PP,
	MAN_P,
	MAN_IP,
	MAN_HP,
	MAN_SM,
	MAN_SB,
	MAN_BI,
	MAN_IB,
	MAN_BR,
	MAN_RB,
	MAN_R,
	MAN_B,
	MAN_I,
	MAN_IR,
	MAN_RI,
	MAN_na,
	MAN_i,
	MAN_sp,
	MAN_nf,
	MAN_fi,
	MAN_r,
	MAN_RE,
	MAN_RS,
	MAN_DT,
	MAN_UC,
	MAN_PD,
	MAN_Sp,
	MAN_Vb,
	MAN_Ve,
	MAN_AT,
	MAN_in,
	MAN_MAX
};

enum	man_type {
	MAN_TEXT,
	MAN_ELEM,
	MAN_ROOT,
	MAN_BLOCK,
	MAN_HEAD,
	MAN_BODY
};

enum	mdoct {
	MDOC_Ap = 0,
	MDOC_Dd,
	MDOC_Dt,
	MDOC_Os,
	MDOC_Sh,
	MDOC_Ss,
	MDOC_Pp,
	MDOC_D1,
	MDOC_Dl,
	MDOC_Bd,
	MDOC_Ed,
	MDOC_Bl,
	MDOC_El,
	MDOC_It,
	MDOC_Ad,
	MDOC_An,
	MDOC_Ar,
	MDOC_Cd,
	MDOC_Cm,
	MDOC_Dv,
	MDOC_Er,
	MDOC_Ev,
	MDOC_Ex,
	MDOC_Fa,
	MDOC_Fd,
	MDOC_Fl,
	MDOC_Fn,
	MDOC_Ft,
	MDOC_Ic,
	MDOC_In,
	MDOC_Li,
	MDOC_Nd,
	MDOC_Nm,
	MDOC_Op,
	MDOC_Ot,
	MDOC_Pa,
	MDOC_Rv,
	MDOC_St,
	MDOC_Va,
	MDOC_Vt,
	MDOC_Xr,
	MDOC__A,
	MDOC__B,
	MDOC__D,
	MDOC__I,
	MDOC__J,
	MDOC__N,
	MDOC__O,
	MDOC__P,
	MDOC__R,
	MDOC__T,
	MDOC__V,
	MDOC_Ac,
	MDOC_Ao,
	MDOC_Aq,
	MDOC_At,
	MDOC_Bc,
	MDOC_Bf,
	MDOC_Bo,
	MDOC_Bq,
	MDOC_Bsx,
	MDOC_Bx,
	MDOC_Db,
	MDOC_Dc,
	MDOC_Do,
	MDOC_Dq,
	MDOC_Ec,
	MDOC_Ef,
	MDOC_Em,
	MDOC_Eo,
	MDOC_Fx,
	MDOC_Ms,
	MDOC_No,
	MDOC_Ns,
	MDOC_Nx,
	MDOC_Ox,
	MDOC_Pc,
	MDOC_Pf,
	MDOC_Po,
	MDOC_Pq,
	MDOC_Qc,
	MDOC_Ql,
	MDOC_Qo,
	MDOC_Qq,
	MDOC_Re,
	MDOC_Rs,
	MDOC_Sc,
	MDOC_So,
	MDOC_Sq,
	MDOC_Sm,
	MDOC_Sx,
	MDOC_Sy,
	MDOC_Tn,
	MDOC_Ux,
	MDOC_Xc,
	MDOC_Xo,
	MDOC_Fo,
	MDOC_Fc,
	MDOC_Oo,
	MDOC_Oc,
	MDOC_Bk,
	MDOC_Ek,
	MDOC_Bt,
	MDOC_Hf,
	MDOC_Fr,
	MDOC_Ud,
	MDOC_Lb,
	MDOC_Lp,
	MDOC_Lk,
	MDOC_Mt,
	MDOC_Brq,
	MDOC_Bro,
	MDOC_Brc,
	MDOC__C,
	MDOC_Es,
	MDOC_En,
	MDOC_Dx,
	MDOC__Q,
	MDOC_br,
	MDOC_sp,
	MDOC__U,
	MDOC_Ta,
	MDOC_MAX
};

enum	mdocargt {
	MDOC_Split,
	MDOC_Nosplit,
	MDOC_Ragged,
	MDOC_Unfilled,
	MDOC_Literal,
	MDOC_File,
	MDOC_Offset,
	MDOC_Bullet,
	MDOC_Dash,
	MDOC_Hyphen,
	MDOC_Item,
	MDOC_Enum,
	MDOC_Tag,
	MDOC_Diag,
	MDOC_Hang,
	MDOC_Ohang,
	MDOC_Inset,
	MDOC_Column,
	MDOC_Width,
	MDOC_Compact,
	MDOC_Std,
	MDOC_Filled,
	MDOC_Words,
	MDOC_Emphasis,
	MDOC_Symbolic,
	MDOC_Nested,
	MDOC_Centred,
	MDOC_ARG_MAX
};

enum	mdoc_type {
	MDOC_TEXT,
	MDOC_ELEM,
	MDOC_HEAD,
	MDOC_TAIL,
	MDOC_BODY,
	MDOC_BLOCK,
	MDOC_ROOT
};

enum	mdoc_sec {
	SEC_NONE,
	SEC_NAME,
	SEC_LIBRARY,
	SEC_SYNOPSIS,
	SEC_DESCRIPTION,
	SEC_IMPLEMENTATION,
	SEC_RETURN_VALUES,
	SEC_ENVIRONMENT, 
	SEC_FILES,
	SEC_EXIT_STATUS,
	SEC_EXAMPLES,
	SEC_DIAGNOSTICS,
	SEC_COMPATIBILITY,
	SEC_ERRORS,
	SEC_SEE_ALSO,
	SEC_STANDARDS,
	SEC_HISTORY,
	SEC_AUTHORS,
	SEC_CAVEATS,
	SEC_BUGS,
	SEC_SECURITY,
	SEC_CUSTOM,
	SEC__MAX
};

enum	mdoc_endbody {
	ENDBODY_NOT = 0,
	ENDBODY_SPACE,
	ENDBODY_NOSPACE
};

enum	mdoc_list {
	LIST__NONE = 0,
	LIST_bullet,
	LIST_column,
	LIST_dash,
	LIST_diag,
	LIST_enum,
	LIST_hang,
	LIST_hyphen,
	LIST_inset,
	LIST_item,
	LIST_ohang,
	LIST_tag
};

enum	mdoc_disp {
	DISP__NONE = 0,
	DISP_centred,
	DISP_ragged,
	DISP_unfilled,
	DISP_filled,
	DISP_literal
};

enum	mdoc_auth {
	AUTH__NONE = 0,
	AUTH_split,
	AUTH_nosplit
};

enum	mdoc_font {
	FONT__NONE = 0,
	FONT_Em,
	FONT_Li,
	FONT_Sy
};

struct	man_meta {
	char		*msec; /* `TH' section (e.g., 1--9) */
	time_t		 date; /* parsed `TH' date */
	char		*rawdate; /* raw `TH' date */
	char		*vol; /* `TH' volume (e.g., KM) */
	char		*title; /* `TH' title (e.g., LS, CAT) */
	char		*source; /* `TH' source (e.g., GNU) */
};

struct	man_node {
	struct man_node	*parent;
	struct man_node	*child;
	struct man_node	*next;
	struct man_node	*prev;
	int		 nchild;
	int		 line;
	int		 pos;
	enum mant	 tok;
	int		 flags;
#define	MAN_VALID	(1 << 0)
#define	MAN_ACTED	(1 << 1)
#define	MAN_EOS		(1 << 2)
	enum man_type	 type;
	char		*string;
	struct man_node	*head;
	struct man_node	*body;
};

struct	mdoc_meta {
	char		 *msec; /* `Dt' section (e.g., 1--9) */
	char		 *vol; /* `Dt' volume (e.g., LOCAL) */
	char		 *arch; /* `Dt' architecture (e.g., i386) */
	time_t		  date; /* `Dd' date */
	char		 *title; /* `Dt' title (e.g., LS, CAT) */
	char		 *os; /* `Os' system (e.g., OpenBSD) */
	char		 *name; /* leading `Nm' (e.g., ls, cat) */
};

struct	mdoc_argv {
	enum mdocargt  	  arg;
	int		  line;
	int		  pos;
	size_t		  sz;
	char		**value;
};

struct 	mdoc_arg {
	size_t		  argc;
	struct mdoc_argv *argv;
	unsigned int	  refcnt;
};


struct	mdoc_bd {
	const char	 *offs; /* -offset */
	enum mdoc_disp	  type; /* -ragged, etc. */
	int		  comp; /* -compact */
};

struct	mdoc_bl {
	const char	 *width; /* -width */
	const char	 *offs; /* -offset */
	enum mdoc_list	  type; /* -tag, -enum, etc. */
	int		  comp; /* -compact */
	size_t		  ncols; /* -column arg count */
	const char	**cols; /* -column val ptr */
};

struct	mdoc_bf {
	enum mdoc_font	  font; /* font */
};

struct	mdoc_an {
	enum mdoc_auth	  auth; /* -split, etc. */
};

struct	mdoc_node {
	struct mdoc_node *parent; /* parent AST node */
	struct mdoc_node *child; /* first child AST node */
	struct mdoc_node *next; /* sibling AST node */
	struct mdoc_node *prev; /* prior sibling AST node */
	int		  nchild; /* number children */
	int		  line; /* parse line */
	int		  pos; /* parse column */
	enum mdoct	  tok; /* tok or MDOC__MAX if none */
	int		  flags;
#define	MDOC_VALID	 (1 << 0) /* has been validated */
#define	MDOC_ACTED	 (1 << 1) /* has been acted upon */
#define	MDOC_EOS	 (1 << 2) /* at sentence boundary */
#define	MDOC_LINE	 (1 << 3) /* first macro/text on line */
#define	MDOC_SYNPRETTY	 (1 << 4) /* SYNOPSIS-style formatting */
#define	MDOC_ENDED	 (1 << 5) /* rendering has been ended */
	enum mdoc_type	  type; /* AST node type */
	enum mdoc_sec	  sec; /* current named section */
	/* XXX: these can be union'd to shave a few bytes. */
	struct mdoc_arg	 *args; 	/* BLOCK/ELEM */
	struct mdoc_node *pending;	/* BLOCK */
	struct mdoc_node *head;		/* BLOCK */
	struct mdoc_node *body;		/* BLOCK */
	struct mdoc_node *tail;		/* BLOCK */
	char		 *string;	/* TEXT */
	enum mdoc_endbody end;		/* BODY */

	union {
		struct mdoc_an  An;
		struct mdoc_bd *Bd;
		struct mdoc_bf *Bf;
		struct mdoc_bl *Bl;
	} data;
};

/*
 * A single register entity.  If "set" is zero, the value of the
 * register should be the default one, which is per-register.  It's
 * assumed that callers know which type in "v" corresponds to which
 * register value.
 */
struct	reg {
	int		  set; /* whether set or not */
	union {
		unsigned  u; /* unsigned integer */
	} v;
};

/*
 * The primary interface to setting register values is in libroff,
 * although libmdoc and libman from time to time will manipulate
 * registers (such as `.Sh SYNOPSIS' enabling REG_nS).
 */
struct	regset {
	struct reg	  regs[REG__MAX];
};

/*
 * Callback function for warnings, errors, and fatal errors as they
 * occur in the compilers libroff, libmdoc, and libman.
 */
typedef	int		(*mandocmsg)(enum mandocerr, void *,
				int, int, const char *);

__END_DECLS

#endif /*!MANDOC_H*/
