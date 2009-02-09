# src/gallium/winsys/egl_xlib/Makefile

# Build softpipe/xlib/EGL driver library/object: "egl_softpipe.so"


TOP = ../../../..
include $(TOP)/configs/current


DRIVER_NAME = egl_softpipe.so


INCLUDE_DIRS = \
	-I$(TOP)/include \
	-I$(TOP)/src/egl/main \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/mesa/main \
	-I$(TOP)/src/gallium/include \
	-I$(TOP)/src/gallium/drivers \
	-I$(TOP)/src/gallium/auxiliary

WINSYS_SOURCES = \
	egl_xlib.c \
	sw_winsys.c

WINSYS_OBJECTS = $(WINSYS_SOURCES:.c=.o)


LIBS = \
	$(GALLIUM_DRIVERS) \
	$(GALLIUM_AUXILIARIES)

# XXX temporary (should create a separate lib with the GL API funcs and
# mesa code, as done for ES 1.x, 2.x, OpenVG, etc)
UNUSED_LIBS = \
	$(TOP)/src/mesa/libglapi.a \
	$(TOP)/src/mesa/libmesa.a \


LOCAL_CFLAGS = -D_EGL_PLATFORM_X=1


.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $(LOCAL_CFLAGS) $< -o $@


.PHONY: library


default: depend library Makefile


library: $(TOP)/$(LIB_DIR)/$(DRIVER_NAME)


# Make the egl_softpipe.so library
$(TOP)/$(LIB_DIR)/$(DRIVER_NAME): $(WINSYS_OBJECTS) $(LIBS)
	$(TOP)/bin/mklib -o $(DRIVER_NAME) \
		-linker "$(CC)" \
		-noprefix \
		-install $(TOP)/$(LIB_DIR) \
		$(MKLIB_OPTIONS) $(WINSYS_OBJECTS) \
		--whole-archive $(LIBS) --no-whole-archive


depend: $(ALL_SOURCES)
	@ echo "running $(MKDEP)"
	@ rm -f depend  # workaround oops on gutsy?!?
	@ touch depend
	@ $(MKDEP) $(MKDEP_OPTIONS) $(DEFINES) $(INCLUDE_DIRS) $(ALL_SOURCES) \
		> /dev/null 2>/dev/null


install: default
	$(INSTALL) -d $(INSTALL_DIR)/$(LIB_DIR)
	@if [ -e $(TOP)/$(LIB_DIR) ]; then \
		$(INSTALL) $(TOP)/$(LIB_DIR)/$(DRIVER_NAME) $(INSTALL_DIR)/$(LIB_DIR); \
	fi


# Emacs tags
tags:
	etags `find . -name \*.[ch]` $(TOP)/include/GL/*.h

clean:
	-rm -f *.o *~ *.bak


include depend
