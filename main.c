/* $Id$ */
/*
 * Copyright (c) 2008, 2009 Kristaps Dzonsons <kristaps@openbsd.org>
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

#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mdoc.h"

#ifdef __linux__
extern	int		  getsubopt(char **, char * const *, char **);
# ifndef __dead
#  define __dead __attribute__((__noreturn__))
# endif
#endif

#define	WARN_WALL	  0x03		/* All-warnings mask. */
#define	WARN_WCOMPAT	 (1 << 0)	/* Compatibility warnings. */
#define	WARN_WSYNTAX	 (1 << 1)	/* Syntax warnings. */
#define	WARN_WERR	 (1 << 2)	/* Warnings->errors. */

enum outt {
	OUTT_ASCII,
	OUTT_LATIN1,
	OUTT_UTF8,
	OUTT_TREE,
	OUTT_LINT
};

typedef	int		(*out_run)(void *, const struct mdoc *);
typedef	void		(*out_free)(void *);

extern	char		 *__progname;

extern	void		 *ascii_alloc(void);
extern	void		 *latin1_alloc(void);
extern	void		 *utf8_alloc(void);
extern	int		  terminal_run(void *, const struct mdoc *);
extern	int		  tree_run(void *, const struct mdoc *);
extern	void		  terminal_free(void *);

__dead	static void	  version(void);
__dead	static void	  usage(void);
static	int		  foptions(int *, char *);
static	int		  toptions(enum outt *, char *);
static	int		  woptions(int *, char *);
static	int		  merr(void *, int, int, const char *);
static	int		  mwarn(void *, int, int, 
				enum mdoc_warn, const char *);
static	int		  file(char **, size_t *, char **, size_t *, 
				const char *, struct mdoc *);
static	int		  fdesc(char **, size_t *, char **, size_t *, 
				const char *, int, struct mdoc *);


int
main(int argc, char *argv[])
{
	int		 c, rc, fflags, wflags;
	struct mdoc_cb	 cb;
	struct mdoc	*mdoc;
	char		*buf, *line;
	size_t		 bufsz, linesz;
	void		*outdata;
	enum outt	 outtype;
	out_run		 outrun;
	out_free	 outfree;

	fflags = wflags = 0;
	outtype = OUTT_ASCII;

	/* LINTED */
	while (-1 != (c = getopt(argc, argv, "f:VW:T:")))
		switch (c) {
		case ('f'):
			if ( ! foptions(&fflags, optarg))
				return(0);
			break;
		case ('T'):
			if ( ! toptions(&outtype, optarg))
				return(0);
			break;
		case ('W'):
			if ( ! woptions(&wflags, optarg))
				return(0);
			break;
		case ('V'):
			version();
			/* NOTREACHED */
		default:
			usage();
			/* NOTREACHED */
		}

	argc -= optind;
	argv += optind;

	/*
	 * Allocate the appropriate front-end.  Note that utf8, ascii
	 * and latin1 all resolve to the terminal front-end with
	 * different encodings (see terminal.c).  Not all frontends have
	 * cleanup or alloc routines.
	 */

	switch (outtype) {
	case (OUTT_LATIN1):
		outdata = latin1_alloc();
		outrun = terminal_run;
		outfree = terminal_free;
		break;
	case (OUTT_UTF8):
		outdata = utf8_alloc();
		outrun = terminal_run;
		outfree = terminal_free;
		break;
	case (OUTT_TREE):
		outdata = NULL;
		outrun = tree_run;
		outfree = NULL;
		break;
	case (OUTT_LINT):
		outdata = NULL;
		outrun = NULL;
		outfree = NULL;
		break;
	default:
		outdata = ascii_alloc();
		outrun = terminal_run;
		outfree = terminal_free;
		break;
	}

	/*
	 * All callbacks route into here, where we print them onto the
	 * screen.  XXX - for now, no path for debugging messages.
	 */

	cb.mdoc_msg = NULL;
	cb.mdoc_err = merr;
	cb.mdoc_warn = mwarn;

	buf = line = NULL;
	bufsz = linesz = 0;

	mdoc = mdoc_alloc(&wflags, fflags, &cb);

	/*
	 * Loop around available files.
	 */

	if (NULL == *argv) {
		c = fdesc(&line, &linesz, &buf, &bufsz, 
				"stdin", STDIN_FILENO, mdoc);
		rc = 0;
		if (c && NULL == outrun)
			rc = 1;
		else if (c && outrun && (*outrun)(outdata, mdoc))
			rc = 1;
	} else {
		while (*argv) {
			c = file(&line, &linesz, &buf, 
					&bufsz, *argv, mdoc);
			if ( ! c)
				break;
			if (outrun && ! (*outrun)(outdata, mdoc))
				break;
			/* Reset the parser for another file. */
			mdoc_reset(mdoc);
			argv++;
		}
		rc = NULL == *argv;
	}

	if (buf)
		free(buf);
	if (line)
		free(line);
	if (outfree)
		(*outfree)(outdata);

	mdoc_free(mdoc);

	return(rc ? EXIT_SUCCESS : EXIT_FAILURE);
}


__dead static void
version(void)
{

	(void)printf("%s %s\n", __progname, VERSION);
	exit(0);
	/* NOTREACHED */
}


__dead static void
usage(void)
{

	(void)fprintf(stderr, "usage: %s\n", __progname);
	exit(1);
	/* NOTREACHED */
}


static int
file(char **ln, size_t *lnsz, char **buf, size_t *bufsz, 
		const char *file, struct mdoc *mdoc)
{
	int		 fd, c;

	if (-1 == (fd = open(file, O_RDONLY, 0))) {
		warn("%s", file);
		return(0);
	}

	c = fdesc(ln, lnsz, buf, bufsz, file, fd, mdoc);

	if (-1 == close(fd))
		warn("%s", file);

	return(c);
}


static int
fdesc(char **lnp, size_t *lnsz, char **bufp, size_t *bufsz, 
		const char *f, int fd, struct mdoc *mdoc)
{
	size_t		 sz;
	ssize_t		 ssz;
	struct stat	 st;
	int		 j, i, pos, lnn;
	char		*ln, *buf;

	buf = *bufp;
	ln = *lnp;

	/*
	 * Two buffers: ln and buf.  buf is the input buffer, optimised
	 * for each file's block size.  ln is a line buffer.  Both
	 * growable, hence passed in by ptr-ptr.
	 */

	if (-1 == fstat(fd, &st)) {
		warnx("%s", f);
		sz = BUFSIZ;
	} else 
		sz = (unsigned)BUFSIZ > st.st_blksize ?
			(size_t)BUFSIZ : st.st_blksize;

	if (sz > *bufsz) {
		if (NULL == (buf = realloc(buf, sz)))
			err(1, "realloc");
		*bufp = buf;
		*bufsz = sz;
	}

	/*
	 * Fill buf with file blocksize and parse newlines into ln.
	 */

	for (lnn = 1, pos = 0; ; ) {
		if (-1 == (ssz = read(fd, buf, sz))) {
			warn("%s", f);
			return(0);
		} else if (0 == ssz) 
			break;

		for (i = 0; i < (int)ssz; i++) {
			if (pos >= (int)*lnsz) {
				*lnsz += 256; /* Step-size. */
				ln = realloc(ln, *lnsz);
				if (NULL == ln)
					err(1, "realloc");
				*lnp = ln;
			}

			if ('\n' != buf[i]) {
				ln[pos++] = buf[i];
				continue;
			}

			/* Check for CPP-escaped newline.  */

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
			if ( ! mdoc_parseln(mdoc, lnn, ln))
				return(0);
			lnn++;
			pos = 0;
		}
	}

	return(mdoc_endparse(mdoc));
}


static int
toptions(enum outt *tflags, char *arg)
{

	if (0 == strcmp(arg, "ascii"))
		*tflags = OUTT_ASCII;
	else if (0 == strcmp(arg, "latin1"))
		*tflags = OUTT_LATIN1;
	else if (0 == strcmp(arg, "utf8"))
		*tflags = OUTT_UTF8;
	else if (0 == strcmp(arg, "lint"))
		*tflags = OUTT_LINT;
	else if (0 == strcmp(arg, "tree"))
		*tflags = OUTT_TREE;
	else {
		warnx("bad argument: -T%s", arg);
		return(0);
	}

	return(1);
}


/*
 * Parse out the options for [-fopt...] setting compiler options.  These
 * can be comma-delimited or called again.
 */
static int
foptions(int *fflags, char *arg)
{
	char		*v;
	char		*toks[4];

	toks[0] = "ign-scope";
	toks[1] = "ign-escape";
	toks[2] = "ign-macro";
	toks[3] = NULL;

	while (*arg) 
		switch (getsubopt(&arg, toks, &v)) {
		case (0):
			*fflags |= MDOC_IGN_SCOPE;
			break;
		case (1):
			*fflags |= MDOC_IGN_ESCAPE;
			break;
		case (2):
			*fflags |= MDOC_IGN_MACRO;
			break;
		default:
			warnx("bad argument: -f%s", arg);
			return(0);
		}

	return(1);
}


/* 
 * Parse out the options for [-Werr...], which sets warning modes.
 * These can be comma-delimited or called again.  
 */
static int
woptions(int *wflags, char *arg)
{
	char		*v;
	char		*toks[5]; 

	toks[0] = "all";
	toks[1] = "compat";
	toks[2] = "syntax";
	toks[3] = "error";
	toks[4] = NULL;

	while (*arg) 
		switch (getsubopt(&arg, toks, &v)) {
		case (0):
			*wflags |= WARN_WALL;
			break;
		case (1):
			*wflags |= WARN_WCOMPAT;
			break;
		case (2):
			*wflags |= WARN_WSYNTAX;
			break;
		case (3):
			*wflags |= WARN_WERR;
			break;
		default:
			warnx("bad argument: -W%s", arg);
			return(0);
		}

	return(1);
}


/* ARGSUSED */
static int
merr(void *arg, int line, int col, const char *msg)
{

	warnx("error: %s (line %d, column %d)", msg, line, col);
	return(0);
}


static int
mwarn(void *arg, int line, int col, 
		enum mdoc_warn type, const char *msg)
{
	int		 flags;
	char		*wtype;

	flags = *(int *)arg;
	wtype = NULL;

	switch (type) {
	case (WARN_COMPAT):
		wtype = "compat";
		if (flags & WARN_WCOMPAT)
			break;
		return(1);
	case (WARN_SYNTAX):
		wtype = "syntax";
		if (flags & WARN_WSYNTAX)
			break;
		return(1);
	}

	assert(wtype);
	warnx("%s warning: %s (line %d, column %d)", 
			wtype, msg, line, col);

	if ( ! (flags & WARN_WERR))
		return(1);

	warnx("%s: considering warnings as errors", 
			__progname);
	return(0);
}


