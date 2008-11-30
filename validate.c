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
#include <sys/param.h>

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libmdocml.h"
#include "private.h"

#define	INDENT		 4

#ifdef	__linux__ /* FIXME */
#define	strlcat		 strncat
#endif

struct	md_valid {
	const struct md_args	*args;
	const struct md_rbuf	*rbuf;
	struct md_mbuf	*mbuf;
	struct rofftree	*tree;

	size_t		 indent;
	size_t		 pos;

	int		 flags;
#define	MD_LITERAL	(1 << 0)
};

static	void		 roffmsg(void *arg, enum roffmsg, 
				const char *, const char *, char *);
static	int		 roffhead(void *);
static	int		 rofftail(void *);
static	int		 roffin(void *, int, int *, char **);
static	int		 roffdata(void *, int, char *);
static	int		 roffout(void *, int);
static	int		 roffblkin(void *, int, int *, char **);
static	int		 roffblkout(void *, int);
static	int		 roffspecial(void *, int);

static	int		 mbuf_newline(struct md_valid *);
static	int		 mbuf_indent(struct md_valid *);
static	int		 mbuf_data(struct md_valid *, int, char *);


static int
mbuf_indent(struct md_valid *p)
{
	size_t		 i;

	assert(p->pos == 0);

	/* LINTED */
	for (i = 0; i < MIN(p->indent, INDENT); i++)
		if ( ! md_buf_putstring(p->mbuf, "    "))
			return(0);

	p->pos += i * INDENT;
	return(1);
}


static int
mbuf_newline(struct md_valid *p)
{

	if ( ! md_buf_putchar(p->mbuf, '\n'))
		return(0);

	p->pos = 0;
	return(1);
}


static int
mbuf_data(struct md_valid *p, int space, char *buf)
{
	size_t		 sz;
	char		*bufp;

	assert(p->mbuf);
	assert(0 != p->indent);

	if (MD_LITERAL & p->flags)
		return(md_buf_putstring(p->mbuf, buf));

	while (*buf) {
		while (*buf && isspace(*buf))
			buf++;

		if (0 == *buf)
			break;

		bufp = buf;
		while (*buf && ! isspace(*buf))
			buf++;

		if (0 != *buf)
			*buf++ = 0;

		/* Process word. */

		sz = strlen(bufp);

		if (0 == p->pos) {
			if ( ! mbuf_indent(p))
				return(0);
			if ( ! md_buf_putstring(p->mbuf, bufp))
				return(0);

			if (p->indent * INDENT + sz >= 72) {
				if ( ! mbuf_newline(p))
					return(0);
				continue;
			}

			p->pos += sz;
			continue;
		}

		/* 
		 * FIXME: punctuation shouldn't have a newline before
		 * it! 
		 */

		if (sz + p->pos >= 72) {
			if ( ! mbuf_newline(p))
				return(0);
			if ( ! mbuf_indent(p))
				return(0);
		} else if (space) 
			if ( ! md_buf_putchar(p->mbuf, ' '))
				return(0);

		if ( ! md_buf_putstring(p->mbuf, bufp))
			return(0);

		p->pos += sz + (size_t)(space ? 1 : 0);
	}

	return(1);
}


int
md_line_valid(void *arg, char *buf)
{
	struct md_valid	*p;

	p = (struct md_valid *)arg;
	return(roff_engine(p->tree, buf));
}


int
md_exit_valid(void *data, int flush)
{
	int		 c;
	struct md_valid	*p;

	p = (struct md_valid *)data;
	c = roff_free(p->tree, flush);
	free(p);

	return(c);
}


void *
md_init_valid(const struct md_args *args,
		struct md_mbuf *mbuf, const struct md_rbuf *rbuf)
{
	struct roffcb	 cb;
	struct md_valid	*p;

	cb.roffhead = roffhead;
	cb.rofftail = rofftail;
	cb.roffin = roffin;
	cb.roffout = roffout;
	cb.roffblkin = roffblkin;
	cb.roffblkout = roffblkout;
	cb.roffspecial = roffspecial;
	cb.roffmsg = roffmsg;
	cb.roffdata = roffdata;

	if (NULL == (p = calloc(1, sizeof(struct md_valid))))
		err(1, "malloc");

	p->args = args;
	p->mbuf = mbuf;
	p->rbuf = rbuf;

	assert(mbuf);

	if (NULL == (p->tree = roff_alloc(&cb, p))) {
		free(p);
		return(NULL);
	}

	return(p);
}


/* ARGSUSED */
static int
roffhead(void *arg)
{
	struct md_valid	*p;

	assert(arg);
	p = (struct md_valid *)arg;

	if ( ! md_buf_putstring(p->mbuf, "<?xml version=\"1.0\" "
				"encoding=\"UTF-8\"?>\n"))
		return(0);

	if ( ! md_buf_putstring(p->mbuf, "<mdoc>"))
		return(0);
	p->indent++;

	return(mbuf_newline(p));
}


static int
rofftail(void *arg)
{
	struct md_valid	*p;

	assert(arg);
	p = (struct md_valid *)arg;

	if (0 != p->pos && ! mbuf_newline(p))
		return(0);
	return(md_buf_putstring(p->mbuf, "</mdoc>\n"));
}


/* ARGSUSED */
static int
roffspecial(void *arg, int tok)
{

	return(1);
}


static int
roffblkin(void *arg, int tok, int *argc, char **argv)
{
	struct md_valid	*p;
	int		 i;

	assert(arg);
	p = (struct md_valid *)arg;

	if (0 != p->pos) {
		if ( ! mbuf_newline(p))
			return(0);
		if ( ! mbuf_indent(p))
			return(0);
	} else if ( ! mbuf_indent(p))
		return(0);

	if ( ! md_buf_putchar(p->mbuf, '<'))
		return(0);
	if ( ! md_buf_putstring(p->mbuf, toknames[tok]))
		return(0);

	for (i = 0; ROFF_ARGMAX != argc[i]; i++) {
		if ( ! md_buf_putchar(p->mbuf, ' '))
			return(0);
		if ( ! md_buf_putstring(p->mbuf, tokargnames[argc[i]]))
			return(0);
		if ( ! md_buf_putstring(p->mbuf, "=\""))
			return(0);
		if ( ! md_buf_putstring(p->mbuf, argv[i] ? 
					argv[i] : "true"))
			return(0);
		if ( ! md_buf_putstring(p->mbuf, "\""))
			return(0);
	}

	if ( ! md_buf_putchar(p->mbuf, '>'))
		return(0);
	if ( ! mbuf_newline(p))
		return(0);

	p->indent++;
	return(1);
}


static int
roffblkout(void *arg, int tok)
{
	struct md_valid	*p;

	assert(arg);
	p = (struct md_valid *)arg;

	p->indent--;

	if (0 != p->pos) {
		if ( ! mbuf_newline(p))
			return(0);
		if ( ! mbuf_indent(p))
			return(0);
	} else if ( ! mbuf_indent(p))
		return(0);

	if ( ! md_buf_putstring(p->mbuf, "</"))
		return(0);
	if ( ! md_buf_putstring(p->mbuf, toknames[tok]))
		return(0);
	if ( ! md_buf_putstring(p->mbuf, ">"))
		return(0);
	if ( ! mbuf_newline(p))
		return(0);

	return(1);
}


static int
roffin(void *arg, int tok, int *argc, char **argv)
{
	struct md_valid	*p;
	int		 i;

	assert(arg);
	p = (struct md_valid *)arg;

	if (0 == p->pos && ! mbuf_indent(p))
		return(0);

	/* FIXME: put into a buffer before writing (line length). */

	/* FIXME: not always with a space... */

	if ( ! md_buf_putstring(p->mbuf, " <"))
		return(0);
	if ( ! md_buf_putstring(p->mbuf, toknames[tok]))
		return(0);

	for (i = 0; ROFF_ARGMAX != argc[i]; i++) {
		if ( ! md_buf_putchar(p->mbuf, ' '))
			return(0);
		if ( ! md_buf_putstring(p->mbuf, tokargnames[argc[i]]))
			return(0);
		if ( ! md_buf_putstring(p->mbuf, "=\""))
			return(0);
		if ( ! md_buf_putstring(p->mbuf, argv[i] ? 
					argv[i] : "true"))
			return(0);
		if ( ! md_buf_putstring(p->mbuf, "\""))
			return(0);

		p->pos += strlen(toknames[tok]) + 4 +
			strlen(tokargnames[argc[i]]) +
			strlen(argv[i] ? argv[i] : "true");
	}

	if ( ! md_buf_putstring(p->mbuf, ">"))
		return(0);

	p->pos += strlen(toknames[tok]) + 3;

	return(1);
}


static int
roffout(void *arg, int tok)
{
	struct md_valid	*p;

	assert(arg);
	p = (struct md_valid *)arg;

	if (0 == p->pos && ! mbuf_indent(p))
		return(0);

	if ( ! md_buf_putstring(p->mbuf, "</"))
		return(0);
	if ( ! md_buf_putstring(p->mbuf, toknames[tok]))
		return(0);
	if ( ! md_buf_putstring(p->mbuf, ">"))
		return(0);

	p->pos += strlen(toknames[tok]) + 2;

	return(1);
}



static void
roffmsg(void *arg, enum roffmsg lvl, 
		const char *buf, const char *pos, char *msg)
{
	char		*level;
	struct md_valid	*p;

	assert(arg);
	p = (struct md_valid *)arg;

	switch (lvl) {
	case (ROFF_WARN):
		if ( ! (MD_WARN_ALL & p->args->warnings))
			return;
		level = "warning";
		break;
	case (ROFF_ERROR):
		level = "error";
		break;
	default:
		abort();
	}
	
	if (pos)
		(void)fprintf(stderr, "%s:%zu: %s: %s\n", 
				p->rbuf->name, p->rbuf->line, level, msg);
	else
		(void)fprintf(stderr, "%s: %s: %s\n", 
				p->rbuf->name, level, msg);

}


static int
roffdata(void *arg, int space, char *buf)
{
	struct md_valid	*p;

	assert(arg);
	p = (struct md_valid *)arg;
	return(mbuf_data(p, space, buf));
}
