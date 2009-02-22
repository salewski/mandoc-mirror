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
#ifndef MDOC_H
#define MDOC_H

/* What follows is a list of ALL possible macros. */

#define	MDOC___	 	 0
#define	MDOC_Dd		 1
#define	MDOC_Dt		 2
#define	MDOC_Os		 3
#define	MDOC_Sh		 4
#define	MDOC_Ss		 5
#define	MDOC_Pp		 6
#define	MDOC_D1		 7
#define	MDOC_Dl		 8
#define	MDOC_Bd		 9
#define	MDOC_Ed		 10
#define	MDOC_Bl		 11
#define	MDOC_El		 12
#define	MDOC_It		 13
#define	MDOC_Ad		 14
#define	MDOC_An		 15
#define	MDOC_Ar		 16
#define	MDOC_Cd		 17
#define	MDOC_Cm		 18
#define	MDOC_Dv		 19
#define	MDOC_Er		 20
#define	MDOC_Ev		 21
#define	MDOC_Ex		 22
#define	MDOC_Fa		 23
#define	MDOC_Fd		 24
#define	MDOC_Fl		 25
#define	MDOC_Fn		 26
#define	MDOC_Ft		 27
#define	MDOC_Ic		 28
#define	MDOC_In		 29
#define	MDOC_Li		 30
#define	MDOC_Nd		 31
#define	MDOC_Nm		 32
#define	MDOC_Op		 33
#define	MDOC_Ot		 34
#define	MDOC_Pa		 35
#define	MDOC_Rv		 36
#define	MDOC_St		 37
#define	MDOC_Va		 38
#define	MDOC_Vt		 39
#define	MDOC_Xr		 40
#define	MDOC__A		 41
#define	MDOC__B		 42
#define	MDOC__D		 43
#define	MDOC__I		 44
#define	MDOC__J		 45
#define	MDOC__N		 46
#define	MDOC__O		 47
#define	MDOC__P		 48
#define	MDOC__R		 49
#define	MDOC__T		 50
#define	MDOC__V		 51
#define MDOC_Ac		 52
#define MDOC_Ao		 53
#define MDOC_Aq		 54
#define MDOC_At		 55
#define MDOC_Bc		 56
#define MDOC_Bf		 57
#define MDOC_Bo		 58
#define MDOC_Bq		 59
#define MDOC_Bsx	 60
#define MDOC_Bx		 61
#define MDOC_Db		 62
#define MDOC_Dc		 63
#define MDOC_Do		 64
#define MDOC_Dq		 65
#define MDOC_Ec		 66
#define MDOC_Ef		 67
#define MDOC_Em		 68
#define MDOC_Eo		 69
#define MDOC_Fx		 70
#define MDOC_Ms		 71
#define MDOC_No		 72
#define MDOC_Ns		 73
#define MDOC_Nx		 74
#define MDOC_Ox		 75
#define MDOC_Pc		 76
#define MDOC_Pf		 77
#define MDOC_Po		 78
#define MDOC_Pq		 79
#define MDOC_Qc		 80
#define MDOC_Ql		 81
#define MDOC_Qo		 82
#define MDOC_Qq		 83
#define MDOC_Re		 84
#define MDOC_Rs		 85
#define MDOC_Sc		 86
#define MDOC_So		 87
#define MDOC_Sq		 88
#define MDOC_Sm		 89
#define MDOC_Sx		 90
#define MDOC_Sy		 91
#define MDOC_Tn		 92
#define MDOC_Ux		 93
#define MDOC_Xc		 94
#define MDOC_Xo		 95
#define	MDOC_Fo		 96
#define	MDOC_Fc		 97
#define	MDOC_Oo		 98
#define	MDOC_Oc		 99
#define	MDOC_Bk		 100
#define	MDOC_Ek		 101
#define	MDOC_Bt		 102
#define	MDOC_Hf		 103
#define	MDOC_Fr		 104
#define	MDOC_Ud		 105
#define	MDOC_MAX	 106

/* What follows is a list of ALL possible macro arguments. */

#define	MDOC_Split	 0
#define	MDOC_Nosplit	 1
#define	MDOC_Ragged	 2
#define	MDOC_Unfilled	 3
#define	MDOC_Literal	 4
#define	MDOC_File	 5
#define	MDOC_Offset	 6
#define	MDOC_Bullet	 7
#define	MDOC_Dash	 8
#define	MDOC_Hyphen	 9
#define	MDOC_Item	 10
#define	MDOC_Enum	 11
#define	MDOC_Tag	 12
#define	MDOC_Diag	 13
#define	MDOC_Hang	 14
#define	MDOC_Ohang	 15
#define	MDOC_Inset	 16
#define	MDOC_Column	 17
#define	MDOC_Width	 18
#define	MDOC_Compact	 19
#define	MDOC_Std	 20
#define MDOC_p1003_1_88	 21
#define MDOC_p1003_1_90	 22
#define MDOC_p1003_1_96	 23
#define MDOC_p1003_1_2001 24
#define MDOC_p1003_1_2004 25
#define MDOC_p1003_1	 26
#define MDOC_p1003_1b	 27
#define MDOC_p1003_1b_93 28
#define MDOC_p1003_1c_95 29
#define MDOC_p1003_1g_2000 30
#define MDOC_p1003_2_92	 31
#define MDOC_p1387_2_95	 32
#define MDOC_p1003_2	 33
#define MDOC_p1387_2	 34
#define MDOC_isoC_90	 35
#define MDOC_isoC_amd1	 36
#define MDOC_isoC_tcor1	 37
#define MDOC_isoC_tcor2	 38
#define MDOC_isoC_99	 39
#define MDOC_ansiC	 40
#define MDOC_ansiC_89	 41
#define MDOC_ansiC_99	 42
#define MDOC_ieee754	 43
#define MDOC_iso8802_3	 44
#define MDOC_xpg3	 45
#define MDOC_xpg4	 46
#define MDOC_xpg4_2	 47
#define MDOC_xpg4_3	 48
#define MDOC_xbd5	 49
#define MDOC_xcu5	 50
#define MDOC_xsh5	 51
#define MDOC_xns5	 52
#define MDOC_xns5_2d2_0	 53
#define MDOC_xcurses4_2	 54
#define MDOC_susv2	 55
#define MDOC_susv3	 56
#define MDOC_svid4	 57
#define	MDOC_Filled	 58
#define	MDOC_Words	 59
#define	MDOC_Emphasis	 60
#define	MDOC_Symbolic	 61
#define	MDOC_ARG_MAX	 62

/* Warnings are either syntax or groff-compatibility. */
enum	mdoc_warn {
	WARN_SYNTAX,
	WARN_COMPAT
};

/* Possible values for the `At' macro. */
enum	mdoc_att {
	ATT_DEFAULT = 0,
	ATT_v1,
	ATT_v2,
	ATT_v3,
	ATT_v4,
	ATT_v5,
	ATT_v6,
	ATT_v7,
	ATT_32v,
	ATT_V1,
	ATT_V2,
	ATT_V3,
	ATT_V4
};

/* An argument to a macro (multiple values = `It -column'). */
struct	mdoc_arg {
	int	  	  arg;
	int		  line;
	int		  pos;
	size_t		  sz;
	char		**value;
};

/* Type of a syntax node. */
enum	mdoc_type {
	MDOC_TEXT,
	MDOC_ELEM,
	MDOC_HEAD,
	MDOC_TAIL,
	MDOC_BODY,
	MDOC_BLOCK,
	MDOC_ROOT
};

/* Manual section. */
enum	mdoc_msec {
	MSEC_DEFAULT = 0,
	MSEC_1,
	MSEC_2,
	MSEC_3,
	MSEC_3f,
	MSEC_3p,
	MSEC_4,
	MSEC_5,
	MSEC_6,
	MSEC_7,
	MSEC_8,
	MSEC_9,
	MSEC_X11,
	MSEC_X11R6,
	MSEC_local,
	MSEC_n,
	MSEC_unass,
	MSEC_draft,
	MSEC_paper
};

/* Section (named/unnamed) of `Ss'. */
enum	mdoc_sec {
	SEC_PROLOGUE = 0,
	SEC_BODY,
	SEC_NAME,
	SEC_SYNOPSIS,
	SEC_DESCRIPTION,
	SEC_RETURN_VALUES,
	SEC_ENVIRONMENT,
	SEC_FILES,
	SEC_EXAMPLES,
	SEC_DIAGNOSTICS,
	SEC_ERRORS,
	SEC_SEE_ALSO,
	SEC_STANDARDS,
	SEC_HISTORY,
	SEC_AUTHORS,
	SEC_CAVEATS,
	SEC_BUGS,
	SEC_CUSTOM
};

/* Volume of `Dt'. */
enum	mdoc_vol {
	VOL_DEFAULT = 0,
	VOL_AMD,
	VOL_IND,
	VOL_KM,
	VOL_LOCAL,
	VOL_PRM,
	VOL_PS1,
	VOL_SMM,
	VOL_URM,
	VOL_USD
};

/* Architecture of `Dt'. */
enum	mdoc_arch {
	ARCH_DEFAULT = 0,
	ARCH_alpha, 
	ARCH_amd64, 
	ARCH_amiga, 
	ARCH_arc, 
	ARCH_arm, 
	ARCH_armish, 
	ARCH_aviion, 
	ARCH_hp300,
	ARCH_hppa, 
	ARCH_hppa64, 
	ARCH_i386, 
	ARCH_landisk, 
	ARCH_luna88k, 
	ARCH_mac68k, 
	ARCH_macppc,
	ARCH_mvme68k, 
	ARCH_mvme88k, 
	ARCH_mvmeppc, 
	ARCH_pmax, 
	ARCH_sgi, 
	ARCH_socppc, 
	ARCH_sparc,
	ARCH_sparc64, 
	ARCH_sun3, 
	ARCH_vax, 
	ARCH_zaurus
};

/* Information from prologue. */
struct	mdoc_meta {
	enum mdoc_msec	  msec;
	enum mdoc_vol	  vol;
	enum mdoc_arch	  arch;
	time_t		  date;
	char		 *title;
	char		 *os;
	char		 *name;
};

/* Text-only node. */
struct	mdoc_text {
	char		 *string;
};

/* Block (scoped) node. */
struct	mdoc_block {
	size_t		  argc;
	struct mdoc_arg	 *argv;
	struct mdoc_node *head;
	struct mdoc_node *body;
	struct mdoc_node *tail;
};

/* In-line element node. */
struct	mdoc_elem {
	size_t		  sz;
	char		**args;
	size_t		  argc;
	struct mdoc_arg	 *argv;
};

/* Typed nodes of an AST node. */
union	mdoc_data {
	struct mdoc_text  text;
	struct mdoc_elem  elem;
	struct mdoc_block block;
};

/* Node in AST. */
struct	mdoc_node {
	struct mdoc_node *parent;
	struct mdoc_node *child;
	struct mdoc_node *next;
	struct mdoc_node *prev;
	int		  line;
	int		  pos;
	int		  tok;
	int		  flags;
#define	MDOC_VALID	 (1 << 0)
#define	MDOC_ACTED	 (1 << 1)
	enum mdoc_type	  type;
	union mdoc_data	  data;
	enum mdoc_sec	  sec;
};

/* Call-backs for parse messages. */
struct	mdoc_cb {
	void	(*mdoc_msg)(void *, int, int, const char *);
	int	(*mdoc_err)(void *, int, int, const char *);
	int	(*mdoc_warn)(void *, int, int, 
			enum mdoc_warn, const char *);
};

/* Global table of macro names (`Bd', `Ed', etc.). */
extern	const char *const *mdoc_macronames;

/* Global table of argument names (`column', `tag', etc.). */
extern	const char *const *mdoc_argnames;

__BEGIN_DECLS

struct	mdoc;

/* Free memory allocated with mdoc_alloc. */
void	 	  mdoc_free(struct mdoc *);

/* Allocate a new parser instance. */
struct	mdoc	 *mdoc_alloc(void *data, const struct mdoc_cb *);

/* Parse a single line in a stream (boolean retval). */
int	 	  mdoc_parseln(struct mdoc *, int, char *buf);

/* Get result first node (after mdoc_endparse!). */
const struct mdoc_node *mdoc_node(const struct mdoc *);

/* Get result meta-information (after mdoc_endparse!). */
const struct mdoc_meta *mdoc_meta(const struct mdoc *);

/* Signal end of parse sequence (boolean retval). */
int		  mdoc_endparse(struct mdoc *);

const char 	 *mdoc_arch2a(enum mdoc_arch);

const char 	 *mdoc_vol2a(enum mdoc_vol);

const char 	 *mdoc_msec2a(enum mdoc_msec);

int		  mdoc_isdelim(const char *);

__END_DECLS

#endif /*!MDOC_H*/
