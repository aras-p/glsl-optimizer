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

bin_PROGRAMS = glsl
glsl_SOURCES = symbol_table.c hash_table.c glsl_types.cpp \
	glsl_parser.ypp glsl_lexer.lpp glsl_parser_extras.cpp \
	ast_expr.cpp ast_to_hir.cpp ast_function.cpp ast_type.cpp \
	ir.cpp hir_field_selection.cpp \
	ir_print_visitor.cpp ir_variable.cpp ir_function.cpp

BUILT_SOURCES = glsl_parser.h builtin_types.h

glsl_parser.h: glsl_parser.ypp

.lpp.cpp:
	$(LEXCOMPILE) --outfile="$@" $<

builtin_types.h: builtin_types.sh
	sh ./builtin_types.sh > builtin_types.h
