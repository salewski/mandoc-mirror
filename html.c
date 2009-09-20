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
#include <sys/queue.h>

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chars.h"
#include "mdoc.h"
#include "man.h"

#define	DOCTYPE		"-//W3C//DTD HTML 4.01//EN"
#define	DTD		"http://www.w3.org/TR/html4/strict.dtd"

#define	INDENT		 5
#define	HALFINDENT	 3
#define	PX_MULT		 8

enum	htmltag {
	TAG_HTML,
	TAG_HEAD,
	TAG_BODY,
	TAG_META,
	TAG_TITLE,
	TAG_DIV,
	TAG_H1,
	TAG_H2,
	TAG_P,
	TAG_SPAN,
	TAG_LINK,
	TAG_BR,
	TAG_A,
	TAG_TABLE,
	TAG_COL,
	TAG_TR,
	TAG_TD,
	TAG_LI,
	TAG_UL,
	TAG_OL,
	TAG_MAX
};

enum	htmlattr {
	ATTR_HTTPEQUIV,
	ATTR_CONTENT,
	ATTR_NAME,
	ATTR_REL,
	ATTR_HREF,
	ATTR_TYPE,
	ATTR_MEDIA,
	ATTR_CLASS,
	ATTR_STYLE,
	ATTR_WIDTH,
	ATTR_VALIGN,
	ATTR_MAX
};

struct	htmldata {
	char		 *name;
	int		  flags;
#define	HTML_CLRLINE	 (1 << 0)
#define	HTML_NOSTACK	 (1 << 1)
};

static	const struct htmldata htmltags[TAG_MAX] = {
	{"html",	HTML_CLRLINE}, /* TAG_HTML */
	{"head",	HTML_CLRLINE}, /* TAG_HEAD */
	{"body",	HTML_CLRLINE}, /* TAG_BODY */
	{"meta",	HTML_CLRLINE | HTML_NOSTACK}, /* TAG_META */
	{"title",	HTML_CLRLINE}, /* TAG_TITLE */
	{"div",		HTML_CLRLINE}, /* TAG_DIV */
	{"h1",		0}, /* TAG_H1 */
	{"h2",		0}, /* TAG_H2 */
	{"p",		HTML_CLRLINE}, /* TAG_P */
	{"span",	0}, /* TAG_SPAN */
	{"link",	HTML_CLRLINE | HTML_NOSTACK}, /* TAG_LINK */
	{"br",		HTML_CLRLINE | HTML_NOSTACK}, /* TAG_LINK */
	{"a",		0}, /* TAG_A */
	{"table",	HTML_CLRLINE}, /* TAG_TABLE */
	{"col",		HTML_CLRLINE | HTML_NOSTACK}, /* TAG_COL */
	{"tr",		HTML_CLRLINE}, /* TAG_TR */
	{"td",		HTML_CLRLINE}, /* TAG_TD */
	{"li",		HTML_CLRLINE}, /* TAG_LI */
	{"ul",		HTML_CLRLINE}, /* TAG_UL */
	{"ol",		HTML_CLRLINE}, /* TAG_OL */
};

static	const char	 *const htmlattrs[ATTR_MAX] = {
	"http-equiv",
	"content",
	"name",
	"rel",
	"href",
	"type",
	"media",
	"class",
	"style",
	"width",
	"valign",
};

struct	htmlpair {
	enum htmlattr	  key;
	char		 *val;
};

struct	tag {
	enum htmltag	  tag;
	SLIST_ENTRY(tag)  entry;
};

struct	ord {
	int		  pos;
	const void	 *cookie;
	SLIST_ENTRY(ord)  entry;
};

SLIST_HEAD(tagq, tag);
SLIST_HEAD(ordq, ord);

struct	html {
	int		  flags;
#define	HTML_NOSPACE	 (1 << 0)
#define	HTML_NEWLINE	 (1 << 1)
	struct tagq	  tags;
	struct ordq	  ords;
	void		 *symtab;
};

#define	MDOC_ARGS	  const struct mdoc_meta *m, \
			  const struct mdoc_node *n, \
			  struct html *h
#define	MAN_ARGS	  const struct man_meta *m, \
			  const struct man_node *n, \
			  struct html *h
struct	htmlmdoc {
	int		(*pre)(MDOC_ARGS);
	void		(*post)(MDOC_ARGS);
};

static	void		  print_gen_doctype(struct html *);
static	void		  print_gen_head(struct html *);
static	void		  print_mdoc(MDOC_ARGS);
static	void		  print_mdoc_head(MDOC_ARGS);
static	void		  print_mdoc_title(MDOC_ARGS);
static	void		  print_mdoc_node(MDOC_ARGS);
static	void		  print_mdoc_nodelist(MDOC_ARGS);
static	void		  print_man(MAN_ARGS);
static	void		  print_man_head(MAN_ARGS);
static	void		  print_man_body(MAN_ARGS);
static	struct tag	 *print_otag(struct html *, enum htmltag, 
				int, const struct htmlpair *);
static	void		  print_tagq(struct html *, const struct tag *);
static	void		  print_stagq(struct html *, const struct tag *);
static	void		  print_ctag(struct html *, enum htmltag);
static	void		  print_encode(struct html *, const char *);
static	void		  print_escape(struct html *, const char **);
static	void		  print_text(struct html *, const char *);
static	void		  print_res(struct html *, const char *, int);
static	void		  print_spec(struct html *, const char *, int);

static	int		  a2width(const char *);
static	int		  a2offs(const char *);
static	int		  a2list(const struct mdoc_node *);

static	void		  mdoc_root_post(MDOC_ARGS);
static	int		  mdoc_root_pre(MDOC_ARGS);
static	int		  mdoc_tbl_pre(MDOC_ARGS, int);
static	int		  mdoc_tbl_block_pre(MDOC_ARGS, int, int, int, int);
static	int		  mdoc_tbl_body_pre(MDOC_ARGS, int, int);
static	int		  mdoc_tbl_head_pre(MDOC_ARGS, int, int);

static	void		  mdoc_aq_post(MDOC_ARGS);
static	int		  mdoc_aq_pre(MDOC_ARGS);
static	int		  mdoc_ar_pre(MDOC_ARGS);
static	int		  mdoc_bd_pre(MDOC_ARGS);
static	void		  mdoc_bl_post(MDOC_ARGS);
static	int		  mdoc_bl_pre(MDOC_ARGS);
static	int		  mdoc_d1_pre(MDOC_ARGS);
static	void		  mdoc_dq_post(MDOC_ARGS);
static	int		  mdoc_dq_pre(MDOC_ARGS);
static	int		  mdoc_fl_pre(MDOC_ARGS);
static	int		  mdoc_em_pre(MDOC_ARGS);
static	int		  mdoc_ex_pre(MDOC_ARGS);
static	int		  mdoc_it_pre(MDOC_ARGS);
static	int		  mdoc_nd_pre(MDOC_ARGS);
static	int		  mdoc_nm_pre(MDOC_ARGS);
static	int		  mdoc_ns_pre(MDOC_ARGS);
static	void		  mdoc_op_post(MDOC_ARGS);
static	int		  mdoc_op_pre(MDOC_ARGS);
static	int		  mdoc_pa_pre(MDOC_ARGS);
static	int		  mdoc_pp_pre(MDOC_ARGS);
static	void		  mdoc_pq_post(MDOC_ARGS);
static	int		  mdoc_pq_pre(MDOC_ARGS);
static	void		  mdoc_qq_post(MDOC_ARGS);
static	int		  mdoc_qq_pre(MDOC_ARGS);
static	int		  mdoc_sh_pre(MDOC_ARGS);
static	void		  mdoc_sq_post(MDOC_ARGS);
static	int		  mdoc_sq_pre(MDOC_ARGS);
static	int		  mdoc_ss_pre(MDOC_ARGS);
static	int		  mdoc_sx_pre(MDOC_ARGS);
static	int		  mdoc_xr_pre(MDOC_ARGS);
static	int		  mdoc_xx_pre(MDOC_ARGS);

#ifdef __linux__
extern	size_t	  	  strlcpy(char *, const char *, size_t);
extern	size_t	  	  strlcat(char *, const char *, size_t);
#endif

static	const struct htmlmdoc mdocs[MDOC_MAX] = {
	{mdoc_pp_pre, NULL}, /* Ap */
	{NULL, NULL}, /* Dd */
	{NULL, NULL}, /* Dt */
	{NULL, NULL}, /* Os */
	{mdoc_sh_pre, NULL }, /* Sh */
	{mdoc_ss_pre, NULL }, /* Ss */ 
	{mdoc_pp_pre, NULL}, /* Pp */ 
	{mdoc_d1_pre, NULL}, /* D1 */
	{mdoc_d1_pre, NULL}, /* Dl */
	{mdoc_bd_pre, NULL}, /* Bd */
	{NULL, NULL}, /* Ed */
	{mdoc_bl_pre, mdoc_bl_post}, /* Bl */
	{NULL, NULL}, /* El */
	{mdoc_it_pre, NULL}, /* It */
	{NULL, NULL}, /* Ad */ 
	{NULL, NULL}, /* An */
	{mdoc_ar_pre, NULL}, /* Ar */
	{NULL, NULL}, /* Cd */
	{NULL, NULL}, /* Cm */
	{NULL, NULL}, /* Dv */ 
	{NULL, NULL}, /* Er */ 
	{NULL, NULL}, /* Ev */ 
	{mdoc_ex_pre, NULL}, /* Ex */
	{NULL, NULL}, /* Fa */ 
	{NULL, NULL}, /* Fd */ 
	{mdoc_fl_pre, NULL}, /* Fl */
	{NULL, NULL}, /* Fn */ 
	{NULL, NULL}, /* Ft */ 
	{NULL, NULL}, /* Ic */ 
	{NULL, NULL}, /* In */ 
	{NULL, NULL}, /* Li */
	{mdoc_nd_pre, NULL}, /* Nd */ 
	{mdoc_nm_pre, NULL}, /* Nm */ 
	{mdoc_op_pre, mdoc_op_post}, /* Op */
	{NULL, NULL}, /* Ot */
	{mdoc_pa_pre, NULL}, /* Pa */
	{NULL, NULL}, /* Rv */
	{NULL, NULL}, /* St */ 
	{NULL, NULL}, /* Va */
	{NULL, NULL}, /* Vt */ 
	{mdoc_xr_pre, NULL}, /* Xr */
	{NULL, NULL}, /* %A */
	{NULL, NULL}, /* %B */
	{NULL, NULL}, /* %D */
	{NULL, NULL}, /* %I */
	{NULL, NULL}, /* %J */
	{NULL, NULL}, /* %N */
	{NULL, NULL}, /* %O */
	{NULL, NULL}, /* %P */
	{NULL, NULL}, /* %R */
	{NULL, NULL}, /* %T */
	{NULL, NULL}, /* %V */
	{NULL, NULL}, /* Ac */
	{mdoc_aq_pre, mdoc_aq_post}, /* Ao */
	{mdoc_aq_pre, mdoc_aq_post}, /* Aq */
	{NULL, NULL}, /* At */
	{NULL, NULL}, /* Bc */
	{NULL, NULL}, /* Bf */ 
	{NULL, NULL}, /* Bo */
	{NULL, NULL}, /* Bq */
	{mdoc_xx_pre, NULL}, /* Bsx */
	{NULL, NULL}, /* Bx */
	{NULL, NULL}, /* Db */
	{NULL, NULL}, /* Dc */
	{NULL, NULL}, /* Do */
	{mdoc_dq_pre, mdoc_dq_post}, /* Dq */
	{NULL, NULL}, /* Ec */
	{NULL, NULL}, /* Ef */
	{mdoc_em_pre, NULL}, /* Em */ 
	{NULL, NULL}, /* Eo */
	{mdoc_xx_pre, NULL}, /* Fx */
	{NULL, NULL}, /* Ms */
	{NULL, NULL}, /* No */
	{mdoc_ns_pre, NULL}, /* Ns */
	{mdoc_xx_pre, NULL}, /* Nx */
	{mdoc_xx_pre, NULL}, /* Ox */
	{NULL, NULL}, /* Pc */
	{NULL, NULL}, /* Pf */
	{mdoc_pq_pre, mdoc_pq_post}, /* Po */
	{mdoc_pq_pre, mdoc_pq_post}, /* Pq */
	{NULL, NULL}, /* Qc */
	{NULL, NULL}, /* Ql */
	{mdoc_qq_pre, mdoc_qq_post}, /* Qo */
	{mdoc_qq_pre, mdoc_qq_post}, /* Qq */
	{NULL, NULL}, /* Re */
	{NULL, NULL}, /* Rs */
	{NULL, NULL}, /* Sc */
	{mdoc_sq_pre, mdoc_sq_post}, /* So */
	{mdoc_sq_pre, mdoc_sq_post}, /* Sq */
	{NULL, NULL}, /* Sm */
	{mdoc_sx_pre, NULL}, /* Sx */
	{NULL, NULL}, /* Sy */
	{NULL, NULL}, /* Tn */
	{mdoc_xx_pre, NULL}, /* Ux */
	{NULL, NULL}, /* Xc */
	{NULL, NULL}, /* Xo */
	{NULL, NULL}, /* Fo */ 
	{NULL, NULL}, /* Fc */ 
	{NULL, NULL}, /* Oo */
	{NULL, NULL}, /* Oc */
	{NULL, NULL}, /* Bk */
	{NULL, NULL}, /* Ek */
	{NULL, NULL}, /* Bt */
	{NULL, NULL}, /* Hf */
	{NULL, NULL}, /* Fr */
	{NULL, NULL}, /* Ud */
	{NULL, NULL}, /* Lb */
	{NULL, NULL}, /* Lp */ 
	{NULL, NULL}, /* Lk */ 
	{NULL, NULL}, /* Mt */ 
	{NULL, NULL}, /* Brq */ 
	{NULL, NULL}, /* Bro */ 
	{NULL, NULL}, /* Brc */ 
	{NULL, NULL}, /* %C */ 
	{NULL, NULL}, /* Es */ 
	{NULL, NULL}, /* En */ 
	{mdoc_xx_pre, NULL}, /* Dx */ 
	{NULL, NULL}, /* %Q */ 
	{NULL, NULL}, /* br */
	{NULL, NULL}, /* sp */ 
};

static	char		  buf[BUFSIZ]; /* XXX */

#define	bufcat(x)	  (void)strlcat(buf, (x), BUFSIZ) 
#define	bufinit()	  buf[0] = 0
#define	buffmt(...)	  (void)snprintf(buf, BUFSIZ - 1, __VA_ARGS__)

void
html_mdoc(void *arg, const struct mdoc *m)
{
	struct html 	*h;
	struct tag	*t;

	h = (struct html *)arg;

	print_gen_doctype(h);
	t = print_otag(h, TAG_HTML, 0, NULL);
	print_mdoc(mdoc_meta(m), mdoc_node(m), h);
	print_tagq(h, t);

	printf("\n");
}


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


void *
html_alloc(void)
{
	struct html	*h;

	if (NULL == (h = calloc(1, sizeof(struct html))))
		return(NULL);

	SLIST_INIT(&h->tags);
	SLIST_INIT(&h->ords);

	if (NULL == (h->symtab = chars_init(CHARS_HTML))) {
		free(h);
		return(NULL);
	}
	return(h);
}


void
html_free(void *p)
{
	struct tag	*tag;
	struct ord	*ord;
	struct html	*h;

	h = (struct html *)p;

	while ( ! SLIST_EMPTY(&h->ords)) {
		ord = SLIST_FIRST(&h->ords);
		SLIST_REMOVE_HEAD(&h->ords, entry);
		free(ord);
	}

	while ( ! SLIST_EMPTY(&h->tags)) {
		tag = SLIST_FIRST(&h->tags);
		SLIST_REMOVE_HEAD(&h->tags, entry);
		free(tag);
	}
	
	if (h->symtab)
		chars_free(h->symtab);
	free(h);
}


static void
print_mdoc(MDOC_ARGS)
{
	struct tag	*t;

	t = print_otag(h, TAG_HEAD, 0, NULL);
	print_mdoc_head(m, n, h);
	print_tagq(h, t);

	t = print_otag(h, TAG_BODY, 0, NULL);
	print_mdoc_title(m, n, h);
	print_mdoc_nodelist(m, n, h);
	print_tagq(h, t);
}


static void
print_gen_head(struct html *h)
{
	struct htmlpair	 meta0[2];
	struct htmlpair	 meta1[2];
	struct htmlpair	 link[4];

	meta0[0].key = ATTR_HTTPEQUIV;
	meta0[0].val = "Content-Type";
	meta0[1].key = ATTR_CONTENT;
	meta0[1].val = "text/html; charset=utf-8";

	meta1[0].key = ATTR_NAME;
	meta1[0].val = "resource-type";
	meta1[1].key = ATTR_CONTENT;
	meta1[1].val = "document";

	link[0].key = ATTR_REL;
	link[0].val = "stylesheet";
	link[1].key = ATTR_HREF;
	link[1].val = "style.css"; /* XXX */
	link[2].key = ATTR_TYPE;
	link[2].val = "text/css";
	link[3].key = ATTR_MEDIA;
	link[3].val = "all";

	print_otag(h, TAG_META, 2, meta0);
	print_otag(h, TAG_META, 2, meta1);
	print_otag(h, TAG_LINK, 4, link);
}


/* ARGSUSED */
static void
print_mdoc_head(MDOC_ARGS)
{

	print_gen_head(h);
	print_otag(h, TAG_TITLE, 0, NULL);
	print_encode(h, m->title);
}


/* ARGSUSED */
static void
print_mdoc_title(MDOC_ARGS)
{

	/* TODO */
}


static void
print_mdoc_nodelist(MDOC_ARGS)
{

	print_mdoc_node(m, n, h);
	if (n->next)
		print_mdoc_nodelist(m, n->next, h);
}


static void
print_mdoc_node(MDOC_ARGS)
{
	int		 child;
	struct tag	*t;

	child = 1;
	t = SLIST_FIRST(&h->tags);

	bufinit();

	switch (n->type) {
	case (MDOC_ROOT):
		child = mdoc_root_pre(m, n, h);
		break;
	case (MDOC_TEXT):
		print_text(h, n->string);
		break;
	default:
		if (mdocs[n->tok].pre)
			child = (*mdocs[n->tok].pre)(m, n, h);
		break;
	}

	if (child && n->child)
		print_mdoc_nodelist(m, n->child, h);

	print_stagq(h, t);

	bufinit();

	switch (n->type) {
	case (MDOC_ROOT):
		mdoc_root_post(m, n, h);
		break;
	case (MDOC_TEXT):
		break;
	default:
		if (mdocs[n->tok].post)
			(*mdocs[n->tok].post)(m, n, h);
		break;
	}
}


static void
print_man(MAN_ARGS)
{
	struct tag	*t;

	t = print_otag(h, TAG_HEAD, 0, NULL);
	print_man_head(m, n, h);
	print_tagq(h, t);

	t = print_otag(h, TAG_BODY, 0, NULL);
	print_man_body(m, n, h);
	print_tagq(h, t);
}


/* ARGSUSED */
static void
print_man_head(MAN_ARGS)
{

	print_gen_head(h);
	print_otag(h, TAG_TITLE, 0, NULL);
	print_encode(h, m->title);
}


/* ARGSUSED */
static void
print_man_body(MAN_ARGS)
{

	/* TODO */
}


static void
print_spec(struct html *h, const char *p, int len)
{
	const char	*rhs;
	int		 i;
	size_t		 sz;

	rhs = chars_a2ascii(h->symtab, p, (size_t)len, &sz);

	if (NULL == rhs) 
		return;
	for (i = 0; i < (int)sz; i++) 
		putchar(rhs[i]);
}


static void
print_res(struct html *h, const char *p, int len)
{
	const char	*rhs;
	int		 i;
	size_t		 sz;

	rhs = chars_a2res(h->symtab, p, (size_t)len, &sz);

	if (NULL == rhs)
		return;
	for (i = 0; i < (int)sz; i++) 
		putchar(rhs[i]);
}


static void
print_escape(struct html *h, const char **p)
{
	int		 j, type;
	const char	*wp;

	wp = *p;
	type = 1;

	if (0 == *(++wp)) {
		*p = wp;
		return;
	}

	if ('(' == *wp) {
		wp++;
		if (0 == *wp || 0 == *(wp + 1)) {
			*p = 0 == *wp ? wp : wp + 1;
			return;
		}

		print_spec(h, wp, 2);
		*p = ++wp;
		return;

	} else if ('*' == *wp) {
		if (0 == *(++wp)) {
			*p = wp;
			return;
		}

		switch (*wp) {
		case ('('):
			wp++;
			if (0 == *wp || 0 == *(wp + 1)) {
				*p = 0 == *wp ? wp : wp + 1;
				return;
			}

			print_res(h, wp, 2);
			*p = ++wp;
			return;
		case ('['):
			type = 0;
			break;
		default:
			print_res(h, wp, 1);
			*p = wp;
			return;
		}
	
	} else if ('f' == *wp) {
		if (0 == *(++wp)) {
			*p = wp;
			return;
		}

		switch (*wp) {
		case ('B'):
			/* TODO */
			break;
		case ('I'):
			/* TODO */
			break;
		case ('P'):
			/* FALLTHROUGH */
		case ('R'):
			/* TODO */
			break;
		default:
			break;
		}

		*p = wp;
		return;

	} else if ('[' != *wp) {
		print_spec(h, wp, 1);
		*p = wp;
		return;
	}

	wp++;
	for (j = 0; *wp && ']' != *wp; wp++, j++)
		/* Loop... */ ;

	if (0 == *wp) {
		*p = wp;
		return;
	}

	if (type)
		print_spec(h, wp - j, j);
	else
		print_res(h, wp - j, j);

	*p = wp;
}


static void
print_encode(struct html *h, const char *p)
{

	for (; *p; p++) {
		if ('\\' == *p) {
			print_escape(h, &p);
			continue;
		}
		switch (*p) {
		case ('<'):
			printf("&lt;");
			break;
		case ('>'):
			printf("&gt;");
			break;
		case ('&'):
			printf("&amp;");
			break;
		default:
			putchar(*p);
			break;
		}
	}
}


static struct tag *
print_otag(struct html *h, enum htmltag tag, 
		int sz, const struct htmlpair *p)
{
	int		 i;
	struct tag	*t;

	if ( ! (HTML_NOSTACK & htmltags[tag].flags)) {
		if (NULL == (t = malloc(sizeof(struct tag))))
			err(EXIT_FAILURE, "malloc");
		t->tag = tag;
		SLIST_INSERT_HEAD(&h->tags, t, entry);
	} else
		t = NULL;

	if ( ! (HTML_NOSPACE & h->flags))
		if ( ! (HTML_CLRLINE & htmltags[tag].flags))
			printf(" ");

	printf("<%s", htmltags[tag].name);
	for (i = 0; i < sz; i++) {
		printf(" %s=\"", htmlattrs[p[i].key]);
		assert(p->val);
		print_encode(h, p[i].val);
		printf("\"");
	}
	printf(">");

	h->flags |= HTML_NOSPACE;
	if (HTML_CLRLINE & htmltags[tag].flags)
		h->flags |= HTML_NEWLINE;
	else
		h->flags &= ~HTML_NEWLINE;

	return(t);
}


/* ARGSUSED */
static void
print_ctag(struct html *h, enum htmltag tag)
{
	
	printf("</%s>", htmltags[tag].name);
	if (HTML_CLRLINE & htmltags[tag].flags)
		h->flags |= HTML_NOSPACE;
	if (HTML_CLRLINE & htmltags[tag].flags)
		h->flags |= HTML_NEWLINE;
	else
		h->flags &= ~HTML_NEWLINE;
}


/* ARGSUSED */
static void
print_gen_doctype(struct html *h)
{
	
	printf("<!DOCTYPE HTML PUBLIC \"%s\" \"%s\">\n", DOCTYPE, DTD);
}


static void
print_text(struct html *h, const char *p)
{

	if (*p && 0 == *(p + 1))
		switch (*p) {
		case('.'):
			/* FALLTHROUGH */
		case(','):
			/* FALLTHROUGH */
		case(';'):
			/* FALLTHROUGH */
		case(':'):
			/* FALLTHROUGH */
		case('?'):
			/* FALLTHROUGH */
		case('!'):
			/* FALLTHROUGH */
		case(')'):
			/* FALLTHROUGH */
		case(']'):
			/* FALLTHROUGH */
		case('}'):
			h->flags |= HTML_NOSPACE;
			break;
		default:
			break;
		}

	if ( ! (h->flags & HTML_NOSPACE))
		printf(" ");

	h->flags &= ~HTML_NOSPACE;
	h->flags &= ~HTML_NEWLINE;

	if (p)
		print_encode(h, p);

	if (*p && 0 == *(p + 1))
		switch (*p) {
		case('('):
			/* FALLTHROUGH */
		case('['):
			/* FALLTHROUGH */
		case('{'):
			h->flags |= HTML_NOSPACE;
			break;
		default:
			break;
		}
}


static void
print_tagq(struct html *h, const struct tag *until)
{
	struct tag	*tag;

	while ( ! SLIST_EMPTY(&h->tags)) {
		tag = SLIST_FIRST(&h->tags);
		print_ctag(h, tag->tag);
		SLIST_REMOVE_HEAD(&h->tags, entry);
		free(tag);
		if (until && tag == until)
			return;
	}
}


static void
print_stagq(struct html *h, const struct tag *suntil)
{
	struct tag	*tag;

	while ( ! SLIST_EMPTY(&h->tags)) {
		tag = SLIST_FIRST(&h->tags);
		if (suntil && tag == suntil)
			return;
		print_ctag(h, tag->tag);
		SLIST_REMOVE_HEAD(&h->tags, entry);
		free(tag);
	}
}


/* FIXME: put in utility file for front-ends. */
static int
a2offs(const char *p)
{
	int		 len, i;

	if (0 == strcmp(p, "left"))
		return(0);
	if (0 == strcmp(p, "indent"))
		return(INDENT + 1);
	if (0 == strcmp(p, "indent-two"))
		return((INDENT + 1) * 2);

	if (0 == (len = (int)strlen(p)))
		return(0);

	for (i = 0; i < len - 1; i++) 
		if ( ! isdigit((u_char)p[i]))
			break;

	if (i == len - 1) 
		if ('n' == p[len - 1] || 'm' == p[len - 1])
			return(atoi(p));

	return(len);
}


/* FIXME: put in utility file for front-ends. */
static int
a2list(const struct mdoc_node *bl)
{
	int		 i;

	assert(MDOC_BLOCK == bl->type && MDOC_Bl == bl->tok);
	assert(bl->args);

	for (i = 0; i < (int)bl->args->argc; i++) 
		switch (bl->args->argv[i].arg) {
		case (MDOC_Enum):
			/* FALLTHROUGH */
		case (MDOC_Dash):
			/* FALLTHROUGH */
		case (MDOC_Hyphen):
			/* FALLTHROUGH */
		case (MDOC_Bullet):
			/* FALLTHROUGH */
		case (MDOC_Tag):
			/* FALLTHROUGH */
		case (MDOC_Hang):
			/* FALLTHROUGH */
		case (MDOC_Inset):
			/* FALLTHROUGH */
		case (MDOC_Diag):
			/* FALLTHROUGH */
		case (MDOC_Item):
			/* FALLTHROUGH */
		case (MDOC_Column):
			/* FALLTHROUGH */
		case (MDOC_Ohang):
			return(bl->args->argv[i].arg);
		default:
			break;
		}

	abort();
	/* NOTREACHED */
}


/* FIXME: put in utility file for front-ends. */
static int
a2width(const char *p)
{
	int		 i, len;

	if (0 == (len = (int)strlen(p)))
		return(0);
	for (i = 0; i < len - 1; i++) 
		if ( ! isdigit((u_char)p[i]))
			break;

	if (i == len - 1) 
		if ('n' == p[len - 1] || 'm' == p[len - 1])
			return(atoi(p) + 2);

	return(len + 2);
}


/* FIXME: parts should be in a utility file for front-ends. */
/* ARGSUSED */
static void
mdoc_root_post(MDOC_ARGS)
{
	struct tm	*tm;
	struct htmlpair	 tag[2];
	struct tag	*t;
	char		 b[BUFSIZ], os[BUFSIZ];

	tm = localtime(&m->date);

	if (0 == strftime(b, BUFSIZ - 1, "%B %e, %Y", tm))
		err(EXIT_FAILURE, "strftime");

	(void)strlcpy(os, m->os, BUFSIZ);

	tag[0].key = ATTR_STYLE;
	tag[0].val = "width: 100%; margin-top: 1em;";
	tag[1].key = ATTR_CLASS;
	tag[1].val = "foot";

	t = print_otag(h, TAG_DIV, 2, tag);

	bufinit();
	bufcat("width: 50%;");
	bufcat("text-align: left;");
	bufcat("float: left;");
	tag[0].key = ATTR_STYLE;
	tag[0].val = buf;
	print_otag(h, TAG_SPAN, 1, tag);
	print_text(h, b);
	print_stagq(h, t);

	bufinit();
	bufcat("width: 50%;");
	bufcat("text-align: right;");
	bufcat("float: left;");
	tag[0].key = ATTR_STYLE;
	tag[0].val = buf;
	print_otag(h, TAG_SPAN, 1, tag);
	print_text(h, os);
	print_tagq(h, t);

}


/* FIXME: parts should be in a utility file for front-ends. */
/* ARGSUSED */
static int
mdoc_root_pre(MDOC_ARGS)
{
	struct htmlpair	 tag[2];
	struct tag	*t, *tt;
	char		 b[BUFSIZ], title[BUFSIZ];

	assert(m->vol);
	(void)strlcpy(b, m->vol, BUFSIZ);

	if (m->arch) {
		(void)strlcat(b, " (", BUFSIZ);
		(void)strlcat(b, m->arch, BUFSIZ);
		(void)strlcat(b, ")", BUFSIZ);
	}

	(void)snprintf(title, BUFSIZ - 1, "%s(%d)", m->title, m->msec);

	tag[0].key = ATTR_CLASS;
	tag[0].val = "body";

	t = print_otag(h, TAG_DIV, 1, tag);

	tag[0].key = ATTR_CLASS;
	tag[0].val = "head";
	tag[1].key = ATTR_STYLE;
	tag[1].val = "margin-bottom: 1em; clear: both;";

	tt = print_otag(h, TAG_DIV, 2, tag);

	bufinit();
	bufcat("width: 30%;");
	bufcat("text-align: left;");
	bufcat("float: left;");
	tag[0].key = ATTR_STYLE;
	tag[0].val = buf;
	print_otag(h, TAG_SPAN, 1, tag);
	print_text(h, b);
	print_stagq(h, tt);

	bufinit();
	bufcat("width: 30%;");
	bufcat("text-align: center;");
	bufcat("float: left;");
	tag[0].key = ATTR_STYLE;
	tag[0].val = buf;
	print_otag(h, TAG_SPAN, 1, tag);
	print_text(h, title);
	print_stagq(h, tt);

	bufinit();
	bufcat("width: 30%;");
	bufcat("text-align: right;");
	bufcat("float: left;");
	tag[0].key = ATTR_STYLE;
	tag[0].val = buf;
	print_otag(h, TAG_SPAN, 1, tag);
	print_text(h, b);

	print_stagq(h, t);

	return(1);
}


/* ARGSUSED */
static int
mdoc_sh_pre(MDOC_ARGS)
{
	struct htmlpair	tag[2];

	if (MDOC_HEAD == n->type) {
		tag[0].key = ATTR_CLASS;
		tag[0].val = "sec-head";
		print_otag(h, TAG_DIV, 1, tag);
		print_otag(h, TAG_SPAN, 1, tag);
		return(1);
	} else if (MDOC_BLOCK == n->type) {
		tag[0].key = ATTR_CLASS;
		tag[0].val = "sec-block";

		if (n->prev && NULL == n->prev->body->child) {
			print_otag(h, TAG_DIV, 1, tag);
			return(1);
		}

		tag[1].key = ATTR_STYLE;
		tag[1].val = "margin-top: 1em;";

		print_otag(h, TAG_DIV, 2, tag);
		return(1);
	}

	buffmt("margin-left: %dem;", INDENT);
	
	tag[0].key = ATTR_CLASS;
	tag[0].val = "sec-body";
	tag[1].key = ATTR_STYLE;
	tag[1].val = buf;

	print_otag(h, TAG_DIV, 2, tag);
	return(1);
}


/* ARGSUSED */
static int
mdoc_ss_pre(MDOC_ARGS)
{
	struct htmlpair	 tag[2];
	int		 i;

	i = 0;

	if (MDOC_BODY == n->type) {
		tag[i].key = ATTR_CLASS;
		tag[i++].val = "ssec-body";
		if (n->parent->next && n->child) {
			bufcat("margin-bottom: 1em;");
			tag[i].key = ATTR_STYLE;
			tag[i++].val = buf;
		}
		print_otag(h, TAG_DIV, i, tag);
		return(1);
	} else if (MDOC_BLOCK == n->type) {
		tag[i].key = ATTR_CLASS;
		tag[i++].val = "ssec-block";
		if (n->prev) {
			bufcat("margin-top: 1em;");
			tag[i].key = ATTR_STYLE;
			tag[i++].val = buf;
		}
		print_otag(h, TAG_DIV, i, tag);
		return(1);
	}

	buffmt("margin-left: -%dem;", INDENT - HALFINDENT);

	tag[0].key = ATTR_CLASS;
	tag[0].val = "ssec-head";
	tag[1].key = ATTR_STYLE;
	tag[1].val = buf;

	print_otag(h, TAG_DIV, 2, tag);
	print_otag(h, TAG_SPAN, 1, tag);
	return(1);
}


/* ARGSUSED */
static int
mdoc_fl_pre(MDOC_ARGS)
{
	struct htmlpair	 tag;

	tag.key = ATTR_CLASS;
	tag.val = "flag";

	print_otag(h, TAG_SPAN, 1, &tag);
	print_text(h, "\\-");
	h->flags |= HTML_NOSPACE;
	return(1);
}


/* ARGSUSED */
static int
mdoc_pp_pre(MDOC_ARGS)
{
	struct htmlpair	tag;

	bufcat("clear: both;");
	bufcat("height: 1em;");

	tag.key = ATTR_STYLE;
	tag.val = buf;

	print_otag(h, TAG_DIV, 1, &tag);
	return(0);
}


/* ARGSUSED */
static int
mdoc_nd_pre(MDOC_ARGS)
{
	struct htmlpair	 tag;

	if (MDOC_BODY != n->type)
		return(1);

	/* XXX - this can contain block elements! */
	print_text(h, "\\(em");
	tag.key = ATTR_CLASS;
	tag.val = "desc-body";
	print_otag(h, TAG_SPAN, 1, &tag);
	return(1);
}


/* ARGSUSED */
static int
mdoc_op_pre(MDOC_ARGS)
{
	struct htmlpair	 tag;

	if (MDOC_BODY != n->type)
		return(1);

	/* XXX - this can contain block elements! */
	print_text(h, "\\(lB");
	tag.key = ATTR_CLASS;
	tag.val = "opt";
	print_otag(h, TAG_SPAN, 1, &tag);
	return(1);
}


/* ARGSUSED */
static void
mdoc_op_post(MDOC_ARGS)
{

	if (MDOC_BODY != n->type) 
		return;
	h->flags |= HTML_NOSPACE;
	print_text(h, "\\(rB");
}


static int
mdoc_nm_pre(MDOC_ARGS)
{
	struct htmlpair	tag;

	if ( ! (HTML_NEWLINE & h->flags))
		if (SEC_SYNOPSIS == n->sec) {
			tag.key = ATTR_STYLE;
			tag.val = "clear: both;";
			print_otag(h, TAG_BR, 1, &tag);
		}

	tag.key = ATTR_CLASS;
	tag.val = "name";

	print_otag(h, TAG_SPAN, 1, &tag);
	if (NULL == n->child)
		print_text(h, m->name);

	return(1);
}


/* ARGSUSED */
static int
mdoc_xr_pre(MDOC_ARGS)
{
	struct htmlpair	tag[2];

	tag[0].key = ATTR_CLASS;
	tag[0].val = "link-man";
	tag[1].key = ATTR_HREF;
	tag[1].val = "#"; /* TODO */

	print_otag(h, TAG_A, 2, tag);

	n = n->child;
	print_text(h, n->string);
	if (NULL == (n = n->next))
		return(0);

	h->flags |= HTML_NOSPACE;
	print_text(h, "(");
	h->flags |= HTML_NOSPACE;
	print_text(h, n->string);
	h->flags |= HTML_NOSPACE;
	print_text(h, ")");

	return(0);
}


/* ARGSUSED */
static int
mdoc_ns_pre(MDOC_ARGS)
{

	h->flags |= HTML_NOSPACE;
	return(1);
}


/* ARGSUSED */
static int
mdoc_ar_pre(MDOC_ARGS)
{
	struct htmlpair tag;

	tag.key = ATTR_CLASS;
	tag.val = "arg";

	print_otag(h, TAG_SPAN, 1, &tag);
	return(1);
}


/* ARGSUSED */
static int
mdoc_xx_pre(MDOC_ARGS)
{
	const char	*pp;
	struct htmlpair	 tag;

	switch (n->tok) {
	case (MDOC_Bsx):
		pp = "BSDI BSD/OS";
		break;
	case (MDOC_Dx):
		pp = "DragonFlyBSD";
		break;
	case (MDOC_Fx):
		pp = "FreeBSD";
		break;
	case (MDOC_Nx):
		pp = "NetBSD";
		break;
	case (MDOC_Ox):
		pp = "OpenBSD";
		break;
	case (MDOC_Ux):
		pp = "UNIX";
		break;
	default:
		return(1);
	}

	tag.key = ATTR_CLASS;
	tag.val = "unix";

	print_otag(h, TAG_SPAN, 1, &tag);
	print_text(h, pp);
	return(1);
}


/* ARGSUSED */
static int
mdoc_tbl_block_pre(MDOC_ARGS, int t, int w, int o, int c)
{
	struct htmlpair	 tag;

	switch (t) {
	case (MDOC_Column):
		/* FALLTHROUGH */
	case (MDOC_Item):
		/* FALLTHROUGH */
	case (MDOC_Ohang):
		buffmt("margin-left: %dem; clear: both;", o);
		break;
	default:
		buffmt("margin-left: %dem; clear: both;", w + o);
		break;
	}

	if ( ! c && n->prev && n->prev->body->child)
		bufcat("padding-top: 1em;");

	tag.key = ATTR_STYLE;
	tag.val = buf;
	print_otag(h, TAG_DIV, 1, &tag);
	return(1);
}


/* ARGSUSED */
static int
mdoc_tbl_body_pre(MDOC_ARGS, int t, int w)
{

	print_otag(h, TAG_DIV, 0, NULL);
	return(1);
}


/* ARGSUSED */
static int
mdoc_tbl_head_pre(MDOC_ARGS, int t, int w)
{
	struct htmlpair	 tag;
	struct ord	*ord;
	char		 nbuf[BUFSIZ];

	switch (t) {
	case (MDOC_Item):
		/* FALLTHROUGH */
	case (MDOC_Ohang):
		print_otag(h, TAG_DIV, 0, NULL);
		break;
	case (MDOC_Column):
		buffmt("min-width: %dem;", w);
		bufcat("clear: none;");
		if (n->next && MDOC_HEAD == n->next->type)
			bufcat("float: left;");
		tag.key = ATTR_STYLE;
		tag.val = buf;
		print_otag(h, TAG_DIV, 1, &tag);
		break;
	default:
		buffmt("margin-left: -%dem;", w);
		bufcat("clear: left;");
		bufcat("float: left;");
		bufcat("padding-right: 1em;");
		tag.key = ATTR_STYLE;
		tag.val = buf;
		print_otag(h, TAG_DIV, 1, &tag);
		break;
	}

	switch (t) {
	case (MDOC_Diag):
		tag.key = ATTR_CLASS;
		tag.val = "diag";
		print_otag(h, TAG_SPAN, 1, &tag);
		break;
	case (MDOC_Enum):
		ord = SLIST_FIRST(&h->ords);
		assert(ord);
		nbuf[BUFSIZ - 1] = 0;
		(void)snprintf(nbuf, BUFSIZ - 1, "%d.", ord->pos++);
		print_text(h, nbuf);
		return(0);
	case (MDOC_Dash):
		print_text(h, "\\(en");
		return(0);
	case (MDOC_Hyphen):
		print_text(h, "\\-");
		return(0);
	case (MDOC_Bullet):
		print_text(h, "\\(bu");
		return(0);
	default:
		break;
	}

	return(1);
}


static int
mdoc_tbl_pre(MDOC_ARGS, int type)
{
	int			 i, w, o, c, wp;
	const struct mdoc_node	*bl, *nn;

	bl = n->parent->parent;
	if (MDOC_BLOCK != n->type) 
		bl = bl->parent;

	/* FIXME: fmt_vspace() equivalent. */

	assert(bl->args);

	w = o = c = 0;
	wp = -1;

	for (i = 0; i < (int)bl->args->argc; i++) 
		if (MDOC_Width == bl->args->argv[i].arg) {
			assert(bl->args->argv[i].sz);
			wp = i;
			w = a2width(bl->args->argv[i].value[0]);
		} else if (MDOC_Offset == bl->args->argv[i].arg) {
			assert(bl->args->argv[i].sz);
			o = a2offs(bl->args->argv[i].value[0]);
		} else if (MDOC_Compact == bl->args->argv[i].arg) 
			c = 1;
	
	if (MDOC_HEAD == n->type && MDOC_Column == type) {
		nn = n->parent->child;
		assert(nn && MDOC_HEAD == nn->type);
		for (i = 0; nn && nn != n; nn = nn->next, i++)
			/* Counter... */ ;
		assert(nn);
		if (wp >= 0 && i < (int)bl->args[wp].argv->sz)
			w = a2width(bl->args->argv[wp].value[i]);
	}

	switch (type) {
	case (MDOC_Enum):
		/* FALLTHROUGH */
	case (MDOC_Dash):
		/* FALLTHROUGH */
	case (MDOC_Hyphen):
		/* FALLTHROUGH */
	case (MDOC_Bullet):
		if (w < 4)
			w = 4;
		break;
	case (MDOC_Inset):
		/* FALLTHROUGH */
	case (MDOC_Diag):
		w = 1;
		break;
	default:
		if (0 == w)
			w = 10;
		break;
	}

	switch (n->type) {
	case (MDOC_BLOCK):
		break;
	case (MDOC_HEAD):
		return(mdoc_tbl_head_pre(m, n, h, type, w));
	case (MDOC_BODY):
		return(mdoc_tbl_body_pre(m, n, h, type, w));
	default:
		abort();
		/* NOTREACHED */
	}

	return(mdoc_tbl_block_pre(m, n, h, type, w, o, c));
}


static int
mdoc_bl_pre(MDOC_ARGS)
{
	struct ord	*ord;

	if (MDOC_BLOCK != n->type)
		return(1);
	if (MDOC_Enum != a2list(n))
		return(1);

	ord = malloc(sizeof(struct ord));
	if (NULL == ord)
		err(EXIT_FAILURE, "malloc");
	ord->cookie = n;
	ord->pos = 1;
	SLIST_INSERT_HEAD(&h->ords, ord, entry);

	return(1);
}


static void
mdoc_bl_post(MDOC_ARGS)
{
	struct ord	*ord;

	if (MDOC_BLOCK != n->type)
		return;
	if (MDOC_Enum != a2list(n))
		return;

	ord = SLIST_FIRST(&h->ords);
	assert(ord);
	SLIST_REMOVE_HEAD(&h->ords, entry);
	free(ord);
}


static int
mdoc_it_pre(MDOC_ARGS)
{
	int		 type;

	if (MDOC_BLOCK == n->type)
		type = a2list(n->parent->parent);
	else
		type = a2list(n->parent->parent->parent);

	return(mdoc_tbl_pre(m, n, h, type));
}


/* ARGSUSED */
static int
mdoc_ex_pre(MDOC_ARGS)
{
	const struct mdoc_node	*nn;
	struct tag		*t;
	struct htmlpair		 tag;

	print_text(h, "The");

	tag.key = ATTR_CLASS;
	tag.val = "utility";

	for (nn = n->child; nn; nn = nn->next) {
		t = print_otag(h, TAG_SPAN, 1, &tag);
		print_text(h, nn->string);
		print_tagq(h, t);

		h->flags |= HTML_NOSPACE;

		if (nn->next && NULL == nn->next->next)
			print_text(h, ", and");
		else if (nn->next)
			print_text(h, ",");
		else
			h->flags &= ~HTML_NOSPACE;
	}

	if (n->child->next)
		print_text(h, "utilities exit");
	else
		print_text(h, "utility exits");

       	print_text(h, "0 on success, and >0 if an error occurs.");
	return(0);
}


/* ARGSUSED */
static int
mdoc_dq_pre(MDOC_ARGS)
{

	if (MDOC_BODY != n->type)
		return(1);
	print_text(h, "\\(lq");
	h->flags |= HTML_NOSPACE;
	return(1);
}


/* ARGSUSED */
static void
mdoc_dq_post(MDOC_ARGS)
{

	if (MDOC_BODY != n->type)
		return;
	h->flags |= HTML_NOSPACE;
	print_text(h, "\\(rq");
}


/* ARGSUSED */
static int
mdoc_pq_pre(MDOC_ARGS)
{

	if (MDOC_BODY != n->type)
		return(1);
	print_text(h, "\\&(");
	h->flags |= HTML_NOSPACE;
	return(1);
}


/* ARGSUSED */
static void
mdoc_pq_post(MDOC_ARGS)
{

	if (MDOC_BODY != n->type)
		return;
	print_text(h, ")");
}


/* ARGSUSED */
static int
mdoc_sq_pre(MDOC_ARGS)
{

	if (MDOC_BODY != n->type)
		return(1);
	print_text(h, "\\(oq");
	h->flags |= HTML_NOSPACE;
	return(1);
}


/* ARGSUSED */
static void
mdoc_sq_post(MDOC_ARGS)
{

	if (MDOC_BODY != n->type)
		return;
	h->flags |= HTML_NOSPACE;
	print_text(h, "\\(aq");
}


/* ARGSUSED */
static int
mdoc_em_pre(MDOC_ARGS)
{
	struct htmlpair	tag;

	tag.key = ATTR_CLASS;
	tag.val = "emph";

	print_otag(h, TAG_SPAN, 1, &tag);
	return(1);
}


/* ARGSUSED */
static int
mdoc_d1_pre(MDOC_ARGS)
{
	struct htmlpair	tag[2];

	if (MDOC_BLOCK != n->type)
		return(1);

	buffmt("margin-left: %dem;", INDENT);

	tag[0].key = ATTR_CLASS;
	tag[0].val = "lit-block";
	tag[1].key = ATTR_STYLE;
	tag[1].val = buf;

	print_otag(h, TAG_DIV, 2, tag);
	return(1);
}


/* ARGSUSED */
static int
mdoc_sx_pre(MDOC_ARGS)
{
	struct htmlpair	tag[2];

	tag[0].key = ATTR_HREF;
	tag[0].val = "#"; /* XXX */
	tag[1].key = ATTR_CLASS;
	tag[1].val = "link-sec";

	print_otag(h, TAG_A, 2, tag);
	return(1);
}


/* ARGSUSED */
static int
mdoc_aq_pre(MDOC_ARGS)
{

	if (MDOC_BODY != n->type)
		return(1);
	print_text(h, "\\(la");
	h->flags |= HTML_NOSPACE;
	return(1);
}


/* ARGSUSED */
static void
mdoc_aq_post(MDOC_ARGS)
{

	if (MDOC_BODY != n->type)
		return;
	h->flags |= HTML_NOSPACE;
	print_text(h, "\\(ra");
}


/* ARGSUSED */
static int
mdoc_bd_pre(MDOC_ARGS)
{
	struct htmlpair	 	 tag[2];
	int		 	 t, c, o, i;
	const struct mdoc_node	*bl;

	/* FIXME: fmt_vspace() shit. */

	if (MDOC_BLOCK == n->type)
		bl = n;
	else if (MDOC_HEAD == n->type)
		return(0);
	else
		bl = n->parent;

	t = o = c = 0;

	for (i = 0; i < (int)bl->args->argc; i++) 
		switch (bl->args->argv[i].arg) {
		case (MDOC_Offset):
			assert(bl->args->argv[i].sz);
			o = a2offs(bl->args->argv[i].value[0]);
			break;
		case (MDOC_Compact):
			c = 1;
			break;
		case (MDOC_Ragged):
			/* FALLTHROUGH */
		case (MDOC_Filled):
			/* FALLTHROUGH */
		case (MDOC_Unfilled):
			/* FALLTHROUGH */
		case (MDOC_Literal):
			t = bl->args->argv[i].arg;
			break;
		}

	if (MDOC_BLOCK == n->type) {
		if (o)
			buffmt("margin-left: %dem;", o);
		bufcat("margin-top: 1em;");
		tag[0].key = ATTR_STYLE;
		tag[0].val = buf;
		print_otag(h, TAG_DIV, 1, tag);
		return(1);
	}

	switch (t) {
	case (MDOC_Unfilled):
	case (MDOC_Literal):
		break;
	default:
		return(1);
	}

	bufcat("white-space: pre;");
	tag[0].key = ATTR_STYLE;
	tag[0].val = buf;
	tag[1].key = ATTR_CLASS;
	tag[1].val = "lit-block";

	print_otag(h, TAG_DIV, 2, tag);

	for (n = n->child; n; n = n->next) {
		h->flags |= HTML_NOSPACE;
		print_mdoc_node(m, n, h);
		if (n->next)
			print_text(h, "\n");
	}

	return(0);
}


/* ARGSUSED */
static int
mdoc_pa_pre(MDOC_ARGS)
{
	struct htmlpair	tag;

	tag.key = ATTR_CLASS;
	tag.val = "file";

	print_otag(h, TAG_SPAN, 1, &tag);
	return(1);
}


/* ARGSUSED */
static int
mdoc_qq_pre(MDOC_ARGS)
{

	if (MDOC_BODY != n->type)
		return(1);
	print_text(h, "\\*q");
	h->flags |= HTML_NOSPACE;
	return(1);
}


/* ARGSUSED */
static void
mdoc_qq_post(MDOC_ARGS)
{

	if (MDOC_BODY != n->type)
		return;
	h->flags |= HTML_NOSPACE;
	print_text(h, "\\*q");
}
