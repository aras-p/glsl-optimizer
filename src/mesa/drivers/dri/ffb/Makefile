# src/mesa/drivers/dri/ffb/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = ffb_dri.so

DEFINES += \
	-D_HAVE_SWRAST=1 \
	-D_HAVE_SWTNL=1 \
	-D_HAVE_SANITY=1 \
	-D_HAVE_CODEGEN=1 \
	-D_HAVE_LIGHTING=1 \
	-D_HAVE_TEXGEN=1 \
	-D_HAVE_USERCLIP=1 \
	-DGLX_DIRECT_RENDERING

# not yet
# MINIGLX_SOURCES = server/ffb_dri.c 

COMMON_SOURCES = \
	../../common/driverfuncs.c \
	../common/mm.c \
	../common/utils.c \
	../common/texmem.c \
	../common/vblank.c \
	../common/xmlconfig.c \

DRIVER_SOURCES = \
	ffb_bitmap.c \
	ffb_clear.c \
	ffb_dd.c \
	ffb_depth.c \
	ffb_fog.c \
	ffb_lines.c \
	ffb_points.c \
	ffb_span.c \
	ffb_state.c \
	ffb_stencil.c \
	ffb_tex.c \
	ffb_tris.c \
	ffb_vb.c \
	ffb_vtxfmt.c \
	ffb_xmesa.c

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(MINIGLX_SOURCES) \
	$(DRIVER_SOURCES)

# Include directories
INCLUDE_DIRS = \
	-I. \
	-I../common \
	-Iserver \
	-I$(TOP)/src/glx/mini \
	-I$(TOP)/include \
	-I$(TOP)/src/mesa/drivers/dri/drm/shared \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/mesa/main \
	-I$(TOP)/src/mesa/glapi \
	-I$(TOP)/src/mesa/math \
	-I$(TOP)/src/mesa/transform \
	-I$(TOP)/src/mesa/shader \
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


##### RULES #####

.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $(DEFINES) $< -o $@

.S.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $(DEFINES)  $< -o $@


##### TARGETS #####

default: depend $(LIB_DIR)/$(LIBNAME)


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


include depend
