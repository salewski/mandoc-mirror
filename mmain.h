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
#ifndef MMAIN_H
#define MMAIN_H

/* 
 * This is a convenience library for utilities implementing mdoc(3)
 * accepting a similar set of command-line patterns.  mmain handles
 * error reporting (to the terminal), command-line parsing, preparing
 * and reading the input file, and enacting the parse itself.
 */

#include "mdoc.h"

/* Rules for "dead" functions: */
#if defined(__NetBSD__)
#define	dead_pre	__dead
#define	dead_post	__attribute__((__noreturn__))
#elif defined(__OpenBSD__)
#define	dead_pre	__dead
#define	dead_post	/* Nothing. */
#else
#define	dead_pre	/* Nothing. */
#define	dead_post	__attribute__((__noreturn__))
#endif

__BEGIN_DECLS

struct	mmain;

struct	mmain		*mmain_alloc(void);
dead_pre void	 	 mmain_exit(struct mmain *, int) dead_post;
int			 mmain_getopt(struct mmain *, int, char *[], 
				const char *, const char *, void *,
				int (*)(void *, int, const char *));
struct mdoc		*mmain_mdoc(struct mmain *);
void			 mmain_usage(const char *);

__END_DECLS

#endif /*!MMAIN_H*/
