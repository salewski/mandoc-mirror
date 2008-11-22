CFLAGS += -W -Wall -g

LINTFLAGS += -c -e -f -u

CLEAN = mdocml mdocml.o mdocml.ln libmdocml.o libmdocml.ln mdocml.tgz

INSTALL = Makefile libmdocml.h mdocml.c libmdocml.c mdocml.1

all: mdocml

lint: mdocml.ln libmdocml.ln

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

mdocml.ln: mdocml.c 

mdocml.o: mdocml.c 

mdocml.c: libmdocml.h

libmdocml.ln: libmdocml.c 

libmdocml.o: libmdocml.c 

libmdocml.c: libmdocml.h
