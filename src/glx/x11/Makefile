TOP = ../../..
include $(TOP)/configs/current

# Context creation has evolved over the years, such that this
# code will not build with DNIO defined.  When we finally drop old interface
# support in libGL, we need to clean up both glxcmds.c and dri_interface.h.

DEFINES += -DGLX_DIRECT_RENDERING -DGLXEXT -DXF86DRI -DGLX_USE_DLOPEN \
	-DGLX_USE_MESA -DXF86VIDMODE -D_REENTRANT -UIN_DRI_DRIVER

C_SOURCES = \
	  $(TOP)/src/mesa/glapi/glapi.c \
	  $(TOP)/src/mesa/glapi/glthread.c \
	  glcontextmodes.c \
	  $(DRM_SOURCE_PATH)/libdrm/xf86drm.c \
	  $(DRM_SOURCE_PATH)/libdrm/xf86drmHash.c \
	  $(DRM_SOURCE_PATH)/libdrm/xf86drmRandom.c \
	  $(DRM_SOURCE_PATH)/libdrm/xf86drmSL.c \
	  clientattrib.c \
	  compsize.c \
	  dispatch.c \
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
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/mesa/main \
	-I$(TOP)/src/mesa/glapi \
	-I$(TOP)/src/mesa/math \
	-I$(TOP)/src/mesa/transform \
	-I$(TOP)/src/mesa/swrast \
	-I$(TOP)/src/mesa/swrast_setup \
	-I$(TOP)/src/mesa/drivers/dri/common \
	-I$(DRM_SOURCE_PATH)/libdrm \
	-I$(DRM_SOURCE_PATH)/shared-core \
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

dispatch.c:
	ln -s $(TOP)/src/mesa/main/dispatch.c .

# Make libGL
$(LIB_DIR)/$(GL_LIB_NAME):  $(OBJECTS) Makefile
	CC="$(CC)" CXX="$(CXX)" $(TOP)/bin/mklib -o $(GL_LIB) -major 1 -minor 2 $(MKLIB_OPTIONS) \
		-install $(LIB_DIR) $(GL_LIB_DEPS) $(OBJECTS)


drmtest: xf86drm.o drmtest.o
	rm -f drmtest && $(CC) -o drmtest xf86drm.o drmtest.o


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
