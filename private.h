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

struct	md_rbuf {
	int		 fd;
	char		*name;
	char		*buf;
	size_t		 bufsz;
	size_t		 line;
};

struct	md_mbuf {
	int		 fd;
	char		*name;
	char		*buf;
	size_t		 bufsz;
	size_t		 pos;
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
#define	ROFF_An		 14
#define	ROFF_Li		 15
#define	ROFF_MAX	 16

#define	ROFF_NAMES			\
	{				\
	"\\\"",				\
	"Dd",				\
	"Dt",				\
	"Os",				\
	"Sh",				\
	"Ss",				\
	"Pp",				\
	"D1",				\
	"Dl",				\
	"Bd",				\
	"Ed",				\
	"Bl",				\
	"El",				\
	"It",				\
	"An",				\
	"Li",				\
	}

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
#define	ROFF_ARGMAX	 18

#define	ROFF_ARGNAMES 			\
	{ 				\
	"split",			\
	"nosplit",			\
	"ragged",			\
	"unfilled",			\
	"literal",			\
	"file",				\
	"offset",			\
	"bullet",			\
	"dash",				\
	"hyphen",			\
	"item",				\
	"enum",				\
	"tag",				\
	"diag",				\
	"hang",				\
	"ohang",			\
	"inset",			\
	"column",			\
	}

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
