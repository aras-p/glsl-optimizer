TOP = ../../..
include $(TOP)/configs/current

DEFINES += -DXF86VIDMODE -D_REENTRANT -UIN_DRI_DRIVER

C_SOURCES = \
	  $(TOP)/src/mesa/glapi/glapi.c \
	  $(TOP)/src/mesa/glapi/glthread.c \
	  $(TOP)/src/mesa/main/dispatch.c \
	  glcontextmodes.c \
	  clientattrib.c \
	  compsize.c \
	  eval.c \
	  glxcmds.c \
	  glxext.c \
	  glxextensions.c \
	  indirect.c \
	  indirect_init.c \
	  indirect_size.c \
	  indirect_window_pos.c \
	  indirect_transpose_matrix.c \
	  indirect_vertex_array.c \
	  indirect_vertex_program.c \
	  pixel.c \
	  pixelstore.c \
	  render2.c \
	  renderpix.c \
	  single2.c \
	  singlepix.c \
	  vertarr.c \
	  xfont.c \
	  glx_pbuffer.c \
	  glx_query.c \
	  glx_texture_compression.c \
	  dri_glx.c \
	  XF86dri.c \

X86_SOURCES = $(TOP)/src/mesa/x86/glapi_x86.S
X86-64_SOURCES = $(TOP)/src/mesa/x86-64/glapi_x86-64.S

# ASM_SOURCES = $(X86_SOURCES)

OBJECTS = $(C_SOURCES:.c=.o) \
	  $(ASM_SOURCES:.S=.o)

INCLUDES = -I. \
	-I$(TOP)/include \
	-I$(TOP)/include/GL/internal \
	-I$(TOP)/src/mesa/main \
	-I$(TOP)/src/mesa/glapi \
	-I$(TOP)/src/mesa/drivers/dri/common \
        `pkg-config --cflags libdrm` \
	$(X11_INCLUDES)


##### RULES #####

.c.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) $< -o $@

.S.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES)  $< -o $@

##### TARGETS #####

default: depend $(LIB_DIR)/$(GL_LIB_NAME)

glcontextmodes.c:
	ln -s $(TOP)/src/mesa/drivers/dri/common/glcontextmodes.c .

# Make libGL
$(LIB_DIR)/$(GL_LIB_NAME):  $(OBJECTS) Makefile
	$(TOP)/bin/mklib -o $(GL_LIB) -linker '$(CC)' \
		-major 1 -minor 2 $(MKLIB_OPTIONS) \
		-install $(LIB_DIR) $(GL_LIB_DEPS) $(OBJECTS)


depend: $(C_SOURCES) $(ASM_SOURCES) Makefile
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(INCLUDES) $(C_SOURCES) $(ASM_SOURCES) 


# Emacs tags
tags:
	etags `find . -name \*.[ch]` `find ../include`


# Remove .o and backup files
clean:
	-rm -f $(LIB_DIR)/libGL.so*
	-rm -f *.o *~
	-rm -f depend

include depend
