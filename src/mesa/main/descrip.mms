# Makefile for core library for VMS
# contributed by Jouk Jansen  joukj@hrem.stm.tudelft.nl
# Last revision : 16 June 2003

.first
	define gl [---.include.gl]
	define math [-.math]

.include [---]mms-config.

##### MACROS #####

VPATH = RCS

INCDIR = [---.include],[-.glapi]
LIBDIR = [---.lib]
CFLAGS = /include=($(INCDIR),[])/define=(PTHREADS=1)/name=(as_is,short)

SOURCES =accum.c \
	api_loopback.c \
	api_noop.c \
	api_validate.c \
	arbprogram.c \
 	attrib.c \
	blend.c \
	bufferobj.c \
	buffers.c \
	clip.c \
	colortab.c \
	context.c \
	convolve.c \
	debug.c \
	depth.c \
	dispatch.c \
	dlist.c \
	drawpix.c \
	enable.c \
	enums.c \
	eval.c \
	extensions.c \
	feedback.c \
	fog.c \
	get.c \
	hash.c \
	hint.c \
	histogram.c \
	image.c \
	imports.c \
	light.c \
	lines.c \
	matrix.c \
	nvprogram.c \
	nvfragparse.c \
	nvvertexec.c \
	nvvertparse.c \
	occlude.c \
	pixel.c \
	points.c \
	polygon.c \
	rastpos.c \
	state.c \
	stencil.c \
	texcompress.c \
	texformat.c \
	teximage.c \
	texobj.c \
	texstate.c \
	texstore.c \
	texutil.c \
	varray.c \
	vtxfmt.c \
	vsnprintf.c

OBJECTS=accum.obj,\
api_loopback.obj,\
api_noop.obj,\
api_validate.obj,\
arbprogram.obj,\
attrib.obj,\
blend.obj,\
bufferobj.obj,\
buffers.obj,\
clip.obj,\
colortab.obj,\
context.obj,\
convolve.obj,\
debug.obj,\
depth.obj,\
dispatch.obj,\
dlist.obj,\
drawpix.obj,\
enable.obj,\
enums.obj,\
eval.obj,\
extensions.obj,\
feedback.obj,\
fog.obj,\
get.obj,\
hash.obj,\
hint.obj,\
histogram.obj,\
image.obj,\
imports.obj,\
light.obj,\
lines.obj,\
matrix.obj,\
nvprogram.obj,\
nvfragparse.obj,\
nvvertexec.obj,\
nvvertparse.obj,\
occlude.obj,\
pixel.obj,\
points.obj,\
polygon.obj,\
rastpos.obj,\
state.obj,\
stencil.obj,\
texcompress.obj,\
texformat.obj,\
teximage.obj,\
texobj.obj,\
texstate.obj,\
texstore.obj,\
texutil.obj,\
varray.obj,\
vtxfmt.obj,\
vsnprintf.obj

##### RULES #####

VERSION=Mesa V3.4

##### TARGETS #####
# Make the library
$(LIBDIR)$(GL_LIB) : $(OBJECTS)
  @ $(MAKELIB) $(GL_LIB) $(OBJECTS)
  @ rename $(GL_LIB)* $(LIBDIR)

clean :
	purge
	delete *.obj;*
