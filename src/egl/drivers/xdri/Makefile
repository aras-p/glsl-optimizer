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
	-I$(TOP)/src/mesa/glapi \
	-I$(TOP)/src/mesa/drivers/dri/common \
	-I$(TOP)/src/egl/main \
	-I$(TOP)/src/glx/x11

SOURCES = egl_xdri.c

OBJECTS = $(SOURCES:.c=.o)

DRM_LIB = `pkg-config --libs libdrm`

MISC_LIBS = -ldl -lXext -lGL


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
		-L $(TOP)/$(LIB_DIR) \
		-install $(TOP)/$(LIB_DIR) \
		$(OBJECTS) $(DRM_LIB) $(MISC_LIBS)


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
