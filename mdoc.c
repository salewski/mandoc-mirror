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

/*
 * Main caller in the libmdoc library.  This begins the parsing routine,
 * handles allocation of data, and so forth.  Most of the "work" is done
 * in macro.c and validate.c.
 */

static	struct mdoc_node *mdoc_node_alloc(const struct mdoc *);
static	int		  mdoc_node_append(struct mdoc *, 
				struct mdoc_node *);

static	int		  parsetext(struct mdoc *, int, char *);
static	int		  parsemacro(struct mdoc *, int, char *);
static	int		  macrowarn(struct mdoc *, int, const char *);


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
	"Fr",		"Ud",		"Lb",		"Ap",
	"Lp",		"Lk",		"Mt",		"Brq",
	"Bro",		"Brc"
	};

const	char *const __mdoc_argnames[MDOC_ARG_MAX] = {		 
	"split",		"nosplit",		"ragged",
	"unfilled",		"literal",		"file",		 
	"offset",		"bullet",		"dash",		 
	"hyphen",		"item",			"enum",		 
	"tag",			"diag",			"hang",		 
	"ohang",		"inset",		"column",	 
	"width",		"compact",		"std",	 
	"filled",		"words",		"emphasis",
	"symbolic"
	};

const	char * const *mdoc_macronames = __mdoc_macronames;
const	char * const *mdoc_argnames = __mdoc_argnames;


const struct mdoc_node *
mdoc_node(const struct mdoc *mdoc)
{

	return(mdoc->first);
}


const struct mdoc_meta *
mdoc_meta(const struct mdoc *mdoc)
{

	return(&mdoc->meta);
}


void
mdoc_free(struct mdoc *mdoc)
{

	if (mdoc->first)
		mdoc_node_freelist(mdoc->first);
	if (mdoc->htab)
		mdoc_tokhash_free(mdoc->htab);
	if (mdoc->meta.title)
		free(mdoc->meta.title);
	if (mdoc->meta.os)
		free(mdoc->meta.os);
	if (mdoc->meta.name)
		free(mdoc->meta.name);
	if (mdoc->meta.arch)
		free(mdoc->meta.arch);
	if (mdoc->meta.vol)
		free(mdoc->meta.vol);

	free(mdoc);
}


struct mdoc *
mdoc_alloc(void *data, int pflags, const struct mdoc_cb *cb)
{
	struct mdoc	*p;

	p = xcalloc(1, sizeof(struct mdoc));

	p->data = data;
	if (cb)
		(void)memcpy(&p->cb, cb, sizeof(struct mdoc_cb));

	p->last = xcalloc(1, sizeof(struct mdoc_node));
	p->last->type = MDOC_ROOT;
	p->first = p->last;
	p->pflags = pflags;
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


/*
 * Main parse routine.  Parses a single line -- really just hands off to
 * the macro or text parser.
 */
int
mdoc_parseln(struct mdoc *m, int ln, char *buf)
{

	/* If in error-mode, then we parse no more. */

	if (MDOC_HALT & m->flags)
		return(0);

	return('.' == *buf ? parsemacro(m, ln, buf) :
			parsetext(m, ln, buf));
}


void
mdoc_vmsg(struct mdoc *mdoc, int ln, int pos, const char *fmt, ...)
{
	char		  buf[256];
	va_list		  ap;

	if (NULL == mdoc->cb.mdoc_msg)
		return;

	va_start(ap, fmt);
	(void)vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
	va_end(ap);
	(*mdoc->cb.mdoc_msg)(mdoc->data, ln, pos, buf);
}


int
mdoc_verr(struct mdoc *mdoc, int ln, int pos, 
		const char *fmt, ...)
{
	char		 buf[256];
	va_list		 ap;

	if (NULL == mdoc->cb.mdoc_err)
		return(0);

	va_start(ap, fmt);
	(void)vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
	va_end(ap);
	return((*mdoc->cb.mdoc_err)(mdoc->data, ln, pos, buf));
}


int
mdoc_vwarn(struct mdoc *mdoc, int ln, int pos, 
		enum mdoc_warn type, const char *fmt, ...)
{
	char		 buf[256];
	va_list		 ap;

	if (NULL == mdoc->cb.mdoc_warn)
		return(0);

	va_start(ap, fmt);
	(void)vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
	va_end(ap);
	return((*mdoc->cb.mdoc_warn)(mdoc->data, ln, pos, type, buf));
}


int
mdoc_macro(struct mdoc *m, int tok, 
		int ln, int pp, int *pos, char *buf)
{

	/* FIXME - these should happen during validation. */

	if (MDOC_PROLOGUE & mdoc_macros[tok].flags && 
			SEC_PROLOGUE != m->lastnamed)
		return(mdoc_perr(m, ln, pp, 
				"disallowed in document body"));

	if ( ! (MDOC_PROLOGUE & mdoc_macros[tok].flags) && 
			SEC_PROLOGUE == m->lastnamed)
		return(mdoc_perr(m, ln, pp, 
				"disallowed in prologue"));

	if (1 != pp && ! (MDOC_CALLABLE & mdoc_macros[tok].flags))
		return(mdoc_perr(m, ln, pp, "not callable"));

	return((*mdoc_macros[tok].fp)(m, tok, ln, pp, pos, buf));
}


static int
mdoc_node_append(struct mdoc *mdoc, struct mdoc_node *p)
{

	assert(mdoc->last);
	assert(mdoc->first);
	assert(MDOC_ROOT != p->type);

	switch (mdoc->next) {
	case (MDOC_NEXT_SIBLING):
		mdoc->last->next = p;
		p->prev = mdoc->last;
		p->parent = mdoc->last->parent;
		break;
	case (MDOC_NEXT_CHILD):
		mdoc->last->child = p;
		p->parent = mdoc->last;
		break;
	default:
		abort();
		/* NOTREACHED */
	}

	if ( ! mdoc_valid_pre(mdoc, p))
		return(0);

	switch (p->type) {
	case (MDOC_HEAD):
		assert(MDOC_BLOCK == p->parent->type);
		p->parent->head = p;
		break;
	case (MDOC_TAIL):
		assert(MDOC_BLOCK == p->parent->type);
		p->parent->tail = p;
		break;
	case (MDOC_BODY):
		assert(MDOC_BLOCK == p->parent->type);
		p->parent->body = p;
		break;
	default:
		break;
	}

	mdoc->last = p;
	return(1);
}


static struct mdoc_node *
mdoc_node_alloc(const struct mdoc *mdoc)
{
	struct mdoc_node *p;

	p = xcalloc(1, sizeof(struct mdoc_node));
	p->sec = mdoc->lastsec;

	return(p);
}


int
mdoc_tail_alloc(struct mdoc *mdoc, int line, int pos, int tok)
{
	struct mdoc_node *p;

	assert(mdoc->first);
	assert(mdoc->last);

	p = mdoc_node_alloc(mdoc);

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

	p = mdoc_node_alloc(mdoc);

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

	p = mdoc_node_alloc(mdoc);

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

	p = mdoc_node_alloc(mdoc);

	p->type = MDOC_ROOT;

	return(mdoc_node_append(mdoc, p));
}


int
mdoc_block_alloc(struct mdoc *mdoc, int line, int pos, 
		int tok, struct mdoc_arg *args)
{
	struct mdoc_node *p;

	p = mdoc_node_alloc(mdoc);

	p->pos = pos;
	p->line = line;
	p->type = MDOC_BLOCK;
	p->tok = tok;
	p->args = args;

	if (args)
		(args->refcnt)++;

	return(mdoc_node_append(mdoc, p));
}


int
mdoc_elem_alloc(struct mdoc *mdoc, int line, int pos, 
		int tok, struct mdoc_arg *args)
{
	struct mdoc_node *p;

	p = mdoc_node_alloc(mdoc);

	p->line = line;
	p->pos = pos;
	p->type = MDOC_ELEM;
	p->tok = tok;
	p->args = args;

	if (args)
		(args->refcnt)++;

	return(mdoc_node_append(mdoc, p));
}


int
mdoc_word_alloc(struct mdoc *mdoc, 
		int line, int pos, const char *word)
{
	struct mdoc_node *p;

	p = mdoc_node_alloc(mdoc);

	p->line = line;
	p->pos = pos;
	p->type = MDOC_TEXT;
	p->string = xstrdup(word);

	return(mdoc_node_append(mdoc, p));
}


void
mdoc_node_free(struct mdoc_node *p)
{

	if (p->string)
		free(p->string);
	if (p->args)
		mdoc_argv_free(p->args);
	free(p);
}


void
mdoc_node_freelist(struct mdoc_node *p)
{

	if (p->child)
		mdoc_node_freelist(p->child);
	if (p->next)
		mdoc_node_freelist(p->next);

	mdoc_node_free(p);
}


/*
 * Parse free-form text, that is, a line that does not begin with the
 * control character.
 */
static int
parsetext(struct mdoc *mdoc, int line, char *buf)
{

	if (SEC_PROLOGUE == mdoc->lastnamed)
		return(mdoc_perr(mdoc, line, 0,
			"text disallowed in prologue"));

	if ( ! mdoc_word_alloc(mdoc, line, 0, buf))
		return(0);

	mdoc->next = MDOC_NEXT_SIBLING;
	return(1);
}


static int
macrowarn(struct mdoc *m, int ln, const char *buf)
{
	if ( ! (MDOC_IGN_MACRO & m->pflags))
		return(mdoc_perr(m, ln, 1, "unknown macro: %s%s", 
				buf, strlen(buf) > 3 ? "..." : ""));
	return(mdoc_pwarn(m, ln, 1, WARN_SYNTAX,
				"unknown macro: %s%s",
				buf, strlen(buf) > 3 ? "..." : ""));
}



/*
 * Parse a macro line, that is, a line beginning with the control
 * character.
 */
int
parsemacro(struct mdoc *m, int ln, char *buf)
{
	int		  i, c;
	char		  mac[5];

	/* Comments are quickly ignored. */

	if (buf[1] && '\\' == buf[1])
		if (buf[2] && '\"' == buf[2])
			return(1);

	/* Copy the first word into a nil-terminated buffer. */

	for (i = 1; i < 5; i++) {
		if (0 == (mac[i - 1] = buf[i]))
			break;
		else if (isspace((unsigned char)buf[i]))
			break;
	}

	mac[i - 1] = 0;

	if (i == 5 || i <= 2) {
		if ( ! macrowarn(m, ln, mac))
			goto err;
		return(1);
	} 
	
	if (MDOC_MAX == (c = mdoc_tokhash_find(m->htab, mac))) {
		if ( ! macrowarn(m, ln, mac))
			goto err;
		return(1);
	}

	/* The macro is sane.  Jump to the next word. */

	while (buf[i] && isspace((unsigned char)buf[i]))
		i++;

	/* Begin recursive parse sequence. */

	if ( ! mdoc_macro(m, c, ln, 1, &i, buf)) 
		goto err;

	return(1);

err:	/* Error out. */

	m->flags |= MDOC_HALT;
	return(0);
}
