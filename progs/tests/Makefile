
# Simple makefile for compiling test programs on Linux
# These programs aren't intended to be included with the normal
# distro as they're not too interesting but good for testing during
# development.


CC = gcc
CFLAGS = -c -g -I../include
LIBS = -L../lib -lglut -lGLU -lGL -L/usr/X11R6/lib -lX11 -lXext -lm


PROGRAMS = manytex sharedtex


default: $(PROGRAMS)

clean:
	rm -f $(PROGRAMS)
	rm -f *.o



manytex: manytex.o
	$(CC) manytex.o $(LIBS) -o $@

manytex.o: manytex.c
	$(CC) $(CFLAGS) manytex.c


sharedtex: sharedtex.o
	$(CC) sharedtex.o $(LIBS) -o $@

sharedtex.o: sharedtex.c
	$(CC) $(CFLAGS) sharedtex.c
