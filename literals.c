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
#include <string.h>
#include <stdlib.h>

#include "private.h"

#define	ROFF_ATTname_V1		"v1"
#define	ROFF_ATTname_V2		"v2"
#define	ROFF_ATTname_V3		"v3"
#define	ROFF_ATTname_V6		"v6"
#define	ROFF_ATTname_V7		"v7"
#define	ROFF_ATTname_32V	"32v"
#define	ROFF_ATTname_V_1	"V.1"
#define	ROFF_ATTname_V_4	"V.4"

#define	ROFFSecname_NAME	"NAME"
#define	ROFFSecname_SYNOP	"SYNOPSIS"
#define	ROFFSecname_DESC	"DESCRIPTION"
#define	ROFFSecname_ENV		"ENVIRONMENT"
#define	ROFFSecname_FILES	"FILES"
#define	ROFFSecname_EX		"EXAMPLES"
#define	ROFFSecname_DIAG	"DIAGNOSTICS"
#define	ROFFSecname_ERRS	"ERRORS"
#define	ROFFSecname_STAND	"STANDARDS"
#define	ROFFSecname_HIST	"HISTORY"
#define	ROFFSecname_AUTH	"AUTHORS"
#define	ROFFSecname_CAVEATS	"CAVEATS"
#define	ROFFSecname_BUGS	"BUGS"
#define	ROFFSecname_RETVAL	"RETURN VALUES"
#define	ROFFSecname_RETVAL1	"RETURN"
#define	ROFFSecname_RETVAL2	"VALUES"
#define	ROFFSecname_SEEALSO	"SEE ALSO"
#define	ROFFSecname_SEEALSO1	"SEE"
#define	ROFFSecname_SEEALSO2	"ALSO"

#define	ROFF_MSECname_1		"1"
#define	ROFF_MSECname_2		"2"
#define	ROFF_MSECname_3		"3"
#define	ROFF_MSECname_3p	"3p"
#define	ROFF_MSECname_4		"4"
#define	ROFF_MSECname_5		"5"
#define	ROFF_MSECname_6		"6"
#define	ROFF_MSECname_7		"7"
#define	ROFF_MSECname_8		"8"
#define	ROFF_MSECname_9		"9"
#define	ROFF_MSECname_UNASS	"unass"
#define	ROFF_MSECname_DRAFT	"draft"
#define	ROFF_MSECname_PAPER	"paper"

#define	ROFF_VOLname_AMD	"AMD"
#define	ROFF_VOLname_IND	"IND"
#define	ROFF_VOLname_KM		"KM"
#define	ROFF_VOLname_LOCAL	"LOCAL"
#define	ROFF_VOLname_PRM	"PRM"
#define	ROFF_VOLname_PS1	"PS1"
#define	ROFF_VOLname_SMM	"SMM"
#define	ROFF_VOLname_URM	"URM"
#define	ROFF_VOLname_USD	"USD"
#define	ROFF_ARCHname_ALPHA	"alpha"
#define	ROFF_ARCHname_AMD64	"amd64"
#define	ROFF_ARCHname_AMIGA	"amiga"
#define	ROFF_ARCHname_ARC	"arc"
#define	ROFF_ARCHname_ARMISH	"armish"
#define	ROFF_ARCHname_AVIION	"aviion"
#define	ROFF_ARCHname_HP300	"hp300"
#define	ROFF_ARCHname_HPPA	"hppa"
#define	ROFF_ARCHname_HPPA64	"hppa64"
#define	ROFF_ARCHname_I386	"i386"
#define	ROFF_ARCHname_LANDISK	"landisk"
#define	ROFF_ARCHname_LUNA88K	"luna88k"
#define	ROFF_ARCHname_MAC68K	"mac68k"
#define	ROFF_ARCHname_MACPPC	"macppc"
#define	ROFF_ARCHname_MVME68K	"mvme68k"
#define	ROFF_ARCHname_MVME88K	"mvme88k"
#define	ROFF_ARCHname_MVMEPPC	"mvmeppc"
#define	ROFF_ARCHname_PMAX	"pmax"
#define	ROFF_ARCHname_SGI	"sgi"
#define	ROFF_ARCHname_SPARC	"sparc"
#define	ROFF_ARCHname_SPARC64	"sparc64"
#define	ROFF_ARCHname_SUN3	"sun3"
#define	ROFF_ARCHname_VAX	"vax"
#define	ROFF_ARCHname_ZAURUS	"zaurus"

int
roff_sec(const char **p)
{

	assert(*p);
	if (NULL != *(p + 1)) {
		if (NULL != *(p + 2))
			return(ROFFSec_OTHER);
		if (0 == strcmp(*p, ROFFSecname_RETVAL1) &&
			0 == strcmp(*(p + 1), ROFFSecname_RETVAL2))
			return(ROFFSec_RETVAL);
		if (0 == strcmp(*p, ROFFSecname_SEEALSO1) &&
			0 == strcmp(*(p + 1), ROFFSecname_SEEALSO2))
			return(ROFFSec_SEEALSO);
		return(ROFFSec_OTHER);
	}

	if (0 == strcmp(*p, ROFFSecname_NAME))
		return(ROFFSec_NAME);
	else if (0 == strcmp(*p, ROFFSecname_SYNOP))
		return(ROFFSec_SYNOP);
	else if (0 == strcmp(*p, ROFFSecname_DESC))
		return(ROFFSec_DESC);
	else if (0 == strcmp(*p, ROFFSecname_ENV))
		return(ROFFSec_ENV);
	else if (0 == strcmp(*p, ROFFSecname_FILES))
		return(ROFFSec_FILES);
	else if (0 == strcmp(*p, ROFFSecname_EX))
		return(ROFFSec_EX);
	else if (0 == strcmp(*p, ROFFSecname_DIAG)) 
		return(ROFFSec_DIAG);
	else if (0 == strcmp(*p, ROFFSecname_ERRS))
		return(ROFFSec_ERRS);
	else if (0 == strcmp(*p, ROFFSecname_STAND))
		return(ROFFSec_STAND);
	else if (0 == strcmp(*p, ROFFSecname_HIST))
		return(ROFFSec_HIST);
	else if (0 == strcmp(*p, ROFFSecname_AUTH))
		return(ROFFSec_AUTH);
	else if (0 == strcmp(*p, ROFFSecname_CAVEATS))
		return(ROFFSec_CAVEATS);
	else if (0 == strcmp(*p, ROFFSecname_BUGS))
		return(ROFFSec_BUGS);
	else if (0 == strcmp(*p, ROFFSecname_RETVAL))
		return(ROFFSec_RETVAL);
	else if (0 == strcmp(*p, ROFFSecname_SEEALSO))
		return(ROFFSec_SEEALSO);

	return(ROFFSec_OTHER);
}


enum roffmsec
roff_msec(const char *p)
{

	assert(p);
	if (0 == strcmp(p, ROFF_MSECname_1))
		return(ROFF_MSEC_1);
	else if (0 == strcmp(p, ROFF_MSECname_2))
		return(ROFF_MSEC_2);
	else if (0 == strcmp(p, ROFF_MSECname_3))
		return(ROFF_MSEC_3);
	else if (0 == strcmp(p, ROFF_MSECname_3p))
		return(ROFF_MSEC_3p);
	else if (0 == strcmp(p, ROFF_MSECname_4))
		return(ROFF_MSEC_4);
	else if (0 == strcmp(p, ROFF_MSECname_5))
		return(ROFF_MSEC_5);
	else if (0 == strcmp(p, ROFF_MSECname_6))
		return(ROFF_MSEC_6);
	else if (0 == strcmp(p, ROFF_MSECname_7))
		return(ROFF_MSEC_7);
	else if (0 == strcmp(p, ROFF_MSECname_8))
		return(ROFF_MSEC_8);
	else if (0 == strcmp(p, ROFF_MSECname_9))
		return(ROFF_MSEC_9);
	else if (0 == strcmp(p, ROFF_MSECname_UNASS))
		return(ROFF_MSEC_UNASS);
	else if (0 == strcmp(p, ROFF_MSECname_DRAFT))
		return(ROFF_MSEC_DRAFT);
	else if (0 == strcmp(p, ROFF_MSECname_PAPER))
		return(ROFF_MSEC_PAPER);

	return(ROFF_MSEC_MAX);
}


char *
roff_msecname(enum roffmsec sec) 
{

	switch (sec) {
	case(ROFF_MSEC_1):
		return(ROFF_MSECname_1);
	case(ROFF_MSEC_2):
		return(ROFF_MSECname_2);
	case(ROFF_MSEC_3):
		return(ROFF_MSECname_3);
	case(ROFF_MSEC_3p):
		return(ROFF_MSECname_3p);
	case(ROFF_MSEC_4):
		return(ROFF_MSECname_4);
	case(ROFF_MSEC_5):
		return(ROFF_MSECname_5);
	case(ROFF_MSEC_6):
		return(ROFF_MSECname_6);
	case(ROFF_MSEC_7):
		return(ROFF_MSECname_7);
	case(ROFF_MSEC_8):
		return(ROFF_MSECname_8);
	case(ROFF_MSEC_9):
		return(ROFF_MSECname_9);
	case(ROFF_MSEC_UNASS):
		return(ROFF_MSECname_UNASS);
	case(ROFF_MSEC_DRAFT):
		return(ROFF_MSECname_DRAFT);
	case(ROFF_MSEC_PAPER):
		return(ROFF_MSECname_PAPER);
	default:
		break;
	}

	abort();
	/* NOTREACHED */
}


char *
roff_fmtstring(int tok)
{

	switch (tok) {
	case (ROFF_Ex):
		return ("The %s utility exits 0 on success, and "
				">0 if an error occurs.");
	case (ROFF_Rv):
		return ("The %s() function returns the value 0 if "
				"successful; otherwise the value -1 "
				"is returned and the global variable "
				"errno is set to indicate the error.");
	case (ROFF_In):
		return("#include \\*(Lt%s\\*(Gt");
	default:
		break;
	}

	abort();
	/* NOTREACHED */
}


char *
roff_literal(int tok, const int *argc, 
		const char **argv, const char **morep)
{

	switch (tok) {
	case (ROFF_At):
		assert(NULL == *argv);
		assert(ROFF_ARGMAX == *argc);
		if (NULL == *morep)
			return("AT&T UNIX");

		switch (roff_att(*morep)) {
		case (ROFF_ATT_V1):
			return("Version 1 AT&T UNIX");
		case (ROFF_ATT_V2):
			return("Version 2 AT&T UNIX");
		case (ROFF_ATT_V3):
			return("Version 3 AT&T UNIX");
		case (ROFF_ATT_V6):
			return("Version 6 AT&T UNIX");
		case (ROFF_ATT_V7):
			return("Version 7 AT&T UNIX");
		case (ROFF_ATT_32V):
			return("Version 32v AT&T UNIX");
		case (ROFF_ATT_V_1):
			return("AT&T System V.1 UNIX");
		case (ROFF_ATT_V_4):
			return("AT&T System V.4 UNIX");
		default:
			break;
		}

		abort();
		/* NOTREACHED */

	case (ROFF_St):
		assert(ROFF_ARGMAX != *argc);
		assert(NULL == *argv);
		switch (*argc) {
		case(ROFF_p1003_1_88):
			return("IEEE Std 1003.1-1988 "
					"(\\*(LqPOSIX\\*(Rq)");
		case(ROFF_p1003_1_90):
			return("IEEE Std 1003.1-1990 "
					"(\\*(LqPOSIX\\*(Rq)");
		case(ROFF_p1003_1_96):
			return("ISO/IEC 9945-1:1996 "
					"(\\*(LqPOSIX\\*(Rq)");
		case(ROFF_p1003_1_2001):
			return("IEEE Std 1003.1-2001 "
					"(\\*(LqPOSIX\\*(Rq)");
		case(ROFF_p1003_1_2004):
			return("IEEE Std 1003.1-2004 "
					"(\\*(LqPOSIX\\*(Rq)");
		case(ROFF_p1003_1):
			return("IEEE Std 1003.1 "
					"(\\*(LqPOSIX\\*(Rq)");
		case(ROFF_p1003_1b):
			return("IEEE Std 1003.1b "
					"(\\*(LqPOSIX\\*(Rq)");
		case(ROFF_p1003_1b_93):
			return("IEEE Std 1003.1b-1993 "
					"(\\*(LqPOSIX\\*(Rq)");
		case(ROFF_p1003_1c_95):
			return("IEEE Std 1003.1c-1995 "
					"(\\*(LqPOSIX\\*(Rq)");
		case(ROFF_p1003_1g_2000):
			return("IEEE Std 1003.1g-2000 "
					"(\\*(LqPOSIX\\*(Rq)");
		case(ROFF_p1003_2_92):
			return("IEEE Std 1003.2-1992 "
					"(\\*(LqPOSIX.2\\*(Rq)");
		case(ROFF_p1387_2_95):
			return("IEEE Std 1387.2-1995 "
					"(\\*(LqPOSIX.7.2\\*(Rq)");
		case(ROFF_p1003_2):
			return("IEEE Std 1003.2 "
					"(\\*(LqPOSIX.2\\*(Rq)");
		case(ROFF_p1387_2):
			return("IEEE Std 1387.2 "
					"(\\*(LqPOSIX.7.2\\*(Rq)");
		case(ROFF_isoC_90):
			return("ISO/IEC 9899:1990 "
					"(\\*(LqISO C90\\*(Rq)");
		case(ROFF_isoC_amd1):
			return("ISO/IEC 9899/AMD1:1995 "
					"(\\*(LqISO C90\\*(Rq)");
		case(ROFF_isoC_tcor1):
			return("ISO/IEC 9899/TCOR1:1994 "
					"(\\*(LqISO C90\\*(Rq)");
		case(ROFF_isoC_tcor2):
			return("ISO/IEC 9899/TCOR2:1995 "
					"(\\*(LqISO C90\\*(Rq)");
		case(ROFF_isoC_99):
			return("ISO/IEC 9899:1999 "
					"(\\*(LqISO C99\\*(Rq)");
		case(ROFF_ansiC):
			return("ANSI X3.159-1989 "
					"(\\*(LqANSI C\\*(Rq)");
		case(ROFF_ansiC_89):
			return("ANSI X3.159-1989 "
					"(\\*(LqANSI C\\*(Rq)");
		case(ROFF_ansiC_99):
			return("ANSI/ISO/IEC 9899-1999 "
					"(\\*(LqANSI C99\\*(Rq)");
		case(ROFF_ieee754):
			return("IEEE Std 754-1985");
		case(ROFF_iso8802_3):
			return("ISO 8802-3: 1989");
		case(ROFF_xpg3):
			return("X/Open Portability Guide Issue 3 "
					"(\\*(LqXPG3\\*(Rq)");
		case(ROFF_xpg4):
			return("X/Open Portability Guide Issue 4 "
					"(\\*(LqXPG4\\*(Rq)");
		case(ROFF_xpg4_2):
			return("X/Open Portability Guide Issue 4.2 "
					"(\\*(LqXPG4.2\\*(Rq)");
		case(ROFF_xpg4_3):
			return("X/Open Portability Guide Issue 4.3 "
					"(\\*(LqXPG4.3\\*(Rq)");
		case(ROFF_xbd5):
			return("X/Open System Interface Definitions "
					"Issue 5 (\\*(LqXBD5\\*(Rq)");
		case(ROFF_xcu5):
			return("X/Open Commands and Utilities Issue 5 "
					"(\\*(LqXCU5\\*(Rq)");
		case(ROFF_xsh5):
			return("X/Open System Interfaces and Headers "
					"Issue 5 (\\*(LqXSH5\\*(Rq)");
		case(ROFF_xns5):
			return("X/Open Networking Services Issue 5 "
					"(\\*(LqXNS5\\*(Rq)");
		case(ROFF_xns5_2d2_0):
			return("X/Open Networking Services "
					"Issue 5.2 Draft 2.0 "
					"(\\*(LqXNS5.2D2.0\\*(Rq)");
		case(ROFF_xcurses4_2):
			return("X/Open Curses Issue 4 Version 2 "
					"(\\*(LqXCURSES4.2\\*(Rq)");
		case(ROFF_susv2):
			return("Version 2 of the Single "
					"UNIX Specification");
		case(ROFF_susv3):
			return("Version 3 of the Single "
					"UNIX Specification");
		case(ROFF_svid4):
			return("System V Interface Definition, Fourth "
					"Edition (\\*(LqSVID4\\*(Rq)");
		default:
			break;
		}
		abort();
		/* NOTREACHED */

	case (ROFF_Bt):
		return("is currently in beta test.");
	case (ROFF_Ud):
		return("currently under development.");
	case (ROFF_Fx):
		return("FreeBSD");
	case (ROFF_Nx):
		return("NetBSD");
	case (ROFF_Ox):
		return("OpenBSD");
	case (ROFF_Ux):
		return("UNIX");
	case (ROFF_Bx):
		return("BSD");
	case (ROFF_Bsx):
		return("BSDI BSD/OS");
	default:
		break;
	}

	abort();
	/* NOTREACHED */
}


enum roffatt
roff_att(const char *p)
{

	assert(p);
	if (0 == strcmp(p, ROFF_ATTname_V1))
		return(ROFF_ATT_V1);
	else if (0 == strcmp(p, ROFF_ATTname_V2)) 
		return(ROFF_ATT_V2);
	else if (0 == strcmp(p, ROFF_ATTname_V3)) 
		return(ROFF_ATT_V3);
	else if (0 == strcmp(p, ROFF_ATTname_V6)) 
		return(ROFF_ATT_V6);
	else if (0 == strcmp(p, ROFF_ATTname_V7)) 
		return(ROFF_ATT_V7);
	else if (0 == strcmp(p, ROFF_ATTname_32V))
		return(ROFF_ATT_32V);
	else if (0 == strcmp(p, ROFF_ATTname_V_1))
		return(ROFF_ATT_V_1);
	else if (0 == strcmp(p, ROFF_ATTname_V_4))
		return(ROFF_ATT_V_4);

	return(ROFF_ATT_MAX);
}


enum roffvol
roff_vol(const char *p)
{

	if (0 == strcmp(p, ROFF_VOLname_AMD))
		return(ROFF_VOL_AMD);
	else if (0 == strcmp(p, ROFF_VOLname_IND))
		return(ROFF_VOL_IND);
	else if (0 == strcmp(p, ROFF_VOLname_KM))
		return(ROFF_VOL_KM);
	else if (0 == strcmp(p, ROFF_VOLname_LOCAL))
		return(ROFF_VOL_LOCAL);
	else if (0 == strcmp(p, ROFF_VOLname_PRM))
		return(ROFF_VOL_PRM);
	else if (0 == strcmp(p, ROFF_VOLname_PS1))
		return(ROFF_VOL_PS1);
	else if (0 == strcmp(p, ROFF_VOLname_SMM))
		return(ROFF_VOL_SMM);
	else if (0 == strcmp(p, ROFF_VOLname_URM))
		return(ROFF_VOL_URM);
	else if (0 == strcmp(p, ROFF_VOLname_USD))
		return(ROFF_VOL_USD);
	else if (0 == strcmp(p, ROFF_ARCHname_ALPHA))
		return(ROFF_ARCH_ALPHA);
	else if (0 == strcmp(p, ROFF_ARCHname_AMD64))
		return(ROFF_ARCH_AMD64);
	else if (0 == strcmp(p, ROFF_ARCHname_AMIGA))
		return(ROFF_ARCH_AMIGA);
	else if (0 == strcmp(p, ROFF_ARCHname_ARC))
		return(ROFF_ARCH_ARC);
	else if (0 == strcmp(p, ROFF_ARCHname_ARMISH))
		return(ROFF_ARCH_ARMISH);
	else if (0 == strcmp(p, ROFF_ARCHname_AVIION))
		return(ROFF_ARCH_AVIION);
	else if (0 == strcmp(p, ROFF_ARCHname_HP300))
		return(ROFF_ARCH_HP300);
	else if (0 == strcmp(p, ROFF_ARCHname_HPPA))
		return(ROFF_ARCH_HPPA);
	else if (0 == strcmp(p, ROFF_ARCHname_HPPA64))
		return(ROFF_ARCH_HPPA64);
	else if (0 == strcmp(p, ROFF_ARCHname_I386))
		return(ROFF_ARCH_I386);
	else if (0 == strcmp(p, ROFF_ARCHname_LANDISK))
		return(ROFF_ARCH_LANDISK);
	else if (0 == strcmp(p, ROFF_ARCHname_LUNA88K))
		return(ROFF_ARCH_LUNA88K);
	else if (0 == strcmp(p, ROFF_ARCHname_MAC68K))
		return(ROFF_ARCH_MAC68K);
	else if (0 == strcmp(p, ROFF_ARCHname_MACPPC))
		return(ROFF_ARCH_MACPPC);
	else if (0 == strcmp(p, ROFF_ARCHname_MVME68K))
		return(ROFF_ARCH_MVME68K);
	else if (0 == strcmp(p, ROFF_ARCHname_MVME88K))
		return(ROFF_ARCH_MVME88K);
	else if (0 == strcmp(p, ROFF_ARCHname_MVMEPPC))
		return(ROFF_ARCH_MVMEPPC);
	else if (0 == strcmp(p, ROFF_ARCHname_PMAX))
		return(ROFF_ARCH_PMAX);
	else if (0 == strcmp(p, ROFF_ARCHname_SGI))
		return(ROFF_ARCH_SGI);
	else if (0 == strcmp(p, ROFF_ARCHname_SPARC))
		return(ROFF_ARCH_SPARC);
	else if (0 == strcmp(p, ROFF_ARCHname_SPARC64))
		return(ROFF_ARCH_SPARC64);
	else if (0 == strcmp(p, ROFF_ARCHname_SUN3))
		return(ROFF_ARCH_SUN3);
	else if (0 == strcmp(p, ROFF_ARCHname_VAX))
		return(ROFF_ARCH_VAX);
	else if (0 == strcmp(p, ROFF_ARCHname_ZAURUS))
		return(ROFF_ARCH_ZAURUS);

	return(ROFF_VOL_MAX);
}


char *
roff_volname(enum roffvol vol)
{

	/* FIXME: is OpenBSD specific! */

	switch (vol) {
	case(ROFF_VOL_AMD):
		return("OpenBSD Ancestral Manual Documents");
	case(ROFF_VOL_IND):
		return("OpenBSD Manual Master Index");
	case(ROFF_VOL_KM):
		return("OpenBSD Kernel Manual");
	case(ROFF_VOL_LOCAL):
		return("OpenBSD Local Manual");
	case(ROFF_VOL_PRM):
		return("OpenBSD Programmer's Manual");
	case(ROFF_VOL_PS1):
		return("OpenBSD Programmer's Supplementary Documents");
	case(ROFF_VOL_SMM):
		return("OpenBSD System Manager's Manual");
	case(ROFF_VOL_URM):
		return("OpenBSD Reference Manual");
	case(ROFF_VOL_USD):
		return("OpenBSD User's Supplementary Documents");
	case(ROFF_ARCH_ALPHA):
		return("Alpha");
	case(ROFF_ARCH_AMD64):
		return("AMD64");
	case(ROFF_ARCH_AMIGA):
		return("Amiga");
	case(ROFF_ARCH_ARC):
		return("ARC");
	case(ROFF_ARCH_ARMISH):
		return("ARMISH");
	case(ROFF_ARCH_AVIION):
		return("AVIION");
	case(ROFF_ARCH_HP300):
		return("HP300");
	case(ROFF_ARCH_HPPA):
		return("HPPA");
	case(ROFF_ARCH_HPPA64):
		return("HPPA64");
	case(ROFF_ARCH_I386):
		return("i386");
	case(ROFF_ARCH_LANDISK):
		return("LANDISK");
	case(ROFF_ARCH_LUNA88K):
		return("Luna88K");
	case(ROFF_ARCH_MAC68K):
		return("Mac68K");
	case(ROFF_ARCH_MACPPC):
		return("MacPPC");
	case(ROFF_ARCH_MVME68K):
		return("MVME68K");
	case(ROFF_ARCH_MVME88K):
		return("MVME88K");
	case(ROFF_ARCH_MVMEPPC):
		return("MVMEPPC");
	case(ROFF_ARCH_PMAX):
		return("PMAX");
	case(ROFF_ARCH_SGI):
		return("SGI");
	case(ROFF_ARCH_SPARC):
		return("SPARC");
	case(ROFF_ARCH_SPARC64):
		return("SPARC64");
	case(ROFF_ARCH_SUN3):
		return("Sun3");
	case(ROFF_ARCH_VAX):
		return("Vax");
	case(ROFF_ARCH_ZAURUS):
		return("Zaurus");
	default:
		break;
	}

	abort();
	/* NOTREACHED */
}
