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
#include "private.h"

#define	BUFFER_LINE	 BUFSIZ	/* Default line-buffer size. */

static	int	 md_run_enter(const struct md_args *, 
			struct md_mbuf *, struct md_rbuf *, void *);
static	int	 md_run_leave(const struct md_args *, struct md_mbuf *,
			struct md_rbuf *, int, void *);

static	ssize_t	 md_buf_fill(struct md_rbuf *);
static	int	 md_buf_flush(struct md_mbuf *);


static ssize_t
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


static int
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
md_buf_putstring(struct md_mbuf *buf, const char *p)
{
	return(md_buf_puts(buf, p, strlen(p)));
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


static int
md_run_leave(const struct md_args *args, struct md_mbuf *mbuf, 
		struct md_rbuf *rbuf, int c, void *data)
{
	assert(args);
	assert(mbuf);
	assert(rbuf);

	/* Run exiters. */
	switch (args->type) {
	case (MD_HTML4_STRICT):
		if ( ! md_exit_html4_strict(args, mbuf, rbuf, c, data))
			return(-1);
		break;
	case (MD_DUMMY):
		break;
	default:
		abort();
	}

	/* Make final flush of buffer. */
	if ( ! md_buf_flush(mbuf))
		return(-1);

	return(c);
}


static int
md_run_enter(const struct md_args *args, struct md_mbuf *mbuf, 
		struct md_rbuf *rbuf, void *p)
{
	ssize_t		 sz, i;
	char		 line[BUFFER_LINE];
	size_t		 pos;
	md_line		 fp;

	assert(args);
	assert(mbuf);
	assert(rbuf); 

	/* Function ptrs to line-parsers. */
	switch (args->type) {
	case (MD_HTML4_STRICT):
		fp = md_line_html4_strict;
		break;
	default:
		fp = md_line_dummy;
		break;
	}

	pos = 0;

again:
	if (-1 == (sz = md_buf_fill(rbuf))) {
		return(md_run_leave(args, mbuf, rbuf, -1, p));
	} else if (0 == sz && 0 != pos) {
		warnx("%s: no newline at end of file", rbuf->name);
		return(md_run_leave(args, mbuf, rbuf, -1, p));
	} else if (0 == sz)
		return(md_run_leave(args, mbuf, rbuf, 0, p));

	for (i = 0; i < sz; i++) {
		if ('\n' != rbuf->buf[i]) {
			if (pos < BUFFER_LINE) {
				/* LINTED */
				line[pos++] = rbuf->buf[i];
				continue;
			}
			warnx("%s: line %zu too long",
					rbuf->name, rbuf->line);
			return(md_run_leave(args, mbuf, rbuf, -1, p));
		}

		line[pos] = 0;
		if ( ! (*fp)(args, mbuf, rbuf, line, pos, p))
			return(md_run_leave(args, mbuf, rbuf, -1, p));
		rbuf->line++;
		pos = 0;
	}

	goto again;
	/* NOTREACHED */
}


int
md_run(const struct md_args *args,
		const struct md_buf *out, const struct md_buf *in)
{
	struct md_mbuf	 mbuf;
	struct md_rbuf	 rbuf;
	void		*data;

	assert(args);
	assert(in);
	assert(out); 

	(void)memcpy(&mbuf, out, sizeof(struct md_buf));
	(void)memcpy(&rbuf, in, sizeof(struct md_buf));

	mbuf.pos = 0;
	rbuf.line = 1;
	data = NULL;

	/* Run initialisers. */
	switch (args->type) {
	case (MD_HTML4_STRICT):
		if ( ! md_init_html4_strict(args, &mbuf, &rbuf, &data))
			return(-1);
		break;
	case (MD_DUMMY):
		break;
	default:
		abort();
	}

	/* Go into mainline. */
	return(md_run_enter(args, &mbuf, &rbuf, data));
}
