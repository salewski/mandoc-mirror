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

__BEGIN_DECLS

typedef	int	(*md_init)(const struct md_args *, struct md_mbuf *,
			const struct md_rbuf *, void **);
typedef	int	(*md_exit)(const struct md_args *, struct md_mbuf *, 
			const struct md_rbuf *, int, void *);
typedef	int	(*md_line)(const struct md_args *, 
			struct md_mbuf *, const struct md_rbuf *, 
			char *, size_t, void *);

int		  md_line_html4_strict(const struct md_args *,
			struct md_mbuf *, const struct md_rbuf *,
			char *, size_t, void *);
int		  md_init_html4_strict(const struct md_args *, 
			struct md_mbuf *, const struct md_rbuf *,
			void **);
int		  md_exit_html4_strict(const struct md_args *,
			struct md_mbuf *, const struct md_rbuf *, 
			int, void *);

int		  md_line_dummy(const struct md_args *,
			struct md_mbuf *, const struct md_rbuf *, 
			char *, size_t, void *);

int	 	  md_buf_puts(struct md_mbuf *, const char *, size_t);
int	 	  md_buf_putchar(struct md_mbuf *, char);
int	 	  md_buf_putstring(struct md_mbuf *, const char *);

__END_DECLS

#endif /*!PRIVATE_H*/
