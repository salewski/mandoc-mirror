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
#include <curses.h>
#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <term.h>
#include <unistd.h>

#include "mdoc.h"


static	int		 termprint_r(size_t, size_t, 
				const struct mdoc_node *);
static	void		 termprint_head(size_t, 
				const struct mdoc_meta *);
static	void		 termprint_tail(size_t, 
				const struct mdoc_meta *);

static	char 		*arch2a(enum mdoc_arch);
static	char 		*vol2a(enum mdoc_vol);
static	char 		*msec2a(enum mdoc_msec);

static	size_t 		 ttitle2a(char *, enum mdoc_vol, enum mdoc_msec,
				enum mdoc_arch, size_t);


static char *
arch2a(enum mdoc_arch arch)
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
		return("AViion");
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
	default:	 	
		break;
	}

	return(NULL);
}


static char *
vol2a(enum mdoc_vol vol)
{

	switch (vol) {
	case (VOL_AMD):
		return("OpenBSD Ancestral Manual Documents");
	case (VOL_IND):
		return("OpenBSD Manual Master Index");
	case (VOL_KM):
		return("OpenBSD Kernel Manual");
	case (VOL_LOCAL):
		return("OpenBSD Local Manual");
	case (VOL_PRM):
		return("OpenBSD Programmer's Manual");
	case (VOL_PS1):
		return("OpenBSD Programmer's Supplementary Documents");
	case (VOL_SMM):
		return("OpenBSD System Manager's Manual");
	case (VOL_URM):
		return("OpenBSD Reference Manual");
	case (VOL_USD):
		return("OpenBSD User's Supplementary Documents");
	default:
		break;
	}

	return(NULL);
}


static char *
msec2a(enum mdoc_msec msec)
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
	default:
		break;
	}
	return(NULL);
}


static size_t
ttitle2a(char *dst, enum mdoc_vol vol, enum mdoc_msec msec,
		enum mdoc_arch arch, size_t sz)
{
	char		*p;
	size_t		 ssz;

	if (NULL == (p = vol2a(vol)))
		switch (msec) {
		case (MSEC_1):
			/* FALLTHROUGH */
		case (MSEC_6):
			/* FALLTHROUGH */
		case (MSEC_7):
			p = vol2a(VOL_URM);
			break;
		case (MSEC_8):
			p = vol2a(VOL_SMM);
			break;
		case (MSEC_2):
			/* FALLTHROUGH */
		case (MSEC_3):
			/* FALLTHROUGH */
		case (MSEC_4):
			/* FALLTHROUGH */
		case (MSEC_5):
			p = vol2a(VOL_PRM);
			break;
		case (MSEC_9):
			p = vol2a(VOL_KM);
			break;
		default:
			/* FIXME: capitalise. */
			if (NULL == (p = msec2a(msec)))
				p = msec2a(MSEC_local);
			break;
		}
	assert(p);

	if ((ssz = strlcpy(dst, p, sz)) >= sz)
		return(ssz);

	if ((p = arch2a(arch))) {
		if ((ssz = strlcat(dst, " (", sz)) >= sz)
			return(ssz);
		if ((ssz = strlcat(dst, p, sz)) >= sz)
			return(ssz);
		if ((ssz = strlcat(dst, ")", sz)) >= sz)
			return(ssz);
	}

	return(ssz);
}


static int
termprint_r(size_t cols, size_t indent, const struct mdoc_node *node)
{

	return(1);
}


static void
termprint_tail(size_t cols, const struct mdoc_meta *meta)
{
	struct tm	*tm;
	char		*buf, *os;
	size_t		 sz, osz, ssz, i;

	if (NULL == (buf = malloc(cols)))
		err(1, "malloc");
	if (NULL == (os = malloc(cols)))
		err(1, "malloc");

	tm = localtime(&meta->date);
	if (NULL == strftime(buf, cols, "%B %d, %Y", tm))
		err(1, "strftime");

	osz = strlcpy(os, meta->os, cols);

	sz = strlen(buf);
	ssz = sz + osz + 1;

	if (ssz > cols) {
		ssz -= cols;
		assert(ssz <= osz);
		os[osz - ssz] = 0;
		ssz = 1;
	} else
		ssz = cols - ssz + 1;

	printf("%s", os);
	for (i = 0; i < ssz; i++)
		printf(" ");

	printf("%s\n", buf);

	free(buf);
	free(os);
}


static void
termprint_head(size_t cols, const struct mdoc_meta *meta)
{
	char		*msec, *buf, *title;
	size_t		 ssz, tsz, ttsz, i;

	if (NULL == (buf = malloc(cols)))
		err(1, "malloc");
	if (NULL == (title = malloc(cols)))
		err(1, "malloc");

	/* Format the manual page header. */

	tsz = ttitle2a(buf, meta->vol, meta->msec, meta->arch, cols);
	ttsz = strlcpy(title, meta->title, cols);

	if (NULL == (msec = msec2a(meta->msec)))
		msec = "";

	ssz = (2 * (ttsz + 2 + strlen(msec))) + tsz + 2;

	if (ssz > cols) {
		if ((ssz -= cols) % 2)
			ssz++;
		ssz /= 2;
	
		assert(ssz <= ttsz);
		title[ttsz - ssz] = 0;
		ssz = 1;
	} else
		ssz = ((cols - ssz) / 2) + 1;

	printf("%s(%s)", title, msec);

	for (i = 0; i < ssz; i++)
		printf(" ");

	printf("%s", buf);

	for (i = 0; i < ssz; i++)
		printf(" ");

	printf("%s(%s)\n\n", title, msec);

	free(title);
	free(buf);
}


int
termprint(const struct mdoc_node *node,
		const struct mdoc_meta *meta)
{
	size_t		 cols;

	if (ERR == setupterm(NULL, STDOUT_FILENO, NULL))
		return(0);

	cols = columns < 60 ? 60 : (size_t)columns;

	termprint_head(cols, meta);
	if ( ! termprint_r(cols, 0, node))
		return(0);
	termprint_tail(cols, meta);
	return(1);
}


