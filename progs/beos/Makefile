# progs/beos/Makefile

TOP = ../..
include $(TOP)/configs/current

# Makefile for BeOS demos

# Written by Brian Paul
# This file is in the public domain.
#
# Modified by Philippe Houdoin

LDFLAGS += -soname=_APP_ $(APP_LIB_DEPS)

INCLUDES = -I. -I- -I../../include

default: demo sample GLInfo

clean:
	rm -f demo sample GLInfo
	rm -f *.o

demo: demo.o
	$(LD) demo.o $(LDFLAGS) -o $@

sample: sample.o
	$(LD) sample.o $(LDFLAGS) -o $@

GTLInfo: GLInfo.o
	$(LD) GLInfo.o $(INCLUDES) $(LDFLAGS) -o $@

.cpp.o:
	$(CC) -c $< $(INCLUDES) $(CFLAGS) -o $@
