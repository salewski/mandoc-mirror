# $OpenBSD: Makefile.inc,v 1.5 2020/07/30 21:32:19 schwarze Exp $

SKIP_GROFF ?= ${REGRESS_TARGETS}
SKIP_TMAN ?= ALL
SKIP_MARKDOWN ?= ALL


# OpenBSD only: maintainer targets and custom extraction

_FULLHTMLFILES = ${HTML_TARGETS:S/$/.html/}

html-clean:
.if !empty(_FULLHTMLFILES)
	rm -f ${_HTMLFILES} ${_FULLHTMLFILES}
.endif

.for t in ${HTML_TARGETS}
${t}.out_html: ${t}.in
	${MANDOC} ${MOPTS} -Thtml ${.ALLSRC} | \
		${.CURDIR}/../extract.pl > ${.TARGET}
.endfor

.include "../Makefile.inc"

.in.mandoc_html:
	${MANDOC} ${MOPTS} -Thtml ${.IMPSRC} > ${.TARGET:S/mandoc_html$/html/}
	${.CURDIR}/../extract.pl < ${.TARGET:S/mandoc_html$/html/} > ${.TARGET}
