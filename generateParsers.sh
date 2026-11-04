#!/bin/sh
# regenerate checked-in lexer/parser sources. Normal builds need none of these tools.
# requires bison, flex, python3, and Mako (python3 -m pip install Mako)
set -eu
cd "$(dirname "$0")"
python3 -c 'import mako.template'

# relative paths keep generator #line directives reproducible across checkouts
flex --nounistd -osrc/compiler/glsl/glcpp/glcpp-lex.c src/compiler/glsl/glcpp/glcpp-lex.l
flex --nounistd -osrc/compiler/glsl/glsl_lexer.cpp src/compiler/glsl/glsl_lexer.ll
bison -o src/compiler/glsl/glcpp/glcpp-parse.c -p glcpp_parser_ --defines=src/compiler/glsl/glcpp/glcpp-parse.h src/compiler/glsl/glcpp/glcpp-parse.y
bison -o src/compiler/glsl/glsl_parser.cpp -p _mesa_glsl_ --defines=src/compiler/glsl/glsl_parser.h src/compiler/glsl/glsl_parser.yy
python3 src/compiler/glsl/ir_expression_operation.py enum > src/compiler/glsl/ir_expression_operation.h
python3 src/compiler/glsl/ir_expression_operation.py strings > src/compiler/glsl/ir_expression_operation_strings.h
python3 src/compiler/glsl/ir_expression_operation.py constant > src/compiler/glsl/ir_expression_operation_constant.h
