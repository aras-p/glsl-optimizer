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

include $(top_srcdir)/src/gallium/Automake.inc

AM_CFLAGS = \
	$(GALLIUM_CFLAGS) \
	$(PTHREAD_CFLAGS) \
	$(XORG_CFLAGS)
AM_CPPFLAGS = \
	-I$(top_srcdir)/src/gallium/drivers \
	-I$(top_srcdir)/src/gallium/winsys \
	-DHAVE_CONFIG_H \
	-DGALLIUM_RBUG \
	-DGALLIUM_TRACE \
	-DGALLIUM_GALAHAD

xorgdir = $(XORG_DRIVER_INSTALL_DIR)
xorg_LTLIBRARIES = i915_drv.la

i915_drv_la_SOURCES = \
	intel_target.c \
	intel_xorg.c

i915_drv_la_LDFLAGS = -module -avoid-version -shared -no-undefined

i915_drv_la_LIBADD = \
	$(top_builddir)/src/gallium/auxiliary/libgallium.la \
	$(top_builddir)/src/gallium/state_trackers/xorg/libxorgtracker.la \
	$(top_builddir)/src/gallium/winsys/i915/drm/libi915drm.la \
	$(top_builddir)/src/gallium/drivers/i915/libi915.la \
	$(top_builddir)/src/gallium/drivers/galahad/libgalahad.la \
	$(top_builddir)/src/gallium/drivers/trace/libtrace.la \
	$(top_builddir)/src/gallium/drivers/rbug/librbug.la \
	$(GALLIUM_DRI_LIB_DEPS) \
	$(LIBDRM_XORG_LIBS) \
	$(LIBKMS_XORG_LIBS) \
	$(INTEL_LIBS)

if HAVE_MESA_LLVM
i915_drv_la_LINK = $(CXXLINK) $(i915_drv_la_LDFLAGS)
# Mention a dummy pure C++ file to trigger generation of the $(LINK) variable
nodist_EXTRA_i915_drv_la_SOURCES = dummy-cpp.cpp

AM_CPPFLAGS += -DGALLIUM_LLVMPIPE
i915_drv_la_LIBADD += $(top_builddir)/src/gallium/drivers/llvmpipe/libllvmpipe.la $(LLVM_LIBS)
else
i915_drv_la_LINK = $(LINK) $(i915_drv_la_LDFLAGS)
# Mention a dummy pure C file to trigger generation of the $(LINK) variable
nodist_EXTRA_i915_drv_la_SOURCES = dummy-c.c
endif

# Provide compatibility with scripts for the old Mesa build system for
# a while by putting a link to the driver into /lib of the build tree.
all-local: i915_drv.la
	$(MKDIR_P) $(top_builddir)/$(LIB_DIR)/gallium
	ln -f .libs/i915_drv.so $(top_builddir)/$(LIB_DIR)/gallium/i915_drv.so
