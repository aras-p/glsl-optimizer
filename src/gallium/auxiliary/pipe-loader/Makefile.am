AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = $(DEFINES) \
	$(GALLIUM_PIPE_LOADER_DEFINES) \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/gallium/include \
	-I$(top_srcdir)/src/gallium/auxiliary \
	-I$(top_srcdir)/src/gallium/winsys

noinst_LTLIBRARIES =

if HAVE_LOADER_GALLIUM
noinst_LTLIBRARIES += libpipe_loader.la

libpipe_loader_la_SOURCES = \
	pipe_loader.h \
	pipe_loader_priv.h \
	pipe_loader.c \
	pipe_loader_sw.c

if HAVE_DRM_LOADER_GALLIUM
libpipe_loader_la_SOURCES += pipe_loader_drm.c
AM_CFLAGS = $(LIBDRM_CFLAGS)
endif
endif
