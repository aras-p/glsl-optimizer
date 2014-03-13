pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = wayland-egl.pc

AM_CFLAGS = $(DEFINES) \
	    $(VISIBILITY_CFLAGS) \
	    $(WAYLAND_CFLAGS)

lib_LTLIBRARIES = libwayland-egl.la
noinst_HEADERS = wayland-egl-priv.h
libwayland_egl_la_SOURCES = wayland-egl.c
libwayland_egl_la_LDFLAGS = \
	-no-undefined \
	-version-info 1 \
	-Wl,--no-undefined

TESTS = wayland-egl-symbols-check

include $(top_srcdir)/install-lib-links.mk
