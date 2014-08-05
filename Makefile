# $Id$
#
# Copyright (c) 2010, 2011, 2012 Kristaps Dzonsons <kristaps@bsd.lv>
# Copyright (c) 2011, 2013, 2014 Ingo Schwarze <schwarze@openbsd.org>
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

VERSION		 = 1.12.4

# === USER SETTINGS ====================================================

# --- user settings relevant for all builds ----------------------------

# Specify this if you want to hard-code the operating system to appear
# in the lower-left hand corner of -mdoc manuals.
#
# CFLAGS	+= -DOSNAME="\"OpenBSD 5.5\""

# IFF your system supports multi-byte functions (setlocale(), wcwidth(),
# putwchar()) AND has __STDC_ISO_10646__ (that is, wchar_t is simply a
# UCS-4 value) should you define USE_WCHAR.  If you define it and your
# system DOESN'T support this, -Tlocale will produce garbage.
# If you don't define it, -Tlocale is a synonym for -Tacsii.
#
CFLAGS	 	+= -DUSE_WCHAR

CFLAGS		+= -g -DHAVE_CONFIG_H
CFLAGS     	+= -W -Wall -Wstrict-prototypes -Wno-unused-parameter -Wwrite-strings
PREFIX		 = /usr/local
BINDIR		 = $(PREFIX)/bin
INCLUDEDIR	 = $(PREFIX)/include/mandoc
LIBDIR		 = $(PREFIX)/lib/mandoc
MANDIR		 = $(PREFIX)/man
EXAMPLEDIR	 = $(PREFIX)/share/examples/mandoc

INSTALL		 = install
INSTALL_PROGRAM	 = $(INSTALL) -m 0555
INSTALL_DATA	 = $(INSTALL) -m 0444
INSTALL_LIB	 = $(INSTALL) -m 0444
INSTALL_SOURCE	 = $(INSTALL) -m 0644
INSTALL_MAN	 = $(INSTALL_DATA)

# --- user settings related to database support ------------------------

# If you want to build without database support, for example to avoid
# the dependency on Berkeley DB, comment the following line.
# However, you won't get apropos(1) and makewhatis(8) in that case.
#
BUILD_TARGETS	+= db-build

# The remaining settings in this section
# are only relevant if db-build is enabled.
# Otherwise, they have no effect either way.

# Non-BSD systems (Linux, etc.) need -ldb to compile mandocdb and
# apropos.
#
#DBLIB		 = -ldb

# If your system has manpath(1), uncomment this.  This is most any
# system that's not OpenBSD or NetBSD.  If uncommented, apropos(1)
# and mandocdb(8) will use manpath(1) to get the MANPATH variable.
#
#CFLAGS		+= -DUSE_MANPATH

WWWPREFIX	 = /var/www
HTDOCDIR	 = $(WWWPREFIX)/htdocs

# === END OF USER SETTINGS =============================================

INSTALL_TARGETS	 = $(BUILD_TARGETS:-build=-install)

BASEBIN		 = mandoc preconv demandoc
DBBIN		 = apropos mandocdb

TESTSRCS	 = test-betoh64.c \
		   test-fgetln.c \
		   test-getsubopt.c \
		   test-mmap.c \
		   test-reallocarray.c \
		   test-strcasestr.c \
		   test-strlcat.c \
		   test-strlcpy.c \
		   test-strptime.c \
		   test-strsep.c

SRCS		 = apropos.c \
		   apropos_db.c \
		   arch.c \
		   att.c \
		   chars.c \
		   compat_fgetln.c \
		   compat_getsubopt.c \
		   compat_reallocarray.c \
		   compat_strcasestr.c \
		   compat_strlcat.c \
		   compat_strlcpy.c \
		   compat_strsep.c \
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
		   manpath.c \
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
		   vol.c \
		   $(TESTSRCS)

DISTFILES	 = LICENSE \
		   Makefile \
		   Makefile.depend \
		   NEWS \
		   TODO \
		   apropos.1 \
		   apropos_db.h \
		   arch.in \
		   att.in \
		   chars.in \
		   config.h.post \
		   config.h.pre \
		   configure \
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
		   man.7 \
		   man.h \
		   mandoc.1 \
		   mandoc.3 \
		   mandoc.h \
		   mandoc_aux.h \
		   mandoc_char.7 \
		   mandoc_html.3 \
		   mandocdb.8 \
		   mandocdb.h \
		   manpath.h \
		   mdoc.7 \
		   mdoc.h \
		   msec.in \
		   out.h \
		   preconv.1 \
		   predefs.in \
		   roff.7 \
		   st.in \
		   style.css \
		   tbl.3 \
		   tbl.7 \
		   term.h \
		   vol.in \
		   whatis.1 \
		   $(SRCS)

LIBMAN_OBJS	 = man.o \
		   man_hash.o \
		   man_macro.o \
		   man_validate.o

LIBMDOC_OBJS	 = arch.o \
		   att.o \
		   lib.o \
		   mdoc.o \
		   mdoc_argv.o \
		   mdoc_hash.o \
		   mdoc_macro.o \
		   mdoc_validate.o \
		   st.o \
		   vol.o

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
		   read.o

COMPAT_OBJS	 = compat_fgetln.o \
		   compat_getsubopt.o \
		   compat_reallocarray.o \
		   compat_strcasestr.o \
		   compat_strlcat.o \
		   compat_strlcpy.o \
		   compat_strsep.o

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

MANDOC_OBJS	 = $(MANDOC_HTML_OBJS) \
		   $(MANDOC_MAN_OBJS) \
		   $(MANDOC_TERM_OBJS) \
		   main.o \
		   out.o \
		   tree.o

MANDOCDB_OBJS	 = mandocdb.o manpath.o

PRECONV_OBJS	 = preconv.o

APROPOS_OBJS	 = apropos.o apropos_db.o manpath.o

DEMANDOC_OBJS	 = demandoc.o

WWW_MANS	 = apropos.1.html \
		   demandoc.1.html \
		   mandoc.1.html \
		   preconv.1.html \
		   whatis.1.html \
		   mandoc.3.html \
		   mandoc_html.3.html \
		   tbl.3.html \
		   eqn.7.html \
		   man.7.html \
		   mandoc_char.7.html \
		   mdoc.7.html \
		   roff.7.html \
		   tbl.7.html \
		   mandocdb.8.html \
		   man.h.html \
		   mandoc.h.html \
		   mandoc_aux.h.html \
		   manpath.h.html \
		   mdoc.h.html

WWW_OBJS	 = mdocml.tar.gz \
		   mdocml.sha256

# === DEPENDENCY HANDLING ==============================================

all: base-build $(BUILD_TARGETS)

base-build: $(BASEBIN)

db-build: $(DBBIN)

install: base-install $(INSTALL_TARGETS)

www: $(WWW_OBJS) $(WWW_MANS)

.include "Makefile.depend"

# === TARGETS CONTAINING SHELL COMMANDS ================================

clean:
	rm -f libmandoc.a $(LIBMANDOC_OBJS)
	rm -f apropos $(APROPOS_OBJS)
	rm -f mandocdb $(MANDOCDB_OBJS)
	rm -f preconv $(PRECONV_OBJS)
	rm -f demandoc $(DEMANDOC_OBJS)
	rm -f mandoc $(MANDOC_OBJS)
	rm -f config.h config.log $(COMPAT_OBJS)
	rm -f $(WWW_MANS) $(WWW_OBJS)
	rm -rf *.dSYM

base-install: base-build
	mkdir -p $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)$(EXAMPLEDIR)
	mkdir -p $(DESTDIR)$(LIBDIR)
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	mkdir -p $(DESTDIR)$(MANDIR)/man1
	mkdir -p $(DESTDIR)$(MANDIR)/man3
	mkdir -p $(DESTDIR)$(MANDIR)/man7
	$(INSTALL_PROGRAM) $(BASEBIN) $(DESTDIR)$(BINDIR)
	$(INSTALL_LIB) libmandoc.a $(DESTDIR)$(LIBDIR)
	$(INSTALL_LIB) man.h mandoc.h mandoc_aux.h mdoc.h \
		$(DESTDIR)$(INCLUDEDIR)
	$(INSTALL_MAN) mandoc.1 preconv.1 demandoc.1 $(DESTDIR)$(MANDIR)/man1
	$(INSTALL_MAN) mandoc.3 tbl.3 $(DESTDIR)$(MANDIR)/man3
	$(INSTALL_MAN) man.7 mdoc.7 roff.7 eqn.7 tbl.7 mandoc_char.7 \
		$(DESTDIR)$(MANDIR)/man7
	$(INSTALL_DATA) example.style.css $(DESTDIR)$(EXAMPLEDIR)

db-install: db-build
	mkdir -p $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)$(MANDIR)/man1
	mkdir -p $(DESTDIR)$(MANDIR)/man8
	$(INSTALL_PROGRAM) $(DBBIN) $(DESTDIR)$(BINDIR)
	ln -f $(DESTDIR)$(BINDIR)/apropos $(DESTDIR)$(BINDIR)/whatis
	$(INSTALL_MAN) apropos.1 whatis.1 $(DESTDIR)$(MANDIR)/man1
	$(INSTALL_MAN) mandocdb.8 $(DESTDIR)$(MANDIR)/man8

www-install: www
	mkdir -p $(DESTDIR)$(HTDOCDIR)/snapshots
	$(INSTALL_DATA) $(WWW_MANS) style.css $(DESTDIR)$(HTDOCDIR)
	$(INSTALL_DATA) $(WWW_OBJS) $(DESTDIR)$(HTDOCDIR)/snapshots
	$(INSTALL_DATA) mdocml.tar.gz \
		$(DESTDIR)$(HTDOCDIR)/snapshots/mdocml-$(VERSION).tar.gz
	$(INSTALL_DATA) mdocml.sha256 \
		$(DESTDIR)$(HTDOCDIR)/snapshots/mdocml-$(VERSION).sha256

Makefile.depend: $(SRCS) config.h Makefile
	mkdep -f Makefile.depend $(CFLAGS) $(SRCS)
	perl -e 'undef $$/; $$_ = <>; s|/usr/include/\S+||g; \
		s|\\\n||g; s|  +| |g; print;' Makefile.depend > Makefile.tmp
	mv Makefile.tmp Makefile.depend

libmandoc.a: $(COMPAT_OBJS) $(LIBMANDOC_OBJS)
	$(AR) rs $@ $(COMPAT_OBJS) $(LIBMANDOC_OBJS)

mandoc: $(MANDOC_OBJS) libmandoc.a
	$(CC) $(LDFLAGS) -o $@ $(MANDOC_OBJS) libmandoc.a

mandocdb: $(MANDOCDB_OBJS) libmandoc.a
	$(CC) $(LDFLAGS) -o $@ $(MANDOCDB_OBJS) libmandoc.a $(DBLIB)

preconv: $(PRECONV_OBJS)
	$(CC) $(LDFLAGS) -o $@ $(PRECONV_OBJS)

apropos: $(APROPOS_OBJS) libmandoc.a
	$(CC) $(LDFLAGS) -o $@ $(APROPOS_OBJS) libmandoc.a $(DBLIB)

demandoc: $(DEMANDOC_OBJS) libmandoc.a
	$(CC) $(LDFLAGS) -o $@ $(DEMANDOC_OBJS) libmandoc.a

mdocml.sha256: mdocml.tar.gz
	sha256 mdocml.tar.gz > $@

mdocml.tar.gz: $(DISTFILES)
	mkdir -p .dist/mdocml-$(VERSION)/
	$(INSTALL_SOURCE) $(DISTFILES) .dist/mdocml-$(VERSION)
	chmod 755 .dist/mdocml-$(VERSION)/configure
	( cd .dist/ && tar zcf ../$@ mdocml-$(VERSION) )
	rm -rf .dist/

config.h: configure config.h.pre config.h.post $(TESTSRCS)
	rm -f config.log
	CC="$(CC)" CFLAGS="$(CFLAGS)" VERSION="$(VERSION)" ./configure

.PHONY: 	 base-install clean db-install install www-install
.SUFFIXES:	 .1       .3       .5       .7       .8       .h
.SUFFIXES:	 .1.html  .3.html  .5.html  .7.html  .8.html  .h.html

.h.h.html:
	highlight -I $< > $@

.1.1.html .3.3.html .5.5.html .7.7.html .8.8.html: mandoc
	./mandoc -Thtml -Wall,stop \
		-Ostyle=style.css,man=%N.%S.html,includes=%I.html $< > $@
