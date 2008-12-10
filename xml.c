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
#include <stdlib.h>
#include <string.h>

#include "private.h"
#include "ml.h"


static	int		xml_alloc(void **);
static	void		xml_free(void *);
static	ssize_t		xml_endtag(struct md_mbuf *, void *,
				const struct md_args *, 
				enum md_ns, int);
static	ssize_t		xml_begintag(struct md_mbuf *, void *,
				const struct md_args *, 
				enum md_ns, int, 
				const int *, const char **);
static	ssize_t		xml_beginstring(struct md_mbuf *, 
				const struct md_args *, 
				const char *, size_t);
static	ssize_t		xml_endstring(struct md_mbuf *, 
				const struct md_args *, 
				const char *, size_t);
static	int		xml_begin(struct md_mbuf *,
	       			const struct md_args *, 
				const struct tm *, 
				const char *, const char *, 
				enum roffmsec, enum roffvol);
static	int		xml_end(struct md_mbuf *, 
				const struct md_args *);
static	ssize_t 	xml_printtagname(struct md_mbuf *, 
				enum md_ns, int);
static	ssize_t 	xml_printtagargs(struct md_mbuf *, 
				const int *, const char **);


static ssize_t 
xml_printtagargs(struct md_mbuf *mbuf, const int *argc,
		const char **argv)
{
	int		 i, c;
	size_t		 res;

	if (NULL == argc || NULL == argv)
		return(0);
	assert(argc && argv);

	/* LINTED */
	for (res = 0, i = 0; ROFF_ARGMAX != (c = argc[i]); i++) {
		if ( ! ml_nputs(mbuf, " ", 1, &res))
			return(-1);

		/* FIXME: should puke on some, no? */

		if ( ! ml_puts(mbuf, tokargnames[c], &res))
			return(-1);
		if ( ! ml_nputs(mbuf, "=\"", 2, &res))
			return(-1);
		if (argv[i]) {
			if ( ! ml_putstring(mbuf, argv[i], &res))
				return(-1);
		} else if ( ! ml_nputs(mbuf, "true", 4, &res))
			return(-1);
		if ( ! ml_nputs(mbuf, "\"", 1, &res))
			return(-1);
	}

	return((ssize_t)res);
}


static ssize_t 
xml_printtagname(struct md_mbuf *mbuf, enum md_ns ns, int tok)
{
	size_t		 res;

	res = 0;
	switch (ns) {
	case (MD_NS_BLOCK):
		if ( ! ml_nputs(mbuf, "block:", 6, &res))
			return(-1);
		break;
	case (MD_NS_INLINE):
		if ( ! ml_nputs(mbuf, "inline:", 7, &res))
			return(-1);
		break;
	case (MD_NS_BODY):
		if ( ! ml_nputs(mbuf, "body:", 5, &res))
			return(-1);
		break;
	case (MD_NS_HEAD):
		if ( ! ml_nputs(mbuf, "head:", 5, &res))
			return(-1);
		break;
	default:
		break;
	}

	if ( ! ml_puts(mbuf, toknames[tok], &res))
		return(-1);
	return((ssize_t)res);
}


/* ARGSUSED */
static int 
xml_begin(struct md_mbuf *mbuf, const struct md_args *args,
		const struct tm *tm, const char *os, 
		const char *title, enum roffmsec sec, enum roffvol vol)
{

	if ( ! ml_puts(mbuf, "<?xml version=\"1.0\" "
				"encoding=\"UTF-8\"?>\n", NULL))
		return(0);
	return(ml_puts(mbuf, "<mdoc xmlns:block=\"block\" "
				"xmlns:body=\"body\" "
				"xmlns:head=\"head\" "
				"xmlns:inline=\"inline\">", NULL));
}


/* ARGSUSED */
static int 
xml_end(struct md_mbuf *mbuf, const struct md_args *args)
{

	return(ml_puts(mbuf, "</mdoc>", NULL));
}


/* ARGSUSED */
static ssize_t 
xml_beginstring(struct md_mbuf *mbuf, 
		const struct md_args *args, 
		const char *buf, size_t sz)
{

	return(0);
}


/* ARGSUSED */
static ssize_t 
xml_endstring(struct md_mbuf *mbuf, 
		const struct md_args *args, 
		const char *buf, size_t sz)
{

	return(0);
}


/* ARGSUSED */
static ssize_t 
xml_begintag(struct md_mbuf *mbuf, void *data,
		const struct md_args *args, enum md_ns ns, 
		int tok, const int *argc, const char **argv)
{
	ssize_t		 res, sz;

	if (-1 == (res = xml_printtagname(mbuf, ns, tok)))
		return(-1);
	if (-1 == (sz = xml_printtagargs(mbuf, argc, argv)))
		return(-1);
	return(res + sz);
}


/* ARGSUSED */
static ssize_t 
xml_endtag(struct md_mbuf *mbuf, void *data,
		const struct md_args *args, enum md_ns ns, int tok)
{

	return(xml_printtagname(mbuf, ns, tok));
}


/* ARGSUSED */
int
xml_alloc(void **p)
{

	*p = NULL;
	return(1);
}


/* ARGSUSED */
void
xml_free(void *p)
{

	/* Do nothing. */
}


int
md_line_xml(void *data, char *buf)
{

	return(mlg_line((struct md_mlg *)data, buf));
}


int
md_exit_xml(void *data, int flush)
{

	return(mlg_exit((struct md_mlg *)data, flush));
}


void *
md_init_xml(const struct md_args *args,
		struct md_mbuf *mbuf, const struct md_rbuf *rbuf)
{
	struct ml_cbs	 cbs;

	cbs.ml_alloc = xml_alloc;
	cbs.ml_free = xml_free;
	cbs.ml_begintag = xml_begintag;
	cbs.ml_endtag = xml_endtag;
	cbs.ml_begin = xml_begin;
	cbs.ml_end = xml_end;
	cbs.ml_beginstring = xml_beginstring;
	cbs.ml_endstring = xml_endstring;

	return(mlg_alloc(args, rbuf, mbuf, &cbs));
}

