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
#include <unistd.h>

#include "html.h"
#include "mdoc.h"

#define	INDENT		 5
#define	HALFINDENT	 3

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

static	void		  print_mdoc(MDOC_ARGS);
static	void		  print_mdoc_head(MDOC_ARGS);
static	void		  print_mdoc_node(MDOC_ARGS);
static	void		  print_mdoc_nodelist(MDOC_ARGS);

static	int		  a2width(const char *);
static	int		  a2offs(const char *);
static	int		  a2list(const struct mdoc_node *);

static	void		  mdoc_root_post(MDOC_ARGS);
static	int		  mdoc_root_pre(MDOC_ARGS);
static	int		  mdoc_tbl_pre(MDOC_ARGS, int);
static	int		  mdoc_tbl_block_pre(MDOC_ARGS, int, int, int, int);
static	int		  mdoc_tbl_body_pre(MDOC_ARGS, int, int);
static	int		  mdoc_tbl_head_pre(MDOC_ARGS, int, int);

static	int		  mdoc_ad_pre(MDOC_ARGS);
static	int		  mdoc_an_pre(MDOC_ARGS);
static	void		  mdoc_aq_post(MDOC_ARGS);
static	int		  mdoc_aq_pre(MDOC_ARGS);
static	int		  mdoc_ar_pre(MDOC_ARGS);
static	int		  mdoc_bd_pre(MDOC_ARGS);
static	void		  mdoc_bl_post(MDOC_ARGS);
static	int		  mdoc_bl_pre(MDOC_ARGS);
static	int		  mdoc_cd_pre(MDOC_ARGS);
static	int		  mdoc_d1_pre(MDOC_ARGS);
static	void		  mdoc_dq_post(MDOC_ARGS);
static	int		  mdoc_dq_pre(MDOC_ARGS);
static	int		  mdoc_dv_pre(MDOC_ARGS);
static	int		  mdoc_fa_pre(MDOC_ARGS);
static	int		  mdoc_fd_pre(MDOC_ARGS);
static	int		  mdoc_fl_pre(MDOC_ARGS);
static	int		  mdoc_fn_pre(MDOC_ARGS);
static	int		  mdoc_ft_pre(MDOC_ARGS);
static	int		  mdoc_em_pre(MDOC_ARGS);
static	int		  mdoc_er_pre(MDOC_ARGS);
static	int		  mdoc_ev_pre(MDOC_ARGS);
static	int		  mdoc_ex_pre(MDOC_ARGS);
static	int		  mdoc_it_pre(MDOC_ARGS);
static	int		  mdoc_nd_pre(MDOC_ARGS);
static	int		  mdoc_nm_pre(MDOC_ARGS);
static	int		  mdoc_ns_pre(MDOC_ARGS);
static	void		  mdoc_op_post(MDOC_ARGS);
static	int		  mdoc_op_pre(MDOC_ARGS);
static	int		  mdoc_pa_pre(MDOC_ARGS);
static	void		  mdoc_pq_post(MDOC_ARGS);
static	int		  mdoc_pq_pre(MDOC_ARGS);
static	void		  mdoc_qq_post(MDOC_ARGS);
static	int		  mdoc_qq_pre(MDOC_ARGS);
static	int		  mdoc_sh_pre(MDOC_ARGS);
static	int		  mdoc_sp_pre(MDOC_ARGS);
static	void		  mdoc_sq_post(MDOC_ARGS);
static	int		  mdoc_sq_pre(MDOC_ARGS);
static	int		  mdoc_ss_pre(MDOC_ARGS);
static	int		  mdoc_sx_pre(MDOC_ARGS);
static	int		  mdoc_vt_pre(MDOC_ARGS);
static	int		  mdoc_xr_pre(MDOC_ARGS);
static	int		  mdoc_xx_pre(MDOC_ARGS);

#ifdef __linux__
extern	size_t	  	  strlcpy(char *, const char *, size_t);
extern	size_t	  	  strlcat(char *, const char *, size_t);
#endif

static	const struct htmlmdoc mdocs[MDOC_MAX] = {
	{NULL, NULL}, /* Ap */
	{NULL, NULL}, /* Dd */
	{NULL, NULL}, /* Dt */
	{NULL, NULL}, /* Os */
	{mdoc_sh_pre, NULL }, /* Sh */
	{mdoc_ss_pre, NULL }, /* Ss */ 
	{mdoc_sp_pre, NULL}, /* Pp */ 
	{mdoc_d1_pre, NULL}, /* D1 */
	{mdoc_d1_pre, NULL}, /* Dl */
	{mdoc_bd_pre, NULL}, /* Bd */
	{NULL, NULL}, /* Ed */
	{mdoc_bl_pre, mdoc_bl_post}, /* Bl */
	{NULL, NULL}, /* El */
	{mdoc_it_pre, NULL}, /* It */
	{mdoc_ad_pre, NULL}, /* Ad */ 
	{mdoc_an_pre, NULL}, /* An */
	{mdoc_ar_pre, NULL}, /* Ar */
	{mdoc_cd_pre, NULL}, /* Cd */
	{mdoc_fl_pre, NULL}, /* Cm */
	{mdoc_dv_pre, NULL}, /* Dv */ 
	{mdoc_er_pre, NULL}, /* Er */ 
	{mdoc_ev_pre, NULL}, /* Ev */ 
	{mdoc_ex_pre, NULL}, /* Ex */
	{mdoc_fa_pre, NULL}, /* Fa */ 
	{mdoc_fd_pre, NULL}, /* Fd */ 
	{mdoc_fl_pre, NULL}, /* Fl */
	{mdoc_fn_pre, NULL}, /* Fn */ 
	{mdoc_ft_pre, NULL}, /* Ft */ 
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
	{mdoc_vt_pre, NULL}, /* Vt */ 
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
	{mdoc_sp_pre, NULL}, /* Lp */ 
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
	{mdoc_sp_pre, NULL}, /* br */
	{mdoc_sp_pre, NULL}, /* sp */ 
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


static int
a2list(const struct mdoc_node *n)
{
	int		 i;

	assert(MDOC_BLOCK == n->type && MDOC_Bl == n->tok);
	assert(n->args);

	for (i = 0; i < (int)n->args->argc; i++) 
		switch (n->args->argv[i].arg) {
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
			return(n->args->argv[i].arg);
		default:
			break;
		}

	abort();
	/* NOTREACHED */
}


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


static void
print_mdoc(MDOC_ARGS)
{
	struct tag	*t;
	struct htmlpair	 tag;

	t = print_otag(h, TAG_HEAD, 0, NULL);
	print_mdoc_head(m, n, h);
	print_tagq(h, t);

	t = print_otag(h, TAG_BODY, 0, NULL);

	tag.key = ATTR_CLASS;
	tag.val = "body";
	print_otag(h, TAG_DIV, 1, &tag);

	print_mdoc_nodelist(m, n, h);
	print_tagq(h, t);
}


/* ARGSUSED */
static void
print_mdoc_head(MDOC_ARGS)
{
	char		b[BUFSIZ];

	print_gen_head(h);

	(void)snprintf(b, BUFSIZ - 1, 
			"%s(%d)", m->title, m->msec);

	if (m->arch) {
		(void)strlcat(b, " (", BUFSIZ);
		(void)strlcat(b, m->arch, BUFSIZ);
		(void)strlcat(b, ")", BUFSIZ);
	}

	print_otag(h, TAG_TITLE, 0, NULL);
	print_text(h, b);
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


/* ARGSUSED */
static void
mdoc_root_post(MDOC_ARGS)
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
	print_text(h, m->os);
	print_tagq(h, t);
}


/* ARGSUSED */
static int
mdoc_root_pre(MDOC_ARGS)
{
	struct htmlpair	 tag[2];
	struct tag	*t, *tt;
	char		 b[BUFSIZ], title[BUFSIZ];

	(void)strlcpy(b, m->vol, BUFSIZ);

	if (m->arch) {
		(void)strlcat(b, " (", BUFSIZ);
		(void)strlcat(b, m->arch, BUFSIZ);
		(void)strlcat(b, ")", BUFSIZ);
	}

	(void)snprintf(title, BUFSIZ - 1, 
			"%s(%d)", m->title, m->msec);

	tag[0].key = ATTR_CLASS;
	tag[0].val = "header";
	tag[1].key = ATTR_STYLE;
	tag[1].val = "width: 100%;";
	t = print_otag(h, TAG_TABLE, 2, tag);
	tt = print_otag(h, TAG_TR, 0, NULL);

	tag[0].key = ATTR_STYLE;
	tag[0].val = "width: 33%;";
	print_otag(h, TAG_TD, 1, tag);
	print_text(h, b);
	print_stagq(h, tt);

	tag[0].key = ATTR_STYLE;
	tag[0].val = "width: 33%; text-align: center;";
	print_otag(h, TAG_TD, 1, tag);
	print_text(h, title);
	print_stagq(h, tt);

	tag[0].key = ATTR_STYLE;
	tag[0].val = "width: 33%; text-align: right;";
	print_otag(h, TAG_TD, 1, tag);
	print_text(h, b);
	print_tagq(h, t);

	return(1);
}


/* ARGSUSED */
static int
mdoc_sh_pre(MDOC_ARGS)
{
	struct htmlpair		 tag[2];
	const struct mdoc_node	*nn;

	if (MDOC_HEAD == n->type) {
		tag[0].key = ATTR_CLASS;
		tag[0].val = "sec-head";
		print_otag(h, TAG_DIV, 1, tag);
		print_otag(h, TAG_SPAN, 1, tag);

		for (nn = n->child; nn; nn = nn->next) {
			bufcat(nn->string);
			if (nn->next)
				bufcat(" ");
		}
		tag[0].key = ATTR_NAME;
		tag[0].val = buf;
		print_otag(h, TAG_A, 1, tag);
		return(1);
	} else if (MDOC_BLOCK == n->type) {
		tag[0].key = ATTR_CLASS;
		tag[0].val = "sec-block";

		if (n->prev && NULL == n->prev->body->child) {
			print_otag(h, TAG_DIV, 1, tag);
			return(1);
		}

		bufcat("margin-top: 1em;");
		if (NULL == n->next)
			bufcat("margin-bottom: 1em;");

		tag[1].key = ATTR_STYLE;
		tag[1].val = buf;

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
	struct htmlpair	 	 tag[2];
	int		 	 i;
	const struct mdoc_node	*nn;

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

	bufinit();
	for (nn = n->child; nn; nn = nn->next) {
		bufcat(nn->string);
		if (nn->next)
			bufcat(" ");
	}
	tag[0].key = ATTR_NAME;
	tag[0].val = buf;
	print_otag(h, TAG_A, 1, tag);

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
	if (MDOC_Fl == n->tok) {
		print_text(h, "\\-");
		h->flags |= HTML_NOSPACE;
	}
	return(1);
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
	h->flags |= HTML_NOSPACE;
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
	struct htmlpair	 	 tag[2];
	const char		*name, *sec;
	const struct mdoc_node	*nn;

	nn = n->child;
	name = nn && nn->string ? nn->string : "";
	nn = nn ? nn->next : NULL;
	sec = nn && nn->string ? nn->string : "";

	buffmt("%s%s%s.html", name, name && sec ? "." : "", sec);

	tag[0].key = ATTR_CLASS;
	tag[0].val = "link-man";
	tag[1].key = ATTR_HREF;
	tag[1].val = buf;
	print_otag(h, TAG_A, 2, tag);

	nn = n->child;
	print_text(h, nn->string);
	if (NULL == (nn = nn->next))
		return(0);

	h->flags |= HTML_NOSPACE;
	print_text(h, "(");
	h->flags |= HTML_NOSPACE;
	print_text(h, nn->string);
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

	if ( ! c && MDOC_Column != t) {
	       if (n->prev && n->prev->body->child)
		       bufcat("padding-top: 1em;");
	       else if (NULL == n->prev)
		       bufcat("padding-top: 1em;");
	}

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


/* ARGSUSED */
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


/* ARGSUSED */
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
	struct htmlpair		 tag[2];
	const struct mdoc_node	*nn;

	bufcat("#");
	for (nn = n->child; nn; nn = nn->next) {
		bufcat(nn->string);
		if (nn->next)
			bufcat(" ");
	}

	tag[0].key = ATTR_HREF;
	tag[0].val = buf;
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
			o = a2offs (bl->args->argv[i].value[0]);
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


/* ARGSUSED */
static int
mdoc_ad_pre(MDOC_ARGS)
{
	struct htmlpair	tag;

	tag.key = ATTR_CLASS;
	tag.val = "addr";
	print_otag(h, TAG_SPAN, 1, &tag);
	return(1);
}


/* ARGSUSED */
static int
mdoc_an_pre(MDOC_ARGS)
{
	struct htmlpair	tag;

	tag.key = ATTR_CLASS;
	tag.val = "author";
	print_otag(h, TAG_SPAN, 1, &tag);
	return(1);
}


/* ARGSUSED */
static int
mdoc_cd_pre(MDOC_ARGS)
{
	struct htmlpair	tag;

	tag.key = ATTR_CLASS;
	tag.val = "config";
	print_otag(h, TAG_SPAN, 1, &tag);
	return(1);
}


/* ARGSUSED */
static int
mdoc_dv_pre(MDOC_ARGS)
{
	struct htmlpair	tag;

	tag.key = ATTR_CLASS;
	tag.val = "define";
	print_otag(h, TAG_SPAN, 1, &tag);
	return(1);
}


/* ARGSUSED */
static int
mdoc_ev_pre(MDOC_ARGS)
{
	struct htmlpair	tag;

	tag.key = ATTR_CLASS;
	tag.val = "env";
	print_otag(h, TAG_SPAN, 1, &tag);
	return(1);
}


/* ARGSUSED */
static int
mdoc_er_pre(MDOC_ARGS)
{
	struct htmlpair	tag;

	tag.key = ATTR_CLASS;
	tag.val = "errno";
	print_otag(h, TAG_SPAN, 1, &tag);
	return(1);
}


/* ARGSUSED */
static int
mdoc_fa_pre(MDOC_ARGS)
{
	const struct mdoc_node	*nn;
	struct htmlpair		 tag;
	struct tag		*t;

	tag.key = ATTR_CLASS;
	tag.val = "farg";

	if (n->parent->tok != MDOC_Fo) {
		print_otag(h, TAG_SPAN, 1, &tag);
		return(1);
	}

	for (nn = n->child; nn; nn = nn->next) {
		t = print_otag(h, TAG_SPAN, 1, &tag);
		print_text(h, nn->string);
		print_tagq(h, t);
		if (nn->next)
			print_text(h, ",");
	}

	if (n->child && n->next && n->next->tok == MDOC_Fa)
		print_text(h, ",");

	return(0);
}


/* ARGSUSED */
static int
mdoc_fd_pre(MDOC_ARGS)
{
	struct htmlpair	tag;

	if (SEC_SYNOPSIS == n->sec) {
		if (n->next && MDOC_Fd != n->next->tok) {
			tag.key = ATTR_STYLE;
			tag.val = "margin-bottom: 1em;";
			print_otag(h, TAG_DIV, 1, &tag);
		} else
			print_otag(h, TAG_DIV, 0, NULL);
	}

	tag.key = ATTR_CLASS;
	tag.val = "macro";
	print_otag(h, TAG_SPAN, 1, &tag);
	return(1);
}


/* ARGSUSED */
static int
mdoc_vt_pre(MDOC_ARGS)
{
	struct htmlpair	tag;

	if (SEC_SYNOPSIS == n->sec) {
		if (n->next && MDOC_Vt != n->next->tok) {
			tag.key = ATTR_STYLE;
			tag.val = "margin-bottom: 1em;";
			print_otag(h, TAG_DIV, 1, &tag);
		} else
			print_otag(h, TAG_DIV, 0, NULL);
	}

	tag.key = ATTR_CLASS;
	tag.val = "type";
	print_otag(h, TAG_SPAN, 1, &tag);
	return(1);
}

/* ARGSUSED */
static int
mdoc_ft_pre(MDOC_ARGS)
{
	struct htmlpair	tag;

	if (SEC_SYNOPSIS == n->sec) {
		if (n->prev && MDOC_Fo == n->prev->tok) {
			tag.key = ATTR_STYLE;
			tag.val = "magin-bottom: 1em;";
			print_otag(h, TAG_DIV, 1, &tag);
		} else
			print_otag(h, TAG_DIV, 0, NULL);
	}

	tag.key = ATTR_CLASS;
	tag.val = "type";
	print_otag(h, TAG_SPAN, 1, &tag);
	return(1);
}


/* ARGSUSED */
static int
mdoc_fn_pre(MDOC_ARGS)
{
	struct tag		*t;
	struct htmlpair	 	 tag;
	const struct mdoc_node	*nn;

	if (SEC_SYNOPSIS == n->sec) {
		if (n->next) {
			tag.key = ATTR_STYLE;
			tag.val = "margin-bottom: 1em";
			print_otag(h, TAG_DIV, 1, &tag);
		} else
			print_otag(h, TAG_DIV, 0, NULL);
	}

	tag.key = ATTR_CLASS;
	tag.val = "type";

	t = print_otag(h, TAG_SPAN, 1, &tag);
	print_text(h, n->child->string);
	print_tagq(h, t);

	h->flags |= HTML_NOSPACE;
	print_text(h, "(");

	for (nn = n->child->next; nn; nn = nn->next) {
		tag.key = ATTR_CLASS;
		tag.val = "farg";
		t = print_otag(h, TAG_SPAN, 1, &tag);
		print_text(h, nn->string);
		print_tagq(h, t);
		if (nn->next)
			print_text(h, ",");
	}

	print_text(h, ")");

	if (SEC_SYNOPSIS == n->sec)
		print_text(h, ";");

	return(0);
}


/* ARGSUSED */
static int
mdoc_sp_pre(MDOC_ARGS)
{
	int		len;
	struct htmlpair	tag;

	switch (n->tok) {
	case (MDOC_sp):
		len = n->child ? atoi(n->child->string) : 1;
		break;
	case (MDOC_br):
		len = 0;
		break;
	default:
		len = 1;
		break;
	}

	buffmt("height: %dem", len);
	tag.key = ATTR_STYLE;
	tag.val = buf;
	print_otag(h, TAG_DIV, 1, &tag);
	return(1);

}
