#
# Copyright (C) 2011 Intel Corporation
# Copyright (C) 2010-2011 Chia-I Wu <olvaffe@gmail.com>
# Copyright (C) 2010-2011 LunarG
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
#

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := i965_dri
LOCAL_MODULE_PATH := $(MESA_DRI_MODULE_PATH)
LOCAL_UNSTRIPPED_PATH := $(MESA_DRI_MODULE_UNSTRIPPED_PATH)

# Import variables i965_*.
include $(LOCAL_PATH)/Makefile.sources

LOCAL_CFLAGS := \
	$(MESA_DRI_CFLAGS)

LOCAL_C_INCLUDES := \
	$(i965_INCLUDES) \
	$(MESA_DRI_C_INCLUDES) \
	$(DRM_TOP)/intel

LOCAL_SRC_FILES := \
	$(i965_FILES)

LOCAL_WHOLE_STATIC_LIBRARIES := \
	$(MESA_DRI_WHOLE_STATIC_LIBRARIES)

LOCAL_SHARED_LIBRARIES := \
	$(MESA_DRI_SHARED_LIBRARIES) \
	libdrm_intel

LOCAL_GENERATED_SOURCES := \
	$(MESA_DRI_OPTIONS_H)

include $(MESA_COMMON_MK)
include $(BUILD_SHARED_LIBRARY)
