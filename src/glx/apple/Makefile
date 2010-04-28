TOP = ../../..

include $(TOP)/configs/current

#CC=gcc
#GL_CFLAGS=-Wall -ggdb3 -Os -DPTHREADS -D_REENTRANT $(RC_CFLAGS) $(CFLAGS)
#GL_LDFLAGS=-L$(INSTALL_DIR)/lib -L$(X11_DIR)/lib $(LDFLAGS) -Wl,-single_module

TCLSH=tclsh8.5
MKDIR=mkdir
INSTALL=install
LN=ln
RM=rm

#INCLUDE=-I. -Iinclude -I.. -DGLX_ALIAS_UNSUPPORTED -I$(INSTALL_DIR)/include -I$(X11_DIR)/include

#COMPILE=$(CC) $(INCLUDE) $(GL_CFLAGS) -c

#The directory with the final binaries.
BUILD_DIR=builds

all: $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME)

SOURCES = \
	apple_cgl.c \
	apple_glx.c \
	apple_glx_context.c \
	apple_glx_drawable.c \
	apple_glx_pbuffer.c \
	apple_glx_pixmap.c \
	apple_glx_surface.c \
	apple_visual.c \
	apple_xgl_api.c \
	apple_xgl_api_additional.c \
	apple_xgl_api_read.c \
	apple_xgl_api_stereo.c \
	apple_xgl_api_viewport.c \
	appledri.c \
	../clientattrib.c \
	../compsize.c \
	../glcontextmodes.c \
	glx_empty.c \
	glx_error.c \
	../glx_pbuffer.c \
	../glx_query.c \
	../glxcmds.c \
	../glxcurrent.c \
	../glxext.c \
	../glxextensions.c \
	glxreply.c \
	../pixel.c \
	../xfont.c

include $(TOP)/src/mesa/sources.mak

LDFLAGS += -lXplugin -framework ApplicationServices -framework CoreFoundation

MESA_GLAPI_ASM_SOURCES = $(addprefix $(TOP)/src/mesa/, $(GLAPI_ASM_SOURCES))
MESA_GLAPI_SOURCES = $(addprefix $(TOP)/src/mesa/, $(GLAPI_SOURCES))
MESA_GLAPI_OBJECTS = $(addprefix $(TOP)/src/mesa/, $(GLAPI_OBJECTS))

OBJECTS = $(SOURCES:.c=.o) # $(MESA_GLAPI_OBJECTS)

INCLUDES = -I. -Iinclude -I..\
	-I$(TOP)/include \
	-I$(TOP)/include/GL/internal \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/mesa/glapi \
	$(LIBDRM_CFLAGS) \
	$(DRI2PROTO_CFLAGS) \
	$(X11_INCLUDES)

##### RULES #####

$(OBJECTS) : apple_xgl_api.h

apple_xgl_api.c : apple_xgl_api.h

apple_xgl_api.h : gen_api_header.tcl  gen_api_library.tcl  gen_code.tcl  gen_defs.tcl  gen_exports.tcl  gen_funcs.tcl  gen_types.tcl
	$(TCLSH) gen_code.tcl

.c.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(EXTRA_DEFINES) $< -o $@

.S.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(EXTRA_DEFINES)  $< -o $@

##### TARGETS #####

default: depend $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME)

# Make libGL
$(TOP)/$(LIB_DIR)/$(GL_LIB_NAME):  $(OBJECTS) Makefile
	$(MKLIB) -o $(GL_LIB) -linker '$(CC)' -ldflags '$(LDFLAGS)' \
		-major 1 -minor 2 $(MKLIB_OPTIONS) \
		-install $(TOP)/$(LIB_DIR) -id $(INSTALL_LIB_DIR)/lib$(GL_LIB).1.dylib \
		$(GL_LIB_DEPS) $(OBJECTS)

depend: $(SOURCES) $(MESA_GLAPI_SOURCES) $(MESA_GLAPI_ASM_SOURCES) Makefile
	rm -f depend
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(INCLUDES) $(SOURCES) \
		$(MESA_GLAPI_SOURCES) $(MESA_GLAPI_ASM_SOURCES) 

# Emacs tags
tags:
	etags `find . -name \*.[ch]` `find $(TOP)/include`

install_headers: include/GL/gl.h
	$(INSTALL) -d $(DESTDIR)$(INSTALL_DIR)/include/GL
	$(INSTALL) -m 644 include/GL/gl.h $(DESTDIR)$(INSTALL_DIR)/include/GL

install_libraries: $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME)
	$(MAKE) -C $(TOP)/src/mesa install-libgl

install: install_libraries

# Remove .o and backup files
clean:
	-rm -f *.o *.a *~
	-rm -f *.c~ *.h~
	-rm -f apple_xgl_api.h apple_xgl_api.c
	-rm -f *.dylib
	-rm -f include/GL/gl.h
	-rm -f $(TOP)/$(LIB_DIR)/$(GL_LIB_GLOB)
	-rm -f *.o *~
	-rm -f depend depend.bak

-include depend
