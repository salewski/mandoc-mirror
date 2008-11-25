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
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libmdocml.h"
#include "private.h"

#ifdef	__Linux__
#define	strlcat		strncat
#endif

static	int		md_dummy_blk_in(int);
static	int		md_dummy_blk_out(int);
static 	int		md_dummy_text_in(int, int *, char **);
static	int		md_dummy_text_out(int);

static	void		dbg_indent(void);

static	int		dbg_lvl = 0;

struct	md_dummy {
	struct rofftree	*tree;
};

static	const char *const toknames[ROFF_MAX] = ROFF_NAMES;


static void
dbg_indent(void)
{
	char		buf[128];
	int		i;

	*buf = 0;
	assert(dbg_lvl >= 0);

	/* LINTED */
	for (i = 0; i < dbg_lvl; i++)
		(void)strlcat(buf, "  ", sizeof(buf) - 1);

	(void)printf("%s", buf);
}


static int
md_dummy_blk_in(int tok)
{

	dbg_indent();
	(void)printf("%s\n", toknames[tok]);
	dbg_lvl++;
	return(1);
}


static int
md_dummy_blk_out(int tok)
{

	assert(dbg_lvl > 0);
	dbg_lvl--;
	dbg_indent();
	(void)printf("%s\n", toknames[tok]);
	return(1);
}


/* ARGSUSED */
static int
md_dummy_text_in(int tok, int *argcp, char **argvp)
{

	dbg_indent();
	(void)printf("%s\n", toknames[tok]);
	return(1);
}


static int
md_dummy_text_out(int tok)
{

	dbg_indent();
	(void)printf("%s\n", toknames[tok]);
	return(1);
}


int
md_line_dummy(void *arg, char *buf, size_t sz)
{
	struct md_dummy	*p;

	p = (struct md_dummy *)arg;
	return(roff_engine(p->tree, buf, sz));
}


int
md_exit_dummy(void *data, int flush)
{
	int		 c;
	struct md_dummy	*p;

	p = (struct md_dummy *)data;
	c = roff_free(p->tree, flush);
	free(p);

	return(c);
}


void *
md_init_dummy(const struct md_args *args,
		struct md_mbuf *mbuf, const struct md_rbuf *rbuf)
{
	struct md_dummy	*p;

	if (NULL == (p = malloc(sizeof(struct md_dummy)))) {
		warn("malloc");
		return(NULL);
	}

	p->tree = roff_alloc(args, mbuf, rbuf, 
			md_dummy_text_in, md_dummy_text_out, 
			md_dummy_blk_in, md_dummy_blk_out);

	if (NULL == p->tree) {
		free(p);
		return(NULL);
	}

	return(p);
}
