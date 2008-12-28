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
	struct mdoc_meta  meta;
	enum mdoc_sec	  sec_lastn;
	enum mdoc_sec	  sec_last;
};

struct	mdoc_macro {
	int	(*fp)(struct mdoc *, int, int, int *, char *);
	int	  flags;
#define	MDOC_CALLABLE	(1 << 0)
#define	MDOC_EXPLICIT	(1 << 1)
#define	MDOC_PPOST	(1 << 2) /* Linescope: punctuation post-line. */
};

extern	const struct mdoc_macro *const mdoc_macros;

#define	MACRO_PROT_ARGS	struct mdoc *mdoc, int tok, \
			int ppos, int *pos, char *buf

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
void		 *mdoc_tokhash_alloc(void);
int		  mdoc_tokhash_find(const void *, const char *);
void		  mdoc_tokhash_free(void *);
int		  mdoc_isdelim(const char *);
int		  mdoc_iscdelim(char);
enum	mdoc_sec  mdoc_atosec(size_t, const char **);
enum	mdoc_msec mdoc_atomsec(const char *);
enum	mdoc_vol  mdoc_atovol(const char *);
enum	mdoc_arch mdoc_atoarch(const char *);
time_t		  mdoc_atotime(const char *);

int		  mdoc_argv(struct mdoc *, int, 
			struct mdoc_arg *, int *, char *);
void		  mdoc_argv_free(int, struct mdoc_arg *);
int		  mdoc_args(struct mdoc *, int,
			int *, char *, int, char **);
#define	ARGS_ERROR	(-1)
#define	ARGS_EOLN	(0)
#define	ARGS_WORD	(1)
#define	ARGS_PUNCT	(2)

#define	ARGS_QUOTED	(1 << 0)
#define	ARGS_DELIM	(1 << 1)

int	  	  xstrlcat(char *, const char *, size_t);
int	  	  xstrlcpy(char *, const char *, size_t);
int	  	  xstrcmp(const char *, const char *);
void	 	 *xcalloc(size_t, size_t);
char	 	 *xstrdup(const char *);

int		  macro_text(MACRO_PROT_ARGS);
int		  macro_scoped_implicit(MACRO_PROT_ARGS);
int		  macro_scoped_explicit(MACRO_PROT_ARGS);
int		  macro_scoped_line(MACRO_PROT_ARGS);
int		  macro_scoped_pline(MACRO_PROT_ARGS);
int		  macro_prologue_ddate(MACRO_PROT_ARGS);
int		  macro_prologue_dtitle(MACRO_PROT_ARGS);
int		  macro_prologue_os(MACRO_PROT_ARGS);

__END_DECLS

#endif /*!PRIVATE_H*/
