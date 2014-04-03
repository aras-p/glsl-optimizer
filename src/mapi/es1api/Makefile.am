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

AM_CPPFLAGS = \
	$(DEFINES) \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/mapi \
	-DMAPI_MODE_BRIDGE \
	-DMAPI_ABI_HEADER=\"glapi_mapi_tmp.h\"

AM_CFLAGS = $(VISIBILITY_CFLAGS)

TESTS = ABI-check

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = glesv1_cm.pc

GLES_includedir = $(includedir)/GLES
GLES_include_HEADERS = \
	$(top_srcdir)/include/GLES/egl.h \
	$(top_srcdir)/include/GLES/gl.h \
	$(top_srcdir)/include/GLES/glext.h \
	$(top_srcdir)/include/GLES/glplatform.h

lib_LTLIBRARIES = libGLESv1_CM.la

libGLESv1_CM_la_SOURCES = ../entry.c glapi_mapi_tmp.h
libGLESv1_CM_la_LIBADD = $(GLESv1_CM_LIB_DEPS)
libGLESv1_CM_la_LDFLAGS = \
	-no-undefined \
	-version-number 1:1 \
	$(GC_SECTIONS) \
	$(LD_NO_UNDEFINED)

if HAVE_SHARED_GLAPI
libGLESv1_CM_la_LIBADD += ../shared-glapi/libglapi.la
endif

include ../glapi/gen/glapi_gen.mk
glapi_mapi_tmp.h: ../glapi/gen/gl_and_es_API.xml $(glapi_gen_mapi_deps)
	$(call glapi_gen_mapi,$<,es1api)

BUILT_SOURCES = glapi_mapi_tmp.h
CLEANFILES = $(BUILT_SOURCES)

include $(top_srcdir)/install-lib-links.mk
