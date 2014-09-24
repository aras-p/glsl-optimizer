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
	$(GALLIUM_TARGET_CFLAGS)

gbmdir = $(libdir)/gbm
gbm_LTLIBRARIES = gbm_gallium_drm.la

nodist_EXTRA_gbm_gallium_drm_la_SOURCES = dummy.cpp
gbm_gallium_drm_la_SOURCES =

gbm_gallium_drm_la_LIBADD = \
	$(top_builddir)/src/gallium/state_trackers/gbm/libgbm.la \
	$(top_builddir)/src/gallium/auxiliary/libgallium.la \
	$(top_builddir)/src/util/libmesautil.la \
	$(LIBDRM_LIBS) \
	$(GALLIUM_COMMON_LIB_DEPS)

gbm_gallium_drm_la_LDFLAGS = \
	-module \
	-no-undefined \
	-avoid-version \
	$(GC_SECTIONS) \
	$(LD_NO_UNDEFINED)

if HAVE_LD_VERSION_SCRIPT
gbm_gallium_drm_la_LDFLAGS += \
	-Wl,--version-script=$(top_srcdir)/src/gallium/targets/gbm/gbm.sym
endif

if HAVE_GALLIUM_STATIC_TARGETS

TARGET_DRIVERS =
TARGET_CPPFLAGS =
TARGET_LIB_DEPS = $(top_builddir)/src/loader/libloader.la

include $(top_srcdir)/src/gallium/drivers/i915/Automake.inc

include $(top_srcdir)/src/gallium/drivers/ilo/Automake.inc

include $(top_srcdir)/src/gallium/drivers/nouveau/Automake.inc

include $(top_srcdir)/src/gallium/drivers/r300/Automake.inc
include $(top_srcdir)/src/gallium/drivers/r600/Automake.inc
include $(top_srcdir)/src/gallium/drivers/radeonsi/Automake.inc

include $(top_srcdir)/src/gallium/drivers/svga/Automake.inc

include $(top_srcdir)/src/gallium/drivers/freedreno/Automake.inc

gbm_gallium_drm_la_SOURCES += target.c
gbm_gallium_drm_la_CPPFLAGS = $(TARGET_CPPFLAGS)
gbm_gallium_drm_la_LIBADD += $(TARGET_LIB_DEPS) \
	$(TARGET_RADEON_WINSYS) $(TARGET_RADEON_COMMON)

else # HAVE_GALLIUM_STATIC_TARGETS
gbm_gallium_drm_la_LIBADD += \
	$(top_builddir)/src/gallium/auxiliary/pipe-loader/libpipe_loader.la \
	$(GALLIUM_PIPE_LOADER_WINSYS_LIBS) \
	$(GALLIUM_PIPE_LOADER_LIBS)

endif # HAVE_GALLIUM_STATIC_TARGETS

if HAVE_MESA_LLVM
gbm_gallium_drm_la_LIBADD += $(LLVM_LIBS)
gbm_gallium_drm_la_LDFLAGS += $(LLVM_LDFLAGS)
endif

TESTS = gallium-gbm-symbols-check
