VERSION	= 1.2.0

CFLAGS += -W -Wall -Wno-unused-parameter -g 

LIBLNS	= macro.ln mdoc.ln hash.ln strings.ln xstd.ln argv.ln \
	  validate.ln action.ln

BINLNS	= tree.ln mdocml.ln

LNS	= $(LIBLNS) $(BINLNS)

LLNS	= llib-llibmdoc.ln llib-lmdocml.ln

LIBS	= libmdoc.a

LIBOBJS	= macro.o mdoc.o hash.o strings.o xstd.o argv.o \
	  validate.o action.o

BINOBJS	= tree.o mdocml.o

OBJS	= $(LIBOBJS) $(BINOBJS)

SRCS	= macro.c mdoc.c mdocml.c hash.c strings.c xstd.c argv.c \
	  validate.c action.c tree.c

HEADS	= mdoc.h private.h

MANS	= mdocml.1 mdoc.3

BINS	= mdocml

CLEAN	= $(BINS) $(LNS) $(LLNS) $(LIBS) $(OBJS)

INSTALL	= $(SRCS) $(HEADS) Makefile Makefile.port DESCR $(MANS)

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
	  regress/test.sh.01 \
	  regress/test.sh.02 \
	  regress/test.sh.03 \
	  regress/test.name.01 \
	  regress/test.name.02 \
	  regress/test.name.03

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
	  regress/test.list.00 \
	  regress/test.list.01 \
	  regress/test.list.02 \
	  regress/test.list.03 \
	  regress/test.list.04 \
	  regress/test.list.05 \
	  regress/test.list.06

all:	$(BINS)

lint:	$(LLNS)

clean:
	rm -f $(CLEAN)

dist:	mdocml-$(VERSION).tar.gz

port:	mdocml-oport-$(VERSION).tar.gz

regress::
	@for f in $(FAIL); do \
		echo "./mdocml $$f" ; \
		./mdocml $$f 2>/dev/null || continue ; exit 1 ; done
	@for f in $(SUCCEED); do \
		echo "./mdocml $$f" ; \
		./mdocml $$f 2>/dev/null || exit 1 ; done

install:
	mkdir -p $(PREFIX)/bin/
	mkdir -p $(PREFIX)/include/mdoc/
	mkdir -p $(PREFIX)/lib/
	mkdir -p $(PREFIX)/man/man1/
	install -m 0755 mdocml $(PREFIX)/bin/
	install -m 0444 mdocml.1 $(PREFIX)/man/man1/
	install -m 0444 mdoc.3 $(PREFIX)/man/man3/
	install -m 0644 libmdoc.a $(PREFIX)/lib/
	install -m 0444 mdoc.h $(PREFIX)/include/

install-dist: mdocml-$(VERSION).tar.gz mdocml-oport-$(VERSION).tar.gz
	install -m 0644 mdocml-$(VERSION).tar.gz $(PREFIX)/
	install -m 0644 mdocml-$(VERSION).tar.gz $(PREFIX)/mdocml.tar.gz
	install -m 0644 mdocml-oport-$(VERSION).tar.gz $(PREFIX)/
	install -m 0644 mdocml-oport-$(VERSION).tar.gz $(PREFIX)/mdocml-oport.tar.gz

uninstall:
	rm -f $(PREFIX)/bin/mdocml
	rm -f $(PREFIX)/man/man1/mdocml.1
	rm -f $(PREFIX)/man/man3/mdoc.3
	rm -f $(PREFIX)/lib/libmdoc.a
	rm -f $(PREFIX)/include/mdoc.h

macro.ln: macro.c private.h

macro.o: macro.c private.h

strings.ln: strings.c private.h

strings.o: strings.c private.h

tree.ln: tree.c mdoc.h

tree.o: tree.c mdoc.h

hash.ln: hash.c private.h

hash.o: hash.c private.h

mdoc.ln: mdoc.c private.h

mdoc.o: mdoc.c private.h

mdocml.ln: mdocml.c mdoc.h

mdocml.o: mdocml.c mdoc.h

xstd.ln: xstd.c private.h

xstd.o: xstd.c private.h

argv.ln: argv.c private.h

argv.o: argv.c private.h

validate.ln: validate.c private.h

validate.o: validate.c private.h

action.ln: action.c private.h

action.o: action.c private.h

private.h: mdoc.h

mdocml-oport-$(VERSION).tar.gz: Makefile.port DESCR
	mkdir -p .dist/mdocml/pkg
	sed -e "s!@VERSION@!$(VERSION)!" Makefile.port > .dist/mdocml/Makefile
	md5 mdocml-$(VERSION).tar.gz > .dist/mdocml/distinfo
	rmd160 mdocml-$(VERSION).tar.gz >> .dist/mdocml/distinfo
	sha1 mdocml-$(VERSION).tar.gz >> .dist/mdocml/distinfo
	install -m 0644 DESCR .dist/mdocml/pkg/DESCR
	echo @comment $$OpenBSD$$ > .dist/mdocml/pkg/PLIST
	echo bin/mdocml >> .dist/mdocml/pkg/PLIST
	echo lib/libmdoc.a >> .dist/mdocml/pkg/PLIST
	echo include/mdoc.h >> .dist/mdocml/pkg/PLIST
	echo @man man/man1/mdocml.1 >> .dist/mdocml/pkg/PLIST
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

llib-lmdocml.ln: $(BINLNS) llib-llibmdoc.ln
	$(LINT) $(LINTFLAGS) -Cmdocml $(BINLNS) llib-llibmdoc.ln

libmdoc.a: $(LIBOBJS)
	$(AR) rs $@ $(LIBOBJS)

mdocml:	$(BINOBJS) libmdoc.a
	$(CC) $(CFLAGS) -o $@ $(BINOBJS) libmdoc.a

