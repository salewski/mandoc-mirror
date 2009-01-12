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
static	void		 msg_msg(void *, int, int, const char *);

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

	/* FIXME: put parts of this in util.c. */
	switch (n->type) {
	case (MDOC_TEXT):
		assert(NULL == n->child);
		p = n->data.text.string;
		t = "text";
		break;
	case (MDOC_BODY):
		p = mdoc_macronames[n->tok];
		t = "block-body";
		break;
	case (MDOC_HEAD):
		p = mdoc_macronames[n->tok];
		t = "block-head";
		break;
	case (MDOC_TAIL):
		p = mdoc_macronames[n->tok];
		t = "block-tail";
		break;
	case (MDOC_ELEM):
		p = mdoc_macronames[n->tok];
		t = "element";
		argv = n->data.elem.argv;
		argc = n->data.elem.argc;
		break;
	case (MDOC_BLOCK):
		p = mdoc_macronames[n->tok];
		t = "block";
		argv = n->data.block.argv;
		argc = n->data.block.argc;
		break;
	case (MDOC_ROOT):
		p = "root";
		t = "root";
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
		if (j > 0)
			(void)printf(" [");
		for (j = 0; j < (int)argv[i].sz; j++)
			(void)printf(" [%s]", argv[i].value[j]);
		if (j > 0)
			(void)printf(" ]");
	}

	for (i = 0; i < (int)sz; i++)
		(void)printf(" [%s]", params[i]);

	(void)printf(" %d:%d\n", n->line, n->pos);

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
			if ( ! mdoc_parseln(p->mdoc, p->lnn, line))
				return(parse_leave(p, 0));

			p->lnn++;
			pos = 0;
		}
	}

	return(parse_leave(p, 1));
}


static int
msg_err(void *arg, int line, int col, enum mdoc_err type)
{
	char		 *lit;
	struct md_parse	 *p;

	p = (struct md_parse *)arg;

	lit = NULL;

	switch (type) {
	case (ERR_SYNTAX_NOTEXT):
		lit = "syntax: context-free text disallowed";
		break;
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
		lit = "syntax: macro arguments malformed";
		break;
	case (ERR_SYNTAX_NOPUNCT):
		lit = "syntax: macro doesn't understand punctuation";
		break;
	case (ERR_SYNTAX_ARG):
		lit = "syntax: unknown argument for macro";
		break;
	case (ERR_SCOPE_BREAK):
		/* Which scope is broken? */
		lit = "scope: macro breaks prior explicit scope";
		break;
	case (ERR_SCOPE_NOCTX):
		lit = "scope: closure macro has no context";
		break;
	case (ERR_SCOPE_NONEST):
		lit = "scope: macro may not be nested in the current context";
		break;
	case (ERR_MACRO_NOTSUP):
		lit = "macro not supported";
		break;
	case (ERR_MACRO_NOTCALL):
		lit = "macro not callable";
		break;
	case (ERR_SEC_PROLOGUE):
		lit = "macro cannot be called in the prologue";
		break;
	case (ERR_SEC_NPROLOGUE):
		lit = "macro called outside of prologue";
		break;
	case (ERR_ARGS_EQ0):
		lit = "macro expects zero arguments";
		break;
	case (ERR_ARGS_EQ1):
		lit = "macro expects one argument";
		break;
	case (ERR_ARGS_GE1):
		lit = "macro expects one or more arguments";
		break;
	case (ERR_ARGS_LE2):
		lit = "macro expects two or fewer arguments";
		break;
	case (ERR_ARGS_LE8):
		lit = "macro expects eight or fewer arguments";
		break;
	case (ERR_ARGS_MANY):
		lit = "macro has too many arguments";
		break;
	case (ERR_SEC_PROLOGUE_OO):
		lit = "prologue macro is out-of-order";
		break;
	case (ERR_SEC_PROLOGUE_REP):
		lit = "prologue macro repeated";
		break;
	case (ERR_SEC_NAME):
		lit = "`NAME' section must be first";
		break;
	case (ERR_SYNTAX_ARGVAL):
		lit = "syntax: expected value for macro argument";
		break;
	case (ERR_SYNTAX_ARGBAD):
		lit = "syntax: invalid value(s) for macro argument";
		break;
	case (ERR_SYNTAX_ARGMISS):
		lit = "syntax: missing required argument(s) for macro";
		break;
	case (ERR_SYNTAX_ARGMANY):
		lit = "syntax: too many values for macro argument";
		break;
	case (ERR_SYNTAX_CHILDBAD):
		lit = "syntax: invalid child for parent macro";
		break;
	case (ERR_SYNTAX_PARENTBAD):
		lit = "syntax: invalid parent for macro";
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

	(void)fprintf(stderr, "%s:%d: error: %s (column %d)\n", 
			p->name, line, lit, col);
	return(0);
}


static void
msg_msg(void *arg, int line, int col, const char *msg)
{
	struct md_parse	 *p;

	p = (struct md_parse *)arg;

	if (p->dbg < 2)
		return;

	(void)printf("%s:%d: %s (column %d)\n", 
			p->name, line, msg, col);
}


static int
msg_warn(void *arg, int line, int col, enum mdoc_warn type)
{
	char		 *lit;
	struct md_parse	 *p;
	extern char	 *__progname;

	p = (struct md_parse *)arg;

	if ( ! (p->warn & MD_WARN_ALL))
		return(1);

	lit = NULL;

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
		lit = "syntax: macro suggests non-empty block-body section";
		break;
	case (WARN_SYNTAX_EMPTYHEAD):
		lit = "syntax: macro suggests non-empty block-head section";
		break;
	case (WARN_SYNTAX_NOBODY):
		lit = "syntax: macro suggests empty block-body section";
		break;
	case (WARN_SEC_OO):
		lit = "section is out of conventional order";
		break;
	case (WARN_SEC_REP):
		lit = "section repeated";
		break;
	case (WARN_ARGS_GE1):
		lit = "macro suggests one or more arguments";
		break;
	case (WARN_ARGS_EQ0):
		lit = "macro suggests zero arguments";
		break;
	case (WARN_IGN_AFTER_BLK):
		lit = "ignore: macro ignored after block macro";
		break;
	case (WARN_IGN_OBSOLETE):
		lit = "ignore: macro is obsolete";
		break;
	case (WARN_IGN_BEFORE_BLK):
		lit = "ignore: macro before block macro ignored";
		break;
	case (WARN_COMPAT_TROFF):
		lit = "compat: macro behaves differently in troff and nroff";
		break;
	default:
		abort();
		/* NOTREACHED */
	}


	(void)fprintf(stderr, "%s:%d: warning: %s (column %d)\n", 
			p->name, line, lit, col);

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

