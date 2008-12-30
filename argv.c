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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "private.h"


static	int		 lookup(int, const char *);
static	int		 parse(struct mdoc *, int, 
				struct mdoc_arg *, int *, char *);
static	int		 postparse(struct mdoc *, int, 
				const struct mdoc_arg *, int);


int
mdoc_args(struct mdoc *mdoc, int tok, int *pos, char *buf, int fl, char **v)
{
	int		 i;

	if (0 == buf[*pos])
		return(ARGS_EOLN);

	if ('\"' == buf[*pos] && ! (fl & ARGS_QUOTED))
		if ( ! mdoc_warn(mdoc, tok, *pos, WARN_SYNTAX_QUOTED))
			return(ARGS_ERROR);

	if ('-' == buf[*pos]) 
		if ( ! mdoc_warn(mdoc, tok, *pos, WARN_SYNTAX_ARGLIKE))
			return(ARGS_ERROR);

	if ((fl & ARGS_DELIM) && mdoc_iscdelim(buf[*pos])) {
		for (i = *pos; buf[i]; ) {
			if ( ! mdoc_iscdelim(buf[i]))
				break;
			i++;
			if (0 == buf[i] || ! isspace(buf[i]))
				break;
			i++;
			while (buf[i] && isspace(buf[i]))
				i++;
		}
		if (0 == buf[i]) {
			*v = &buf[*pos];
			return(ARGS_PUNCT);
		}
	}

	/*
	 * Parse routine for non-quoted string.  
	 */

	if ('\"' != buf[*pos]) {
		*v = &buf[*pos];

		while (buf[*pos] && ! isspace(buf[*pos]))
			(*pos)++;

		if (0 == buf[*pos])
			return(ARGS_WORD);

		buf[(*pos)++] = 0;
		if (0 == buf[*pos])
			return(ARGS_WORD);

		while (buf[*pos] && isspace(buf[*pos]))
			(*pos)++;

		if (buf[*pos])
			return(ARGS_WORD);

		if ( ! mdoc_warn(mdoc, tok, *pos, WARN_SYNTAX_WS_EOLN))
			return(ARGS_ERROR);

		return(ARGS_WORD);
	}

	/*
	 * If we're a quoted string (and quoted strings are allowed),
	 * then parse ahead to the next quote.  If none's found, it's an
	 * error.  After, parse to the next word.  We're not allowed to
	 * also be DELIM requests (for now).
	 */
	assert( ! (fl & ARGS_DELIM));

	*v = &buf[++(*pos)];

	while (buf[*pos] && '\"' != buf[*pos])
		(*pos)++;

	if (0 == buf[*pos]) {
		(void)mdoc_err(mdoc, tok, *pos, ERR_SYNTAX_UNQUOTE);
		return(ARGS_ERROR);
	}

	buf[(*pos)++] = 0;
	if (0 == buf[*pos])
		return(ARGS_WORD);

	while (buf[*pos] && isspace(buf[*pos]))
		(*pos)++;

	if (buf[*pos])
		return(ARGS_WORD);

	if ( ! mdoc_warn(mdoc, tok, *pos, WARN_SYNTAX_WS_EOLN))
		return(ARGS_ERROR);

	return(ARGS_WORD);
}


static int
lookup(int tok, const char *argv)
{

	switch (tok) {
	case (MDOC_Bd):
		if (xstrcmp(argv, "ragged"))
			return(MDOC_Ragged);
		else if (xstrcmp(argv, "unfilled"))
			return(MDOC_Unfilled);
		else if (xstrcmp(argv, "literal"))
			return(MDOC_Literal);
		else if (xstrcmp(argv, "file"))
			return(MDOC_File);
		else if (xstrcmp(argv, "offset"))
			return(MDOC_Offset);
		break;

	case (MDOC_Bl):
		if (xstrcmp(argv, "bullet"))
			return(MDOC_Bullet);
		else if (xstrcmp(argv, "dash"))
			return(MDOC_Dash);
		else if (xstrcmp(argv, "hyphen"))
			return(MDOC_Hyphen);
		else if (xstrcmp(argv, "item"))
			return(MDOC_Item);
		else if (xstrcmp(argv, "enum"))
			return(MDOC_Enum);
		else if (xstrcmp(argv, "tag"))
			return(MDOC_Tag);
		else if (xstrcmp(argv, "diag"))
			return(MDOC_Diag);
		else if (xstrcmp(argv, "hang"))
			return(MDOC_Hang);
		else if (xstrcmp(argv, "ohang"))
			return(MDOC_Ohang);
		else if (xstrcmp(argv, "inset"))
			return(MDOC_Inset);
		else if (xstrcmp(argv, "column"))
			return(MDOC_Column);
		else if (xstrcmp(argv, "width"))
			return(MDOC_Width);
		else if (xstrcmp(argv, "offset"))
			return(MDOC_Offset);
		else if (xstrcmp(argv, "compact"))
			return(MDOC_Compact);
		break;
	
	case (MDOC_Rv):
		/* FALLTHROUGH */
	case (MDOC_Ex):
		if (xstrcmp(argv, "std"))
			return(MDOC_Std);
		break;

	default:
		abort();
		/* NOTREACHED */
	}

	return(MDOC_ARG_MAX);
}


static int
postparse(struct mdoc *mdoc, int tok, const struct mdoc_arg *v, int pos)
{

	switch (v->arg) {
	case (MDOC_Offset):
		assert(v->value);
		assert(v->value[0]);
		if (xstrcmp(v->value[0], "left"))
			break;
		if (xstrcmp(v->value[0], "right"))
			break;
		if (xstrcmp(v->value[0], "center"))
			break;
		if (xstrcmp(v->value[0], "indent"))
			break;
		if (xstrcmp(v->value[0], "indent-two"))
			break;
		return(mdoc_err(mdoc, tok, pos, ERR_SYNTAX_ARGBAD));
	default:
		break;
	}

	return(1);
}


static int
parse(struct mdoc *mdoc, int tok, 
		struct mdoc_arg *v, int *pos, char *buf)
{
	char		*p;
	int		 c, ppos, i;

	ppos = *pos;

	switch (v->arg) {
	case(MDOC_Compact):
		/* FALLTHROUGH */
	case(MDOC_Ragged):
		/* FALLTHROUGH */
	case(MDOC_Unfilled):
		/* FALLTHROUGH */
	case(MDOC_Literal):
		/* FALLTHROUGH */
	case(MDOC_File):
		/* FALLTHROUGH */
	case(MDOC_Bullet):
		/* FALLTHROUGH */
	case(MDOC_Dash):
		/* FALLTHROUGH */
	case(MDOC_Hyphen):
		/* FALLTHROUGH */
	case(MDOC_Item):
		/* FALLTHROUGH */
	case(MDOC_Enum):
		/* FALLTHROUGH */
	case(MDOC_Tag):
		/* FALLTHROUGH */
	case(MDOC_Diag):
		/* FALLTHROUGH */
	case(MDOC_Hang):
		/* FALLTHROUGH */
	case(MDOC_Ohang):
		/* FALLTHROUGH */
	case(MDOC_Inset):
		v->sz = 0;
		v->value = NULL;
		break;

	case(MDOC_Std):
		/* FALLTHROUGH */
	case(MDOC_Width):
		/* FALLTHROUGH */
	case(MDOC_Offset):
		/*
		 * This has a single value for an argument.
		 */
		c = mdoc_args(mdoc, tok, pos, buf, ARGS_QUOTED, &p);
		if (ARGS_ERROR == c)
			return(0);
		else if (ARGS_EOLN == c)
			return(mdoc_err(mdoc, tok, ppos, ERR_SYNTAX_ARGVAL));
			
		v->sz = 1;
		v->value = xcalloc(1, sizeof(char *));
		v->value[0] = p;
		break;

	case(MDOC_Column):
		/*
		 * This has several value for a single argument.  We
		 * pre-allocate a pointer array and don't let it exceed
		 * this size.
		 */
		v->sz = 0;
		v->value = xcalloc(MDOC_LINEARG_MAX, sizeof(char *));
		for (i = 0; i < MDOC_LINEARG_MAX; i++) {
			c = mdoc_args(mdoc, tok, pos, buf, ARGS_QUOTED, &p);
			if (ARGS_ERROR == c) {
				free(v->value);
				return(0);
			} else if (ARGS_EOLN == c)
				break;
			v->value[i] = p;
		}
		if (0 == i) {
			free(v->value);
			return(mdoc_err(mdoc, tok, ppos, ERR_SYNTAX_ARGVAL));
		} else if (MDOC_LINEARG_MAX == i)
			return(mdoc_err(mdoc, tok, ppos, ERR_SYNTAX_ARGMANY));

		v->sz = i;
		break;
	default:
		abort();
		/* NOTREACHED */
	}

	return(1);
}


int
mdoc_argv(struct mdoc *mdoc, int tok, 
		struct mdoc_arg *v, int *pos, char *buf)
{
	int		 i, ppos;
	char		*argv;

	(void)memset(v, 0, sizeof(struct mdoc_arg));

	if (0 == buf[*pos])
		return(0);

	assert( ! isspace(buf[*pos]));

	if ('-' != buf[*pos]) {
		(void)mdoc_err(mdoc, tok, *pos, ERR_SYNTAX_ARGFORM);
		return(-1);
	}

	i = *pos;
	argv = &buf[++(*pos)];

	while (buf[*pos] && ! isspace(buf[*pos]))
		(*pos)++;

	if (buf[*pos])
		buf[(*pos)++] = 0;

	if (MDOC_ARG_MAX == (v->arg = lookup(tok, argv))) {
		(void)mdoc_err(mdoc, tok, i, ERR_SYNTAX_ARG);
		return(-1);
	}

	while (buf[*pos] && isspace(buf[*pos]))
		(*pos)++;

	/* FIXME: whitespace if no value. */

	ppos = *pos;
	if ( ! parse(mdoc, tok, v, pos, buf))
		return(-1);
	if ( ! postparse(mdoc, tok, v, ppos))
		return(-1);

	return(1);
}


void
mdoc_argv_free(int sz, struct mdoc_arg *arg)
{
	int		 i;

	for (i = 0; i < sz; i++) {
		if (0 == arg[i].sz) {
			assert(NULL == arg[i].value);
			continue;
		}
		assert(arg[i].value);
		free(arg[i].value);
	}
}

