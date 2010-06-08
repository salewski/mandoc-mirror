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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "out.h"
#include "main.h"
#include "term.h"

#define	PS_CHAR_WIDTH	  6
#define	PS_CHAR_HEIGHT	  12
#define	PS_CHAR_TOPMARG	 (792 - 24)
#define	PS_CHAR_TOP	 (PS_CHAR_TOPMARG - 36)
#define	PS_CHAR_LEFT	  36
#define	PS_CHAR_BOTMARG	  24
#define	PS_CHAR_BOT	 (PS_CHAR_BOTMARG + 36)

static	void		  ps_letter(struct termp *, char);
static	void		  ps_begin(struct termp *);
static	void		  ps_end(struct termp *);
static	void		  ps_pageopen(struct termp *);
static	void		  ps_advance(struct termp *, size_t);
static	void		  ps_endline(struct termp *);


void *
ps_alloc(void)
{
	struct termp	*p;

	if (NULL == (p = term_alloc(TERMENC_ASCII)))
		return(NULL);

	p->type = TERMTYPE_PS;
	p->letter = ps_letter;
	p->begin = ps_begin;
	p->end = ps_end;
	p->advance = ps_advance;
	p->endline = ps_endline;
	return(p);
}


void
ps_free(void *arg)
{

	term_free((struct termp *)arg);
}


static void
ps_end(struct termp *p)
{

	printf("%s\n", "%%EOF");
}


static void
ps_begin(struct termp *p)
{

	/*
	 * Emit the standard PostScript prologue, set our initial page
	 * position, then run pageopen() on the initial page.
	 */

	printf("%s\n", "%!PS");
	printf("%s\n", "/Courier");
	printf("%s\n", "10 selectfont");

	p->engine.ps.pspage = 1;
	p->engine.ps.psstate = 0;
	ps_pageopen(p);
}


static void
ps_letter(struct termp *p, char c)
{
	
	if ( ! (PS_INLINE & p->engine.ps.psstate)) {
		/*
		 * If we're not in a PostScript "word" context, then
		 * open one now at the current cursor.
		 */
		printf("%zu %zu moveto\n", 
				p->engine.ps.pscol, 
				p->engine.ps.psrow);
		putchar('(');
		p->engine.ps.psstate |= PS_INLINE;
	}

	/*
	 * We need to escape these characters as per the PostScript
	 * specification.  We would also escape non-graphable characters
	 * (like tabs), but none of them would get to this point and
	 * it's superfluous to abort() on them.
	 */

	switch (c) {
	case ('('):
		/* FALLTHROUGH */
	case (')'):
		/* FALLTHROUGH */
	case ('\\'):
		putchar('\\');
		break;
	default:
		break;
	}

	/* Write the character and adjust where we are on the page. */
	putchar(c);
	p->engine.ps.pscol += PS_CHAR_WIDTH;
}


/*
 * Open a page.  This is only used for -Tps at the moment.  It opens a
 * page context, printing the header and the footer.  THE OUTPUT BUFFER
 * MUST BE EMPTY.  If it is not, output will ghost on the next line and
 * we'll be all gross and out of state.
 */
static void
ps_pageopen(struct termp *p)
{
	
	assert(TERMTYPE_PS == p->type);
	assert(0 == p->engine.ps.psstate);

	p->engine.ps.pscol = PS_CHAR_LEFT;
	p->engine.ps.psrow = PS_CHAR_TOPMARG;
	p->engine.ps.psstate |= PS_MARGINS;

	(*p->headf)(p, p->argf);
	(*p->endline)(p);

	p->engine.ps.psstate &= ~PS_MARGINS;
	assert(0 == p->engine.ps.psstate);

	p->engine.ps.pscol = PS_CHAR_LEFT;
	p->engine.ps.psrow = PS_CHAR_BOTMARG;
	p->engine.ps.psstate |= PS_MARGINS;

	(*p->footf)(p, p->argf);
	(*p->endline)(p);

	p->engine.ps.psstate &= ~PS_MARGINS;
	assert(0 == p->engine.ps.psstate);

	p->engine.ps.pscol = PS_CHAR_LEFT;
	p->engine.ps.psrow = PS_CHAR_TOP;

}


static void
ps_advance(struct termp *p, size_t len)
{

	if (PS_INLINE & p->engine.ps.psstate) {
		/* Dump out any existing line scope. */
		printf(") show\n");
		p->engine.ps.psstate &= ~PS_INLINE;
	}

	p->engine.ps.pscol += len ? len * PS_CHAR_WIDTH : 0;
}


static void
ps_endline(struct termp *p)
{

	if (PS_INLINE & p->engine.ps.psstate) {
		printf(") show\n");
		p->engine.ps.psstate &= ~PS_INLINE;
	} 

	if (PS_MARGINS & p->engine.ps.psstate)
		return;

	p->engine.ps.pscol = PS_CHAR_LEFT;
	if (p->engine.ps.psrow >= PS_CHAR_HEIGHT + PS_CHAR_BOT) {
		p->engine.ps.psrow -= PS_CHAR_HEIGHT;
		return;
	}

	/* 
	 * XXX: can't run pageopen() until we're certain a flushln() has
	 * occured, else the buf will reopen in an awkward state on the
	 * next line.
	 */
	printf("showpage\n");
	p->engine.ps.psrow = PS_CHAR_TOP;
}
