CFLAGS += -W -Wall -g

LINTFLAGS += -c -e -f -u

CLEAN = mdocml mdocml.o mdocml.ln libmdocml.o libmdocml.ln mdocml.tgz llib-lmdocml.ln

INSTALL = Makefile libmdocml.h mdocml.c libmdocml.c mdocml.1

all: mdocml

lint: llib-lmdocml.ln

dist: mdocml.tgz

mdocml: mdocml.o libmdocml.o
	$(CC) $(CFLAGS) -o $@ mdocml.o libmdocml.o

clean:
	rm -f $(CLEAN)

mdocml.tgz: $(INSTALL)
	mkdir -p .dist/mdocml/
	install -m 0644 $(INSTALL) .dist/mdocml/
	( cd .dist/ && tar zcf ../mdocml.tgz mdocml/ )
	rm -rf .dist/

llib-lmdocml.ln: mdocml.ln libmdocml.ln
	$(LINT) $(LINTFLAGS) -Cmdocml mdocml.ln libmdocml.ln

mdocml.ln: mdocml.c 

mdocml.o: mdocml.c 

mdocml.c: libmdocml.h

libmdocml.ln: libmdocml.c 

libmdocml.o: libmdocml.c 

libmdocml.c: libmdocml.h
