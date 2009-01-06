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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef __linux__
#include <time.h>
#endif

#include "private.h"

#ifdef __linux__
extern	char		*strptime(const char *, const char *, struct tm *);
#endif

int
mdoc_iscdelim(char p)
{

	switch (p) {
	case('{'):
		/* FALLTHROUGH */
	case('.'):
		/* FALLTHROUGH */
	case(','):
		/* FALLTHROUGH */
	case(';'):
		/* FALLTHROUGH */
	case(':'):
		/* FALLTHROUGH */
	case('?'):
		/* FALLTHROUGH */
	case('!'):
		/* FALLTHROUGH */
	case('('):
		/* FALLTHROUGH */
	case(')'):
		/* FALLTHROUGH */
	case('['):
		/* FALLTHROUGH */
	case(']'):
		/* FALLTHROUGH */
	case('}'):
		return(1);
	default:
		break;
	}

	return(0);
}


int
mdoc_isdelim(const char *p)
{

	if (0 == *p)
		return(0);
	if (0 != *(p + 1))
		return(0);
	return(mdoc_iscdelim(*p));
}


enum mdoc_sec 
mdoc_atosec(size_t sz, const char **p)
{

	assert(sz > 0);
	if (sz > 2)
		return(SEC_CUSTOM);
	if (sz == 2) {
		if (0 == strcmp(*p, "RETURN") &&
				0 == strcmp(*(p + 1), "VALUES"))
			return(SEC_RETURN_VALUES);
		if (0 == strcmp(*p, "SEE") &&
				0 == strcmp(*(p + 1), "ALSO"))
			return(SEC_SEE_ALSO);
		return(SEC_CUSTOM);
	}

	if (0 == strcmp(*p, "NAME"))
		return(SEC_NAME);
	else if (0 == strcmp(*p, "SYNOPSIS"))
		return(SEC_SYNOPSIS);
	else if (0 == strcmp(*p, "DESCRIPTION"))
		return(SEC_DESCRIPTION);
	else if (0 == strcmp(*p, "ENVIRONMENT"))
		return(SEC_ENVIRONMENT);
	else if (0 == strcmp(*p, "FILES"))
		return(SEC_FILES);
	else if (0 == strcmp(*p, "EXAMPLES"))
		return(SEC_EXAMPLES);
	else if (0 == strcmp(*p, "DIAGNOSTICS"))
		return(SEC_DIAGNOSTICS);
	else if (0 == strcmp(*p, "ERRORS"))
		return(SEC_ERRORS);
	else if (0 == strcmp(*p, "STANDARDS"))
		return(SEC_STANDARDS);
	else if (0 == strcmp(*p, "HISTORY"))
		return(SEC_HISTORY);
	else if (0 == strcmp(*p, "AUTHORS"))
		return(SEC_AUTHORS);
	else if (0 == strcmp(*p, "CAVEATS"))
		return(SEC_CAVEATS);
	else if (0 == strcmp(*p, "BUGS"))
		return(SEC_BUGS);

	return(SEC_CUSTOM);
}


time_t
mdoc_atotime(const char *p)
{
	struct tm	 tm;

	(void)memset(&tm, 0, sizeof(struct tm));

	if (strptime(p, "%b %d %Y", &tm))
		return(mktime(&tm));
	if (strptime(p, "%b %d, %Y", &tm))
		return(mktime(&tm));

	return(0);
}


enum mdoc_msec
mdoc_atomsec(const char *p)
{

	if (0 == strcmp(p, "1"))
		return(MSEC_1);
	else if (0 == strcmp(p, "2"))
		return(MSEC_2);
	else if (0 == strcmp(p, "3"))
		return(MSEC_3);
	else if (0 == strcmp(p, "3f"))
		return(MSEC_3f);
	else if (0 == strcmp(p, "3p"))
		return(MSEC_3p);
	else if (0 == strcmp(p, "4"))
		return(MSEC_4);
	else if (0 == strcmp(p, "5"))
		return(MSEC_5);
	else if (0 == strcmp(p, "6"))
		return(MSEC_6);
	else if (0 == strcmp(p, "7"))
		return(MSEC_7);
	else if (0 == strcmp(p, "8"))
		return(MSEC_8);
	else if (0 == strcmp(p, "9"))
		return(MSEC_9);
	else if (0 == strcmp(p, "X11"))
		return(MSEC_X11);
	else if (0 == strcmp(p, "X11R6"))
		return(MSEC_X11R6);
	else if (0 == strcmp(p, "local"))
		return(MSEC_local);
	else if (0 == strcmp(p, "n"))
		return(MSEC_n);
	else if (0 == strcmp(p, "unass"))
		return(MSEC_unass);
	else if (0 == strcmp(p, "draft"))
		return(MSEC_draft);
	else if (0 == strcmp(p, "paper"))
		return(MSEC_paper);

	return(MSEC_DEFAULT);
}


enum mdoc_vol
mdoc_atovol(const char *p)
{

	if (0 == strcmp(p, "AMD"))
		return(VOL_AMD);
	else if (0 == strcmp(p, "IND"))
		return(VOL_IND);
	else if (0 == strcmp(p, "KM"))
		return(VOL_KM);
	else if (0 == strcmp(p, "LOCAL"))
		return(VOL_LOCAL);
	else if (0 == strcmp(p, "PRM"))
		return(VOL_PRM);
	else if (0 == strcmp(p, "PS1"))
		return(VOL_PS1);
	else if (0 == strcmp(p, "SMM"))
		return(VOL_SMM);
	else if (0 == strcmp(p, "URM"))
		return(VOL_URM);
	else if (0 == strcmp(p, "USD"))
		return(VOL_USD);

	return(VOL_DEFAULT);
}


enum mdoc_arch
mdoc_atoarch(const char *p)
{

	if (0 == strcmp(p, "alpha"))
		return(ARCH_alpha);
	else if (0 == strcmp(p, "amd64"))
		return(ARCH_amd64);
	else if (0 == strcmp(p, "amiga"))
		return(ARCH_amiga);
	else if (0 == strcmp(p, "arc"))
		return(ARCH_arc);
	else if (0 == strcmp(p, "armish"))
		return(ARCH_armish);
	else if (0 == strcmp(p, "aviion"))
		return(ARCH_aviion);
	else if (0 == strcmp(p, "hp300"))
		return(ARCH_hp300);
	else if (0 == strcmp(p, "hppa"))
		return(ARCH_hppa);
	else if (0 == strcmp(p, "hppa64"))
		return(ARCH_hppa64);
	else if (0 == strcmp(p, "i386"))
		return(ARCH_i386);
	else if (0 == strcmp(p, "landisk"))
		return(ARCH_landisk);
	else if (0 == strcmp(p, "luna88k"))
		return(ARCH_luna88k);
	else if (0 == strcmp(p, "mac68k"))
		return(ARCH_mac68k);
	else if (0 == strcmp(p, "macppc"))
		return(ARCH_macppc);
	else if (0 == strcmp(p, "mvme68k"))
		return(ARCH_mvme68k);
	else if (0 == strcmp(p, "mvme88k"))
		return(ARCH_mvme88k);
	else if (0 == strcmp(p, "mvmeppc"))
		return(ARCH_mvmeppc);
	else if (0 == strcmp(p, "pmax"))
		return(ARCH_pmax);
	else if (0 == strcmp(p, "sgi"))
		return(ARCH_sgi);
	else if (0 == strcmp(p, "socppc"))
		return(ARCH_socppc);
	else if (0 == strcmp(p, "sparc"))
		return(ARCH_sparc);
	else if (0 == strcmp(p, "sparc64"))
		return(ARCH_sparc64);
	else if (0 == strcmp(p, "sun3"))
		return(ARCH_sun3);
	else if (0 == strcmp(p, "vax"))
		return(ARCH_vax);
	else if (0 == strcmp(p, "zaurus"))
		return(ARCH_zaurus);

	return(ARCH_DEFAULT);
}


enum mdoc_att
mdoc_atoatt(const char *p)
{

	assert(p);
	if (0 == strcmp(p, "v1"))
		return(ATT_v1);
	else if (0 == strcmp(p, "v2"))
		return(ATT_v2);
	else if (0 == strcmp(p, "v3"))
		return(ATT_v3);
	else if (0 == strcmp(p, "v4"))
		return(ATT_v4);
	else if (0 == strcmp(p, "v5"))
		return(ATT_v5);
	else if (0 == strcmp(p, "v6"))
		return(ATT_v6);
	else if (0 == strcmp(p, "v7"))
		return(ATT_v7);
	else if (0 == strcmp(p, "32v"))
		return(ATT_32v);
	else if (0 == strcmp(p, "V.1"))
		return(ATT_V1);
	else if (0 == strcmp(p, "V.2"))
		return(ATT_V2);
	else if (0 == strcmp(p, "V.3"))
		return(ATT_V3);
	else if (0 == strcmp(p, "V.4"))
		return(ATT_V4);
	
	return(ATT_DEFAULT);
}
