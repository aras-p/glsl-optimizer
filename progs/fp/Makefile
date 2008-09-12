# progs/tests/Makefile


# These programs aren't intended to be included with the normal distro.
# They're not too interesting but they're good for testing.

TOP = ../..
include $(TOP)/configs/current


LIBS = $(APP_LIB_DEPS)

SOURCES = \
	tri-tex.c \
	point-position.c \
	tri-depth.c \
	tri-depth2.c \
	tri-depthwrite.c \
	tri-depthwrite2.c \
        tri-param.c \
        fp-tri.c

NOTDONE=\
	tri-txb.c \
	tri-txp.c \
	tri-depthwrite.c \
	tri-fogoption.c 


PROGS = $(SOURCES:%.c=%)

INCLUDES = -I. -I$(TOP)/include -I../samples

UTIL_FILES = readtex.h readtex.c


##### RULES #####

.SUFFIXES:
.SUFFIXES: .c

.c:
	$(APP_CC) $(INCLUDES) $(CFLAGS) $< $(LIBS) -o $@

.c.o:
	$(APP_CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) $< -o $@

.S.o:
	$(APP_CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES)  $< -o $@


##### TARGETS #####

default: $(UTIL_FILES) $(PROGS)

clean:
	rm -f $(PROGS)
	rm -f *.o
	rm -f getproclist.h


# auto code generation
getprocaddress: getprocaddress.c getproclist.h

getproclist.h: $(TOP)/src/mesa/glapi/gl_API.xml getprocaddress.c getprocaddress.py
	python getprocaddress.py > getproclist.h


texrect: texrect.o readtex.o
	$(APP_CC) texrect.o readtex.o $(LIBS) -o $@

texrect.o: texrect.c readtex.h
	$(APP_CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) $< -o $@

invert: invert.o readtex.o
	$(APP_CC) invert.o readtex.o $(LIBS) -o $@

invert.o: invert.c readtex.h
	$(APP_CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) $< -o $@

readtex.o: readtex.c
	$(APP_CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) $< -o $@


readtex.h: $(TOP)/progs/util/readtex.h
	ln -s $(TOP)/progs/util/readtex.h .

readtex.c: $(TOP)/progs/util/readtex.c
	ln -s $(TOP)/progs/util/readtex.c .




# Emacs tags
tags:
	etags `find . -name \*.[ch]` `find ../include`
