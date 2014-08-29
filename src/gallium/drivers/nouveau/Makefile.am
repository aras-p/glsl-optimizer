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

AUTOMAKE_OPTIONS = subdir-objects

include Makefile.sources
include $(top_srcdir)/src/gallium/Automake.inc

AM_CPPFLAGS = \
	$(GALLIUM_DRIVER_CFLAGS) \
	$(LIBDRM_CFLAGS) \
	$(NOUVEAU_CFLAGS)

noinst_LTLIBRARIES = libnouveau.la

libnouveau_la_SOURCES = \
	$(C_SOURCES) \
	$(NV30_C_SOURCES) \
	$(NV50_CODEGEN_SOURCES) \
	$(NV50_C_SOURCES) \
	$(NVC0_CODEGEN_SOURCES) \
	$(NVC0_C_SOURCES)

noinst_PROGRAMS = nouveau_compiler

nodist_EXTRA_nouveau_compiler_SOURCES = dummy.cpp
nouveau_compiler_SOURCES = \
	nouveau_compiler.c

nouveau_compiler_LDADD = \
	libnouveau.la \
	../../auxiliary/libgallium.la \
	$(top_builddir)/src/util/libmesautil.la \
	$(GALLIUM_COMMON_LIB_DEPS)

EXTRA_DIST = Android.mk codegen/lib nvc0/mme
