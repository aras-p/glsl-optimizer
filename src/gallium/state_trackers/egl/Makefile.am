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

include $(top_srcdir)/src/gallium/Automake.inc

AM_CFLAGS = $(GALLIUM_CFLAGS)
AM_CPPFLAGS = \
	-I$(top_srcdir)/src/egl/main \
	-I$(top_builddir)/src/egl/wayland/wayland-drm/ \
	-I$(top_srcdir)/include

noinst_LTLIBRARIES = libegl.la
libegl_la_SOURCES = \
	common/egl_g3d_api.c \
	common/egl_g3d.c \
	common/egl_g3d_image.c \
	common/egl_g3d_st.c \
	common/egl_g3d_sync.c \
	common/native_helper.c \
	common/native_wayland_drm_bufmgr_helper.c

if HAVE_EGL_PLATFORM_X11
libegl_la_SOURCES += \
	x11/glxinit.c \
	x11/native_dri2.c \
	x11/native_x11.c \
	x11/native_ximage.c \
	x11/x11_screen.c \
	x11/dri2.c
AM_CFLAGS += \
	$(X11_CFLAGS) \
	$(LIBDRM_CFLAGS) \
	$(DRI2PROTO_CFLAGS)
AM_CPPFLAGS += \
	-I$(top_srcdir)/src/gallium/drivers \
	-I$(top_srcdir)/src/glx \
	-I$(top_srcdir)/src/mapi \
	-I$(top_srcdir)/src/mesa \
	-DHAVE_X11_BACKEND
endif

if HAVE_EGL_PLATFORM_WAYLAND
libegl_la_SOURCES += \
	wayland/native_drm.c \
	wayland/native_shm.c \
	wayland/native_wayland.c
AM_CFLAGS += \
	$(LIBDRM_CFLAGS) \
	$(WAYLAND_CFLAGS)
AM_CPPFLAGS += \
	-I$(top_srcdir)/src/gallium/winsys \
	-I$(top_srcdir)/src/egl/wayland/wayland-egl \
	-I$(top_srcdir)/src/egl/wayland/wayland-drm \
	-I$(top_builddir)/src/egl/wayland/wayland-drm \
	-DHAVE_WAYLAND_BACKEND
endif

if HAVE_EGL_PLATFORM_DRM
libegl_la_SOURCES += \
	drm/modeset.c \
	drm/native_drm.c
AM_CFLAGS += \
	$(LIBDRM_CFLAGS)
AM_CPPFLAGS += \
	-I$(top_srcdir)/src/gallium/winsys \
	-I$(top_srcdir)/src/gbm/main \
	-I$(top_srcdir)/src/gallium/state_trackers/gbm \
	-DHAVE_DRM_BACKEND
endif

if HAVE_EGL_PLATFORM_FBDEV
libegl_la_SOURCES += fbdev/native_fbdev.c
AM_CPPFLAGS += \
	-I$(top_srcdir)/src/gallium/winsys/sw \
	-DHAVE_FBDEV_BACKEND
endif

if HAVE_EGL_PLATFORM_NULL
libegl_la_SOURCES += null/native_null.c
AM_CPPFLAGS += \
	-I$(top_srcdir)/src/gallium/winsys/sw \
	-DHAVE_NULL_BACKEND
endif
