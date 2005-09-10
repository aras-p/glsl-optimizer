# progs/tests/Makefile


# These programs aren't intended to be included with the normal distro.
# They're not too interesting but they're good for testing.

TOP = ../..
include $(TOP)/configs/current


LIBS = $(APP_LIB_DEPS)

SOURCES = antialias.c \
	arbfpspec.c \
	arbfptest1.c \
	arbfptexture.c \
	arbfptrig.c \
	arbvptest1.c \
	arbvptest3.c \
	arbvptorus.c \
	arbvpwarpmesh.c \
	blendminmax.c \
	blendsquare.c \
	bufferobj.c \
	bug_3101.c \
	bug_3195.c \
	crossbar.c \
	cva.c \
	dinoshade.c \
	floattex.c \
	fbotest1.c \
	fbotexture.c \
	fogcoord.c \
	fptest1.c \
	fptexture.c \
	getprocaddress.c \
	invert.c \
	manytex.c \
	multipal.c \
	no_s3tc.c \
	packedpixels.c \
	pbo.c \
	projtex.c \
	seccolor.c \
	sharedtex.c \
	stencilwrap.c \
	stencil_wrap.c \
	tex1d.c \
	texline.c \
	texobjshare.c \
	texrect.c \
	texwrap.c \
	vparray.c \
	vptest1.c \
	vptest2.c \
	vptest3.c \
	vptorus.c \
	vpwarpmesh.c \
	yuvrect.c \
	yuvsquare.c \
	zreaddraw.c

PROGS = $(SOURCES:%.c=%)

INCLUDES = -I. -I$(TOP)/include

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

afsmultiarb: afsmultiarb.o readtex.o
	$(CC) afsmultiarb.o readtex.o $(LIBS) -o $@

afsmultiarb.o: afsmultiarb.c readtex.h
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) $< -o $@

texrect: texrect.o readtex.o
	$(CC) texrect.o readtex.o $(LIBS) -o $@

texrect.o: texrect.c readtex.h
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) $< -o $@

bug_3195: bug_3195.o readtex.o
	$(CC) $(CFLAGS) bug_3195.o readtex.o $(LIBS) -o $@

bug_3195.o: bug_3195.c readtex.h
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
