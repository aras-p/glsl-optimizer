# Copyright © 2010 Intel Corporation
# All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# on the rights to use, copy, modify, merge, publish, distribute, sub
# license, and/or sell copies of the Software, and to permit persons to whom
# the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
# AUTHORS, COPYRIGHT HOLDERS, AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
# DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
# OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
# USE OR OTHER DEALINGS IN THE SOFTWARE.

AUTOMAKE_OPTIONS = foreign
AM_CPPFLAGS = -I mesa

SUBDIRS = glcpp

bin_PROGRAMS = glsl

glsl_LDADD = ./glcpp/libglcpp.la
glsl_LDFLAGS = @LDFLAGS@ $(talloc_LIBS)
glsl_SOURCES = \
	main.cpp \
	builtin_types.h \
	glsl_types.cpp \
	glsl_parser.ypp glsl_lexer.lpp glsl_parser_extras.cpp \
	ast_expr.cpp ast_to_hir.cpp ast_function.cpp ast_type.cpp \
	ir.cpp hir_field_selection.cpp builtin_function.cpp \
	ir_print_visitor.cpp ir_variable.cpp ir_function.cpp \
	ir_basic_block.cpp \
	ir_basic_block.h \
	ir_clone.cpp \
	ir_constant_expression.cpp \
	ir_constant_folding.cpp \
	ir_constant_variable.cpp \
	ir_copy_propagation.cpp \
	ir_copy_propagation.h \
	ir_dead_code.cpp \
	ir_dead_code.h \
	ir_dead_code_local.cpp \
	ir_expression_flattening.cpp \
	ir_function_can_inline.cpp \
	ir_function_inlining.cpp \
	ir_if_simplification.cpp \
	ir_optimization.h \
	ir_reader.cpp s_expression.cpp \
	ir_hv_accept.cpp \
	ir_hierarchical_visitor.h \
	ir_hierarchical_visitor.cpp \
	ir_swizzle_swizzle.cpp \
	ir_to_mesa.cpp \
	ir_to_mesa.h \
	ir_validate.cpp \
	ir_vec_index_to_swizzle.cpp \
	linker.cpp \
	loop_analysis.cpp \
	loop_controls.cpp \
	loop_unroll.cpp \
	lower_noise.cpp \
	lower_variable_index_to_cond_assign.cpp \
	opt_redundant_jumps.cpp

BUILT_SOURCES = glsl_parser.h glsl_parser.cpp glsl_lexer.cpp
CLEANFILES = $(BUILT_SOURCES)

builtin_function.cpp: builtins/*/*
	./builtins/tools/generate_builtins.pl > builtin_function.cpp
glsl_parser.h: glsl_parser.cpp

.lpp.cpp:
	$(LEXCOMPILE) --outfile="$@" $<
