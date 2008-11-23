CFLAGS += -W -Wall -g
LINTFLAGS += -c -e -f -u

LNS	= mdocml.ln html4_strict.ln dummy.ln libmdocml.ln

LLNS	= llib-lmdocml.ln

LIBS	= libmdocml.a

OBJS	= mdocml.o html4_strict.o dummy.o libmdocml.o

SRCS	= mdocml.c html4_strict.c dummy.c libmdocml.c

HEADS	= libmdocml.h private.h

MANS	= mdocml.1

CLEAN	= mdocml mdocml.tgz $(LLNS) $(LNS) $(OBJS) $(LIBS)

INSTALL	= Makefile $(HEADS) $(SRCS) $(MANS)


all: mdocml

lint: llib-lmdocml.ln

dist: mdocml.tgz

mdocml: mdocml.o libmdocml.a
	$(CC) $(CFLAGS) -o $@ mdocml.o libmdocml.a

clean:
	rm -f $(CLEAN)

mdocml.tgz: $(INSTALL)
	mkdir -p .dist/mdocml/
	install -m 0644 $(INSTALL) .dist/mdocml/
	( cd .dist/ && tar zcf ../mdocml.tgz mdocml/ )
	rm -rf .dist/

llib-lmdocml.ln: mdocml.ln libmdocml.ln html4_strict.ln dummy.ln
	$(LINT) $(LINTFLAGS) -Cmdocml mdocml.ln libmdocml.ln html4_strict.ln dummy.ln

mdocml.ln: mdocml.c 

mdocml.o: mdocml.c 

mdocml.c: libmdocml.h

libmdocml.a: libmdocml.o html4_strict.o dummy.o
	$(AR) rs $@ libmdocml.o html4_strict.o dummy.o

html4_strict.ln: html4_strict.c 

html4_strict.o: html4_strict.c 

html4_strict.c: private.h libmdocml.h

libmdocml.ln: libmdocml.c 

libmdocml.o: libmdocml.c 

libmdocml.c: private.h libmdocml.h
