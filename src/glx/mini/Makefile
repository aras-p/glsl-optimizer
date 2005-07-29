# Build a subset DRI-based libGL.so library.
# Indirect rendering not supported, etc.

TOP = ../../..
include $(TOP)/configs/current


DEFINES += -DGLX_DIRECT_RENDERING -DIN_MINI_GLX -UIN_DRI_DRIVER

C_SOURCES = \
	  $(TOP)/src/mesa/main/dispatch.c \
	  $(TOP)/src/mesa/glapi/glapi.c \
	  $(TOP)/src/mesa/glapi/glthread.c \
	  $(TOP)/src/mesa/drivers/dri/common/glcontextmodes.c \
	  $(DRM_SOURCE_PATH)/libdrm/xf86drm.c \
	  $(DRM_SOURCE_PATH)/libdrm/xf86drmHash.c \
	  $(DRM_SOURCE_PATH)/libdrm/xf86drmRandom.c \
	  miniglx.c \
	  miniglx_events.c

X86_SOURCES = $(TOP)/src/mesa/x86/glapi_x86.S

OBJECTS = $(C_SOURCES:.c=.o) \
	  $(ASM_SOURCES:.S=.o)

INCLUDES = -I. $(INCLUDE_DIRS)

INCLUDE_DIRS = \
	-I$(TOP)/include \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/mesa/main \
	-I$(TOP)/src/mesa/glapi \
	-I$(TOP)/src/mesa/math \
	-I$(TOP)/src/mesa/transform \
	-I$(TOP)/src/mesa/swrast \
	-I$(TOP)/src/mesa/swrast_setup \
	-I$(TOP)/src/mesa/drivers/dri/common \
	-I$(DRM_SOURCE_PATH)/libdrm \
	-I$(DRM_SOURCE_PATH)/shared


##### RULES #####

.c.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) $< -o $@

.S.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES)  $< -o $@


##### TARGETS #####

default: depend $(LIB_DIR)/$(GL_LIB_NAME)


# Make libGL
$(LIB_DIR)/$(GL_LIB_NAME):  $(OBJECTS) Makefile
	$(TOP)/bin/mklib -o $(GL_LIB) -linker '$(CC)' \
		-major 1 -minor 2 $(MKLIB_OPTIONS) \
		-install $(LIB_DIR) $(GL_LIB_DEPS) $(OBJECTS)
	rm -f $(LIB_DIR)/miniglx.conf
	install example.miniglx.conf $(LIB_DIR)/miniglx.conf


drmtest: xf86drm.o drmtest.o
	rm -f drmtest && $(CC) -o drmtest xf86drm.o drmtest.o


depend: $(C_SOURCES) $(ASM_SOURCES)
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(INCLUDES) $(C_SOURCES) $(ASM_SOURCES) \
		> /dev/null 


# Emacs tags
tags:
	etags `find . -name \*.[ch]` `find ../include`


# Remove .o and backup files
clean:
	-rm -f drmtest $(LIB_DIR)/libGL.so*
	-rm -f *.o *~
	-rm -f depend

include depend
