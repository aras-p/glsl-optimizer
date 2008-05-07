# progs/beos/Makefile

TOP = ../..
include $(TOP)/configs/current

# Makefile for BeOS demos

# Written by Brian Paul
# This file is in the public domain.
#
# Modified by Philippe Houdoin

LDFLAGS += -soname=_APP_

LIBS = -L$(TOP)/$(LIB_DIR) -l$(GLUT_LIB) -l$(GLU_LIB) -l$(GL_LIB) $(APP_LIB_DEPS)

INCLUDES = -I. -I- -I../../include

default: demo sample GLInfo

clean:
	-rm -f demo sample GLInfo
	-rm -f *.o

demo: demo.o
	$(LD) demo.o $(LDFLAGS) $(LIBS) -o $@

sample: sample.o
	$(LD) sample.o $(LDFLAGS) $(LIBS) -o $@

GTLInfo: GLInfo.o
	$(LD) GLInfo.o $(INCLUDES) $(LDFLAGS) $(LIBS) -o $@

.cpp.o:
	$(CC) -c $< $(INCLUDES) $(CFLAGS) -o $@
