# $Id$
#
# Copyright (c) 2010, 2011, 2012 Kristaps Dzonsons <kristaps@bsd.lv>
# Copyright (c) 2011, 2013, 2014, 2015 Ingo Schwarze <schwarze@openbsd.org>
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

VERSION = 1.13.3

# === LIST OF FILES ====================================================

TESTSRCS	 = test-dirent-namlen.c \
		   test-fgetln.c \
		   test-fts.c \
		   test-getsubopt.c \
		   test-isblank.c \
		   test-mkdtemp.c \
		   test-mmap.c \
		   test-ohash.c \
		   test-reallocarray.c \
		   test-sqlite3.c \
		   test-sqlite3_errstr.c \
		   test-strcasestr.c \
		   test-stringlist.c \
		   test-strlcat.c \
		   test-strlcpy.c \
		   test-strptime.c \
		   test-strsep.c \
		   test-strtonum.c \
		   test-vasprintf.c \
		   test-wchar.c

SRCS		 = att.c \
		   cgi.c \
		   chars.c \
		   compat_fgetln.c \
		   compat_fts.c \
		   compat_getsubopt.c \
		   compat_isblank.c \
		   compat_mkdtemp.c \
		   compat_ohash.c \
		   compat_reallocarray.c \
		   compat_sqlite3_errstr.c \
		   compat_strcasestr.c \
		   compat_stringlist.c \
		   compat_strlcat.c \
		   compat_strlcpy.c \
		   compat_strsep.c \
		   compat_strtonum.c \
		   compat_vasprintf.c \
		   demandoc.c \
		   eqn.c \
		   eqn_html.c \
		   eqn_term.c \
		   html.c \
		   lib.c \
		   main.c \
		   man.c \
		   man_hash.c \
		   man_html.c \
		   man_macro.c \
		   man_term.c \
		   man_validate.c \
		   mandoc.c \
		   mandoc_aux.c \
		   mandocdb.c \
		   manpage.c \
		   manpath.c \
		   mansearch.c \
		   mansearch_const.c \
		   mdoc.c \
		   mdoc_argv.c \
		   mdoc_hash.c \
		   mdoc_html.c \
		   mdoc_macro.c \
		   mdoc_man.c \
		   mdoc_term.c \
		   mdoc_validate.c \
		   msec.c \
		   out.c \
		   preconv.c \
		   read.c \
		   roff.c \
		   soelim.c \
		   st.c \
		   tbl.c \
		   tbl_data.c \
		   tbl_html.c \
		   tbl_layout.c \
		   tbl_opts.c \
		   tbl_term.c \
		   term.c \
		   term_ascii.c \
		   term_ps.c \
		   tree.c \
		   $(TESTSRCS)

DISTFILES	 = INSTALL \
		   LICENSE \
		   Makefile \
		   Makefile.depend \
		   NEWS \
		   TODO \
		   apropos.1 \
		   cgi.h.example \
		   chars.in \
		   compat_fts.h \
		   compat_ohash.h \
		   compat_stringlist.h \
		   configure \
		   configure.local.example \
		   demandoc.1 \
		   eqn.7 \
		   example.style.css \
		   gmdiff \
		   html.h \
		   lib.in \
		   libman.h \
		   libmandoc.h \
		   libmdoc.h \
		   libroff.h \
		   main.h \
		   makewhatis.8 \
		   man-cgi.css \
		   man.1 \
		   man.7 \
		   man.cgi.8 \
		   man.conf.5 \
		   man.h \
		   manconf.h \
		   mandoc.1 \
		   mandoc.3 \
		   mandoc.db.5 \
		   mandoc.h \
		   mandoc_aux.h \
		   mandoc_char.7 \
		   mandoc_escape.3 \
		   mandoc_headers.3 \
		   mandoc_html.3 \
		   mandoc_malloc.3 \
		   mansearch.3 \
		   mansearch.h \
		   mchars_alloc.3 \
		   mdoc.7 \
		   mdoc.h \
		   msec.in \
		   out.h \
		   predefs.in \
		   roff.7 \
		   roff.h \
		   soelim.1 \
		   st.in \
		   style.css \
		   tbl.3 \
		   tbl.7 \
		   term.h \
		   $(SRCS)

LIBMAN_OBJS	 = man.o \
		   man_hash.o \
		   man_macro.o \
		   man_validate.o

LIBMDOC_OBJS	 = att.o \
		   lib.o \
		   mdoc.o \
		   mdoc_argv.o \
		   mdoc_hash.o \
		   mdoc_macro.o \
		   mdoc_validate.o \
		   st.o

LIBROFF_OBJS	 = eqn.o \
		   roff.o \
		   tbl.o \
		   tbl_data.o \
		   tbl_layout.o \
		   tbl_opts.o

LIBMANDOC_OBJS	 = $(LIBMAN_OBJS) \
		   $(LIBMDOC_OBJS) \
		   $(LIBROFF_OBJS) \
		   chars.o \
		   mandoc.o \
		   mandoc_aux.o \
		   msec.o \
		   preconv.o \
		   read.o

COMPAT_OBJS	 = compat_fgetln.o \
		   compat_fts.o \
		   compat_getsubopt.o \
		   compat_isblank.o \
		   compat_mkdtemp.o \
		   compat_ohash.o \
		   compat_reallocarray.o \
		   compat_sqlite3_errstr.o \
		   compat_strcasestr.o \
		   compat_strlcat.o \
		   compat_strlcpy.o \
		   compat_strsep.o \
		   compat_strtonum.o \
		   compat_vasprintf.o

MANDOC_HTML_OBJS = eqn_html.o \
		   html.o \
		   man_html.o \
		   mdoc_html.o \
		   tbl_html.o

MANDOC_MAN_OBJS  = mdoc_man.o

MANDOC_TERM_OBJS = eqn_term.o \
		   man_term.o \
		   mdoc_term.o \
		   term.o \
		   term_ascii.o \
		   term_ps.o \
		   tbl_term.o

BASE_OBJS	 = $(MANDOC_HTML_OBJS) \
		   $(MANDOC_MAN_OBJS) \
		   $(MANDOC_TERM_OBJS) \
		   main.o \
		   manpath.o \
		   out.o \
		   tree.o

MAIN_OBJS	 = $(BASE_OBJS)

DB_OBJS		 = mandocdb.o \
		   mansearch.o \
		   mansearch_const.o

CGI_OBJS	 = $(MANDOC_HTML_OBJS) \
		   cgi.o \
		   mansearch.o \
		   mansearch_const.o \
		   out.o

MANPAGE_OBJS	 = manpage.o mansearch.o mansearch_const.o manpath.o

DEMANDOC_OBJS	 = demandoc.o

SOELIM_OBJS	 = soelim.o compat_stringlist.o

WWW_MANS	 = apropos.1.html \
		   demandoc.1.html \
		   man.1.html \
		   mandoc.1.html \
		   soelim.1.html \
		   mandoc.3.html \
		   mandoc_escape.3.html \
		   mandoc_headers.3.html \
		   mandoc_html.3.html \
		   mandoc_malloc.3.html \
		   mansearch.3.html \
		   mchars_alloc.3.html \
		   tbl.3.html \
		   man.conf.5.html \
		   mandoc.db.5.html \
		   eqn.7.html \
		   man.7.html \
		   mandoc_char.7.html \
		   mdoc.7.html \
		   roff.7.html \
		   tbl.7.html \
		   makewhatis.8.html \
		   man.cgi.8.html \
		   man.h.html \
		   manconf.h.html \
		   mandoc.h.html \
		   mandoc_aux.h.html \
		   mansearch.h.html \
		   mdoc.h.html \
		   roff.h.html

WWW_OBJS	 = mdocml.tar.gz \
		   mdocml.sha256

# === USER CONFIGURATION ===============================================

include Makefile.local

# === DEPENDENCY HANDLING ==============================================

all: base-build $(BUILD_TARGETS) Makefile.local

base-build: mandoc demandoc soelim

cgi-build: man.cgi

install: base-install $(INSTALL_TARGETS)

www: $(WWW_OBJS) $(WWW_MANS)

$(WWW_MANS): mandoc

.PHONY: base-install cgi-install db-install install www-install
.PHONY: clean distclean depend

include Makefile.depend

# === TARGETS CONTAINING SHELL COMMANDS ================================

distclean: clean
	rm -f Makefile.local config.h config.h.old config.log config.log.old

clean:
	rm -f libmandoc.a $(LIBMANDOC_OBJS) $(COMPAT_OBJS)
	rm -f mandoc $(BASE_OBJS) $(DB_OBJS)
	rm -f man.cgi $(CGI_OBJS)
	rm -f manpage $(MANPAGE_OBJS)
	rm -f demandoc $(DEMANDOC_OBJS)
	rm -f soelim $(SOELIM_OBJS)
	rm -f $(WWW_MANS) $(WWW_OBJS)
	rm -rf *.dSYM

base-install: base-build
	mkdir -p $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)$(EXAMPLEDIR)
	mkdir -p $(DESTDIR)$(LIBDIR)
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	mkdir -p $(DESTDIR)$(MANDIR)/man1
	mkdir -p $(DESTDIR)$(MANDIR)/man3
	mkdir -p $(DESTDIR)$(MANDIR)/man5
	mkdir -p $(DESTDIR)$(MANDIR)/man7
	$(INSTALL_PROGRAM) mandoc demandoc soelim $(DESTDIR)$(BINDIR)
	ln -f $(DESTDIR)$(BINDIR)/mandoc $(DESTDIR)$(BINDIR)/$(BINM_MAN)
	$(INSTALL_LIB) libmandoc.a $(DESTDIR)$(LIBDIR)
	$(INSTALL_LIB) man.h mandoc.h mandoc_aux.h mdoc.h roff.h \
		$(DESTDIR)$(INCLUDEDIR)
	$(INSTALL_MAN) mandoc.1 demandoc.1 soelim.1 $(DESTDIR)$(MANDIR)/man1
	$(INSTALL_MAN) man.1 $(DESTDIR)$(MANDIR)/man1/$(BINM_MAN).1
	$(INSTALL_MAN) mandoc.3 mandoc_escape.3 mandoc_malloc.3 \
		mchars_alloc.3 tbl.3 $(DESTDIR)$(MANDIR)/man3
	$(INSTALL_MAN) man.conf.5 $(DESTDIR)$(MANDIR)/man5/${MANM_MANCONF}.5
	$(INSTALL_MAN) man.7 $(DESTDIR)$(MANDIR)/man7/${MANM_MAN}.7
	$(INSTALL_MAN) mdoc.7 $(DESTDIR)$(MANDIR)/man7/${MANM_MDOC}.7
	$(INSTALL_MAN) roff.7 $(DESTDIR)$(MANDIR)/man7/${MANM_ROFF}.7
	$(INSTALL_MAN) eqn.7 $(DESTDIR)$(MANDIR)/man7/${MANM_EQN}.7
	$(INSTALL_MAN) tbl.7 $(DESTDIR)$(MANDIR)/man7/${MANM_TBL}.7
	$(INSTALL_MAN) mandoc_char.7 $(DESTDIR)$(MANDIR)/man7
	$(INSTALL_DATA) example.style.css $(DESTDIR)$(EXAMPLEDIR)

db-install: base-build
	mkdir -p $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)$(SBINDIR)
	mkdir -p $(DESTDIR)$(MANDIR)/man1
	mkdir -p $(DESTDIR)$(MANDIR)/man3
	mkdir -p $(DESTDIR)$(MANDIR)/man5
	mkdir -p $(DESTDIR)$(MANDIR)/man8
	ln -f $(DESTDIR)$(BINDIR)/mandoc $(DESTDIR)$(BINDIR)/$(BINM_APROPOS)
	ln -f $(DESTDIR)$(BINDIR)/mandoc $(DESTDIR)$(BINDIR)/$(BINM_WHATIS)
	ln -f $(DESTDIR)$(BINDIR)/mandoc \
		$(DESTDIR)$(SBINDIR)/$(BINM_MAKEWHATIS)
	$(INSTALL_MAN) apropos.1 $(DESTDIR)$(MANDIR)/man1/$(BINM_APROPOS).1
	ln -f $(DESTDIR)$(MANDIR)/man1/$(BINM_APROPOS).1 \
		$(DESTDIR)$(MANDIR)/man1/$(BINM_WHATIS).1
	$(INSTALL_MAN) mansearch.3 $(DESTDIR)$(MANDIR)/man3
	$(INSTALL_MAN) mandoc.db.5 $(DESTDIR)$(MANDIR)/man5
	$(INSTALL_MAN) makewhatis.8 \
		$(DESTDIR)$(MANDIR)/man8/$(BINM_MAKEWHATIS).8

cgi-install: cgi-build
	mkdir -p $(DESTDIR)$(CGIBINDIR)
	mkdir -p $(DESTDIR)$(HTDOCDIR)
	mkdir -p $(DESTDIR)$(WWWPREFIX)/man/mandoc/man1
	mkdir -p $(DESTDIR)$(WWWPREFIX)/man/mandoc/man8
	$(INSTALL_PROGRAM) man.cgi $(DESTDIR)$(CGIBINDIR)
	$(INSTALL_DATA) example.style.css $(DESTDIR)$(HTDOCDIR)/man.css
	$(INSTALL_DATA) man-cgi.css $(DESTDIR)$(HTDOCDIR)
	$(INSTALL_MAN) apropos.1 $(DESTDIR)$(WWWPREFIX)/man/mandoc/man1/
	$(INSTALL_MAN) man.cgi.8 $(DESTDIR)$(WWWPREFIX)/man/mandoc/man8/

Makefile.local config.h: configure ${TESTSRCS}
	@echo "$@ is out of date; please run ./configure"
	@exit 1

libmandoc.a: $(COMPAT_OBJS) $(LIBMANDOC_OBJS)
	$(AR) rs $@ $(COMPAT_OBJS) $(LIBMANDOC_OBJS)

mandoc: $(MAIN_OBJS) libmandoc.a
	$(CC) $(LDFLAGS) -o $@ $(MAIN_OBJS) libmandoc.a $(DBLIB)

manpage: $(MANPAGE_OBJS) libmandoc.a
	$(CC) $(LDFLAGS) -o $@ $(MANPAGE_OBJS) libmandoc.a $(DBLIB)

man.cgi: $(CGI_OBJS) libmandoc.a
	$(CC) $(LDFLAGS) $(STATIC) -o $@ $(CGI_OBJS) libmandoc.a $(DBLIB)

demandoc: $(DEMANDOC_OBJS) libmandoc.a
	$(CC) $(LDFLAGS) -o $@ $(DEMANDOC_OBJS) libmandoc.a

soelim: $(SOELIM_OBJS) compat_reallocarray.o
	$(CC) $(LDFLAGS) -o $@ $(SOELIM_OBJS) compat_reallocarray.o

# --- maintainer targets ---

www-install: www
	mkdir -p $(HTDOCDIR)/snapshots
	$(INSTALL_DATA) $(WWW_MANS) style.css $(HTDOCDIR)
	$(INSTALL_DATA) $(WWW_OBJS) $(HTDOCDIR)/snapshots
	$(INSTALL_DATA) mdocml.tar.gz \
		$(HTDOCDIR)/snapshots/mdocml-$(VERSION).tar.gz
	$(INSTALL_DATA) mdocml.sha256 \
		$(HTDOCDIR)/snapshots/mdocml-$(VERSION).sha256

depend: config.h
	mkdep -f Makefile.depend $(CFLAGS) $(SRCS)
	perl -e 'undef $$/; $$_ = <>; s|/usr/include/\S+||g; \
		s|\\\n||g; s|  +| |g; s| $$||mg; print;' \
		Makefile.depend > Makefile.tmp
	mv Makefile.tmp Makefile.depend

mdocml.sha256: mdocml.tar.gz
	sha256 mdocml.tar.gz > $@

mdocml.tar.gz: $(DISTFILES)
	mkdir -p .dist/mdocml-$(VERSION)/
	$(INSTALL) -m 0644 $(DISTFILES) .dist/mdocml-$(VERSION)
	chmod 755 .dist/mdocml-$(VERSION)/configure
	( cd .dist/ && tar zcf ../$@ mdocml-$(VERSION) )
	rm -rf .dist/

# === SUFFIX RULES =====================================================

.SUFFIXES:	 .1       .3       .5       .7       .8       .h
.SUFFIXES:	 .1.html  .3.html  .5.html  .7.html  .8.html  .h.html

.h.h.html:
	highlight -I $< > $@

.1.1.html .3.3.html .5.5.html .7.7.html .8.8.html: mandoc
	./mandoc -Thtml -Wall,stop \
		-Ostyle=style.css,man=%N.%S.html,includes=%I.html $< > $@
