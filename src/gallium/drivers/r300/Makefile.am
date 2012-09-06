include Makefile.sources
include $(top_srcdir)/src/gallium/Automake.inc

noinst_LTLIBRARIES = libr300.la libr300-helper.la
check_PROGRAMS = r300_compiler_tests
testdir = compiler/tests
TESTS = r300_compiler_tests

AM_CFLAGS = \
	-I$(top_srcdir)/src/gallium/drivers \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/mesa \
	-I$(top_srcdir)/src/glsl \
	-I$(top_srcdir)/src/mapi \
	$(VISIBILITY_CFLAGS) \
	$(GALLIUM_CFLAGS) \
	$(LLVM_CFLAGS) \
	$(RADEON_CFLAGS)

r300_compiler_tests_LDADD = libr300.la
r300_compiler_tests_CPPFLAGS = \
	-I$(top_srcdir)/src/gallium/drivers/r300/compiler
r300_compiler_tests_SOURCES = \
	$(testdir)/r300_compiler_tests.c \
	$(testdir)/radeon_compiler_util_tests.c \
	$(testdir)/rc_test_helpers.c \
	$(testdir)/unit_test.c

libr300_la_SOURCES = $(C_SOURCES)

# These two files are included in libmesagallium, which is included in the dri
# targets. So, they were added directly to r300g the dri-r300 target would have
# duplicated symbols, and if they weren't the other *-r300 targets would fail
# with undefined symbols.
#
# Solve this by building them into a separate helper library that can be linked
# in place of libmesagallium.
libr300_helper_la_SOURCES = \
	$(top_srcdir)/src/glsl/ralloc.c \
	$(top_srcdir)/src/mesa/program/register_allocate.c
