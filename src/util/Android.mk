# Mesa 3-D graphics library
#
# Copyright (C) 2014 Tomasz Figa <tomasz.figa@gmail.com>
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
# Build libmesa_util
# ---------------------------------------

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	$(MESA_UTIL_FILES)

LOCAL_C_INCLUDES := \
	$(MESA_TOP)/src/mesa \
	$(MESA_TOP)/src/mapi \
	$(MESA_TOP)/src

LOCAL_MODULE := libmesa_util

# Generated sources

ifeq ($(LOCAL_MODULE_CLASS),)
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
endif

intermediates := $(call local-intermediates-dir)

# This is the list of auto-generated files: sources and headers
sources := $(addprefix $(intermediates)/, $(MESA_UTIL_GENERATED_FILES))

LOCAL_GENERATED_SOURCES += $(sources)

FORMAT_SRGB := $(LOCAL_PATH)/format_srgb.py

$(intermediates)/format_srgb.c: $(FORMAT_SRGB)
	@$(MESA_PYTHON2) $(FORMAT_SRGB) $< > $@

include $(MESA_COMMON_MK)
include $(BUILD_STATIC_LIBRARY)

# ---------------------------------------
# Build host libmesa_util
# ---------------------------------------

include $(CLEAR_VARS)

LOCAL_IS_HOST_MODULE := true
LOCAL_CFLAGS := -D_POSIX_C_SOURCE=199309L

LOCAL_SRC_FILES := \
	$(MESA_UTIL_FILES)

LOCAL_C_INCLUDES := \
	$(MESA_TOP)/src/mesa \
	$(MESA_TOP)/src/mapi \
	$(MESA_TOP)/src

LOCAL_MODULE := libmesa_util

# Generated sources

ifeq ($(LOCAL_MODULE_CLASS),)
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
endif

intermediates := $(call local-intermediates-dir)

# This is the list of auto-generated files: sources and headers
sources := $(addprefix $(intermediates)/, $(MESA_UTIL_GENERATED_FILES))

LOCAL_GENERATED_SOURCES += $(sources)

FORMAT_SRGB := $(LOCAL_PATH)/format_srgb.py

$(intermediates)/format_srgb.c: $(FORMAT_SRGB)
	@$(MESA_PYTHON2) $(FORMAT_SRGB) $< > $@

include $(MESA_COMMON_MK)
include $(BUILD_HOST_STATIC_LIBRARY)
