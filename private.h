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

enum	mdoc_next {
	MDOC_NEXT_SIBLING = 0,
	MDOC_NEXT_CHILD
};

struct	mdoc {
	void		 *data;
	struct mdoc_cb	  cb;
	void		 *htab;
	int		  flags;
#define	MDOC_HALT	 (1 << 0)
	enum mdoc_next	  next;
	struct mdoc_node *last;
	struct mdoc_node *first;
	struct mdoc_meta  meta;
	enum mdoc_sec	  sec_lastn;
	enum mdoc_sec	  sec_last;
};


/* FIXME: it's 9 (this isn't used properly). */

#define	MDOC_LINEARG_MAX 12

#define	MACRO_PROT_ARGS	struct mdoc *mdoc, int tok, int line, \
			int ppos, int *pos, char *buf

struct	mdoc_macro {
	int		(*fp)(MACRO_PROT_ARGS);
	int		  flags;
#define	MDOC_CALLABLE	 (1 << 0)
#define	MDOC_PARSED	 (1 << 1)
#define	MDOC_EXPLICIT	 (1 << 2)
#define	MDOC_QUOTABLE	 (1 << 3)
#define	MDOC_PROLOGUE	 (1 << 4)
#define	MDOC_TABSEP	 (1 << 5)
};

#define	mdoc_nwarn(mdoc, node, type, fmt, ...) \
		  mdoc_vwarn((mdoc), (node)->line, \
		  (node)->pos, (type), (fmt), ##__VA_ARGS__)

#define	mdoc_nerr(mdoc, node, fmt, ...) \
		  mdoc_verr((mdoc), (node)->line, \
		  (node)->pos, (fmt), ##__VA_ARGS__)

#define	mdoc_warn(mdoc, type, fmt, ...) \
		  mdoc_vwarn((mdoc), (mdoc)->last->line, \
		  (mdoc)->last->pos, (type), (fmt), ##__VA_ARGS__)

#define	mdoc_err(mdoc, fmt, ...) \
		  mdoc_verr((mdoc), (mdoc)->last->line, \
		  (mdoc)->last->pos, (fmt), ##__VA_ARGS__)

#define	mdoc_msg(mdoc, fmt, ...) \
		  mdoc_vmsg((mdoc), (mdoc)->last->line, \
		  (mdoc)->last->pos, (fmt), ##__VA_ARGS__)

#define	mdoc_pmsg(mdoc, line, pos, fmt, ...) \
		  mdoc_vmsg((mdoc), (line), \
	  	  (pos), (fmt), ##__VA_ARGS__)

#define	mdoc_pwarn(mdoc, line, pos, type, fmt, ...) \
		  mdoc_vwarn((mdoc), (line), \
	  	  (pos), (type), (fmt), ##__VA_ARGS__)

#define	mdoc_perr(mdoc, line, pos, fmt, ...) \
		  mdoc_verr((mdoc), (line), \
	  	  (pos), (fmt), ##__VA_ARGS__)

extern	const struct mdoc_macro *const mdoc_macros;

__BEGIN_DECLS

int		  mdoc_vwarn(struct mdoc *, int, int, 
			enum mdoc_warn, const char *, ...);
void		  mdoc_vmsg(struct mdoc *, int, int, 
			const char *, ...);
int		  mdoc_verr(struct mdoc *, int, int, 
			const char *, ...);

int		  mdoc_macro(MACRO_PROT_ARGS);
int		  mdoc_find(const struct mdoc *, const char *);
int		  mdoc_word_alloc(struct mdoc *, 
			int, int, const char *);
int		  mdoc_elem_alloc(struct mdoc *, int, int, 
			int, size_t, const struct mdoc_arg *);
int		  mdoc_block_alloc(struct mdoc *, int, int, 
			int, size_t, const struct mdoc_arg *);
int		  mdoc_root_alloc(struct mdoc *);
int		  mdoc_head_alloc(struct mdoc *, int, int, int);
int		  mdoc_tail_alloc(struct mdoc *, int, int, int);
int		  mdoc_body_alloc(struct mdoc *, int, int, int);
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
enum	mdoc_att  mdoc_atoatt(const char *);
time_t		  mdoc_atotime(const char *);

int		  mdoc_valid_pre(struct mdoc *, struct mdoc_node *);
int		  mdoc_valid_post(struct mdoc *);
int		  mdoc_action_pre(struct mdoc *, struct mdoc_node *);
int		  mdoc_action_post(struct mdoc *);

int		  mdoc_argv(struct mdoc *, int, int, 
			struct mdoc_arg *, int *, char *);
#define	ARGV_ERROR	(-1)
#define	ARGV_EOLN	(0)
#define	ARGV_ARG	(1)
#define	ARGV_WORD	(2)
void		  mdoc_argv_free(int, struct mdoc_arg *);
int		  mdoc_args(struct mdoc *, int,
			int *, char *, int, char **);
#define	ARGS_ERROR	(-1)
#define	ARGS_EOLN	(0)
#define	ARGS_WORD	(1)
#define	ARGS_PUNCT	(2)

#define	ARGS_QUOTED	(1 << 0)
#define	ARGS_DELIM	(1 << 1)
#define	ARGS_TABSEP	(1 << 2)

int	  	  xstrlcat(char *, const char *, size_t);
int	  	  xstrlcpy(char *, const char *, size_t);
int	  	  xstrcmp(const char *, const char *);
void	 	 *xcalloc(size_t, size_t);
char	 	 *xstrdup(const char *);

int		  macro_obsolete(MACRO_PROT_ARGS);
int		  macro_constant(MACRO_PROT_ARGS);
int		  macro_constant_scoped(MACRO_PROT_ARGS);
int		  macro_constant_delimited(MACRO_PROT_ARGS);
int		  macro_text(MACRO_PROT_ARGS);
int		  macro_scoped(MACRO_PROT_ARGS);
int		  macro_scoped_close(MACRO_PROT_ARGS);
int		  macro_scoped_line(MACRO_PROT_ARGS);
int		  macro_prologue(MACRO_PROT_ARGS);
int		  macro_end(struct mdoc *);

__END_DECLS

#endif /*!PRIVATE_H*/
