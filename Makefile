.SUFFIXES:	.html .xml .sgml .1.txt .3.txt .7.txt .1 .3 .7 .md5 .tar.gz 

BINDIR		= $(PREFIX)/bin
INCLUDEDIR	= $(PREFIX)/include
LIBDIR		= $(PREFIX)/lib
MANDIR		= $(PREFIX)/man
INSTALL_PROGRAM	= install -m 0755
INSTALL_DATA	= install -m 0444
INSTALL_LIB	= install -m 0644
INSTALL_MAN	= $(INSTALL_DATA)

VERSION	   = 1.8.1
VDATE	   = 19 July 2009

VFLAGS     = -DVERSION=\"$(VERSION)\"
CFLAGS    += -W -Wall -Wstrict-prototypes -Wno-unused-parameter -g
CFLAGS    += $(VFLAGS)
LINTFLAGS += $(VFLAGS)

MDOCLNS	   = mdoc_macro.ln mdoc.ln mdoc_hash.ln mdoc_strings.ln \
	     mdoc_argv.ln mdoc_validate.ln mdoc_action.ln \
	     lib.ln att.ln arch.ln vol.ln msec.ln st.ln \
	     mandoc.ln
MDOCOBJS   = mdoc_macro.o mdoc.o mdoc_hash.o mdoc_strings.o \
	     mdoc_argv.o mdoc_validate.o mdoc_action.o lib.o att.o \
	     arch.o vol.o msec.o st.o mandoc.o
MDOCSRCS   = mdoc_macro.c mdoc.c mdoc_hash.c mdoc_strings.c \
	     mdoc_argv.c mdoc_validate.c mdoc_action.c lib.c att.c \
	     arch.c vol.c msec.c st.c mandoc.c

MANLNS	   = man_macro.ln man.ln man_hash.ln man_validate.ln \
	     man_action.ln mandoc.ln
MANOBJS	   = man_macro.o man.o man_hash.o man_validate.o \
	     man_action.o mandoc.o
MANSRCS	   = man_macro.c man.c man_hash.c man_validate.c \
	     man_action.c mandoc.c

MAINLNS	   = main.ln mdoc_term.ln ascii.ln term.ln tree.ln \
	     compat.ln man_term.ln
MAINOBJS   = main.o mdoc_term.o ascii.o term.o tree.o compat.o \
	     man_term.o
MAINSRCS   = main.c mdoc_term.c ascii.c term.c tree.c compat.c \
	     man_term.c

LLNS	   = llib-llibmdoc.ln llib-llibman.ln llib-lmandoc.ln
LNS	   = $(MAINLNS) $(MDOCLNS) $(MANLNS)
LIBS	   = libmdoc.a libman.a
OBJS	   = $(MDOCOBJS) $(MAINOBJS) $(MANOBJS)
SRCS	   = $(MDOCSRCS) $(MAINSRCS) $(MANSRCS)
DATAS	   = arch.in att.in lib.in msec.in st.in vol.in ascii.in
HEADS	   = mdoc.h libmdoc.h man.h libman.h term.h libmandoc.h
SGMLS	   = index.sgml 
XSLS	   = ChangeLog.xsl
HTMLS	   = index.html ChangeLog.html
XMLS	   = ChangeLog.xml
STATICS	   = style.css external.png
MD5S	   = mdocml-$(VERSION).md5 
TARGZS	   = mdocml-$(VERSION).tar.gz
MANS	   = mandoc.1 mdoc.3 mdoc.7 manuals.7 mandoc_char.7 \
	     man.7 man.3
TEXTS	   = mandoc.1.txt mdoc.3.txt mdoc.7.txt manuals.7.txt \
	     mandoc_char.7.txt man.7.txt man.3.txt
BINS	   = mandoc
CLEAN	   = $(BINS) $(LNS) $(LLNS) $(LIBS) $(OBJS) $(HTMLS) \
	     $(TARGZS) tags $(TEXTS) $(MD5S) $(XMLS) 
INSTALL	   = $(SRCS) $(HEADS) Makefile DESCR $(MANS) $(SGMLS) \
	     $(STATICS) $(DATAS) $(XSLS)

all:	$(BINS)

lint:	$(LLNS)

clean:
	rm -f $(CLEAN)

cleanlint:
	rm -f $(LNS) $(LLNS)

dist:	mdocml-$(VERSION).tar.gz

html:	$(HTMLS)

www:	all $(HTMLS) $(MD5S) $(TARGZS) $(TEXTS) 

installwww: www
	install -m 0444 $(TEXTS) $(HTMLS) $(STATICS) $(PREFIX)/
	install -m 0444 mdocml-$(VERSION).tar.gz $(PREFIX)/snapshots/
	install -m 0444 mdocml-$(VERSION).md5 $(PREFIX)/snapshots/
	install -m 0444 mdocml-$(VERSION).tar.gz $(PREFIX)/snapshots/mdocml.tar.gz
	install -m 0444 mdocml-$(VERSION).md5 $(PREFIX)/snapshots/mdocml.md5

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

mdoc_term.ln: mdoc_term.c term.h mdoc.h
mdoc_term.o: mdoc_term.c term.h mdoc.h

mdoc_strings.ln: mdoc_strings.c libmdoc.h
mdoc_strings.o: mdoc_strings.c libmdoc.h

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

compat.ln: compat.c 
compat.o: compat.c

term.ln: term.c term.h man.h mdoc.h
term.o: term.c term.h man.h mdoc.h

mdoc_argv.ln: mdoc_argv.c libmdoc.h
mdoc_argv.o: mdoc_argv.c libmdoc.h

man_validate.ln: man_validate.c libman.h
man_validate.o: man_validate.c libman.h

mdoc_validate.ln: mdoc_validate.c libmdoc.h
mdoc_validate.o: mdoc_validate.c libmdoc.h

mdoc_action.ln: mdoc_action.c libmdoc.h
mdoc_action.o: mdoc_action.c libmdoc.h

libmdoc.h: mdoc.h

ChangeLog.xml:
	cvs2cl --xml --xml-encoding iso-8859-15 -t --noxmlns -f $@

ChangeLog.html: ChangeLog.xml ChangeLog.xsl
	xsltproc -o $@ ChangeLog.xsl ChangeLog.xml

mdocml-$(VERSION).tar.gz: $(INSTALL)
	mkdir -p .dist/mdocml/mdocml-$(VERSION)/
	install -m 0644 $(INSTALL) .dist/mdocml/mdocml-$(VERSION)/
	( cd .dist/mdocml/ && tar zcf ../../$@ mdocml-$(VERSION)/ )
	rm -rf .dist/

llib-llibmdoc.ln: $(MDOCLNS)
	$(LINT) -Clibmdoc $(MDOCLNS)

llib-llibman.ln: $(MANLNS)
	$(LINT) -Clibman $(MANLNS)

llib-lmandoc.ln: $(MAINLNS) llib-llibmdoc.ln
	$(LINT) -Cmandoc $(MAINLNS) llib-llibmdoc.ln

libmdoc.a: $(MDOCOBJS)
	$(AR) rs $@ $(MDOCOBJS)

libman.a: $(MANOBJS)
	$(AR) rs $@ $(MANOBJS)

mandoc: $(MAINOBJS) libmdoc.a libman.a
	$(CC) $(CFLAGS) -o $@ $(MAINOBJS) libmdoc.a libman.a

.sgml.html:
	validate $<
	sed -e "s!@VERSION@!$(VERSION)!" -e "s!@VDATE@!$(VDATE)!" $< > $@

.1.1.txt:
	./mandoc -Wall,error -fstrict $< | col -b > $@

.3.3.txt:
	./mandoc -Wall,error -fstrict $< | col -b > $@

.7.7.txt:
	./mandoc -Wall,error -fstrict $< | col -b > $@

.tar.gz.md5:
	md5 $< > $@
