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

#include "mmain.h"

#define	MD_LINE_SZ	(256)		/* Input line step-size. */

struct	mmain {
	int		  warn;		/* Warning flags. */
#define	MD_WARN_SYNTAX	 (1 << 0)	/* Show syntax warnings. */
#define	MD_WARN_COMPAT	 (1 << 1)	/* Show compat warnings. */
#define	MD_WARN_ALL	 (0x03)		/* Show all warnings. */
#define	MD_WARN_ERR	 (1 << 2)	/* Make warnings->errors. */
	int		  dbg;		/* Debug level. */
	struct mdoc	 *mdoc;		/* Active parser. */
	char		 *buf;		/* Input buffer. */
	size_t		  bufsz;	/* Input buffer size. */
	char		 *in; 		/* Input file name. */
	int		  fdin;		/* Input file desc. */
	int		  pflags;	/* Parse flags. */
};

extern	char	 	 *__progname;

static	int		  optswarn(struct mmain *, char *);
static	int		  optsopt(struct mmain *, char *);
static	int		  parse(struct mmain *);
static	void		  msg_msg(void *, int, int, const char *);
static	int		  msg_err(void *, int, int, const char *);
static	int		  msg_warn(void *, int, int, 
				enum mdoc_warn, const char *);

#ifdef __linux__
extern	int		  getsubopt(char **, char *const *, char **);
extern	size_t		  strlcpy(char *, const char *, size_t);
extern	size_t		  strlcat(char *, const char *, size_t);
#endif


/*
 * Print our and our caller's usage message.
 */
void
mmain_usage(const char *help)
{

	warnx("usage: %s %s%s[-v] [-foption...] [-Wwarn...] [infile]", __progname, 
			help ? help : "", help ? " " : "");
}


/*
 * Allocate the convenience library and initialise some values.
 */ 
struct mmain *
mmain_alloc(void)
{
	struct mmain	*p;

	if (NULL == (p = calloc(1, sizeof(struct mmain))))
		err(1, "malloc");

	p->in = "-";
	p->fdin = STDIN_FILENO;

	return(p);
}


/*
 * Parse command-line options.  Accepts a small (<28 char) opstring "u"
 * parameter (e.g. "ho:") or NULL, a corresponding "help" string (e.g.
 * "[-h] [-o output]" or NULL, a callback function for parsed arguments
 * and an opaque pointer argument for that function.
 */
int
mmain_getopt(struct mmain *p, int argc, char *argv[], 
		const char *help, const char *u, void *arg,
		int (*getopt_cb)(void *, int, const char *))
{
	int		 c;
	char		 opts[32]; /* XXX */
	size_t		 sz;

	extern int	 optind;

	sz = strlcpy(opts, "VvW:f:", 32);
	assert(sz < 32);

	if (u) {
		sz = strlcat(opts, u, 32);
		assert(sz < 32);
	}

	optind = 1;

	/* LINTED */
	while (-1 != (c = getopt(argc, argv, opts)))
		switch (c) {
		case ('f'):
			if ( ! optsopt(p, optarg))
				return(-1);
			break;
		case ('v'):
			p->dbg++;
			break;
		case ('V'):
			(void)printf("%s %s\n", __progname, VERSION);
			return(0);
		case ('W'):
			if ( ! optswarn(p, optarg))
				return(-1);
			break;
		case ('?'):
			mmain_usage(help);
			return(-1);
		default:
			assert(getopt_cb);
			if ((*getopt_cb)(arg, c, optarg))
				break;
			return(-1);
		}

	argv += optind;
	if ((argc -= optind) > 0)
		p->in = *argv++;

	return(1);
}


dead_pre void
mmain_exit(struct mmain *p, int code) 
{

	if (p->mdoc)
		mdoc_free(p->mdoc);
	free(p);
	exit(code);
}


struct mdoc *
mmain_mdoc(struct mmain *p)
{
	struct stat	 st;
	int		 c;
	struct mdoc_cb	 cb;

	cb.mdoc_err = msg_err;
	cb.mdoc_warn = msg_warn;
	cb.mdoc_msg = msg_msg;

	if (0 != strcmp(p->in, "-"))
		if (-1 == (p->fdin = open(p->in, O_RDONLY, 0))) {
			warn("%s", p->in);
			return(0);
		}

	/* Allocate a buffer to be BUFSIZ/block size. */

	if (-1 == fstat(p->fdin, &st)) {
		warn("%s", p->in);
		p->bufsz = BUFSIZ;
	} else 
		p->bufsz = (size_t)MAX(st.st_blksize, BUFSIZ);

	p->buf = malloc(p->bufsz);
	if (NULL == p->buf)
		err(1, "malloc");

	/* Allocate the parser. */

	p->mdoc = mdoc_alloc(p, p->pflags, &cb);

	/* Parse the input file. */

	c = parse(p);
	free(p->buf);

	if (STDIN_FILENO != p->fdin)
		if (-1 == close(p->fdin))
			warn("%s", p->in);

	return(c ? p->mdoc : NULL);
}


static int
optsopt(struct mmain *p, char *arg)
{
	char		*v;
	char		*toks[] = { "ign-scope", "ign-escape", 
				    "ign-macro", NULL };

	while (*arg) 
		switch (getsubopt(&arg, toks, &v)) {
		case (0):
			p->pflags |= MDOC_IGN_SCOPE;
			break;
		case (1):
			p->pflags |= MDOC_IGN_ESCAPE;
			break;
		case (2):
			p->pflags |= MDOC_IGN_MACRO;
			break;
		default:
			warnx("unknown -f argument");
			return(0);
		}

	return(1);
}


static int
optswarn(struct mmain *p, char *arg)
{
	char		*v;
	char		*toks[] = { "all", "compat", 
				"syntax", "error", NULL };

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
			warnx("unknown -W argument");
			return(0);
		}

	return(1);
}


static int
parse(struct mmain *p)
{
	ssize_t		 sz;
	int		 j, i, pos, len, lnn;
	char		*ln;

	for (ln = NULL, lnn = 1, len = pos = 0; ; ) {
		if (-1 == (sz = read(p->fdin, p->buf, p->bufsz))) {
			warn("%s", p->in);
			return(0);
		} else if (0 == sz) 
			break;

		for (i = 0; i < (int)sz; i++) {
			if (pos >= len) {
				len += MD_LINE_SZ;
				ln = realloc(ln, (size_t)len);
				if (NULL == ln)
					err(1, "realloc");
			}

			if ('\n' != p->buf[i]) {
				ln[pos++] = p->buf[i];
				continue;
			}

			/* Check for escaped newline.  */

			if (pos > 0 && '\\' == ln[pos - 1]) {
				for (j = pos - 1; j >= 0; j--)
					if ('\\' != ln[j])
						break;

				if ( ! ((pos - j) % 2)) {
					pos--;
					lnn++;
					continue;
				}
			}

			ln[pos] = 0;
			if ( ! mdoc_parseln(p->mdoc, lnn, ln))
				return(0);
			lnn++;
			pos = 0;
		}
	}

	if (pos > 0)
		warnx("%s: file not eof-terminated", p->in);

	if (ln)
		free(ln);
	return(mdoc_endparse(p->mdoc));
}


static int
msg_err(void *arg, int line, int col, const char *msg)
{
	struct mmain	 *p;

	p = (struct mmain *)arg;

	warnx("%s:%d: error: %s (column %d)", 
			p->in, line, msg, col);
	return(0);
}


static void
msg_msg(void *arg, int line, int col, const char *msg)
{
	struct mmain	 *p;

	p = (struct mmain *)arg;

	if (0 == p->dbg)
		return;

	warnx("%s:%d: debug: %s (column %d)", 
			p->in, line, msg, col);
}


static int
msg_warn(void *arg, int line, int col, 
		enum mdoc_warn type, const char *msg)
{
	struct mmain	 *p;

	p = (struct mmain *)arg;

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
