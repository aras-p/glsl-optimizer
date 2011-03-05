# src/egl/wayland/wayland-egl/Makefile

TOP = ../../../..
include $(TOP)/configs/current

INCLUDE_DIRS = -I$(TOP)/include \
	       -I$(TOP)/include/EGL \
	       -I$(TOP)/src/egl/wayland/wayland-drm


HEADERS = wayland-egl-priv.h
SOURCES = wayland-egl.c

OBJECTS = $(SOURCES:.c=.o)

LOCAL_CFLAGS = $(LIBDRM_CFLAGS) \
	       $(WAYLAND_CFLAGS)

LOCAL_LIBS =

.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $(LOCAL_CFLAGS) $< -o $@


default: depend library

# wayland-egl Library
library: $(TOP)/$(LIB_DIR)/$(WAYLAND_EGL_LIB_NAME)

$(TOP)/$(LIB_DIR)/$(WAYLAND_EGL_LIB_NAME): $(OBJECTS) $(LOCAL_LIBS)
	$(MKLIB) -o $(WAYLAND_EGL_LIB) -linker '$(CC)' -ldflags '$(LDFLAGS)' \
		-install $(TOP)/$(LIB_DIR) $(MKLIB_OPTIONS) \
		-L$(TOP)/$(LIB_DIR) $(WAYLAND_EGL_LIB_DEPS) \
		$(OBJECTS) $(LOCAL_LIBS)

PKG_CONFIG_DIR = $(INSTALL_LIB_DIR)/pkgconfig

gl_pcedit = sed \
	-e 's,@INSTALL_DIR@,$(INSTALL_DIR),' \
	-e 's,@INSTALL_LIB_DIR@,$(INSTALL_LIB_DIR),' \
	-e 's,@INSTALL_INC_DIR@,$(INSTALL_INC_DIR),' \
	-e 's,@VERSION@,$(MESA_MAJOR).$(MESA_MINOR).$(MESA_TINY),' \
	-e 's,@WAYLAND_EGL_PC_REQ_PRIV@,$(WAYLAND_EGL_PC_REQ_PRIV),' \
	-e 's,@WAYLAND_EGL_PC_LIB_PRIV@,$(WAYLAND_EGL_PC_LIB_PRIV),' \
	-e 's,@WAYLAND_EGL_PC_CFLAGS@,$(WAYLAND_EGL_PC_CFLAGS),' \
	-e 's,@WAYLAND_EGL_LIB@,$(WAYLAND_EGL_LIB),'

wayland-egl.pc: wayland-egl.pc.in
	$(gl_pcedit) $< > $@

install: default wayland-egl.pc
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)
	$(MINSTALL) $(TOP)/$(LIB_DIR)/$(WAYLAND_EGL_LIB_GLOB) \
		$(DESTDIR)$(INSTALL_LIB_DIR)
	$(INSTALL) -d $(DESTDIR)$(PKG_CONFIG_DIR)
	$(INSTALL) -m 644 wayland-egl.pc $(DESTDIR)$(PKG_CONFIG_DIR)

clean:
	-rm -f *.o
	-rm -f depend depend.bak

depend: $(SOURCES) $(HEADERS)
	@ echo "running $(MKDEP)"
	@ rm -f depend
	@ touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(DEFINES) $(INCLUDE_DIRS) \
		$(SOURCES) $(HEADERS) > /dev/null 2>/dev/null


-include depend
# DO NOT DELETE
