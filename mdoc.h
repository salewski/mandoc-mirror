/*	$Id$ */
/*
 * Copyright (c) 2008, 2009, 2010 Kristaps Dzonsons <kristaps@bsd.lv>
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
#ifndef MDOC_H
#define MDOC_H

/*
 * This library implements a validating scanner/parser for ``mdoc'' roff
 * macro documents, a.k.a. BSD manual page documents.  The mdoc.c file
 * drives the parser, while macro.c describes the macro ontologies.
 * validate.c pre- and post-validates parsed macros, and action.c
 * performs actions on parsed and validated macros.
 */

/* See mdoc.3 for documentation. */

extern	const char *const *mdoc_macronames;
extern	const char *const *mdoc_argnames;

__BEGIN_DECLS

struct	mdoc;

/* See mdoc.3 for documentation. */

void	 	  mdoc_free(struct mdoc *);
struct	mdoc	 *mdoc_alloc(struct regset *, void *, mandocmsg);
void		  mdoc_reset(struct mdoc *);
int	 	  mdoc_parseln(struct mdoc *, int, char *, int);
const struct mdoc_node *mdoc_node(const struct mdoc *);
const struct mdoc_meta *mdoc_meta(const struct mdoc *);
int		  mdoc_endparse(struct mdoc *);

__END_DECLS

#endif /*!MDOC_H*/
