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

#include "roff.h"

static int
macro_args_next(struct rofftree *tree, int *pos, char *buf, char **v)
{
	int		 i;

	if (0 == buf[*pos])
		return(0);

	if ('\"' == buf[*pos]) {
		/* Syntax error: quotation marks not allowed. */
		return(-1);
	}

	*v = &buf[*pos];

	while (buf[*pos] && ! isspace(buf[*pos]))
		(*pos)++;

	if (buf[*pos + 1] && '\\' == buf[*pos]) {
		/* Syntax error: escaped whitespace not allowed. */
		return(-1);
	}

	buf[i] = 0;
	return(1);
}

/*
 * Parses the following:
 *
 *     .Xx foo bar baz ; foo "bar baz" ; ;
 *         ^----------   ^----------
 */
static int
macro_fl(struct rofftree *tree, int tok, int *pos, char *buf)
{
	int		  i, j, c, first; 
	char		 *args[ROFF_MAXLINEARG];

	first = *pos == 0;

	for (j = 0; ; ) {
		i = *pos;
		c = macro_args_next(tree, *i, buf, args[j]);
		if (-1 == c) 
			return(0);
		if (0 == c) 
			break;

		/* Break at the next command.  */

		if (ROFF_MAX != (c = rofffindcallable(args[pos]))) {
			if ( ! macro(tree, tok, argc, argv, i, p))
				return(0);
			if ( ! parse(tree, c, pos, args))
				return(0);
			break;
		}

		/* Continue if we're just words. */

		if ( ! roffispunct(args[pos])) {
			i++;
			continue;
		}

		/* Break if there's only remaining punctuation. */

		if (args[pos + 1] && roffispunct(args[pos + 1]))
			break;

		/* If there are remaining words, start anew. */

		if ( ! macro(tree, tok, argc, argv, i, p))
			return(0);

		/* Spit out the punctuation. */

		if ( ! word(tree, tok, *args++))
			return(0);
		i++;
	}
}
