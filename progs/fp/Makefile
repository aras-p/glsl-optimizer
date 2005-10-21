# progs/tests/Makefile


# These programs aren't intended to be included with the normal distro.
# They're not too interesting but they're good for testing.

TOP = ../..
include $(TOP)/configs/current


LIBS = $(APP_LIB_DEPS)

SOURCES = \
	tri-abs.c \
	tri-add.c \
	tri-cmp.c \
	tri-cos.c \
	tri-dp3.c \
	tri-dp4.c \
	tri-dph.c \
	tri-dst.c \
	tri-ex2.c \
	tri-flr.c \
	tri-frc.c \
	tri-kil.c \
	tri-lg2.c \
	tri-lit.c \
	tri-lrp.c \
	tri-mad.c \
	tri-max.c \
	tri-min.c \
	tri-mov.c \
	tri-mul.c \
	tri-pow.c \
	tri-rcp.c \
	tri-rsq.c \
	tri-scs.c \
	tri-sge.c \
	tri-sge2.c \
	tri-sin.c \
	tri-slt.c \
	tri-sub.c \
	tri-swz.c \
	tri-swz2.c \
	tri-tex.c \
	tri-xpd.c \
	tri-position.c \
	tri-depth.c \
	tri-depth2.c \
	tri-depthwrite.c \
	tri-depthwrite2.c \

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
	$(CC) $(INCLUDES) $(CFLAGS) $< $(LIBS) -o $@

.c.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) $< -o $@

.S.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES)  $< -o $@


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
	$(CC) texrect.o readtex.o $(LIBS) -o $@

texrect.o: texrect.c readtex.h
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) $< -o $@

invert: invert.o readtex.o
	$(CC) invert.o readtex.o $(LIBS) -o $@

invert.o: invert.c readtex.h
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) $< -o $@

readtex.o: readtex.c
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) $< -o $@


readtex.h: $(TOP)/progs/util/readtex.h
	ln -s $(TOP)/progs/util/readtex.h .

readtex.c: $(TOP)/progs/util/readtex.c
	ln -s $(TOP)/progs/util/readtex.c .




# Emacs tags
tags:
	etags `find . -name \*.[ch]` `find ../include`
