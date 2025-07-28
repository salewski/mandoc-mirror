# $OpenBSD: Makefile,v 1.16 2025/07/28 15:17:54 schwarze Exp $

REGRESS_TARGETS	 = append cond empty escname factorial
REGRESS_TARGETS	+= indir infinite startde tab TH Dd
LINT_TARGETS	 = escname indir infinite

# groff-1.23.0 defect:
# infinite recursion aborts output completely

SKIP_GROFF	 = infinite

.include <bsd.regress.mk>
