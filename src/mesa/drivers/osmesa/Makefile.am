

# Copyright © 2012 Matt Turner <mattst88@gmail.com>
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

# Hack to make some of the non-automake variables work.
TOP = $(top_srcdir)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = osmesa.pc

AM_CPPFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/mapi \
	-I$(top_srcdir)/src/mesa \
	-I$(top_srcdir)/src/mesa/main

LIBADD = \
	$(top_srcdir)/src/mesa/libmesa.a \
	$(top_srcdir)/src/mapi/glapi/libglapi.a \
	$(top_srcdir)/src/glsl/libglsl.la \
	$(OSMESA_LIB_DEPS)

if BUILD_SHARED
LIBOSMESA = lib$(OSMESA_LIB).so
endif

if BUILD_STATIC
LIBOSMESA = lib$(OSMESA_LIB).a
endif

if HAVE_OSMESA8
lib_LTLIBRARIES = libOSMesa.la

libOSMesa_la_SOURCES = osmesa.c
libOSMesa_la_LDFLAGS = -no-undefined -version-info 8:0:0 #FIXME
libOSMesa_la_LIBADD = $(LIBADD)
endif

if HAVE_OSMESA16
lib_LTLIBRARIES = libOSMesa16.la

libOSMesa16_la_SOURCES = osmesa.c
libOSMesa16_la_LDFLAGS = -no-undefined -version-info 8:0:0 # FIXME
libOSMesa16_la_LIBADD = $(LIBADD)
endif

if HAVE_OSMESA32
lib_LTLIBRARIES = libOSMesa32.la

libOSMesa32_la_SOURCES = osmesa.c
libOSMesa32_la_LDFLAGS = -no-undefined -version-info 8:0:0 # FIXME
libOSMesa32_la_LIBADD = $(LIBADD)
endif

# Provide compatibility with scripts for the old Mesa build system for
# a while by putting a link to the library into /lib of the build tree.
all-local: lib$(OSMESA_LIB).la
	$(MKDIR_P) $(top_builddir)/$(LIB_DIR);
if BUILD_SHARED
	ln -f .libs/$(LIBOSMESA).8.0.0 $(top_builddir)/$(LIB_DIR)/$(LIBOSMESA).8.0.0;
	ln -f .libs/$(LIBOSMESA).8 $(top_builddir)/$(LIB_DIR)/$(LIBOSMESA).8;
endif
	ln -f .libs/$(LIBOSMESA) $(top_builddir)/$(LIB_DIR)/$(LIBOSMESA);
