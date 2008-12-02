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

#define	COLUMNS		 72

enum	md_ns {
	MD_NS_BLOCK,
	MD_NS_INLINE,
	MD_NS_DEFAULT
};

enum	md_tok {
	MD_BLKIN,		/* Controls spacing. */
	MD_BLKOUT,
	MD_IN,
	MD_OUT,
	MD_TEXT
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
#define	MD_LITERAL	(1 << 0) /* TODO */
#define	MD_OVERRIDE_ONE	(1 << 1)
#define	MD_OVERRIDE_ALL	(1 << 2)
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
static	int		 roffspecial(void *, int, int *, char **, char **);

static	void		 mbuf_mode(struct md_xml *, enum md_ns);
static	int		 mbuf_newline(struct md_xml *);
static	int		 xml_indent(struct md_xml *);
static	int		 mbuf_data(struct md_xml *, int, char *);
static	int		 xml_nputstring(struct md_xml *, 
				const char *, size_t);
static	int		 xml_puts(struct md_xml *, const char *);
static	int		 xml_nputs(struct md_xml *, 
				const char *, size_t);
static	int		 xml_begintag(struct md_xml *, const char *, 
				enum md_ns, int *, char **);
static	int		 xml_endtag(struct md_xml *, 
				const char *, enum md_ns);

#ifdef __linux__ /* FIXME: remove */
static	size_t		  strlcat(char *, const char *, size_t);
static	size_t		  strlcpy(char *, const char *, size_t);
#endif


static void
mbuf_mode(struct md_xml *p, enum md_ns ns)
{
	p->flags &= ~MD_OVERRIDE_ONE;
	p->last = ns;
}


static int
xml_begintag(struct md_xml *p, const char *name, enum md_ns ns, 
		int *argc, char **argv)
{
	char		 buf[64];
	ssize_t		 sz;
	size_t		 res;

	switch (ns) {
	case (MD_NS_BLOCK):
		res = strlcpy(buf, "block:", sizeof(buf));
		assert(res < sizeof(buf));
		break;
	case (MD_NS_INLINE):
		res = strlcpy(buf, "inline:", sizeof(buf));
		assert(res < sizeof(buf));
		break;
	default:
		*buf = 0;
		break;
	}

	res = strlcat(buf, name, sizeof(buf));
	assert(res < sizeof(buf));

	if (-1 == (sz = ml_begintag(p->mbuf, buf, argc, argv)))
		return(0);

	p->pos += sz;
	return(1);
}


static int
xml_endtag(struct md_xml *p, const char *name, enum md_ns ns)
{
	char		 buf[64];
	ssize_t		 sz;
	size_t		 res;

	switch (ns) {
	case (MD_NS_BLOCK):
		res = strlcpy(buf, "block:", sizeof(buf));
		assert(res < sizeof(buf));
		break;
	case (MD_NS_INLINE):
		res = strlcpy(buf, "inline:", sizeof(buf));
		assert(res < sizeof(buf));
		break;
	default:
		*buf = 0;
		break;
	}

	res = strlcat(buf, name, sizeof(buf));
	assert(res < sizeof(buf));

	if (-1 == (sz = ml_endtag(p->mbuf, buf)))
		return(0);

	p->pos += sz;
	return(1);
}


static int
xml_nputstring(struct md_xml *p, const char *buf, size_t sz)
{
	ssize_t		 res;

	if (-1 == (res = ml_nputstring(p->mbuf, buf, sz)))
		return(0);
	p->pos += res;
	return(1);
}


static int
xml_nputs(struct md_xml *p, const char *buf, size_t sz)
{
	ssize_t		 res;

	if (-1 == (res = ml_nputs(p->mbuf, buf, sz)))
		return(0);
	p->pos += res;
	return(1);
}


static int
xml_puts(struct md_xml *p, const char *buf)
{

	return(xml_nputs(p, buf, strlen(buf)));
}


static int
xml_indent(struct md_xml *p)
{
	ssize_t		 res;

	if (-1 == (res = ml_indent(p->mbuf, p->indent)))
		return(0);
	p->pos += res;
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

	if (MD_OVERRIDE_ONE & p->flags || MD_OVERRIDE_ALL & p->flags)
		space = 0;

	if (MD_LITERAL & p->flags)
		return(xml_nputstring(p, buf, sizeof(buf)));

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
			if ( ! xml_indent(p))
				return(0);
			if ( ! xml_nputstring(p, bufp, sz))
				return(0);
			if (p->indent * MAXINDENT + sz >= COLUMNS)
				if ( ! mbuf_newline(p))
					return(0);
			if ( ! (MD_OVERRIDE_ALL & p->flags))
				space = 1;
			continue;
		}

		if (space && sz + p->pos >= COLUMNS) {
			if ( ! mbuf_newline(p))
				return(0);
			if ( ! xml_indent(p))
				return(0);
		} else if (space) {
			if ( ! xml_nputs(p, " ", 1))
				return(0);
		}

		if ( ! xml_nputstring(p, bufp, sz))
			return(0);

		if ( ! (MD_OVERRIDE_ALL & p->flags))
			space = 1;
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

	if (-1 == xml_puts(p, "<?xml version=\"1.0\" "
				"encoding=\"UTF-8\"?>\n"))
		return(0);
	if (-1 == xml_puts(p, "<mdoc xmlns:block=\"block\" "
				"xmlns:special=\"special\" "
				"xmlns:inline=\"inline\">"))
		return(0);

	p->indent++;
	mbuf_mode(p, MD_BLKIN);
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

	mbuf_mode(p, MD_BLKOUT);
	if ( ! xml_endtag(p, "mdoc", MD_NS_DEFAULT))
		return(0);
	return(mbuf_newline(p));
}


/* ARGSUSED */
static int
roffspecial(void *arg, int tok, int *argc, char **argv, char **more)
{
	struct md_xml	*p;

	assert(arg);
	p = (struct md_xml *)arg;

	/* FIXME: this is completely ad hoc. */

	switch (tok) {
	case (ROFF_Ns):
		p->flags |= MD_OVERRIDE_ONE;
		break;
	case (ROFF_Sm):
		assert(*more);
		if (0 == strcmp(*more, "on"))
			p->flags |= MD_OVERRIDE_ALL;
		else
			p->flags &= ~MD_OVERRIDE_ALL;
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
		if ( ! xml_indent(p))
			return(0);
	} else if ( ! xml_indent(p))
		return(0);

	/* FIXME: xml won't like standards args (e.g., p1003.1-90). */

	p->indent++;
	mbuf_mode(p, MD_BLKIN);

	if ( ! xml_begintag(p, toknames[tok], MD_NS_BLOCK,
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
		if ( ! xml_indent(p))
			return(0);
	} else if ( ! xml_indent(p))
		return(0);

	mbuf_mode(p, MD_BLKOUT);
	if ( ! xml_endtag(p, toknames[tok], MD_NS_BLOCK))
		return(0);
	return(mbuf_newline(p));
}


static int
roffin(void *arg, int tok, int *argc, char **argv)
{
	struct md_xml	*p;

	assert(arg);
	p = (struct md_xml *)arg;

	if ( ! (MD_OVERRIDE_ONE & p->flags) && 
			! (MD_OVERRIDE_ALL & p->flags) && 
			p->pos + 11 > COLUMNS) 
		if ( ! mbuf_newline(p))
			return(0);

	if (0 != p->pos && (MD_TEXT == p->last || MD_OUT == p->last)
			&& ! (MD_OVERRIDE_ONE & p->flags)
			&& ! (MD_OVERRIDE_ALL & p->flags))
		if ( ! xml_nputs(p, " ", 1))
			return(0);

	if (0 == p->pos && ! xml_indent(p))
		return(0);

	mbuf_mode(p, MD_IN);
	return(xml_begintag(p, toknames[tok], 
				MD_NS_INLINE, argc, argv));
}


static int
roffout(void *arg, int tok)
{
	struct md_xml	*p;

	assert(arg);
	p = (struct md_xml *)arg;

	if (0 == p->pos && ! xml_indent(p))
		return(0);

	mbuf_mode(p, MD_OUT);
	return(xml_endtag(p, toknames[tok], MD_NS_INLINE));
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

	mbuf_mode(p, MD_TEXT);
	return(1);
}


#ifdef __linux /* FIXME: remove. */
/*	$OpenBSD: strlcat.c,v 1.13 2005/08/08 08:05:37 espie Exp $	*/

/*
 * Copyright (c) 1998 Todd C. Miller <Todd.Miller@courtesan.com>
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
static size_t
strlcat(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;
	size_t dlen;

	/* Find the end of dst and adjust bytes left but don't go past
	 * end */
	while (n-- != 0 && *d != '\0')
		d++;
	dlen = d - dst;
	n = siz - dlen;

	if (n == 0)
		return(dlen + strlen(s));
	while (*s != '\0') {
		if (n != 1) {
			*d++ = *s;
			n--;
		}
		s++;
	}
	*d = '\0';

	return(dlen + (s - src));	/* count does not include NUL */
}


static size_t
strlcpy(char *dst, const char *src, size_t siz)
{
	char *d = dst;
	const char *s = src;
	size_t n = siz;

	/* Copy as many bytes as will fit */
	if (n != 0) {
		while (--n != 0) {
			if ((*d++ = *s++) == '\0')
				break;
		}
	}

	/* Not enough room in dst, add NUL and traverse rest of src */
	if (n == 0) {
		if (siz != 0)
			*d = '\0';		/* NUL-terminate dst */
		while (*s++)
			;
	}

	return(s - src - 1);	/* count does not include NUL */
}
#endif /*__linux__*/
