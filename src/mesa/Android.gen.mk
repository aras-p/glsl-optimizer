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

# included by core mesa Android.mk for source generation

ifeq ($(LOCAL_MODULE_CLASS),)
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
endif

intermediates := $(call local-intermediates-dir)

# This is the list of auto-generated files: sources and headers
sources := \
	main/enums.c \
	main/api_exec.c \
	main/dispatch.h \
	main/remap_helper.h \
	main/get_hash.h

LOCAL_SRC_FILES := $(filter-out $(sources), $(LOCAL_SRC_FILES))

LOCAL_C_INCLUDES += $(intermediates)/main

ifeq ($(strip $(MESA_ENABLE_ASM)),true)
ifeq ($(TARGET_ARCH),x86)
sources += x86/matypes.h
LOCAL_C_INCLUDES += $(intermediates)/x86
endif
endif

sources += main/git_sha1.h

sources := $(addprefix $(intermediates)/, $(sources))

LOCAL_GENERATED_SOURCES += $(sources)

glapi := $(MESA_TOP)/src/mapi/glapi/gen

dispatch_deps := \
	$(wildcard $(glapi)/*.py) \
	$(wildcard $(glapi)/*.xml)

define es-gen
	@mkdir -p $(dir $@)
	@echo "Gen ES: $(PRIVATE_MODULE) <= $(notdir $(@))"
	$(hide) $(PRIVATE_SCRIPT) $(1) $(PRIVATE_XML) > $@
endef

$(intermediates)/main/git_sha1.h:
	@mkdir -p $(dir $@)
	@echo "GIT-SHA1: $(PRIVATE_MODULE) <= git"
	$(hide) touch $@
	$(hide) if which git > /dev/null; then \
			git --git-dir $(PRIVATE_PATH)/../../.git log -n 1 --oneline | \
			sed 's/^\([^ ]*\) .*/#define MESA_GIT_SHA1 "git-\1"/' \
			> $@; \
		fi

matypes_deps := \
	$(BUILD_OUT_EXECUTABLES)/mesa_gen_matypes$(BUILD_EXECUTABLE_SUFFIX) \
	$(LOCAL_PATH)/main/mtypes.h \
	$(LOCAL_PATH)/tnl/t_context.h

$(intermediates)/x86/matypes.h: $(matypes_deps) 
	@mkdir -p $(dir $@)
	@echo "MATYPES: $(PRIVATE_MODULE) <= $(notdir $@)"
	$(hide) $< > $@

$(intermediates)/main/dispatch.h: PRIVATE_SCRIPT := $(MESA_PYTHON2) $(glapi)/gl_table.py
$(intermediates)/main/dispatch.h: PRIVATE_XML := -f $(glapi)/gl_and_es_API.xml

$(intermediates)/main/dispatch.h: $(dispatch_deps)
	$(call es-gen, $* -m remap_table)

$(intermediates)/main/remap_helper.h: PRIVATE_SCRIPT := $(MESA_PYTHON2) $(glapi)/remap_helper.py
$(intermediates)/main/remap_helper.h: PRIVATE_XML := -f $(glapi)/gl_and_es_API.xml

$(intermediates)/main/remap_helper.h: $(dispatch_deps)
	$(call es-gen, $*)

$(intermediates)/main/enums.c: PRIVATE_SCRIPT :=$(MESA_PYTHON2) $(glapi)/gl_enums.py
$(intermediates)/main/enums.c: PRIVATE_XML := -f $(glapi)/gl_and_es_API.xml

$(intermediates)/main/enums.c: $(dispatch_deps)
	$(call es-gen)

$(intermediates)/main/api_exec.c: PRIVATE_SCRIPT := $(MESA_PYTHON2) $(glapi)/gl_genexec.py
$(intermediates)/main/api_exec.c: PRIVATE_XML := -f $(glapi)/gl_and_es_API.xml

$(intermediates)/main/api_exec.c: $(dispatch_deps)
	$(call es-gen)

GET_HASH_GEN := $(LOCAL_PATH)/main/get_hash_generator.py

$(intermediates)/main/get_hash.h: $(glapi)/gl_and_es_API.xml \
               $(LOCAL_PATH)/main/get_hash_params.py $(GET_HASH_GEN)
	@$(MESA_PYTHON2) $(GET_HASH_GEN) -f $< > $@
