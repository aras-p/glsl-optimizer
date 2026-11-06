#!/bin/sh
# Regenerate checked-in generated sources: GLSL/glcpp lexers and parsers, GLSL
# IR and NIR tables. Normal builds need none of these tools.
# requires bison (3.x), flex, python3 with Mako (python3 -m pip install Mako)
set -eu
cd "$(dirname "$0")/.."
PYTHON=${PYTHON:-python3}
"$PYTHON" -c 'import mako.template'

# relative paths keep generator #line directives reproducible across checkouts

# GLSL preprocessor and parser
flex -o src/compiler/glsl/glcpp/glcpp-lex.c src/compiler/glsl/glcpp/glcpp-lex.l
flex -o src/compiler/glsl/glsl_lexer.cpp src/compiler/glsl/glsl_lexer.ll
bison -Wno-deprecated -o src/compiler/glsl/glcpp/glcpp-parse.c -p glcpp_parser_ --defines=src/compiler/glsl/glcpp/glcpp-parse.h src/compiler/glsl/glcpp/glcpp-parse.y
bison -Wno-deprecated -o src/compiler/glsl/glsl_parser.cpp -p _mesa_glsl_ --defines=src/compiler/glsl/glsl_parser.h src/compiler/glsl/glsl_parser.yy

# GLSL IR
"$PYTHON" src/compiler/glsl/ir_expression_operation.py enum > src/compiler/ir_expression_operation.h
"$PYTHON" src/compiler/glsl/ir_expression_operation.py strings > src/compiler/glsl/ir_expression_operation_strings.h
"$PYTHON" src/compiler/glsl/ir_expression_operation.py constant > src/compiler/glsl/ir_expression_operation_constant.h

# GLSL types
"$PYTHON" src/compiler/builtin_types_h.py src/compiler/builtin_types.h
"$PYTHON" src/compiler/builtin_types_c.py src/compiler/builtin_types.c

# NIR
"$PYTHON" src/compiler/nir/nir_opcodes_h.py > src/compiler/nir/nir_opcodes.h
"$PYTHON" src/compiler/nir/nir_opcodes_c.py > src/compiler/nir/nir_opcodes.c
"$PYTHON" src/compiler/nir/nir_builder_opcodes_h.py > src/compiler/nir/nir_builder_opcodes.h
"$PYTHON" src/compiler/nir/nir_constant_expressions.py > src/compiler/nir/nir_constant_expressions.c
"$PYTHON" src/compiler/nir/nir_intrinsics_h.py --out src/compiler/nir/nir_intrinsics.h
"$PYTHON" src/compiler/nir/nir_intrinsics_c.py --out src/compiler/nir/nir_intrinsics.c
"$PYTHON" src/compiler/nir/nir_intrinsics_indices_h.py --out src/compiler/nir/nir_intrinsics_indices.h
"$PYTHON" src/compiler/nir/nir_opt_algebraic.py --passes nir_opt_algebraic,nir_opt_algebraic_late --out src/compiler/nir/nir_opt_algebraic.c

# util formats: minimal pipe_format enum/descriptions are generated from an
# upstream Mesa tree, see upgrade-tools/gen_minimal_formats.py

"$PYTHON" upgrade-tools/compact_c_tables.py src/compiler/nir/nir_opt_algebraic.c
"$PYTHON" upgrade-tools/compact_c_tables.py --blank-lines-only src/compiler/nir/nir_constant_expressions.c
