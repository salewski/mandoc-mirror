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
#ifndef __OpenBSD__
#include <time.h>
#endif

/*
 * Convert scalars to and from string format.
 */

#include "private.h"

#ifdef __linux__
extern	char		*strptime(const char *, const char *, struct tm *);
#endif


size_t
mdoc_isescape(const char *p)
{
	size_t		 c;
	
	if ('\\' != *p++)
		return(0);

	switch (*p) {
	case ('\\'):
		/* FALLTHROUGH */
	case ('\''):
		/* FALLTHROUGH */
	case ('`'):
		/* FALLTHROUGH */
	case ('-'):
		/* FALLTHROUGH */
	case (' '):
		/* FALLTHROUGH */
	case ('&'):
		/* FALLTHROUGH */
	case ('.'):
		/* FALLTHROUGH */
	case ('e'):
		return(2);
	case ('*'):
		if (0 == *++p || ! isgraph((u_char)*p))
			return(0);
		switch (*p) {
		case ('('):
			if (0 == *++p || ! isgraph((u_char)*p))
				return(0);
			return(4);
		case ('['):
			for (c = 3, p++; *p && ']' != *p; p++, c++)
				if ( ! isgraph((u_char)*p))
					break;
			return(*p == ']' ? c : 0);
		default:
			break;
		}
		return(3);
	case ('('):
		if (0 == *++p || ! isgraph((u_char)*p))
			return(0);
		if (0 == *++p || ! isgraph((u_char)*p))
			return(0);
		return(4);
	case ('['):
		break;
	default:
		return(0);
	}

	for (c = 3, p++; *p && ']' != *p; p++, c++)
		if ( ! isgraph((u_char)*p))
			break;

	return(*p == ']' ? c : 0);
}


int
mdoc_iscdelim(char p)
{

	switch (p) {
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
	case('{'):
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
mdoc_atosec(const char *p)
{

	assert(p);
	if (0 == strcmp(p, "NAME"))
		return(SEC_NAME);
	else if (0 == strcmp(p, "RETURN VALUES"))
		return(SEC_RETURN_VALUES);
	else if (0 == strcmp(p, "SEE ALSO"))
		return(SEC_SEE_ALSO);
	else if (0 == strcmp(p, "SYNOPSIS"))
		return(SEC_SYNOPSIS);
	else if (0 == strcmp(p, "DESCRIPTION"))
		return(SEC_DESCRIPTION);
	else if (0 == strcmp(p, "ENVIRONMENT"))
		return(SEC_ENVIRONMENT);
	else if (0 == strcmp(p, "FILES"))
		return(SEC_FILES);
	else if (0 == strcmp(p, "EXAMPLES"))
		return(SEC_EXAMPLES);
	else if (0 == strcmp(p, "DIAGNOSTICS"))
		return(SEC_DIAGNOSTICS);
	else if (0 == strcmp(p, "ERRORS"))
		return(SEC_ERRORS);
	else if (0 == strcmp(p, "STANDARDS"))
		return(SEC_STANDARDS);
	else if (0 == strcmp(p, "HISTORY"))
		return(SEC_HISTORY);
	else if (0 == strcmp(p, "AUTHORS"))
		return(SEC_AUTHORS);
	else if (0 == strcmp(p, "CAVEATS"))
		return(SEC_CAVEATS);
	else if (0 == strcmp(p, "BUGS"))
		return(SEC_BUGS);

	return(SEC_CUSTOM);
}


time_t
mdoc_atotime(const char *p)
{
	struct tm	 tm;
	char		*pp;

	(void)memset(&tm, 0, sizeof(struct tm));

	if (xstrcmp(p, "$Mdocdate$"))
		return(time(NULL));
	if ((pp = strptime(p, "$Mdocdate$", &tm)) && 0 == *pp)
		return(mktime(&tm));
	/* XXX - this matches "June 1999", which is wrong. */
	if ((pp = strptime(p, "%b %d %Y", &tm)) && 0 == *pp)
		return(mktime(&tm));
	if ((pp = strptime(p, "%b %d, %Y", &tm)) && 0 == *pp)
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
	else if (0 == strcmp(p, "arm"))
		return(ARCH_arm);
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
	else if (0 == strcmp(p, "V"))
		return(ATT_V);
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


char *
mdoc_type2a(enum mdoc_type type)
{
	switch (type) {
	case (MDOC_ROOT):
		return("root");
	case (MDOC_BLOCK):
		return("block");
	case (MDOC_HEAD):
		return("block-head");
	case (MDOC_BODY):
		return("block-body");
	case (MDOC_TAIL):
		return("block-tail");
	case (MDOC_ELEM):
		return("elem");
	case (MDOC_TEXT):
		return("text");
	default:
		break;
	}

	abort();
	/* NOTREACHED */
}


const char *
mdoc_arch2a(enum mdoc_arch arch)
{

	switch (arch) {
	case (ARCH_alpha):
		return("Alpha");
	case (ARCH_amd64):
		return("AMD64");
	case (ARCH_amiga):
		return("Amiga");
	case (ARCH_arc):
		return("ARC");
	case (ARCH_arm):
		return("ARM");
	case (ARCH_armish):
		return("ARMISH");
	case (ARCH_aviion):
		return("AViiON");
	case (ARCH_hp300):
		return("HP300");
	case (ARCH_hppa):
		return("HPPA");
	case (ARCH_hppa64):
		return("HPPA64");
	case (ARCH_i386):
		return("i386");
	case (ARCH_landisk):
		return("LANDISK");
	case (ARCH_luna88k):
		return("Luna88k");
	case (ARCH_mac68k):
		return("Mac68k");
	case (ARCH_macppc):
		return("MacPPC");
	case (ARCH_mvme68k):
		return("MVME68k");
	case (ARCH_mvme88k):
		return("MVME88k");
	case (ARCH_mvmeppc):
		return("MVMEPPC");
	case (ARCH_pmax):
		return("PMAX");
	case (ARCH_sgi):
		return("SGI");
	case (ARCH_socppc):
		return("SOCPPC");
	case (ARCH_sparc):
		return("SPARC");
	case (ARCH_sparc64):
		return("SPARC64");
	case (ARCH_sun3):
		return("Sun3");
	case (ARCH_vax):
		return("VAX");
	case (ARCH_zaurus):
		return("Zaurus");
	case (ARCH_DEFAULT):
		return(NULL);
	default:
		break;
	}

	abort();
	/* NOTREACHED */
}


const char *
mdoc_vol2a(enum mdoc_vol vol)
{

	switch (vol) {
	case (VOL_AMD):
		return("Ancestral Manual Documents");
	case (VOL_IND):
		return("Manual Master Index");
	case (VOL_KM):
		return("Kernel Manual");
	case (VOL_LOCAL):
		return("Local Manual");
	case (VOL_PRM):
		return("Programmer's Manual");
	case (VOL_PS1):
		return("Programmer's Supplementary Documents");
	case (VOL_SMM):
		return("System Manager's Manual");
	case (VOL_URM):
		return("Reference Manual");
	case (VOL_USD):
		return("User's Supplementary Documents");
	case (VOL_DEFAULT):
		return(NULL);
	default:
		break;
	}

	abort();
	/* NOTREACHED */
}


const char *
mdoc_msec2a(enum mdoc_msec msec)
{

	switch (msec) {
	case(MSEC_1):
		return("1");
	case(MSEC_2):
		return("2");
	case(MSEC_3):
		return("3");
	case(MSEC_3f):
		return("3f");
	case(MSEC_3p):
		return("3p");
	case(MSEC_4):
		return("4");
	case(MSEC_5):
		return("5");
	case(MSEC_6):
		return("6");
	case(MSEC_7):
		return("7");
	case(MSEC_8):
		return("8");
	case(MSEC_9):
		return("9");
	case(MSEC_X11):
		return("X11");
	case(MSEC_X11R6):
		return("X11R6");
	case(MSEC_local):
		return("local");
	case(MSEC_n):
		return("n");
	case(MSEC_unass):
		/* FALLTHROUGH */
	case(MSEC_draft):
		return("draft");
	case(MSEC_paper):
		return("paper");
	case(MSEC_DEFAULT):
		return(NULL);
	default:
		break;
	}

	abort();
	/* NOTREACHED */
}


const char *
mdoc_st2a(int c)
{
	char		 *p;

	switch (c) {
	case(MDOC_p1003_1_88):
		p = "IEEE Std 1003.1-1988 (\\(lqPOSIX\\(rq)";
		break;
	case(MDOC_p1003_1_90):
		p = "IEEE Std 1003.1-1990 (\\(lqPOSIX\\(rq)";
		break;
	case(MDOC_p1003_1_96):
		p = "ISO/IEC 9945-1:1996 (\\(lqPOSIX\\(rq)";
		break;
	case(MDOC_p1003_1_2001):
		p = "IEEE Std 1003.1-2001 (\\(lqPOSIX\\(rq)";
		break;
	case(MDOC_p1003_1_2004):
		p = "IEEE Std 1003.1-2004 (\\(lqPOSIX\\(rq)";
		break;
	case(MDOC_p1003_1):
		p = "IEEE Std 1003.1 (\\(lqPOSIX\\(rq)";
		break;
	case(MDOC_p1003_1b):
		p = "IEEE Std 1003.1b (\\(lqPOSIX\\(rq)";
		break;
	case(MDOC_p1003_1b_93):
		p = "IEEE Std 1003.1b-1993 (\\(lqPOSIX\\(rq)";
		break;
	case(MDOC_p1003_1c_95):
		p = "IEEE Std 1003.1c-1995 (\\(lqPOSIX\\(rq)";
		break;
	case(MDOC_p1003_1g_2000):
		p = "IEEE Std 1003.1g-2000 (\\(lqPOSIX\\(rq)";
		break;
	case(MDOC_p1003_2_92):
		p = "IEEE Std 1003.2-1992 (\\(lqPOSIX.2\\(rq)";
		break;
	case(MDOC_p1387_2_95):
		p = "IEEE Std 1387.2-1995 (\\(lqPOSIX.7.2\\(rq)";
		break;
	case(MDOC_p1003_2):
		p = "IEEE Std 1003.2 (\\(lqPOSIX.2\\(rq)";
		break;
	case(MDOC_p1387_2):
		p = "IEEE Std 1387.2 (\\(lqPOSIX.7.2\\(rq)";
		break;
	case(MDOC_isoC_90):
		p = "ISO/IEC 9899:1990 (\\(lqISO C90\\(rq)";
		break;
	case(MDOC_isoC_amd1):
		p = "ISO/IEC 9899/AMD1:1995 (\\(lqISO C90\\(rq)";
		break;
	case(MDOC_isoC_tcor1):
		p = "ISO/IEC 9899/TCOR1:1994 (\\(lqISO C90\\(rq)";
		break;
	case(MDOC_isoC_tcor2):
		p = "ISO/IEC 9899/TCOR2:1995 (\\(lqISO C90\\(rq)";
		break;
	case(MDOC_isoC_99):
		p = "ISO/IEC 9899:1999 (\\(lqISO C99\\(rq)";
		break;
	case(MDOC_ansiC):
		p = "ANSI X3.159-1989 (\\(lqANSI C\\(rq)";
		break;
	case(MDOC_ansiC_89):
		p = "ANSI X3.159-1989 (\\(lqANSI C\\(rq)";
		break;
	case(MDOC_ansiC_99):
		p = "ANSI/ISO/IEC 9899-1999 (\\(lqANSI C99\\(rq)";
		break;
	case(MDOC_ieee754):
		p = "IEEE Std 754-1985";
		break;
	case(MDOC_iso8802_3):
		p = "ISO 8802-3: 1989";
		break;
	case(MDOC_xpg3):
		p = "X/Open Portability Guide Issue 3 "
			"(\\(lqXPG3\\(rq)";
		break;
	case(MDOC_xpg4):
		p = "X/Open Portability Guide Issue 4 "
			"(\\(lqXPG4\\(rq)";
		break;
	case(MDOC_xpg4_2):
		p = "X/Open Portability Guide Issue 4.2 "
			"(\\(lqXPG4.2\\(rq)";
		break;
	case(MDOC_xpg4_3):
		p = "X/Open Portability Guide Issue 4.3 "
			"(\\(lqXPG4.3\\(rq)";
		break;
	case(MDOC_xbd5):
		p = "X/Open System Interface Definitions Issue 5 "
			"(\\(lqXBD5\\(rq)";
		break;
	case(MDOC_xcu5):
		p = "X/Open Commands and Utilities Issue 5 "
			"(\\(lqXCU5\\(rq)";
		break;
	case(MDOC_xsh5):
		p = "X/Open System Interfaces and Headers Issue 5 "
			"(\\(lqXSH5\\(rq)";
		break;
	case(MDOC_xns5):
		p = "X/Open Networking Services Issue 5 "
			"(\\(lqXNS5\\(rq)";
		break;
	case(MDOC_xns5_2d2_0):
		p = "X/Open Networking Services Issue 5.2 Draft 2.0 "
			"(\\(lqXNS5.2D2.0\\(rq)";
		break;
	case(MDOC_xcurses4_2):
		p = "X/Open Curses Issue 4 Version 2 "
			"(\\(lqXCURSES4.2\\(rq)";
		break;
	case(MDOC_susv2):
		p = "Version 2 of the Single UNIX Specification";
		break;
	case(MDOC_susv3):
		p = "Version 3 of the Single UNIX Specification";
		break;
	case(MDOC_svid4):
		p = "System V Interface Definition, Fourth Edition "
			"(\\(lqSVID4\\(rq)";
		break;
	default:
		p = NULL;
		break;
	}

	return(p);
}


const char *
mdoc_att2a(enum mdoc_att c)
{
	char		*p;
	
	switch (c) {
	case(ATT_v1):
		p = "Version 1 AT&T UNIX";
		break;
	case(ATT_v2):
		p = "Version 2 AT&T UNIX";
		break;
	case(ATT_v3):
		p = "Version 3 AT&T UNIX";
		break;
	case(ATT_v4):
		p = "Version 4 AT&T UNIX";
		break;
	case(ATT_v5):
		p = "Version 5 AT&T UNIX";
		break;
	case(ATT_v6):
		p = "Version 6 AT&T UNIX";
		break;
	case(ATT_v7):
		p = "Version 7 AT&T UNIX";
		break;
	case(ATT_32v):
		p = "Version 32V AT&T UNIX";
		break;
	case(ATT_V):
		p = "AT&T System V UNIX";
		break;
	case(ATT_V1):
		p = "AT&T System V.1 UNIX";
		break;
	case(ATT_V2):
		p = "AT&T System V.2 UNIX";
		break;
	case(ATT_V3):
		p = "AT&T System V.3 UNIX";
		break;
	case(ATT_V4):
		p = "AT&T System V.4 UNIX";
		break;
	default:
		p = "AT&T UNIX";
		break;
	}

	return(p);
}


size_t
mdoc_macro2len(int macro)
{

	switch (macro) {
	case(MDOC_Ad):
		return(12);
	case(MDOC_Ao):
		return(12);
	case(MDOC_An):
		return(12);
	case(MDOC_Aq):
		return(12);
	case(MDOC_Ar):
		return(12);
	case(MDOC_Bo):
		return(12);
	case(MDOC_Bq):
		return(12);
	case(MDOC_Cd):
		return(12);
	case(MDOC_Cm):
		return(10);
	case(MDOC_Do):
		return(10);
	case(MDOC_Dq):
		return(12);
	case(MDOC_Dv):
		return(12);
	case(MDOC_Eo):
		return(12);
	case(MDOC_Em):
		return(10);
	case(MDOC_Er):
		return(12);
	case(MDOC_Ev):
		return(15);
	case(MDOC_Fa):
		return(12);
	case(MDOC_Fl):
		return(10);
	case(MDOC_Fo):
		return(16);
	case(MDOC_Fn):
		return(16);
	case(MDOC_Ic):
		return(10);
	case(MDOC_Li):
		return(16);
	case(MDOC_Ms):
		return(6);
	case(MDOC_Nm):
		return(10);
	case(MDOC_No):
		return(12);
	case(MDOC_Oo):
		return(10);
	case(MDOC_Op):
		return(14);
	case(MDOC_Pa):
		return(32);
	case(MDOC_Pf):
		return(12);
	case(MDOC_Po):
		return(12);
	case(MDOC_Pq):
		return(12);
	case(MDOC_Ql):
		return(16);
	case(MDOC_Qo):
		return(12);
	case(MDOC_So):
		return(12);
	case(MDOC_Sq):
		return(12);
	case(MDOC_Sy):
		return(6);
	case(MDOC_Sx):
		return(16);
	case(MDOC_Tn):
		return(10);
	case(MDOC_Va):
		return(12);
	case(MDOC_Vt):
		return(12);
	case(MDOC_Xr):
		return(10);
	default:
		break;
	};
	return(0);
}
