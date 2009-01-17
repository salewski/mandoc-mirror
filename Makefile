VERSION	= 1.1.0

CFLAGS += -W -Wall -Wno-unused-parameter -g 

LNS	= macro.ln mdoc.ln mdocml.ln hash.ln strings.ln xstd.ln argv.ln validate.ln action.ln tree.ln

LLNS	= llib-llibmdoc.ln llib-lmdocml.ln

LIBS	= libmdoc.a

OBJS	= macro.o mdoc.o mdocml.o hash.o strings.o xstd.o argv.o validate.o action.o tree.o

SRCS	= macro.c mdoc.c mdocml.c hash.c strings.c xstd.c argv.c validate.c action.c tree.c

HEADS	= mdoc.h

BINS	= mdocml

CLEAN	= $(BINS) $(LNS) $(LLNS) $(LIBS) $(OBJS)

SUCCESS	= test.1 test.7 test.8 test.9 test.11 test.11 test.12 test.16 \
	  test.17 test.19 test.20 test.21 test.23 test.25 test.27     \
	  test.28 test.29 test.31 test.32 test.33 test.34 test.35     \
	  test.38 test.39 test.40 test.41 test.42 test.43 test.44     \
	  test.45 test.46 test.47 test.49 test.51 test.52 test.53     \
	  test.54 test.55 test.56 test.57 test.58 test.59 test.60     \
	  test.62 test.67 test.68 test.71 test.72 test.73 test.74     \
	  test.75

FAIL	= test.0 test.2 test.3 test.4 test.5 test.6 test.13 test.14   \
	  test.15 test.18 test.22 test.24 test.26 test.30 test.36     \
	  test.37 test.48 test.50 test.61 test.63 test.64 test.65     \
	  test.66 test.69 test.70

all:	$(BINS)

lint:	$(LLNS)

mdocml:	mdocml.o tree.o libmdoc.a
	$(CC) $(CFLAGS) -o $@ mdocml.o tree.o libmdoc.a

clean:
	rm -f $(CLEAN)

regress: mdocml $(SUCCESS) $(FAIL)
	@for f in $(SUCCESS) ; do \
		echo "./mdocml $$f" ; \
		./mdocml $$f || exit 1 ; \
	done

llib-llibmdoc.ln: macro.ln mdoc.ln hash.ln strings.ln xstd.ln argv.ln validate.ln action.ln
	$(LINT) $(LINTFLAGS) -Clibmdoc mdoc.ln macro.ln hash.ln strings.ln xstd.ln argv.ln validate.ln action.ln

llib-lmdocml.ln: mdocml.ln tree.ln llib-llibmdoc.ln
	$(LINT) $(LINTFLAGS) -Cmdocml mdocml.ln tree.ln llib-llibmdoc.ln

macro.ln: macro.c private.h

macro.o: macro.c private.h

strings.ln: strings.c private.h

strings.o: strings.c private.h

tree.ln: tree.c mdoc.h

tree.o: tree.c mdoc.h

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

action.ln: action.c private.h

action.o: action.c private.h

private.h: mdoc.h

libmdoc.a: macro.o mdoc.o hash.o strings.o xstd.o argv.o validate.o action.o
	$(AR) rs $@ macro.o mdoc.o hash.o strings.o xstd.o argv.o validate.o action.o

