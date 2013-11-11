AM_CFLAGS = -I$(top_srcdir)/src/egl/main \
	    -I$(top_srcdir)/include \
	    $(DEFINES) \
	    $(VISIBILITY_CFLAGS) \
	    $(WAYLAND_CFLAGS) 

noinst_LTLIBRARIES = libwayland-drm.la
libwayland_drm_la_SOURCES = \
	wayland-drm.c \
	wayland-drm.h \
	wayland-drm-protocol.c
noinst_HEADERS = wayland-drm.h

EXTRA_DIST = wayland-drm.xml

BUILT_SOURCES = wayland-drm-protocol.c \
		wayland-drm-client-protocol.h \
		wayland-drm-server-protocol.h
CLEANFILES = $(BUILT_SOURCES)

%-protocol.c : %.xml
	$(AM_V_GEN)$(WAYLAND_SCANNER) code < $< > $@

%-server-protocol.h : %.xml
	$(AM_V_GEN)$(WAYLAND_SCANNER) server-header < $< > $@

%-client-protocol.h : %.xml
	$(AM_V_GEN)$(WAYLAND_SCANNER) client-header < $< > $@
