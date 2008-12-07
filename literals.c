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

#include "libmdocml.h"
#include "private.h"
#include "ml.h"


char *
ml_literal(int tok, const int *argc, 
		const char **argv, const char **morep)
{

	switch (tok) {
	case (ROFF_Ex):
		return ("The %s utility exits 0 on success, and "
				"&gt;0 if an error occurs.");
	case (ROFF_Rv):
		return ("The %s() function returns the value 0 if "
				"successful; otherwise the value -1 "
				"is returned and the global variable "
				"<span class=\"inline-Va\">errno</span> "
				"is set to indicate the error.");
	case (ROFF_In):
		return("#include &lt;%s&gt;");
	case (ROFF_At):
		/* FIXME: this should be in roff.c. */
		assert(NULL == *argv);
		assert(ROFF_ARGMAX == *argc);
		if (NULL == *morep)
			return("AT&amp;T UNIX");
		if (0 == strcmp(*morep, "v6"))
			return("Version 6 AT&amp;T UNIX");
		else if (0 == strcmp(*morep, "v7")) 
			return("Version 7 AT&amp;T UNIX");
		else if (0 == strcmp(*morep, "32v"))
			return("Version 32v AT&amp;T UNIX");
		else if (0 == strcmp(*morep, "V.1"))
			return("AT&amp;T System V.1 UNIX");
		else if (0 == strcmp(*morep, "V.4"))
			return("AT&amp;T System V.4 UNIX");
		abort();
		/* NOTREACHED */
	case (ROFF_St):
		assert(ROFF_ARGMAX != *argc);
		assert(NULL == *argv);
		switch (*argc) {
		case(ROFF_p1003_1_88):
			return("IEEE Std 1003.1-1988 "
					"(&#8220;POSIX&#8221;)");
		case(ROFF_p1003_1_90):
			return("IEEE Std 1003.1-1990 "
					"(&#8220;POSIX&#8221;)");
		case(ROFF_p1003_1_96):
			return("ISO/IEC 9945-1:1996 "
					"(&#8220;POSIX&#8221;)");
		case(ROFF_p1003_1_2001):
			return("IEEE Std 1003.1-2001 "
					"(&#8220;POSIX&#8221;)");
		case(ROFF_p1003_1_2004):
			return("IEEE Std 1003.1-2004 "
					"(&#8220;POSIX&#8221;)");
		case(ROFF_p1003_1):
			return("IEEE Std 1003.1 "
					"(&#8220;POSIX&#8221;)");
		case(ROFF_p1003_1b):
			return("IEEE Std 1003.1b "
					"(&#8220;POSIX&#8221;)");
		case(ROFF_p1003_1b_93):
			return("IEEE Std 1003.1b-1993 "
					"(&#8220;POSIX&#8221;)");
		case(ROFF_p1003_1c_95):
			return("IEEE Std 1003.1c-1995 "
					"(&#8220;POSIX&#8221;)");
		case(ROFF_p1003_1g_2000):
			return("IEEE Std 1003.1g-2000 "
					"(&#8220;POSIX&#8221;)");
		case(ROFF_p1003_2_92):
			return("IEEE Std 1003.2-1992 "
					"(&#8220;POSIX.2&#8221;)");
		case(ROFF_p1387_2_95):
			return("IEEE Std 1387.2-1995 "
					"(&#8220;POSIX.7.2&#8221;)");
		case(ROFF_p1003_2):
			return("IEEE Std 1003.2 "
					"(&#8220;POSIX.2&#8221;)");
		case(ROFF_p1387_2):
			return("IEEE Std 1387.2 "
					"(&#8220;POSIX.7.2&#8221;)");
		case(ROFF_isoC_90):
			return("ISO/IEC 9899:1990 "
					"(&#8220;ISO C90&#8221;)");
		case(ROFF_isoC_amd1):
			return("ISO/IEC 9899/AMD1:1995 "
					"(&#8220;ISO C90&#8221;)");
		case(ROFF_isoC_tcor1):
			return("ISO/IEC 9899/TCOR1:1994 "
					"(&#8220;ISO C90&#8221;)");
		case(ROFF_isoC_tcor2):
			return("ISO/IEC 9899/TCOR2:1995 "
					"(&#8220;ISO C90&#8221;)");
		case(ROFF_isoC_99):
			return("ISO/IEC 9899:1999 "
					"(&#8220;ISO C99&#8221;)");
		case(ROFF_ansiC):
			return("ANSI X3.159-1989 "
					"(&#8220;ANSI C&#8221;)");
		case(ROFF_ansiC_89):
			return("ANSI X3.159-1989 "
					"(&#8220;ANSI C&#8221;)");
		case(ROFF_ansiC_99):
			return("ANSI/ISO/IEC 9899-1999 "
					"(&#8220;ANSI C99&#8221;)");
		case(ROFF_ieee754):
			return("IEEE Std 754-1985");
		case(ROFF_iso8802_3):
			return("ISO 8802-3: 1989");
		case(ROFF_xpg3):
			return("X/Open Portability Guide Issue 3 "
					"(&#8220;XPG3&#8221;)");
		case(ROFF_xpg4):
			return("X/Open Portability Guide Issue 4 "
					"(&#8220;XPG4&#8221;)");
		case(ROFF_xpg4_2):
			return("X/Open Portability Guide Issue 4.2 "
					"(&#8220;XPG4.2&#8221;)");
		case(ROFF_xpg4_3):
			return("X/Open Portability Guide Issue 4.3 "
					"(&#8220;XPG4.3&#8221;)");
		case(ROFF_xbd5):
			return("X/Open System Interface Definitions "
					"Issue 5 (&#8220;XBD5&#8221;)");
		case(ROFF_xcu5):
			return("X/Open Commands and Utilities Issue 5 "
					"(&#8220;XCU5&#8221;)");
		case(ROFF_xsh5):
			return("X/Open System Interfaces and Headers "
					"Issue 5 (&#8220;XSH5&#8221;)");
		case(ROFF_xns5):
			return("X/Open Networking Services Issue 5 "
					"(&#8220;XNS5&#8221;)");
		case(ROFF_xns5_2d2_0):
			return("X/Open Networking Services "
					"Issue 5.2 Draft 2.0 "
					"(&#8220;XNS5.2D2.0&#8221;)");
		case(ROFF_xcurses4_2):
			return("X/Open Curses Issue 4 Version 2 "
					"(&#8220;XCURSES4.2&#8221;)");
		case(ROFF_susv2):
			return("Version 2 of the Single "
					"UNIX Specification");
		case(ROFF_susv3):
			return("Version 3 of the Single "
					"UNIX Specification");
		case(ROFF_svid4):
			return("System V Interface Definition, Fourth "
					"Edition (&#8220;SVID4&#8221;)");
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

