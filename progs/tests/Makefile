
# Simple makefile for compiling test programs on Linux
# These programs aren't intended to be included with the normal
# distro.  They're not too interesting but they're good for testing.


CC = gcc
CFLAGS = -g -I../include
LIBS = -L../lib -lglut -lGLU -lGL -L/usr/X11R6/lib -lX11 -lXext -lm

PROGS = cva \
	dinoshade \
	fogcoord \
	getprocaddress \
	manytex \
	multipal \
	projtex \
	seccolor \
	sharedtex \
	stencilwrap \
	texline \
	texwrap \
	vptest1 \
	vptest2 \
	vptest3 \
	vptorus \
	vpwarpmesh


##### RULES #####

.SUFFIXES:
.SUFFIXES: .c

.c:
	$(CC) $(CFLAGS) $< $(LIBS) -o $@


##### TARGETS #####

default: $(PROGS)

clean:
	rm -f $(PROGS)
	rm -f *.o


# auto code generation
getprocaddress: getprocaddress.c getproclist.h

getproclist.h: ../bin/APIspec getprocaddress.c getprocaddress.py
	python getprocaddress.py > getproclist.h

