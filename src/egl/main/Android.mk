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

# Android.mk for libGLES_mesa

LOCAL_PATH := $(call my-dir)

include $(LOCAL_PATH)/Makefile.sources

SOURCES := \
	${LIBEGL_C_FILES}

# ---------------------------------------
# Build libGLES_mesa
# ---------------------------------------

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(SOURCES)

LOCAL_CFLAGS := \
	-D_EGL_NATIVE_PLATFORM=_EGL_PLATFORM_ANDROID \
	-D_EGL_DRIVER_SEARCH_DIR=\"/system/lib/egl\" \
	-D_EGL_OS_UNIX=1

LOCAL_STATIC_LIBRARIES :=

LOCAL_SHARED_LIBRARIES := \
	libglapi \
	libdl \
	libhardware \
	liblog \
	libcutils \
	libgralloc_drm \

ifeq ($(shell echo "$(MESA_ANDROID_VERSION) >= 4.2" | bc),1)
LOCAL_SHARED_LIBRARIES += libsync
endif

# add libdrm if there are hardware drivers
ifneq ($(MESA_GPU_DRIVERS),swrast)
LOCAL_SHARED_LIBRARIES += libdrm
endif

ifeq ($(strip $(MESA_BUILD_CLASSIC)),true)
LOCAL_CFLAGS += -D_EGL_BUILT_IN_DRIVER_DRI2
LOCAL_STATIC_LIBRARIES += libmesa_egl_dri2

# require i915_dri and/or i965_dri
LOCAL_REQUIRED_MODULES += \
	$(addsuffix _dri, $(filter i915 i965, $(MESA_GPU_DRIVERS)))
endif # MESA_BUILD_CLASSIC

ifeq ($(strip $(MESA_BUILD_GALLIUM)),true)

LOCAL_CFLAGS += -D_EGL_BUILT_IN_DRIVER_GALLIUM

gallium_DRIVERS :=

# swrast
gallium_DRIVERS += libmesa_pipe_softpipe libmesa_winsys_sw_android

# freedreno
ifneq ($(filter freedreno, $(MESA_GPU_DRIVERS)),)
gallium_DRIVERS += libmesa_winsys_freedreno libmesa_pipe_freedreno
LOCAL_SHARED_LIBRARIES += libdrm_freedreno
endif

# i915g
ifneq ($(filter i915g, $(MESA_GPU_DRIVERS)),)
gallium_DRIVERS += libmesa_winsys_i915 libmesa_pipe_i915
LOCAL_SHARED_LIBRARIES += libdrm_intel
endif

# ilo
ifneq ($(filter ilo, $(MESA_GPU_DRIVERS)),)
gallium_DRIVERS += libmesa_winsys_intel libmesa_pipe_ilo
LOCAL_SHARED_LIBRARIES += libdrm_intel
endif

# nouveau
ifneq ($(filter nouveau, $(MESA_GPU_DRIVERS)),)
gallium_DRIVERS +=  libmesa_winsys_nouveau libmesa_pipe_nouveau
LOCAL_SHARED_LIBRARIES += libdrm_nouveau
LOCAL_SHARED_LIBRARIES += libstlport
endif

# r300g/r600g/radeonsi
ifneq ($(filter r300g r600g radeonsi, $(MESA_GPU_DRIVERS)),)
gallium_DRIVERS += libmesa_winsys_radeon
LOCAL_SHARED_LIBRARIES += libdrm_radeon
ifneq ($(filter r300g, $(MESA_GPU_DRIVERS)),)
gallium_DRIVERS += libmesa_pipe_r300
endif # r300g
ifneq ($(filter r600g radeonsi, $(MESA_GPU_DRIVERS)),)
ifneq ($(filter r600g, $(MESA_GPU_DRIVERS)),)
gallium_DRIVERS += libmesa_pipe_r600
LOCAL_SHARED_LIBRARIES += libstlport
endif # r600g
ifneq ($(filter radeonsi, $(MESA_GPU_DRIVERS)),)
gallium_DRIVERS += libmesa_pipe_radeonsi
endif # radeonsi
gallium_DRIVERS += libmesa_pipe_radeon
endif # r600g || radeonsi
endif # r300g || r600g || radeonsi

# vmwgfx
ifneq ($(filter vmwgfx, $(MESA_GPU_DRIVERS)),)
gallium_DRIVERS += libmesa_winsys_svga libmesa_pipe_svga
endif

#
# Notes about the order here:
#
#  * libmesa_st_egl depends on libmesa_winsys_sw_android in $(gallium_DRIVERS)
#  * libmesa_pipe_r300 in $(gallium_DRIVERS) depends on libmesa_st_mesa and
#    libmesa_glsl
#  * libmesa_st_mesa depends on libmesa_glsl
#  * libmesa_glsl depends on libmesa_glsl_utils
#
LOCAL_STATIC_LIBRARIES := \
	libmesa_egl_gallium \
	libmesa_st_egl \
	$(gallium_DRIVERS) \
	libmesa_st_mesa \
	libmesa_util \
	libmesa_glsl \
	libmesa_glsl_utils \
	libmesa_gallium \
	$(LOCAL_STATIC_LIBRARIES)

endif # MESA_BUILD_GALLIUM

LOCAL_STATIC_LIBRARIES := \
	$(LOCAL_STATIC_LIBRARIES) \
	libmesa_loader

LOCAL_MODULE := libGLES_mesa
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/egl

include $(MESA_COMMON_MK)
include $(BUILD_SHARED_LIBRARY)
