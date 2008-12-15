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


void
mdoc_hash_free(void *htab)
{

	free(htab);
}


void *
mdoc_hash_alloc(void)
{
	int		  i, major, minor, index;
	const void	**htab;

	htab = calloc(27 * 26, sizeof(struct mdoc_macro *));
	if (NULL == htab) 
		err(1, "calloc");

	for (i = 1; i < MDOC_MAX; i++) {
		major = mdoc_macronames[i][0];
		assert((major >= 65 && major <= 90) ||
				major == 37);

		if (major == 37) 
			major = 0;
		else
			major -= 64;

		minor = mdoc_macronames[i][1];
		assert((minor >= 65 && minor <= 90) ||
				(minor == 49) ||
				(minor >= 97 && minor <= 122));

		if (minor == 49)
			minor = 0;
		else if (minor <= 90)
			minor -= 65;
		else 
			minor -= 97;

		assert(major >= 0 && major < 27);
		assert(minor >= 0 && minor < 26);

		index = (major * 27) + minor;

		assert(NULL == htab[index]);
		htab[index] = &mdoc_macros[i];
	}

	return((void *)htab);
}


int
mdoc_hash_find(const void *arg, const char *tmp)
{
	int		  major, minor, index, slot;
	const void	**htab;

	htab = (const void **)arg;

	if (0 == tmp[0] || 0 == tmp[1])
		return(MDOC_MAX);

	if ( ! (tmp[0] == 37 || (tmp[0] >= 65 && tmp[0] <= 90)))
		return(MDOC_MAX);

	if ( ! ((tmp[1] >= 65 && tmp[1] <= 90) ||
				(tmp[1] == 49) ||
				(tmp[1] >= 97 && tmp[1] <= 122)))
		return(MDOC_MAX);

	if (tmp[0] == 37)
		major = 0;
	else
		major = tmp[0] - 64;

	if (tmp[1] == 49)
		minor = 0;
	else if (tmp[1] <= 90)
		minor = tmp[1] - 65;
	else
		minor = tmp[1] - 97;

	index = (major * 27) + minor;

	if (NULL == htab[index])
		return(MDOC_MAX);

	slot = htab[index] - (void *)mdoc_macros;
	assert(0 == slot % sizeof(struct mdoc_macro));
	slot /= sizeof(struct mdoc_macro);

	if (0 != strcmp(mdoc_macronames[slot], tmp))
		return(MDOC_MAX);
	return(slot);
}

