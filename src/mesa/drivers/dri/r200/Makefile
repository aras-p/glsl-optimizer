# src/mesa/drivers/dri/r200/Makefile
# Note, this Makefile requires GNU make

TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = r200_dri.so

DEFINES += \
	-D_HAVE_SWRAST=1 \
	-D_HAVE_SWTNL=1 \
	-D_HAVE_SANITY=1 \
	-D_HAVE_CODEGEN=1 \
	-D_HAVE_LIGHTING=1 \
	-D_HAVE_TEXGEN=1 \
	-D_HAVE_USERCLIP=1 \
	-DGLX_DIRECT_RENDERING

MINIGLX_SOURCES = server/radeon_dri.c 

COMMON_SOURCES = \
	../../common/driverfuncs.c \
	../common/mm.c \
	../common/utils.c \
	../common/texmem.c \
	../common/vblank.c \
	../common/xmlconfig.c

DRIVER_SOURCES = r200_context.c \
		 r200_ioctl.c \
		 r200_lock.c \
		 r200_screen.c \
		 r200_state.c \
		 r200_state_init.c \
		 r200_cmdbuf.c \
		 r200_pixel.c \
		 r200_tex.c \
		 r200_texmem.c \
		 r200_texstate.c \
		 r200_tcl.c \
		 r200_swtcl.c \
		 r200_span.c \
		 r200_maos.c \
		 r200_sanity.c \
		 r200_vtxfmt.c \
		 r200_vtxfmt_c.c \
		 r200_vtxfmt_sse.c \
		 r200_vtxfmt_x86.c 

C_SOURCES = $(COMMON_SOURCES) $(MINIGLX_SOURCES) $(DRIVER_SOURCES)


# Include directories
INCLUDE_DIRS = \
	-I. \
	-I../common \
	-Iserver \
	-I$(TOP)/src/glx/mini \
	-I$(TOP)/include \
	-I$(DRM_SOURCE_PATH)/shared \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/mesa/main \
	-I$(TOP)/src/mesa/glapi \
	-I$(TOP)/src/mesa/math \
	-I$(TOP)/src/mesa/transform \
	-I$(TOP)/src/mesa/swrast \
	-I$(TOP)/src/mesa/swrast_setup


# Core Mesa objects
MESA_MODULES = $(TOP)/src/mesa/mesa.a


# Libraries that the driver shared lib depends on
LIB_DEPS = -L/usr/X11R6/lib -lX11 -lXext -lm -lpthread -lexpat -ldl -lc


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

SYMLINKS = \
	server/radeon_common.h \
	server/radeon_dri.c \
	server/radeon_dri.h \
	server/radeon.h \
	server/radeon_macros.h \
	server/radeon_reg.h \
	server/radeon_sarea.h \



##### RULES #####

.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $(DEFINES) $< -o $@

.S.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $(DEFINES)  $< -o $@


##### TARGETS #####

default: depend $(SYMLINKS) $(LIB_DIR)/$(LIBNAME)

$(LIB_DIR)/$(LIBNAME): $(OBJECTS) $(MESA_MODULES) $(WINOBJ) Makefile
	$(TOP)/bin/mklib -o $(LIBNAME) -noprefix -install $(LIB_DIR) \
		$(WINLIB) $(LIB_DEPS) $(WINOBJ) $(MESA_MODULES) $(OBJECTS)


$(SYMLINKS):
	mkdir -p server
	cd server
	rm -f $@ && ln -s ../../radeon/$@ $@


depend: $(C_SOURCES) $(ASM_SOURCES)
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(INCLUDE_DIRS) $(C_SOURCES) $(ASM_SOURCES) \
		>& /dev/null


# Emacs tags
tags:
	etags `find . -name \*.[ch]` `find ../include`


clean:
	-rm -f *.o server/*.o
	-rm -f $(SYMLINKS)


include depend
