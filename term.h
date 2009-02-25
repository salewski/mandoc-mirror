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
#ifndef TERM_H
#define TERM_H

#include "mdoc.h"

__BEGIN_DECLS

struct	termp {
	size_t		  rmargin;
	size_t		  maxrmargin;
	size_t		  maxcols;
	size_t		  offset;
	size_t		  col;
	int		  flags;
#define	TERMP_BOLD	 (1 << 0)	/* Embolden words. */
#define	TERMP_UNDERLINE	 (1 << 1)	/* Underline words. */
#define	TERMP_NOSPACE	 (1 << 2)	/* No space before words. */
#define	TERMP_NOLPAD	 (1 << 3)	/* No leftpad before flush. */
#define	TERMP_NOBREAK	 (1 << 4)	/* No break after flush. */
#define	TERMP_LITERAL	 (1 << 5)	/* Literal words. */
#define	TERMP_IGNDELIM	 (1 << 6)	/* Delims like regulars. */
	char		 *buf;
};

struct	termpair {
	int		  type;
#define	TERMPAIR_FLAG	 (1 << 0)
	union {
		int	  flag;
	} data;
};

#define	TERMPAIR_SETFLAG(p, fl) \
	do { \
		(p)->data.flag = (fl); \
		(p)->type = TERMPAIR_FLAG; \
	} while (0)

struct	termact {
	int		(*pre)(struct termp *,
				struct termpair *,
				const struct mdoc_meta *,
				const struct mdoc_node *);
	void		(*post)(struct termp *,
				struct termpair *,
				const struct mdoc_meta *,
				const struct mdoc_node *);
};

void			  newln(struct termp *);
void			  vspace(struct termp *);
void			  word(struct termp *, const char *);
void			  flushln(struct termp *);
void			  transcode(struct termp *, 
				const char *, size_t);

void			  subtree(struct termp *,
				const struct mdoc_meta *,
				const struct mdoc_node *);

const	struct termact 	 *termacts;

__END_DECLS

#endif /*!TERM_H*/
