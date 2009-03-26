.SUFFIXES:	.html .sgml

BINDIR		= $(PREFIX)/bin
INCLUDEDIR	= $(PREFIX)/include
LIBDIR		= $(PREFIX)/lib
MANDIR		= $(PREFIX)/man
INSTALL_PROGRAM	= install -m 0755
INSTALL_DATA	= install -m 0444
INSTALL_LIB	= install -m 0644
INSTALL_MAN	= $(INSTALL_DATA)

VERSION	   = 1.7.3
VDATE	   = 24 March 2009

VFLAGS     = -DVERSION=\"$(VERSION)\"
CFLAGS    += -W -Wall -Wstrict-prototypes -Wno-unused-parameter -g
LINTFLAGS += $(VFLAGS)
CFLAGS    += $(VFLAGS)

MDOCLNS	   = mdoc_macro.ln mdoc.ln mdoc_hash.ln strings.ln xstd.ln \
	     argv.ln mdoc_validate.ln mdoc_action.ln lib.ln att.ln \
	     arch.ln vol.ln msec.ln st.ln
MDOCOBJS   = mdoc_macro.o mdoc.o mdoc_hash.o strings.o xstd.o argv.o \
	     mdoc_validate.o mdoc_action.o lib.o att.o arch.o vol.o \
	     msec.o st.o
MDOCSRCS   = mdoc_macro.c mdoc.c mdoc_hash.c strings.c xstd.c argv.c \
	     mdoc_validate.c mdoc_action.c lib.c att.c arch.c vol.c \
	     msec.c st.c

MANLNS	   = man_macro.ln man.ln man_hash.ln man_validate.ln \
	     man_action.ln
MANOBJS	   = man_macro.o man.o man_hash.o man_validate.o \
	     man_action.o
MANSRCS	   = man_macro.c man.c man_hash.c man_validate.c \
	     man_action.c

MAINLNS	   = main.ln term.ln ascii.ln terminal.ln tree.ln compat.ln
MAINOBJS   = main.o term.o ascii.o terminal.o tree.o compat.o
MAINSRCS   = main.c term.c ascii.c terminal.c tree.c compat.c

LLNS	   = llib-llibmdoc.ln llib-llibman.ln llib-lmandoc.ln
LNS	   = $(MAINLNS) $(MDOCLNS) $(MANLNS)
LIBS	   = libmdoc.a libman.a
OBJS	   = $(MDOCOBJS) $(MAINOBJS) $(MANOBJS)
SRCS	   = $(MDOCSRCS) $(MAINSRCS) $(MANSRCS)
DATAS	   = arch.in att.in lib.in msec.in st.in vol.in ascii.in
HEADS	   = mdoc.h libmdoc.h man.h libman.h term.h 
SGMLS	   = index.sgml 
HTMLS	   = index.html
STATICS	   = style.css external.png
TARGZS	   = mdocml-$(VERSION).tar.gz \
	     mdocml-oport-$(VERSION).tar.gz \
	     mdocml-fport-$(VERSION).tar.gz \
	     mdocml-nport-$(VERSION).tar.gz
MANS	   = mandoc.1 mdoc.3 mdoc.7 manuals.7
BINS	   = mandoc
CLEAN	   = $(BINS) $(LNS) $(LLNS) $(LIBS) $(OBJS) $(HTMLS) $(TARGZS) 
MAKEFILES  = Makefile.netbsd Makefile.openbsd Makefile.freebsd \
	     Makefile
INSTALL	   = $(SRCS) $(HEADS) $(MAKEFILES) DESCR $(MANS) $(SGMLS) \
	     $(STATICS) $(DATAS)

all:	$(BINS)

lint:	$(LLNS)

clean:
	rm -f $(CLEAN)

cleanlint:
	rm -f $(LNS) $(LLNS)

dist:	mdocml-$(VERSION).tar.gz

port:	mdocml-oport-$(VERSION).tar.gz \
	mdocml-fport-$(VERSION).tar.gz \
	mdocml-nport-$(VERSION).tar.gz

www:	$(HTMLS) $(TARGZS)

installwww: www
	install -m 0444 $(HTMLS) $(STATICS) $(PREFIX)/
	install -m 0444 mdocml-$(VERSION).tar.gz $(PREFIX)/snapshots/
	install -m 0444 mdocml-$(VERSION).tar.gz $(PREFIX)/snapshots/mdocml.tar.gz
	install -m 0444 mdocml-oport-$(VERSION).tar.gz $(PREFIX)/ports-openbsd/
	install -m 0444 mdocml-oport-$(VERSION).tar.gz $(PREFIX)/ports-openbsd/mdocml.tar.gz
	install -m 0444 mdocml-nport-$(VERSION).tar.gz $(PREFIX)/ports-netbsd/
	install -m 0444 mdocml-nport-$(VERSION).tar.gz $(PREFIX)/ports-netbsd/mdocml.tar.gz
	install -m 0444 mdocml-fport-$(VERSION).tar.gz $(PREFIX)/ports-freebsd/
	install -m 0444 mdocml-fport-$(VERSION).tar.gz $(PREFIX)/ports-freebsd/mdocml.tar.gz

install:
	mkdir -p $(BINDIR)
	mkdir -p $(MANDIR)/man1
	mkdir -p $(MANDIR)/man7
	$(INSTALL_PROGRAM) mandoc $(BINDIR)
	$(INSTALL_MAN) mandoc.1 $(MANDIR)/man1
	$(INSTALL_MAN) mdoc.7 $(MANDIR)/man7

uninstall:
	rm -f $(BINDIR)/mandoc
	rm -f $(MANDIR)/man1/mandoc.1
	rm -f $(MANDIR)/man7/mdoc.7

man_macro.ln: man_macro.c libman.h
man_macro.o: man_macro.c libman.h

lib.ln: lib.c lib.in libmdoc.h
lib.o: lib.c lib.in libmdoc.h

att.ln: att.c att.in libmdoc.h
att.o: att.c att.in libmdoc.h

arch.ln: arch.c arch.in libmdoc.h
arch.o: arch.c arch.in libmdoc.h

vol.ln: vol.c vol.in libmdoc.h
vol.o: vol.c vol.in libmdoc.h

ascii.ln: ascii.c ascii.in term.h
ascii.o: ascii.c ascii.in term.h

msec.ln: msec.c msec.in libmdoc.h
msec.o: msec.c msec.in libmdoc.h

st.ln: st.c st.in libmdoc.h
st.o: st.c st.in libmdoc.h

mdoc_macro.ln: mdoc_macro.c libmdoc.h
mdoc_macro.o: mdoc_macro.c libmdoc.h

term.ln: term.c term.h 
term.o: term.c term.h

strings.ln: strings.c libmdoc.h
strings.o: strings.c libmdoc.h

man_hash.ln: man_hash.c libman.h
man_hash.o: man_hash.c libman.h

mdoc_hash.ln: mdoc_hash.c libmdoc.h
mdoc_hash.o: mdoc_hash.c libmdoc.h

mdoc.ln: mdoc.c libmdoc.h
mdoc.o: mdoc.c libmdoc.h

man.ln: man.c libman.h
man.o: man.c libman.h

main.ln: main.c mdoc.h
main.o: main.c mdoc.h

terminal.ln: terminal.c term.h
terminal.o: terminal.c term.h

xstd.ln: xstd.c libmdoc.h
xstd.o: xstd.c libmdoc.h

argv.ln: argv.c libmdoc.h
argv.o: argv.c libmdoc.h

man_validate.ln: man_validate.c libman.h
man_validate.o: man_validate.c libman.h

mdoc_validate.ln: mdoc_validate.c libmdoc.h
mdoc_validate.o: mdoc_validate.c libmdoc.h

mdoc_action.ln: mdoc_action.c libmdoc.h
mdoc_action.o: mdoc_action.c libmdoc.h

libmdoc.h: mdoc.h

term.h: mdoc.h

mdocml-nport-$(VERSION).tar.gz: mdocml-$(VERSION).tar.gz Makefile.netbsd DESCR
	mkdir -p .dist/mdocml/
	sed -e "s!@VERSION@!$(VERSION)!" Makefile.netbsd > \
		.dist/mdocml/Makefile
	md5 mdocml-$(VERSION).tar.gz > .dist/mdocml/distinfo
	rmd160 mdocml-$(VERSION).tar.gz >> .dist/mdocml/distinfo
	sha1 mdocml-$(VERSION).tar.gz >> .dist/mdocml/distinfo
	install -m 0644 DESCR .dist/mdocml/
	echo @comment $$NetBSD$$ > .dist/mdocml/PLIST
	echo bin/mandoc >> .dist/mdocml/PLIST
	echo man/man1/mandoc.1 >> .dist/mdocml/PLIST
	echo man/man7/mdoc.7 >> .dist/mdocml/PLIST
	( cd .dist/ && tar zcf ../$@ mdocml/ )
	rm -rf .dist/

mdocml-oport-$(VERSION).tar.gz: mdocml-$(VERSION).tar.gz Makefile.openbsd DESCR
	mkdir -p .dist/mdocml/pkg
	sed -e "s!@VERSION@!$(VERSION)!" Makefile.openbsd > \
		.dist/mdocml/Makefile
	md5 mdocml-$(VERSION).tar.gz > .dist/mdocml/distinfo
	rmd160 mdocml-$(VERSION).tar.gz >> .dist/mdocml/distinfo
	sha1 mdocml-$(VERSION).tar.gz >> .dist/mdocml/distinfo
	install -m 0644 DESCR .dist/mdocml/pkg/DESCR
	echo @comment $$OpenBSD$$ > .dist/mdocml/pkg/PLIST
	echo bin/mandoc >> .dist/mdocml/pkg/PLIST
	echo @man man/man1/mandoc.1 >> .dist/mdocml/pkg/PLIST
	echo @man man/man7/mdoc.7 >> .dist/mdocml/pkg/PLIST
	( cd .dist/ && tar zcf ../$@ mdocml/ )
	rm -rf .dist/

mdocml-fport-$(VERSION).tar.gz: mdocml-$(VERSION).tar.gz Makefile.freebsd DESCR
	mkdir -p .dist/mdocml
	sed -e "s!@VERSION@!$(VERSION)!" Makefile.freebsd > \
		.dist/mdocml/Makefile
	( md5 mdocml-$(VERSION).tar.gz; \
	  cksum -a SHA256 mdocml-$(VERSION).tar.gz; \
	  echo -n "SIZE (mdocml-$(VERSION).tar.gz) = "; \
	  ls -l mdocml-$(VERSION).tar.gz | awk '{print $$5}' \
	  ) > .dist/mdocml/distinfo
	install -m 0644 DESCR .dist/mdocml/pkg-descr
	( echo; echo "WWW: http://mdocml.bsd.lv/") >> .dist/mdocml/pkg-descr
	( cd .dist/ && tar zcf ../$@ mdocml/ )
	rm -rf .dist/

mdocml-$(VERSION).tar.gz: $(INSTALL)
	mkdir -p .dist/mdocml/mdocml-$(VERSION)/
	install -m 0644 $(INSTALL) .dist/mdocml/mdocml-$(VERSION)/
	( cd .dist/mdocml/ && tar zcf ../../$@ mdocml-$(VERSION)/ )
	rm -rf .dist/

llib-llibmdoc.ln: $(MDOCLNS)
	$(LINT) $(LINTFLAGS) -Clibmdoc $(MDOCLNS)

llib-llibman.ln: $(MANLNS)
	$(LINT) $(LINTFLAGS) -Clibman $(MANLNS)

llib-lmandoc.ln: $(MAINLNS) llib-llibmdoc.ln
	$(LINT) $(LINTFLAGS) -Cmandoc $(MAINLNS) llib-llibmdoc.ln

libmdoc.a: $(MDOCOBJS)
	$(AR) rs $@ $(MDOCOBJS)

libman.a: $(MANOBJS)
	$(AR) rs $@ $(MANOBJS)

mandoc: $(MAINOBJS) libmdoc.a libman.a
	$(CC) $(CFLAGS) -o $@ $(MAINOBJS) libmdoc.a libman.a

.sgml.html:
	validate $<
	sed -e "s!@VERSION@!$(VERSION)!" -e "s!@VDATE@!$(VDATE)!" $< > $@
