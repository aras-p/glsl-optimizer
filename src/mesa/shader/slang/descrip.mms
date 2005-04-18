# Makefile for core library for VMS
# contributed by Jouk Jansen  joukj@hrem.stm.tudelft.nl
# Last revision : 18 April 2005

.first
	define gl [----.include.gl]
	define math [--.math]
	define swrast [--.swrast]
	define array_cache [--.array_cache]

.include [----]mms-config.

##### MACROS #####

VPATH = RCS

INCDIR = [----.include],[--.main],[--.glapi],[-.slang],[-]
LIBDIR = [----.lib]
CFLAGS = /include=($(INCDIR),[])/define=(PTHREADS=1)/name=(as_is,short)

SOURCES = \
	slang_compile.c,slang_preprocess.c

OBJECTS = \
	slang_compile.obj,slang_preprocess.obj

##### RULES #####

VERSION=Mesa V3.4

##### TARGETS #####
# Make the library
$(LIBDIR)$(GL_LIB) : $(OBJECTS)
  @ library $(LIBDIR)$(GL_LIB) $(OBJECTS)

clean :
	purge
	delete *.obj;*

slang_compile.obj : slang_compile.c
slang_preprocess.obj : slang_preprocess.c
