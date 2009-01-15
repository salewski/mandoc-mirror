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
#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "private.h"

const	char *const __mdoc_macronames[MDOC_MAX] = {		 
	"\\\"",		"Dd",		"Dt",		"Os",
	"Sh",		"Ss",		"Pp",		"D1",
	"Dl",		"Bd",		"Ed",		"Bl",
	"El",		"It",		"Ad",		"An",
	"Ar",		"Cd",		"Cm",		"Dv",
	"Er",		"Ev",		"Ex",		"Fa",
	"Fd",		"Fl",		"Fn",		"Ft",
	"Ic",		"In",		"Li",		"Nd",
	"Nm",		"Op",		"Ot",		"Pa",
	"Rv",		"St",		"Va",		"Vt",
	/* LINTED */
	"Xr",		"\%A",		"\%B",		"\%D",
	/* LINTED */
	"\%I",		"\%J",		"\%N",		"\%O",
	/* LINTED */
	"\%P",		"\%R",		"\%T",		"\%V",
	"Ac",		"Ao",		"Aq",		"At",
	"Bc",		"Bf",		"Bo",		"Bq",
	"Bsx",		"Bx",		"Db",		"Dc",
	"Do",		"Dq",		"Ec",		"Ef",
	"Em",		"Eo",		"Fx",		"Ms",
	"No",		"Ns",		"Nx",		"Ox",
	"Pc",		"Pf",		"Po",		"Pq",
	"Qc",		"Ql",		"Qo",		"Qq",
	"Re",		"Rs",		"Sc",		"So",
	"Sq",		"Sm",		"Sx",		"Sy",
	"Tn",		"Ux",		"Xc",		"Xo",
	"Fo",		"Fc",		"Oo",		"Oc",
	"Bk",		"Ek",		"Bt",		"Hf",
	"Fr",		"Ud",
	};

const	char *const __mdoc_argnames[MDOC_ARG_MAX] = {		 
	"split",		"nosplit",		"ragged",
	"unfilled",		"literal",		"file",		 
	"offset",		"bullet",		"dash",		 
	"hyphen",		"item",			"enum",		 
	"tag",			"diag",			"hang",		 
	"ohang",		"inset",		"column",	 
	"width",		"compact",		"std",	 
	"p1003.1-88",		"p1003.1-90",		"p1003.1-96",
	"p1003.1-2001",		"p1003.1-2004",		"p1003.1",
	"p1003.1b",		"p1003.1b-93",		"p1003.1c-95",
	"p1003.1g-2000",	"p1003.2-92",		"p1387.2-95",
	"p1003.2",		"p1387.2",		"isoC-90",
	"isoC-amd1",		"isoC-tcor1",		"isoC-tcor2",
	"isoC-99",		"ansiC",		"ansiC-89",
	"ansiC-99",		"ieee754",		"iso8802-3",
	"xpg3",			"xpg4",			"xpg4.2",
	"xpg4.3",		"xbd5",			"xcu5",
	"xsh5",			"xns5",			"xns5.2d2.0",
	"xcurses4.2",		"susv2",		"susv3",
	"svid4",		"filled",		"words",
	"emphasis",		"symbolic",
	};

const	struct mdoc_macro __mdoc_macros[MDOC_MAX] = {
	{ NULL, 0 }, /* \" */
	{ macro_constant, MDOC_PROLOGUE }, /* Dd */
	{ macro_constant, MDOC_PROLOGUE }, /* Dt */
	{ macro_constant, MDOC_PROLOGUE }, /* Os */
	{ macro_scoped, 0 }, /* Sh */
	{ macro_scoped, 0 }, /* Ss */ 
	{ macro_text, 0 }, /* Pp */ 
	{ macro_scoped_line, MDOC_PARSED }, /* D1 */
	{ macro_scoped_line, MDOC_PARSED }, /* Dl */
	{ macro_scoped, MDOC_EXPLICIT }, /* Bd */
	{ macro_scoped_close, MDOC_EXPLICIT }, /* Ed */
	{ macro_scoped, MDOC_EXPLICIT }, /* Bl */
	{ macro_scoped_close, MDOC_EXPLICIT }, /* El */
	{ macro_scoped, MDOC_PARSED | MDOC_TABSEP}, /* It */
	{ macro_text, MDOC_CALLABLE | MDOC_PARSED }, /* Ad */ 
	{ macro_constant, MDOC_PARSED }, /* An */
	{ macro_text, MDOC_CALLABLE | MDOC_PARSED }, /* Ar */
	{ macro_constant, MDOC_QUOTABLE }, /* Cd */
	{ macro_text, MDOC_CALLABLE | MDOC_PARSED }, /* Cm */
	{ macro_text, MDOC_CALLABLE | MDOC_PARSED }, /* Dv */ 
	{ macro_text, MDOC_CALLABLE | MDOC_PARSED }, /* Er */ 
	{ macro_text, MDOC_CALLABLE | MDOC_PARSED }, /* Ev */ 
	{ macro_constant, 0 }, /* Ex */
	{ macro_text, MDOC_CALLABLE | MDOC_QUOTABLE | MDOC_PARSED }, /* Fa */ 
	{ macro_constant, 0 }, /* Fd */ 
	{ macro_text, MDOC_CALLABLE | MDOC_PARSED }, /* Fl */
	{ macro_text, MDOC_CALLABLE | MDOC_QUOTABLE | MDOC_PARSED }, /* Fn */ 
	{ macro_text, MDOC_PARSED }, /* Ft */ 
	{ macro_text, MDOC_CALLABLE | MDOC_PARSED }, /* Ic */ 
	{ macro_constant, 0 }, /* In */ 
	{ macro_text, MDOC_CALLABLE | MDOC_PARSED }, /* Li */
	{ macro_constant, 0 }, /* Nd */ 
	{ macro_text, MDOC_CALLABLE | MDOC_PARSED }, /* Nm */ 
	{ macro_scoped_line, MDOC_CALLABLE | MDOC_PARSED }, /* Op */
	{ macro_obsolete, 0 }, /* Ot */
	{ macro_text, MDOC_CALLABLE | MDOC_PARSED }, /* Pa */
	{ macro_constant, 0 }, /* Rv */
	/* XXX - .St supposed to be (but isn't) callable. */
	{ macro_constant_delimited, MDOC_PARSED }, /* St */ 
	{ macro_text, MDOC_CALLABLE | MDOC_PARSED }, /* Va */
	{ macro_text, MDOC_CALLABLE | MDOC_PARSED }, /* Vt */ 
	{ macro_text, MDOC_CALLABLE | MDOC_PARSED }, /* Xr */
	{ macro_constant, MDOC_QUOTABLE }, /* %A */
	{ macro_constant, MDOC_QUOTABLE }, /* %B */
	{ macro_constant, MDOC_QUOTABLE }, /* %D */
	{ macro_constant, MDOC_QUOTABLE }, /* %I */
	{ macro_constant, MDOC_QUOTABLE }, /* %J */
	{ macro_constant, MDOC_QUOTABLE }, /* %N */
	{ macro_constant, MDOC_QUOTABLE }, /* %O */
	{ macro_constant, MDOC_QUOTABLE }, /* %P */
	{ macro_constant, MDOC_QUOTABLE }, /* %R */
	{ macro_constant, MDOC_QUOTABLE }, /* %T */
	{ macro_constant, MDOC_QUOTABLE }, /* %V */
	{ macro_scoped_close, MDOC_EXPLICIT | MDOC_CALLABLE | MDOC_PARSED }, /* Ac */
	{ macro_constant_scoped, MDOC_CALLABLE | MDOC_PARSED | MDOC_EXPLICIT }, /* Ao */
	{ macro_scoped_line, MDOC_CALLABLE | MDOC_PARSED }, /* Aq */
	{ macro_constant, 0 }, /* At */
	{ macro_scoped_close, MDOC_EXPLICIT | MDOC_CALLABLE | MDOC_PARSED }, /* Bc */
	{ macro_scoped, MDOC_EXPLICIT }, /* Bf */ 
	{ macro_constant_scoped, MDOC_CALLABLE | MDOC_PARSED | MDOC_EXPLICIT }, /* Bo */
	{ macro_scoped_line, MDOC_CALLABLE | MDOC_PARSED }, /* Bq */
	{ macro_constant_delimited, MDOC_PARSED }, /* Bsx */
	{ macro_constant_delimited, MDOC_PARSED }, /* Bx */
	{ macro_constant, 0 }, /* Db */
	{ macro_scoped_close, MDOC_EXPLICIT | MDOC_CALLABLE | MDOC_PARSED }, /* Dc */
	{ macro_constant_scoped, MDOC_CALLABLE | MDOC_PARSED | MDOC_EXPLICIT }, /* Do */
	{ macro_scoped_line, MDOC_CALLABLE | MDOC_PARSED }, /* Dq */
	{ macro_scoped_close, MDOC_EXPLICIT | MDOC_CALLABLE | MDOC_PARSED }, /* Ec */
	{ macro_scoped_close, MDOC_EXPLICIT }, /* Ef */
	{ macro_text, MDOC_CALLABLE | MDOC_PARSED }, /* Em */ 
	{ macro_constant_scoped, MDOC_CALLABLE | MDOC_PARSED | MDOC_EXPLICIT }, /* Eo */
	{ macro_constant_delimited, MDOC_PARSED }, /* Fx */
	{ macro_text, MDOC_PARSED }, /* Ms */
	{ macro_constant_delimited, MDOC_CALLABLE | MDOC_PARSED }, /* No */
	{ macro_constant_delimited, MDOC_CALLABLE | MDOC_PARSED }, /* Ns */
	{ macro_constant_delimited, MDOC_PARSED }, /* Nx */
	{ macro_constant_delimited, MDOC_PARSED }, /* Ox */
	{ macro_scoped_close, MDOC_EXPLICIT | MDOC_CALLABLE | MDOC_PARSED }, /* Pc */
	{ macro_constant, MDOC_PARSED }, /* Pf */
	{ macro_constant_scoped, MDOC_CALLABLE | MDOC_PARSED | MDOC_EXPLICIT }, /* Po */
	{ macro_scoped_line, MDOC_CALLABLE | MDOC_PARSED }, /* Pq */
	{ macro_scoped_close, MDOC_EXPLICIT | MDOC_CALLABLE | MDOC_PARSED }, /* Qc */
	{ macro_scoped_line, MDOC_CALLABLE | MDOC_PARSED }, /* Ql */
	{ macro_constant_scoped, MDOC_CALLABLE | MDOC_PARSED | MDOC_EXPLICIT }, /* Qo */
	{ macro_scoped_line, MDOC_CALLABLE | MDOC_PARSED }, /* Qq */
	{ macro_scoped_close, MDOC_EXPLICIT }, /* Re */
	{ macro_scoped, MDOC_EXPLICIT }, /* Rs */
	{ macro_scoped_close, MDOC_EXPLICIT | MDOC_CALLABLE | MDOC_PARSED }, /* Sc */
	{ macro_constant_scoped, MDOC_CALLABLE | MDOC_PARSED | MDOC_EXPLICIT }, /* So */
	{ macro_scoped_line, MDOC_CALLABLE | MDOC_PARSED }, /* Sq */
	{ macro_constant, 0 }, /* Sm */
	{ macro_text, MDOC_CALLABLE | MDOC_PARSED }, /* Sx */
	{ macro_text, MDOC_CALLABLE | MDOC_PARSED }, /* Sy */
	{ macro_text, MDOC_CALLABLE | MDOC_PARSED }, /* Tn */
	{ macro_constant_delimited, MDOC_PARSED }, /* Ux */
	{ macro_scoped_close, MDOC_EXPLICIT | MDOC_CALLABLE | MDOC_PARSED }, /* Xc */
	{ macro_constant_scoped, MDOC_CALLABLE | MDOC_PARSED | MDOC_EXPLICIT }, /* Xo */
	/* XXX - .Fo supposed to be (but isn't) callable. */
	{ macro_scoped, MDOC_EXPLICIT | MDOC_PARSED }, /* Fo */ 
	/* XXX - .Fc supposed to be (but isn't) callable. */
	{ macro_scoped_close, MDOC_EXPLICIT | MDOC_PARSED }, /* Fc */ 
	{ macro_constant_scoped, MDOC_CALLABLE | MDOC_PARSED | MDOC_EXPLICIT }, /* Oo */
	{ macro_scoped_close, MDOC_EXPLICIT | MDOC_CALLABLE | MDOC_PARSED }, /* Oc */
	{ macro_scoped, MDOC_EXPLICIT }, /* Bk */
	{ macro_scoped_close, MDOC_EXPLICIT }, /* Ek */
	{ macro_constant, 0 }, /* Bt */
	{ macro_constant, 0 }, /* Hf */
	{ macro_obsolete, 0 }, /* Fr */
	{ macro_constant, 0 }, /* Ud */
};

const	char * const *mdoc_macronames = __mdoc_macronames;
const	char * const *mdoc_argnames = __mdoc_argnames;
const	struct mdoc_macro * const mdoc_macros = __mdoc_macros;


static	struct mdoc_arg	 *argdup(size_t, const struct mdoc_arg *);
static	void		  argfree(size_t, struct mdoc_arg *);
static	void	  	  argcpy(struct mdoc_arg *, 
				const struct mdoc_arg *);

static	void		  mdoc_node_freelist(struct mdoc_node *);
static	int		  mdoc_node_append(struct mdoc *, 
				struct mdoc_node *);
static	void		  mdoc_elem_free(struct mdoc_elem *);
static	void		  mdoc_text_free(struct mdoc_text *);


const struct mdoc_node *
mdoc_result(struct mdoc *mdoc)
{

	return(mdoc->first);
}


void
mdoc_free(struct mdoc *mdoc)
{

	if (mdoc->first)
		mdoc_node_freelist(mdoc->first);
	if (mdoc->htab)
		mdoc_tokhash_free(mdoc->htab);
	
	free(mdoc);
}


struct mdoc *
mdoc_alloc(void *data, const struct mdoc_cb *cb)
{
	struct mdoc	*p;

	p = xcalloc(1, sizeof(struct mdoc));

	p->data = data;
	(void)memcpy(&p->cb, cb, sizeof(struct mdoc_cb));

	p->last = xcalloc(1, sizeof(struct mdoc_node));
	p->last->type = MDOC_ROOT;
	p->first = p->last;

	p->next = MDOC_NEXT_CHILD;
	p->htab = mdoc_tokhash_alloc();

	return(p);
}


int
mdoc_endparse(struct mdoc *mdoc)
{

	if (MDOC_HALT & mdoc->flags)
		return(0);
	if (NULL == mdoc->first)
		return(1);

	assert(mdoc->last);
	if ( ! macro_end(mdoc)) {
		mdoc->flags |= MDOC_HALT;
		return(0);
	}
	return(1);
}


int
mdoc_parseln(struct mdoc *mdoc, int line, char *buf)
{
	int		  c, i;
	char		  tmp[5];

	if (MDOC_HALT & mdoc->flags)
		return(0);

	if ('.' != *buf) {
		if (SEC_PROLOGUE != mdoc->sec_lastn) {
			if ( ! mdoc_word_alloc(mdoc, line, 0, buf))
				return(0);
			mdoc->next = MDOC_NEXT_SIBLING;
			return(1);
		}
		return(mdoc_perr(mdoc, line, 0, ERR_SYNTAX_NOTEXT));
	}

	if (buf[1] && '\\' == buf[1])
		if (buf[2] && '\"' == buf[2])
			return(1);

	i = 1;
	while (buf[i] && ! isspace(buf[i]) && i < (int)sizeof(tmp))
		i++;

	if (i == (int)sizeof(tmp)) {
		mdoc->flags |= MDOC_HALT;
		return(mdoc_perr(mdoc, line, 1, ERR_MACRO_NOTSUP));
	} else if (i <= 2) {
		mdoc->flags |= MDOC_HALT;
		return(mdoc_perr(mdoc, line, 1, ERR_MACRO_NOTSUP));
	}

	i--;

	(void)memcpy(tmp, buf + 1, (size_t)i);
	tmp[i++] = 0;

	if (MDOC_MAX == (c = mdoc_find(mdoc, tmp))) {
		mdoc->flags |= MDOC_HALT;
		return(mdoc_perr(mdoc, line, 1, ERR_MACRO_NOTSUP));
	}

	while (buf[i] && isspace(buf[i]))
		i++;

	if ( ! mdoc_macro(mdoc, c, line, 1, &i, buf)) {
		mdoc->flags |= MDOC_HALT;
		return(0);
	}
	return(1);
}


void
mdoc_msg(struct mdoc *mdoc, const char *fmt, ...)
{
	struct mdoc_node *n;
	va_list		  ap;
	char		  buf[256];

	if (NULL == mdoc->cb.mdoc_msg)
		return;

	n = mdoc->last;
	assert(n);

	va_start(ap, fmt);
	(void)vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	(*mdoc->cb.mdoc_msg)(mdoc->data, n->line, n->pos, buf);
}


int
mdoc_perr(struct mdoc *mdoc, 
		int line, int pos, enum mdoc_err type)
{

	if (NULL == mdoc->cb.mdoc_err)
		return(0);
	return((*mdoc->cb.mdoc_err)(mdoc->data, line, pos, type));
}


int
mdoc_pwarn(struct mdoc *mdoc, 
		int line, int pos, enum mdoc_warn type)
{

	if (NULL == mdoc->cb.mdoc_warn)
		return(0);
	return((*mdoc->cb.mdoc_warn)(mdoc->data, line, pos, type));
}


int
mdoc_macro(struct mdoc *mdoc, int tok, 
		int ln, int ppos, int *pos, char *buf)
{

	if ( ! (MDOC_PROLOGUE & mdoc_macros[tok].flags) &&
			SEC_PROLOGUE == mdoc->sec_lastn)
		return(mdoc_perr(mdoc, ln, ppos, ERR_SEC_PROLOGUE));

	if (NULL == (mdoc_macros[tok].fp))
		return(mdoc_perr(mdoc, ln, ppos, ERR_MACRO_NOTSUP));

	if (1 != ppos && ! (MDOC_CALLABLE & mdoc_macros[tok].flags))
		return(mdoc_perr(mdoc, ln, ppos, ERR_MACRO_NOTCALL));

	return((*mdoc_macros[tok].fp)(mdoc, tok, ln, ppos, pos, buf));
}


static int
mdoc_node_append(struct mdoc *mdoc, struct mdoc_node *p)
{
	const char	 *nn, *nt, *on, *ot, *act;

	assert(mdoc->last);
	assert(mdoc->first);
	assert(MDOC_ROOT != p->type);

	if (MDOC_TEXT == mdoc->last->type)
		on = "<text>";
	else if (MDOC_ROOT == mdoc->last->type)
		on = "<root>";
	else
		on = mdoc_macronames[mdoc->last->tok];

	if (MDOC_TEXT == p->type)
		nn = "<text>";
	else if (MDOC_ROOT == p->type)
		nn = "<root>";
	else
		nn = mdoc_macronames[p->tok];

	ot = mdoc_type2a(mdoc->last->type);
	nt = mdoc_type2a(p->type);

	switch (mdoc->next) {
	case (MDOC_NEXT_SIBLING):
		mdoc->last->next = p;
		p->prev = mdoc->last;
		p->parent = mdoc->last->parent;
		act = "sibling";
		break;
	case (MDOC_NEXT_CHILD):
		mdoc->last->child = p;
		p->parent = mdoc->last;
		act = "child";
		break;
	default:
		abort();
		/* NOTREACHED */
	}

	if ( ! mdoc_valid_pre(mdoc, p))
		return(0);
	if ( ! mdoc_action_pre(mdoc, p))
		return(0);

	switch (p->type) {
	case (MDOC_HEAD):
		assert(MDOC_BLOCK == p->parent->type);
		p->parent->data.block.head = p;
		break;
	case (MDOC_TAIL):
		assert(MDOC_BLOCK == p->parent->type);
		p->parent->data.block.tail = p;
		break;
	case (MDOC_BODY):
		assert(MDOC_BLOCK == p->parent->type);
		p->parent->data.block.body = p;
		break;
	default:
		break;
	}

	mdoc->last = p;
	mdoc_msg(mdoc, "parse: %s `%s' %s of %s `%s'", 
			nt, nn, act, ot, on);
	return(1);
}


int
mdoc_tail_alloc(struct mdoc *mdoc, int line, int pos, int tok)
{
	struct mdoc_node *p;

	assert(mdoc->first);
	assert(mdoc->last);

	p = xcalloc(1, sizeof(struct mdoc_node));

	p->line = line;
	p->pos = pos;
	p->type = MDOC_TAIL;
	p->tok = tok;

	return(mdoc_node_append(mdoc, p));
}


int
mdoc_head_alloc(struct mdoc *mdoc, int line, int pos, int tok)
{
	struct mdoc_node *p;

	assert(mdoc->first);
	assert(mdoc->last);

	p = xcalloc(1, sizeof(struct mdoc_node));

	p->line = line;
	p->pos = pos;
	p->type = MDOC_HEAD;
	p->tok = tok;

	return(mdoc_node_append(mdoc, p));
}


int
mdoc_body_alloc(struct mdoc *mdoc, int line, int pos, int tok)
{
	struct mdoc_node *p;

	assert(mdoc->first);
	assert(mdoc->last);

	p = xcalloc(1, sizeof(struct mdoc_node));

	p->line = line;
	p->pos = pos;
	p->type = MDOC_BODY;
	p->tok = tok;

	return(mdoc_node_append(mdoc, p));
}


int
mdoc_root_alloc(struct mdoc *mdoc)
{
	struct mdoc_node *p;

	p = xcalloc(1, sizeof(struct mdoc_node));

	p->type = MDOC_ROOT;

	return(mdoc_node_append(mdoc, p));
}


int
mdoc_block_alloc(struct mdoc *mdoc, int line, int pos, 
		int tok, size_t argsz, const struct mdoc_arg *args)
{
	struct mdoc_node *p;

	p = xcalloc(1, sizeof(struct mdoc_node));

	p->pos = pos;
	p->line = line;
	p->type = MDOC_BLOCK;
	p->tok = tok;
	p->data.block.argc = argsz;
	p->data.block.argv = argdup(argsz, args);

	return(mdoc_node_append(mdoc, p));
}


int
mdoc_elem_alloc(struct mdoc *mdoc, int line, int pos, 
		int tok, size_t argsz, const struct mdoc_arg *args)
{
	struct mdoc_node *p;

	p = xcalloc(1, sizeof(struct mdoc_node));

	p->line = line;
	p->pos = pos;
	p->type = MDOC_ELEM;
	p->tok = tok;
	p->data.elem.argc = argsz;
	p->data.elem.argv = argdup(argsz, args);

	return(mdoc_node_append(mdoc, p));
}


int
mdoc_word_alloc(struct mdoc *mdoc, 
		int line, int pos, const char *word)
{
	struct mdoc_node *p;

	p = xcalloc(1, sizeof(struct mdoc_node));
	p->line = line;
	p->pos = pos;
	p->type = MDOC_TEXT;
	p->data.text.string = xstrdup(word);

	return(mdoc_node_append(mdoc, p));
}


static void
argfree(size_t sz, struct mdoc_arg *p)
{
	int		 i, j;

	if (0 == sz)
		return;

	assert(p);
	/* LINTED */
	for (i = 0; i < (int)sz; i++)
		if (p[i].sz > 0) {
			assert(p[i].value);
			/* LINTED */
			for (j = 0; j < (int)p[i].sz; j++)
				free(p[i].value[j]);
			free(p[i].value);
		}
	free(p);
}


static void
mdoc_elem_free(struct mdoc_elem *p)
{

	argfree(p->argc, p->argv);
}


static void
mdoc_block_free(struct mdoc_block *p)
{

	argfree(p->argc, p->argv);
}


static void
mdoc_text_free(struct mdoc_text *p)
{

	if (p->string)
		free(p->string);
}


void
mdoc_node_free(struct mdoc_node *p)
{

	switch (p->type) {
	case (MDOC_TEXT):
		mdoc_text_free(&p->data.text);
		break;
	case (MDOC_ELEM):
		mdoc_elem_free(&p->data.elem);
		break;
	case (MDOC_BLOCK):
		mdoc_block_free(&p->data.block);
		break;
	default:
		break;
	}

	free(p);
}


static void
mdoc_node_freelist(struct mdoc_node *p)
{

	if (p->child)
		mdoc_node_freelist(p->child);
	if (p->next)
		mdoc_node_freelist(p->next);

	mdoc_node_free(p);
}


int
mdoc_find(const struct mdoc *mdoc, const char *key)
{

	return(mdoc_tokhash_find(mdoc->htab, key));
}


static void
argcpy(struct mdoc_arg *dst, const struct mdoc_arg *src)
{
	int		 i;

	dst->line = src->line;
	dst->pos = src->pos;
	dst->arg = src->arg;
	if (0 == (dst->sz = src->sz))
		return;
	dst->value = xcalloc(dst->sz, sizeof(char *));
	for (i = 0; i < (int)dst->sz; i++)
		dst->value[i] = xstrdup(src->value[i]);
}


static struct mdoc_arg *
argdup(size_t argsz, const struct mdoc_arg *args)
{
	struct mdoc_arg	*pp;
	int		 i;

	if (0 == argsz)
		return(NULL);

	pp = xcalloc((size_t)argsz, sizeof(struct mdoc_arg));
	for (i = 0; i < (int)argsz; i++)
		argcpy(&pp[i], &args[i]);

	return(pp);
}

