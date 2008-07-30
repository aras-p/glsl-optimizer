# progs/tests/Makefile


# These programs aren't intended to be included with the normal distro.
# They're not too interesting but they're good for testing.

TOP = ../..
include $(TOP)/configs/current


LIBS = -L$(TOP)/$(LIB_DIR) -l$(GLUT_LIB) -l$(GLU_LIB) -l$(GL_LIB) $(APP_LIB_DEPS)

SOURCES = \
	clear.c \
	line-clip.c \
	line-cull.c \
	line-userclip-clip.c \
	line-userclip-nop-clip.c \
	line-userclip-nop.c \
	line-userclip.c \
	line.c \
	lineloop-clip.c \
	lineloop.c \
	point.c \
	point-clip.c \
	point-param.c \
	point-wide.c \
	poly-flat.c \
	poly-unfilled.c \
	poly.c \
	quad-clip.c \
	quad-clip-all-vertices.c \
	quad-clip-nearplane.c \
	quad-degenerate.c \
	quad-flat.c \
	quad-offset-factor.c \
	quad-offset-units.c \
	quad-offset-unfilled.c \
	quad-unfilled.c \
	quad-tex-2d.c \
	quad-tex-pbo.c \
	quad-tex-3d.c \
	quad.c \
	quads.c \
	quadstrip.c \
	quadstrip-flat.c \
	dlist-edgeflag.c \
	dlist-dangling.c \
	dlist-edgeflag-dangling.c \
	drawrange.c \
	drawelements.c \
	drawarrays.c \
	tri-blend.c \
	tri-tex-3d.c \
	tri-clip.c \
	tri-cull.c \
	tri-edgeflag.c \
	tri-flat-clip.c \
	tri-flat.c \
	tri-unfilled-clip.c \
	tri-unfilled-smooth.c \
	tri-unfilled-userclip.c \
	tri-unfilled.c \
	tri-userclip.c \
	tri-dlist.c \
	tri.c \
	tristrip-clip.c \
	tristrip.c \
	vbo-drawrange.c \
	vbo-drawelements.c \
	vbo-drawarrays.c \
	vp-clip.c \
	vp-tri.c \
	vp-line-clip.c \
	vp-unfilled.c \
	vp-array.c 

PROGS = $(SOURCES:%.c=%)

INCLUDES = -I. -I$(TOP)/include -I../samples

UTIL_FILES = readtex.h readtex.c


##### RULES #####

.SUFFIXES:
.SUFFIXES: .c

.c:
	$(CC) $(INCLUDES) $(CFLAGS) $(LDFLAGS) $< $(LIBS) -o $@

.c.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) $< -o $@

.S.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES)  $< -o $@


##### TARGETS #####

default: $(UTIL_FILES) $(PROGS)

clean:
	-rm -f $(PROGS)
	-rm -f *.o
	-rm -f getproclist.h


# auto code generation
getprocaddress: getprocaddress.c getproclist.h

getproclist.h: $(TOP)/src/mesa/glapi/gl_API.xml getprocaddress.c getprocaddress.py
	python getprocaddress.py > getproclist.h


texrect: texrect.o readtex.o
	$(CC) $(CFLAGS) $(LDFLAGS) texrect.o readtex.o $(LIBS) -o $@

texrect.o: texrect.c readtex.h
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) $< -o $@

invert: invert.o readtex.o
	$(CC) $(CFLAGS) $(LDFLAGS) invert.o readtex.o $(LIBS) -o $@

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
