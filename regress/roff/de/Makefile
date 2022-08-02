# $OpenBSD: Makefile,v 1.14 2020/07/30 21:32:19 schwarze Exp $

REGRESS_TARGETS	 = append cond empty escname factorial
REGRESS_TARGETS	+= indir infinite startde tab TH Dd
LINT_TARGETS	 = escname indir infinite

# groff-1.22.4 defect:
# infinite recursion aborts output completely

SKIP_GROFF	 = infinite

.include <bsd.regress.mk>
