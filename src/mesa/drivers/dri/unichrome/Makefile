# src/mesa/drivers/dri/unichrome/Makefile

TOP = ../../../../..
include $(TOP)/configs/default

SHARED_INCLUDES = $(INCLUDE_DIRS) -I. -I../common -Iserver
MINIGLX_INCLUDES = -I$(TOP)/src/glx/mini

DEFINES += \
	-D_HAVE_SWRAST=1 \
	-D_HAVE_SWTNL=1 \
	-D_HAVE_SANITY=1 \
	-D_HAVE_CODEGEN=1 \
	-D_HAVE_LIGHTING=1 \
	-D_HAVE_TEXGEN=1 \
	-D_HAVE_USERCLIP=1 \
	-DGLX_DIRECT_RENDERING 

MINIGLX_SOURCES = server/via_dri.c 

DRIVER_SOURCES = \
		via_context.c \
		via_fb.c \
		via_ioctl.c \
		via_render.c \
		via_screen.c \
		via_span.c \
		via_state.c \
		via_tex.c \
		via_texmem.c \
		via_texstate.c \
		via_tris.c \
		via_vb.c \
		xf86drmVIA.c \
                ../common/mm.c \
                ../common/utils.c \
                ../common/texmem.c \
                ../common/vblank.c \
		../common/xmlconfig.c \
		../../common/driverfuncs.c

INCLUDES = $(MINIGLX_INCLUDES) \
	   $(SHARED_INCLUDES)


C_SOURCES = $(DRIVER_SOURCES) \
	    $(MINIGLX_SOURCES) 

MESA_MODULES = $(TOP)/src/mesa/mesa.a


ifeq ($(WINDOW_SYSTEM),dri)
WINOBJ=$(MESABUILDDIR)/dri/dri.a
WINLIB=
else
WINOBJ=
WINLIB=-L$(MESA)/src/glx/mini
endif

ASM_SOURCES = 
OBJECTS = $(C_SOURCES:.c=.o) \
	  $(ASM_SOURCES:.S=.o) 

### Include directories

INCLUDE_DIRS = \
	-I$(TOP)/include \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/mesa/main \
	-I$(TOP)/src/mesa/glapi \
	-I$(TOP)/src/mesa/math \
	-I$(TOP)/src/mesa/transform \
	-I$(TOP)/src/mesa/swrast \
	-I$(TOP)/src/mesa/swrast_setup


##### RULES #####

.c.o:
	$(CC) -c $(SHARED_INCLUDES) $(MINIGLX_INCLUDES) $(CFLAGS) $(DEFINES) $< -o $@

.S.o:
	$(CC) -c $(SHARED_INCLUDES) $(MINIGLX_INCLUDES) $(CFLAGS) $(DEFINES)  $< -o $@


##### TARGETS #####

default: depend unichrome_dri.so

unichrome_dri.so:  $(OBJECTS) $(MESA_MODULES) $(WINOBJ) Makefile.solo
	rm -f $@ && gcc -o $@ -shared $(OBJECTS) $(MESA_MODULES) $(WINOBJ) $(WINLIB) -lc $(GL_LIB_DEPS)
	rm -f $(LIB_DIR)/unichrome_dri.so && \
	install unichrome_dri.so $(LIB_DIR)/unichrome_dri.so

$(LIB_DIR)/unichrome_dri.so:	unichrome_dri.so
	rm -f $(LIB_DIR)/unichrome_dri.so && \
	install unichrome_dri.so $(LIB_DIR)/unichrome_dri.so

# Run 'make -f Makefile.solo dep' to update the dependencies if you change
# what's included by any source file.
depend: $(C_SOURCES) $(ASM_SOURCES)
	touch depend
	makedepend -fdepend -Y $(SHARED_INCLUDES) $(MINIGLX_INCLUDES) \
		$(C_SOURCES) $(ASM_SOURCES) >& /dev/null


# Emacs tags
tags:
	etags `find . -name \*.[ch]` `find ../include`


# Remove .o and backup files
clean:
	-rm -f *.o */*.o *~ *.o *~ *.so server/*.o


include depend
