.PHONY: 	 clean install installwww
.SUFFIXES:	 .sgml .html .md5 .h .h.html
.SUFFIXES:	 .1       .3       .7
.SUFFIXES:	 .1.txt   .3.txt   .7.txt
.SUFFIXES:	 .1.pdf   .3.pdf   .7.pdf
.SUFFIXES:	 .1.ps    .3.ps    .7.ps
.SUFFIXES:	 .1.html  .3.html  .7.html 
.SUFFIXES:	 .1.xhtml .3.xhtml .7.xhtml 

# Specify this if you want to hard-code the operating system to appear
# in the lower-left hand corner of -mdoc manuals.
# CFLAGS	+= -DOSNAME="\"OpenBSD 4.5\""

VERSION		 = 1.10.10
VDATE		 = 20 March 2011
CFLAGS		+= -g -DHAVE_CONFIG_H -DVERSION="\"$(VERSION)\""
CFLAGS     	+= -W -Wall -Wstrict-prototypes -Wno-unused-parameter -Wwrite-strings
PREFIX		 = /usr/local
BINDIR		 = $(PREFIX)/bin
INCLUDEDIR	 = $(PREFIX)/include/mandoc
LIBDIR		 = $(PREFIX)/lib/mandoc
MANDIR		 = $(PREFIX)/man
EXAMPLEDIR	 = $(PREFIX)/share/examples/mandoc
INSTALL		 = install
INSTALL_PROGRAM	 = $(INSTALL) -m 0755
INSTALL_DATA	 = $(INSTALL) -m 0444
INSTALL_LIB	 = $(INSTALL) -m 0644
INSTALL_MAN	 = $(INSTALL_DATA)

all: mandoc

SRCS		 = Makefile \
		   arch.c \
		   arch.in \
		   att.c \
		   att.in \
		   chars.c \
		   chars.in \
		   compat.c \
		   config.h.post \
		   config.h.pre \
		   eqn.7 \
		   eqn.c \
		   example.style.css \
		   external.png \
		   html.c \
		   html.h \
		   index.css \
		   index.sgml \
		   lib.c \
		   lib.in \
		   libman.h \
		   libmandoc.h \
		   libmdoc.h \
		   libroff.h \
		   main.c \
		   main.h \
		   man.7 \
		   man.c \
		   man_argv.c \
		   man_hash.c \
		   man_html.c \
		   man_macro.c \
		   man_term.c \
		   man_validate.c \
		   mandoc.1 \
		   mandoc.3 \
		   mandoc.c \
		   mandoc.h \
		   mandoc_char.7 \
		   mdoc.7 \
		   mdoc.c \
		   mdoc_argv.c \
		   mdoc_hash.c \
		   mdoc_html.c \
		   mdoc_macro.c \
		   mdoc_term.c \
		   mdoc_validate.c \
		   msec.c \
		   msec.in \
		   out.c \
		   out.h \
		   read.c \
		   roff.7 \
		   roff.c \
		   st.c \
		   st.in \
		   style.css \
		   tbl.7 \
		   tbl.c \
		   tbl_data.c \
		   tbl_html.c \
		   tbl_layout.c \
		   tbl_opts.c \
		   tbl_term.c \
		   term.c \
		   term.h \
		   term_ascii.c \
		   term_ps.c \
		   test-strlcat.c \
		   test-strlcpy.c \
		   tree.c \
		   vol.c \
		   vol.in

LIBMAN_OBJS	 = man.o \
		   man_argv.o \
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
		   msec.o \
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
		   mandoc.o \
		   read.o

arch.o: arch.in
att.o: att.in
lib.o: lib.in
msec.o: msec.in
st.o: st.in
vol.o: vol.in

$(LIBMAN_OBJS): libman.h
$(LIBMDOC_OBJS): libmdoc.h
$(LIBROFF_OBJS): libroff.h
$(LIBMANDOC_OBJS): mandoc.h mdoc.h man.h libmandoc.h config.h

MANDOC_HTML_OBJS = html.o \
		   man_html.o \
		   mdoc_html.o \
		   tbl_html.o
MANDOC_TERM_OBJS = man_term.o \
		   mdoc_term.o \
		   term.o \
		   term_ascii.o \
		   term_ps.o \
		   tbl_term.o
MANDOC_OBJS	 = $(MANDOC_HTML_OBJS) \
		   $(MANDOC_TERM_OBJS) \
		   chars.o \
		   main.o \
		   out.o \
		   tree.o

chars.o: chars.in

$(MANDOC_HTML_OBJS): html.h
$(MANDOC_TERM_OBJS): term.h
$(MANDOC_OBJS): main.h mandoc.h mdoc.h man.h config.h out.h

compat.o: config.h

INDEX_MANS	 = mandoc.1.html \
		   mandoc.1.xhtml \
		   mandoc.1.ps \
		   mandoc.1.pdf \
		   mandoc.1.txt \
		   mandoc.3.html \
		   mandoc.3.xhtml \
		   mandoc.3.ps \
		   mandoc.3.pdf \
		   mandoc.3.txt \
		   eqn.7.html \
		   eqn.7.xhtml \
		   eqn.7.ps \
		   eqn.7.pdf \
		   eqn.7.txt \
		   man.7.html \
		   man.7.xhtml \
		   man.7.ps \
		   man.7.pdf \
		   man.7.txt \
		   mandoc_char.7.html \
		   mandoc_char.7.xhtml \
		   mandoc_char.7.ps \
		   mandoc_char.7.pdf \
		   mandoc_char.7.txt \
		   mdoc.7.html \
		   mdoc.7.xhtml \
		   mdoc.7.ps \
		   mdoc.7.pdf \
		   mdoc.7.txt \
		   roff.7.html \
		   roff.7.xhtml \
		   roff.7.ps \
		   roff.7.pdf \
		   roff.7.txt \
		   tbl.7.html \
		   tbl.7.xhtml \
		   tbl.7.ps \
		   tbl.7.pdf \
		   tbl.7.txt

$(INDEX_MANS): mandoc

INDEX_OBJS	 = $(INDEX_MANS) \
		   man.h.html \
		   mandoc.h.html \
		   mdoc.h.html \
		   mdocml.tar.gz \
		   mdocml.md5

www: index.html

clean:
	rm -f libmandoc.a $(LIBMANDOC_OBJS)
	rm -f mandoc $(MANDOC_OBJS)
	rm -f config.h compat.o config.log
	rm -f mdocml.tar.gz
	rm -f index.html $(INDEX_OBJS)

install: all
	mkdir -p $(DESTDIR)$(BINDIR)
	mkdir -p $(DESTDIR)$(EXAMPLEDIR)
	mkdir -p $(DESTDIR)$(MANDIR)/man1
	mkdir -p $(DESTDIR)$(MANDIR)/man3
	mkdir -p $(DESTDIR)$(MANDIR)/man7
	$(INSTALL_PROGRAM) mandoc $(DESTDIR)$(BINDIR)
	$(INSTALL_LIB) libmandoc.a $(DESTDIR)$(LIBDIR)/
	$(INSTALL_MAN) mandoc.1 $(DESTDIR)$(MANDIR)/man1
	$(INSTALL_MAN) mandoc.3 $(DESTDIR)$(MANDIR)/man3
	$(INSTALL_MAN) man.7 mdoc.7 roff.7 eqn.7 tbl.7 mandoc_char.7 $(DESTDIR)$(MANDIR)/man7
	$(INSTALL_DATA) example.style.css $(DESTDIR)$(EXAMPLEDIR)

installwww: www
	mkdir -p $(PREFIX)/snapshots
	$(INSTALL_DATA) index.html external.png index.css $(PREFIX)
	$(INSTALL_DATA) $(INDEX_MANS) style.css $(PREFIX)
	$(INSTALL_DATA) mandoc.h.html man.h.html mdoc.h.html $(PREFIX)
	$(INSTALL_DATA) mdocml.tar.gz $(PREFIX)/snapshots
	$(INSTALL_DATA) mdocml.md5 $(PREFIX)/snapshots
	$(INSTALL_DATA) mdocml.tar.gz $(PREFIX)/snapshots/mdocml-$(VERSION).tar.gz
	$(INSTALL_DATA) mdocml.md5 $(PREFIX)/snapshots/mdocml-$(VERSION).md5

libmandoc.a: compat.o $(LIBMANDOC_OBJS)
	$(AR) rs $@ compat.o $(LIBMANDOC_OBJS)

mandoc: $(MANDOC_OBJS) libmandoc.a
	$(CC) -o $@ $(MANDOC_OBJS) libmandoc.a

mdocml.md5: mdocml.tar.gz
	md5 mdocml.tar.gz >$@

mdocml.tar.gz: $(SRCS)
	mkdir -p .dist/mdocml-$(VERSION)/
	$(INSTALL) -m 0444 $(SRCS) .dist/mdocml-$(VERSION)
	( cd .dist/ && tar zcf ../$@ ./ )
	rm -rf .dist/

index.html: $(INDEX_OBJS)

config.h: config.h.pre config.h.post
	rm -f config.log
	( cat config.h.pre; \
	  echo; \
	  if $(CC) $(CFLAGS) -Werror -o test-strlcat test-strlcat.c >> config.log 2>&1; then \
		echo '#define HAVE_STRLCAT'; \
		rm test-strlcat; \
	  fi; \
	  if $(CC) $(CFLAGS) -Werror -o test-strlcpy test-strlcpy.c >> config.log 2>&1; then \
		echo '#define HAVE_STRLCPY'; \
		rm test-strlcpy; \
	  fi; \
	  echo; \
	  cat config.h.post \
	) > $@

.h.h.html:
	highlight -I $< >$@

.1.1.txt .3.3.txt .7.7.txt:
	./mandoc -Tascii -Wall,stop $< | col -b >$@

.1.1.html .3.3.html .7.7.html:
	./mandoc -Thtml -Wall,stop -Ostyle=style.css,man=%N.%S.html,includes=%I.html $< >$@

.1.1.ps .3.3.ps .7.7.ps:
	./mandoc -Tps -Wall,stop $< >$@

.1.1.xhtml .3.3.xhtml .7.7.xhtml:
	./mandoc -Txhtml -Wall,stop -Ostyle=style.css,man=%N.%S.xhtml,includes=%I.html $< >$@

.1.1.pdf .3.3.pdf .7.7.pdf:
	./mandoc -Tpdf -Wall,stop $< >$@

.sgml.html:
	validate --warn $<
	sed -e "s!@VERSION@!$(VERSION)!" -e "s!@VDATE@!$(VDATE)!" $< >$@
