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

# Android.mk for glsl

LOCAL_PATH := $(call my-dir)

include $(LOCAL_PATH)/Makefile.sources

GLSL_SRCDIR = .
# ---------------------------------------
# Build libmesa_glsl
# ---------------------------------------

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	$(LIBGLCPP_FILES) \
	$(LIBGLSL_FILES)

LOCAL_C_INCLUDES := \
	external/astl/include \
	$(MESA_TOP)/src/mapi \
	$(MESA_TOP)/src/mesa

LOCAL_MODULE := libmesa_glsl

include $(LOCAL_PATH)/Android.gen.mk
include $(MESA_COMMON_MK)
include $(BUILD_STATIC_LIBRARY)

# ---------------------------------------
# Build mesa_builtin_compiler for host
# ---------------------------------------

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	$(LIBGLCPP_FILES) \
	$(LIBGLSL_FILES) \
	$(BUILTIN_COMPILER_CXX_FILES) \
	$(GLSL_COMPILER_CXX_FILES)

LOCAL_C_INCLUDES := \
	$(MESA_TOP)/src/mapi \
	$(MESA_TOP)/src/mesa

LOCAL_STATIC_LIBRARIES := libmesa_glsl_utils

LOCAL_MODULE := mesa_builtin_compiler

LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_IS_HOST_MODULE := true
include $(LOCAL_PATH)/Android.gen.mk
include $(MESA_COMMON_MK)
include $(BUILD_HOST_EXECUTABLE)

# ---------------------------------------
# Build glsl_compiler
# ---------------------------------------

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	$(GLSL_COMPILER_CXX_FILES)

LOCAL_C_INCLUDES := \
	$(MESA_TOP)/src/mapi \
	$(MESA_TOP)/src/mesa

LOCAL_STATIC_LIBRARIES := libmesa_glsl libmesa_glsl_utils

LOCAL_MODULE_TAGS := eng
LOCAL_MODULE := glsl_compiler

include $(MESA_COMMON_MK)
include $(BUILD_EXECUTABLE)
