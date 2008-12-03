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

#define	BUFFER_IN_DEF	BUFSIZ	 /* See begin_bufs. */
#define	BUFFER_OUT_DEF	BUFSIZ	 /* See begin_bufs. */

static	void		 usage(void);

static	int		 begin_io(const struct md_args *, 
				char *, char *);
static	int		 leave_io(const struct md_buf *, 
				const struct md_buf *, int);
static	int		 begin_bufs(const struct md_args *,
				struct md_buf *, struct md_buf *);
static int		 leave_bufs(const struct md_buf *, 
				const struct md_buf *, int);

int
main(int argc, char *argv[])
{
	int		 c;
	char		*out, *in, *filter;
	struct md_args	 args;

	extern char	*optarg;
	extern int	 optind;

	out = in = filter = NULL;

	(void)memset(&args, 0, sizeof(struct md_args));
	
	while (-1 != (c = getopt(argc, argv, "f:o:vW")))
		switch (c) {
		case ('f'):
			filter = optarg;
			break;
		case ('o'):
			out = optarg;
			break;
		case ('v'):
			args.verbosity++;
			break;
		case ('W'):
			args.warnings |= MD_WARN_ALL;
			break;
		default:
			usage();
			return(1);
		}

	argv += optind;
	argc -= optind;

	if (1 == argc)
		in = *argv++;

	if (filter) {
		if (0 == strcmp(filter, "html"))
			args.type = MD_HTML;
		else if (0 == strcmp(filter, "xml"))
			args.type = MD_XML;
		else
			errx(1, "invalid filter type");
	} else
		args.type = MD_XML;

	return(begin_io(&args, out ? out : "-", in ? in : "-"));
}


/* 
 * Close out file descriptors opened in begin_io.  If the descriptor
 * refers to stdin/stdout, then do nothing.
 */
static int
leave_io(const struct md_buf *out, 
		const struct md_buf *in, int c)
{
	assert(out);
	assert(in);

	if (-1 != in->fd && -1 == close(in->fd)) {
		assert(in->name);
		warn("%s", in->name);
		c = 1;
	}
	if (-1 != out->fd && STDOUT_FILENO != out->fd &&
			-1 == close(out->fd)) {
		assert(out->name);
		warn("%s", out->name);
		c = 1;
	}
	if (1 == c && STDOUT_FILENO != out->fd)
		if (-1 == unlink(out->name))
			warn("%s", out->name);

	return(c);
}


/*
 * Open file descriptors or assign stdin/stdout, if dictated by the "-"
 * token instead of a filename.
 */
static int
begin_io(const struct md_args *args, char *out, char *in)
{
	struct md_buf	 fi;
	struct md_buf	 fo;

#define	FI_FL	O_RDONLY
#define	FO_FL	O_WRONLY|O_CREAT|O_TRUNC

	assert(args);
	assert(out);
	assert(in);

	bzero(&fi, sizeof(struct md_buf));
	bzero(&fo, sizeof(struct md_buf));

	fi.fd = STDIN_FILENO;
	fo.fd = STDOUT_FILENO;

	fi.name = in;
	fo.name = out;

	if (0 != strncmp(fi.name, "-", 1))
		if (-1 == (fi.fd = open(fi.name, FI_FL, 0))) {
			warn("%s", fi.name);
			return(leave_io(&fo, &fi, 1));
		}

	if (0 != strncmp(fo.name, "-", 1)) 
		if (-1 == (fo.fd = open(fo.name, FO_FL, 0644))) {
			warn("%s", fo.name);
			return(leave_io(&fo, &fi, 1));
		}

	return(leave_io(&fo, &fi, begin_bufs(args, &fo, &fi)));
}


/*
 * Free buffers allocated in begin_bufs.
 */
static int
leave_bufs(const struct md_buf *out, 
		const struct md_buf *in, int c)
{
	assert(out);
	assert(in);
	if (out->buf)
		free(out->buf);
	if (in->buf)
		free(in->buf);
	return(c);
}


/*
 * Allocate buffers to the maximum of either the input file's blocksize
 * or BUFFER_IN_DEF/BUFFER_OUT_DEF, which should be around BUFSIZE.
 */
static int
begin_bufs(const struct md_args *args, 
		struct md_buf *out, struct md_buf *in)
{
	struct stat	 stin, stout;
	int		 c;

	assert(args);
	assert(in);
	assert(out);

	if (-1 == fstat(in->fd, &stin)) {
		warn("%s", in->name);
		return(1);
	} else if (STDIN_FILENO != in->fd && 0 == stin.st_size) {
		warnx("%s: empty file", in->name);
		return(1);
	} else if (-1 == fstat(out->fd, &stout)) {
		warn("%s", out->name);
		return(1);
	}

	in->bufsz = MAX(stin.st_blksize, BUFFER_IN_DEF);
	out->bufsz = MAX(stout.st_blksize, BUFFER_OUT_DEF);

	if (NULL == (in->buf = malloc(in->bufsz))) {
		warn("malloc");
		return(leave_bufs(out, in, 1));
	} else if (NULL == (out->buf = malloc(out->bufsz))) {
		warn("malloc");
		return(leave_bufs(out, in, 1));
	}

	c = md_run(args, out, in);
	return(leave_bufs(out, in, -1 == c ? 1 : 0));
}


static void
usage(void)
{
	extern char	*__progname;

	(void)printf("usage: %s [-vW] [-f filter] [-o outfile] "
			"[infile]\n", __progname);
}
