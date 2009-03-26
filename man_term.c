/* $Id$ */
/*
 * Copyright (c) 2008, 2009 Kristaps Dzonsons <kristaps@openbsd.org>
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
#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "term.h"
#include "man.h"

#define	DECL_ARGS 	  struct termp *p, \
			  const struct man_node *n, \
			  const struct man_meta *m

struct	termact {
	int		(*pre)(DECL_ARGS);
	void		(*post)(DECL_ARGS);
};

static	int		  pre_B(DECL_ARGS);
static	int		  pre_I(DECL_ARGS);
static	int		  pre_PP(DECL_ARGS);
static	int		  pre_SH(DECL_ARGS);
static	int		  pre_SS(DECL_ARGS);
static	int		  pre_TP(DECL_ARGS);

static	void		  post_B(DECL_ARGS);
static	void		  post_I(DECL_ARGS);
static	void		  post_SH(DECL_ARGS);
static	void		  post_SS(DECL_ARGS);

static const struct termact termacts[MAN_MAX] = {
	{ NULL, NULL }, /* __ */
	{ NULL, NULL }, /* TH */
	{ pre_SH, post_SH }, /* SH */
	{ pre_SS, post_SS }, /* SS */
	{ pre_TP, NULL }, /* TP */
	{ pre_PP, NULL }, /* LP */
	{ pre_PP, NULL }, /* PP */
	{ pre_PP, NULL }, /* P */
	{ NULL, NULL }, /* IP */
	{ pre_PP, NULL }, /* HP */ /* XXX */
	{ NULL, NULL }, /* SM */
	{ pre_B, post_B }, /* SB */
	{ NULL, NULL }, /* BI */
	{ NULL, NULL }, /* IB */
	{ NULL, NULL }, /* BR */
	{ NULL, NULL }, /* RB */
	{ NULL, NULL }, /* R */
	{ pre_B, post_B }, /* B */
	{ pre_I, post_I }, /* I */
	{ NULL, NULL }, /* IR */
	{ NULL, NULL }, /* RI */
};

static	void		  print_head(struct termp *, 
				const struct man_meta *);
static	void		  print_body(DECL_ARGS);
static	void		  print_node(DECL_ARGS);
static	void		  print_foot(struct termp *, 
				const struct man_meta *);


int
man_run(struct termp *p, const struct man *m)
{

	print_head(p, man_meta(m));
	p->flags |= TERMP_NOSPACE;
	print_body(p, man_node(m), man_meta(m));
	print_foot(p, man_meta(m));

	return(1);
}


static int
pre_I(DECL_ARGS)
{

	p->flags |= TERMP_UNDER;
	return(1);
}


static void
post_I(DECL_ARGS)
{

	p->flags &= ~TERMP_UNDER;
}


static int
pre_B(DECL_ARGS)
{

	p->flags |= TERMP_BOLD;
	return(1);
}


static void
post_B(DECL_ARGS)
{

	p->flags &= ~TERMP_BOLD;
}


static int
pre_PP(DECL_ARGS)
{

	term_vspace(p);
	p->offset = INDENT;
	return(0);
}


static int
pre_TP(DECL_ARGS)
{
	const struct man_node *nn;
	size_t		 offs;

	term_vspace(p);
	p->offset = INDENT;

	if (NULL == (nn = n->child))
		return(1);

	if (nn->line == n->line) {
		if (MAN_TEXT != nn->type)
			errx(1, "expected text line argument");
		offs = atoi(nn->string);
		nn = nn->next;
	} else
		offs = INDENT;

	for ( ; nn; nn = nn->next)
		print_node(p, nn, m);

	term_flushln(p);
	p->flags |= TERMP_NOSPACE;
	p->offset += offs;
	return(0);
}


static int
pre_SS(DECL_ARGS)
{

	term_vspace(p);
	p->flags |= TERMP_BOLD;
	return(1);
}


static void
post_SS(DECL_ARGS)
{
	
	term_flushln(p);
	p->flags &= ~TERMP_BOLD;
	p->flags |= TERMP_NOSPACE;
}


static int
pre_SH(DECL_ARGS)
{

	term_vspace(p);
	p->offset = 0;
	p->flags |= TERMP_BOLD;
	return(1);
}


static void
post_SH(DECL_ARGS)
{
	
	term_flushln(p);
	p->offset = INDENT;
	p->flags &= ~TERMP_BOLD;
	p->flags |= TERMP_NOSPACE;
}


static void
print_node(DECL_ARGS)
{
	int		 c;

	c = 1;

	switch (n->type) {
	case(MAN_ELEM):
		if (termacts[n->tok].pre)
			c = (*termacts[n->tok].pre)(p, n, m);
		break;
	case(MAN_TEXT):
		if (*n->string) {
			term_word(p, n->string);
			break;
		}
		term_vspace(p);
		break;
	default:
		break;
	}

	if (c && n->child)
		print_body(p, n->child, m);

	switch (n->type) {
	case (MAN_ELEM):
		if (termacts[n->tok].post)
			(*termacts[n->tok].post)(p, n, m);
		break;
	default:
		break;
	}
}


static void
print_body(DECL_ARGS)
{
	print_node(p, n, m);
	if ( ! n->next)
		return;
	print_body(p, n->next, m);
}


static void
print_foot(struct termp *p, const struct man_meta *meta)
{
	struct tm	*tm;
	char		*buf;

	if (NULL == (buf = malloc(p->rmargin)))
		err(1, "malloc");

	tm = localtime(&meta->date);

#ifdef __OpenBSD__
	if (NULL == strftime(buf, p->rmargin, "%B %d, %Y", tm))
#else
	if (0 == strftime(buf, p->rmargin, "%B %d, %Y", tm))
#endif
		err(1, "strftime");

	/*
	 * This is /slightly/ different from regular groff output
	 * because we don't have page numbers.  Print the following:
	 *
	 * OS                                            MDOCDATE
	 */

	term_vspace(p);

	p->flags |= TERMP_NOSPACE | TERMP_NOBREAK;
	p->rmargin = p->maxrmargin - strlen(buf);
	p->offset = 0;

	if (meta->source)
		term_word(p, meta->source);
	if (meta->source)
		term_word(p, "");
	term_flushln(p);

	p->flags |= TERMP_NOLPAD | TERMP_NOSPACE;
	p->offset = p->rmargin;
	p->rmargin = p->maxrmargin;
	p->flags &= ~TERMP_NOBREAK;

	term_word(p, buf);
	term_flushln(p);

	free(buf);
}


static void
print_head(struct termp *p, const struct man_meta *meta)
{
	char		*buf, *title;

	p->rmargin = p->maxrmargin;
	p->offset = 0;

	if (NULL == (buf = malloc(p->rmargin)))
		err(1, "malloc");
	if (NULL == (title = malloc(p->rmargin)))
		err(1, "malloc");

	/*
	 * The header is strange.  It has three components, which are
	 * really two with the first duplicated.  It goes like this:
	 *
	 * IDENTIFIER              TITLE                   IDENTIFIER
	 *
	 * The IDENTIFIER is NAME(SECTION), which is the command-name
	 * (if given, or "unknown" if not) followed by the manual page
	 * section.  These are given in `Dt'.  The TITLE is a free-form
	 * string depending on the manual volume.  If not specified, it
	 * switches on the manual section.
	 */

	if (meta->vol)
		(void)strlcpy(buf, meta->vol, p->rmargin);
	else
		*buf = 0;

	(void)snprintf(title, p->rmargin, "%s(%d)", 
			meta->title, meta->msec);

	p->offset = 0;
	p->rmargin = (p->maxrmargin - strlen(buf)) / 2;
	p->flags |= TERMP_NOBREAK | TERMP_NOSPACE;

	term_word(p, title);
	term_flushln(p);

	p->flags |= TERMP_NOLPAD | TERMP_NOSPACE;
	p->offset = p->rmargin;
	p->rmargin = p->maxrmargin - strlen(title);

	term_word(p, buf);
	term_flushln(p);

	p->offset = p->rmargin;
	p->rmargin = p->maxrmargin;
	p->flags &= ~TERMP_NOBREAK;
	p->flags |= TERMP_NOLPAD | TERMP_NOSPACE;

	term_word(p, title);
	term_flushln(p);

	p->rmargin = p->maxrmargin;
	p->offset = 0;
	p->flags &= ~TERMP_NOSPACE;

	free(title);
	free(buf);
}

