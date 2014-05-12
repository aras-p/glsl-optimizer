noinst_LTLIBRARIES = libappleglx.la

AM_CFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/glx \
	-I$(top_srcdir)/src/mesa \
	-I$(top_builddir)/src/mesa \
	-I$(top_srcdir)/src/mapi \
	-I$(top_srcdir)/src/mapi/glapi \
	-I$(top_builddir)/src/mapi/glapi \
	$(VISIBILITY_CFLAGS) \
	$(SHARED_GLAPI_CFLAGS) \
	$(DEFINES) \
	$(X11_INCLUDES)

libappleglx_la_SOURCES = \
	apple_cgl.c \
	apple_glapi.c \
	apple_glx.c \
	apple_glx_context.c \
	apple_glx_drawable.c \
	apple_glx_log.c \
	apple_glx_pbuffer.c \
	apple_glx_pixmap.c \
	apple_glx_surface.c \
	apple_visual.c \
	apple_xgl_api_read.c \
	apple_xgl_api_stereo.c \
	apple_xgl_api_viewport.c \
	appledri.c \
	glx_empty.c

libappleglx_la_LDFLAGS = -lXplugin -framework ApplicationServices -framework CoreFoundation
