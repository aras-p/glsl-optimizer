# src/mesa/drivers/dri/mach64/Makefile

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

# Not yet
# MINIGLX_SOURCES = server/mach64_dri.c 

DRIVER_SOURCES = \
		mach64_context.c \
		mach64_ioctl.c \
		mach64_screen.c \
		mach64_span.c \
		mach64_state.c \
		mach64_tex.c \
		mach64_texmem.c \
		mach64_texstate.c \
		mach64_tris.c \
		mach64_vb.c \
		mach64_dd.c \
		mach64_lock.c \
		../../common/driverfuncs.c \
                ../common/mm.c \
                ../common/utils.c \
                ../common/texmem.c \
                ../common/vblank.c

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

default: depend mach64_dri.so

mach64_dri.so:  $(OBJECTS) $(MESA_MODULES) $(WINOBJ) Makefile.solo
	rm -f $@ && gcc -o $@ -shared $(OBJECTS) $(MESA_MODULES) $(WINOBJ) $(WINLIB) -lc $(GL_LIB_DEPS)
	rm -f $(LIB_DIR)/mach64_dri.so && \
	install mach64_dri.so $(LIB_DIR)/mach64_dri.so

$(LIB_DIR)/mach64_dri.so: mach64_dri.so
	rm -f $(LIB_DIR)/mach64_dri.so && \
	install mach64_dri.so $(LIB_DIR)/mach64_dri.so

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
