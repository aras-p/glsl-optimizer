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

AM_CPPFLAGS = \
	$(GALLIUM_CFLAGS) \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/gallium/drivers \
	-I$(top_srcdir)/src/gallium/winsys \
	 $(LIBDRM_CFLAGS) \
	-DGALLIUM_RBUG \
	-DGALLIUM_TRACE \
	-DGALLIUM_GALAHAD

pipedir = $(libdir)/gallium-pipe
pipe_LTLIBRARIES =

PIPE_LIBS = \
	$(top_builddir)/src/gallium/auxiliary/libgallium.la \
	$(top_builddir)/src/gallium/drivers/rbug/librbug.la \
	$(top_builddir)/src/gallium/drivers/trace/libtrace.la \
	$(DLOPEN_LIBS) \
	$(CLOCK_LIB) \
	-lpthread \
	-lm

if HAVE_GALAHAD_GALLIUM
PIPE_LIBS += $(top_builddir)/src/gallium/drivers/galahad/libgalahad.la
endif


if HAVE_GALLIUM_I915
pipe_LTLIBRARIES += pipe_i915.la
pipe_i915_la_SOURCES = pipe_i915.c
pipe_i915_la_LIBADD = \
	$(PIPE_LIBS) \
	$(top_builddir)/src/gallium/winsys/i915/drm/libi915drm.la \
	$(top_builddir)/src/gallium/drivers/i915/libi915.la \
	$(LIBDRM_LIBS) \
	$(INTEL_LIBS)
pipe_i915_la_LDFLAGS = -no-undefined -avoid-version -module
if HAVE_MESA_LLVM
nodist_EXTRA_pipe_i915_la_SOURCES = dummy.cpp
pipe_i915_la_LIBADD += $(LLVM_LIBS)
pipe_i915_la_LDFLAGS += $(LLVM_LDFLAGS)
endif
endif

if HAVE_GALLIUM_NOUVEAU
pipe_LTLIBRARIES += pipe_nouveau.la
pipe_nouveau_la_SOURCES = pipe_nouveau.c
nodist_EXTRA_pipe_nouveau_la_SOURCES = dummy.cpp
pipe_nouveau_la_LIBADD = \
	$(PIPE_LIBS) \
	$(top_builddir)/src/gallium/winsys/nouveau/drm/libnouveaudrm.la \
	$(top_builddir)/src/gallium/drivers/nv30/libnv30.la \
	$(top_builddir)/src/gallium/drivers/nv50/libnv50.la \
	$(top_builddir)/src/gallium/drivers/nvc0/libnvc0.la \
	$(top_builddir)/src/gallium/drivers/nouveau/libnouveau.la \
	$(NOUVEAU_LIBS)
pipe_nouveau_la_LDFLAGS = -no-undefined -avoid-version -module
if HAVE_MESA_LLVM
pipe_nouveau_la_LIBADD += $(LLVM_LIBS)
pipe_nouveau_la_LDFLAGS += $(LLVM_LDFLAGS)
endif
endif

if HAVE_GALLIUM_R300
pipe_LTLIBRARIES += pipe_r300.la
pipe_r300_la_SOURCES = pipe_r300.c
nodist_EXTRA_pipe_r300_la_SOURCES = dummy.cpp
pipe_r300_la_LIBADD = \
	$(PIPE_LIBS) \
	$(top_builddir)/src/gallium/winsys/radeon/drm/libradeonwinsys.la \
	$(top_builddir)/src/gallium/drivers/r300/libr300-helper.la \
	$(top_builddir)/src/gallium/drivers/r300/libr300.la \
	$(LIBDRM_LIBS) \
	$(RADEON_LIBS)
pipe_r300_la_LDFLAGS = -no-undefined -avoid-version -module
if HAVE_MESA_LLVM
pipe_r300_la_LIBADD += $(LLVM_LIBS)
pipe_r300_la_LDFLAGS += $(LLVM_LDFLAGS)
endif
endif

if HAVE_GALLIUM_R600
pipe_LTLIBRARIES += pipe_r600.la
pipe_r600_la_SOURCES = pipe_r600.c
pipe_r600_la_LIBADD = \
	$(PIPE_LIBS) \
	$(top_builddir)/src/gallium/winsys/radeon/drm/libradeonwinsys.la \
	$(top_builddir)/src/gallium/drivers/r600/libr600.la \
	$(LIBDRM_LIBS) \
	$(RADEON_LIBS)
pipe_r600_la_LDFLAGS = -no-undefined -avoid-version -module
if HAVE_MESA_LLVM
nodist_EXTRA_pipe_r600_la_SOURCES = dummy.cpp
pipe_r600_la_LIBADD += $(LLVM_LIBS)
pipe_r600_la_LDFLAGS += $(LLVM_LDFLAGS)
endif
endif

if HAVE_GALLIUM_RADEONSI
pipe_LTLIBRARIES += pipe_radeonsi.la
pipe_radeonsi_la_SOURCES = pipe_radeonsi.c
nodist_EXTRA_pipe_radeonsi_la_SOURCES = dummy.cpp
pipe_radeonsi_la_LIBADD = \
	$(PIPE_LIBS) \
	$(top_builddir)/src/gallium/winsys/radeon/drm/libradeonwinsys.la \
	$(top_builddir)/src/gallium/drivers/radeonsi/libradeonsi.la \
	$(LIBDRM_LIBS) \
	$(RADEON_LIBS)
pipe_radeonsi_la_LDFLAGS = -no-undefined -avoid-version -module
if HAVE_MESA_LLVM
pipe_radeonsi_la_LIBADD += $(LLVM_LIBS)
pipe_radeonsi_la_LDFLAGS += $(LLVM_LDFLAGS)
endif
endif

if HAVE_GALLIUM_SVGA
pipe_LTLIBRARIES += pipe_vmwgfx.la
pipe_vmwgfx_la_SOURCES = pipe_vmwgfx.c
pipe_vmwgfx_la_LIBADD = \
	$(PIPE_LIBS) \
	$(top_builddir)/src/gallium/winsys/svga/drm/libsvgadrm.la \
	$(top_builddir)/src/gallium/drivers/svga/libsvga.la \
	$(LIBDRM_LIBS)
pipe_vmwgfx_la_LDFLAGS = -no-undefined -avoid-version -module
if HAVE_MESA_LLVM
nodist_EXTRA_pipe_vmwgfx_la_SOURCES = dummy.cpp
pipe_vmwgfx_la_LIBADD += $(LLVM_LIBS)
pipe_vmwgfx_la_LDFLAGS += $(LLVM_LDFLAGS)
endif
endif

if HAVE_GALLIUM_LLVMPIPE
pipe_LTLIBRARIES += pipe_swrast.la
pipe_swrast_la_SOURCES = pipe_swrast.c
nodist_EXTRA_pipe_swrast_la_SOURCES = dummy.cpp
pipe_swrast_la_LIBADD = \
	$(PIPE_LIBS) \
	$(top_builddir)/src/gallium/drivers/llvmpipe/libllvmpipe.la \
	$(LLVM_LIBS)
pipe_swrast_la_LDFLAGS = -no-undefined -avoid-version -module $(LLVM_LDFLAGS)
else
if HAVE_GALLIUM_SOFTPIPE
pipe_LTLIBRARIES += pipe_swrast.la
pipe_swrast_la_SOURCES = pipe_swrast.c
pipe_swrast_la_LIBADD = \
	$(PIPE_LIBS) \
	$(top_builddir)/src/gallium/drivers/softpipe/libsoftpipe.la
pipe_swrast_la_LDFLAGS = -no-undefined -avoid-version -module
endif
endif
