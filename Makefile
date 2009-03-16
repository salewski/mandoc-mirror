.SUFFIXES:	.html .sgml

VERSION	= 1.5.1
VDATE	= 15 March 2009

BINDIR		= $(PREFIX)/bin
INCLUDEDIR	= $(PREFIX)/include
LIBDIR		= $(PREFIX)/lib
MANDIR		= $(PREFIX)/man

INSTALL_PROGRAM	= install -m 0755
INSTALL_DATA	= install -m 0444
INSTALL_LIB	= install -m 0644
INSTALL_MAN	= $(INSTALL_DATA)

VFLAGS  = -DVERSION=\"$(VERSION)\"
CFLAGS += -W -Wall -Wstrict-prototypes -Wno-unused-parameter -g 

LINTFLAGS += $(VFLAGS)
CFLAGS    += $(VFLAGS)

LIBLNS	= macro.ln mdoc.ln hash.ln strings.ln xstd.ln argv.ln \
	  validate.ln action.ln lib.ln att.ln arch.ln vol.ln \
	  msec.ln st.ln

TREELNS	= mdoctree.ln mmain.ln 

TERMLNS	= mdoctree.ln mmain.ln term.ln ascii.ln

LINTLNS	= mdoclint.ln mmain.ln

LNS	= $(LIBLNS) $(TREELNS) $(TERMLNS)

LLNS	= llib-llibmdoc.ln llib-lmdoctree.ln llib-lmdocterm.ln

LIBS	= libmdoc.a

LIBOBJS	= macro.o mdoc.o hash.o strings.o xstd.o argv.o \
	  validate.o action.o lib.o att.o arch.o vol.o msec.o \
	  st.o

TERMOBJS= mdocterm.o mmain.o term.o ascii.o

TREEOBJS= mdoctree.o mmain.o

LINTOBJS= mdoclint.o mmain.o

OBJS	= $(LIBOBJS) $(TERMOBJS) $(TREEOBJS) $(LINTOBJS)

SRCS	= macro.c mdoc.c hash.c strings.c xstd.c argv.c validate.c \
	  action.c term.c mdoctree.c mdocterm.c mmain.c mdoclint.c

DATAS	= arch.in att.in lib.in msec.in st.in vol.in ascii.in

HEADS	= mdoc.h private.h term.h mmain.h

SGMLS	= index.sgml

HTMLS	= index.html

STATICS	= style.css external.png

TARGZS	= mdocml-$(VERSION).tar.gz mdocml-oport-$(VERSION).tar.gz \
	  mdocml-nport-$(VERSION).tar.gz

MANS	= mdoctree.1 mdocterm.1 mdoclint.1 mdoc.3 mdoc.7

BINS	= mdocterm mdoctree mdoclint

CLEAN	= $(BINS) $(LNS) $(LLNS) $(LIBS) $(OBJS) $(HTMLS) \
	  $(TARGZS) 

INSTALL	= $(SRCS) $(HEADS) Makefile DESCR $(MANS) $(SGMLS) \
	  $(STATICS) Makefile.netbsd Makefile.openbsd $(DATAS)

all:	$(BINS)

lint:	$(LLNS)

clean:
	rm -f $(CLEAN)

cleanlint:
	rm -f $(LNS) $(LLNS)

dist:	mdocml-$(VERSION).tar.gz

port:	mdocml-oport-$(VERSION).tar.gz mdocml-nport-$(VERSION).tar.gz

www:	$(HTMLS) $(TARGZS)

installwww: www
	install -m 0444 $(HTMLS) $(STATICS) $(PREFIX)/
	install -m 0444 mdocml-$(VERSION).tar.gz $(PREFIX)/snapshots/
	install -m 0444 mdocml-$(VERSION).tar.gz $(PREFIX)/snapshots/mdocml.tar.gz
	install -m 0444 mdocml-oport-$(VERSION).tar.gz $(PREFIX)/ports-openbsd/
	install -m 0444 mdocml-oport-$(VERSION).tar.gz $(PREFIX)/ports-openbsd/mdocml.tar.gz
	install -m 0444 mdocml-nport-$(VERSION).tar.gz $(PREFIX)/ports-netbsd/
	install -m 0444 mdocml-nport-$(VERSION).tar.gz $(PREFIX)/ports-netbsd/mdocml.tar.gz

install:
	mkdir -p $(BINDIR)
	mkdir -p $(INCLUDEDIR)
	mkdir -p $(LIBDIR)/lib
	mkdir -p $(MANDIR)/man1
	mkdir -p $(MANDIR)/man3
	$(INSTALL_PROGRAM) mdocterm $(BINDIR)
	$(INSTALL_PROGRAM) mdoctree $(BINDIR)
	$(INSTALL_PROGRAM) mdoclint $(BINDIR)
	$(INSTALL_MAN) mdocterm.1 $(MANDIR)/man1
	$(INSTALL_MAN) mdoctree.1 $(MANDIR)/man1
	$(INSTALL_MAN) mdoclint.1 $(MANDIR)/man1
	$(INSTALL_MAN) mdoc.3 $(MANDIR)/man3
	$(INSTALL_LIB) libmdoc.a $(LIBDIR)
	$(INSTALL_DATA) mdoc.h $(INCLUDEDIR)

uninstall:
	rm -f $(BINDIR)/mdocterm
	rm -f $(BINDIR)/mdoctree
	rm -f $(BINDIR)/mdoclint
	rm -f $(MANDIR)/man1/mdocterm.1
	rm -f $(MANDIR)/man1/mdoctree.1
	rm -f $(MANDIR)/man1/mdoclint.1
	rm -f $(MANDIR)/man3/mdoc.3
	rm -f $(LIBDIR)/libmdoc.a
	rm -f $(INCLUDEDIR)/mdoc.h

lib.ln: lib.c lib.in private.h
lib.o: lib.c lib.in private.h

att.ln: att.c att.in private.h
att.o: att.c att.in private.h

arch.ln: arch.c arch.in private.h
arch.o: arch.c arch.in private.h

vol.ln: vol.c vol.in private.h
vol.o: vol.c vol.in private.h

ascii.ln: ascii.c ascii.in term.h
ascii.o: ascii.c ascii.in term.h

msec.ln: msec.c msec.in private.h
msec.o: msec.c msec.in private.h

st.ln: st.c st.in private.h
st.o: st.c st.in private.h

macro.ln: macro.c private.h
macro.o: macro.c private.h

term.ln: term.c term.h 
term.o: term.c term.h

strings.ln: strings.c private.h
strings.o: strings.c private.h

hash.ln: hash.c private.h
hash.o: hash.c private.h

mdoc.ln: mdoc.c private.h
mdoc.o: mdoc.c private.h

mdocterm.ln: mdocterm.c mmain.h
mdocterm.o: mdocterm.c mmain.h

mdoclint.ln: mdoclint.c mmain.h
mdoclint.o: mdoclint.c mmain.h

mdoctree.ln: mdoctree.c mmain.h
mdoctree.o: mdoctree.c mmain.h

xstd.ln: xstd.c private.h
xstd.o: xstd.c private.h

argv.ln: argv.c private.h
argv.o: argv.c private.h

validate.ln: validate.c private.h
validate.o: validate.c private.h

action.ln: action.c private.h
action.o: action.c private.h

mmain.ln: mmain.c mmain.h
mmain.o: mmain.c mmain.h

private.h: mdoc.h

mmain.h: mdoc.h

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
	echo bin/mdocterm >> .dist/mdocml/PLIST
	echo bin/mdoctree >> .dist/mdocml/PLIST
	echo bin/mdoclint >> .dist/mdocml/PLIST
	echo lib/libmdoc.a >> .dist/mdocml/PLIST
	echo include/mdoc.h >> .dist/mdocml/PLIST
	echo man/man1/mdoctree.1 >> .dist/mdocml/PLIST
	echo man/man1/mdocterm.1 >> .dist/mdocml/PLIST
	echo man/man1/mdoclint.1 >> .dist/mdocml/PLIST
	echo man/man3/mdoc.3 >> .dist/mdocml/PLIST
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
	echo bin/mdocterm >> .dist/mdocml/pkg/PLIST
	echo bin/mdoctree >> .dist/mdocml/pkg/PLIST
	echo bin/mdoclint >> .dist/mdocml/pkg/PLIST
	echo lib/libmdoc.a >> .dist/mdocml/pkg/PLIST
	echo include/mdoc.h >> .dist/mdocml/pkg/PLIST
	echo @man man/man1/mdoctree.1 >> .dist/mdocml/pkg/PLIST
	echo @man man/man1/mdocterm.1 >> .dist/mdocml/pkg/PLIST
	echo @man man/man1/mdoclint.1 >> .dist/mdocml/pkg/PLIST
	echo @man man/man3/mdoc.3 >> .dist/mdocml/pkg/PLIST
	( cd .dist/ && tar zcf ../$@ mdocml/ )
	rm -rf .dist/

mdocml-$(VERSION).tar.gz: $(INSTALL)
	mkdir -p .dist/mdocml/mdocml-$(VERSION)/
	install -m 0644 $(INSTALL) .dist/mdocml/mdocml-$(VERSION)/
	( cd .dist/mdocml/ && tar zcf ../../$@ mdocml-$(VERSION)/ )
	rm -rf .dist/

llib-llibmdoc.ln: $(LIBLNS)
	$(LINT) $(LINTFLAGS) -Clibmdoc $(LIBLNS)

llib-lmdoctree.ln: $(TREELNS) llib-llibmdoc.ln
	$(LINT) $(LINTFLAGS) -Cmdoctree $(TREELNS) llib-llibmdoc.ln

llib-lmdocterm.ln: $(TERMLNS) llib-llibmdoc.ln
	$(LINT) $(LINTFLAGS) -Cmdocterm $(TERMLNS) llib-llibmdoc.ln

libmdoc.a: $(LIBOBJS)
	$(AR) rs $@ $(LIBOBJS)

mdocterm: $(TERMOBJS) libmdoc.a
	$(CC) $(CFLAGS) -o $@ $(TERMOBJS) libmdoc.a 

mdoctree: $(TREEOBJS) libmdoc.a
	$(CC) $(CFLAGS) -o $@ $(TREEOBJS) libmdoc.a 

mdoclint: $(LINTOBJS) libmdoc.a
	$(CC) $(CFLAGS) -o $@ $(LINTOBJS) libmdoc.a 

.sgml.html:
	validate $<
	sed -e "s!@VERSION@!$(VERSION)!" -e "s!@VDATE@!$(VDATE)!" $< > $@

