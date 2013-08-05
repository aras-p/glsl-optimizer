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

omxdir = $(OMX_LIB_INSTALL_DIR)
omx_LTLIBRARIES = libomx_radeonsi.la

EXPORTS = '^(omx_component_library_Setup|radeon_drm_winsys_create)$$'

libomx_radeonsi_la_SOURCES = \
	drm_target.c \
	$(top_srcdir)/src/gallium/auxiliary/vl/vl_winsys_dri.c

libomx_radeonsi_la_LDFLAGS = \
	-module \
	-export-symbols-regex $(EXPORTS) \
	-shared \
	-no-undefined

libomx_radeonsi_la_LIBADD = \
	$(top_builddir)/src/gallium/auxiliary/libgallium.la \
	$(top_builddir)/src/gallium/drivers/radeonsi/libradeonsi.la \
	$(top_builddir)/src/gallium/state_trackers/omx/libomxtracker.la \
	$(top_builddir)/src/gallium/winsys/radeon/drm/libradeonwinsys.la \
	$(top_builddir)/src/gallium/drivers/trace/libtrace.la \
	$(GALLIUM_DRI_LIB_DEPS) \
	$(OMX_LIBS) \
	$(LIBDRM_LIBS) \
	$(RADEON_LIBS)

if HAVE_MESA_LLVM
libomx_radeonsi_la_LDFLAGS += $(LLVM_LDFLAGS)
libomx_radeonsi_la_LIBADD += $(LLVM_LIBS)
endif

# Provide compatibility with scripts for the old Mesa build system for
# a while by putting a link to the driver into /lib of the build tree.
all-local: libomx_radeonsi.la
	$(MKDIR_P) $(top_builddir)/$(LIB_DIR)/gallium
	ln -f .libs/libomx_radeonsi.so* $(top_builddir)/$(LIB_DIR)/gallium/
