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

#define	MAXINDENT	 8
#define	COLUMNS		 72

#ifdef	__linux__ /* FIXME */
#define	strlcat		 strncat
#endif

enum	md_ns {
	MD_NS_BLOCK,
	MD_NS_INLINE,
	MD_NS_DEFAULT
};

enum	md_tok {
	MD_BLKIN,
	MD_BLKOUT,
	MD_IN,
	MD_OUT,
	MD_TEXT,
	MD_OVERRIDE
};

struct	md_xml {
	const struct md_args	*args;
	const struct md_rbuf	*rbuf;

	struct md_mbuf	*mbuf;
	struct rofftree	*tree;
	size_t		 indent;
	size_t		 pos;
	enum md_tok	 last;
	int		 flags;
#define	MD_LITERAL	(1 << 0) /* FIXME */
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

static	int		 mbuf_newline(struct md_xml *);
static	int		 mbuf_indent(struct md_xml *);
static	int		 mbuf_data(struct md_xml *, int, char *);
static	int		 mbuf_putstring(struct md_xml *, 
				const char *);
static	int		 mbuf_nputstring(struct md_xml *, 
				const char *, size_t);
static	int		 mbuf_puts(struct md_xml *, const char *);
static	int		 mbuf_nputs(struct md_xml *, 
				const char *, size_t);
static	int		 mbuf_begintag(struct md_xml *, const char *, 
				enum md_ns, int *, char **);
static	int		 mbuf_endtag(struct md_xml *, 
				const char *, enum md_ns);


static int
mbuf_begintag(struct md_xml *p, const char *name, enum md_ns ns, 
		int *argc, char **argv)
{
	int		 i;

	if ( ! mbuf_nputs(p, "<", 1))
		return(0);

	switch (ns) {
		case (MD_NS_BLOCK):
			if ( ! mbuf_nputs(p, "block:", 6))
				return(0);
			break;
		case (MD_NS_INLINE):
			if ( ! mbuf_nputs(p, "inline:", 7))
				return(0);
			break;
		default:
			break;
	}

	if ( ! mbuf_puts(p, name))
		return(0);

	for (i = 0; ROFF_ARGMAX != argc[i]; i++) {
		if ( ! mbuf_nputs(p, " ", 1))
			return(0);
		if ( ! mbuf_puts(p, tokargnames[argc[i]]))
			return(0);
		if ( ! mbuf_nputs(p, "=\"", 2))
			return(0);
		if ( ! mbuf_putstring(p, argv[i] ? argv[i] : "true"))
			return(0);
		if ( ! mbuf_nputs(p, "\"", 1))
			return(0);
	}
	return(mbuf_nputs(p, ">", 1));
}


static int
mbuf_endtag(struct md_xml *p, const char *tag, enum md_ns ns)
{
	if ( ! mbuf_nputs(p, "</", 2))
		return(0);

	switch (ns) {
		case (MD_NS_BLOCK):
			if ( ! mbuf_nputs(p, "block:", 6))
				return(0);
			break;
		case (MD_NS_INLINE):
			if ( ! mbuf_nputs(p, "inline:", 7))
				return(0);
			break;
		default:
			break;
	}

	if ( ! mbuf_puts(p, tag))
		return(0);
	return(mbuf_nputs(p, ">", 1));
}


static int
mbuf_putstring(struct md_xml *p, const char *buf)
{

	return(mbuf_nputstring(p, buf, strlen(buf)));
}


static int
mbuf_nputstring(struct md_xml *p, const char *buf, size_t sz)
{
	int		 i;

	for (i = 0; i < (int)sz; i++) {
		switch (buf[i]) {
		case ('&'):
			if ( ! md_buf_puts(p->mbuf, "&amp;", 5))
				return(0);
			p->pos += 5;
			break;
		case ('"'):
			if ( ! md_buf_puts(p->mbuf, "&quot;", 6))
				return(0);
			p->pos += 6;
			break;
		case ('<'):
			if ( ! md_buf_puts(p->mbuf, "&lt;", 4))
				return(0);
			p->pos += 4;
			break;
		case ('>'):
			if ( ! md_buf_puts(p->mbuf, "&gt;", 4))
				return(0);
			p->pos += 4;
			break;
		default:
			if ( ! md_buf_putchar(p->mbuf, buf[i]))
				return(0);
			p->pos++;
			break;
		}
	}
	return(1);
}


static int
mbuf_nputs(struct md_xml *p, const char *buf, size_t sz)
{

	p->pos += sz;
	return(md_buf_puts(p->mbuf, buf, sz));
}


static int
mbuf_puts(struct md_xml *p, const char *buf)
{

	return(mbuf_nputs(p, buf, strlen(buf)));
}


static int
mbuf_indent(struct md_xml *p)
{
	size_t		 i;

	assert(p->pos == 0);

	/* LINTED */
	for (i = 0; i < MIN(p->indent, MAXINDENT); i++)
		if ( ! md_buf_putstring(p->mbuf, "    "))
			return(0);

	p->pos += i * 4;
	return(1);
}


static int
mbuf_newline(struct md_xml *p)
{

	if ( ! md_buf_putchar(p->mbuf, '\n'))
		return(0);

	p->pos = 0;
	return(1);
}


static int
mbuf_data(struct md_xml *p, int space, char *buf)
{
	size_t		 sz;
	char		*bufp;

	assert(p->mbuf);
	assert(0 != p->indent);

	if (MD_LITERAL & p->flags)
		return(mbuf_putstring(p, buf));

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

		sz = strlen(bufp);

		if (0 == p->pos) {
			if ( ! mbuf_indent(p))
				return(0);
			if ( ! mbuf_nputstring(p, bufp, sz))
				return(0);
			if (p->indent * MAXINDENT + sz >= COLUMNS) {
				if ( ! mbuf_newline(p))
					return(0);
				continue;
			}
			continue;
		}

		if (space && sz + p->pos >= COLUMNS) {
			if ( ! mbuf_newline(p))
				return(0);
			if ( ! mbuf_indent(p))
				return(0);
		} else if (space) {
			if ( ! mbuf_nputs(p, " ", 1))
				return(0);
		}

		if ( ! mbuf_nputstring(p, bufp, sz))
			return(0);

		if ( ! space && p->pos >= COLUMNS)
			if ( ! mbuf_newline(p))
				return(0);
	}

	return(1);
}


int
md_line_xml(void *arg, char *buf)
{
	struct md_xml	*p;

	p = (struct md_xml *)arg;
	return(roff_engine(p->tree, buf));
}


int
md_exit_xml(void *data, int flush)
{
	int		 c;
	struct md_xml	*p;

	p = (struct md_xml *)data;
	c = roff_free(p->tree, flush);
	free(p);

	return(c);
}


void *
md_init_xml(const struct md_args *args,
		struct md_mbuf *mbuf, const struct md_rbuf *rbuf)
{
	struct roffcb	 cb;
	struct md_xml	*p;

	cb.roffhead = roffhead;
	cb.rofftail = rofftail;
	cb.roffin = roffin;
	cb.roffout = roffout;
	cb.roffblkin = roffblkin;
	cb.roffblkout = roffblkout;
	cb.roffspecial = roffspecial;
	cb.roffmsg = roffmsg;
	cb.roffdata = roffdata;

	if (NULL == (p = calloc(1, sizeof(struct md_xml))))
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
	struct md_xml	*p;

	assert(arg);
	p = (struct md_xml *)arg;

	if ( ! mbuf_puts(p, "<?xml version=\"1.0\" "
				"encoding=\"UTF-8\"?>\n"))
		return(0);
	if ( ! mbuf_puts(p, "<mdoc xmlns:block=\"block\" "
				"xmlns:special=\"special\" "
				"xmlns:inline=\"inline\">"))
		return(0);

	p->indent++;
	p->last = MD_BLKIN;
	return(mbuf_newline(p));
}


static int
rofftail(void *arg)
{
	struct md_xml	*p;

	assert(arg);
	p = (struct md_xml *)arg;

	if (0 != p->pos && ! mbuf_newline(p))
		return(0);

	p->last = MD_BLKOUT;
	if ( ! mbuf_endtag(p, "mdoc", MD_NS_DEFAULT))
		return(0);

	return(mbuf_newline(p));
}


static int
roffspecial(void *arg, int tok)
{
	struct md_xml	*p;

	assert(arg);
	p = (struct md_xml *)arg;

	/* FIXME: this is completely ad hoc. */

	switch (tok) {
	case (ROFF_Ns):
		p->last = MD_OVERRIDE;
		break;
	default:
		break;
	}

	return(1);
}


static int
roffblkin(void *arg, int tok, int *argc, char **argv)
{
	struct md_xml	*p;

	assert(arg);
	p = (struct md_xml *)arg;

	if (0 != p->pos) {
		if ( ! mbuf_newline(p))
			return(0);
		if ( ! mbuf_indent(p))
			return(0);
	} else if ( ! mbuf_indent(p))
		return(0);

	/* FIXME: xml won't like standards args (e.g., p1003.1-90). */

	p->last = MD_BLKIN;
	p->indent++;

	if ( ! mbuf_begintag(p, toknames[tok], MD_NS_BLOCK,
				argc, argv))
		return(0);
	return(mbuf_newline(p));
}


static int
roffblkout(void *arg, int tok)
{
	struct md_xml	*p;

	assert(arg);
	p = (struct md_xml *)arg;

	p->indent--;

	if (0 != p->pos) {
		if ( ! mbuf_newline(p))
			return(0);
		if ( ! mbuf_indent(p))
			return(0);
	} else if ( ! mbuf_indent(p))
		return(0);

	p->last = MD_BLKOUT;

	if ( ! mbuf_endtag(p, toknames[tok], MD_NS_BLOCK))
		return(0);
	return(mbuf_newline(p));
}


static int
roffin(void *arg, int tok, int *argc, char **argv)
{
	struct md_xml	*p;

	assert(arg);
	p = (struct md_xml *)arg;

	/* 
	 * FIXME: put all of this in a buffer, then check the buffer
	 * length versus the column width for nicer output.  This is a
	 * bit hacky.
	 */

	if (p->pos + 11 > COLUMNS) 
		if ( ! mbuf_newline(p))
			return(0);

	if (0 != p->pos) {
		switch (p->last) {
		case (MD_TEXT):
			/* FALLTHROUGH */
		case (MD_OUT):
			if ( ! mbuf_nputs(p, " ", 1))
				return(0);
			break;
		default:
			break;
		}
	} else if ( ! mbuf_indent(p))
		return(0);

	p->last = MD_IN;
	return(mbuf_begintag(p, toknames[tok], 
				MD_NS_INLINE, argc, argv));
}


static int
roffout(void *arg, int tok)
{
	struct md_xml	*p;

	assert(arg);
	p = (struct md_xml *)arg;

	if (0 == p->pos && ! mbuf_indent(p))
		return(0);

	p->last = MD_OUT;
	return(mbuf_endtag(p, toknames[tok], MD_NS_INLINE));
}


static void
roffmsg(void *arg, enum roffmsg lvl, 
		const char *buf, const char *pos, char *msg)
{
	char		*level;
	struct md_xml	*p;

	assert(arg);
	p = (struct md_xml *)arg;

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
		(void)fprintf(stderr, "%s:%zu: %s: %s (column %zu)\n", 
				p->rbuf->name, p->rbuf->line, level, 
				msg, pos - buf);
	else
		(void)fprintf(stderr, "%s: %s: %s\n", 
				p->rbuf->name, level, msg);

}


static int
roffdata(void *arg, int space, char *buf)
{
	struct md_xml	*p;

	assert(arg);
	p = (struct md_xml *)arg;
	if ( ! mbuf_data(p, space, buf))
		return(0);

	p->last = MD_TEXT;
	return(1);
}

