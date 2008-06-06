# src/gallium/winsys/egl_xlib/Makefile

# Build softpipe/xlib/EGL driver library/object: "softpipe_egl.so"


TOP = ../../../..
include $(TOP)/configs/current


DRIVER_NAME = softpipe_egl.so


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
	$(TOP)/src/mesa/libglapi.a \
	$(TOP)/src/mesa/libmesa.a \
	$(GALLIUM_AUXILIARIES)


LOCAL_CFLAGS = -D_EGL_PLATFORM_X=1


.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $(LOCAL_CFLAGS) $< -o $@


.PHONY: library


default: depend library Makefile


library: $(TOP)/$(LIB_DIR)/$(DRIVER_NAME)


# Make the softpipe_egl.so library
$(TOP)/$(LIB_DIR)/$(DRIVER_NAME): $(WINSYS_OBJECTS) $(LIBS)
	$(TOP)/bin/mklib -o $(DRIVER_NAME) \
		-linker "$(CC)" \
		-noprefix \
		-install $(TOP)/$(LIB_DIR) \
		$(MKLIB_OPTIONS) $(WINSYS_OBJECTS) \
		--start-group $(LIBS) --end-group


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
