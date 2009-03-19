.SUFFIXES:	.html .sgml

BINDIR		= $(PREFIX)/bin
INCLUDEDIR	= $(PREFIX)/include
LIBDIR		= $(PREFIX)/lib
MANDIR		= $(PREFIX)/man
INSTALL_PROGRAM	= install -m 0755
INSTALL_DATA	= install -m 0444
INSTALL_LIB	= install -m 0644
INSTALL_MAN	= $(INSTALL_DATA)

VERSION	   = 1.5.5
VDATE	   = 19 March 2009

VFLAGS     = -DVERSION=\"$(VERSION)\"
CFLAGS    += -W -Wall -Wstrict-prototypes -Wno-unused-parameter -g 
LINTFLAGS += $(VFLAGS)
CFLAGS    += $(VFLAGS)
LIBLNS	   = macro.ln mdoc.ln hash.ln strings.ln xstd.ln argv.ln \
	     validate.ln action.ln lib.ln att.ln arch.ln vol.ln \
	     msec.ln st.ln
LIBOBJS	   = macro.o mdoc.o hash.o strings.o xstd.o argv.o validate.o \
	     action.o lib.o att.o arch.o vol.o msec.o st.o
TERMLNS	   = mdocterm.ln term.ln ascii.ln
TERMOBJS   = mdocterm.o term.o ascii.o
LLNS	   = llib-llibmdoc.ln llib-lmdocterm.ln
LNS	   = $(TERMLNS) $(LIBLNS)
LIBS	   = libmdoc.a
OBJS	   = $(LIBOBJS) $(TERMOBJS)
SRCS	   = macro.c mdoc.c hash.c strings.c xstd.c argv.c validate.c \
	     action.c term.c mdocterm.c lib.c att.c arch.c vol.c \
	     msec.c st.c ascii.c
DATAS	   = arch.in att.in lib.in msec.in st.in vol.in ascii.in
HEADS	   = mdoc.h private.h term.h 
SGMLS	   = index.sgml
HTMLS	   = index.html
STATICS	   = style.css external.png
TARGZS	   = mdocml-$(VERSION).tar.gz mdocml-oport-$(VERSION).tar.gz \
	     mdocml-nport-$(VERSION).tar.gz
MANS	   = mdocterm.1 mdoc.3 mdoc.7
BINS	   = mdocterm 
CLEAN	   = $(BINS) $(LNS) $(LLNS) $(LIBS) $(OBJS) $(HTMLS) $(TARGZS) 
INSTALL	   = $(SRCS) $(HEADS) Makefile DESCR $(MANS) $(SGMLS) \
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
	mkdir -p $(MANDIR)/man7
	$(INSTALL_PROGRAM) mdocterm $(BINDIR)
	$(INSTALL_MAN) mdocterm.1 $(MANDIR)/man1
	$(INSTALL_MAN) mdoc.3 $(MANDIR)/man3
	$(INSTALL_MAN) mdoc.7 $(MANDIR)/man7
	$(INSTALL_LIB) libmdoc.a $(LIBDIR)
	$(INSTALL_DATA) mdoc.h $(INCLUDEDIR)

uninstall:
	rm -f $(BINDIR)/mdocterm
	rm -f $(MANDIR)/man1/mdocterm.1
	rm -f $(MANDIR)/man3/mdoc.3
	rm -f $(MANDIR)/man7/mdoc.7
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

mdocterm.ln: mdocterm.c
mdocterm.o: mdocterm.c

xstd.ln: xstd.c private.h
xstd.o: xstd.c private.h

argv.ln: argv.c private.h
argv.o: argv.c private.h

validate.ln: validate.c private.h
validate.o: validate.c private.h

action.ln: action.c private.h
action.o: action.c private.h

private.h: mdoc.h

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
	echo lib/libmdoc.a >> .dist/mdocml/PLIST
	echo include/mdoc.h >> .dist/mdocml/PLIST
	echo man/man1/mdocterm.1 >> .dist/mdocml/PLIST
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
	echo lib/libmdoc.a >> .dist/mdocml/pkg/PLIST
	echo include/mdoc.h >> .dist/mdocml/pkg/PLIST
	echo @man man/man1/mdocterm.1 >> .dist/mdocml/pkg/PLIST
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

llib-lmdocterm.ln: $(TERMLNS) llib-llibmdoc.ln
	$(LINT) $(LINTFLAGS) -Cmdocterm $(TERMLNS) llib-llibmdoc.ln

libmdoc.a: $(LIBOBJS)
	$(AR) rs $@ $(LIBOBJS)

mdocterm: $(TERMOBJS) libmdoc.a
	$(CC) $(CFLAGS) -o $@ $(TERMOBJS) libmdoc.a 

.sgml.html:
	validate $<
	sed -e "s!@VERSION@!$(VERSION)!" -e "s!@VDATE@!$(VDATE)!" $< > $@

