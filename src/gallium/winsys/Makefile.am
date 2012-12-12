# Copyright © 2013 Intel Corporation
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

SUBDIRS = sw/null

if NEED_WINSYS_XLIB
SUBDIRS += sw/xlib
endif

if HAVE_DRI
SUBDIRS += sw/dri
endif

if HAVE_EGL_PLATFORM_FBDEV
SUBDIRS += sw/fbdev
endif

if HAVE_EGL_PLATFORM_WAYLAND
SUBDIRS += sw/wayland
endif

if NEED_WINSYS_WRAPPER
SUBDIRS += sw/wrapper
endif

if NEED_NONNULL_WINSYS
if HAVE_GALLIUM_FREEDRENO
SUBDIRS += freedreno/drm
endif

if HAVE_GALLIUM_I915
SUBDIRS += i915/sw i915/drm
endif

if HAVE_GALLIUM_ILO
SUBDIRS += intel/drm
endif

if HAVE_GALLIUM_NOUVEAU
SUBDIRS += nouveau/drm
endif

if NEED_RADEON_DRM_WINSYS
SUBDIRS += radeon/drm
endif

if HAVE_GALLIUM_SVGA
SUBDIRS += svga/drm
endif
endif
