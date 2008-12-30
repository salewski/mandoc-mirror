VERSION	= 1.1.0

CFLAGS += -W -Wall -Wno-unused-parameter -g 

LNS	= macro.ln mdoc.ln mdocml.ln hash.ln strings.ln xstd.ln argv.ln validate.ln

LLNS	= llib-llibmdoc.ln llib-lmdocml.ln

LIBS	= libmdoc.a

OBJS	= macro.o mdoc.o mdocml.o hash.o strings.o xstd.o argv.o validate.o

SRCS	= macro.c mdoc.c mdocml.c hash.c strings.c xstd.c argv.c validate.c

HEADS	= mdoc.h

BINS	= mdocml

CLEAN	= $(BINS) $(LNS) $(LLNS) $(LIBS) $(OBJS)

all:	$(BINS)

lint:	$(LLNS)

mdocml:	mdocml.o libmdoc.a
	$(CC) $(CFLAGS) -o $@ mdocml.o libmdoc.a

clean:
	rm -f $(CLEAN)

llib-llibmdoc.ln: macro.ln mdoc.ln hash.ln strings.ln xstd.ln argv.ln validate.ln
	$(LINT) $(LINTFLAGS) -Clibmdoc mdoc.ln macro.ln hash.ln strings.ln xstd.ln argv.ln validate.ln

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

xstd.ln: xstd.c private.h

xstd.o: xstd.c private.h

argv.ln: argv.c private.h

argv.o: argv.c private.h

validate.ln: validate.c private.h

validate.o: validate.c private.h

private.h: mdoc.h

libmdoc.a: macro.o mdoc.o hash.o strings.o xstd.o argv.o validate.o
	$(AR) rs $@ macro.o mdoc.o hash.o strings.o xstd.o argv.o validate.o

