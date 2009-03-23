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
#include <ctype.h>
#include <err.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "libman.h"

const	char *const __man_macronames[MAN_MAX] = {		 
	"\\\"",		"TH",		"SH",		"SS",
	"TP", 		"LP",		"PP",		"P",
	"IP",		"HP",		"SM",		"SB",
	"BI",		"IB",		"BR",		"RB",
	"R",		"B",		"I"
	};

const	char * const *man_macronames = __man_macronames;

static	struct man_node	*man_node_alloc(int, int, enum man_type);
static	int		 man_node_append(struct man *, 
				struct man_node *);
static	int		 man_ptext(struct man *, int, char *);
static	int		 man_pmacro(struct man *, int, char *);


const struct man_node *
man_node(const struct man *man)
{

	return(man->first);
}


const struct man_meta *
man_meta(const struct man *man)
{

	return(&man->meta);
}


void
man_reset(struct man *man)
{

	if (man->first)
		man_node_freelist(man->first);
	if (man->meta.title)
		free(man->meta.title);
	if (man->meta.os)
		free(man->meta.os);
	if (man->meta.vol)
		free(man->meta.vol);

	bzero(&man->meta, sizeof(struct man_meta));
	man->flags = 0;
	if (NULL == (man->last = calloc(1, sizeof(struct man_node))))
		err(1, "malloc");
	man->first = man->last;
	man->last->type = MAN_ROOT;
	man->next = MAN_NEXT_CHILD;
}


void
man_free(struct man *man)
{

	if (man->first)
		man_node_freelist(man->first);
	if (man->meta.title)
		free(man->meta.title);
	if (man->meta.os)
		free(man->meta.os);
	if (man->meta.vol)
		free(man->meta.vol);
	if (man->htab)
		man_hash_free(man->htab);
	free(man);
}


struct man *
man_alloc(void)
{
	struct man	*p;

	if (NULL == (p = calloc(1, sizeof(struct man))))
		err(1, "malloc");
	if (NULL == (p->last = calloc(1, sizeof(struct man_node))))
		err(1, "malloc");

	p->first = p->last;
	p->last->type = MAN_ROOT;
	p->next = MAN_NEXT_CHILD;
	p->htab = man_hash_alloc();
	return(p);
}


int
man_endparse(struct man *m)
{

	return(1);
}


int
man_parseln(struct man *m, int ln, char *buf)
{

	return('.' == *buf ? 
			man_pmacro(m, ln, buf) : 
			man_ptext(m, ln, buf));
}


static int
man_node_append(struct man *man, struct man_node *p)
{

	assert(man->last);
	assert(man->first);
	assert(MAN_ROOT != p->type);

	switch (man->next) {
	case (MAN_NEXT_SIBLING):
		man->last->next = p;
		p->prev = man->last;
		p->parent = man->last->parent;
		break;
	case (MAN_NEXT_CHILD):
		man->last->child = p;
		p->parent = man->last;
		break;
	default:
		abort();
		/* NOTREACHED */
	}

#if 0
	if ( ! man_valid_pre(man, p))
		return(0);
	if ( ! man_action_pre(man, p))
		return(0);
#endif

	switch (p->type) {
	case (MAN_HEAD):
		assert(MAN_BLOCK == p->parent->type);
		p->parent->head = p;
		break;
	case (MAN_BODY):
		assert(MAN_BLOCK == p->parent->type);
		p->parent->body = p;
		break;
	default:
		break;
	}

	man->last = p;
	return(1);
}


static struct man_node *
man_node_alloc(int line, int pos, enum man_type type)
{
	struct man_node *p;

	if (NULL == (p = calloc(1, sizeof(struct man_node))))
		err(1, "malloc");
	p->line = line;
	p->pos = pos;
	p->type = type;

	return(p);
}


int
man_head_alloc(struct man *man, int line, int pos, int tok)
{
	struct man_node	*p;

	p = man_node_alloc(line, pos, MAN_HEAD);
	p->tok = tok;

	return(man_node_append(man, p));
}


int
man_body_alloc(struct man *man, int line, int pos, int tok)
{
	struct man_node	*p;

	p = man_node_alloc(line, pos, MAN_BODY);
	p->tok = tok;

	return(man_node_append(man, p));
}


int
man_block_alloc(struct man *man, int line, int pos, int tok)
{
	struct man_node	*p;

	p = man_node_alloc(line, pos, MAN_BLOCK);
	p->tok = tok;

	return(man_node_append(man, p));
}


int
man_elem_alloc(struct man *man, int line, int pos, int tok)
{
	struct man_node *p;

	p = man_node_alloc(line, pos, MAN_ELEM);
	p->tok = tok;

	return(man_node_append(man, p));
}


int
man_word_alloc(struct man *man, 
		int line, int pos, const char *word)
{
	struct man_node	*p;

	p = man_node_alloc(line, pos, MAN_TEXT);
	if (NULL == (p->string = strdup(word)))
		err(1, "strdup");

	return(man_node_append(man, p));
}


void
man_node_free(struct man_node *p)
{

	if (p->string)
		free(p->string);
	free(p);
}


void
man_node_freelist(struct man_node *p)
{

	if (p->child)
		man_node_freelist(p->child);
	if (p->next)
		man_node_freelist(p->next);

	man_node_free(p);
}


static int
man_ptext(struct man *m, int line, char *buf)
{

	if (0 == buf[0]) {
		warnx("blank line!");
		return(1);
	}

	if ( ! man_word_alloc(m, line, 0, buf))
		return(0);

	m->next = MAN_NEXT_SIBLING;
	return(1);
}


int
man_pmacro(struct man *m, int ln, char *buf)
{
	int		  i, c;
	char		  mac[5];

	/* Comments and empties are quickly ignored. */

	if (0 == buf[1])
		return(1);

	if (' ' == buf[1]) {
		i = 2;
		while (buf[i] && ' ' == buf[i])
			i++;
		if (0 == buf[i])
			return(1);
		warnx("invalid syntax");
		return(0);
	}

	if (buf[1] && '\\' == buf[1])
		if (buf[2] && '\"' == buf[2])
			return(1);

	/* Copy the first word into a nil-terminated buffer. */

	for (i = 1; i < 5; i++) {
		if (0 == (mac[i - 1] = buf[i]))
			break;
		else if (' ' == buf[i])
			break;
	}

	mac[i - 1] = 0;

	if (i == 5 || i <= 1) {
		warnx("unknown macro: %s", mac);
		goto err;
	} 
	
	if (MAN_MAX == (c = man_hash_find(m->htab, mac))) {
		warnx("unknown macro: %s", mac);
		goto err;
	}

	/* The macro is sane.  Jump to the next word. */

	while (buf[i] && ' ' == buf[i])
		i++;

	/* Begin recursive parse sequence. */

	if ( ! (*man_macros[c].fp)(m, c, ln, 1, &i, buf))
		goto err;

	return(1);

err:	/* Error out. */

#if 0
	m->flags |= MDOC_HALT;
#endif
	return(0);
}
