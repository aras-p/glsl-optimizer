
# Simple makefile for compiling test programs on Linux
# These programs aren't intended to be included with the normal
# distro.  They're not too interesting but they're good for testing.


CC = gcc
CFLAGS = -g -I../include
LIBS = -L../lib -lglut -lGLU -lGL -L/usr/X11R6/lib -lX11 -lXext -lm

PROGS = antialias \
	cva \
	dinoshade \
	fogcoord \
	fptest1 \
	fptexture \
	getprocaddress \
	manytex \
	multipal \
	projtex \
	seccolor \
	sharedtex \
	stencilwrap \
	texline \
	texrect \
	texwrap \
	vptest1 \
	vptest2 \
	vptest3 \
	vptorus \
	vpwarpmesh \
	yuvrect \
	yuvsquare


##### RULES #####

.SUFFIXES:
.SUFFIXES: .c

.c:
	$(CC) $(CFLAGS) $< $(LIBS) -o $@

.c.o:
	$(CC) -c -I. -I../include $(CFLAGS) $< -o $@


##### TARGETS #####

default: $(PROGS)

clean:
	rm -f $(PROGS)
	rm -f *.o


# auto code generation
getprocaddress: getprocaddress.c getproclist.h

getproclist.h: ../bin/APIspec getprocaddress.c getprocaddress.py
	python getprocaddress.py > getproclist.h


texrect: texrect.o readtex.o
	$(CC) texrect.o readtex.o $(LIBS) -o $@

texrect.o: texrect.c readtex.h
	$(CC) -c $(CFLAGS) $< -o $@

readtex.o: readtex.c
	$(CC) -c $(CFLAGS) $< -o $@


readtex.h: ../util/readtex.h
	ln -s ../util/readtex.h .

readtex.c: ../util/readtex.c
	ln -s ../util/readtex.c .
