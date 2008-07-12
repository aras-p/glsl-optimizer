# src/egl/drivers/dri/Makefile

TOP = ../../../..
include $(TOP)/configs/current


### Include directories
INCLUDE_DIRS = \
	-I. \
	-I/usr/include \
	-I/usr/include/drm \
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

SOURCES = egldri.c

OBJECTS = $(SOURCES:.c=.o)


.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@



default: depend library Makefile


# EGLdri Library
library: $(TOP)/$(LIB_DIR)/libEGLdri.so

$(TOP)/$(LIB_DIR)/libEGLdri.so: $(OBJECTS)
	$(MKLIB) -o EGLdri -linker '$(CC)' -ldflags '$(LDFLAGS)' \
		-major 1 -minor 0 \
		-install $(TOP)/$(LIB_DIR) -ldl $(OBJECTS)


clean:
	-rm -f *.o
	-rm -f *.so

depend: $(SOURCES) $(HEADERS)
	@ echo "running $(MKDEP)"
	@ touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(DEFINES) $(INCLUDE_DIRS) \
		$(SOURCES) $(HEADERS) > /dev/null 

include depend
# DO NOT DELETE

