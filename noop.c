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
#include <stdlib.h>
#include <string.h>

#include "private.h"


struct	md_noop {
	const struct md_args	*args;
	const struct md_rbuf	*rbuf;
	struct rofftree	 	*tree;
};


static	int		 noop_roffmsg(void *arg, 
				enum roffmsg, const char *, 
				const char *, const char *);
static	int		 noop_roffhead(void *, const struct tm *, 
				const char *, const char *, 
				enum roffmsec, enum roffvol);
static	int		 noop_rofftail(void *, const struct tm *, 
				const char *, const char *, 
				enum roffmsec, enum roffvol);
static	int		 noop_roffin(void *, int, 
				int *, const char **);
static	int		 noop_roffdata(void *, int, 
				const char *, const char *);
static	int		 noop_roffout(void *, int);
static	int		 noop_roffblkin(void *, int, int *, 
				const char **);
static	int		 noop_roffblkout(void *, int);
static	int		 noop_roffspecial(void *, int, 
				const char *, const int *,
				const char **, const char **);
static	int		 noop_roffblkheadin(void *, int, 
				int *, const char **);
static	int		 noop_roffblkheadout(void *, int);
static	int		 noop_roffblkbodyin(void *, int, 
				int *, const char **);
static	int		 noop_roffblkbodyout(void *, int);

#ifdef __linux__
extern	size_t		 strlcat(char *, const char *, size_t);
extern	size_t		 strlcpy(char *, const char *, size_t);
#endif


int
md_exit_noop(void *data, int flush)
{
	struct md_noop	*noop;
	int		 c;

	noop = (struct md_noop *)data;
	c = roff_free(noop->tree, flush);
	free(noop);
	return(c);
}


int
md_line_noop(void *data, char *buf)
{
	struct md_noop	*noop;

	noop = (struct md_noop *)data;
	return(roff_engine(noop->tree, buf));
}


/* ARGSUSED */
void *
md_init_noop(const struct md_args *args, 
		struct md_mbuf *mbuf, const struct md_rbuf *rbuf)
{
	struct roffcb	 cb;
	struct md_noop	*noop;

	if (NULL == (noop = calloc(1, sizeof(struct md_noop))))
		err(1, "calloc");

	noop->args = args;
	noop->rbuf = rbuf;

	cb.roffhead = noop_roffhead;
	cb.rofftail = noop_rofftail;
	cb.roffin = noop_roffin;
	cb.roffout = noop_roffout;
	cb.roffblkin = noop_roffblkin;
	cb.roffblkheadin = noop_roffblkheadin;
	cb.roffblkheadout = noop_roffblkheadout;
	cb.roffblkbodyin = noop_roffblkbodyin;
	cb.roffblkbodyout = noop_roffblkbodyout;
	cb.roffblkout = noop_roffblkout;
	cb.roffspecial = noop_roffspecial;
	cb.roffmsg = noop_roffmsg;
	cb.roffdata = noop_roffdata;

	if (NULL == (noop->tree = roff_alloc(&cb, noop))) {
		free(noop);
		return(NULL);
	}
	return(noop);
}


/* ARGSUSED */
static int
noop_roffhead(void *arg, const struct tm *tm, const char *os, 
		const char *title, enum roffmsec sec, enum roffvol vol)
{

	return(1);
}


/* ARGSUSED */
static int
noop_rofftail(void *arg, const struct tm *tm, const char *os, 
		const char *title, enum roffmsec sec, enum roffvol vol)
{

	return(1);
}


/* ARGSUSED */
static int
noop_roffspecial(void *arg, int tok, const char *start, 
		const int *argc, const char **argv, const char **more)
{

	return(1);
}


/* ARGSUSED */
static int
noop_roffblkin(void *arg, int tok, 
		int *argc, const char **argv)
{

	return(1);
}


/* ARGSUSED */
static int
noop_roffblkout(void *arg, int tok)
{

	return(1);
}


/* ARGSUSED */
static int
noop_roffblkbodyin(void *arg, int tok, 
		int *argc, const char **argv)
{

	return(1);
}


/* ARGSUSED */
static int
noop_roffblkbodyout(void *arg, int tok)
{

	return(1);
}


/* ARGSUSED */
static int
noop_roffblkheadin(void *arg, int tok, 
		int *argc, const char **argv)
{

	return(1);
}


/* ARGSUSED */
static int
noop_roffblkheadout(void *arg, int tok)
{

	return(1);
}


/* ARGSUSED */
static int
noop_roffin(void *arg, int tok, int *argc, const char **argv)
{

	return(1);
}


/* ARGSUSED */
static int
noop_roffout(void *arg, int tok)
{

	return(1);
}


/* ARGSUSED */
static int
noop_roffdata(void *arg, int tok, 
		const char *start, const char *buf)
{

	return(1);
}


static int
noop_roffmsg(void *arg, enum roffmsg lvl, 
		const char *buf, const char *pos, const char *msg)
{
	struct md_noop	*p;
	char		*level;
	char		 b[256];
	int		 i;

	p = (struct md_noop *)arg;
	assert(p);

	switch (lvl) {
	case (ROFF_WARN):
		level = "warning";
		if ( ! (MD_WARN_ALL & p->args->warnings))
			return(1);
		break;
	case (ROFF_ERROR):
		level = "error";
		break;
	default:
		abort();
		/* NOTREACHED */
	}

	if (pos) {
		assert(pos >= buf);
		if (0 < p->args->verbosity) {
			(void)snprintf(b, sizeof(b), 
					"%s:%zu: %s: %s\n",
					p->rbuf->name, p->rbuf->line, 
					level, msg);
			(void)strlcat(b, "Error at: ", sizeof(b));
			(void)strlcat(b, p->rbuf->linebuf, sizeof(b));

			(void)strlcat(b, "\n          ", sizeof(b));
			for (i = 0; i < pos - buf; i++)
				(void)strlcat(b, " ", sizeof(b));
			(void)strlcat(b, "^", sizeof(b));

		} else
			(void)snprintf(b, sizeof(b), 
					"%s:%zu: %s: %s (%zu)", 
					p->rbuf->name, p->rbuf->line, 
					level, msg, pos - buf);
	} else 
		(void)snprintf(b, sizeof(b), "%s: %s: %s", 
				p->rbuf->name, level, msg);

	(void)fprintf(stderr, "%s\n", b);
	return(lvl == ROFF_WARN ? 1 : 0);
}

