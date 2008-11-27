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

#ifdef	__linux__
#define	strlcat		strncat
#endif

static	int		md_dummy_blk_in(int);
static	int		md_dummy_blk_out(int);
static 	int		md_dummy_text_in(int, int *, char **);
static	int		md_dummy_text_out(int);
static	int		md_dummy_special(int);
static	int		md_dummy_head(void);
static	int		md_dummy_tail(void);
static	void		md_dummy_msg(const struct md_args *, 
				enum roffmsg, const char *,
				const char *, const char *,
				int, char *);

static	void		dbg_prologue(const char *);
static	void		dbg_epilogue(void);

static	int		dbg_lvl = 0;
static	char		dbg_line[72];

struct	md_dummy {
	struct rofftree	*tree;
	struct roffcb	 cb;
};


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

	p->cb.roffhead = md_dummy_head;
	p->cb.rofftail = md_dummy_tail;
	p->cb.roffin = md_dummy_text_in;
	p->cb.roffout = md_dummy_text_out;
	p->cb.roffblkin = md_dummy_blk_in;
	p->cb.roffblkout = md_dummy_blk_out;
	p->cb.roffspecial = md_dummy_special;
	p->cb.roffmsg = md_dummy_msg;

	p->tree = roff_alloc(args, mbuf, rbuf, &p->cb);

	if (NULL == p->tree) {
		free(p);
		return(NULL);
	}

	return(p);
}


static void
dbg_prologue(const char *p)
{
	int		 i;

	(void)snprintf(dbg_line, sizeof(dbg_line) - 1, "%6s", p);
	(void)strlcat(dbg_line, ": ", sizeof(dbg_line) - 1);
	/* LINTED */
	for (i = 0; i < dbg_lvl; i++)
		(void)strlcat(dbg_line, "    ", sizeof(dbg_line) - 1);
}


static void
dbg_epilogue(void)
{

	assert(0 != dbg_line[0]);
	(void)printf("%s\n", dbg_line);
}


static int
md_dummy_head(void)
{

	return(1);
}


static int
md_dummy_tail(void)
{

	return(1);
}


static int
md_dummy_special(int tok)
{

	dbg_prologue("noop");
	(void)strlcat(dbg_line, toknames[tok], sizeof(dbg_line) - 1);
	dbg_epilogue();

	return(1);
}


static int
md_dummy_blk_in(int tok)
{

	dbg_prologue("blk");
	(void)strlcat(dbg_line, toknames[tok], sizeof(dbg_line) - 1);
	dbg_epilogue();

	dbg_lvl++;
	return(1);
}


static int
md_dummy_blk_out(int tok)
{

	dbg_lvl--;
	return(1);
}


/* ARGSUSED */
static int
md_dummy_text_in(int tok, int *argcp, char **argvp)
{

	dbg_prologue("text");
	(void)strlcat(dbg_line, toknames[tok], sizeof(dbg_line) - 1);
	(void)strlcat(dbg_line, " ", sizeof(dbg_line) - 1);
	while (ROFF_ARGMAX != *argcp) {
		(void)strlcat(dbg_line, "[", sizeof(dbg_line) - 1);
		(void)strlcat(dbg_line, tokargnames[*argcp], 
				sizeof(dbg_line) - 1);
		if (*argvp) {
			(void)strlcat(dbg_line, " [", 
					sizeof(dbg_line) - 1);
			(void)strlcat(dbg_line, *argvp,
					sizeof(dbg_line) - 1);
			(void)strlcat(dbg_line, "]", 
					sizeof(dbg_line) - 1);
		}
		(void)strlcat(dbg_line, "]", sizeof(dbg_line) - 1);
		argcp++;
		argvp++;
	}
	dbg_epilogue();
	return(1);
}


static int
md_dummy_text_out(int tok)
{

	return(1);
}


static void
md_dummy_msg(const struct md_args *args, enum roffmsg lvl, 
		const char *buf, const char *pos, 
		const char *name, int line, char *msg)
{
	char		*p;

	switch (lvl) {
	case (ROFF_WARN):
		p = "warning";
		break;
	case (ROFF_ERROR):
		p = "error";
		break;
	}

	assert(pos >= buf);
	(void)fprintf(stderr, "%s:%d: %s: %s\n", name, line, p, msg);
}
