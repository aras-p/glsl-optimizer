# src/egl/drivers/dri/Makefile

TOP = ../../../..
include $(TOP)/configs/current


### Include directories
INCLUDE_DIRS = \
	-I. \
	-I$(DRM_SOURCE_PATH)/shared-core \
	-I$(DRM_SOURCE_PATH)/libdrm \
	-I$(TOP)/include \
	-I$(TOP)/include/GL/internal \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/mesa/main \
	-I$(TOP)/src/mesa/glapi \
	-I$(TOP)/src/mesa/math \
	-I$(TOP)/src/mesa/transform \
	-I$(TOP)/src/mesa/shader \
	-I$(TOP)/src/mesa/swrast \
	-I$(TOP)/src/mesa/swrast_setup \
	-I$(TOP)/src/egl/main \
	-I$(TOP)/src/mesa/drivers/dri/common


HEADERS = egldri.h

SOURCES = egldri.c \
          $(DRM_SOURCE_PATH)/libdrm/xf86drm.c \
          $(DRM_SOURCE_PATH)/libdrm/xf86drmHash.c \
          $(DRM_SOURCE_PATH)/libdrm/xf86drmRandom.c

OBJECTS = $(SOURCES:.c=.o)


.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@



default: depend library Makefile


# EGLdri Library
library: $(LIB_DIR)/libEGLdri.so

$(LIB_DIR)/libEGLdri.so: $(OBJECTS)
	$(TOP)/bin/mklib -o EGLdri -major 1 -minor 0 \
		-install $(LIB_DIR) -ldl $(OBJECTS)


clean:
	rm -f *.o
	rm -f *.so

depend: $(SOURCES) $(HEADERS)
	@ echo "running $(MKDEP)"
	@ touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(DEFINES) $(INCLUDE_DIRS) \
		$(SOURCES) $(HEADERS) > /dev/null 

include depend
# DO NOT DELETE

