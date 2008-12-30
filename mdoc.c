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
	};

const	struct mdoc_macro __mdoc_macros[MDOC_MAX] = {
	{ NULL, 0 }, /* \" */
	{ macro_prologue_ddate, 0 }, /* Dd */
	{ macro_prologue_dtitle, 0 }, /* Dt */
	{ macro_prologue_os, 0 }, /* Os */
	{ macro_scoped_implicit, 0 }, /* Sh */
	{ macro_scoped_implicit, 0 }, /* Ss */ 
	{ macro_text, 0 }, /* Pp */ 
	{ macro_scoped_line, 0 }, /* D1 */
	{ macro_scoped_line, 0 }, /* Dl */
	{ macro_scoped_explicit, MDOC_EXPLICIT }, /* Bd */
	{ macro_scoped_explicit, 0 }, /* Ed */
	{ macro_scoped_explicit, MDOC_EXPLICIT }, /* Bl */
	{ macro_scoped_explicit, 0 }, /* El */
	{ NULL, 0 }, /* It */
	{ macro_text, MDOC_CALLABLE }, /* Ad */ 
	{ NULL, 0 }, /* An */ 
	{ macro_text, MDOC_CALLABLE }, /* Ar */
	{ NULL, 0 }, /* Cd */
	{ macro_text, MDOC_CALLABLE }, /* Cm */
	{ macro_text, MDOC_CALLABLE }, /* Dv */ 
	{ macro_text, MDOC_CALLABLE }, /* Er */ 
	{ macro_text, MDOC_CALLABLE }, /* Ev */ 
	{ macro_constant_argv, 0 }, /* Ex */
	{ macro_text, MDOC_CALLABLE }, /* Fa */ 
	{ NULL, 0 }, /* Fd */ 
	{ macro_text, MDOC_CALLABLE }, /* Fl */
	{ NULL, 0 }, /* Fn */ 
	{ macro_text, 0 }, /* Ft */ 
	{ macro_text, MDOC_CALLABLE }, /* Ic */ 
	{ NULL, 0 }, /* In */ 
	{ macro_text, MDOC_CALLABLE }, /* Li */
	{ macro_constant, 0 }, /* Nd */ 
	{ NULL, 0 }, /* Nm */ 
	{ NULL, 0 }, /* Op */
	{ NULL, 0 }, /* Ot */
	{ macro_text, MDOC_CALLABLE }, /* Pa */
	{ macro_constant_argv, 0 }, /* Rv */
	{ NULL, 0 }, /* St */
	{ macro_text, MDOC_CALLABLE }, /* Va */
	{ macro_text, MDOC_CALLABLE }, /* Vt */ 
	{ NULL, 0 }, /* Xr */
	{ NULL, 0 }, /* %A */
	{ NULL, 0 }, /* %B */
	{ NULL, 0 }, /* %D */
	{ NULL, 0 }, /* %I */
	{ NULL, 0 }, /* %J */
	{ NULL, 0 }, /* %N */
	{ NULL, 0 }, /* %O */
	{ NULL, 0 }, /* %P */
	{ NULL, 0 }, /* %R */
	{ NULL, 0 }, /* %T */
	{ NULL, 0 }, /* %V */
	{ NULL, 0 }, /* Ac */
	{ NULL, 0 }, /* Ao */
	{ macro_scoped_pline, MDOC_CALLABLE }, /* Aq */
	{ macro_constant, 0 }, /* At */
	{ NULL, 0 }, /* Bc */
	{ NULL, 0 }, /* Bf */ 
	{ NULL, 0 }, /* Bo */
	{ macro_scoped_pline, MDOC_CALLABLE }, /* Bq */
	{ macro_constant_delimited, 0 }, /* Bsx */
	{ macro_constant_delimited, 0 }, /* Bx */
	{ NULL, 0 }, /* Db */
	{ NULL, 0 }, /* Dc */
	{ NULL, 0 }, /* Do */
	{ macro_scoped_pline, MDOC_CALLABLE }, /* Dq */
	{ NULL, 0 }, /* Ec */
	{ NULL, 0 }, /* Ef */
	{ macro_text, MDOC_CALLABLE }, /* Em */ 
	{ NULL, 0 }, /* Eo */
	{ macro_constant_delimited, 0 }, /* Fx */
	{ macro_text, 0 }, /* Ms */
	{ NULL, 0 }, /* No */
	{ NULL, 0 }, /* Ns */
	{ macro_constant_delimited, 0 }, /* Nx */
	{ macro_constant_delimited, 0 }, /* Ox */
	{ NULL, 0 }, /* Pc */
	{ NULL, 0 }, /* Pf */
	{ NULL, 0 }, /* Po */
	{ macro_scoped_pline, MDOC_CALLABLE }, /* Pq */
	{ NULL, 0 }, /* Qc */
	{ macro_scoped_pline, MDOC_CALLABLE }, /* Ql */
	{ NULL, 0 }, /* Qo */
	{ macro_scoped_pline, MDOC_CALLABLE }, /* Qq */
	{ NULL, 0 }, /* Re */
	{ NULL, 0 }, /* Rs */
	{ NULL, 0 }, /* Sc */
	{ NULL, 0 }, /* So */
	{ macro_scoped_pline, MDOC_CALLABLE }, /* Sq */
	{ NULL, 0 }, /* Sm */
	{ macro_text, MDOC_CALLABLE }, /* Sx */
	{ macro_text, MDOC_CALLABLE }, /* Sy */
	{ macro_text, MDOC_CALLABLE }, /* Tn */
	{ macro_constant_delimited, 0 }, /* Ux */
	{ NULL, 0 }, /* Xc */
	{ NULL, 0 }, /* Xo */
	{ NULL, 0 }, /* Fo */ 
	{ NULL, 0 }, /* Fc */ 
	{ NULL, 0 }, /* Oo */
	{ NULL, 0 }, /* Oc */
	{ NULL, 0 }, /* Bk */
	{ NULL, 0 }, /* Ek */
	{ macro_constant, 0 }, /* Bt */
	{ macro_constant, 0 }, /* Hf */
	{ NULL, 0 }, /* Fr */
	{ macro_constant, 0 }, /* Ud */
};

const	char * const *mdoc_macronames = __mdoc_macronames;
const	char * const *mdoc_argnames = __mdoc_argnames;
const	struct mdoc_macro * const mdoc_macros = __mdoc_macros;


static	struct mdoc_arg	 *argdup(size_t, const struct mdoc_arg *);
static	void		  argfree(size_t, struct mdoc_arg *);
static	void	  	  argcpy(struct mdoc_arg *, 
				const struct mdoc_arg *);
static	char		**paramdup(size_t, const char **);
static	void		  paramfree(size_t, char **);

static	void		  mdoc_node_freelist(struct mdoc_node *);
static	void		  mdoc_node_append(struct mdoc *, int, 
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

	p->htab = mdoc_tokhash_alloc();
	return(p);
}


int
mdoc_parseln(struct mdoc *mdoc, char *buf)
{
	int		  c, i;
	char		  tmp[5];

	if ('.' != *buf) {
		mdoc_word_alloc(mdoc, 0, buf);
		return(1);
	}

	if (buf[1] && '\\' == buf[1])
		if (buf[2] && '\"' == buf[2])
			return(1);

	i = 1;
	while (buf[i] && ! isspace(buf[i]) && i < (int)sizeof(tmp))
		i++;

	if (i == (int)sizeof(tmp))
		return(mdoc_err(mdoc, -1, 1, ERR_MACRO_NOTSUP));
	else if (i <= 2)
		return(mdoc_err(mdoc, -1, 1, ERR_MACRO_NOTSUP));

	i--;

	(void)memcpy(tmp, buf + 1, (size_t)i);
	tmp[i++] = 0;

	if (MDOC_MAX == (c = mdoc_find(mdoc, tmp)))
		return(mdoc_err(mdoc, c, 1, ERR_MACRO_NOTSUP));

	while (buf[i] && isspace(buf[i]))
		i++;

	return(mdoc_macro(mdoc, c, 1, &i, buf));
}


void
mdoc_msg(struct mdoc *mdoc, int pos, const char *fmt, ...)
{
	va_list		 ap;
	char		 buf[256];

	if (NULL == mdoc->cb.mdoc_msg)
		return;

	va_start(ap, fmt);
	(void)vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	(*mdoc->cb.mdoc_msg)(mdoc->data, pos, buf);
}


int
mdoc_err(struct mdoc *mdoc, int tok, int pos, enum mdoc_err type)
{

	if (NULL == mdoc->cb.mdoc_err)
		return(0);
	return((*mdoc->cb.mdoc_err)(mdoc->data, tok, pos, type));
}


int
mdoc_warn(struct mdoc *mdoc, int tok, int pos, enum mdoc_warn type)
{

	if (NULL == mdoc->cb.mdoc_warn)
		return(0);
	return((*mdoc->cb.mdoc_warn)(mdoc->data, tok, pos, type));
}


int
mdoc_macro(struct mdoc *mdoc, int tok, int ppos, int *pos, char *buf)
{

	if (NULL == (mdoc_macros[tok].fp)) {
		(void)mdoc_err(mdoc, tok, ppos, ERR_MACRO_NOTSUP);
		return(0);
	}

	if (1 != ppos && ! (MDOC_CALLABLE & mdoc_macros[tok].flags)) {
		(void)mdoc_err(mdoc, tok, ppos, ERR_MACRO_NOTCALL);
		return(0);
	}

	/*mdoc_msg(mdoc, ppos, "calling `%s'", mdoc_macronames[tok]);*/

	return((*mdoc_macros[tok].fp)(mdoc, tok, ppos, pos, buf));
}


static void
mdoc_node_append(struct mdoc *mdoc, int pos, struct mdoc_node *p)
{
	const char	 *nn, *on, *nt, *ot, *act;

	switch (p->type) {
	case (MDOC_TEXT):
		nn = p->data.text.string;
		nt = "text";
		break;
	case (MDOC_BODY):
		nn = mdoc_macronames[p->data.body.tok];
		nt = "body";
		break;
	case (MDOC_ELEM):
		nn = mdoc_macronames[p->data.elem.tok];
		nt = "elem";
		break;
	case (MDOC_HEAD):
		nn = mdoc_macronames[p->data.head.tok];
		nt = "head";
		break;
	case (MDOC_BLOCK):
		nn = mdoc_macronames[p->data.block.tok];
		nt = "block";
		break;
	default:
		abort();
		/* NOTREACHED */
	}

	if (NULL == mdoc->first) {
		assert(NULL == mdoc->last);
		mdoc->first = p;
		mdoc->last = p;
		mdoc_msg(mdoc, pos, "parse: root %s `%s'", nt, nn);
		return;
	}

	switch (mdoc->last->type) {
	case (MDOC_TEXT):
		on = "<text>";
		ot = "text";
		break;
	case (MDOC_BODY):
		on = mdoc_macronames[mdoc->last->data.body.tok];
		ot = "body";
		break;
	case (MDOC_ELEM):
		on = mdoc_macronames[mdoc->last->data.elem.tok];
		ot = "elem";
		break;
	case (MDOC_HEAD):
		on = mdoc_macronames[mdoc->last->data.head.tok];
		ot = "head";
		break;
	case (MDOC_BLOCK):
		on = mdoc_macronames[mdoc->last->data.block.tok];
		ot = "block";
		break;
	default:
		abort();
		/* NOTREACHED */
	}

	switch (p->type) {
	case (MDOC_BODY):
		p->parent = mdoc->last->parent;
		mdoc->last->next = p;
		p->prev = mdoc->last;
		act = "sibling";
		break;

	case (MDOC_HEAD):
		assert(mdoc->last->type == MDOC_BLOCK);
		p->parent = mdoc->last;
		mdoc->last->child = p;
		act = "child";
		break;

	default:
		switch (mdoc->last->type) {
		case (MDOC_BODY):
			/* FALLTHROUGH */
		case (MDOC_HEAD):
			p->parent = mdoc->last;
			mdoc->last->child = p;
			act = "child";
			break;
		default:
			p->parent = mdoc->last->parent;
			p->prev = mdoc->last;
			mdoc->last->next = p;
			act = "sibling";
			break;
		}
		break;
	}

	mdoc_msg(mdoc, pos, "parse: %s `%s' %s %s `%s'", 
			nt, nn, act, ot, on);
	mdoc->last = p;
}


/* FIXME: deprecate paramsz, params. */
void
mdoc_head_alloc(struct mdoc *mdoc, int pos, int tok, 
		size_t paramsz, const char **params)
{
	struct mdoc_node *p;

	assert(mdoc->first);
	assert(mdoc->last);
	assert(mdoc->last->type == MDOC_BLOCK);
	assert(mdoc->last->data.block.tok == tok);

	p = xcalloc(1, sizeof(struct mdoc_node));
	p->type = MDOC_HEAD;
	p->data.head.tok = tok;
	p->data.head.sz = paramsz;
	p->data.head.args = paramdup(paramsz, params);

	mdoc_node_append(mdoc, pos, p);
}


void
mdoc_body_alloc(struct mdoc *mdoc, int pos, int tok)
{
	struct mdoc_node *p;

	assert(mdoc->first);
	assert(mdoc->last);
	assert((mdoc->last->type == MDOC_BLOCK) ||
			(mdoc->last->type == MDOC_HEAD));
	if (mdoc->last->type == MDOC_BLOCK) 
		assert(mdoc->last->data.block.tok == tok);
	else
		assert(mdoc->last->data.head.tok == tok);

	p = xcalloc(1, sizeof(struct mdoc_node));

	p->type = MDOC_BODY;
	p->data.body.tok = tok;

	mdoc_node_append(mdoc, pos, p);
}


void
mdoc_block_alloc(struct mdoc *mdoc, int pos, int tok,
		size_t argsz, const struct mdoc_arg *args)
{
	struct mdoc_node *p;

	p = xcalloc(1, sizeof(struct mdoc_node));

	p->type = MDOC_BLOCK;
	p->data.block.tok = tok;
	p->data.block.argc = argsz;
	p->data.block.argv = argdup(argsz, args);

	mdoc_node_append(mdoc, pos, p);
}


void
mdoc_elem_alloc(struct mdoc *mdoc, int pos, int tok, 
		size_t argsz, const struct mdoc_arg *args, 
		size_t paramsz, const char **params)
{
	struct mdoc_node *p;

	p = xcalloc(1, sizeof(struct mdoc_node));
	p->type = MDOC_ELEM;
	p->data.elem.tok = tok;
	p->data.elem.sz = paramsz;
	p->data.elem.args = paramdup(paramsz, params);
	p->data.elem.argc = argsz;
	p->data.elem.argv = argdup(argsz, args);

	mdoc_node_append(mdoc, pos, p);
}


void
mdoc_word_alloc(struct mdoc *mdoc, int pos, const char *word)
{
	struct mdoc_node *p;

	p = xcalloc(1, sizeof(struct mdoc_node));
	p->type = MDOC_TEXT;
	p->data.text.string = xstrdup(word);

	mdoc_node_append(mdoc, pos, p);
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
		}
	free(p);
}


static void
mdoc_elem_free(struct mdoc_elem *p)
{

	paramfree(p->sz, p->args);
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


static void
mdoc_head_free(struct mdoc_head *p)
{

	paramfree(p->sz, p->args);
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
	case (MDOC_HEAD):
		mdoc_head_free(&p->data.head);
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


static void
paramfree(size_t sz, char **p)
{
	int		 i;

	if (0 == sz)
		return;

	assert(p);
	/* LINTED */
	for (i = 0; i < (int)sz; i++)
		free(p[i]);
	free(p);
}


static char **
paramdup(size_t sz, const char **p)
{
	char		**pp;
	int		  i;

	if (0 == sz)
		return(NULL);

	pp = xcalloc(sz, sizeof(char *));
	for (i = 0; i < (int)sz; i++) 
		pp[i] = xstrdup(p[i]);

	return(pp);
}
