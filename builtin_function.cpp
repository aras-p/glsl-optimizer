/*
 * Copyright © 2010 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <stdlib.h>
#include "glsl_symbol_table.h"
#include "glsl_parser_extras.h"
#include "glsl_types.h"
#include "ir.h"

static void
generate_exp(exec_list *instructions,
	     ir_variable **declarations,
	     const glsl_type *type)
{
   ir_dereference *const retval = new ir_dereference(declarations[16]);
   ir_dereference *const arg = new ir_dereference(declarations[0]);
   ir_rvalue *result;

   result = new ir_expression(ir_unop_exp, type, arg, NULL);

   ir_instruction *inst = new ir_assignment(retval, result, NULL);
   instructions->push_tail(inst);
}

void
generate_function_instance(ir_function *f,
			   const char *name,
			   exec_list *instructions,
			   void (*generate)(exec_list *instructions,
					    ir_variable **declarations,
					    const glsl_type *type),
			   const glsl_type *type)
{
   ir_variable *declarations[17];

   ir_function_signature *const sig = new ir_function_signature(type);
   f->signatures.push_tail(sig);

   ir_label *const label = new ir_label(name);
   instructions->push_tail(label);
   sig->definition = label;

   ir_variable *var = new ir_variable(type, "arg");

   var->mode = ir_var_in;
   sig->parameters.push_tail(var);

   var = new ir_variable(type, "arg");

   declarations[0] = var;

   ir_variable *retval = new ir_variable(type, "__retval");
   instructions->push_tail(retval);

   declarations[16] = retval;

   generate(instructions, declarations, type);
}

void
make_gentype_function(glsl_symbol_table *symtab, exec_list *instructions,
		      const char *name,
		      void (*generate)(exec_list *instructions,
				       ir_variable **declarations,
				       const glsl_type *type))
{
   ir_function *const f = new ir_function(name);
   const glsl_type *vec2_type = glsl_type::get_instance(GLSL_TYPE_FLOAT, 2, 1);
   const glsl_type *vec3_type = glsl_type::get_instance(GLSL_TYPE_FLOAT, 3, 1);
   const glsl_type *vec4_type = glsl_type::get_instance(GLSL_TYPE_FLOAT, 4, 1);

   bool added = symtab->add_function(name, f);
   assert(added);

   generate_function_instance(f, name, instructions, generate, glsl_type::float_type);
   generate_function_instance(f, name, instructions, generate, vec2_type);
   generate_function_instance(f, name, instructions, generate, vec3_type);
   generate_function_instance(f, name, instructions, generate, vec4_type);
}

void
generate_110_functions(glsl_symbol_table *symtab, exec_list *instructions)
{
   /* FINISHME: radians() */
   /* FINISHME: degrees() */
   /* FINISHME: sin() */
   /* FINISHME: cos() */
   /* FINISHME: tan() */
   /* FINISHME: asin() */
   /* FINISHME: acos() */
   /* FINISHME: atan(y,x) */
   /* FINISHME: atan(y/x) */
   /* FINISHME: pow() */
   make_gentype_function(symtab, instructions, "exp", generate_exp);
   /* FINISHME: log() */
   /* FINISHME: exp2() */
   /* FINISHME: log2() */
   /* FINISHME: sqrt() */
   /* FINISHME: inversesqrt() */
   /* FINISHME: abs() */
   /* FINISHME: sign() */
   /* FINISHME: floor() */
   /* FINISHME: ceil() */
   /* FINISHME: fract() */
   /* FINISHME: mod(x, float y) */
   /* FINISHME: mod(x, y) */
   /* FINISHME: min() */
   /* FINISHME: max() */
   /* FINISHME: clamp() */
   /* FINISHME: clamp() */
   /* FINISHME: mix() */
   /* FINISHME: mix() */
   /* FINISHME: step() */
   /* FINISHME: step() */
   /* FINISHME: smoothstep() */
   /* FINISHME: smoothstep() */
   /* FINISHME: floor() */
   /* FINISHME: step() */
   /* FINISHME: length() */
   /* FINISHME: distance() */
   /* FINISHME: dot() */
   /* FINISHME: cross() */
   /* FINISHME: normalize() */
   /* FINISHME: ftransform() */
   /* FINISHME: faceforward() */
   /* FINISHME: reflect() */
   /* FINISHME: refract() */
   /* FINISHME: matrixCompMult() */
   /* FINISHME: lessThan() */
   /* FINISHME: lessThanEqual() */
   /* FINISHME: greaterThan() */
   /* FINISHME: greaterThanEqual() */
   /* FINISHME: equal() */
   /* FINISHME: notEqual() */
   /* FINISHME: any() */
   /* FINISHME: all() */
   /* FINISHME: not() */
   /* FINISHME: texture*() */
   /* FINISHME: shadow*() */
   /* FINISHME: dFd[xy]() */
   /* FINISHME: fwidth() */
}

void
_mesa_glsl_initialize_functions(exec_list *instructions,
				struct _mesa_glsl_parse_state *state)
{
   generate_110_functions(state->symbols, instructions);
}
