# $Id: Makefile,v 1.2 1999/09/17 00:55:21 brianp Exp $

# Makefile for BeOS demos

# Written by Brian Paul
# This file is in the public domain.



CC = g++

# Use Mesa:
CFLAGS = -I../include -c -g
LFLAGS = -L../lib  -Xlinker -rpath ../lib -lbe -lGL

# Use BeOS OpenGL:
#CFLAGS = -I/boot/develop/headers/be/opengl -c -g
#LFLAGS = -L../lib  -Xlinker -rpath ../lib -lbe -lGL


PROGRAMS = demo sample

default: $(PROGRAMS)


clean:
	rm -f demo sample
	rm -f *.o


demo: demo.o
	$(CC) demo.o $(LFLAGS) -o $@

demo.o: demo.cpp
	$(CC) $(CFLAGS) demo.cpp


sample: sample.o
	$(CC) sample.o $(LFLAGS) -o $@

sample.o: sample.cpp
	$(CC) $(CFLAGS) sample.cpp
