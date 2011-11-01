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

# from Makefile
LIBGLCPP_SOURCES = \
	glcpp/glcpp-lex.c \
	glcpp/glcpp-parse.c \
	glcpp/pp.c

C_SOURCES = \
	strtod.c \
	ralloc.c \
	$(LIBGLCPP_SOURCES)

CXX_SOURCES = \
	ast_expr.cpp \
	ast_function.cpp \
	ast_to_hir.cpp \
	ast_type.cpp \
	glsl_lexer.cpp \
	glsl_parser.cpp \
	glsl_parser_extras.cpp \
	glsl_types.cpp \
	glsl_symbol_table.cpp \
	hir_field_selection.cpp \
	ir_basic_block.cpp \
	ir_clone.cpp \
	ir_constant_expression.cpp \
	ir.cpp \
	ir_expression_flattening.cpp \
	ir_function_can_inline.cpp \
	ir_function_detect_recursion.cpp \
	ir_function.cpp \
	ir_hierarchical_visitor.cpp \
	ir_hv_accept.cpp \
	ir_import_prototypes.cpp \
	ir_print_visitor.cpp \
	ir_reader.cpp \
	ir_rvalue_visitor.cpp \
	ir_set_program_inouts.cpp \
	ir_validate.cpp \
	ir_variable.cpp \
	ir_variable_refcount.cpp \
	linker.cpp \
	link_functions.cpp \
	link_uniforms.cpp \
	loop_analysis.cpp \
	loop_controls.cpp \
	loop_unroll.cpp \
	lower_clip_distance.cpp \
	lower_discard.cpp \
	lower_if_to_cond_assign.cpp \
	lower_instructions.cpp \
	lower_jumps.cpp \
	lower_mat_op_to_vec.cpp \
	lower_noise.cpp \
	lower_texture_projection.cpp \
	lower_variable_index_to_cond_assign.cpp \
	lower_vec_index_to_cond_assign.cpp \
	lower_vec_index_to_swizzle.cpp \
	lower_vector.cpp \
	opt_algebraic.cpp \
	opt_constant_folding.cpp \
	opt_constant_propagation.cpp \
	opt_constant_variable.cpp \
	opt_copy_propagation.cpp \
	opt_copy_propagation_elements.cpp \
	opt_dead_code.cpp \
	opt_dead_code_local.cpp \
	opt_dead_functions.cpp \
	opt_discard_simplification.cpp \
	opt_function_inlining.cpp \
	opt_if_simplification.cpp \
	opt_noop_swizzle.cpp \
	opt_redundant_jumps.cpp \
	opt_structure_splitting.cpp \
	opt_swizzle_swizzle.cpp \
	opt_tree_grafting.cpp \
	s_expression.cpp

# ---------------------------------------
# Build libmesa_glsl
# ---------------------------------------

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	$(C_SOURCES) \
	$(CXX_SOURCES) \
	builtin_function.cpp

LOCAL_C_INCLUDES := \
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
	$(C_SOURCES) \
	$(CXX_SOURCES) \
	builtin_stubs.cpp \
	main.cpp \
	standalone_scaffolding.cpp

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
	main.cpp \
	standalone_scaffolding.cpp

LOCAL_C_INCLUDES := \
	$(MESA_TOP)/src/mapi \
	$(MESA_TOP)/src/mesa

LOCAL_STATIC_LIBRARIES := libmesa_glsl libmesa_glsl_utils

LOCAL_MODULE_TAGS := eng
LOCAL_MODULE := glsl_compiler

include $(MESA_COMMON_MK)
include $(BUILD_EXECUTABLE)
