CFLAGS += -W -Wall -Wno-unused-parameter -g 

LINTFLAGS += -c -e -f -u

LNS	= mdocml.ln html4_strict.ln dummy.ln libmdocml.ln roff.ln

LLNS	= llib-lmdocml.ln

LIBS	= libmdocml.a

OBJS	= mdocml.o html4_strict.o dummy.o libmdocml.o roff.o

SRCS	= mdocml.c html4_strict.c dummy.c libmdocml.c roff.c

HEADS	= libmdocml.h private.h

MANS	= mdocml.1

CLEAN	= mdocml mdocml.tgz $(LLNS) $(LNS) $(OBJS) $(LIBS)

INSTALL	= Makefile $(HEADS) $(SRCS) $(MANS)

FAIL	= test.0 test.1 test.2 test.3 test.4 test.5 test.6 \
	  test.15

SUCCEED	= test.7 test.8 test.9 test.10 test.11 test.12 test.13 \
	  test.14 test.16 test.17


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

llib-lmdocml.ln: mdocml.ln libmdocml.ln html4_strict.ln dummy.ln roff.ln
	$(LINT) $(LINTFLAGS) -Cmdocml mdocml.ln libmdocml.ln html4_strict.ln dummy.ln roff.ln

mdocml.ln: mdocml.c 

mdocml.o: mdocml.c 

mdocml.c: libmdocml.h

libmdocml.a: libmdocml.o html4_strict.o dummy.o roff.o
	$(AR) rs $@ libmdocml.o html4_strict.o dummy.o roff.o

html4_strict.ln: html4_strict.c 

html4_strict.o: html4_strict.c 

html4_strict.c: private.h libmdocml.h

roff.ln: roff.c 

roff.o: roff.c 

roff.c: private.h libmdocml.h

libmdocml.ln: libmdocml.c 

libmdocml.o: libmdocml.c 

libmdocml.c: private.h libmdocml.h

