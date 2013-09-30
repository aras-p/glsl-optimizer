# Copyright 2012 Intel Corporation
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

# ----------------------------------------------------------------------
# libmesa_st_mesa.a
# ----------------------------------------------------------------------

ifeq ($(strip $(MESA_BUILD_GALLIUM)),true)

LOCAL_PATH := $(call my-dir)

# Import variables:
# 	MESA_GALLIUM_FILES.
# 	X86_FILES
include $(LOCAL_PATH)/Makefile.sources

include $(CLEAR_VARS)

LOCAL_MODULE := libmesa_st_mesa

LOCAL_SRC_FILES := \
	$(MESA_GALLIUM_FILES)

ifeq ($(strip $(MESA_ENABLE_ASM)),true)
ifeq ($(TARGET_ARCH),x86)
	LOCAL_SRC_FILES += $(X86_FILES)
endif # x86
endif # MESA_ENABLE_ASM

LOCAL_C_INCLUDES := \
	$(call intermediates-dir-for STATIC_LIBRARIES,libmesa_program,,) \
	$(MESA_TOP)/src/gallium/auxiliary \
	$(MESA_TOP)/src/gallium/include \
	$(MESA_TOP)/src/glsl \
	$(MESA_TOP)/src/mapi

LOCAL_WHOLE_STATIC_LIBRARIES := \
	libmesa_program

include $(LOCAL_PATH)/Android.gen.mk
include $(MESA_COMMON_MK)
include $(BUILD_STATIC_LIBRARY)

endif # MESA_BUILD_GALLIUM
