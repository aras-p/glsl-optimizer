# Copyright © 2012 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
# HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
# WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

include Makefile.sources
include $(top_srcdir)/src/gallium/Automake.inc

AM_CFLAGS = \
	$(GALLIUM_DRIVER_CFLAGS) \
	$(LLVM_CFLAGS)
AM_CXXFLAGS= \
	$(GALLIUM_DRIVER_CXXFLAGS) \
	$(LLVM_CXXFLAGS)

noinst_LTLIBRARIES = libllvmpipe.la

libllvmpipe_la_SOURCES = $(C_SOURCES)

libllvmpipe_la_LDFLAGS = $(LLVM_LDFLAGS)

noinst_HEADERS = lp_test.h

check_PROGRAMS = \
	lp_test_format	\
	lp_test_arit	\
	lp_test_blend	\
	lp_test_conv	\
	lp_test_printf
TESTS = $(check_PROGRAMS)

TEST_LIBS = \
	libllvmpipe.la \
	$(top_builddir)/src/gallium/auxiliary/libgallium.la \
	$(top_builddir)/src/util/libmesautil.la \
	$(LLVM_LIBS) \
	$(DLOPEN_LIBS) \
	$(PTHREAD_LIBS)

lp_test_format_SOURCES = lp_test_format.c lp_test_main.c
lp_test_format_LDADD = $(TEST_LIBS)
nodist_EXTRA_lp_test_format_SOURCES = dummy.cpp

lp_test_arit_SOURCES = lp_test_arit.c lp_test_main.c
lp_test_arit_LDADD = $(TEST_LIBS)
nodist_EXTRA_lp_test_arit_SOURCES = dummy.cpp

lp_test_blend_SOURCES = lp_test_blend.c lp_test_main.c
lp_test_blend_LDADD = $(TEST_LIBS)
nodist_EXTRA_lp_test_blend_SOURCES = dummy.cpp

lp_test_conv_SOURCES = lp_test_conv.c lp_test_main.c
lp_test_conv_LDADD = $(TEST_LIBS)
nodist_EXTRA_lp_test_conv_SOURCES = dummy.cpp

lp_test_printf_SOURCES = lp_test_printf.c lp_test_main.c
lp_test_printf_LDADD = $(TEST_LIBS)
nodist_EXTRA_lp_test_printf_SOURCES = dummy.cpp

EXTRA_DIST = SConscript
