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
#include <sys/stat.h>

#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libmdocml.h"

struct md_file {
	int		 fd;
	const char	*name;
};

struct md_buf {
	struct md_file	*file;
	char		*buf;
	size_t		 bufsz;
	size_t		 line;
};

static void		 usage(void);

static int		 md_begin(const char *, const char *);
static int		 md_begin_io(const char *, const char *);
static int		 md_begin_bufs(struct md_file *, struct md_file *);
static int		 md_run(struct md_buf *, struct md_buf *);
static int		 md_line(struct md_buf *, const struct md_buf *,
				const char *, size_t);

static ssize_t		 md_buf_fill(struct md_buf *);


int
main(int argc, char *argv[])
{
	int		 c;
	char		*out, *in;

	extern char	*optarg;
	extern int	 optind;

	out = NULL;
	
	while (-1 != (c = getopt(argc, argv, "o:")))
		switch (c) {
		case ('o'):
			out = optarg;
			break;
		default:
			usage();
			return(1);
		}

	argv += optind;
	if (1 != (argc -= optind)) {
		usage();
		return(1);
	}

	argc--;
	in = *argv++;

	return(md_begin(out, in));
}


static int
md_begin(const char *out, const char *in)
{
	char		 buf[MAXPATHLEN];

	assert(in);
	if (out)
		return(md_begin_io(out, in));

	if (strlcpy(buf, in, MAXPATHLEN) >= MAXPATHLEN)
		warnx("output filename too long");
	else if (strlcat(buf, ".html", MAXPATHLEN) >= MAXPATHLEN)
		warnx("output filename too long");
	else 
		return(md_begin_io(buf, in));

	return(1);
}


static int
md_begin_io(const char *out, const char *in)
{
	int		 c;
	struct md_file	 fin, fout;

	assert(out);
	assert(in);

	/* XXX: put an output file in TMPDIR and switch it out when the
	 * true file is ready to be written.
	 */

	fin.name = in;

	if (-1 == (fin.fd = open(fin.name, O_RDONLY, 0))) {
		warn("%s", fin.name);
		return(1);
	}

	fout.name = out;

	fout.fd = open(fout.name, O_WRONLY | O_CREAT | O_EXCL, 0644);
	if (-1 == fout.fd) {
		warn("%s", fout.name);
		if (-1 == close(fin.fd))
			warn("%s", fin.name);
		return(1);
	}

	c = md_begin_bufs(&fout, &fin);

	if (-1 == close(fin.fd)) {
		warn("%s", in);
		c = 1;
	}
	if (-1 == close(fout.fd)) {
		warn("%s", out);
		c = 1;
	}

	return(c);
}


static int
md_begin_bufs(struct md_file *out, struct md_file *in)
{
	struct stat	 stin, stout;
	struct md_buf	 inbuf, outbuf;
	int		 c;

	assert(in);
	assert(out);

	if (-1 == fstat(in->fd, &stin)) {
		warn("fstat: %s", in->name);
		return(1);
	} else if (-1 == fstat(out->fd, &stout)) {
		warn("fstat: %s", out->name);
		return(1);
	}

	inbuf.file = in;
	inbuf.line = 1;
	inbuf.bufsz = MAX(stin.st_blksize, BUFSIZ);

	outbuf.file = out;
	outbuf.line = 1;
	outbuf.bufsz = MAX(stout.st_blksize, BUFSIZ);

	if (NULL == (inbuf.buf = malloc(inbuf.bufsz))) {
		warn("malloc");
		return(1);
	} else if (NULL == (outbuf.buf = malloc(outbuf.bufsz))) {
		warn("malloc");
		free(inbuf.buf);
		return(1);
	}

	c = md_run(&outbuf, &inbuf);

	free(inbuf.buf);
	free(outbuf.buf);

	return(c);
}


static ssize_t
md_buf_fill(struct md_buf *in)
{
	ssize_t		 ssz;

	assert(in);
	assert(in->file);
	assert(in->buf);
	assert(in->bufsz > 0);
	assert(in->file->name);

	if (-1 == (ssz = read(in->file->fd, in->buf, in->bufsz))) 
		warn("%s", in->file->name);

	return(ssz);
}


static int
md_run(struct md_buf *out, struct md_buf *in)
{
	ssize_t		 sz, i;
	char		 line[BUFSIZ];
	size_t		 pos;

	assert(in);
	assert(out); 

	/* LINTED */
	for (pos = 0; ; ) {
		if (-1 == (sz = md_buf_fill(in)))
			return(1);
		else if (0 == sz)
			break;

		for (i = 0; i < sz; i++) {
			if ('\n' == in->buf[i]) {
				if (md_line(out, in, line, pos))
					return(1);
				in->line++;
				pos = 0;
				continue;
			}

			if (pos < BUFSIZ) {
				/* LINTED */
				line[pos++] = in->buf[i];
				continue;
			}

			warnx("%s: line %zu too long",
					in->file->name, in->line);
			return(1);
		}
	}

	if (0 != pos)
		return(md_line(out, in, line, pos));

	return(0);
}


static int
md_line(struct md_buf *out, const struct md_buf *in,
		const char *buf, size_t sz)
{
	size_t		 i;

	assert(buf);
	assert(out);
	assert(in);

	for (i = 0; i < sz; i++) 
		(void)putchar(buf[i]);

	(void)putchar('\n');
	return(0);
}


static void
usage(void)
{
	extern char	*__progname;

	(void)printf("usage: %s [-o outfile] infile\n", __progname);
}
