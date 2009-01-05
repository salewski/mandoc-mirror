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

#define	MD_LINE_SZ	(256)

struct	md_parse {
	int		 warn;
#define	MD_WARN_ALL	(1 << 0)
#define	MD_WARN_ERR	(1 << 1)
	int		 dbg;
	struct mdoc	*mdoc;
	char		*buf;
	u_long		 bufsz;
	char		*name;
	int		 fd;
	int		 lnn;
	char		*line;
};

static	void		 usage(void);

static	int		 parse_begin(struct md_parse *);
static	int		 parse_leave(struct md_parse *, int);
static	int		 io_begin(struct md_parse *);
static	int		 io_leave(struct md_parse *, int);
static	int		 buf_begin(struct md_parse *);
static	int		 buf_leave(struct md_parse *, int);

static	int		 msg_err(void *, int, int, enum mdoc_err);
static	int		 msg_warn(void *, int, int, enum mdoc_warn);
static	void		 msg_msg(void *, int, const char *);

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
#define ERROR     	 1
	char		*toks[] = { "all", "error", NULL };

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


static void
print_node(const struct mdoc_node *n, int indent)
{
	const char	 *p, *t;
	int		  i, j;
	size_t		  argc, sz;
	char		**params;
	struct mdoc_arg	 *argv;

	argv = NULL;
	argc = 0;
	params = NULL;
	sz = 0;

	switch (n->type) {
	case (MDOC_TEXT):
		assert(NULL == n->child);
		p = n->data.text.string;
		t = "text";
		break;
	case (MDOC_BODY):
		p = mdoc_macronames[n->data.body.tok];
		t = "block-body";
		break;
	case (MDOC_HEAD):
		p = mdoc_macronames[n->data.head.tok];
		t = "block-head";
		break;
	case (MDOC_TAIL):
		p = mdoc_macronames[n->data.tail.tok];
		t = "block-tail";
		break;
	case (MDOC_ELEM):
		p = mdoc_macronames[n->data.elem.tok];
		t = "element";
		argv = n->data.elem.argv;
		argc = n->data.elem.argc;
		params = n->data.elem.args;
		sz = n->data.elem.sz;
		break;
	case (MDOC_BLOCK):
		p = mdoc_macronames[n->data.block.tok];
		t = "block";
		argv = n->data.block.argv;
		argc = n->data.block.argc;
		break;
	default:
		abort();
		/* NOTREACHED */
	}

	for (i = 0; i < indent; i++)
		(void)printf("    ");
	(void)printf("%s (%s)", p, t);

	for (i = 0; i < (int)argc; i++) {
		(void)printf(" -%s", mdoc_argnames[argv[i].arg]);
		for (j = 0; j < (int)argv[i].sz; j++)
			(void)printf(" \"%s\"", argv[i].value[j]);
	}

	for (i = 0; i < (int)sz; i++)
		(void)printf(" \"%s\"", params[i]);

	(void)printf("\n");

	if (n->child)
		print_node(n->child, indent + 1);
	if (n->next)
		print_node(n->next, indent);
}


static int
parse_leave(struct md_parse *p, int code)
{
	const struct mdoc_node *n;

	if (p->mdoc) {
		if ((n = mdoc_result(p->mdoc)))
			print_node(n, 0);
		mdoc_free(p->mdoc);
	}
	return(code);
}


static int
parse_begin(struct md_parse *p)
{
	ssize_t		 sz, i;
	size_t		 pos;
	char		 line[256], sv[256];
	struct mdoc_cb	 cb;

	cb.mdoc_err = msg_err;
	cb.mdoc_warn = msg_warn;
	cb.mdoc_msg = msg_msg;

	if (NULL == (p->mdoc = mdoc_alloc(p, &cb)))
		return(parse_leave(p, 0));

	p->lnn = 1;
	p->line = sv;

	for (pos = 0; ; ) {
		if (-1 == (sz = read(p->fd, p->buf, p->bufsz))) {
			warn("%s", p->name);
			return(parse_leave(p, 0));
		} else if (0 == sz) 
			break;

		for (i = 0; i < sz; i++) {
			if ('\n' != p->buf[i]) {
				if (pos < sizeof(line)) {
					sv[(int)pos] = p->buf[(int)i];
					line[(int)pos++] = 
						p->buf[(int)i];
					continue;
				}
				warnx("%s: line %d too long", 
						p->name, p->lnn);
				return(parse_leave(p, 0));
			}
	
			line[(int)pos] = sv[(int)pos] = 0;
			if ( ! mdoc_parseln(p->mdoc, line))
				return(parse_leave(p, 0));

			p->lnn++;
			pos = 0;
		}
	}

	return(parse_leave(p, 1));
}


static int
msg_err(void *arg, int tok, int col, enum mdoc_err type)
{
	char		 *fmt, *lit;
	struct md_parse	 *p;
	int		  i;

	p = (struct md_parse *)arg;

	fmt = lit = NULL;

	switch (type) {
	case (ERR_SYNTAX_QUOTE):
		lit = "syntax: disallowed argument quotation";
		break;
	case (ERR_SYNTAX_UNQUOTE):
		lit = "syntax: unterminated quotation";
		break;
	case (ERR_SYNTAX_WS):
		lit = "syntax: whitespace in argument";
		break;
	case (ERR_SYNTAX_ARGFORM):
		fmt = "syntax: macro `%s' arguments malformed";
		break;
	case (ERR_SYNTAX_NOPUNCT):
		fmt = "syntax: macro `%s' doesn't understand punctuation";
		break;
	case (ERR_SYNTAX_ARG):
		fmt = "syntax: unknown argument for macro `%s'";
		break;
	case (ERR_SCOPE_BREAK):
		/* Which scope is broken? */
		fmt = "scope: macro `%s' breaks prior explicit scope";
		break;
	case (ERR_SCOPE_NOCTX):
		fmt = "scope: closure macro `%s' has no context";
		break;
	case (ERR_SCOPE_NONEST):
		fmt = "scope: macro `%s' may not be nested in the current context";
		break;
	case (ERR_MACRO_NOTSUP):
		fmt = "macro `%s' not supported";
		break;
	case (ERR_MACRO_NOTCALL):
		fmt = "macro `%s' not callable";
		break;
	case (ERR_SEC_PROLOGUE):
		fmt = "macro `%s' cannot be called in the prologue";
		break;
	case (ERR_SEC_NPROLOGUE):
		fmt = "macro `%s' called outside of prologue";
		break;
	case (ERR_ARGS_EQ0):
		fmt = "macro `%s' expects zero arguments";
		break;
	case (ERR_ARGS_EQ1):
		fmt = "macro `%s' expects one argument";
		break;
	case (ERR_ARGS_GE1):
		fmt = "macro `%s' expects one or more arguments";
		break;
	case (ERR_ARGS_LE2):
		fmt = "macro `%s' expects two or fewer arguments";
		break;
	case (ERR_ARGS_MANY):
		fmt = "macro `%s' has too many arguments";
		break;
	case (ERR_SEC_PROLOGUE_OO):
		fmt = "prologue macro `%s' is out-of-order";
		break;
	case (ERR_SEC_PROLOGUE_REP):
		fmt = "prologue macro `%s' repeated";
		break;
	case (ERR_SEC_NAME):
		lit = "`NAME' section must be first";
		break;
	case (ERR_SYNTAX_ARGVAL):
		lit = "syntax: expected value for macro argument";
		break;
	case (ERR_SYNTAX_ARGBAD):
		lit = "syntax: invalid value for macro argument";
		break;
	case (ERR_SYNTAX_ARGMANY):
		lit = "syntax: too many values for macro argument";
		break;
	case (ERR_SYNTAX_CHILDHEAD):
		lit = "syntax: expected only block-header section";
		break;
	case (ERR_SYNTAX_CHILDBODY):
		lit = "syntax: expected only a block-body section";
		break;
	case (ERR_SYNTAX_EMPTYHEAD):
		lit = "syntax: block-header section may not be empty";
		break;
	case (ERR_SYNTAX_EMPTYBODY):
		lit = "syntax: block-body section may not be empty";
		break;
	default:
		abort();
		/* NOTREACHED */
	}

	if (fmt) {
		(void)fprintf(stderr, "%s:%d: error: ",
				p->name, p->lnn);
		(void)fprintf(stderr, fmt, mdoc_macronames[tok]);
	} else
		(void)fprintf(stderr, "%s:%d: error: %s",
				p->name, p->lnn, lit);

	if (p->dbg < 1) {
		if (-1 != col)
			(void)fprintf(stderr, " (column %d)\n", col);
		return(0);
	} else if (-1 == col) {
		(void)fprintf(stderr, "\nFrom: %s", p->line);
		return(0);
	}

	(void)fprintf(stderr, "\nFrom: %s\n      ", p->line);
	for (i = 0; i < col; i++)
		(void)fprintf(stderr, " ");
	(void)fprintf(stderr, "^\n");

	return(0);
}


static void
msg_msg(void *arg, int col, const char *msg)
{
	struct md_parse	 *p;
	int		  i;

	p = (struct md_parse *)arg;

	if (p->dbg < 2)
		return;

	(void)printf("%s:%d: %s", p->name, p->lnn, msg);

	if (p->dbg < 3) {
		if (-1 != col)
			(void)printf(" (column %d)\n", col);
		return;
	} else if (-1 == col) {
		(void)printf("\nFrom %s\n", p->line);
		return;
	}

	(void)printf("\nFrom: %s\n      ", p->line);
	for (i = 0; i < col; i++)
		(void)printf(" ");
	(void)printf("^\n");
}


static int
msg_warn(void *arg, int tok, int col, enum mdoc_warn type)
{
	char		 *fmt, *lit;
	struct md_parse	 *p;
	int		  i;
	extern char	 *__progname;

	p = (struct md_parse *)arg;

	if ( ! (p->warn & MD_WARN_ALL))
		return(1);

	fmt = lit = NULL;

	switch (type) {
	case (WARN_SYNTAX_WS_EOLN):
		lit = "syntax: whitespace at end-of-line";
		break;
	case (WARN_SYNTAX_QUOTED):
		lit = "syntax: quotation mark starting string";
		break;
	case (WARN_SYNTAX_MACLIKE):
		lit = "syntax: macro-like argument";
		break;
	case (WARN_SYNTAX_ARGLIKE):
		lit = "syntax: argument-like value";
		break;
	case (WARN_SYNTAX_EMPTYBODY):
		lit = "syntax: empty block-body section";
		break;
	case (WARN_SEC_OO):
		lit = "section is out of conventional order";
		break;
	case (WARN_ARGS_GE1):
		fmt = "macro `%s' suggests one or more arguments";
		break;
	case (WARN_ARGS_EQ0):
		fmt = "macro `%s' suggests zero arguments";
		break;
	case (WARN_IGN_AFTER_BLK):
		fmt = "ignore: macro `%s' ignored after block macro";
		break;
	case (WARN_IGN_OBSOLETE):
		fmt = "ignore: macro `%s' is obsolete";
		break;
	case (WARN_IGN_BEFORE_BLK):
		fmt = "ignore: macro before block macro `%s' ignored";
		break;
	case (WARN_COMPAT_TROFF):
		fmt = "compat: macro `%s' behaves differently in troff and nroff";
		break;
	default:
		abort();
		/* NOTREACHED */
	}

	if (fmt) {
		(void)fprintf(stderr, "%s:%d: warning: ",
				p->name, p->lnn);
		(void)fprintf(stderr, fmt, mdoc_macronames[tok]);
	} else
		(void)fprintf(stderr, "%s:%d: warning: %s",
				p->name, p->lnn, lit);

	if (p->dbg >= 1) {
		(void)fprintf(stderr, "\nFrom: %s\n      ", p->line);
		for (i = 0; i < col; i++)
			(void)fprintf(stderr, " ");
		(void)fprintf(stderr, "^\n");
	} else
		(void)fprintf(stderr, " (column %d)\n", col);

	if (p->warn & MD_WARN_ERR) {
		(void)fprintf(stderr, "%s: considering warnings as "
				"errors\n", __progname);
		return(0);
	}

	return(1);
}


static void
usage(void)
{
	extern char	*__progname;

	(void)fprintf(stderr, "usage: %s [-v] [-Wwarn...] [infile]\n",
			__progname);
}

