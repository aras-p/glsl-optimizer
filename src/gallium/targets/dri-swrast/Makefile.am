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
	-I$(top_srcdir)/src/gallium/winsys/sw/dri \
	-I$(top_srcdir)/src/gallium/drivers \
	-I$(top_srcdir)/src/gallium/winsys \
	-I$(top_srcdir)/src/mesa \
	-I$(top_srcdir)/src/mapi \
	-DGALLIUM_RBUG \
	-DGALLIUM_TRACE \
	-DGALLIUM_SOFTPIPE \
	-D__NOT_HAVE_DRM_H

dridir = $(DRI_DRIVER_INSTALL_DIR)
dri_LTLIBRARIES = swrast_dri.la

swrast_dri_la_SOURCES = \
	swrast_drm_api.c \
	$(top_srcdir)/src/mesa/drivers/dri/common/utils.c \
	$(top_srcdir)/src/mesa/drivers/dri/common/drisw_util.c \
	$(top_srcdir)/src/mesa/drivers/dri/common/xmlconfig.c

swrast_dri_la_LDFLAGS = -module -avoid-version -shared -no-undefined

swrast_dri_la_LIBADD = \
	$(top_builddir)/src/mesa/libmesagallium.la \
	$(top_builddir)/src/gallium/auxiliary/libgallium.la \
	$(top_builddir)/src/gallium/state_trackers/dri/sw/libdrisw.la \
	$(top_builddir)/src/gallium/winsys/sw/dri/libswdri.la \
	$(top_builddir)/src/gallium/drivers/softpipe/libsoftpipe.la \
	$(top_builddir)/src/gallium/drivers/trace/libtrace.la \
	$(top_builddir)/src/gallium/drivers/rbug/librbug.la \
	$(GALLIUM_DRI_LIB_DEPS)

nodist_EXTRA_swrast_dri_la_SOURCES = dummy.cpp

if HAVE_MESA_LLVM
AM_CPPFLAGS += -DGALLIUM_LLVMPIPE
swrast_dri_la_LDFLAGS += $(LLVM_LDFLAGS)
swrast_dri_la_LIBADD += $(top_builddir)/src/gallium/drivers/llvmpipe/libllvmpipe.la $(LLVM_LIBS)
endif

# Provide compatibility with scripts for the old Mesa build system for
# a while by putting a link to the driver into /lib of the build tree.
all-local: swrast_dri.la
	$(MKDIR_P) $(top_builddir)/$(LIB_DIR)/gallium
	ln -f .libs/swrast_dri.so $(top_builddir)/$(LIB_DIR)/gallium/swrast_dri.so
