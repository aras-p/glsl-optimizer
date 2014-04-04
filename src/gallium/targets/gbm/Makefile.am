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

gbmdir = $(libdir)/gbm

AM_CPPFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/gallium/state_trackers/gbm \
	-I$(top_srcdir)/src/gbm/main \
	-I$(top_srcdir)/src/gallium/winsys \
	$(GALLIUM_PIPE_LOADER_DEFINES) \
	-DPIPE_SEARCH_DIR=\"$(libdir)/gallium-pipe\"

AM_CFLAGS = \
	$(GALLIUM_CFLAGS) \
	$(VISIBILITY_CFLAGS) \
	$(LIBDRM_CFLAGS)

gbm_LTLIBRARIES = gbm_gallium_drm.la

nodist_EXTRA_gbm_gallium_drm_la_SOURCES = dummy.cpp
gbm_gallium_drm_la_SOURCES = gbm.c

gbm_gallium_drm_la_LIBADD = \
	$(GALLIUM_PIPE_LOADER_CLIENT_LIBS) \
	$(top_builddir)/src/gallium/auxiliary/pipe-loader/libpipe_loader_client.la \
	$(top_builddir)/src/gallium/winsys/sw/null/libws_null.la \
	$(top_builddir)/src/gallium/state_trackers/gbm/libgbm.la \
	$(top_builddir)/src/gallium/auxiliary/libgallium.la \
	$(LIBDRM_LIBS) \
	$(GALLIUM_COMMON_LIB_DEPS)

if HAVE_DRI
gbm_gallium_drm_la_LIBADD += \
	$(top_builddir)/src/gallium/winsys/sw/dri/libswdri.la
endif

if NEED_WINSYS_XLIB
gbm_gallium_drm_la_LIBADD += \
	$(top_builddir)/src/gallium/winsys/sw/xlib/libws_xlib.la \
	-lX11 -lXext -lXfixes \
	$(LIBDRM_LIBS)
endif

gbm_gallium_drm_la_LDFLAGS = \
	-module \
	-no-undefined \
	-avoid-version \
	-Wl,--version-script=$(top_srcdir)/src/gallium/targets/gbm/gbm.sym \
	$(GC_SECTIONS) \
	$(LD_NO_UNDEFINED)

if HAVE_MESA_LLVM
gbm_gallium_drm_la_LIBADD += $(LLVM_LIBS)
gbm_gallium_drm_la_LDFLAGS += $(LLVM_LDFLAGS)
endif

TESTS = gallium-gbm-symbols-check
