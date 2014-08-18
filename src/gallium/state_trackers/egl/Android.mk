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

LOCAL_PATH := $(call my-dir)

# get common_FILES, android_FILES
include $(LOCAL_PATH)/Makefile.sources

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	$(common_FILES) \
	$(android_FILES)

LOCAL_CFLAGS := \
	-DFEATURE_ES1=1 \
	-DFEATURE_ES2=1 \
	-DHAVE_ANDROID_BACKEND

LOCAL_C_INCLUDES := \
	$(GALLIUM_TOP)/state_trackers/egl \
	$(GALLIUM_TOP)/winsys \
	$(MESA_TOP)/src/egl/main

# swrast only
ifeq ($(MESA_GPU_DRIVERS),swrast)
LOCAL_CFLAGS += -DANDROID_BACKEND_NO_DRM
else
LOCAL_C_INCLUDES += $(DRM_GRALLOC_TOP)
endif

LOCAL_MODULE := libmesa_st_egl

include $(GALLIUM_COMMON_MK)
include $(BUILD_STATIC_LIBRARY)
