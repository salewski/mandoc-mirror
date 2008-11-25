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
#ifndef PRIVATE_H
#define PRIVATE_H

/* Input buffer (input read into buffer, then filled when empty). */
struct	md_rbuf {
	int		 fd;		/* Open descriptor. */
	char		*name;		/* Name of file. */
	char		*buf;		/* Buffer. */
	size_t		 bufsz;		/* Size of buffer. */
	size_t		 line;		/* Current line number. */
};

/* Output buffer (output buffered until full, then flushed). */
struct	md_mbuf {
	int		 fd;		/* Open descriptor. */
	char		*name;		/* Name of file. */
	char		*buf;		/* Buffer. */
	size_t		 bufsz;		/* Size of buffer. */
	size_t		 pos;		/* Position in buffer. */
};

#define	ROFF___	 	 0
#define	ROFF_Dd		 1
#define	ROFF_Dt		 2
#define	ROFF_Os		 3
#define	ROFF_Sh		 4
#define	ROFF_Ss		 5
#define	ROFF_Pp		 6
#define	ROFF_D1		 7
#define	ROFF_Dl		 8
#define	ROFF_Bd		 9
#define	ROFF_Ed		 10
#define	ROFF_Bl		 11
#define	ROFF_El		 12
#define	ROFF_It		 13
#define	ROFF_Ad		 15
#define	ROFF_An		 16
#define	ROFF_Ar		 17
#define	ROFF_Cd		 18
#define	ROFF_Cm		 19
#define	ROFF_Dv		 20
#define	ROFF_Er		 21
#define	ROFF_Ev		 22
#define	ROFF_Ex		 23
#define	ROFF_Fa		 24
#define	ROFF_Fd		 25
#define	ROFF_Fl		 26
#define	ROFF_Fn		 27
#define	ROFF_Ft		 28
#define	ROFF_Ic		 29
#define	ROFF_In		 30
#define	ROFF_Li		 31
#define	ROFF_Nd		 32
#define	ROFF_Nm		 33
#define	ROFF_Op		 34
#define	ROFF_Ot		 35
#define	ROFF_Pa		 36
#define	ROFF_Rv		 37
#define	ROFF_St		 38
#define	ROFF_Va		 39
#define	ROFF_Vt		 40
#define	ROFF_Xr		 41
#define	ROFF__A		 42
#define	ROFF__B		 43
#define	ROFF__D		 44
#define	ROFF__I		 45
#define	ROFF__J		 46
#define	ROFF__N		 47
#define	ROFF__O		 48
#define	ROFF__P		 49
#define	ROFF__R		 50
#define	ROFF__T		 51
#define	ROFF__V		 52
#define ROFF_Ac		 53
#define ROFF_Ao		 54
#define ROFF_Aq		 55
#define ROFF_At		 56
#define ROFF_Bc		 57
#define ROFF_Bf		 58
#define ROFF_Bo		 59
#define ROFF_Bq		 60
#define ROFF_Bsx	 61
#define ROFF_Bx		 62
#define ROFF_Db		 63
#define ROFF_Dc		 64
#define ROFF_Do		 65
#define ROFF_Dq		 66
#define ROFF_Ec		 67
#define ROFF_Ef		 68
#define ROFF_Em		 60
#define ROFF_Eo		 70
#define ROFF_Fx		 71
#define ROFF_Ms		 72
#define ROFF_No		 73
#define ROFF_Ns		 74
#define ROFF_Nx		 75
#define ROFF_Ox		 76
#define ROFF_Pc		 77
#define ROFF_Pf		 78
#define ROFF_Po		 79
#define ROFF_Pq		 80
#define ROFF_Qc		 81
#define ROFF_Ql		 82
#define ROFF_Qo		 83
#define ROFF_Qq		 84
#define ROFF_Re		 85
#define ROFF_Rs		 86
#define ROFF_Sc		 87
#define ROFF_So		 88
#define ROFF_Sq		 89
#define ROFF_Sm		 90
#define ROFF_Sx		 91
#define ROFF_Sy		 92
#define ROFF_Tn		 93
#define ROFF_Ux		 94
#define ROFF_Xc		 95
#define ROFF_Xo		 96
#define	ROFF_MAX	 97

#define	ROFF_Split	 0
#define	ROFF_Nosplit	 1
#define	ROFF_Ragged	 2
#define	ROFF_Unfilled	 3
#define	ROFF_Literal	 4
#define	ROFF_File	 5
#define	ROFF_Offset	 6
#define	ROFF_Bullet	 7
#define	ROFF_Dash	 8
#define	ROFF_Hyphen	 9
#define	ROFF_Item	 10
#define	ROFF_Enum	 11
#define	ROFF_Tag	 12
#define	ROFF_Diag	 13
#define	ROFF_Hang	 14
#define	ROFF_Ohang	 15
#define	ROFF_Inset	 16
#define	ROFF_Column	 17
#define	ROFF_Width	 18
#define	ROFF_Compact	 19
#define	ROFF_ARGMAX	 20

extern	const char *const *toknames;
extern	const char *const *tokargnames;

/* FIXME: have a md_roff with all necessary parameters. */

typedef	int	(*roffin)(int, int *, char **);
typedef	int	(*roffout)(int);
typedef	int	(*roffblkin)(int);
typedef	int	(*roffblkout)(int);

__BEGIN_DECLS

typedef	void  (*(*md_init)(const struct md_args *, 
			struct md_mbuf *, const struct md_rbuf *));
typedef	int	(*md_line)(void *, char *, size_t);
typedef	int	(*md_exit)(void *, int);

void		 *md_init_html4_strict(const struct md_args *,
			struct md_mbuf *, const struct md_rbuf *);
int		  md_line_html4_strict(void *, char *, size_t);
int		  md_exit_html4_strict(void *, int);

void		 *md_init_dummy(const struct md_args *,
			struct md_mbuf *, const struct md_rbuf *);
int		  md_line_dummy(void *, char *, size_t);
int		  md_exit_dummy(void *, int);

int	 	  md_buf_puts(struct md_mbuf *, const char *, size_t);
int	 	  md_buf_putchar(struct md_mbuf *, char);
int	 	  md_buf_putstring(struct md_mbuf *, const char *);

struct	rofftree;

struct	rofftree *roff_alloc(const struct md_args *, 
			struct md_mbuf *, const struct md_rbuf *,
			roffin, roffout, roffblkin, roffblkout);
int		  roff_engine(struct rofftree *, char *, size_t);
int		  roff_free(struct rofftree *, int);

__END_DECLS

#endif /*!PRIVATE_H*/
