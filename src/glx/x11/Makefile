TOP = ../../..
include $(TOP)/configs/current

EXTRA_DEFINES = -DXF86VIDMODE -D_REENTRANT -UIN_DRI_DRIVER \
                -DDEFAULT_DRIVER_DIR=\"$(DRI_DRIVER_SEARCH_DIR)\"

SOURCES = \
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
	  glxhash.c

include $(TOP)/src/mesa/sources

MESA_ASM_API = $(addprefix $(TOP)/src/mesa/, $(ASM_API))
MESA_GLAPI_SOURCES = $(addprefix $(TOP)/src/mesa/, $(GLAPI_SOURCES))
MESA_GLAPI_OBJECTS = $(addprefix $(TOP)/src/mesa/, $(GLAPI_OBJECTS))

OBJECTS = $(SOURCES:.c=.o) $(MESA_GLAPI_OBJECTS)

INCLUDES = -I. \
	-I$(TOP)/include \
	-I$(TOP)/include/GL/internal \
	-I$(TOP)/src/mesa/main \
	-I$(TOP)/src/mesa/glapi \
	$(LIBDRM_CFLAGS) \
	$(X11_INCLUDES)


##### RULES #####

.c.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(EXTRA_DEFINES) $< -o $@

.S.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(EXTRA_DEFINES)  $< -o $@

##### TARGETS #####

default: depend $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME)

# Make libGL
$(TOP)/$(LIB_DIR)/$(GL_LIB_NAME):  $(OBJECTS) Makefile
	$(TOP)/bin/mklib -o $(GL_LIB) -linker '$(CC)' -ldflags '$(LDFLAGS)' \
		-major 1 -minor 2 $(MKLIB_OPTIONS) \
		-install $(TOP)/$(LIB_DIR) $(GL_LIB_DEPS) $(OBJECTS)


depend: $(SOURCES) $(MESA_GLAPI_SOURCES) $(MESA_ASM_API) Makefile
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(INCLUDES) $(SOURCES) \
		$(MESA_GLAPI_SOURCES) $(MESA_ASM_API) 


# Emacs tags
tags:
	etags `find . -name \*.[ch]` `find $(TOP)/include`

pcedit = sed \
	-e 's,@INSTALL_DIR@,$(INSTALL_DIR),' \
	-e 's,@LIB_DIR@,$(LIB_DIR),' \
	-e 's,@VERSION@,$(MESA_MAJOR).$(MESA_MINOR).$(MESA_TINY),'

gl.pc: gl.pc.in
	$(pcedit) $< > $@

install: $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME) gl.pc
	$(INSTALL) -d $(DESTDIR)$(INSTALL_DIR)/include/GL
	$(INSTALL) -d $(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR)
	$(INSTALL) -d $(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR)/pkgconfig
	$(INSTALL) -m 644 $(TOP)/include/GL/*.h $(DESTDIR)$(INSTALL_DIR)/include/GL
	$(INSTALL) $(TOP)/$(LIB_DIR)/libGL* $(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR); \
	$(INSTALL) -m 644 gl.pc $(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR)/pkgconfig

# Remove .o and backup files
clean:
	-rm -f $(TOP)/$(LIB_DIR)/libGL.so*
	-rm -f *.o *~
	-rm -f depend

-include depend
