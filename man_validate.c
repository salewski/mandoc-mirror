/*	$Id$ */
/*
 * Copyright (c) 2008, 2009 Kristaps Dzonsons <kristaps@kth.se>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <sys/types.h>

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>

#include "libman.h"
#include "libmandoc.h"

#define	CHKARGS	  struct man *m, const struct man_node *n

typedef	int	(*v_check)(CHKARGS);

struct	man_valid {
	v_check	 *pres;
	v_check	 *posts;
};

static	int	  check_bline(CHKARGS);
static	int	  check_eline(CHKARGS);
static	int	  check_eq0(CHKARGS);
static	int	  check_eq1(CHKARGS);
static	int	  check_ge2(CHKARGS);
static	int	  check_le5(CHKARGS);
static	int	  check_par(CHKARGS);
static	int	  check_root(CHKARGS);
static	int	  check_sec(CHKARGS);
static	int	  check_sp(CHKARGS);
static	int	  check_text(CHKARGS);

static	v_check	  posts_eq0[] = { check_eq0, NULL };
static	v_check	  posts_ge2_le5[] = { check_ge2, check_le5, NULL };
static	v_check	  posts_par[] = { check_par, NULL };
static	v_check	  posts_sec[] = { check_sec, NULL };
static	v_check	  posts_sp[] = { check_sp, NULL };
static	v_check	  pres_eline[] = { check_eline, NULL };
static	v_check	  pres_bline[] = { check_bline, NULL };

static	const struct man_valid man_valids[MAN_MAX] = {
	{ pres_bline, posts_eq0 }, /* br */
	{ pres_bline, posts_ge2_le5 }, /* TH */
	{ pres_bline, posts_sec }, /* SH */
	{ pres_bline, posts_sec }, /* SS */
	{ pres_bline, posts_par }, /* TP */
	{ pres_bline, posts_par }, /* LP */
	{ pres_bline, posts_par }, /* PP */
	{ pres_bline, posts_par }, /* P */
	{ pres_bline, posts_par }, /* IP */
	{ pres_bline, posts_par }, /* HP */
	{ pres_eline, NULL }, /* SM */
	{ pres_eline, NULL }, /* SB */
	{ NULL, NULL }, /* BI */
	{ NULL, NULL }, /* IB */
	{ NULL, NULL }, /* BR */
	{ NULL, NULL }, /* RB */
	{ pres_eline, NULL }, /* R */
	{ pres_eline, NULL }, /* B */
	{ pres_eline, NULL }, /* I */
	{ NULL, NULL }, /* IR */
	{ NULL, NULL }, /* RI */
	{ pres_bline, posts_eq0 }, /* na */
	{ NULL, NULL }, /* i */
	{ pres_bline, posts_sp }, /* sp */
	{ pres_bline, posts_eq0 }, /* nf */
	{ pres_bline, posts_eq0 }, /* fi */
	{ NULL, NULL }, /* r */
	{ NULL, NULL }, /* RE */
	{ NULL, NULL }, /* RS */
};


int
man_valid_pre(struct man *m, const struct man_node *n)
{
	v_check		*cp;

	if (MAN_TEXT == n->type)
		return(1);
	if (MAN_ROOT == n->type)
		return(1);

	if (NULL == (cp = man_valids[n->tok].pres))
		return(1);
	for ( ; *cp; cp++)
		if ( ! (*cp)(m, n)) 
			return(0);
	return(1);
}


int
man_valid_post(struct man *m)
{
	v_check		*cp;

	if (MAN_VALID & m->last->flags)
		return(1);
	m->last->flags |= MAN_VALID;

	switch (m->last->type) {
	case (MAN_TEXT): 
		return(check_text(m, m->last));
	case (MAN_ROOT):
		return(check_root(m, m->last));
	default:
		break;
	}

	if (NULL == (cp = man_valids[m->last->tok].posts))
		return(1);
	for ( ; *cp; cp++)
		if ( ! (*cp)(m, m->last))
			return(0);

	return(1);
}


static int
check_root(CHKARGS) 
{

	/* XXX - make this into a warning? */
	if (MAN_BLINE & m->flags)
		return(man_nerr(m, n, WEXITSCOPE));
	/* XXX - make this into a warning? */
	if (MAN_ELINE & m->flags)
		return(man_nerr(m, n, WEXITSCOPE));

	if (NULL == m->first->child)
		return(man_nerr(m, n, WNODATA));
	if (NULL == m->meta.title)
		return(man_nerr(m, n, WNOTITLE));

	return(1);
}


static int
check_text(CHKARGS) 
{
	const char	*p;
	int		 pos, c;

	assert(n->string);

	for (p = n->string, pos = n->pos + 1; *p; p++, pos++) {
		if ('\\' == *p) {
			c = mandoc_special(p);
			if (c) {
				p += c - 1;
				pos += c - 1;
				continue;
			}
			if ( ! (MAN_IGN_ESCAPE & m->pflags))
				return(man_perr(m, n->line, pos, WESCAPE));
			if ( ! man_pwarn(m, n->line, pos, WESCAPE))
				return(0);
			continue;
		}

		if ('\t' == *p || isprint((u_char)*p)) 
			continue;

		if (MAN_IGN_CHARS & m->pflags)
			return(man_pwarn(m, n->line, pos, WNPRINT));
		return(man_perr(m, n->line, pos, WNPRINT));
	}

	return(1);
}


#define	INEQ_DEFINE(x, ineq, name) \
static int \
check_##name(CHKARGS) \
{ \
	if (n->nchild ineq (x)) \
		return(1); \
	return(man_verr(m, n->line, n->pos, \
			"expected line arguments %s %d, have %d", \
			#ineq, (x), n->nchild)); \
}

INEQ_DEFINE(0, ==, eq0)
INEQ_DEFINE(1, ==, eq1)
INEQ_DEFINE(2, >=, ge2)
INEQ_DEFINE(5, <=, le5)


static int
check_sp(CHKARGS)
{
	long		 lval;
	char		*ep, *buf;

	if (NULL == n->child)
		return(1);
	else if ( ! check_eq1(m, n))
		return(0);

	assert(MAN_TEXT == n->child->type);
	buf = n->child->string;
	assert(buf);
	
	/* From OpenBSD's strtol(3). */

	errno = 0;
	lval = strtol(buf, &ep, 10);
	if (buf[0] == '\0' || *ep != '\0')
		return(man_nerr(m, n->child, WNUMFMT));

	if ((errno == ERANGE && (lval == LONG_MAX || lval == LONG_MIN)) ||
			(lval > INT_MAX || lval < 0))
		return(man_nerr(m, n->child, WNUMFMT));

	return(1);
}


static int
check_sec(CHKARGS)
{

	if (MAN_BODY == n->type && 0 == n->nchild)
		return(man_nwarn(m, n, WBODYARGS));
	if (MAN_HEAD == n->type && 0 == n->nchild)
		return(man_nerr(m, n, WHEADARGS));
	return(1);
}


static int
check_par(CHKARGS)
{

	if (MAN_BODY == n->type) 
		switch (n->tok) {
		case (MAN_IP):
			/* FALLTHROUGH */
		case (MAN_HP):
			/* FALLTHROUGH */
		case (MAN_TP):
			/* Body-less lists are ok. */
			break;
		default:
			if (n->nchild)
				break;
			return(man_nwarn(m, n, WBODYARGS));
		}
	if (MAN_HEAD == n->type)
		switch (n->tok) {
		case (MAN_PP):
			/* FALLTHROUGH */
		case (MAN_P):
			/* FALLTHROUGH */
		case (MAN_LP):
			if (0 == n->nchild)
				break;
			return(man_nwarn(m, n, WNHEADARGS));
		default:
			if (n->nchild)
				break;
			return(man_nwarn(m, n, WHEADARGS));
		}

	return(1);
}


static int
check_eline(CHKARGS)
{

	if (MAN_ELINE & m->flags)
		return(man_nerr(m, n, WLNSCOPE));
	return(1);
}


static int
check_bline(CHKARGS)
{

	if (MAN_BLINE & m->flags)
		return(man_nerr(m, n, WLNSCOPE));
	if (MAN_ELINE & m->flags)
		return(man_nerr(m, n, WLNSCOPE));
	return(1);
}

