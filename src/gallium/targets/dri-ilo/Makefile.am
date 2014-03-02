# Copyright © 2012 Intel Corporation
# Copyright © 2013 LunarG, Inc.
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
	$(GALLIUM_DRI_CFLAGS)
AM_CPPFLAGS = \
	-DGALLIUM_RBUG \
	-DGALLIUM_TRACE \
	-DGALLIUM_GALAHAD

noinst_LTLIBRARIES = ilo_dri.la

ilo_dri_la_SOURCES = target.c

# need -rpath to create a noinst shared library
ilo_dri_la_LDFLAGS = $(GALLIUM_DRI_LINKER_FLAGS) \
		     -rpath $(abs_builddir)

ilo_dri_la_LIBADD = \
	$(top_builddir)/src/mesa/drivers/dri/common/libdricommon.la \
	$(top_builddir)/src/mesa/libmesagallium.la \
	$(top_builddir)/src/gallium/auxiliary/libgallium.la \
	$(top_builddir)/src/gallium/state_trackers/dri/drm/libdridrm.la \
	$(top_builddir)/src/gallium/winsys/intel/drm/libintelwinsys.la \
	$(top_builddir)/src/gallium/drivers/galahad/libgalahad.la \
	$(top_builddir)/src/gallium/drivers/trace/libtrace.la \
	$(top_builddir)/src/gallium/drivers/rbug/librbug.la \
	$(top_builddir)/src/gallium/drivers/ilo/libilo.la \
	$(GALLIUM_DRI_LIB_DEPS) \
	$(INTEL_LIBS)

# Mention a dummy pure C++ file to trigger generation of the $(LINK) variable
nodist_EXTRA_ilo_dri_la_SOURCES = dummy-cpp.cpp

if HAVE_MESA_LLVM
ilo_dri_la_LDFLAGS += $(LLVM_LDFLAGS)
ilo_dri_la_LIBADD += $(LLVM_LIBS)
endif

include $(top_srcdir)/install-gallium-links.mk
