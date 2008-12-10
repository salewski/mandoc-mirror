VERSION	= 1.0.3

# FIXME
CFLAGS += -W -Wall -Wno-unused-parameter -g -DDEBUG

LNS	= mdocml.ln html.ln xml.ln libmdocml.ln roff.ln ml.ln mlg.ln \
	  compat.ln tokens.ln literals.ln tags.ln noop.ln

LLNS	= llib-lmdocml.ln

LIBS	= libmdocml.a

OBJS	= mdocml.o html.o xml.o libmdocml.o roff.o ml.o mlg.o \
	  compat.o tokens.o literals.o tags.o noop.o

SRCS	= mdocml.c html.c xml.c libmdocml.c roff.c ml.c mlg.c \
	  compat.c tokens.c literals.c tags.c noop.c

HEADS	= libmdocml.h private.h ml.h roff.h html.h

MANS	= mdocml.1 index.7

HTML	= index.html mdocml.html 

XML	= index.xml

TEXT	= index.txt

CLEAN	= mdocml mdocml.tgz $(LLNS) $(LNS) $(OBJS) $(LIBS) $(HTML) \
	  $(XML) $(TEXT)

INSTALL	= Makefile $(HEADS) $(SRCS) $(MANS)

FAIL	= test.0 test.1 test.2 test.3 test.4 test.5 test.6 \
	  test.15 test.20 test.22 test.24 test.26 test.27 test.30 \
	  test.36 test.37 test.40 test.50 test.61 test.64 test.65 \
	  test.66 test.69 test.70

SUCCEED	= test.7 test.8 test.9 test.10 test.11 test.12 test.13 \
	  test.14 test.16 test.17 test.18 test.19 test.21 test.23 \
	  test.25 test.28 test.29 test.31 test.32 test.33 test.34 \
	  test.35 test.38 test.39 test.41 test.42 test.43 test.44 \
	  test.45 test.46 test.47 test.48 test.49 test.51 test.52 \
	  test.54 test.55 test.56 test.57 test.58 test.59 test.60 \
	  test.62 test.63 test.67 test.68 test.71 test.72 test.73

all: mdocml

lint: llib-lmdocml.ln

dist: mdocml.tgz mdocml-port.tgz

www: all $(HTML) $(XML) $(TEXT)

regress: mdocml
	@for f in $(FAIL); do \
		echo "./mdocml $$f" ; \
		./mdocml -v $$f 1>/dev/null 2>/dev/null || continue ; \
	done
	@for f in $(SUCCEED); do \
		echo "./mdocml $$f" ; \
		./mdocml -v $$f 1>/dev/null || exit 1 ; \
	done

mdocml: mdocml.o libmdocml.a
	$(CC) $(CFLAGS) -o $@ mdocml.o libmdocml.a

clean:
	rm -f $(CLEAN)

index.html: index.7 mdocml.css
	./mdocml -Wall -fhtml -e -o $@ index.7

index.xml: index.7 mdocml.css
	./mdocml -Wall -o $@ index.7

index.txt: index.7
	cp -f index.7 index.txt

mdocml.html: mdocml.1 mdocml.css
	./mdocml -Wall -fhtml -e -o $@ mdocml.1

install-www: www dist
	install -m 0644 mdocml.tgz $(PREFIX)/mdocml-$(VERSION).tgz
	install -m 0644 mdocml.tgz $(PREFIX)/mdocml.tgz
	install -m 0644 mdocml-port.tgz $(PREFIX)/mdocml-port-$(VERSION).tgz
	install -m 0644 mdocml-port.tgz $(PREFIX)/mdocml-port.tgz
	install -m 0644 $(HTML) $(XML) $(TEXT) $(PREFIX)/

mdocml.tgz: $(INSTALL)
	mkdir -p .dist/mdocml/mdocml-$(VERSION)/
	install -m 0644 $(INSTALL) .dist/mdocml/mdocml-$(VERSION)/
	( cd .dist/mdocml/ && tar zcf ../../$@ mdocml-$(VERSION)/ )
	rm -rf .dist/

mdocml-port.tgz: $(INSTALL)
	mkdir -p .dist/mdocml/pkg
	sed -e "s!@VERSION@!$(VERSION)!" Makefile.port > .dist/mdocml/Makefile
	md5 mdocml-$(VERSION).tgz > .dist/mdocml/distinfo
	rmd160 mdocml-$(VERSION).tgz >> .dist/mdocml/distinfo
	sha1 mdocml-$(VERSION).tgz >> .dist/mdocml/distinfo
	install -m 0644 DESCR .dist/mdocml/pkg/DESCR
	echo @comment $$OpenBSD$$ > .dist/mdocml/pkg/PLIST
	echo bin/mdocml >> .dist/mdocml/pkg/PLIST
	echo @man man/man1/mdocml.1 >> .dist/mdocml/pkg/PLIST
	( cd .dist/ && tar zcf ../$@ mdocml/ )
	rm -rf .dist/

llib-lmdocml.ln: mdocml.ln libmdocml.ln html.ln xml.ln roff.ln ml.ln mlg.ln compat.ln tokens.ln literals.ln tags.ln noop.ln
	$(LINT) $(LINTFLAGS) -Cmdocml mdocml.ln libmdocml.ln html.ln xml.ln roff.ln ml.ln mlg.ln compat.ln tokens.ln literals.ln tags.ln noop.ln

mdocml.ln: mdocml.c libmdocml.h

mdocml.o: mdocml.c libmdocml.h

libmdocml.a: libmdocml.o html.o xml.o roff.o ml.o mlg.o compat.o tokens.o literals.o tags.o noop.o
	$(AR) rs $@ libmdocml.o html.o xml.o roff.o ml.o mlg.o compat.o tokens.o literals.o tags.o noop.o

xml.ln: xml.c ml.h

xml.o: xml.c ml.h

html.ln: html.c private.h

html.o: html.c private.h 

tags.ln: tags.c html.h 

tags.o: tags.c html.h

roff.ln: roff.c private.h

roff.o: roff.c private.h 

libmdocml.ln: libmdocml.c private.h

libmdocml.o: libmdocml.c private.h

ml.ln: ml.c ml.h

ml.o: ml.c ml.h

mlg.ln: mlg.c ml.h

mlg.o: mlg.c ml.h

compat.ln: compat.c

compat.o: compat.c

noop.ln: noop.c private.h

noop.o: noop.c private.h

html.h: ml.h

ml.h: private.h

private.h: libmdocml.h

