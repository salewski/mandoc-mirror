CFLAGS += -W -Wall -Wno-unused-parameter -g -DDEBUG

LINTFLAGS += -c -e -f -u

LNS	= mdocml.ln html.ln xml.ln libmdocml.ln roff.ln ml.ln mlg.ln compat.ln tokens.ln

LLNS	= llib-lmdocml.ln

LIBS	= libmdocml.a

OBJS	= mdocml.o html.o xml.o libmdocml.o roff.o ml.o mlg.o compat.o tokens.o

SRCS	= mdocml.c html.c xml.c libmdocml.c roff.c ml.c mlg.c compat.c tokens.c

HEADS	= libmdocml.h private.h

MANS	= mdocml.1

CLEAN	= mdocml mdocml.tgz $(LLNS) $(LNS) $(OBJS) $(LIBS)

INSTALL	= Makefile $(HEADS) $(SRCS) $(MANS)

FAIL	= test.0 test.1 test.2 test.3 test.4 test.5 test.6 \
	  test.15 test.20 test.22 test.24 test.26 test.27 test.30 \
	  test.36 test.37 test.40

SUCCEED	= test.7 test.8 test.9 test.10 test.11 test.12 test.13 \
	  test.14 test.16 test.17 test.18 test.19 test.21 test.23 \
	  test.25 test.28 test.29 test.31 test.32 test.33 test.34 \
	  test.35 test.38 test.39 test.41


all: mdocml

lint: llib-lmdocml.ln

dist: mdocml.tgz

regress: mdocml
	@for f in $(FAIL); do ./mdocml $$f 1>/dev/null 2>/dev/null || continue ; done
	@for f in $(SUCCEED); do ./mdocml $$f 1>/dev/null || exit 1 ; done

mdocml: mdocml.o libmdocml.a
	$(CC) $(CFLAGS) -o $@ mdocml.o libmdocml.a

clean:
	rm -f $(CLEAN)

mdocml.tgz: $(INSTALL)
	mkdir -p .dist/mdocml/
	install -m 0644 $(INSTALL) .dist/mdocml/
	( cd .dist/ && tar zcf ../mdocml.tgz mdocml/ )
	rm -rf .dist/

llib-lmdocml.ln: mdocml.ln libmdocml.ln html.ln xml.ln roff.ln ml.ln mlg.ln compat.ln tokens.ln
	$(LINT) $(LINTFLAGS) -Cmdocml mdocml.ln libmdocml.ln html.ln xml.ln roff.ln ml.ln mlg.ln compat.ln tokens.ln

mdocml.ln: mdocml.c libmdocml.h

mdocml.o: mdocml.c libmdocml.h

libmdocml.a: libmdocml.o html.o xml.o roff.o ml.o mlg.o compat.o tokens.o
	$(AR) rs $@ libmdocml.o html.o xml.o roff.o ml.o mlg.o compat.o tokens.o

xml.ln: xml.c private.h libmdocml.h ml.h

xml.o: xml.c private.h libmdocml.h ml.h

html.ln: html.c private.h libmdocml.h

html.o: html.c private.h libmdocml.h

roff.ln: roff.c private.h libmdocml.h

roff.o: roff.c private.h libmdocml.h

libmdocml.ln: libmdocml.c private.h libmdocml.h

libmdocml.o: libmdocml.c private.h libmdocml.h

ml.ln: ml.c private.h libmdocml.h ml.h

ml.o: ml.c private.h libmdocml.h ml.h

mlg.ln: mlg.c private.h libmdocml.h ml.h

mlg.o: mlg.c private.h libmdocml.h ml.h

compat.ln: compat.c

compat.o: compat.c
