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

#include "mdoc.h"

struct	mdoc {
	void		 *data;
	struct mdoc_cb	  cb;
	void		 *htab;
	struct mdoc_node *last;
	struct mdoc_node *first;

	enum mdoc_sec	  sec_lastn;
	enum mdoc_sec	  sec_last;
};

struct	mdoc_macro {
	int	(*fp)(struct mdoc *, int, int, int *, char *);
	int	  flags;
#define	MDOC_CALLABLE	(1 << 0)
#define	MDOC_EXPLICIT	(1 << 1)
};

extern	const struct mdoc_macro *const mdoc_macros;

__BEGIN_DECLS

int		  mdoc_err(struct mdoc *, int, int, enum mdoc_err);
int		  mdoc_warn(struct mdoc *, int, int, enum mdoc_warn);
void		  mdoc_msg(struct mdoc *, int, const char *, ...);
int		  mdoc_macro(struct mdoc *, int, int, int *, char *);
int		  mdoc_find(const struct mdoc *, const char *);
void		  mdoc_word_alloc(struct mdoc *, int, const char *);
void		  mdoc_elem_alloc(struct mdoc *, int, int, 
			size_t, const struct mdoc_arg *, 
			size_t, const char **);
void		  mdoc_block_alloc(struct mdoc *, int, int, 
			size_t, const struct mdoc_arg *);
void		  mdoc_head_alloc(struct mdoc *, 
			int, int, size_t, const char **);
void		  mdoc_body_alloc(struct mdoc *, int, int);
void		  mdoc_node_free(struct mdoc_node *);
void		  mdoc_sibling(struct mdoc *, int, struct mdoc_node **,
			struct mdoc_node **, struct mdoc_node *);
void		 *mdoc_hash_alloc(void);
int		  mdoc_hash_find(const void *, const char *);
void		  mdoc_hash_free(void *);
int		  mdoc_isdelim(const char *);

__END_DECLS

#endif /*!PRIVATE_H*/
