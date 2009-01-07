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
#include <stdlib.h>
#ifdef __linux__
#include <time.h>
#endif

#include "private.h"

/* FIXME: deprecate into actions.c! */

static int		 prologue_dt(MACRO_PROT_ARGS);
static int		 prologue_dd(MACRO_PROT_ARGS);
static int		 prologue_os(MACRO_PROT_ARGS);

static int
prologue_dt(MACRO_PROT_ARGS)
{
	int		  lastarg, j;
	char		 *args[MDOC_LINEARG_MAX];

	if (SEC_PROLOGUE != mdoc->sec_lastn)
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_NPROLOGUE));
	if (0 == mdoc->meta.date)
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_PROLOGUE_OO));
	if (mdoc->meta.title[0])
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_PROLOGUE_REP));

	j = -1;
	lastarg = ppos;

again:
	if (j == MDOC_LINEARG_MAX)
		return(mdoc_err(mdoc, tok, lastarg, ERR_ARGS_MANY));

	lastarg = *pos;

	switch (mdoc_args(mdoc, tok, pos, buf, 0, &args[++j])) {
	case (ARGS_EOLN):
		if (mdoc->meta.title)
			return(1);
		if ( ! mdoc_warn(mdoc, tok, ppos, WARN_ARGS_GE1))
			return(0);
		(void)xstrlcpy(mdoc->meta.title, 
				"UNTITLED", META_TITLE_SZ);
		return(1);
	case (ARGS_ERROR):
		return(0);
	default:
		break;
	}

	if (0 == j) {
		if (xstrlcpy(mdoc->meta.title, args[0], META_TITLE_SZ))
			goto again;
		return(mdoc_err(mdoc, tok, lastarg, ERR_SYNTAX_ARGFORM));

	} else if (1 == j) {
		mdoc->meta.msec = mdoc_atomsec(args[1]);
		if (MSEC_DEFAULT != mdoc->meta.msec)
			goto again;
		return(mdoc_err(mdoc, tok, -1, ERR_SYNTAX_ARGFORM));

	} else if (2 == j) {
		mdoc->meta.vol = mdoc_atovol(args[2]);
		if (VOL_DEFAULT != mdoc->meta.vol)
			goto again;
		mdoc->meta.arch = mdoc_atoarch(args[2]);
		if (ARCH_DEFAULT != mdoc->meta.arch)
			goto again;
		return(mdoc_err(mdoc, tok, lastarg, ERR_SYNTAX_ARGFORM));
	}

	return(mdoc_err(mdoc, tok, lastarg, ERR_ARGS_MANY));
}


static int
prologue_os(MACRO_PROT_ARGS)
{
	int		  lastarg, j;
	char		 *args[MDOC_LINEARG_MAX];

	/* FIXME: if we use `Os' again... ? */

	if (SEC_PROLOGUE != mdoc->sec_lastn)
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_NPROLOGUE));
	if (0 == mdoc->meta.title[0])
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_PROLOGUE_OO));
	if (mdoc->meta.os[0])
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_PROLOGUE_REP));

	j = -1;
	lastarg = ppos;

again:
	if (j == MDOC_LINEARG_MAX)
		return(mdoc_err(mdoc, tok, lastarg, ERR_ARGS_MANY));

	lastarg = *pos;

	switch (mdoc_args(mdoc, tok, pos, buf, 
				ARGS_QUOTED, &args[++j])) {
	case (ARGS_EOLN):
		mdoc->sec_lastn = mdoc->sec_last = SEC_BODY;
		return(1);
	case (ARGS_ERROR):
		return(0);
	default:
		break;
	}
	
	if ( ! xstrlcat(mdoc->meta.os, args[j], sizeof(mdoc->meta.os)))
		return(mdoc_err(mdoc, tok, lastarg, ERR_SYNTAX_ARGFORM));
	if ( ! xstrlcat(mdoc->meta.os, " ", sizeof(mdoc->meta.os)))
		return(mdoc_err(mdoc, tok, lastarg, ERR_SYNTAX_ARGFORM));

	goto again;
	/* NOTREACHED */
}


static int
prologue_dd(MACRO_PROT_ARGS)
{
	int		  lastarg, j;
	char		 *args[MDOC_LINEARG_MAX], date[64];

	if (SEC_PROLOGUE != mdoc->sec_lastn)
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_NPROLOGUE));
	if (mdoc->meta.title[0])
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_PROLOGUE_OO));
	if (mdoc->meta.date)
		return(mdoc_err(mdoc, tok, ppos, ERR_SEC_PROLOGUE_REP));

	j = -1;
	date[0] = 0;
	lastarg = ppos;

again:
	if (j == MDOC_LINEARG_MAX)
		return(mdoc_err(mdoc, tok, lastarg, ERR_ARGS_MANY));

	lastarg = *pos;
	switch (mdoc_args(mdoc, tok, pos, buf, 0, &args[++j])) {
	case (ARGS_EOLN):
		if (mdoc->meta.date)
			return(1);
		mdoc->meta.date = mdoc_atotime(date);
		if (mdoc->meta.date)
			return(1);
		return(mdoc_err(mdoc, tok, ppos, ERR_SYNTAX_ARGFORM));
	case (ARGS_ERROR):
		return(0);
	default:
		break;
	}
	
	if (MDOC_MAX != mdoc_find(mdoc, args[j]) && ! mdoc_warn
			(mdoc, tok, lastarg, WARN_SYNTAX_MACLIKE))
		return(0);
	
	if (0 == j) {
		if (xstrcmp("$Mdocdate$", args[j])) {
			mdoc->meta.date = time(NULL);
			goto again;
		} else if (xstrcmp("$Mdocdate:", args[j])) 
			goto again;
	} else if (4 == j)
		if ( ! xstrcmp("$", args[j]))
			goto again;

	if ( ! xstrlcat(date, args[j], sizeof(date)))
		return(mdoc_err(mdoc, tok, lastarg, ERR_SYNTAX_ARGFORM));
	if ( ! xstrlcat(date, " ", sizeof(date)))
		return(mdoc_err(mdoc, tok, lastarg, ERR_SYNTAX_ARGFORM));

	goto again;
	/* NOTREACHED */
}


int
macro_prologue(MACRO_PROT_ARGS)
{

	switch (tok) {
	case (MDOC_Dt):
		return(prologue_dt(mdoc, tok, line, ppos, pos, buf));
	case (MDOC_Dd):
		return(prologue_dd(mdoc, tok, line, ppos, pos, buf));
	case (MDOC_Os):
		return(prologue_os(mdoc, tok, line, ppos, pos, buf));
	default:
		break;
	}

	abort();
	/* NOTREACHED */
}

