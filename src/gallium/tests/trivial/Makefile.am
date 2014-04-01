include $(top_srcdir)/src/gallium/Automake.inc

PIPE_SRC_DIR = $(top_builddir)/src/gallium/targets/pipe-loader

AM_CFLAGS = \
	$(GALLIUM_CFLAGS)

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/gallium/drivers \
	-I$(top_srcdir)/src/gallium/winsys \
	-DPIPE_SEARCH_DIR=\"$(PIPE_SRC_DIR)/.libs\" \
	$(GALLIUM_PIPE_LOADER_DEFINES)

LDADD = $(GALLIUM_PIPE_LOADER_CLIENT_LIBS) \
	$(top_builddir)/src/gallium/auxiliary/pipe-loader/libpipe_loader_client.la \
	$(top_builddir)/src/gallium/winsys/sw/null/libws_null.la \
	$(top_builddir)/src/gallium/auxiliary/libgallium.la \
	$(GALLIUM_COMMON_LIB_DEPS)

if HAVE_DRI
LDADD += \
	$(top_builddir)/src/gallium/winsys/sw/dri/libswdri.la
endif

if NEED_WINSYS_XLIB
LDADD += \
	$(top_builddir)/src/gallium/winsys/sw/xlib/libws_xlib.la \
	-lX11 -lXext -lXfixes \
	$(LIBDRM_LIBS)
endif

noinst_PROGRAMS = compute tri quad-tex

compute_SOURCES = compute.c

tri_SOURCES = tri.c

quad_tex_SOURCES = quad-tex.c

clean-local:
	-rm -f result.bmp
