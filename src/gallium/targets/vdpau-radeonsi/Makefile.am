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
	$(LIBDRM_CFLAGS)
AM_CPPFLAGS = \
	-I$(top_srcdir)/src/gallium/drivers \
	-I$(top_srcdir)/src/gallium/winsys

vdpaudir = $(VDPAU_LIB_INSTALL_DIR)
vdpau_LTLIBRARIES = libvdpau_radeonsi.la

nodist_EXTRA_libvdpau_radeonsi_la_SOURCES = dummy.cpp
libvdpau_radeonsi_la_SOURCES = \
	target.c \
	$(top_srcdir)/src/gallium/auxiliary/vl/vl_winsys_dri.c

libvdpau_radeonsi_la_LDFLAGS = \
	-module \
	-version-number $(VDPAU_MAJOR):$(VDPAU_MINOR) \
	-shared \
	-no-undefined

libvdpau_radeonsi_la_LIBADD = \
	$(top_builddir)/src/gallium/auxiliary/libgallium.la \
	$(top_builddir)/src/gallium/drivers/radeonsi/libradeonsi.la \
	$(top_builddir)/src/gallium/state_trackers/vdpau/libvdpautracker.la \
	$(top_builddir)/src/gallium/winsys/radeon/drm/libradeonwinsys.la \
	$(top_builddir)/src/gallium/drivers/trace/libtrace.la \
	$(top_builddir)/src/gallium/drivers/rbug/librbug.la \
	$(top_builddir)/src/gallium/drivers/noop/libnoop.la \
	$(GALLIUM_DRI_LIB_DEPS) \
	$(VDPAU_LIBS) \
	$(LIBDRM_LIBS) \
	$(RADEON_LIBS)

if HAVE_MESA_LLVM
libvdpau_radeonsi_la_LDFLAGS += $(LLVM_LDFLAGS)
libvdpau_radeonsi_la_LIBADD += $(LLVM_LIBS)
endif

# Provide compatibility with scripts for the old Mesa build system for
# a while by putting a link to the driver into /lib of the build tree.
all-local: libvdpau_radeonsi.la
	$(MKDIR_P) $(top_builddir)/$(LIB_DIR)/gallium
	ln -f .libs/libvdpau_radeonsi.so* $(top_builddir)/$(LIB_DIR)/gallium/
