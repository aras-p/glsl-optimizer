AUTOMAKE_OPTIONS = subdir-objects

AM_CPPFLAGS = $(DEFINES) \
	$(VISIBILITY_CFLAGS) \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/loader \
	-I$(top_srcdir)/src/gallium/include \
	-I$(top_srcdir)/src/gallium/auxiliary \
	-I$(top_srcdir)/src/gallium/winsys

noinst_LTLIBRARIES =

if HAVE_LOADER_GALLIUM
noinst_LTLIBRARIES += libpipe_loader.la
noinst_LTLIBRARIES += libpipe_loader_client.la

COMMON_SOURCES = \
	pipe_loader.h \
	pipe_loader_priv.h \
	pipe_loader.c \
	pipe_loader_sw.c

if HAVE_DRM_LOADER_GALLIUM
AM_CFLAGS = $(LIBDRM_CFLAGS)

COMMON_SOURCES += pipe_loader_drm.c

COMMON_LIBADD = \
	$(top_builddir)/src/loader/libloader.la

endif

libpipe_loader_la_CFLAGS  = \
	$(GALLIUM_PIPE_LOADER_DEFINES) \
	$(AM_CFLAGS) $(AM_CPPFLAGS)
libpipe_loader_la_SOURCES = $(COMMON_SOURCES)
libpipe_loader_la_LIBADD  = $(COMMON_LIBADD) \
	$(GALLIUM_PIPE_LOADER_LIBS)

libpipe_loader_client_la_CFLAGS  = \
	$(GALLIUM_PIPE_LOADER_CLIENT_DEFINES) \
	$(AM_CFLAGS) $(AM_CPPFLAGS)
libpipe_loader_client_la_SOURCES = $(COMMON_SOURCES)
libpipe_loader_client_la_LIBADD  = $(COMMON_LIBADD) \
	$(GALLIUM_PIPE_LOADER_CLIENT_LIBS)

endif
