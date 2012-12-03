#
# Copyright (C) 2011 Intel Corporation
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

# Import mesa_dri_common_INCLUDES.
include $(LOCAL_PATH)/common/Makefile.sources

#-----------------------------------------------
# Variables common to all DRI drivers

MESA_DRI_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/dri
MESA_DRI_MODULE_UNSTRIPPED_PATH := $(TARGET_OUT_SHARED_LIBRARIES_UNSTRIPPED)/dri

MESA_DRI_CFLAGS := \
	-DFEATURE_GL=1 \
	-DFEATURE_ES1=1 \
	-DFEATURE_ES2=1 \
	-DHAVE_ANDROID_PLATFORM

MESA_DRI_C_INCLUDES := \
	$(call intermediates-dir-for,STATIC_LIBRARIES,libmesa_dri_common) \
	$(addprefix $(MESA_TOP)/, $(mesa_dri_common_INCLUDES)) \
	$(DRM_TOP) \
	$(DRM_TOP)/include/drm \
	external/expat/lib

MESA_DRI_WHOLE_STATIC_LIBRARIES := \
	libmesa_glsl \
	libmesa_dri_common \
	libmesa_dricore

MESA_DRI_SHARED_LIBRARIES := \
	libcutils \
	libdl \
	libdrm \
	libexpat \
	libglapi \
	liblog

# All DRI modules must add this to LOCAL_GENERATED_SOURCES.
MESA_DRI_OPTIONS_H := $(call intermediates-dir-for,STATIC_LIBRARIES,libmesa_dri_common)/xmlpool/options.h

#-----------------------------------------------
# Build drivers and libmesa_dri_common

SUBDIRS := common

ifneq ($(filter i915, $(MESA_GPU_DRIVERS)),)
	SUBDIRS += i915
endif

ifneq ($(filter i965, $(MESA_GPU_DRIVERS)),)
	SUBDIRS += i965
endif

include $(foreach d, $(SUBDIRS), $(LOCAL_PATH)/$(d)/Android.mk)
