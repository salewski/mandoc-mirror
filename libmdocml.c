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

struct	md_rbuf {
	int		 fd;
	char		*name;
	char		*buf;
	size_t		 bufsz;
	size_t		 line;
};

struct	md_mbuf {
	int		 fd;
	char		*name;
	char		*buf;
	size_t		 bufsz;
	size_t		 pos;
};

typedef int (*md_line)	(const struct md_args *, struct md_mbuf *, 
				const struct md_rbuf *,
				const char *, size_t);
typedef int (*md_init)	(const struct md_args *, struct md_mbuf *);
typedef int (*md_exit)	(const struct md_args *, struct md_mbuf *);

static int		 md_line_dummy(const struct md_args *,
				struct md_mbuf *, 
				const struct md_rbuf *, 
				const char *, size_t);

static int		 md_line_html4_strict(const struct md_args *,
				struct md_mbuf *, 
				const struct md_rbuf *,
				const char *, size_t);
static int		 md_init_html4_strict(const struct md_args *,
				struct md_mbuf *);
static int		 md_exit_html4_strict(const struct md_args *,
				struct md_mbuf *);

static int		 md_run_enter(const struct md_args *, 
				struct md_mbuf *, struct md_rbuf *);
static int		 md_run_leave(const struct md_args *, 
				struct md_mbuf *, 
				struct md_rbuf *, int);

static ssize_t		 md_buf_fill(struct md_rbuf *);
static int		 md_buf_flush(struct md_mbuf *);
static int		 md_buf_putchar(struct md_mbuf *, char);
static int		 md_buf_putstring(struct md_mbuf *, 
				const char *);
static int		 md_buf_puts(struct md_mbuf *, 
				const char *, size_t);


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


static int
md_buf_putchar(struct md_mbuf *buf, char c)
{
	return(md_buf_puts(buf, &c, 1));
}


static int
md_buf_putstring(struct md_mbuf *buf, const char *p)
{
	return(md_buf_puts(buf, p, strlen(p)));
}


static int
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
md_run_leave(const struct md_args *args, 
		struct md_mbuf *mbuf, struct md_rbuf *rbuf, int c)
{
	assert(args);
	assert(mbuf);
	assert(rbuf);

	/* Run exiters. */
	switch (args->type) {
	case (MD_HTML4_STRICT):
		if ( ! md_exit_html4_strict(args, mbuf))
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
md_run_enter(const struct md_args *args, 
		struct md_mbuf *mbuf, struct md_rbuf *rbuf)
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
	case (MD_DUMMY):
		fp = md_line_dummy;
		break;
	default:
		abort();
	}

	/* LINTED */
	for (pos = 0; ; ) {
		if (-1 == (sz = md_buf_fill(rbuf)))
			return(-1);
		else if (0 == sz)
			break;

		for (i = 0; i < sz; i++) {
			if ('\n' == rbuf->buf[i]) {
				if ( ! (*fp)(args, mbuf, rbuf, line, pos))
					return(-1);
				rbuf->line++;
				pos = 0;
				continue;
			}

			if (pos < BUFFER_LINE) {
				/* LINTED */
				line[pos++] = rbuf->buf[i];
				continue;
			}

			warnx("%s: line %zu too long",
					rbuf->name, rbuf->line);
			return(-1);
		}
	}

	if (0 != pos && ! (*fp)(args, mbuf, rbuf, line, pos))
		return(-1);

	return(md_run_leave(args, mbuf, rbuf, 0));
}


int
md_run(const struct md_args *args,
		const struct md_buf *out, const struct md_buf *in)
{
	struct md_mbuf	 mbuf;
	struct md_rbuf	 rbuf;

	assert(args);
	assert(in);
	assert(out); 

	(void)memcpy(&mbuf, out, sizeof(struct md_buf));
	(void)memcpy(&rbuf, in, sizeof(struct md_buf));

	mbuf.pos = 0;
	rbuf.line = 1;

	/* Run initialisers. */
	switch (args->type) {
	case (MD_HTML4_STRICT):
		if ( ! md_init_html4_strict(args, &mbuf))
			return(-1);
		break;
	case (MD_DUMMY):
		break;
	default:
		abort();
	}

	/* Go into mainline. */
	return(md_run_enter(args, &mbuf, &rbuf));
}


static int
md_line_dummy(const struct md_args *args, struct md_mbuf *out, 
		const struct md_rbuf *in, const char *buf, size_t sz)
{

	assert(buf);
	assert(out);
	assert(in);
	assert(args);

	if ( ! md_buf_puts(out, buf, sz))
		return(0);
	if ( ! md_buf_putchar(out, '\n'))
		return(0);

	return(1);
}


static int
md_exit_html4_strict(const struct md_args *args, struct md_mbuf *out) 
{
	char		*tail;

	assert(out);
	assert(args);

	tail =	"		</pre>\n"
		"	</body>\n"
		"</html>\n";

	if ( ! md_buf_putstring(out, tail))
		return(0);

	return(1);
}


static int
md_init_html4_strict(const struct md_args *args, struct md_mbuf *out) 
{
	char		*head;

	assert(out);
	assert(args);

	head =	"<html>\n"
		"	<head>\n"
		"		<title>Manual Page</title>\n"
		"	</head>\n"
		"	<body>\n"
		"		<pre>\n";

	if ( ! md_buf_putstring(out, head))
		return(0);

	return(1);
}


struct md_roff_macro {
	char		 name[2];
	int		 flags;
#define	MD_PARSED	(1 << 0)
#define	MD_CALLABLE	(1 << 1)
#define	MD_TITLE	(1 << 2)
};

struct md_roff_macro[] = {
	{ "Dd",		MD_TITLE 	},
	{ "Dt",		MD_TITLE 	},
	{ "Os",		MD_TITLE 	},
	{ "Sh",		MD_PARSED 	},
};


static int
md_roff(struct md_mbuf *out, const struct md_rbuf *in, 
		const char *buf, size_t sz)
{

	assert(out);
	assert(in);
	assert(buf);
	assert(sz >= 1);
}


static int
md_line_html4_strict(const struct md_args *args, struct md_mbuf *out, 
		const struct md_rbuf *in, const char *buf, size_t sz)
{

	assert(args);
	assert(in);

	if (0 == sz) {
		warnx("%s: blank line (line %zu)", in->name, in->line);
		return(0);
	}

	if ('.' == *buf) {
		return(1);
	}
	
	return(md_buf_puts(out, buf, sz));
}
