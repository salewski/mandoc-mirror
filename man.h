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
#ifndef MAN_H
#define MAN_H

#include <time.h>

extern	const char *const *man_macronames;

__BEGIN_DECLS

struct	man;

void	 	  man_free(struct man *);
struct	man	 *man_alloc(struct regset *, void *, mandocmsg);
void		  man_reset(struct man *);
int	 	  man_parseln(struct man *, int, char *, int);
int		  man_endparse(struct man *);

const struct man_node *man_node(const struct man *);
const struct man_meta *man_meta(const struct man *);

__END_DECLS

#endif /*!MAN_H*/
