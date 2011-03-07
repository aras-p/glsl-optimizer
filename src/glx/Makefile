TOP = ../..
include $(TOP)/configs/current

ifeq ($(HAVE_XF86VIDMODE),yes)
EXTRA_DEFINES_XF86VIDMODE = -DXF86VIDMODE
endif

EXTRA_DEFINES = $(EXTRA_DEFINES_XF86VIDMODE) -D_REENTRANT \
                -DDEFAULT_DRIVER_DIR=\"$(DRI_DRIVER_SEARCH_DIR)\"

SOURCES = \
	  clientattrib.c \
	  compsize.c \
	  eval.c \
	  glxconfig.c \
	  glxcmds.c \
	  glxcurrent.c \
	  glxext.c \
	  glxextensions.c \
	  indirect_glx.c \
	  indirect.c \
	  indirect_init.c \
	  indirect_size.c \
	  indirect_window_pos.c \
	  indirect_texture_compression.c \
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
	  drisw_glx.c \
	  dri_common.c \
	  dri_glx.c \
	  XF86dri.c \
	  glxhash.c \
	  dri2_glx.c \
	  dri2.c \
	  applegl_glx.c


ifeq ($(SHARED_GLAPI),1)
GL_LIB_DEPS := -L$(TOP)/$(LIB_DIR) -l$(GLAPI_LIB) $(GL_LIB_DEPS)
EXTRA_DEFINES += -DGLX_SHARED_GLAPI
endif

# override GLAPI_LIB
GLAPI_LIB = $(TOP)/src/mapi/glapi/libglapi.a

OBJECTS = $(SOURCES:.c=.o)

INCLUDES = -I. \
	-I$(TOP)/include \
	-I$(TOP)/include/GL/internal \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/mapi \
	-I$(TOP)/src/mapi/glapi \
	$(LIBDRM_CFLAGS) \
	$(DRI2PROTO_CFLAGS) \
	$(GLPROTO_CFLAGS) \
	$(X11_INCLUDES)


##### RULES #####

.c.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(EXTRA_DEFINES) $< -o $@

.S.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(EXTRA_DEFINES)  $< -o $@

##### TARGETS #####

default: depend $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME)

# Make libGL
$(TOP)/$(LIB_DIR)/$(GL_LIB_NAME): $(OBJECTS) $(GLAPI_LIB) Makefile
	$(MKLIB) -o $(GL_LIB) -linker '$(CXX)' -ldflags '$(LDFLAGS)' \
		-major 1 -minor 2 \
		-cplusplus $(MKLIB_OPTIONS) \
		-install $(TOP)/$(LIB_DIR) -id $(INSTALL_LIB_DIR)/lib$(GL_LIB).1.dylib \
		$(GL_LIB_DEPS) $(OBJECTS) $(GLAPI_LIB)

$(GLAPI_LIB):
	@$(MAKE) -C $(TOP)/src/mapi/glapi

depend: $(SOURCES) Makefile
	rm -f depend
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(INCLUDES) $(SOURCES)


# Emacs tags
tags:
	etags `find . -name \*.[ch]` `find $(TOP)/include`

install: $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME)
	$(MAKE) -C $(TOP)/src/mesa install-libgl

# Remove .o and backup files
clean:
	-rm -f $(TOP)/$(LIB_DIR)/$(GL_LIB_GLOB)
	-rm -f *.o *~
	-rm -f depend depend.bak

-include depend
