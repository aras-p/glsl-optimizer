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

AUTOMAKE_OPTIONS = subdir-objects

include Makefile.sources
include $(top_srcdir)/src/gallium/Automake.inc

AM_CFLAGS = \
	$(GALLIUM_CFLAGS) \
	$(VISIBILITY_CFLAGS)

AM_CPPFLAGS = \
	-I$(top_srcdir)/src/egl/main \
	-I$(top_builddir)/src/egl/wayland/wayland-drm/ \
	-I$(top_srcdir)/src/gallium/winsys

noinst_LTLIBRARIES = libegl.la
libegl_la_SOURCES = $(common_FILES)

if HAVE_EGL_PLATFORM_X11
libegl_la_SOURCES += $(x11_FILES) $(x11_drm_FILES)
AM_CFLAGS += \
	$(X11_INCLUDES) \
	$(LIBDRM_CFLAGS) \
	$(DRI2PROTO_CFLAGS)
AM_CPPFLAGS += \
	-I$(top_srcdir)/src/glx \
	-I$(top_srcdir)/src/mapi \
	-I$(top_srcdir)/src/mesa \
	-DHAVE_X11_BACKEND
endif

if HAVE_EGL_PLATFORM_WAYLAND
libegl_la_SOURCES += $(wayland_FILES)
AM_CFLAGS += \
	$(LIBDRM_CFLAGS) \
	$(WAYLAND_CFLAGS)
AM_CPPFLAGS += \
	-I$(top_srcdir)/src/egl/wayland/wayland-egl \
	-I$(top_srcdir)/src/egl/wayland/wayland-drm \
	-I$(top_builddir)/src/egl/wayland/wayland-drm \
	-DHAVE_WAYLAND_BACKEND
endif

if HAVE_EGL_PLATFORM_DRM
libegl_la_SOURCES += $(drm_FILES)
AM_CFLAGS += \
	$(LIBDRM_CFLAGS)
AM_CPPFLAGS += \
	-I$(top_srcdir)/src/loader \
	-I$(top_srcdir)/src/gbm/main \
	-I$(top_srcdir)/src/gallium/state_trackers/gbm \
	-DHAVE_DRM_BACKEND
endif

if HAVE_EGL_PLATFORM_FBDEV
libegl_la_SOURCES += $(fbdev_FILES)
AM_CPPFLAGS += \
	-DHAVE_FBDEV_BACKEND
endif

if HAVE_EGL_PLATFORM_NULL
libegl_la_SOURCES += $(null_FILES)
AM_CPPFLAGS += \
	-DHAVE_NULL_BACKEND
endif

if HAVE_OPENGL
AM_CPPFLAGS += \
	-DFEATURE_GL=1
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
	-DFEATURE_VG=1
endif

EXTRA_DIST = \
	Android.mk SConscript \
	$(android_FILES) $(gdi_FILES)
