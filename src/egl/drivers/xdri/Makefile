# src/egl/drivers/xdri/Makefile

# Build XEGL DRI driver loader library:  egl_xdri.so


TOP = ../../../..
include $(TOP)/configs/current


DRIVER_NAME = egl_xdri.so


INCLUDE_DIRS = \
	-I. \
	-I/usr/include \
	$(shell pkg-config --cflags-only-I libdrm) \
	-I$(TOP)/include \
	-I$(TOP)/include/GL/internal \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/mesa/glapi \
	-I$(TOP)/src/egl/main \
	-I$(TOP)/src/glx/x11

HEADERS = glxinit.h driinit.h
SOURCES = egl_xdri.c glxinit.c driinit.c

DRI_SOURCES = dri_common.c XF86dri.c dri2.c dri2_glx.c dri_glx.c
DRI_SOURCES := $(addprefix ../../../glx/x11/,$(DRI_SOURCES))

SOURCES += $(DRI_SOURCES)

OBJECTS = $(SOURCES:.c=.o)

DRM_LIB = `pkg-config --libs libdrm`

CFLAGS += -DGLX_DIRECT_RENDERING

.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@


.PHONY: library


default: depend library Makefile


library: $(TOP)/$(LIB_DIR)/$(DRIVER_NAME)


# Make the egl_xdri.so library
$(TOP)/$(LIB_DIR)/$(DRIVER_NAME): $(OBJECTS)
	$(TOP)/bin/mklib -o $(DRIVER_NAME) \
		-noprefix \
		-major 1 -minor 0 \
		-L$(TOP)/$(LIB_DIR) \
		-install $(TOP)/$(LIB_DIR) \
		$(OBJECTS) $(DRM_LIB) $(GL_LIB_DEPS)

install:
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)
	$(MINSTALL) $(TOP)/$(LIB_DIR)/$(DRIVER_NAME) $(DESTDIR)$(INSTALL_LIB_DIR)

clean:
	rm -f *.o
	rm -f *.so
	rm -f depend depend.bak


depend: $(SOURCES) $(HEADERS)
	@ echo "running $(MKDEP)"
	@ rm -f depend
	@ touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(DEFINES) $(INCLUDE_DIRS) \
		$(SOURCES) $(HEADERS) >/dev/null 2>/dev/null

include depend
# DO NOT DELETE
