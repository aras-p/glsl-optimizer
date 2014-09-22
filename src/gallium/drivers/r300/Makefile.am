AUTOMAKE_OPTIONS = subdir-objects

include Makefile.sources
include $(top_srcdir)/src/gallium/Automake.inc

AM_CFLAGS = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/mesa/program \
	-I$(top_srcdir)/src/mesa \
	-I$(top_srcdir)/src/glsl \
	-I$(top_srcdir)/src/mapi \
	$(GALLIUM_DRIVER_CFLAGS) \
	$(LLVM_CFLAGS) \
	$(RADEON_CFLAGS)

noinst_LTLIBRARIES = libr300.la
check_PROGRAMS = r300_compiler_tests
TESTS = r300_compiler_tests

r300_compiler_tests_LDADD = libr300.la \
	$(top_builddir)/src/gallium/auxiliary/libgallium.la \
	$(top_builddir)/src/util/libmesautil.la \
	$(GALLIUM_COMMON_LIB_DEPS)
r300_compiler_tests_CPPFLAGS = \
	-DTEST_PATH=\"$(top_srcdir)/src/gallium/drivers/r300/compiler/tests\" \
	-I$(top_srcdir)/src/gallium/drivers/r300/compiler
r300_compiler_tests_SOURCES = $(COMPILER_TESTS_SOURCES)

libr300_la_SOURCES = $(C_SOURCES)

EXTRA_DIST = Android.mk \
	compiler/tests/omod_two_writers.test \
	compiler/tests/regalloc_tex_1d_swizzle.test
