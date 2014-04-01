AUTOMAKE_OPTIONS = subdir-objects

include Makefile.sources
include $(top_srcdir)/src/gallium/Automake.inc

AM_CFLAGS = \
	-I$(top_srcdir)/src/mesa/program \
	-I$(top_srcdir)/src/mesa \
	-I$(top_srcdir)/src/glsl \
	-I$(top_srcdir)/src/mapi \
	$(GALLIUM_DRIVER_CFLAGS) \
	$(LLVM_CFLAGS) \
	$(RADEON_CFLAGS)

noinst_LTLIBRARIES = libr300.la libr300-helper.la
check_PROGRAMS = r300_compiler_tests
TESTS = r300_compiler_tests

r300_compiler_tests_LDADD = libr300.la libr300-helper.la \
	$(top_builddir)/src/gallium/auxiliary/libgallium.la \
	$(GALLIUM_COMMON_LIB_DEPS)
r300_compiler_tests_CPPFLAGS = \
	-I$(top_srcdir)/src/gallium/drivers/r300/compiler
r300_compiler_tests_SOURCES = $(COMPILER_TESTS_SOURCES)

libr300_la_SOURCES = $(C_SOURCES)

# These two files are included in libmesagallium, which is included in the dri
# targets. So, they were added directly to r300g the dri-r300 target would have
# duplicated symbols, and if they weren't the other *-r300 targets would fail
# with undefined symbols.
#
# Solve this by building them into a separate helper library that can be linked
# in place of libmesagallium.
libr300_helper_la_SOURCES = $(HELPER_SOURCES)
