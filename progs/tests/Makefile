# progs/tests/Makefile


# These programs aren't intended to be included with the normal distro.
# They're not too interesting but they're good for testing.

TOP = ../..
include $(TOP)/configs/current


LIBS = -L$(TOP)/$(LIB_DIR) -l$(GLUT_LIB) -l$(GLU_LIB) -l$(GL_LIB) $(APP_LIB_DEPS)

SOURCES = \
	afsmultiarb.c \
	antialias.c \
	arbfpspec.c \
	arbfptest1.c \
	arbfptexture.c \
	arbfptrig.c \
	arbnpot.c \
	arbnpot-mipmap.c \
	arbvptest1.c \
	arbvptest3.c \
	arbvptorus.c \
	arbvpwarpmesh.c \
	arraytexture.c \
	blendminmax.c \
	blendsquare.c \
	bufferobj.c \
	bug_3050.c \
	bug_3101.c \
	bug_3195.c \
	copypixrate.c \
	crossbar.c \
	cva.c \
	dinoshade.c \
	drawbuffers.c \
	exactrast.c \
	floattex.c \
	fbotest1.c \
	fbotest2.c \
	fbotexture.c \
	fog.c \
	fogcoord.c \
	fptest1.c \
	fptexture.c \
	getprocaddress.c \
	interleave.c \
	invert.c \
	jkrahntest.c \
	lineclip.c \
	manytex.c \
	minmag.c \
	mipmap_limits.c \
	multipal.c \
	no_s3tc.c \
	packedpixels.c \
	pbo.c \
	prog_parameter.c \
	projtex.c \
	random.c \
	readrate.c \
	seccolor.c \
	shader_api.c \
	sharedtex.c \
	stencil_twoside.c \
	stencilwrap.c \
	stencil_wrap.c \
	subtexrate.c \
	tex1d.c \
	texcompress2.c \
	texfilt.c \
	texline.c \
	texobjshare.c \
	texrect.c \
	texwrap.c \
	unfilledclip.c \
	vao-01.c \
	vao-02.c \
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


##### TARGETS #####

default: $(UTIL_FILES) $(PROGS)

clean:
	-rm -f $(PROGS)
	-rm -f *.o
	-rm -f getproclist.h

##### RULES #####

.SUFFIXES:
.SUFFIXES: .c

.c:
	$(CC) $(INCLUDES) $(CFLAGS) $(LDFLAGS) $< $(LIBS) -o $@

.c.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) $< -o $@

.S.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES)  $< -o $@

# auto code generation
getprocaddress: getprocaddress.c getproclist.h

getproclist.h: $(TOP)/src/mesa/glapi/gl_API.xml getprocaddress.c getprocaddress.py
	python getprocaddress.py > getproclist.h

arraytexture: arraytexture.o readtex.o
	$(CC) $(CFLAGS) arraytexture.o readtex.o $(LIBS) -o $@

arraytexture.o: arraytexture.c readtex.h
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) arraytexture.c -o $@

afsmultiarb: afsmultiarb.o readtex.o
	$(CC) $(CFLAGS) $(LDFLAGS) afsmultiarb.o readtex.o $(LIBS) -o $@

afsmultiarb.o: afsmultiarb.c readtex.h
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) afsmultiarb.c -o $@

drawbuffers: drawbuffers.o
	$(CC) $(CFLAGS) $(LDFLAGS) drawbuffers.o $(LIBS) -o $@

drawbuffers.o: drawbuffers.c extfuncs.h
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) drawbuffers.c -o $@

texrect: texrect.o readtex.o
	$(CC) $(CFLAGS) $(LDFLAGS) texrect.o readtex.o $(LIBS) -o $@

texrect.o: texrect.c readtex.h
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) texrect.c -o $@

bug_3195: bug_3195.o readtex.o
	$(CC) $(CFLAGS) $(LDFLAGS) bug_3195.o readtex.o $(LIBS) -o $@

bug_3195.o: bug_3195.c readtex.h
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) bug_3195.c -o $@

invert: invert.o readtex.o
	$(CC) $(CFLAGS) $(LDFLAGS) invert.o readtex.o $(LIBS) -o $@

invert.o: invert.c readtex.h
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) invert.c -o $@

readtex.o: readtex.c
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) readtex.c -o $@


readtex.h: $(TOP)/progs/util/readtex.h
	ln -s $(TOP)/progs/util/readtex.h .

readtex.c: $(TOP)/progs/util/readtex.c
	ln -s $(TOP)/progs/util/readtex.c .

extfuncs.h: $(TOP)/progs/util/extfuncs.h
	ln -s $(TOP)/progs/util/extfuncs.h .



# Emacs tags
tags:
	etags `find . -name \*.[ch]` `find ../include`
