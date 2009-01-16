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

#define	xfprintf	(void)fprintf
#define	xprintf		(void)printf
#define	xvfprintf	(void)fvprintf

#define	MD_LINE_SZ	(256)		/* Max input line size. */

struct	md_parse {
	int		 warn;		/* Warning flags. */
#define	MD_WARN_SYNTAX	(1 << 0)	/* Show syntax warnings. */
#define	MD_WARN_COMPAT	(1 << 1)	/* Show compat warnings. */
#define	MD_WARN_ALL	(0x03)		/* Show all warnings. */
#define	MD_WARN_ERR	(1 << 2)	/* Make warnings->errors. */
	int		 dbg;		/* Debug level. */
	struct mdoc	*mdoc;		/* Active parser. */
	char		*buf;		/* Input buffer. */
	u_long		 bufsz;		/* Input buffer size. */
	char		*name;		/* Input file name. */
	int		 fd;		/* Input file desc. */
};

extern	char	 	*__progname;

static	void		 usage(void);

static	int		 parse_begin(struct md_parse *);
static	int		 parse_leave(struct md_parse *, int);
static	int		 io_begin(struct md_parse *);
static	int		 io_leave(struct md_parse *, int);
static	int		 buf_begin(struct md_parse *);
static	int		 buf_leave(struct md_parse *, int);

static	void		 msg_msg(void *, int, int, const char *);
static	int		 msg_err(void *, int, int, const char *);
static	int		 msg_warn(void *, int, int, 
				enum mdoc_warn, const char *);

#ifdef __linux__
extern	int		 getsubopt(char **, char *const *, char **);
#endif

int
main(int argc, char *argv[])
{
	int		 c;
	struct md_parse	 parser;
	char		*opts, *v;
#define ALL     	 0
#define COMPAT     	 1
#define SYNTAX     	 2
#define ERROR     	 3
	char		*toks[] = { "all", "compat", "syntax", 
				    "error", NULL };

	extern char	*optarg;
	extern int	 optind;

	(void)memset(&parser, 0, sizeof(struct md_parse));

	while (-1 != (c = getopt(argc, argv, "vW:")))
		switch (c) {
		case ('v'):
			parser.dbg++;
			break;
		case ('W'):
			opts = optarg;
			while (*opts) 
				switch (getsubopt(&opts, toks, &v)) {
				case (ALL):
					parser.warn |= MD_WARN_ALL;
					break;
				case (COMPAT):
					parser.warn |= MD_WARN_COMPAT;
					break;
				case (SYNTAX):
					parser.warn |= MD_WARN_SYNTAX;
					break;
				case (ERROR):
					parser.warn |= MD_WARN_ERR;
					break;
				default:
					usage();
					return(1);
				}
			break;
		default:
			usage();
			return(1);
		}

	argv += optind;
	argc -= optind;

	parser.name = "-";
	if (1 == argc)
		parser.name = *argv++;

	if ( ! io_begin(&parser))
		return(EXIT_FAILURE);

	return(EXIT_SUCCESS);
}


static int
io_leave(struct md_parse *p, int code)
{

	if (-1 == p->fd || STDIN_FILENO == p->fd)
		return(code);

	if (-1 == close(p->fd)) {
		warn("%s", p->name);
		code = 0;
	}
	return(code);
}


static int
io_begin(struct md_parse *p)
{

	p->fd = STDIN_FILENO;
	if (0 != strncmp(p->name, "-", 1))
		if (-1 == (p->fd = open(p->name, O_RDONLY, 0))) {
			warn("%s", p->name);
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

	if (-1 == fstat(p->fd, &st)) {
		warn("%s", p->name);
		return(1);
	} 

	p->bufsz = MAX(st.st_blksize, BUFSIZ);

	if (NULL == (p->buf = malloc(p->bufsz))) {
		warn("malloc");
		return(buf_leave(p, 0));
	}

	return(buf_leave(p, parse_begin(p)));
}


/* TODO: remove this to a print-tree output filter. */
static void
print_node(const struct mdoc_node *n, int indent)
{
	const char	 *p, *t;
	int		  i, j;
	size_t		  argc, sz;
	char		**params;
	struct mdoc_arg	 *argv;

	argv = NULL;
	argc = sz = 0;
	params = NULL;

	t = mdoc_type2a(n->type);

	switch (n->type) {
	case (MDOC_TEXT):
		p = n->data.text.string;
		break;
	case (MDOC_BODY):
		p = mdoc_macronames[n->tok];
		break;
	case (MDOC_HEAD):
		p = mdoc_macronames[n->tok];
		break;
	case (MDOC_TAIL):
		p = mdoc_macronames[n->tok];
		break;
	case (MDOC_ELEM):
		p = mdoc_macronames[n->tok];
		argv = n->data.elem.argv;
		argc = n->data.elem.argc;
		break;
	case (MDOC_BLOCK):
		p = mdoc_macronames[n->tok];
		argv = n->data.block.argv;
		argc = n->data.block.argc;
		break;
	case (MDOC_ROOT):
		p = "root";
		break;
	default:
		abort();
		/* NOTREACHED */
	}

	for (i = 0; i < indent; i++)
		xprintf("    ");
	xprintf("%s (%s)", p, t);

	for (i = 0; i < (int)argc; i++) {
		xprintf(" -%s", mdoc_argnames[argv[i].arg]);
		if (argv[i].sz > 0)
			xprintf(" [");
		for (j = 0; j < (int)argv[i].sz; j++)
			xprintf(" [%s]", argv[i].value[j]);
		if (argv[i].sz > 0)
			xprintf(" ]");
	}

	for (i = 0; i < (int)sz; i++)
		xprintf(" [%s]", params[i]);

	xprintf(" %d:%d\n", n->line, n->pos);

	if (n->child)
		print_node(n->child, indent + 1);
	if (n->next)
		print_node(n->next, indent);
}


static int
parse_leave(struct md_parse *p, int code)
{
	const struct mdoc_node *n;

	if (NULL == p->mdoc)
		return(code);

	if ( ! mdoc_endparse(p->mdoc))
		code = 0;
	if ((n = mdoc_result(p->mdoc)))
		print_node(n, 0);

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

	for (lnn = 1, pos = 0; ; ) {
		if (-1 == (sz = read(p->fd, p->buf, p->bufsz))) {
			warn("%s", p->name);
			return(parse_leave(p, 0));
		} else if (0 == sz) 
			break;

		for (i = 0; i < sz; i++) {
			if ('\n' != p->buf[i]) {
				if (pos < sizeof(line)) {
					line[(int)pos++] = p->buf[(int)i];
					continue;
				}
				warnx("%s: line %d too long", 
						p->name, lnn);
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

	xfprintf(stderr, "%s:%d: error: %s (column %d)\n", 
			p->name, line, msg, col);
	return(0);
}


static void
msg_msg(void *arg, int line, int col, const char *msg)
{
	struct md_parse	 *p;

	p = (struct md_parse *)arg;

	if (0 == p->dbg)
		return;

	xfprintf(stderr, "%s:%d: debug: %s (column %d)\n", 
			p->name, line, msg, col);
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

	xfprintf(stderr, "%s:%d: warning: %s (column %d)\n", 
			p->name, line, msg, col);

	if ( ! (p->warn & MD_WARN_ERR))
		return(1);

	xfprintf(stderr, "%s: considering warnings as errors\n", 
			__progname);
	return(0);
}


static void
usage(void)
{

	xfprintf(stderr, "usage: %s [-v] [-Wwarn...] [infile]\n",
			__progname);
}

