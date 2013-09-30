# Copyright 2012 Intel Corporation
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

define local-l-to-c
	@mkdir -p $(dir $@)
	@echo "Mesa Lex: $(PRIVATE_MODULE) <= $<"
	$(hide) $(LEX) -o$@ $<
endef

define mesa_local-y-to-c-and-h
	@mkdir -p $(dir $@)
	@echo "Mesa Yacc: $(PRIVATE_MODULE) <= $<"
	$(hide) $(YACC) -o $@ -p "_mesa_program_" $<
endef

# ----------------------------------------------------------------------
# libmesa_program.a
# ----------------------------------------------------------------------

# Import the following variables:
#     PROGRAM_FILES
include $(MESA_TOP)/src/mesa/Makefile.sources

include $(CLEAR_VARS)

LOCAL_MODULE := libmesa_program
LOCAL_MODULE_CLASS := STATIC_LIBRARIES

intermediates := $(call local-intermediates-dir)

# TODO(chadv): In Makefile.sources, move these vars to a different list so we can
# remove this kludge.
generated_sources_basenames := \
	lex.yy.c \
	program_parse.tab.c \
	program_parse.tab.h

LOCAL_SRC_FILES := \
	$(filter-out $(generated_sources_basenames),$(subst program/,,$(PROGRAM_FILES)))

LOCAL_GENERATED_SOURCES := \
	$(addprefix $(intermediates)/program/,$(generated_sources_basenames))

$(intermediates)/program/program_parse.tab.c: $(LOCAL_PATH)/program_parse.y
	$(mesa_local-y-to-c-and-h)

$(intermediates)/program/program_parse.tab.h: $(intermediates)/program/program_parse.tab.c
	@

$(intermediates)/program/lex.yy.c: $(LOCAL_PATH)/program_lexer.l
	$(local-l-to-c)

LOCAL_C_INCLUDES := \
	$(intermediates) \
	$(MESA_TOP)/src/mapi \
	$(MESA_TOP)/src/mesa \
	$(MESA_TOP)/src/glsl

include $(MESA_COMMON_MK)
include $(BUILD_STATIC_LIBRARY)
