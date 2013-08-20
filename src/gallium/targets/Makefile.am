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

SUBDIRS =

if HAVE_X11_DRIVER
SUBDIRS += libgl-xlib
endif

if HAVE_GALLIUM_OSMESA
SUBDIRS += osmesa
endif

if HAVE_GALLIUM_GBM
SUBDIRS += gbm
endif

if HAVE_CLOVER
SUBDIRS += opencl
endif

if HAVE_GALLIUM_SVGA
if HAVE_DRI
SUBDIRS += dri-vmwgfx
endif

if HAVE_ST_XA
SUBDIRS += xa-vmwgfx
endif
endif

if HAVE_GALLIUM_FREEDRENO
if HAVE_DRI
SUBDIRS += dri-freedreno
endif
endif

if HAVE_GALLIUM_I915
if HAVE_DRI
SUBDIRS += dri-i915
endif

if HAVE_ST_XORG
SUBDIRS += xorg-i915
endif
endif

if HAVE_GALLIUM_ILO
if HAVE_DRI
SUBDIRS += dri-ilo
endif
endif

if HAVE_GALLIUM_R300
if HAVE_DRI
SUBDIRS += dri-r300
endif

if HAVE_ST_XVMC
SUBDIRS += xvmc-r300
endif

if HAVE_ST_VDPAU
SUBDIRS += vdpau-r300
endif
endif

if HAVE_GALLIUM_R600
if HAVE_DRI
SUBDIRS += dri-r600
endif

if HAVE_ST_XORG
SUBDIRS += xorg-r600
endif

if HAVE_ST_XVMC
SUBDIRS += xvmc-r600
endif

if HAVE_ST_VDPAU
SUBDIRS += vdpau-r600
endif
endif

if HAVE_GALLIUM_RADEONSI
if HAVE_DRI
SUBDIRS += dri-radeonsi
endif

if HAVE_ST_XORG
SUBDIRS += xorg-radeonsi
endif

if HAVE_ST_VDPAU
SUBDIRS += vdpau-radeonsi
endif
endif

if HAVE_GALLIUM_NOUVEAU
if HAVE_DRI
SUBDIRS += dri-nouveau
endif

if HAVE_ST_XORG
SUBDIRS += xorg-nouveau
endif

if HAVE_ST_XVMC
SUBDIRS += xvmc-nouveau
endif

if HAVE_ST_VDPAU
SUBDIRS += vdpau-nouveau
endif
endif

if HAVE_GALLIUM_SOFTPIPE
if HAVE_DRI
SUBDIRS += dri-swrast
endif

if HAVE_ST_XVMC
SUBDIRS += xvmc-softpipe
endif

if HAVE_ST_VDPAU
SUBDIRS += vdpau-softpipe
endif
endif

if NEED_GALLIUM_LOADER
SUBDIRS += pipe-loader
endif

if HAVE_GALLIUM_EGL
SUBDIRS += egl-static
endif
