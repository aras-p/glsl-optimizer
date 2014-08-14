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

TOP = $(top_srcdir)

BUILT_SOURCES =
CLEANFILES = $(BUILT_SOURCES)

lib_LTLIBRARIES =

AM_CFLAGS = $(PTHREAD_CFLAGS)
AM_CPPFLAGS =							\
	$(DEFINES)						\
	$(SELINUX_CFLAGS)					\
	-I$(top_srcdir)/include					\
	-I$(top_srcdir)/src/mapi				\
	-I$(top_builddir)/src/mapi

GLAPI = $(top_srcdir)/src/mapi/glapi
include Makefile.sources
include glapi/gen/glapi_gen.mk

if HAVE_SHARED_GLAPI
SUBDIRS += shared-glapi/tests

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
endif

if HAVE_OPENGL
SUBDIRS += glapi
endif

if HAVE_OPENGL_ES1
SUBDIRS += es1api
endif

if HAVE_OPENGL_ES2
SUBDIRS += es2api
endif

if HAVE_OPENVG
SUBDIRS += vgapi
endif

include $(top_srcdir)/install-lib-links.mk
