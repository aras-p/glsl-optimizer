AUTOMAKE_OPTIONS = subdir-objects

include Makefile.sources
include $(top_srcdir)/src/gallium/Automake.inc

AM_CFLAGS = \
	-Wno-packed-bitfield-compat \
	-I$(top_srcdir)/src/gallium/drivers/freedreno/ir3 \
	$(GALLIUM_DRIVER_CFLAGS) \
	$(FREEDRENO_CFLAGS)

noinst_LTLIBRARIES = libfreedreno.la

libfreedreno_la_SOURCES = \
	$(C_SOURCES) \
	$(a2xx_SOURCES) \
	$(a3xx_SOURCES) \
	$(ir3_SOURCES)

noinst_PROGRAMS = ir3_compiler

ir3_compiler_SOURCES = \
	ir3/ir3_cmdline.c

ir3_compiler_LDADD = \
	libfreedreno.la \
	../../auxiliary/libgallium.la \
	$(top_builddir)/src/util/libmesautil.la \
	$(GALLIUM_COMMON_LIB_DEPS) \
	$(FREEDRENO_LIBS)

EXTRA_DIST = Android.mk
