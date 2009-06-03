# progs/tests/Makefile


# These programs aren't intended to be included with the normal distro.
# They're not too interesting but they're good for testing.

TOP = ../..
include $(TOP)/configs/current


LIBS = -L$(TOP)/$(LIB_DIR) -l$(GLUT_LIB) -l$(GLEW_LIB) -l$(GLU_LIB) -l$(GL_LIB) $(APP_LIB_DEPS)

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
	blendxor.c \
	bufferobj.c \
	bumpmap.c \
	bug_3050.c \
	bug_3101.c \
	bug_3195.c \
	bug_texstore_i8.c \
	calibrate_rast.c \
	copypixrate.c \
	crossbar.c \
	cva.c \
	drawbuffers.c \
	exactrast.c \
	floattex.c \
	fbotest1.c \
	fbotest2.c \
	fillrate.c \
	fog.c \
	fogcoord.c \
	fptest1.c \
	fptexture.c \
	getprocaddress.c \
	glutfx \
	interleave.c \
	invert.c \
	jkrahntest.c \
	lineclip.c \
	manytex.c \
	mapbufrange.c \
	mapvbo.c \
	minmag.c \
	mipgen.c \
	mipmap_comp.c \
	mipmap_limits.c \
	mipmap_view.c \
	multipal.c \
	no_s3tc.c \
	packedpixels.c \
	pbo.c \
	prog_parameter.c \
	quads.c \
	random.c \
	readrate.c \
	rubberband.c \
	scissor.c \
	scissor-viewport.c \
	seccolor.c \
	shader_api.c \
	sharedtex.c \
	stencil_twoside.c \
	stencilwrap.c \
	stencil_wrap.c \
	streaming_rect \
	subtex \
	subtexrate.c \
	tex1d.c \
	texcompress2.c \
	texdown \
	texfilt.c \
	texline.c \
	texobj.c \
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
	zcomp.c \
	zdrawpix.c \
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
	$(APP_CC) $(INCLUDES) $(CFLAGS) $(LDFLAGS) $< $(LIBS) -o $@

.c.o:
	$(APP_CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) $< -o $@

.S.o:
	$(APP_CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES)  $< -o $@

# auto code generation
getprocaddress: getprocaddress.c getproclist.h

getproclist.h: $(TOP)/src/mesa/glapi/gl_API.xml getprocaddress.c getprocaddress.py
	python getprocaddress.py > getproclist.h

arraytexture: arraytexture.o readtex.o
	$(APP_CC) $(CFLAGS) arraytexture.o readtex.o $(LIBS) -o $@

arraytexture.o: arraytexture.c readtex.h
	$(APP_CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) arraytexture.c -o $@

afsmultiarb: afsmultiarb.o readtex.o
	$(APP_CC) $(CFLAGS) $(LDFLAGS) afsmultiarb.o readtex.o $(LIBS) -o $@

afsmultiarb.o: afsmultiarb.c readtex.h
	$(APP_CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) afsmultiarb.c -o $@

bumpmap: bumpmap.o readtex.o
	$(CC) $(CFLAGS) $(LDFLAGS) bumpmap.o readtex.o $(LIBS) -o $@

bumpmap.o: bumpmap.c readtex.h
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) bumpmap.c -o $@

drawbuffers: drawbuffers.o
	$(APP_CC) $(CFLAGS) $(LDFLAGS) drawbuffers.o $(LIBS) -o $@

drawbuffers.o: drawbuffers.c extfuncs.h
	$(APP_CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) drawbuffers.c -o $@

texrect: texrect.o readtex.o
	$(APP_CC) $(CFLAGS) $(LDFLAGS) texrect.o readtex.o $(LIBS) -o $@

texrect.o: texrect.c readtex.h
	$(APP_CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) texrect.c -o $@

bug_3195: bug_3195.o readtex.o
	$(APP_CC) $(CFLAGS) $(LDFLAGS) bug_3195.o readtex.o $(LIBS) -o $@

bug_3195.o: bug_3195.c readtex.h
	$(APP_CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) bug_3195.c -o $@

invert: invert.o readtex.o
	$(APP_CC) $(CFLAGS) $(LDFLAGS) invert.o readtex.o $(LIBS) -o $@

invert.o: invert.c readtex.h
	$(APP_CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) invert.c -o $@

mipmap_view: mipmap_view.o readtex.o
	$(APP_CC) $(CFLAGS) mipmap_view.o readtex.o $(LIBS) -o $@

mipmap_view.o: mipmap_view.c readtex.h
	$(APP_CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) $< -o $@

mipmap_limits: mipmap_limits.o readtex.o
	$(APP_CC) $(CFLAGS) mipmap_limits.o readtex.o $(LIBS) -o $@

mipmap_limits.o: mipmap_limits.c readtex.h
	$(APP_CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) $< -o $@

fillrate: fillrate.o readtex.o
	$(APP_CC) $(CFLAGS) fillrate.o readtex.o $(LIBS) -o $@

fillrate.o: fillrate.c readtex.h
	$(APP_CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) $< -o $@




floattex: floattex.o readtex.o shaderutil.o
	$(APP_CC) $(CFLAGS) $(LDFLAGS) floattex.o readtex.o shaderutil.o $(LIBS) -o $@

floattex.o: floattex.c readtex.h shaderutil.h
	$(APP_CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) floattex.c -o $@


readtex.o: readtex.c
	$(APP_CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) readtex.c -o $@

readtex.h: $(TOP)/progs/util/readtex.h
	ln -s $(TOP)/progs/util/readtex.h .

readtex.c: $(TOP)/progs/util/readtex.c
	ln -s $(TOP)/progs/util/readtex.c .



extfuncs.h: $(TOP)/progs/util/extfuncs.h
	ln -s $(TOP)/progs/util/extfuncs.h .



shaderutil.c: $(TOP)/progs/util/shaderutil.c
	cp $< .

shaderutil.h: $(TOP)/progs/util/shaderutil.h
	cp $< .

shaderutil.o: shaderutil.c shaderutil.h
	$(APP_CC) -c -I$(INCDIR) $(INCLUDES) $(CFLAGS) shaderutil.c



# Emacs tags
tags:
	etags `find . -name \*.[ch]` `find ../include`
