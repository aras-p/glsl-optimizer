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

# This is Makefile for egl_gallium.so.  It is static in that all state trackers
# and pipe drivers are linked statically when possible.
#
# The following variables are examined
#
#   EGL_CLIENT_APIS     - state trackers to support
#   GALLIUM_WINSYS_DIRS - pipe drivers to support
#
include $(top_srcdir)/src/gallium/Automake.inc

AM_CFLAGS = \
	$(GALLIUM_TARGET_CFLAGS)

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/gallium/state_trackers/egl \
	-I$(top_srcdir)/src/egl/main \
	-DGALLIUM_TRACE \
	-DGALLIUM_RBUG \
	-D_EGL_MAIN=_eglMain

AM_LDFLAGS = \
	-module \
	-no-undefined \
	-avoid-version \
	$(GC_SECTIONS) \
	$(LD_NO_UNDEFINED)

if HAVE_LD_VERSION_SCRIPT
AM_LDFLAGS += \
	-Wl,--version-script=$(top_srcdir)/src/gallium/targets/egl-static/egl.sym
endif

egldir = $(EGL_DRIVER_INSTALL_DIR)
egl_LTLIBRARIES = egl_gallium.la

nodist_EXTRA_egl_gallium_la_SOURCES = dummy.cpp
egl_gallium_la_SOURCES = \
	egl.c \
	egl_pipe.c \
	egl_st.c

egl_gallium_la_LIBADD = \
	$(top_builddir)/src/loader/libloader.la \
	$(top_builddir)/src/gallium/auxiliary/libgallium.la \
	$(top_builddir)/src/gallium/drivers/identity/libidentity.la \
	$(top_builddir)/src/gallium/drivers/trace/libtrace.la \
	$(top_builddir)/src/gallium/drivers/rbug/librbug.la \
	$(top_builddir)/src/gallium/state_trackers/egl/libegl.la \
	$(top_builddir)/src/egl/main/libEGL.la \
	$(GALLIUM_COMMON_LIB_DEPS)

if HAVE_EGL_PLATFORM_X11
AM_CPPFLAGS += $(LIBDRM_CFLAGS)
egl_gallium_la_LIBADD += \
	$(top_builddir)/src/gallium/winsys/sw/xlib/libws_xlib.la \
	-lX11 -lXext -lXfixes $(LIBDRM_LIBS)
endif

if HAVE_EGL_PLATFORM_WAYLAND
AM_CPPFLAGS += $(LIBDRM_CFLAGS)
egl_gallium_la_LIBADD += \
	$(top_builddir)/src/gallium/winsys/sw/wayland/libws_wayland.la \
	$(top_builddir)/src/egl/wayland/wayland-drm/libwayland-drm.la \
	$(LIBDRM_LIBS) \
	$(WAYLAND_LIBS)
endif

if HAVE_EGL_PLATFORM_DRM
AM_CPPFLAGS += $(LIBDRM_CFLAGS)
egl_gallium_la_LIBADD += \
	$(top_builddir)/src/gbm/libgbm.la \
	$(LIBDRM_LIBS)
endif

if HAVE_EGL_PLATFORM_FBDEV
egl_gallium_la_LIBADD += $(top_builddir)/src/gallium/winsys/sw/fbdev/libfbdev.la
endif

if HAVE_EGL_PLATFORM_NULL
egl_gallium_la_LIBADD += $(top_builddir)/src/gallium/winsys/sw/null/libws_null.la
endif

if HAVE_OPENGL
AM_CPPFLAGS += \
	-I$(top_srcdir)/src/mesa \
	-DFEATURE_GL=1

egl_gallium_la_LIBADD += \
	$(top_builddir)/src/mesa/libmesagallium.la
# make st/mesa built-in when there is a single glapi provider
if HAVE_SHARED_GLAPI
egl_gallium_la_LIBADD += \
	$(top_builddir)/src/mapi/shared-glapi/libglapi.la
else
egl_gallium_la_LIBADD += \
	$(top_builddir)/src/mapi/glapi/libglapi.la

AM_CPPFLAGS += -D_EGL_EXTERNAL_GL=1
egl_LTLIBRARIES += st_GL.la

nodist_EXTRA_st_GL_la_SOURCES = dummy.cpp
st_GL_la_SOURCES = st_GL.c

st_GL_la_LDFLAGS = $(AM_LDFLAGS)
# st_GL, built only when shared glapi is not enabled
st_GL_la_LIBADD = \
	$(top_builddir)/src/mesa/libmesagallium.la \
	$(top_builddir)/src/gallium/auxiliary/libgallium.la \
	$(top_builddir)/src/mapi/glapi/libglapi.la \
	$(GALLIUM_COMMON_LIB_DEPS)
endif
endif

if HAVE_OPENGL_ES1
AM_CPPFLAGS += \
	-DFEATURE_ES1=1
endif

if HAVE_OPENGL_ES2
AM_CPPFLAGS += \
	-DFEATURE_ES2=1
endif

if HAVE_OPENVG
AM_CPPFLAGS += \
	-I$(top_srcdir)/src/gallium/state_trackers/vega \
	-DFEATURE_VG=1
egl_gallium_la_LIBADD += \
	$(top_builddir)/src/gallium/state_trackers/vega/libvega.la \
	$(top_builddir)/src/mapi/vgapi/libOpenVG.la
endif

#if HAVE_GALLIUM_STATIC_TARGETS

egl_gallium_la_LDFLAGS = $(AM_LDFLAGS)
egl_gallium_la_CPPFLAGS = $(AM_CPPFLAGS)

if HAVE_GALLIUM_I915
egl_gallium_la_CPPFLAGS += -DGALLIUM_I915
egl_gallium_la_LIBADD += \
	$(top_builddir)/src/gallium/winsys/i915/drm/libi915drm.la \
	$(top_builddir)/src/gallium/drivers/i915/libi915.la \
	$(INTEL_LIBS)
endif

if HAVE_GALLIUM_ILO
egl_gallium_la_CPPFLAGS += -DGALLIUM_ILO
egl_gallium_la_LIBADD += \
	$(top_builddir)/src/gallium/winsys/intel/drm/libintelwinsys.la \
	$(top_builddir)/src/gallium/drivers/ilo/libilo.la \
	$(INTEL_LIBS)
endif

if HAVE_GALLIUM_NOUVEAU
egl_gallium_la_CPPFLAGS += -DGALLIUM_NOUVEAU
egl_gallium_la_LIBADD += \
	$(top_builddir)/src/gallium/winsys/nouveau/drm/libnouveaudrm.la \
	$(top_builddir)/src/gallium/drivers/nouveau/libnouveau.la \
	$(NOUVEAU_LIBS)
endif

if NEED_RADEON_DRM_WINSYS
egl_gallium_la_LIBADD += \
	$(top_builddir)/src/gallium/winsys/radeon/drm/libradeonwinsys.la
endif

if HAVE_GALLIUM_RADEON_COMMON
egl_gallium_la_LIBADD += \
	$(top_builddir)/src/gallium/drivers/radeon/libradeon.la
endif

if HAVE_GALLIUM_R300
egl_gallium_la_CPPFLAGS += -DGALLIUM_R300
egl_gallium_la_LIBADD += \
	$(top_builddir)/src/gallium/drivers/r300/libr300.la \
	$(RADEON_LIBS)

if !HAVE_OPENGL
egl_gallium_la_LIBADD += \
	$(top_builddir)/src/gallium/drivers/r300/libr300-helper.la
endif
endif

if HAVE_GALLIUM_R600
egl_gallium_la_CPPFLAGS += -DGALLIUM_R600
egl_gallium_la_LIBADD += \
	$(top_builddir)/src/gallium/drivers/r600/libr600.la \
	$(RADEON_LIBS)
endif

if HAVE_GALLIUM_RADEONSI
egl_gallium_la_CPPFLAGS += -DGALLIUM_RADEONSI
egl_gallium_la_LIBADD += \
	$(top_builddir)/src/gallium/drivers/radeonsi/libradeonsi.la \
	$(RADEON_LIBS)
endif

if HAVE_GALLIUM_SVGA
egl_gallium_la_CPPFLAGS += -DGALLIUM_VMWGFX
egl_gallium_la_LIBADD += \
	$(top_builddir)/src/gallium/winsys/svga/drm/libsvgadrm.la \
	$(top_builddir)/src/gallium/drivers/svga/libsvga.la
endif

if HAVE_GALLIUM_FREEDRENO
egl_gallium_la_CPPFLAGS += -DGALLIUM_FREEDRENO
egl_gallium_la_LIBADD += \
	$(top_builddir)/src/gallium/winsys/freedreno/drm/libfreedrenodrm.la \
	$(top_builddir)/src/gallium/drivers/freedreno/libfreedreno.la \
	$(FREEDRENO_LIBS)

endif

if HAVE_GALLIUM_SOFTPIPE
egl_gallium_la_CPPFLAGS += -DGALLIUM_SOFTPIPE
egl_gallium_la_LIBADD += \
	$(top_builddir)/src/gallium/drivers/softpipe/libsoftpipe.la
endif

if NEED_GALLIUM_LLVMPIPE_DRIVER
egl_gallium_la_CPPFLAGS += -DGALLIUM_LLVMPIPE
egl_gallium_la_LIBADD += \
	$(top_builddir)/src/gallium/drivers/llvmpipe/libllvmpipe.la
endif

#else # HAVE_GALLIUM_STATIC_TARGETS

#egl_gallium_la_LIBADD += \
#	$(top_builddir)/src/gallium/auxiliary/pipe-loader/libpipe_loader.la \
#	$(GALLIUM_PIPE_LOADER_WINSYS_LIBS) \
#	$(GALLIUM_PIPE_LOADER_LIBS)
#endif

if HAVE_MESA_LLVM

egl_gallium_la_LDFLAGS += $(LLVM_LDFLAGS)
egl_gallium_la_LIBADD += $(LLVM_LIBS)

if HAVE_OPENGL
if !HAVE_SHARED_GLAPI
st_GL_la_LDFLAGS += $(LLVM_LDFLAGS)
st_GL_la_LIBADD += $(LLVM_LIBS)
endif
endif

endif

include $(top_srcdir)/install-gallium-links.mk
