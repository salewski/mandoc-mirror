.SUFFIXES:	.html .xml .sgml .1 .3 .7 .md5 .tar.gz .1.html .3.html .7.html

BINDIR		= $(PREFIX)/bin
INCLUDEDIR	= $(PREFIX)/include
LIBDIR		= $(PREFIX)/lib
MANDIR		= $(PREFIX)/man
INSTALL_PROGRAM	= install -m 0755
INSTALL_DATA	= install -m 0444
INSTALL_LIB	= install -m 0644
INSTALL_MAN	= $(INSTALL_DATA)

VERSION	   = 1.9.5
VDATE	   = 21 September 2009

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
	     man_action.ln mandoc.ln man_argv.ln
MANOBJS	   = man_macro.o man.o man_hash.o man_validate.o \
	     man_action.o mandoc.o man_argv.o
MANSRCS	   = man_macro.c man.c man_hash.c man_validate.c \
	     man_action.c mandoc.c man_argv.c

MAINLNS	   = main.ln mdoc_term.ln chars.ln term.ln tree.ln \
	     compat.ln man_term.ln html.ln mdoc_html.ln \
	     man_html.ln
MAINOBJS   = main.o mdoc_term.o chars.o term.o tree.o compat.o \
	     man_term.o html.o mdoc_html.o man_html.o
MAINSRCS   = main.c mdoc_term.c chars.c term.c tree.c compat.c \
	     man_term.c html.c mdoc_html.c man_html.c

LLNS	   = llib-llibmdoc.ln llib-llibman.ln llib-lmandoc.ln
LNS	   = $(MAINLNS) $(MDOCLNS) $(MANLNS)
LIBS	   = libmdoc.a libman.a
OBJS	   = $(MDOCOBJS) $(MAINOBJS) $(MANOBJS)
SRCS	   = $(MDOCSRCS) $(MAINSRCS) $(MANSRCS)
DATAS	   = arch.in att.in lib.in msec.in st.in vol.in chars.in
HEADS	   = mdoc.h libmdoc.h man.h libman.h term.h libmandoc.h html.h
SGMLS	   = index.sgml 
XSLS	   = ChangeLog.xsl
HTMLS	   = index.html ChangeLog.html mandoc.1.html mdoc.3.html \
	     man.3.html mdoc.7.html man.7.html mandoc_char.7.html \
	     manuals.7.html
EXAMPLES   = example.style.css
XMLS	   = ChangeLog.xml
STATICS	   = index.css style.css external.png
MD5S	   = mdocml-$(VERSION).md5 
TARGZS	   = mdocml-$(VERSION).tar.gz
MANS	   = mandoc.1 mdoc.3 mdoc.7 manuals.7 mandoc_char.7 \
	     man.7 man.3
BINS	   = mandoc
CLEAN	   = $(BINS) $(LNS) $(LLNS) $(LIBS) $(OBJS) $(HTMLS) \
	     $(TARGZS) tags $(MD5S) $(XMLS) 
INSTALL	   = $(SRCS) $(HEADS) Makefile $(MANS) $(SGMLS) $(STATICS) \
	     $(DATAS) $(XSLS) $(EXAMPLES)

all:	$(BINS)

lint:	$(LLNS)

clean:
	rm -f $(CLEAN)

cleanlint:
	rm -f $(LNS) $(LLNS)

dist:	mdocml-$(VERSION).tar.gz

www:	all $(HTMLS) $(MD5S) $(TARGZS)

installwww: www
	install -m 0444 $(HTMLS) $(STATICS) $(PREFIX)/
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

chars.ln: chars.c chars.in chars.h
chars.o: chars.c chars.in chars.h

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

term.ln: term.c term.h man.h mdoc.h chars.h
term.o: term.c term.h man.h mdoc.h chars.h

html.ln: html.c html.h chars.h
html.o: html.c html.h chars.h

mdoc_html.ln: mdoc_html.c html.h mdoc.h
mdoc_html.o: mdoc_html.c html.h mdoc.h

man_html.ln: man_html.c html.h man.h
man_html.o: man_html.c html.h man.h

tree.ln: tree.c man.h mdoc.h
tree.o: tree.c man.h mdoc.h

mdoc_argv.ln: mdoc_argv.c libmdoc.h
mdoc_argv.o: mdoc_argv.c libmdoc.h

man_argv.ln: man_argv.c libman.h
man_argv.o: man_argv.c libman.h

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
	cp -f $(INSTALL) .dist/mdocml/mdocml-$(VERSION)/
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

.1.1.html:
	./mandoc -Thtml -ostyle=style.css -Wall,error -fstrict $< > $@

.3.3.txt:
	./mandoc -Wall,error -fstrict $< | col -b > $@

.3.3.html:
	./mandoc -Thtml -ostyle=style.css -Wall,error -fstrict $< > $@

.7.7.txt:
	./mandoc -Wall,error -fstrict $< | col -b > $@

.7.7.html:
	./mandoc -Thtml -ostyle=style.css -Wall,error -fstrict $< > $@

.tar.gz.md5:
	md5 $< > $@
