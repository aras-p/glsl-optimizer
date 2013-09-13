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

# included by glsl Android.mk for source generation

ifeq ($(LOCAL_MODULE_CLASS),)
LOCAL_MODULE_CLASS := STATIC_LIBRARIES
endif

intermediates := $(call local-intermediates-dir)

sources := \
	glsl_lexer.cpp \
	glsl_parser.cpp \
	glcpp/glcpp-lex.c \
	glcpp/glcpp-parse.c

LOCAL_SRC_FILES := $(filter-out $(sources), $(LOCAL_SRC_FILES))

LOCAL_C_INCLUDES += $(intermediates) $(intermediates)/glcpp $(MESA_TOP)/src/glsl/glcpp

sources := $(addprefix $(intermediates)/, $(sources))
LOCAL_GENERATED_SOURCES += $(sources)

define local-l-or-ll-to-c-or-cpp
	@mkdir -p $(dir $@)
	@echo "Mesa Lex: $(PRIVATE_MODULE) <= $<"
	$(hide) $(LEX) --nounistd -o$@ $<
endef

define glsl_local-y-to-c-and-h
	@mkdir -p $(dir $@)
	@echo "Mesa Yacc: $(PRIVATE_MODULE) <= $<"
	$(hide) $(YACC) -o $@ -p "glcpp_parser_" $<
endef

define local-yy-to-cpp-and-h
	@mkdir -p $(dir $@)
	@echo "Mesa Yacc: $(PRIVATE_MODULE) <= $<"
	$(hide) $(YACC) -p "_mesa_glsl_" -o $@ $<
	touch $(@:$1=$(YACC_HEADER_SUFFIX))
	echo '#ifndef '$(@F:$1=_h) > $(@:$1=.h)
	echo '#define '$(@F:$1=_h) >> $(@:$1=.h)
	cat $(@:$1=$(YACC_HEADER_SUFFIX)) >> $(@:$1=.h)
	echo '#endif' >> $(@:$1=.h)
	rm -f $(@:$1=$(YACC_HEADER_SUFFIX))
endef

$(intermediates)/glsl_lexer.cpp: $(LOCAL_PATH)/glsl_lexer.ll
	$(call local-l-or-ll-to-c-or-cpp)

$(intermediates)/glsl_parser.cpp: $(LOCAL_PATH)/glsl_parser.yy
	$(call local-yy-to-cpp-and-h,.cpp)

$(intermediates)/glcpp/glcpp-lex.c: $(LOCAL_PATH)/glcpp/glcpp-lex.l
	$(call local-l-or-ll-to-c-or-cpp)

$(intermediates)/glcpp/glcpp-parse.c: $(LOCAL_PATH)/glcpp/glcpp-parse.y
	$(call glsl_local-y-to-c-and-h)
