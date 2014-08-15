# Copyright © 2013, 2014 Intel Corporation
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

SUBDIRS = glapi/gen .
TESTS =

TOP = $(top_srcdir)

BUILT_SOURCES =
CLEANFILES = $(BUILT_SOURCES)

lib_LTLIBRARIES =
check_PROGRAMS =
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA =

AM_CFLAGS = $(PTHREAD_CFLAGS)
AM_CPPFLAGS =							\
	$(DEFINES)						\
	$(SELINUX_CFLAGS)					\
	-I$(top_srcdir)/include					\
	-I$(top_srcdir)/src/mapi				\
	-I$(top_builddir)/src/mapi

GLAPI = $(top_srcdir)/src/mapi/glapi
include Makefile.sources
include glapi/Makefile.sources
include glapi/gen/glapi_gen.mk

if HAVE_SHARED_GLAPI
BUILT_SOURCES += shared-glapi/glapi_mapi_tmp.h

lib_LTLIBRARIES += shared-glapi/libglapi.la
shared_glapi_libglapi_la_SOURCES = $(MAPI_GLAPI_FILES)
shared_glapi_libglapi_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-DMAPI_MODE_GLAPI \
	-DMAPI_ABI_HEADER=\"shared-glapi/glapi_mapi_tmp.h\"
shared_glapi_libglapi_la_LIBADD = \
	$(PTHREAD_LIBS) \
	$(SELINUX_LIBS)
shared_glapi_libglapi_la_LDFLAGS = \
	-no-undefined \
	$(GC_SECTIONS) \
	$(LD_NO_UNDEFINED)

shared-glapi/glapi_mapi_tmp.h : $(GLAPI)/gen/gl_and_es_API.xml $(glapi_gen_mapi_deps)
	$(call glapi_gen_mapi,$<,shared-glapi)

TESTS += shared-glapi-test
check_PROGRAMS += shared-glapi-test

shared_glapi_test_SOURCES = shared-glapi/tests/check_table.cpp
shared_glapi_test_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/gtest/include
shared_glapi_test_LDADD = \
	$(top_builddir)/src/mapi/shared-glapi/libglapi.la \
	$(top_builddir)/src/gtest/libgtest.la
endif

if HAVE_OPENGL
noinst_LTLIBRARIES = glapi/libglapi.la

if HAVE_X86_ASM
if HAVE_X86_64_ASM
GLAPI_ASM_SOURCES = $(X86_64_API)
else
GLAPI_ASM_SOURCES = $(X86_API)
endif
endif
if HAVE_SPARC_ASM
GLAPI_ASM_SOURCES = $(SPARC_API)
endif

glapi_libglapi_la_SOURCES = $(GLAPI_UTIL_SOURCES)
glapi_libglapi_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/mapi/glapi \
	-I$(top_srcdir)/src/mesa

if HAVE_SHARED_GLAPI
glapi_libglapi_la_SOURCES += $(MAPI_BRIDGE_FILES)
glapi_libglapi_la_CPPFLAGS += \
	-DMAPI_MODE_BRIDGE \
	-DMAPI_ABI_HEADER=\"glapi/glapi_mapi_tmp.h\"
else
glapi_libglapi_la_CPPFLAGS += \
	-DMAPI_MODE_UTIL
glapi_libglapi_la_SOURCES += \
	$(GLAPI_SOURCES) \
	$(GLAPI_ASM_SOURCES) \
	$(MAPI_UTIL_FILES)

TESTS += glapi-test
check_PROGRAMS += glapi-test

glapi_test_SOURCES = glapi/tests/check_table.cpp
glapi_test_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-I$(top_srcdir)/src/gtest/include

glapi_test_LDADD = \
	$(top_builddir)/src/mapi/glapi/libglapi.la \
	$(top_builddir)/src/gtest/libgtest.la
endif
endif

if HAVE_OPENGL_ES1
TESTS += es1api/ABI-check

BUILT_SOURCES += es1api/glapi_mapi_tmp.h

pkgconfig_DATA += es1api/glesv1_cm.pc

GLES_includedir = $(includedir)/GLES
GLES_include_HEADERS = \
	$(top_srcdir)/include/GLES/egl.h \
	$(top_srcdir)/include/GLES/gl.h \
	$(top_srcdir)/include/GLES/glext.h \
	$(top_srcdir)/include/GLES/glplatform.h

lib_LTLIBRARIES += es1api/libGLESv1_CM.la

es1api_libGLESv1_CM_la_SOURCES = entry.c es1api/glapi_mapi_tmp.h
es1api_libGLESv1_CM_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(VISIBILITY_CFLAGS)
es1api_libGLESv1_CM_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-DMAPI_MODE_BRIDGE \
	-DMAPI_ABI_HEADER=\"es1api/glapi_mapi_tmp.h\"
es1api_libGLESv1_CM_la_LIBADD = $(GLESv1_CM_LIB_DEPS)
es1api_libGLESv1_CM_la_LDFLAGS = \
	-no-undefined \
	-version-number 1:1 \
	$(GC_SECTIONS) \
	$(LD_NO_UNDEFINED)

if HAVE_SHARED_GLAPI
es1api_libGLESv1_CM_la_LIBADD += shared-glapi/libglapi.la
endif

es1api/glapi_mapi_tmp.h: glapi/gen/gl_and_es_API.xml $(glapi_gen_mapi_deps)
	$(call glapi_gen_mapi,$<,es1api)
endif

if HAVE_OPENGL_ES2
TESTS += es2api/ABI-check

BUILT_SOURCES += es2api/glapi_mapi_tmp.h

pkgconfig_DATA += es2api/glesv2.pc

GLES2_includedir = $(includedir)/GLES2
GLES2_include_HEADERS = \
	$(top_srcdir)/include/GLES2/gl2.h \
	$(top_srcdir)/include/GLES2/gl2ext.h \
	$(top_srcdir)/include/GLES2/gl2platform.h
GLES3_includedir = $(includedir)/GLES3
GLES3_include_HEADERS = \
	$(top_srcdir)/include/GLES3/gl3.h \
	$(top_srcdir)/include/GLES3/gl31.h \
	$(top_srcdir)/include/GLES3/gl3ext.h \
	$(top_srcdir)/include/GLES3/gl3platform.h

lib_LTLIBRARIES += es2api/libGLESv2.la

es2api_libGLESv2_la_SOURCES = entry.c es2api/glapi_mapi_tmp.h
es2api_libGLESv2_la_CFLAGS = \
	$(AM_CFLAGS) \
	$(VISIBILITY_CFLAGS)
es2api_libGLESv2_la_CPPFLAGS = \
	$(AM_CPPFLAGS) \
	-DMAPI_MODE_BRIDGE \
	-DMAPI_ABI_HEADER=\"es2api/glapi_mapi_tmp.h\"
es2api_libGLESv2_la_LIBADD = $(GLESv2_LIB_DEPS)
es2api_libGLESv2_la_LDFLAGS = \
	-no-undefined \
	-version-number 2 \
	$(GC_SECTIONS) \
	$(LD_NO_UNDEFINED)

if HAVE_SHARED_GLAPI
es2api_libGLESv2_la_LIBADD += shared-glapi/libglapi.la
endif

es2api/glapi_mapi_tmp.h: glapi/gen/gl_and_es_API.xml $(glapi_gen_mapi_deps)
	$(call glapi_gen_mapi,$<,es2api)
endif

if HAVE_OPENVG
SUBDIRS += vgapi
endif

include $(top_srcdir)/install-lib-links.mk
