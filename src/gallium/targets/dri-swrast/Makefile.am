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
	$(EXPAT_CFLAGS) \
	$(GALLIUM_DRI_CFLAGS)
AM_CPPFLAGS = \
	-I$(top_srcdir)/src/gallium/winsys/sw/dri \
	-I$(top_builddir)/src/mesa/drivers/dri/common \
	-DGALLIUM_RBUG \
	-DGALLIUM_TRACE \
	-DGALLIUM_SOFTPIPE \
	-D__NOT_HAVE_DRM_H

dridir = $(DRI_DRIVER_INSTALL_DIR)
dri_LTLIBRARIES = swrast_dri.la

nodist_EXTRA_swrast_dri_la_SOURCES = dummy.cpp
swrast_dri_la_SOURCES = \
	swrast_drm_api.c \
	$(top_srcdir)/src/mesa/drivers/dri/common/utils.c \
	$(top_srcdir)/src/mesa/drivers/dri/common/dri_util.c \
	$(top_srcdir)/src/mesa/drivers/dri/common/xmlconfig.c

swrast_dri_la_LDFLAGS = $(GALLIUM_DRI_LINKER_FLAGS)

swrast_dri_la_LIBADD = \
	$(top_builddir)/src/gallium/state_trackers/dri/sw/libdrisw.la \
	$(top_builddir)/src/gallium/winsys/sw/dri/libswdri.la \
	$(top_builddir)/src/gallium/drivers/softpipe/libsoftpipe.la \
	$(top_builddir)/src/gallium/drivers/trace/libtrace.la \
	$(top_builddir)/src/gallium/drivers/rbug/librbug.la \
	$(GALLIUM_DRI_LIB_DEPS)

if HAVE_MESA_LLVM
AM_CPPFLAGS += -DGALLIUM_LLVMPIPE
swrast_dri_la_LIBADD += $(top_builddir)/src/gallium/drivers/llvmpipe/libllvmpipe.la
endif

include $(top_srcdir)/install-gallium-links.mk
