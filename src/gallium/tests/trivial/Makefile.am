include $(top_srcdir)/src/gallium/Automake.inc

PIPE_SRC_DIR = $(top_builddir)/src/gallium/targets/pipe-loader

AM_CFLAGS = \
	$(GALLIUM_CFLAGS)

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/gallium/drivers \
	-I$(top_srcdir)/src/gallium/winsys \
	-DPIPE_SEARCH_DIR=\"$(PIPE_SRC_DIR)/.libs\" \
	$(GALLIUM_PIPE_LOADER_DEFINES)

LDADD = $(GALLIUM_PIPE_LOADER_LIBS) \
	$(top_builddir)/src/gallium/auxiliary/libgallium.la \
	$(LIBUDEV_LIBS) \
	$(DLOPEN_LIBS) \
	$(PTHREAD_LIBS) \
	-lm

noinst_PROGRAMS = compute tri quad-tex

compute_SOURCES = compute.c

tri_SOURCES = tri.c

quad_tex_SOURCES = quad-tex.c

all-local:
	@$(MAKE) -C $(PIPE_SRC_DIR)

clean-local:
	@$(MAKE) -C $(PIPE_SRC_DIR) clean
	-rm -f result.bmp
