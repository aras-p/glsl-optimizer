# src/mesa/drivers/dri/sis/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = sis_dri.so

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
# MINIGLX_SOURCES = server/sis_dri.c 

COMMON_SOURCES = \
	../../common/driverfuncs.c \
	../common/mm.c \
	../common/utils.c \
	../common/texmem.c \
	../common/vblank.c \
	../common/xmlconfig.c

DRIVER_SOURCES = \
	sis_alloc.c \
	sis_clear.c \
	sis_context.c \
	sis_dd.c \
	sis_fog.c \
	sis_lock.c \
	sis_screen.c \
	sis_span.c \
	sis_state.c \
	sis_stencil.c \
	sis_tex.c \
	sis_texstate.c \
	sis_tris.c \
	sis_vb.c

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
