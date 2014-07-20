# Mesa 3-D graphics library
#
# Copyright (C) 2010-2011 Chia-I Wu <olvaffe@gmail.com>
# Copyright (C) 2010-2011 LunarG Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

# src/gallium/Android.mk

GALLIUM_TOP := $(call my-dir)
GALLIUM_COMMON_MK := $(GALLIUM_TOP)/Android.common.mk

SUBDIRS := \
	targets/egl-static \
	state_trackers/egl \
	auxiliary

# swrast
SUBDIRS += winsys/sw/android drivers/softpipe

# freedreno
ifneq ($(filter freedreno, $(MESA_GPU_DRIVERS)),)
SUBDIRS += winsys/freedreno/drm drivers/freedreno
endif

# i915g
ifneq ($(filter i915g, $(MESA_GPU_DRIVERS)),)
SUBDIRS += winsys/i915/drm drivers/i915
endif

# ilo
ifneq ($(filter ilo, $(MESA_GPU_DRIVERS)),)
SUBDIRS += winsys/intel/drm drivers/ilo
endif

# nouveau
ifneq ($(filter nouveau, $(MESA_GPU_DRIVERS)),)
SUBDIRS += \
	winsys/nouveau/drm \
	drivers/nouveau
endif

# r300g/r600g/radeonsi
ifneq ($(filter r300g r600g radeonsi, $(MESA_GPU_DRIVERS)),)
SUBDIRS += winsys/radeon/drm
ifneq ($(filter r300g, $(MESA_GPU_DRIVERS)),)
SUBDIRS += drivers/r300
endif
ifneq ($(filter r600g radeonsi, $(MESA_GPU_DRIVERS)),)
SUBDIRS += drivers/radeon
ifneq ($(filter r600g, $(MESA_GPU_DRIVERS)),)
SUBDIRS += drivers/r600
endif
ifneq ($(filter radeonsi, $(MESA_GPU_DRIVERS)),)
SUBDIRS += drivers/radeonsi
endif
endif
endif

# vmwgfx
ifneq ($(filter vmwgfx, $(MESA_GPU_DRIVERS)),)
SUBDIRS += winsys/svga/drm drivers/svga
endif

mkfiles := $(patsubst %,$(GALLIUM_TOP)/%/Android.mk,$(SUBDIRS))
include $(mkfiles)
