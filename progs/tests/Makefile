
# Simple makefile for compiling test programs on Linux
# These programs aren't intended to be included with the normal
# distro as they're not too interesting but good for testing during
# development.


CC = gcc
CFLAGS = -g -I../include
LIBS = -L../lib -lglut -lGLU -lGL -L/usr/X11R6/lib -lX11 -lXext -lm

PROGS = cva \
	dinoshade \
	manytex \
	multipal \
	projtex \
	sharedtex \
	texline


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
