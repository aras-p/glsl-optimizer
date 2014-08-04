# TODO: wire up some useful tests to make check

include $(top_srcdir)/src/gallium/Automake.inc

AM_CFLAGS = \
	$(GALLIUM_CFLAGS)

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/gallium/drivers \
	-I$(top_srcdir)/src/gallium/winsys

LDADD = \
	$(top_builddir)/src/gallium/auxiliary/libgallium.la \
	$(top_builddir)/src/util/libmesautil.la \
	$(top_builddir)/src/gallium/drivers/trace/libtrace.la \
	$(top_builddir)/src/gallium/winsys/sw/null/libws_null.la \
	$(top_builddir)/src/gallium/drivers/softpipe/libsoftpipe.la \
	$(GALLIUM_COMMON_LIB_DEPS)

noinst_PROGRAMS = pipe_barrier_test u_cache_test u_half_test \
	u_format_test u_format_compatible_test translate_test

pipe_barrier_test_SOURCES = pipe_barrier_test.c

u_cache_test_SOURCES = u_cache_test.c

u_half_test_SOURCES = u_half_test.c

u_format_test_SOURCES = u_format_test.c

u_format_compatible_test_SOURCES = u_format_compatible_test.c

translate_test_SOURCES = translate_test.c
