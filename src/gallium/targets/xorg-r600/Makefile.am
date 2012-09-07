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
xorg_LTLIBRARIES = r600g_drv.la

r600g_drv_la_SOURCES = \
	target.c \
	xorg.c

r600g_drv_la_LDFLAGS = -module -avoid-version -shared -no-undefined

r600g_drv_la_LIBADD = \
	$(top_builddir)/src/gallium/auxiliary/libgallium.la \
	$(top_builddir)/src/gallium/state_trackers/xorg/libxorgtracker.la \
	$(top_builddir)/src/gallium/drivers/r600/libr600.la \
	$(top_builddir)/src/gallium/winsys/radeon/drm/libradeonwinsys.la \
	$(top_builddir)/src/gallium/drivers/galahad/libgalahad.la \
	$(top_builddir)/src/gallium/drivers/trace/libtrace.la \
	$(top_builddir)/src/gallium/drivers/rbug/librbug.la \
	$(GALLIUM_DRI_LIB_DEPS) \
	$(LIBDRM_XORG_LIBS) \
	$(LIBKMS_XORG_LIBS) \
	$(RADEON_LIBS)

if HAVE_MESA_LLVM
r600g_drv_la_LINK = $(CXXLINK) $(r600g_drv_la_LDFLAGS)
# Mention a dummy pure C++ file to trigger generation of the $(LINK) variable
nodist_EXTRA_r600g_drv_la_SOURCES = dummy-cpp.cpp

r600g_drv_la_LDFLAGS += $(LLVM_LDFLAGS)
r600g_drv_la_LIBADD += $(LLVM_LIBS)
else
r600g_drv_la_LINK = $(LINK) $(r600g_drv_la_LDFLAGS)
# Mention a dummy pure C file to trigger generation of the $(LINK) variable
nodist_EXTRA_r600g_drv_la_SOURCES = dummy-c.c
endif

# Provide compatibility with scripts for the old Mesa build system for
# a while by putting a link to the driver into /lib of the build tree.
all-local: r600g_drv.la
	$(MKDIR_P) $(top_builddir)/$(LIB_DIR)/gallium
	ln -f .libs/r600g_drv.so $(top_builddir)/$(LIB_DIR)/gallium/r600g_drv.so
