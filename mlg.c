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
#include <ctype.h>
#include <err.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ml.h"

/* TODO: literal tokens. */

enum	md_tok {
	MD_TEXT,
	MD_INLINE_IN,
	MD_INLINE_OUT,
	MD_BLK_IN,
	MD_BLK_OUT
};

struct	md_mlg {
	const struct md_args	*args;
	const struct md_rbuf	*rbuf;

	struct md_mbuf	 *mbuf;
	struct rofftree	 *tree;
	size_t		  indent;
	size_t		  pos;
	enum md_tok	  last;
	void		 *arg;
	struct ml_cbs	  cbs;
	int		  flags;
#define	ML_OVERRIDE_ONE	 (1 << 0)
#define	ML_OVERRIDE_ALL	 (1 << 1)
	void		 *data;
};

static	int		 mlg_roffmsg(void *arg, 
				enum roffmsg, const char *, 
				const char *, const char *);
static	int		 mlg_roffhead(void *, const struct tm *, 
				const char *, const char *, 
				enum roffmsec, enum roffvol);
static	int		 mlg_rofftail(void *, const struct tm *, 
				const char *, const char *, 
				enum roffmsec, enum roffvol);
static	int		 mlg_roffin(void *, int, 
				int *, const char **);
static	int		 mlg_roffdata(void *, int, 
				const char *, const char *);
static	int		 mlg_roffout(void *, int);
static	int		 mlg_roffblkin(void *, int, int *, 
				const char **);
static	int		 mlg_roffblkout(void *, int);
static	int		 mlg_roffspecial(void *, int, 
				const char *, const int *,
				const char **, const char **);
static	int		 mlg_roffblkheadin(void *, int, 
				int *, const char **);
static	int		 mlg_roffblkheadout(void *, int);
static	int		 mlg_roffblkbodyin(void *, int, 
				int *, const char **);
static	int		 mlg_roffblkbodyout(void *, int);

static	int		 mlg_ref_special(struct md_mlg *, int,
				const char *, const char **);
static	int		 mlg_formatted_special(struct md_mlg *, 
				int, const char *, const int *, 
				const char **, const char **);
static	int		 mlg_literal_special(struct md_mlg *, 
				int,  const char *, const int *, 
				const char **, const char **);
static	int		 mlg_function_special(struct md_mlg *, 
				const char *, const char **);
static	int		 mlg_atom_special(struct md_mlg *, int,
				const char *, const char **);

static	int		 mlg_begintag(struct md_mlg *, enum md_ns, 
				int, int *, const char **);
static	int		 mlg_endtag(struct md_mlg *, enum md_ns, int);
static	int	  	 mlg_indent(struct md_mlg *);
static	int		 mlg_newline(struct md_mlg *);
static	void		 mlg_mode(struct md_mlg *, enum md_tok);
static	int		 mlg_nstring(struct md_mlg *, 
				const char *, const char *, size_t);
static	int		 mlg_string(struct md_mlg *,
				const char *, const char *);
static	int		 mlg_data(struct md_mlg *, int, 
				const char *, const char *);
static	int		 mlg_err(struct md_mlg *, const char *, 
				const char *, const char *, ...);
static	int		 mlg_msg(struct md_mlg *, 
				enum roffmsg, const char *, 
				const char *, const char *);
static	int		 mlg_vmsg(struct md_mlg *, enum roffmsg, 
				const char *, const char *, 
				const char *, va_list);

#ifdef __linux__
extern	size_t		 strlcat(char *, const char *, size_t);
extern	size_t		 strlcpy(char *, const char *, size_t);
#endif


static int
mlg_begintag(struct md_mlg *p, enum md_ns ns, int tok,
		int *argc, const char **argv)
{
	ssize_t		 res;

	assert(MD_NS_DEFAULT != ns);

	switch (ns) {
	case (MD_NS_INLINE):
		if ( ! (ML_OVERRIDE_ONE & p->flags) && 
				! (ML_OVERRIDE_ALL & p->flags) && 
				p->pos + 11 >= COLUMNS)
			if ( ! mlg_newline(p))
				return(0);
		if (0 != p->pos && (MD_TEXT == p->last || 
					MD_INLINE_OUT == p->last)
				&& ! (ML_OVERRIDE_ONE & p->flags)
				&& ! (ML_OVERRIDE_ALL & p->flags))
			if ( ! ml_nputs(p->mbuf, " ", 1, &p->pos))
				return(0);
		if (0 == p->pos && ! mlg_indent(p))
			return(0);
		mlg_mode(p, MD_INLINE_IN);
		break;
	default:
		if (0 != p->pos) {
			if ( ! mlg_newline(p))
				return(0);
			if ( ! mlg_indent(p))
				return(0);
		} else if ( ! mlg_indent(p))
			return(0);
		p->indent++;
		mlg_mode(p, MD_BLK_IN);
		break;
	}

	if ( ! ml_nputs(p->mbuf, "<", 1, &p->pos))
		return(0);

	res = (*p->cbs.ml_begintag)(p->mbuf, p->data, 
			p->args, ns, tok, argc, argv);
	if (-1 == res)
		return(0);

	assert(res >= 0);
	p->pos += (size_t)res;

	if ( ! ml_nputs(p->mbuf, ">", 1, &p->pos))
		return(0);

	switch (ns) {
	case (MD_NS_INLINE):
		break;
	default:
		if ( ! mlg_newline(p))
			return(0);
		break;
	}

	return(1);
}


static int
mlg_endtag(struct md_mlg *p, enum md_ns ns, int tok)
{
	ssize_t		 res;

	assert(MD_NS_DEFAULT != ns);

	switch (ns) {
	case (MD_NS_INLINE):
		break;
	default:
		p->indent--;
		if (0 != p->pos) {
			if ( ! mlg_newline(p))
				return(0);
			if ( ! mlg_indent(p))
				return(0);
		} else if ( ! mlg_indent(p))
			return(0);
		break;
	}

	if ( ! ml_nputs(p->mbuf, "</", 2, &p->pos))
		return(0);

	res = (*p->cbs.ml_endtag)(p->mbuf, p->data, p->args, ns, tok);
	if (-1 == res)
		return(0);

	assert(res >= 0);
	p->pos += (size_t)res;

	if ( ! ml_nputs(p->mbuf, ">", 1, &p->pos))
		return(0);
	
	switch (ns) {
	case (MD_NS_INLINE):
		mlg_mode(p, MD_INLINE_OUT);
		break;
	default:
		mlg_mode(p, MD_BLK_OUT);
		break;
	}

	return(1);
}


static int
mlg_indent(struct md_mlg *p)
{

	assert(0 == p->pos);
	return(ml_putchars(p->mbuf, ' ', INDENT_SZ * 
				INDENT(p->indent), &p->pos));
}


static int
mlg_newline(struct md_mlg *p)
{

	p->pos = 0;
	return(ml_nputs(p->mbuf, "\n", 1, NULL));
}


static void
mlg_mode(struct md_mlg *p, enum md_tok ns)
{

	p->flags &= ~ML_OVERRIDE_ONE;
	p->last = ns;
}


static int
mlg_string(struct md_mlg *p, const char *start, const char *buf)
{
	
	return(mlg_nstring(p, start, buf, strlen(buf)));
}


static int
mlg_nstring(struct md_mlg *p, const char *start, 
		const char *buf, size_t sz)
{
	int		 c;
	ssize_t		 res;

	assert(p->mbuf);
	assert(0 != p->indent);

	res = (*p->cbs.ml_beginstring)(p->mbuf, p->args, buf, sz);
	if (-1 == res) 
		return(0);

	if (0 == (c = ml_nputstring(p->mbuf, buf, sz, &p->pos)))
		return(mlg_err(p, start, buf, "bad string "
					"encoding: `%s'", buf));
	else if (-1 == c)
		return(0);

	res = (*p->cbs.ml_endstring)(p->mbuf, p->args, buf, sz);
	if (-1 == res) 
		return(0);

	return(1);
}


static int
mlg_data(struct md_mlg *p, int space, 
		const char *start, const char *buf)
{
	size_t		 sz;

	assert(p->mbuf);
	assert(0 != p->indent);

	if (ML_OVERRIDE_ONE & p->flags || 
			ML_OVERRIDE_ALL & p->flags)
		space = 0;

	sz = strlen(buf);

	if (0 == p->pos) {
		if ( ! mlg_indent(p))
			return(0);
		if ( ! mlg_nstring(p, start, buf, sz))
			return(0);

		if (INDENT(p->indent) * INDENT_SZ + sz >= COLUMNS)
			if ( ! mlg_newline(p))
				return(0);

		return(1);
	}

	if (space && sz + p->pos >= COLUMNS) {
		if ( ! mlg_newline(p))
			return(0);
		if ( ! mlg_indent(p))
			return(0);
	} else if (space) {
		if ( ! ml_nputs(p->mbuf, " ", 1, &p->pos))
			return(0);
	}

	return(mlg_nstring(p, start, buf, sz));
}


int
mlg_line(struct md_mlg *p, char *buf)
{

	return(roff_engine(p->tree, buf));
}


int
mlg_exit(struct md_mlg *p, int flush)
{
	int		 c;

	c = roff_free(p->tree, flush);
	(*p->cbs.ml_free)(p->data);

	free(p);

	return(c);
}


struct md_mlg *
mlg_alloc(const struct md_args *args, 
		const struct md_rbuf *rbuf,
		struct md_mbuf *mbuf, 
		const struct ml_cbs *cbs)
{
	struct roffcb	 cb;
	struct md_mlg	*p;

	cb.roffhead = mlg_roffhead;
	cb.rofftail = mlg_rofftail;
	cb.roffin = mlg_roffin;
	cb.roffout = mlg_roffout;
	cb.roffblkin = mlg_roffblkin;
	cb.roffblkheadin = mlg_roffblkheadin;
	cb.roffblkheadout = mlg_roffblkheadout;
	cb.roffblkbodyin = mlg_roffblkbodyin;
	cb.roffblkbodyout = mlg_roffblkbodyout;
	cb.roffblkout = mlg_roffblkout;
	cb.roffspecial = mlg_roffspecial;
	cb.roffmsg = mlg_roffmsg;
	cb.roffdata = mlg_roffdata;

	if (NULL == (p = calloc(1, sizeof(struct md_mlg))))
		err(1, "calloc");

	p->args = args;
	p->mbuf = mbuf;
	p->rbuf = rbuf;

	(void)memcpy(&p->cbs, cbs, sizeof(struct ml_cbs));

	if (NULL == (p->tree = roff_alloc(&cb, p))) 
		free(p);
	else if ( ! (*p->cbs.ml_alloc)(&p->data))
		free(p);
	else
		return(p);

	return(NULL);
}


static int
mlg_roffhead(void *arg, const struct tm *tm, const char *os, 
		const char *title, enum roffmsec sec, enum roffvol vol)
{
	struct md_mlg	*p;

	assert(arg);
	p = (struct md_mlg *)arg;

	mlg_mode(p, MD_BLK_IN);

	if ( ! (*p->cbs.ml_begin)(p->mbuf, p->args, 
				tm, os, title, sec, vol))
		return(0);

	p->indent++;
	return(mlg_newline(p));
}


static int
mlg_rofftail(void *arg, const struct tm *tm, const char *os, 
		const char *title, enum roffmsec sec, enum roffvol vol)
{
	struct md_mlg	*p;

	assert(arg);
	p = (struct md_mlg *)arg;

	if (0 != p->pos)
		if ( ! mlg_newline(p))
			return(0);

	if ( ! (*p->cbs.ml_end)(p->mbuf, p->args, 
				tm, os, title, sec, vol))
		return(0);

	mlg_mode(p, MD_BLK_OUT);
	return(mlg_newline(p));
}


static int
mlg_literal_special(struct md_mlg *p, int tok, const char *start,
		const int *argc, const char **argv, const char **more)
{
	char		 *lit;

	if ( ! mlg_begintag(p, MD_NS_INLINE, tok, NULL, more))
		return(0);

	lit = roff_literal(tok, argc, argv, more);
	assert(lit);

	if ( ! mlg_string(p, start, lit))
		return(0);

	while (*more) { 
		if ( ! ml_nputs(p->mbuf, " ", 1, &p->pos))
			return(0);
		if ( ! mlg_string(p, start, *more++))
			return(0);
	}

	return(mlg_endtag(p, MD_NS_INLINE, tok));
}


static int
mlg_ref_special(struct md_mlg *p, int tok,
		const char *start, const char **more)
{

	if ( ! mlg_begintag(p, MD_NS_INLINE, tok, NULL, more))
		return(0);

	assert(*more);
	if ( ! ml_puts(p->mbuf, *more++, &p->pos))
		return(0);

	if (*more) {
		if ( ! ml_nputs(p->mbuf, "(", 1, &p->pos))
			return(0);
		if ( ! mlg_string(p, start, *more++))
			return(0);
		if ( ! ml_nputs(p->mbuf, ")", 1, &p->pos))
			return(0);
	}

	assert(NULL == *more);
	return(mlg_endtag(p, MD_NS_INLINE, tok));
}


/* ARGSUSED */
static int
mlg_formatted_special(struct md_mlg *p, int tok, const char *start,
		const int *argc, const char **argv, const char **more)
{
	char		 buf[256], *lit;

	if ( ! mlg_begintag(p, MD_NS_INLINE, tok, NULL, more))
		return(0);

	lit = roff_fmtstring(tok);

	assert(lit);
	assert(*more);
	(void)snprintf(buf, sizeof(buf), lit, *more++);
	assert(NULL == *more);

	if ( ! mlg_string(p, start, buf))
		return(0);

	return(mlg_endtag(p, MD_NS_INLINE, tok));
}


static int
mlg_atom_special(struct md_mlg *p, int tok,
		const char *start, const char **more)
{

	if ( ! mlg_begintag(p, MD_NS_INLINE, tok, NULL, more))
		return(0);

	assert(*more);
	if ( ! mlg_string(p, start, *more++))
		return(0);

	/*assert(NULL == *more);*/ /* FIXME: ROFF_Sx */
	return(mlg_endtag(p, MD_NS_INLINE, tok));
}


static int
mlg_function_special(struct md_mlg *p, 
		const char *start, const char **more)
{

	assert(*more);

	if ( ! mlg_begintag(p, MD_NS_INLINE, ROFF_Fn, NULL, more))
		return(0);
	if ( ! mlg_string(p, start, *more++))
		return(0);
	if ( ! mlg_endtag(p, MD_NS_INLINE, ROFF_Fn))
		return(0);

	if (NULL == *more)
		return(1);

	if ( ! ml_nputs(p->mbuf, "(", 1, &p->pos))
		return(0);

	p->flags |= ML_OVERRIDE_ONE;

	if ( ! mlg_begintag(p, MD_NS_INLINE, ROFF_Fa, NULL, more))
		return(0);
	if ( ! mlg_string(p, start, *more++))
		return(0);
	if ( ! mlg_endtag(p, MD_NS_INLINE, ROFF_Fa))
		return(0);

	while (*more) {
		if ( ! ml_nputs(p->mbuf, ", ", 2, &p->pos))
			return(0);
		if ( ! mlg_begintag(p, MD_NS_INLINE, ROFF_Fa, NULL, more))
			return(0);
		if ( ! mlg_string(p, start, *more++))
			return(0);
		if ( ! mlg_endtag(p, MD_NS_INLINE, ROFF_Fa))
			return(0);
	}

	return(ml_nputs(p->mbuf, ")", 1, &p->pos));
}


/* ARGSUSED */
static int
mlg_roffspecial(void *arg, int tok, const char *start, 
		const int *argc, const char **argv, const char **more)
{
	struct md_mlg	*p;

	assert(arg);
	p = (struct md_mlg *)arg;

	switch (tok) {
	case (ROFF_Ns):
		p->flags |= ML_OVERRIDE_ONE;
		return(1);

	case (ROFF_Sm):
		assert(*more);
		if (0 == strcmp(*more, "on"))
			p->flags |= ML_OVERRIDE_ALL;
		else
			p->flags &= ~ML_OVERRIDE_ALL;
		return(1);

	case (ROFF_Fn):
		return(mlg_function_special(p, start, more));

	case (ROFF_Xr):
		return(mlg_ref_special(p, tok, start, more));
 
	case (ROFF_Sx): /* FIXME */
		/* FALLTHROUGH */
	case (ROFF_Nm):
		return(mlg_atom_special(p, tok, start, more));
	
	case (ROFF_In):
		/* NOTREACHED */
	case (ROFF_Ex):
		/* NOTREACHED */
	case (ROFF_Rv):
		return(mlg_formatted_special(p, tok, start,
					argc, argv, more));

	case (ROFF_At):
		/* FALLTHROUGH */
	case (ROFF_Bt):
		/* FALLTHROUGH */
	case (ROFF_Ud):
		/* FALLTHROUGH */
	case (ROFF_Ux):
		/* FALLTHROUGH */
	case (ROFF_Bx):
		/* FALLTHROUGH */
	case (ROFF_Bsx):
		/* FALLTHROUGH */
	case (ROFF_Fx):
		/* FALLTHROUGH */
	case (ROFF_Nx):
		/* FALLTHROUGH */
	case (ROFF_St):
		/* FALLTHROUGH */
	case (ROFF_Ox):
		return(mlg_literal_special(p, tok, start, 
					argc, argv, more));
	default:
		break;
	}

	return(mlg_err(p, start, start, "`%s' not yet supported", 
				toknames[tok]));
}


static int
mlg_roffblkin(void *arg, int tok, 
		int *argc, const char **argv)
{

	return(mlg_begintag((struct md_mlg *)arg, 
				MD_NS_BLOCK, tok, argc, argv));
}


static int
mlg_roffblkout(void *arg, int tok)
{

	return(mlg_endtag((struct md_mlg *)arg, MD_NS_BLOCK, tok));
}


static int
mlg_roffblkbodyin(void *arg, int tok, 
		int *argc, const char **argv)
{

	return(mlg_begintag((struct md_mlg *)arg, 
				MD_NS_BODY, tok, argc, argv));
}


static int
mlg_roffblkbodyout(void *arg, int tok)
{

	return(mlg_endtag((struct md_mlg *)arg, MD_NS_BODY, tok));
}


static int
mlg_roffblkheadin(void *arg, int tok, 
		int *argc, const char **argv)
{

	return(mlg_begintag((struct md_mlg *)arg, 
				MD_NS_HEAD, tok, argc, argv));
}


static int
mlg_roffblkheadout(void *arg, int tok)
{

	return(mlg_endtag((struct md_mlg *)arg, MD_NS_HEAD, tok));
}


static int
mlg_roffin(void *arg, int tok, int *argc, const char **argv)
{

	return(mlg_begintag((struct md_mlg *)arg, 
				MD_NS_INLINE, tok, argc, argv));
}


static int
mlg_roffout(void *arg, int tok)
{

	return(mlg_endtag((struct md_mlg *)arg, MD_NS_INLINE, tok));
}


static int
mlg_roffmsg(void *arg, enum roffmsg lvl, const char *buf, 
		const char *pos, const char *msg)
{

	return(mlg_msg((struct md_mlg *)arg, lvl, buf, pos, msg));
}


static int
mlg_roffdata(void *arg, int space, 
		const char *start, const char *buf)
{
	struct md_mlg	*p;

	assert(arg);
	p = (struct md_mlg *)arg;

	if ( ! mlg_data(p, space, start, buf))
		return(0);

	mlg_mode(p, MD_TEXT);
	return(1);
}


static int
mlg_vmsg(struct md_mlg *p, enum roffmsg lvl, const char *start, 
		const char *pos, const char *fmt, va_list ap)
{
	char		 buf[128];

	(void)vsnprintf(buf, sizeof(buf), fmt, ap);
	return(mlg_msg(p, lvl, start, pos, buf));
}


static int
mlg_err(struct md_mlg *p, const char *start, 
		const char *pos, const char *fmt, ...)
{
	va_list		 ap;
	int		 c;

	va_start(ap, fmt);
	c = mlg_vmsg(p, ROFF_ERROR, start, pos, fmt, ap);
	va_end(ap);
	return(c);
}


static int
mlg_msg(struct md_mlg *p, enum roffmsg lvl, 
		const char *buf, const char *pos, const char *msg)
{
	char		*level;
	char		 b[256];
	int		 i;

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
					"%s:%zu: %s: %s (col %zu)", 
					p->rbuf->name, p->rbuf->line, 
					level, msg, pos - buf);
	} else 
		(void)snprintf(b, sizeof(b), "%s: %s: %s", 
				p->rbuf->name, level, msg);

	(void)fprintf(stderr, "%s\n", b);
	return(lvl == ROFF_WARN ? 1 : 0);
}

