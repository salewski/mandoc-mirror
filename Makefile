.SUFFIXES:	.html .sgml

VERSION	= 1.3.5
VDATE	= 25 February 2009

CFLAGS += -W -Wall -Wstrict-prototypes -Wno-unused-parameter -g 

LIBLNS	= macro.ln mdoc.ln hash.ln strings.ln xstd.ln argv.ln \
	  validate.ln action.ln 

TREELNS	= mdoctree.ln mmain.ln 

TERMLNS	= mdoctree.ln mmain.ln term.ln

LINTLNS	= mdoclint.ln mmain.ln

LNS	= $(LIBLNS) $(TREELNS) $(TERMLNS)

LLNS	= llib-llibmdoc.ln llib-lmdoctree.ln llib-lmdocterm.ln

LIBS	= libmdoc.a

LIBOBJS	= macro.o mdoc.o hash.o strings.o xstd.o argv.o \
	  validate.o action.o

TERMOBJS= mdocterm.o mmain.o term.o

TREEOBJS= mdoctree.o mmain.o

LINTOBJS= mdoclint.o mmain.o

OBJS	= $(LIBOBJS) $(TERMOBJS) $(TREEOBJS)

SRCS	= macro.c mdoc.c hash.c strings.c xstd.c argv.c validate.c \
	  action.c term.c mdoctree.c mdocterm.c mmain.c mdoclint.c

HEADS	= mdoc.h private.h term.h mmain.h

SGMLS	= index.sgml

HTMLS	= index.html

STATICS	= style.css external.png

TARGZS	= mdocml-$(VERSION).tar.gz mdocml-oport-$(VERSION).tar.gz

MANS	= mdoctree.1 mdocterm.1 mdoclint.1 mdoc.3

BINS	= mdocterm mdoctree mdoclint

CLEAN	= $(BINS) $(LNS) $(LLNS) $(LIBS) $(OBJS) $(HTMLS) \
	  $(TARGZS)

INSTALL	= $(SRCS) $(HEADS) Makefile Makefile.port DESCR $(MANS) \
	  $(SGMLS) $(STATICS)

FAIL	= regress/test.empty \
	  regress/test.prologue.00 \
	  regress/test.prologue.01 \
	  regress/test.prologue.02 \
	  regress/test.prologue.03 \
	  regress/test.prologue.04 \
	  regress/test.prologue.06 \
	  regress/test.prologue.13 \
	  regress/test.prologue.15 \
	  regress/test.prologue.16 \
	  regress/test.prologue.18 \
	  regress/test.prologue.19 \
	  regress/test.prologue.21 \
	  regress/test.prologue.22 \
	  regress/test.prologue.23 \
	  regress/test.prologue.24 \
	  regress/test.prologue.25 \
	  regress/test.prologue.26 \
	  regress/test.prologue.27 \
	  regress/test.prologue.28 \
	  regress/test.prologue.29 \
	  regress/test.prologue.30 \
	  regress/test.prologue.31 \
	  regress/test.prologue.32 \
	  regress/test.prologue.33 \
	  regress/test.sh.03 \
	  regress/test.escape.01 \
	  regress/test.escape.02 \
	  regress/test.escape.03 \
	  regress/test.escape.04 \
	  regress/test.escape.06 \
	  regress/test.escape.07 \
	  regress/test.escape.08 \
	  regress/test.escape.09

SUCCEED	= regress/test.prologue.05 \
	  regress/test.prologue.07 \
	  regress/test.prologue.08 \
	  regress/test.prologue.09 \
	  regress/test.prologue.10 \
	  regress/test.prologue.11 \
	  regress/test.prologue.12 \
	  regress/test.prologue.14 \
	  regress/test.prologue.17 \
	  regress/test.prologue.20 \
	  regress/test.sh.00 \
	  regress/test.name.00 \
	  regress/test.name.01 \
	  regress/test.name.02 \
	  regress/test.name.03 \
	  regress/test.list.00 \
	  regress/test.list.01 \
	  regress/test.list.02 \
	  regress/test.list.03 \
	  regress/test.list.04 \
	  regress/test.list.05 \
	  regress/test.list.06 \
	  regress/test.sh.01 \
	  regress/test.sh.02 \
	  regress/test.escape.00 \
	  regress/test.escape.05

REGRESS	= $(FAIL) $(SUCCEED)

all:	$(BINS)

lint:	$(LLNS)

clean:
	rm -f $(CLEAN)

dist:	mdocml-$(VERSION).tar.gz

port:	mdocml-oport-$(VERSION).tar.gz

www:	$(HTMLS) $(TARGZS)

installwww: www
	install -m 0444 $(HTMLS) $(STATICS) $(PREFIX)/
	install -m 0444 mdocml-$(VERSION).tar.gz $(PREFIX)/snapshots/
	install -m 0444 mdocml-oport-$(VERSION).tar.gz $(PREFIX)/ports-openbsd/
	install -m 0444 mdocml-$(VERSION).tar.gz $(PREFIX)/snapshots/mdocml.tar.gz
	install -m 0444 mdocml-oport-$(VERSION).tar.gz $(PREFIX)/ports-openbsd/mdocml.tar.gz

regress::
	@for f in $(FAIL); do \
		echo "./mdoclint $$f" ; \
		./mdoclint $$f 2>/dev/null || continue ; exit 1 ; done
	@for f in $(SUCCEED); do \
		echo "./mdoclint $$f" ; \
		./mdoclint $$f 2>/dev/null || exit 1 ; done

install:
	mkdir -p $(PREFIX)/bin/
	mkdir -p $(PREFIX)/include/mdoc/
	mkdir -p $(PREFIX)/lib/
	mkdir -p $(PREFIX)/man/man1/
	install -m 0755 mdocterm $(PREFIX)/bin/
	install -m 0755 mdoctree $(PREFIX)/bin/
	install -m 0755 mdoclint $(PREFIX)/bin/
	install -m 0444 mdocterm.1 $(PREFIX)/man/man1/
	install -m 0444 mdoctree.1 $(PREFIX)/man/man1/
	install -m 0444 mdoclint.1 $(PREFIX)/man/man1/
	install -m 0444 mdoc.3 $(PREFIX)/man/man3/
	install -m 0644 libmdoc.a $(PREFIX)/lib/
	install -m 0444 mdoc.h $(PREFIX)/include/

uninstall:
	rm -f $(PREFIX)/bin/mdocterm
	rm -f $(PREFIX)/bin/mdoctree
	rm -f $(PREFIX)/bin/mdoclint
	rm -f $(PREFIX)/man/man1/mdocterm.1
	rm -f $(PREFIX)/man/man1/mdoctree.1
	rm -f $(PREFIX)/man/man1/mdoclint.1
	rm -f $(PREFIX)/man/man3/mdoc.3
	rm -f $(PREFIX)/lib/libmdoc.a
	rm -f $(PREFIX)/include/mdoc.h

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

mdocml-oport-$(VERSION).tar.gz: mdocml-$(VERSION).tar.gz Makefile.port DESCR
	mkdir -p .dist/mdocml/pkg
	sed -e "s!@VERSION@!$(VERSION)!" Makefile.port > \
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
	mkdir -p .dist/mdocml/mdocml-$(VERSION)/regress/
	install -m 0644 $(INSTALL) .dist/mdocml/mdocml-$(VERSION)/
	install -m 0644 $(REGRESS) .dist/mdocml/mdocml-$(VERSION)/regress/
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

