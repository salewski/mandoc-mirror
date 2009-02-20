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
#include <sys/stat.h>
#include <sys/param.h>

#include <assert.h>
#include <fcntl.h>
#include <err.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mdoc.h"

#define	MD_LINE_SZ	(256)		/* Max input line size. */

struct	md_parse {
	int		  warn;		/* Warning flags. */
#define	MD_WARN_SYNTAX	 (1 << 0)	/* Show syntax warnings. */
#define	MD_WARN_COMPAT	 (1 << 1)	/* Show compat warnings. */
#define	MD_WARN_ALL	 (0x03)		/* Show all warnings. */
#define	MD_WARN_ERR	 (1 << 2)	/* Make warnings->errors. */
	int		  dbg;		/* Debug level. */
	struct mdoc	 *mdoc;		/* Active parser. */
	char		 *buf;		/* Input buffer. */
	u_long		  bufsz;	/* Input buffer size. */
	char		 *in;		/* Input file name. */
	int		  fdin;		/* Input file desc. */
};

extern	char	 	 *__progname;

static	void		  usage(void);

static	int		  parse_opts(struct md_parse *, int, char *[]);
static	int		  parse_subopts(struct md_parse *, char *);

static	int		  parse_begin(struct md_parse *);
static	int		  parse_leave(struct md_parse *, int);
static	int		  io_begin(struct md_parse *);
static	int		  io_leave(struct md_parse *, int);
static	int		  buf_begin(struct md_parse *);
static	int		  buf_leave(struct md_parse *, int);

static	void		  msg_msg(void *, int, int, const char *);
static	int		  msg_err(void *, int, int, const char *);
static	int		  msg_warn(void *, int, int, 
				enum mdoc_warn, const char *);

#ifdef __linux__
extern	int		  getsubopt(char **, char *const *, char **);
#endif

int
main(int argc, char *argv[])
{
	struct md_parse	 parser;

	(void)memset(&parser, 0, sizeof(struct md_parse));

	if ( ! parse_opts(&parser, argc, argv))
		return(EXIT_FAILURE);
	if ( ! io_begin(&parser))
		return(EXIT_FAILURE);

	return(EXIT_SUCCESS);
}


static int
io_leave(struct md_parse *p, int code)
{

	if (-1 == p->fdin || STDIN_FILENO == p->fdin)
		return(code);

	if (-1 == close(p->fdin)) {
		warn("%s", p->in);
		code = 0;
	}
	return(code);
}


static int
parse_subopts(struct md_parse *p, char *arg)
{
	char		*v;
	char		*toks[] = { "all", "compat", 
				"syntax", "error", NULL };

	/* 
	 * Future -Wxxx levels and so on should be here.  For now we
	 * only recognise syntax and compat warnings as categories,
	 * beyond the usually "all" and "error" (make warn error out).
	 */

	while (*arg) 
		switch (getsubopt(&arg, toks, &v)) {
		case (0):
			p->warn |= MD_WARN_ALL;
			break;
		case (1):
			p->warn |= MD_WARN_COMPAT;
			break;
		case (2):
			p->warn |= MD_WARN_SYNTAX;
			break;
		case (3):
			p->warn |= MD_WARN_ERR;
			break;
		default:
			usage();
			return(0);
		}

	return(1);
}


static int
parse_opts(struct md_parse *p, int argc, char *argv[])
{
	int		 c;

	extern char	*optarg;
	extern int	 optind;

	p->in = "-";

	while (-1 != (c = getopt(argc, argv, "vW:")))
		switch (c) {
		case ('v'):
			p->dbg++;
			break;
		case ('W'):
			if ( ! parse_subopts(p, optarg))
				return(0);
			break;
		default:
			usage();
			return(0);
		}

	argv += optind;
	if (0 == (argc -= optind))
		return(1);

	p->in = *argv++;
	return(1);
}


static int
io_begin(struct md_parse *p)
{

	p->fdin = STDIN_FILENO;
	if (0 != strncmp(p->in, "-", 1))
		if (-1 == (p->fdin = open(p->in, O_RDONLY, 0))) {
			warn("%s", p->in);
			return(io_leave(p, 0));
		}

	return(io_leave(p, buf_begin(p)));
}


static int
buf_leave(struct md_parse *p, int code)
{

	if (p->buf)
		free(p->buf);
	return(code);
}


static int
buf_begin(struct md_parse *p)
{
	struct stat	 st;

	if (-1 == fstat(p->fdin, &st)) {
		warn("%s", p->in);
		return(0);
	} 

	/*
	 * Try to intuit the fastest way of sucking down buffered data
	 * by using either the block buffer size or the hard-coded one.
	 * This is inspired by bin/cat.c.
	 */

	p->bufsz = MAX(st.st_blksize, BUFSIZ);

	if (NULL == (p->buf = malloc(p->bufsz))) {
		warn("malloc");
		return(buf_leave(p, 0));
	}

	return(buf_leave(p, parse_begin(p)));
}


static int
parse_leave(struct md_parse *p, int code)
{
	extern int termprint(const struct mdoc_node *, 
			const struct mdoc_meta *);

	if (NULL == p->mdoc)
		return(code);

	if ( ! mdoc_endparse(p->mdoc))
		code = 0;

	/* TODO */
	if (code && ! termprint(mdoc_node(p->mdoc), mdoc_meta(p->mdoc)))
		code = 0;

	mdoc_free(p->mdoc);
	return(code);
}


static int
parse_begin(struct md_parse *p)
{
	ssize_t		 sz, i;
	size_t		 pos;
	char		 line[MD_LINE_SZ];
	struct mdoc_cb	 cb;
	int		 lnn;

	cb.mdoc_err = msg_err;
	cb.mdoc_warn = msg_warn;
	cb.mdoc_msg = msg_msg;

	if (NULL == (p->mdoc = mdoc_alloc(p, &cb)))
		return(parse_leave(p, 0));

	/*
	 * This is a little more complicated than fgets.  TODO: have
	 * some benchmarks that show it's faster (note that I want to
	 * check many, many manuals simultaneously, so speed is
	 * important).  Fill a buffer (sized to the block size) with a
	 * single read, then parse \n-terminated lines into a line
	 * buffer, which is passed to the parser.  Hard-code the line
	 * buffer to a particular size -- a reasonable assumption.
	 */

	for (lnn = 1, pos = 0; ; ) {
		if (-1 == (sz = read(p->fdin, p->buf, p->bufsz))) {
			warn("%s", p->in);
			return(parse_leave(p, 0));
		} else if (0 == sz) 
			break;

		for (i = 0; i < sz; i++) {
			if ('\n' != p->buf[i]) {
				if (pos < sizeof(line)) {
					line[(int)pos++] = p->buf[(int)i];
					continue;
				}
				warnx("%s: line %d too long", p->in, lnn);
				return(parse_leave(p, 0));
			}
	
			line[(int)pos] = 0;
			if ( ! mdoc_parseln(p->mdoc, lnn, line))
				return(parse_leave(p, 0));

			lnn++;
			pos = 0;
		}
	}

	return(parse_leave(p, 1));
}


static int
msg_err(void *arg, int line, int col, const char *msg)
{
	struct md_parse	 *p;

	p = (struct md_parse *)arg;

	warnx("%s:%d: error: %s (column %d)", 
			p->in, line, msg, col);
	return(0);
}


static void
msg_msg(void *arg, int line, int col, const char *msg)
{
	struct md_parse	 *p;

	p = (struct md_parse *)arg;

	if (0 == p->dbg)
		return;

	warnx("%s:%d: debug: %s (column %d)", 
			p->in, line, msg, col);
}


static int
msg_warn(void *arg, int line, int col, 
		enum mdoc_warn type, const char *msg)
{
	struct md_parse	 *p;

	p = (struct md_parse *)arg;

	switch (type) {
	case (WARN_COMPAT):
		if (p->warn & MD_WARN_COMPAT)
			break;
		return(1);
	case (WARN_SYNTAX):
		if (p->warn & MD_WARN_SYNTAX)
			break;
		return(1);
	}

	warnx("%s:%d: warning: %s (column %d)", 
			p->in, line, msg, col);

	if ( ! (p->warn & MD_WARN_ERR))
		return(1);

	warnx("%s: considering warnings as errors", __progname);
	return(0);
}


static void
usage(void)
{

	warnx("usage: %s [-v] [-Wwarn...] [infile]", __progname);
}

