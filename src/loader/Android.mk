# Mesa 3-D graphics library
#
# Copyright (C) 2014 Emil Velikov <emil.l.velikov@gmail.com>
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

include $(LOCAL_PATH)/Makefile.sources

# ---------------------------------------
# Build libloader
# ---------------------------------------

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	$(LOADER_C_FILES)

# swrast only
ifeq ($(MESA_GPU_DRIVERS),swrast)
	LOCAL_CFLAGS += -D__NOT_HAVE_DRM_H
else
LOCAL_C_INCLUDES += \
	$(DRM_TOP)/include/drm \
	$(DRM_TOP)
endif

LOCAL_MODULE := libloader

include $(MESA_COMMON_MK)
include $(BUILD_STATIC_LIBRARY)
