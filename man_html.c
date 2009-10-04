/*	$Id$ */
/*
 * Copyright (c) 2008, 2009 Kristaps Dzonsons <kristaps@kth.se>
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
#include <sys/types.h>
#include <sys/queue.h>

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "html.h"
#include "man.h"

#define	INDENT		  7
#define	HALFINDENT	  3

#define	MAN_ARGS	  const struct man_meta *m, \
			  const struct man_node *n, \
			  struct html *h

struct	htmlman {
	int		(*pre)(MAN_ARGS);
	int		(*post)(MAN_ARGS);
};

static	void		  print_man(MAN_ARGS);
static	void		  print_man_head(MAN_ARGS);
static	void		  print_man_nodelist(MAN_ARGS);
static	void		  print_man_node(MAN_ARGS);

static	int		  a2width(const struct man_node *);

static	int		  man_br_pre(MAN_ARGS);
static	int		  man_IP_pre(MAN_ARGS);
static	int		  man_PP_pre(MAN_ARGS);
static	void		  man_root_post(MAN_ARGS);
static	int		  man_root_pre(MAN_ARGS);
static	int		  man_SH_pre(MAN_ARGS);
static	int		  man_SS_pre(MAN_ARGS);

#ifdef __linux__
extern	size_t	  	  strlcpy(char *, const char *, size_t);
extern	size_t	  	  strlcat(char *, const char *, size_t);
#endif

static	const struct htmlman mans[MAN_MAX] = {
	{ man_br_pre, NULL }, /* br */
	{ NULL, NULL }, /* TH */
	{ man_SH_pre, NULL }, /* SH */
	{ man_SS_pre, NULL }, /* SS */
	{ NULL, NULL }, /* TP */
	{ man_PP_pre, NULL }, /* LP */
	{ man_PP_pre, NULL }, /* PP */
	{ man_PP_pre, NULL }, /* P */
	{ man_IP_pre, NULL }, /* IP */
	{ NULL, NULL }, /* HP */ 
	{ NULL, NULL }, /* SM */
	{ NULL, NULL }, /* SB */
	{ NULL, NULL }, /* BI */
	{ NULL, NULL }, /* IB */
	{ NULL, NULL }, /* BR */
	{ NULL, NULL }, /* RB */
	{ NULL, NULL }, /* R */
	{ NULL, NULL }, /* B */
	{ NULL, NULL }, /* I */
	{ NULL, NULL }, /* IR */
	{ NULL, NULL }, /* RI */
	{ NULL, NULL }, /* na */
	{ NULL, NULL }, /* i */
	{ man_br_pre, NULL }, /* sp */
	{ NULL, NULL }, /* nf */
	{ NULL, NULL }, /* fi */
	{ NULL, NULL }, /* r */
	{ NULL, NULL }, /* RE */
	{ NULL, NULL }, /* RS */
	{ NULL, NULL }, /* DT */
	{ NULL, NULL }, /* UC */
};


void
html_man(void *arg, const struct man *m)
{
	struct html	*h;
	struct tag	*t;

	h = (struct html *)arg;

	print_gen_doctype(h);

	t = print_otag(h, TAG_HTML, 0, NULL);
	print_man(man_meta(m), man_node(m), h);
	print_tagq(h, t);

	printf("\n");
}


static void
print_man(MAN_ARGS) 
{
	struct tag	*t;
	struct htmlpair	 tag;

	t = print_otag(h, TAG_HEAD, 0, NULL);

	print_man_head(m, n, h);
	print_tagq(h, t);
	t = print_otag(h, TAG_BODY, 0, NULL);

	tag.key = ATTR_CLASS;
	tag.val = "body";
	print_otag(h, TAG_DIV, 1, &tag);

	print_man_nodelist(m, n, h);

	print_tagq(h, t);
}


/* ARGSUSED */
static void
print_man_head(MAN_ARGS)
{

	print_gen_head(h);
	bufinit(h);
	buffmt(h, "%s(%d)", m->title, m->msec);

	print_otag(h, TAG_TITLE, 0, NULL);
	print_text(h, h->buf);
}


static void
print_man_nodelist(MAN_ARGS)
{

	print_man_node(m, n, h);
	if (n->next)
		print_man_nodelist(m, n->next, h);
}


static void
print_man_node(MAN_ARGS)
{
	int		 child;
	struct tag	*t;

	child = 1;
	t = SLIST_FIRST(&h->tags);

	bufinit(h);

	switch (n->type) {
	case (MAN_ROOT):
		child = man_root_pre(m, n, h);
		break;
	case (MAN_TEXT):
		print_text(h, n->string);
		break;
	default:
		if (mans[n->tok].pre)
			child = (*mans[n->tok].pre)(m, n, h);
		break;
	}

	if (child && n->child)
		print_man_nodelist(m, n->child, h);

	print_stagq(h, t);

	bufinit(h);

	switch (n->type) {
	case (MAN_ROOT):
		man_root_post(m, n, h);
		break;
	case (MAN_TEXT):
		break;
	default:
		if (mans[n->tok].post)
			(*mans[n->tok].post)(m, n, h);
		break;
	}
}


static int
a2width(const struct man_node *n)
{
	int		 i, len;
	const char	*p;

	assert(MAN_TEXT == n->type);
	assert(n->string);

	p = n->string;

	if (0 == (len = (int)strlen(p)))
		return(-1);

	for (i = 0; i < len; i++) 
		if ( ! isdigit((u_char)p[i]))
			break;

	if (i == len - 1)  {
		if ('n' == p[len - 1] || 'm' == p[len - 1])
			return(atoi(p));
	} else if (i == len)
		return(atoi(p));

	return(-1);
}


/* ARGSUSED */
static int
man_root_pre(MAN_ARGS)
{
	struct htmlpair	 tag[2];
	struct tag	*t, *tt;
	char		 b[BUFSIZ], title[BUFSIZ];

	b[0] = 0;
	if (m->vol)
		(void)strlcat(b, m->vol, BUFSIZ);

	(void)snprintf(title, BUFSIZ - 1, 
			"%s(%d)", m->title, m->msec);

	tag[0].key = ATTR_CLASS;
	tag[0].val = "header";
	tag[1].key = ATTR_STYLE;
	tag[1].val = "width: 100%;";
	t = print_otag(h, TAG_TABLE, 2, tag);
	tt = print_otag(h, TAG_TR, 0, NULL);

	tag[0].key = ATTR_STYLE;
	tag[0].val = "width: 10%;";
	print_otag(h, TAG_TD, 1, tag);
	print_text(h, title);
	print_stagq(h, tt);

	tag[0].key = ATTR_STYLE;
	tag[0].val = "width: 80%; white-space: nowrap; text-align: center;";
	print_otag(h, TAG_TD, 1, tag);
	print_text(h, b);
	print_stagq(h, tt);

	tag[0].key = ATTR_STYLE;
	tag[0].val = "width: 10%; text-align: right;";
	print_otag(h, TAG_TD, 1, tag);
	print_text(h, title);
	print_tagq(h, t);

	return(1);
}


/* ARGSUSED */
static void
man_root_post(MAN_ARGS)
{
	struct tm	 tm;
	struct htmlpair	 tag[2];
	struct tag	*t, *tt;
	char		 b[BUFSIZ];

	(void)localtime_r(&m->date, &tm);

	if (0 == strftime(b, BUFSIZ - 1, "%B %e, %Y", &tm))
		err(EXIT_FAILURE, "strftime");

	tag[0].key = ATTR_CLASS;
	tag[0].val = "footer";
	tag[1].key = ATTR_STYLE;
	tag[1].val = "width: 100%;";
	t = print_otag(h, TAG_TABLE, 2, tag);
	tt = print_otag(h, TAG_TR, 0, NULL);

	tag[0].key = ATTR_STYLE;
	tag[0].val = "width: 50%;";
	print_otag(h, TAG_TD, 1, tag);
	print_text(h, b);
	print_stagq(h, tt);

	tag[0].key = ATTR_STYLE;
	tag[0].val = "width: 50%; text-align: right;";
	print_otag(h, TAG_TD, 1, tag);
	if (m->source)
		print_text(h, m->source);
	print_tagq(h, t);
}



/* ARGSUSED */
static int
man_br_pre(MAN_ARGS)
{
	int		len;
	struct htmlpair	tag;

	switch (n->tok) {
	case (MAN_sp):
		len = n->child ? atoi(n->child->string) : 1;
		break;
	case (MAN_br):
		len = 0;
		break;
	default:
		len = 1;
		break;
	}

	buffmt(h, "height: %dem;", len);
	tag.key = ATTR_STYLE;
	tag.val = h->buf;
	print_otag(h, TAG_DIV, 1, &tag);
	return(1);
}


/* ARGSUSED */
static int
man_SH_pre(MAN_ARGS)
{
	struct htmlpair		tag[2];

	if (MAN_BODY == n->type) {
		buffmt(h, "margin-left: %dem;", INDENT);

		tag[0].key = ATTR_CLASS;
		tag[0].val = "sec-body";
		tag[1].key = ATTR_STYLE;
		tag[1].val = h->buf;

		print_otag(h, TAG_DIV, 2, tag);
		return(1);
	} else if (MAN_BLOCK == n->type) {
		tag[0].key = ATTR_CLASS;
		tag[0].val = "sec-block";

		if (n->prev && MAN_SH == n->prev->tok)
			if (NULL == n->prev->body->child) {
				print_otag(h, TAG_DIV, 1, tag);
				return(1);
			}

		bufcat(h, "margin-top: 1em;");
		if (NULL == n->next)
			bufcat(h, "margin-bottom: 1em;");

		tag[1].key = ATTR_STYLE;
		tag[1].val = h->buf;

		print_otag(h, TAG_DIV, 2, tag);
		return(1);
	}

	tag[0].key = ATTR_CLASS;
	tag[0].val = "sec-head";

	print_otag(h, TAG_DIV, 1, tag);
	return(1);
}


/* ARGSUSED */
static int
man_SS_pre(MAN_ARGS)
{
	struct htmlpair	 tag[3];
	int		 i;

	i = 0;

	if (MAN_BODY == n->type) {
		tag[i].key = ATTR_CLASS;
		tag[i++].val = "ssec-body";

		if (n->parent->next && n->child) {
			bufcat(h, "margin-bottom: 1em;");
			tag[i].key = ATTR_STYLE;
			tag[i++].val = h->buf;
		}

		print_otag(h, TAG_DIV, i, tag);
		return(1);
	} else if (MAN_BLOCK == n->type) {
		tag[i].key = ATTR_CLASS;
		tag[i++].val = "ssec-block";

		if (n->prev && MAN_SS == n->prev->tok) 
			if (n->prev->body->child) {
				bufcat(h, "margin-top: 1em;");
				tag[i].key = ATTR_STYLE;
				tag[i++].val = h->buf;
			}

		print_otag(h, TAG_DIV, i, tag);
		return(1);
	}

	buffmt(h, "margin-left: -%dem;", INDENT - HALFINDENT);

	tag[0].key = ATTR_CLASS;
	tag[0].val = "ssec-head";
	tag[1].key = ATTR_STYLE;
	tag[1].val = h->buf;

	print_otag(h, TAG_DIV, 2, tag);
	return(1);
}


/* ARGSUSED */
static int
man_PP_pre(MAN_ARGS)
{
	struct htmlpair	 tag;
	int		 i;

	if (MAN_BLOCK != n->type)
		return(1);

	i = 0;

	if (MAN_ROOT == n->parent->tok) {
		buffmt(h, "margin-left: %dem;", INDENT);
		i = 1;
	}
	if (n->next && n->next->child) {
		i = 1;
		bufcat(h, "margin-bottom: 1em;");
	}

	tag.key = ATTR_STYLE;
	tag.val = h->buf;
	print_otag(h, TAG_DIV, i, &tag);
	return(1);
}


/* ARGSUSED */
static int
man_IP_pre(MAN_ARGS)
{
	struct htmlpair	 	 tag;
	int		 	 len, ival;
	const struct man_node	*nn;

	len = 1;
	if (NULL != (nn = n->parent->head->child))
		if (NULL != (nn = nn->next)) {
			for ( ; nn->next; nn = nn->next)
				/* Do nothing. */ ;
			if ((ival = a2width(nn)) >= 0)
				len = ival;
		}

	if (MAN_BLOCK == n->type) {
		buffmt(h, "clear: both; margin-left: %dem;", len);
		tag.key = ATTR_STYLE;
		tag.val = h->buf;
		print_otag(h, TAG_DIV, 1, &tag);
		return(1);
	} else if (MAN_HEAD == n->type) {
		buffmt(h, "margin-left: -%dem; min-width: %dem;", 
				len, len - 1);
		bufcat(h, "clear: left;");
		bufcat(h, "padding-right: 1em;");
		if (n->next && n->next->child)
			bufcat(h, "float: left;");
		tag.key = ATTR_STYLE;
		tag.val = h->buf;
		print_otag(h, TAG_DIV, 1, &tag);

		/* Don't print the length value. */

		for (nn = n->child; nn->next; nn = nn->next)
			print_man_node(m, nn, h);
		return(0);
	}

	print_otag(h, TAG_DIV, 0, &tag);
	return(1);
}
