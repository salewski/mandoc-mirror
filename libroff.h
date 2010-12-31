/*	$Id$ */
/*
 * Copyright (c) 2009, 2010 Kristaps Dzonsons <kristaps@bsd.lv>
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
#ifndef LIBROFF_H
#define LIBROFF_H

__BEGIN_DECLS

enum	tbl_part {
	TBL_PART_OPTS, /* in options (first line) */
	TBL_PART_LAYOUT, /* describing layout */
	TBL_PART_DATA  /* creating data rows */
};

enum	tbl_cellt {
	TBL_CELL_CENTRE, /* c, C */
	TBL_CELL_RIGHT, /* r, R */
	TBL_CELL_LEFT, /* l, L */
	TBL_CELL_NUMBER, /* n, N */
	TBL_CELL_SPAN, /* s, S */
	TBL_CELL_LONG, /* a, A */
	TBL_CELL_DOWN, /* ^ */
	TBL_CELL_HORIZ, /* _, - */
	TBL_CELL_DHORIZ, /* = */
	TBL_CELL_VERT, /* | */
	TBL_CELL_DVERT, /* || */
	TBL_CELL_MAX
};

struct	tbl_cell {
	struct tbl_cell	 *next;
	enum tbl_cellt	  pos;
	int		  spacing;
	int		  flags;
#define	TBL_CELL_TALIGN	 (1 << 0) /* t, T */
#define	TBL_CELL_BALIGN	 (1 << 1) /* d, D */
#define	TBL_CELL_BOLD	 (1 << 2) /* fB, B, b */
#define	TBL_CELL_ITALIC	 (1 << 3) /* fI, I, i */
#define	TBL_CELL_EQUAL	 (1 << 4) /* e, E */
#define	TBL_CELL_UP	 (1 << 5) /* u, U */
#define	TBL_CELL_WIGN	 (1 << 6) /* z, Z */
};

struct	tbl_row {
	struct tbl_row	 *next;
	struct tbl_cell	 *first;
	struct tbl_cell	 *last;
};

struct	tbl_dat {
	struct tbl_cell	 *layout; /* layout cell: CAN BE NULL */
	struct tbl_dat	 *next;
	char		 *string;
	int		  flags;
#define	TBL_DATA_HORIZ	 (1 << 0)
#define	TBL_DATA_DHORIZ	 (1 << 1)
#define	TBL_DATA_NHORIZ	 (1 << 2)
#define	TBL_DATA_NDHORIZ (1 << 3)
};

struct	tbl_span {
	struct tbl_row	 *layout; /* layout row: CAN BE NULL */
	struct tbl_dat	 *first;
	struct tbl_dat	 *last;
	int		  flags;
#define	TBL_SPAN_HORIZ	(1 << 0)
#define	TBL_SPAN_DHORIZ	(1 << 1)
	struct tbl_span	 *next;
};

struct	tbl {
	mandocmsg	  msg; /* status messages */
	void		 *data; /* privdata for messages */
	enum tbl_part	  part;
	char		  tab; /* cell-separator */
	char		  decimal; /* decimal point */
	int		  linesize;
	char		  delims[2];
	int		  opts;
#define	TBL_OPT_CENTRE	 (1 << 0)
#define	TBL_OPT_EXPAND	 (1 << 1)
#define	TBL_OPT_BOX	 (1 << 2)
#define	TBL_OPT_DBOX	 (1 << 3)
#define	TBL_OPT_ALLBOX	 (1 << 4)
#define	TBL_OPT_NOKEEP	 (1 << 5)
#define	TBL_OPT_NOSPACE	 (1 << 6)
	struct tbl_row	 *first_row;
	struct tbl_row	 *last_row;
	struct tbl_span	 *first_span;
	struct tbl_span	 *last_span;
	struct tbl	 *next;
};

#define	TBL_MSG(tblp, type, line, col) \
	(*(tblp)->msg)((type), (tblp)->data, (line), (col), NULL)

struct tbl	*tbl_alloc(void *, mandocmsg);
void		 tbl_restart(struct tbl *);
void		 tbl_free(struct tbl *);
void		 tbl_reset(struct tbl *);
enum rofferr 	 tbl_read(struct tbl *, int, const char *, int);
int		 tbl_option(struct tbl *, int, const char *);
int		 tbl_layout(struct tbl *, int, const char *);
int		 tbl_data(struct tbl *, int, const char *);

__END_DECLS

#endif /*LIBROFF_H*/
