VERSION	= 1.1.0

CFLAGS += -W -Wall -Wno-unused-parameter -g 

LNS	= macro.ln mdoc.ln mdocml.ln hash.ln strings.ln

LLNS	= llib-llibmdoc.ln llib-lmdocml.ln

LIBS	= libmdoc.a

OBJS	= macro.o mdoc.o mdocml.o hash.o strings.o

SRCS	= macro.c mdoc.c mdocml.c hash.c strings.c

HEADS	= mdoc.h

BINS	= mdocml

CLEAN	= $(BINS) $(LNS) $(LLNS) $(LIBS) $(OBJS)

all:	$(BINS)

lint:	$(LLNS)

mdocml:	mdocml.o libmdoc.a
	$(CC) $(CFLAGS) -o $@ mdocml.o libmdoc.a

clean:
	rm -f $(CLEAN)

llib-llibmdoc.ln: macro.ln mdoc.ln hash.ln strings.ln
	$(LINT) $(LINTFLAGS) -Clibmdoc mdoc.ln macro.ln hash.ln strings.ln

llib-lmdocml.ln: mdocml.ln llib-llibmdoc.ln
	$(LINT) $(LINTFLAGS) -Cmdocml mdocml.ln llib-llibmdoc.ln

macro.ln: macro.c private.h

macro.o: macro.c private.h

strings.ln: strings.c private.h

strings.o: strings.c private.h

hash.ln: hash.c private.h

hash.o: hash.c private.h

mdoc.ln: mdoc.c private.h

mdoc.o: mdoc.c private.h

mdocml.ln: mdocml.c mdoc.h

mdocml.o: mdocml.c mdoc.h

private.h: mdoc.h

libmdoc.a: macro.o mdoc.o hash.o strings.o
	$(AR) rs $@ macro.o mdoc.o hash.o strings.o

