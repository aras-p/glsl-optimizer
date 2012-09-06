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
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

SRCDIR=$(top_srcdir)/src/mesa/
BUILDDIR=$(top_builddir)/src/mesa/
include ../sources.mak
include ../../glsl/Makefile.sources

noinst_PROGRAMS =

AM_CPPFLAGS = \
	$(INCLUDE_DIRS) \
	$(API_DEFINES) \
	$(DEFINES) \
	-DUSE_DRICORE

libdricore@VERSION@_la_SOURCES = \
	$(MESA_FILES) \
	$(LIBGLCPP_GENERATED_FILES) \
	$(LIBGLCPP_FILES) \
	$(LIBGLSL_FILES) \
	$(BUILTIN_COMPILER_GENERATED_CXX_FILES) \
	$(top_builddir)/src/glsl/builtin_function.cpp
libdricore@VERSION@_la_LDFLAGS = -version-number 1:0
libdricore@VERSION@_la_LIBADD = \
        ../program/libdricore_program.la \
        $()

if HAVE_X86_ASM
libdricore@VERSION@_la_SOURCES += $(X86_FILES)
AM_CPPFLAGS += \
        -I$(top_builddir)/src/mesa/x86 \
        -I$(top_srcdir)/src/mesa/x86
endif

if HAVE_X86_64_ASM
libdricore@VERSION@_la_SOURCES += $(X86_64_FILES)
AM_CPPFLAGS += \
        -I$(top_builddir)/src/mesa/x86-64 \
        -I$(top_srcdir)/src/mesa/x86-64
endif

if HAVE_SPARC_ASM
libdricore@VERSION@_la_SOURCES += $(SPARC_FILES)
AM_CPPFLAGS += \
        -I$(top_builddir)/src/mesa/sparc \
        -I$(top_srcdir)/src/mesa/sparc
endif

if HAVE_DRI
lib_LTLIBRARIES = libdricore@VERSION@.la

# Provide compatibility with scripts for the old Mesa build system for
# a while by putting a link to the driver into /lib of the build tree.
all-local: libdricore@VERSION@.la
	$(MKDIR_P) $(top_builddir)/$(LIB_DIR);
	ln -f .libs/libdricore@VERSION@.so.1.0.0 $(top_builddir)/$(LIB_DIR)/libdricore@VERSION@.so.1;
	ln -sf libdricore@VERSION@.so.1 $(top_builddir)/$(LIB_DIR)/libdricore@VERSION@.so
endif

CLEANFILES = \
	$(top_builddir)/$(LIB_DIR)/libdricore@VERSION@.so.1 \
	$(top_builddir)/$(LIB_DIR)/libdricore@VERSION@.so

-include $(DEPENDS)
