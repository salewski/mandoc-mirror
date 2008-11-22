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
#include <fcntl.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libmdocml.h"

#define	BUFFER_LINE	 BUFSIZ

typedef int (*md_line)	(struct md_mbuf *, const struct md_rbuf *,
				const char *, size_t);

static int		 md_line_dummy(struct md_mbuf *, 
				const struct md_rbuf *,
				const char *, size_t);
static ssize_t		 md_buf_fill(struct md_rbuf *);
static int		 md_buf_flush(struct md_mbuf *);
static int		 md_buf_putchar(struct md_mbuf *, char);
static int		 md_buf_puts(struct md_mbuf *, 
				const char *, size_t);


ssize_t
md_buf_fill(struct md_rbuf *in)
{
	ssize_t		 ssz;

	assert(in);
	assert(in->buf);
	assert(in->bufsz > 0);
	assert(in->name);

	if (-1 == (ssz = read(in->fd, in->buf, in->bufsz))) 
		warn("%s", in->name);

	return(ssz);
}


int
md_buf_flush(struct md_mbuf *buf)
{
	ssize_t		 sz;

	assert(buf);
	assert(buf->buf);
	assert(buf->name);

	if (0 == buf->pos)
		return(1);

	sz = write(buf->fd, buf->buf, buf->pos);

	if (-1 == sz) {
		warn("%s", buf->name);
		return(0);
	} else if ((size_t)sz != buf->pos) {
		warnx("%s: short write", buf->name);
		return(0);
	}

	buf->pos = 0;
	return(1);
}


int
md_buf_putchar(struct md_mbuf *buf, char c)
{
	return(md_buf_puts(buf, &c, 1));
}


int
md_buf_puts(struct md_mbuf *buf, const char *p, size_t sz)
{
	size_t		 ssz;

	assert(p);
	assert(buf);
	assert(buf->buf);

	/* LINTED */
	while (buf->pos + sz > buf->bufsz) {
		ssz = buf->bufsz - buf->pos;
		(void)memcpy(/* LINTED */
				buf->buf + buf->pos, p, ssz);
		p += (long)ssz;
		sz -= ssz;
		buf->pos += ssz;

		if ( ! md_buf_flush(buf))
			return(0);
	}

	(void)memcpy(/* LINTED */
			buf->buf + buf->pos, p, sz);
	buf->pos += sz;
	return(1);
}


int
md_run(enum md_type type, struct md_mbuf *out, struct md_rbuf *in)
{
	ssize_t		 sz, i;
	char		 line[BUFFER_LINE];
	size_t		 pos;
	md_line		 func;

	assert(in);
	assert(out); 

	out->pos = 0;
	in->line = 1;

	assert(MD_DUMMY == type);
	func = md_line_dummy;

	/* LINTED */
	for (pos = 0; ; ) {
		if (-1 == (sz = md_buf_fill(in)))
			return(1);
		else if (0 == sz)
			break;

		for (i = 0; i < sz; i++) {
			if ('\n' == in->buf[i]) {
				if ((*func)(out, in, line, pos))
					return(1);
				in->line++;
				pos = 0;
				continue;
			}

			if (pos < BUFFER_LINE) {
				/* LINTED */
				line[pos++] = in->buf[i];
				continue;
			}

			warnx("%s: line %zu too long",
					in->name, in->line);
			return(1);
		}
	}

	if (0 != pos && (*func)(out, in, line, pos))
		return(1);

	return(md_buf_flush(out) ? 0 : 1);
}


static int
md_line_dummy(struct md_mbuf *out, const struct md_rbuf *in,
		const char *buf, size_t sz)
{

	assert(buf);
	assert(out);
	assert(in);

	if ( ! md_buf_puts(out, buf, sz))
		return(1);
	if ( ! md_buf_putchar(out, '\n'))
		return(1);

	return(0);
}


