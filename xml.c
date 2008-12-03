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
#include <stdlib.h>
#include <string.h>

#include "libmdocml.h"
#include "private.h"
#include "ml.h"


static	ssize_t		xml_endtag(struct md_mbuf *, 
				const struct md_args *, 
				enum md_ns, int);
static	ssize_t		xml_begintag(struct md_mbuf *, 
				const struct md_args *, 
				enum md_ns, int, 
				const int *, const char **);

static ssize_t 
xml_begintag(struct md_mbuf *mbuf, const struct md_args *args, 
		enum md_ns ns, int tok, 
		const int *argc, const char **argv)
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
	default:
		if ( ! ml_nputs(mbuf, "mbuf", 4, &res))
			return(-1);
		return((ssize_t)res);
	}

	if ( ! ml_nputs(mbuf, toknames[tok], 
				strlen(toknames[tok]), &res))
		return(-1);

	return((ssize_t)res);
}


static ssize_t 
xml_endtag(struct md_mbuf *mbuf, const struct md_args *args, 
		enum md_ns ns, int tok)
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
	default:
		if ( ! ml_nputs(mbuf, "mbuf", 4, &res))
			return(-1);
		return((ssize_t)res);
	}

	if ( ! ml_nputs(mbuf, toknames[tok], 
				strlen(toknames[tok]), &res))
		return(-1);

	return((ssize_t)res);
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

	return(mlg_alloc(args, rbuf, mbuf, xml_begintag, xml_endtag));
}

