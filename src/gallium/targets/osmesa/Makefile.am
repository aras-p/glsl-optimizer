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

include $(top_srcdir)/src/gallium/Automake.inc

AM_CFLAGS = \
	$(GALLIUM_CFLAGS) \
	$(VISIBILITY_CFLAGS)

AM_CPPFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/mapi \
	-I$(top_srcdir)/src/mesa \
	-I$(top_srcdir)/src/gallium/include \
	-I$(top_srcdir)/src/gallium/drivers \
	-I$(top_srcdir)/src/gallium/winsys \
	-I$(top_srcdir)/src/gallium/auxiliary \
	-DGALLIUM_SOFTPIPE \
	-DGALLIUM_TRACE

lib_LTLIBRARIES = lib@OSMESA_LIB@.la

nodist_EXTRA_lib@OSMESA_LIB@_la_SOURCES = dummy.cpp
lib@OSMESA_LIB@_la_SOURCES = target.c

lib@OSMESA_LIB@_la_LDFLAGS = \
	-module \
	-no-undefined \
	-version-number @OSMESA_VERSION@ \
	$(GC_SECTIONS) \
	$(LD_NO_UNDEFINED)

if HAVE_LD_VERSION_SCRIPT
lib@OSMESA_LIB@_la_LDFLAGS += \
	-Wl,--version-script=$(top_srcdir)/src/gallium/targets/osmesa/osmesa.sym
endif

if HAVE_SHARED_GLAPI
SHARED_GLAPI_LIB = $(top_builddir)/src/mapi/shared-glapi/libglapi.la
endif

lib@OSMESA_LIB@_la_LIBADD = \
	$(top_builddir)/src/mesa/libmesagallium.la \
	$(top_builddir)/src/gallium/auxiliary/libgallium.la \
	$(top_builddir)/src/gallium/winsys/sw/null/libws_null.la \
	$(top_builddir)/src/gallium/drivers/trace/libtrace.la \
	$(top_builddir)/src/gallium/drivers/softpipe/libsoftpipe.la \
	$(top_builddir)/src/gallium/state_trackers/osmesa/libosmesa.la \
	$(top_builddir)/src/mapi/glapi/libglapi.la \
	$(SHARED_GLAPI_LIB) \
	$(OSMESA_LIB_DEPS) \
	$(CLOCK_LIB)

if HAVE_MESA_LLVM
AM_CPPFLAGS += -DGALLIUM_LLVMPIPE
lib@OSMESA_LIB@_la_LDFLAGS += $(LLVM_LDFLAGS)
lib@OSMESA_LIB@_la_LIBADD += $(top_builddir)/src/gallium/drivers/llvmpipe/libllvmpipe.la $(LLVM_LIBS)
endif

include $(top_srcdir)/install-gallium-links.mk

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = osmesa.pc
