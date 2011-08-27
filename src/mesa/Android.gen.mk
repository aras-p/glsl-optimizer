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

sources := \
	main/api_exec_es1.c \
	main/api_exec_es1_dispatch.h \
	main/api_exec_es1_remap_helper.h \
	main/api_exec_es2.c \
	main/api_exec_es2_dispatch.h \
	main/api_exec_es2_remap_helper.h \
	program/lex.yy.c \
	program/program_parse.tab.c

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

es_src_deps := \
	$(LOCAL_PATH)/main/APIspec.xml \
	$(LOCAL_PATH)/main/es_generator.py \
	$(LOCAL_PATH)/main/APIspecutil.py \
	$(LOCAL_PATH)/main/APIspec.py

es_hdr_deps := \
	$(wildcard $(glapi)/*.py) \
	$(wildcard $(glapi)/*.xml)

define es-gen
	@mkdir -p $(dir $@)
	@echo "Gen ES: $(PRIVATE_MODULE) <= $(notdir $(@))"
	$(hide) $(PRIVATE_SCRIPT) $(1) $(PRIVATE_XML) > $@
endef

define local-l-to-c
	@mkdir -p $(dir $@)
	@echo "Mesa Lex: $(PRIVATE_MODULE) <= $<"
	$(hide) $(LEX) -o$@ $<
endef

define local-y-to-c-and-h
	@mkdir -p $(dir $@)
	@echo "Mesa Yacc: $(PRIVATE_MODULE) <= $<"
	$(hide) $(YACC) -o $@ $<
endef

$(intermediates)/main/api_exec_%.c: PRIVATE_SCRIPT := $(MESA_PYTHON2) $(LOCAL_PATH)/main/es_generator.py
$(intermediates)/main/api_exec_%.c: PRIVATE_XML := -S $(LOCAL_PATH)/main/APIspec.xml
$(intermediates)/main/api_exec_%_dispatch.h: PRIVATE_SCRIPT := $(MESA_PYTHON2) $(glapi)/gl_table.py
$(intermediates)/main/api_exec_%_dispatch.h: PRIVATE_XML := -f $(glapi)/gl_and_es_API.xml
$(intermediates)/main/api_exec_%_remap_helper.h: PRIVATE_SCRIPT := $(MESA_PYTHON2) $(glapi)/remap_helper.py
$(intermediates)/main/api_exec_%_remap_helper.h: PRIVATE_XML := -f $(glapi)/gl_and_es_API.xml

$(intermediates)/main/api_exec_es1.c: $(es_src_deps)
	$(call es-gen,-V GLES1.1)

$(intermediates)/main/api_exec_es2.c: $(es_src_deps)
	$(call es-gen,-V GLES2.0)

$(intermediates)/main/api_exec_%_dispatch.h: $(es_hdr_deps)
	$(call es-gen, -c $* -m remap_table)

$(intermediates)/main/api_exec_%_remap_helper.h: $(es_hdr_deps)
	$(call es-gen, -c $*)

$(intermediates)/program/program_parse.tab.c: $(LOCAL_PATH)/program/program_parse.y
	$(local-y-to-c-and-h)

$(intermediates)/program/lex.yy.c: $(LOCAL_PATH)/program/program_lexer.l
	$(local-l-to-c)

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
