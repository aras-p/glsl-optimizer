# src/mesa/drivers/dri/radeon/Makefile
# Note, this Makefile requires GNU make

TOP = ../../../../..
include $(TOP)/configs/current

ifeq ($(EMBEDDED),true)
LIBNAME = radeon_es_dri.so
DEFINES += \
	-D_EMBEDDED \
	-D_HAVE_SWRAST=0 \
	-D_HAVE_SWTNL=0 \
	-D_HAVE_SANITY=0 \
	-D_HAVE_CODEGEN=0 \
	-D_HAVE_LIGHTING=0 \
	-D_HAVE_TEXGEN=0 \
	-D_HAVE_USERCLIP=0 \
	-DGLX_DIRECT_RENDERING
else
LIBNAME = radeon_dri.so
DEFINES += \
	-D_HAVE_SWRAST=1 \
	-D_HAVE_SWTNL=1 \
	-D_HAVE_SANITY=1 \
	-D_HAVE_CODEGEN=1 \
	-D_HAVE_LIGHTING=1 \
	-D_HAVE_TEXGEN=1 \
	-D_HAVE_USERCLIP=1 \
	-DGLX_DIRECT_RENDERING
endif

MINIGLX_SOURCES = server/radeon_dri.c 

COMMON_SOURCES = \
	../../common/driverfuncs.c \
	../common/mm.c \
	../common/utils.c \
	../common/texmem.c \
	../common/vblank.c \
	../common/xmlconfig.c

DRIVER_SOURCES = \
	radeon_context.c \
	radeon_ioctl.c \
	radeon_lock.c \
	radeon_screen.c \
	radeon_state.c \
	radeon_state_init.c

SUBSET_DRIVER_SOURCES = \
	radeon_subset_bitmap.c \
	radeon_subset_readpix.c \
	radeon_subset_select.c \
	radeon_subset_tex.c \
	radeon_subset_vtx.c 

FULL_DRIVER_SOURCES = \
	radeon_tex.c \
	radeon_texmem.c \
	radeon_texstate.c \
	radeon_tcl.c \
	radeon_swtcl.c \
	radeon_span.c \
	radeon_maos.c \
	radeon_sanity.c \
	radeon_compat.c \
	radeon_vtxfmt.c \
	radeon_vtxfmt_c.c \
	radeon_vtxfmt_sse.c \
	radeon_vtxfmt_x86.c 

ifeq ($(EMBEDDED),true)
C_SOURCES = \
	$(COMMON_SOURCES) \
	$(MINIGLX_SOURCES) \
	$(DRIVER_SOURCES) \
	$(SUBSET_DRIVER_SOURCES)
else
C_SOURCES = \
	$(COMMON_SOURCES) \
	$(MINIGLX_SOURCES) \
	$(DRIVER_SOURCES) \
	$(FULL_DRIVER_SOURCES)
endif


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
	-I$(TOP)/src/mesa/shader \
	-I$(TOP)/src/mesa/swrast \
	-I$(TOP)/src/mesa/swrast_setup

# Core Mesa objects
MESA_MODULES = $(TOP)/src/mesa/mesa.a

# Libraries that the driver shared lib depends on
LIB_DEPS = $(GL_LIB_DEPS)


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
		$(WINLIB) $(LIB_DEPS) $(WINOBJ) $(OBJECTS) $(MESA_MODULES)


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
