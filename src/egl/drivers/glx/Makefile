# src/egl/drivers/glx/Makefile

# Build XEGL DRI driver loader library:  egl_glx.so


TOP = ../../../..
include $(TOP)/configs/current


EXTRA_DEFINES = -DDEFAULT_DRIVER_DIR=\"$(DRI_DRIVER_SEARCH_DIR)\"

DRIVER_NAME = egl_glx.so


INCLUDE_DIRS = \
	-I. \
	-I/usr/include \
	$(shell pkg-config --cflags-only-I libdrm) \
	-I$(TOP)/include \
	-I$(TOP)/include/GL/internal \
	-I$(TOP)/src/mesa/glapi \
	-I$(TOP)/src/mesa/drivers/dri/common \
	-I$(TOP)/src/mesa/main \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/egl/main \
	-I$(TOP)/src/glx/x11

SOURCES = egl_glx.c

OBJECTS = $(SOURCES:.c=.o)

DRM_LIB = `pkg-config --libs libdrm`

MISC_LIBS = -ldl -lXext -lGL


.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $(EXTRA_DEFINES) $< -o $@


.PHONY: library


default: depend library Makefile


library: $(TOP)/$(LIB_DIR)/$(DRIVER_NAME)


# Make the egl_glx.so library
$(TOP)/$(LIB_DIR)/$(DRIVER_NAME): $(OBJECTS)
	$(TOP)/bin/mklib -o $(DRIVER_NAME) \
		-noprefix \
		-major 1 -minor 0 \
		-L$(TOP)/$(LIB_DIR) \
		-install $(TOP)/$(LIB_DIR) \
		$(OBJECTS) $(DRM_LIB) $(MISC_LIBS)

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
