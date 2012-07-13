include Makefile.sources
include $(top_srcdir)/src/gallium/Automake.inc

noinst_LTLIBRARIES = libr300.la
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

libr300_la_SOURCES = \
	$(C_SOURCES) \
	$(top_srcdir)/src/glsl/ralloc.c \
	$(top_srcdir)/src/mesa/program/register_allocate.c

#XXX: Delete this when all r300 targets are converted to automake.
all-local: libr300.la
	ln -f $(builddir)/.libs/libr300.a $(builddir)/libr300.a
