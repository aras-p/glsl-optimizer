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
#include <math.h>
#include "glsl_symbol_table.h"
#include "glsl_parser_extras.h"
#include "glsl_types.h"
#include "ir.h"

static void
generate_unop(exec_list *instructions,
	      ir_variable **declarations,
	      const glsl_type *type,
	      enum ir_expression_operation op)
{
   ir_dereference *const arg = new ir_dereference(declarations[0]);
   ir_rvalue *result;

   result = new ir_expression(op, type, arg, NULL);

   ir_instruction *inst = new ir_return(result);
   instructions->push_tail(inst);
}

static void
generate_binop(exec_list *instructions,
	       ir_variable **declarations,
	       const glsl_type *type,
	       enum ir_expression_operation op)
{
   ir_dereference *const arg1 = new ir_dereference(declarations[0]);
   ir_dereference *const arg2 = new ir_dereference(declarations[1]);
   ir_rvalue *result;

   result = new ir_expression(op, type, arg1, arg2);

   ir_instruction *inst = new ir_return(result);
   instructions->push_tail(inst);
}

static void
generate_radians(exec_list *instructions,
		 ir_variable **declarations,
		 const glsl_type *type)
{
   ir_dereference *const arg = new ir_dereference(declarations[0]);
   ir_rvalue *result;

   result = new ir_expression(ir_binop_mul, type,
			      arg,
			      new ir_constant((float)(M_PI / 180.0)));

   ir_instruction *inst = new ir_return(result);
   instructions->push_tail(inst);
}

static void
generate_degrees(exec_list *instructions,
		 ir_variable **declarations,
		 const glsl_type *type)
{
   ir_dereference *const arg = new ir_dereference(declarations[0]);
   ir_rvalue *result;

   result = new ir_expression(ir_binop_mul, type,
			      arg,
			      new ir_constant((float)(180.0 / M_PI)));

   ir_instruction *inst = new ir_return(result);
   instructions->push_tail(inst);
}

static void
generate_exp(exec_list *instructions,
	     ir_variable **declarations,
	     const glsl_type *type)
{
   generate_unop(instructions, declarations, type, ir_unop_exp);
}

static void
generate_log(exec_list *instructions,
	     ir_variable **declarations,
	     const glsl_type *type)
{
   generate_unop(instructions, declarations, type, ir_unop_log);
}

static void
generate_exp2(exec_list *instructions,
	      ir_variable **declarations,
	      const glsl_type *type)
{
   generate_unop(instructions, declarations, type, ir_unop_exp2);
}

static void
generate_log2(exec_list *instructions,
	      ir_variable **declarations,
	      const glsl_type *type)
{
   generate_unop(instructions, declarations, type, ir_unop_log2);
}

static void
generate_rsq(exec_list *instructions,
	       ir_variable **declarations,
	       const glsl_type *type)
{
   generate_unop(instructions, declarations, type, ir_unop_rsq);
}

static void
generate_sqrt(exec_list *instructions,
	       ir_variable **declarations,
	       const glsl_type *type)
{
   generate_unop(instructions, declarations, type, ir_unop_sqrt);
}

static void
generate_abs(exec_list *instructions,
	     ir_variable **declarations,
	     const glsl_type *type)
{
   generate_unop(instructions, declarations, type, ir_unop_abs);
}

static void
generate_ceil(exec_list *instructions,
	      ir_variable **declarations,
	      const glsl_type *type)
{
   generate_unop(instructions, declarations, type, ir_unop_ceil);
}

static void
generate_floor(exec_list *instructions,
	       ir_variable **declarations,
	       const glsl_type *type)
{
   generate_unop(instructions, declarations, type, ir_unop_floor);
}

static void
generate_mod(exec_list *instructions,
	       ir_variable **declarations,
	       const glsl_type *type)
{
   generate_binop(instructions, declarations, type, ir_binop_mod);
}

static void
generate_min(exec_list *instructions,
	       ir_variable **declarations,
	       const glsl_type *type)
{
   generate_binop(instructions, declarations, type, ir_binop_min);
}

static void
generate_max(exec_list *instructions,
	       ir_variable **declarations,
	       const glsl_type *type)
{
   generate_binop(instructions, declarations, type, ir_binop_max);
}

static void
generate_mix_vec(exec_list *instructions,
		 ir_variable **declarations,
		 const glsl_type *type)
{
   ir_dereference *const x = new ir_dereference(declarations[0]);
   ir_dereference *const y = new ir_dereference(declarations[1]);
   ir_dereference *const a = new ir_dereference(declarations[2]);
   ir_rvalue *result, *temp;

   temp = new ir_expression(ir_binop_sub, type, new ir_constant(1.0f), a);
   result = new ir_expression(ir_binop_mul, type, x, temp);

   temp = new ir_expression(ir_binop_mul, type, y, a);
   result = new ir_expression(ir_binop_add, type, result, temp);

   ir_instruction *inst = new ir_return(result);
   instructions->push_tail(inst);
}


static void
generate_normalize(exec_list *instructions,
		   ir_variable **declarations,
		   const glsl_type *type)
{
   ir_dereference *const arg = new ir_dereference(declarations[0]);
   ir_rvalue *temp;
   ir_rvalue *result;

   temp = new ir_expression(ir_binop_dot, glsl_type::float_type, arg, arg);
   temp = new ir_expression(ir_unop_rsq, glsl_type::float_type, temp, NULL);
   result = new ir_expression(ir_binop_mul, type, arg, temp);

   ir_instruction *inst = new ir_return(result);
   instructions->push_tail(inst);
}


static void
generate_pow(exec_list *instructions,
	       ir_variable **declarations,
	       const glsl_type *type)
{
   generate_binop(instructions, declarations, type, ir_binop_pow);
}

void
generate_function_instance(ir_function *f,
			   const char *name,
			   exec_list *instructions,
			   int n_args,
			   void (*generate)(exec_list *instructions,
					    ir_variable **declarations,
					    const glsl_type *type),
			   const glsl_type *ret_type,
			   const glsl_type *type)
{
   ir_variable *declarations[16];

   ir_function_signature *const sig = new ir_function_signature(ret_type);
   f->add_signature(sig);

   ir_label *const label = new ir_label(name, sig);
   instructions->push_tail(label);
   sig->definition = label;
   static const char *arg_names[] = {
      "arg0",
      "arg1",
      "arg2"
   };
   int i;

   for (i = 0; i < n_args; i++) {
      ir_variable *var = new ir_variable(type, arg_names[i]);

      var->mode = ir_var_in;
      sig->parameters.push_tail(var);

      declarations[i] = var;
   }

   generate(&sig->body, declarations, type);
}

void
make_gentype_function(glsl_symbol_table *symtab, exec_list *instructions,
		      const char *name,
		      int n_args,
		      void (*generate)(exec_list *instructions,
				       ir_variable **declarations,
				       const glsl_type *type))
{
   ir_function *const f = new ir_function(name);
   const glsl_type *float_type = glsl_type::float_type;
   const glsl_type *vec2_type = glsl_type::get_instance(GLSL_TYPE_FLOAT, 2, 1);
   const glsl_type *vec3_type = glsl_type::get_instance(GLSL_TYPE_FLOAT, 3, 1);
   const glsl_type *vec4_type = glsl_type::get_instance(GLSL_TYPE_FLOAT, 4, 1);

   bool added = symtab->add_function(name, f);
   assert(added);

   generate_function_instance(f, name, instructions, n_args, generate,
			      float_type, float_type);
   generate_function_instance(f, name, instructions, n_args, generate,
			      vec2_type, vec2_type);
   generate_function_instance(f, name, instructions, n_args, generate,
			      vec3_type, vec3_type);
   generate_function_instance(f, name, instructions, n_args, generate,
			      vec4_type, vec4_type);
}

static void
generate_vec_compare(exec_list *instructions,
		     ir_variable **declarations,
		     const glsl_type *type,
		     enum ir_expression_operation op)
{
   ir_dereference *const x = new ir_dereference(declarations[0]);
   ir_dereference *const y = new ir_dereference(declarations[1]);
   ir_variable *temp;
   const glsl_type *return_type;
   int i;

   return_type = glsl_type::get_instance(GLSL_TYPE_BOOL,
					 type->vector_elements, 1);
   temp = new ir_variable(return_type, "temp");

   for (i = 0; i < type->vector_elements; i++) {
      ir_assignment *assign;
      ir_expression *compare;

      compare = new ir_expression(op,
				  glsl_type::get_instance(type->base_type,
							  1, 1),
				  new ir_swizzle(x, i, 0, 0, 0, 1),
				  new ir_swizzle(y, i, 0, 0, 0, 1));
      assign = new ir_assignment(new ir_swizzle(new ir_dereference(temp),
						i, 0, 0, 0, 1),
				 compare, NULL);
      instructions->push_tail(assign);
   }
   ir_instruction *inst = new ir_return(new ir_dereference(temp));
   instructions->push_tail(inst);
}

static void
generate_lessThan(exec_list *instructions,
		  ir_variable **declarations,
		  const glsl_type *type)
{
   generate_vec_compare(instructions, declarations, type, ir_binop_less);
}

static void
generate_lessThanEqual(exec_list *instructions,
		       ir_variable **declarations,
		       const glsl_type *type)
{
   generate_vec_compare(instructions, declarations, type, ir_binop_lequal);
}

static void
generate_greaterThan(exec_list *instructions,
		     ir_variable **declarations,
		     const glsl_type *type)
{
   generate_vec_compare(instructions, declarations, type, ir_binop_greater);
}

static void
generate_greaterThanEqual(exec_list *instructions,
			  ir_variable **declarations,
			  const glsl_type *type)
{
   generate_vec_compare(instructions, declarations, type, ir_binop_gequal);
}

static void
generate_equal(exec_list *instructions,
	       ir_variable **declarations,
	       const glsl_type *type)
{
   generate_vec_compare(instructions, declarations, type, ir_binop_equal);
}

static void
generate_notEqual(exec_list *instructions,
		  ir_variable **declarations,
		  const glsl_type *type)
{
   generate_vec_compare(instructions, declarations, type, ir_binop_nequal);
}

static void
generate_vec_compare_function(glsl_symbol_table *symtab,
			      exec_list *instructions,
			      const char *name,
			      void (*generate)(exec_list *instructions,
					       ir_variable **declarations,
					       const glsl_type *type),
			      bool do_bool)
{
   ir_function *const f = new ir_function(name);
   const glsl_type *vec2_type = glsl_type::get_instance(GLSL_TYPE_FLOAT, 2, 1);
   const glsl_type *vec3_type = glsl_type::get_instance(GLSL_TYPE_FLOAT, 3, 1);
   const glsl_type *vec4_type = glsl_type::get_instance(GLSL_TYPE_FLOAT, 4, 1);
   const glsl_type *ivec2_type = glsl_type::get_instance(GLSL_TYPE_INT, 2, 1);
   const glsl_type *ivec3_type = glsl_type::get_instance(GLSL_TYPE_INT, 3, 1);
   const glsl_type *ivec4_type = glsl_type::get_instance(GLSL_TYPE_INT, 4, 1);
   const glsl_type *uvec2_type = glsl_type::get_instance(GLSL_TYPE_UINT, 2, 1);
   const glsl_type *uvec3_type = glsl_type::get_instance(GLSL_TYPE_UINT, 3, 1);
   const glsl_type *uvec4_type = glsl_type::get_instance(GLSL_TYPE_UINT, 4, 1);
   const glsl_type *bvec2_type = glsl_type::get_instance(GLSL_TYPE_BOOL, 2, 1);
   const glsl_type *bvec3_type = glsl_type::get_instance(GLSL_TYPE_BOOL, 3, 1);
   const glsl_type *bvec4_type = glsl_type::get_instance(GLSL_TYPE_BOOL, 4, 1);

   bool added = symtab->add_function(name, f);
   assert(added);

   generate_function_instance(f, name, instructions, 2, generate,
			      bvec2_type, vec2_type);
   generate_function_instance(f, name, instructions, 2, generate,
			      bvec3_type, vec3_type);
   generate_function_instance(f, name, instructions, 2, generate,
			      bvec4_type, vec4_type);

   generate_function_instance(f, name, instructions, 2, generate,
			      bvec2_type, ivec2_type);
   generate_function_instance(f, name, instructions, 2, generate,
			      bvec3_type, ivec3_type);
   generate_function_instance(f, name, instructions, 2, generate,
			      bvec4_type, ivec4_type);

   generate_function_instance(f, name, instructions, 2, generate,
			      bvec2_type, uvec2_type);
   generate_function_instance(f, name, instructions, 2, generate,
			      bvec3_type, uvec3_type);
   generate_function_instance(f, name, instructions, 2, generate,
			      bvec4_type, uvec4_type);

   if (do_bool) {
      generate_function_instance(f, name, instructions, 2, generate,
				 bvec2_type, bvec2_type);
      generate_function_instance(f, name, instructions, 2, generate,
				 bvec3_type, bvec3_type);
      generate_function_instance(f, name, instructions, 2, generate,
				 bvec4_type, bvec4_type);
   }
}

static void
generate_length(exec_list *instructions,
		ir_variable **declarations,
		const glsl_type *type)
{
   ir_dereference *const arg = new ir_dereference(declarations[0]);
   ir_rvalue *result, *temp;

   (void)type;

   /* FINISHME: implement the abs(arg) variant for length(float f) */

   temp = new ir_expression(ir_binop_dot, glsl_type::float_type, arg, arg);
   result = new ir_expression(ir_unop_sqrt, glsl_type::float_type, temp, NULL);

   ir_instruction *inst = new ir_return(result);
   instructions->push_tail(inst);
}

void
generate_length_functions(glsl_symbol_table *symtab, exec_list *instructions)
{
   const char *name = "length";
   ir_function *const f = new ir_function(name);
   const glsl_type *float_type = glsl_type::float_type;
   const glsl_type *vec2_type = glsl_type::get_instance(GLSL_TYPE_FLOAT, 2, 1);
   const glsl_type *vec3_type = glsl_type::get_instance(GLSL_TYPE_FLOAT, 3, 1);
   const glsl_type *vec4_type = glsl_type::get_instance(GLSL_TYPE_FLOAT, 4, 1);

   bool added = symtab->add_function(name, f);
   assert(added);

   generate_function_instance(f, name, instructions, 1, generate_length,
			      float_type, float_type);
   generate_function_instance(f, name, instructions, 1, generate_length,
			      float_type, vec2_type);
   generate_function_instance(f, name, instructions, 1, generate_length,
			      float_type, vec3_type);
   generate_function_instance(f, name, instructions, 1, generate_length,
			      float_type, vec4_type);
}

static void
generate_dot(exec_list *instructions,
		ir_variable **declarations,
		const glsl_type *type)
{
   ir_dereference *const arg0 = new ir_dereference(declarations[0]);
   ir_dereference *const arg1 = new ir_dereference(declarations[1]);
   ir_rvalue *result;

   (void)type;

   result = new ir_expression(ir_binop_dot, glsl_type::float_type, arg0, arg1);

   ir_instruction *inst = new ir_return(result);
   instructions->push_tail(inst);
}

void
generate_dot_functions(glsl_symbol_table *symtab, exec_list *instructions)
{
   const char *name = "dot";
   ir_function *const f = new ir_function(name);
   const glsl_type *float_type = glsl_type::float_type;
   const glsl_type *vec2_type = glsl_type::get_instance(GLSL_TYPE_FLOAT, 2, 1);
   const glsl_type *vec3_type = glsl_type::get_instance(GLSL_TYPE_FLOAT, 3, 1);
   const glsl_type *vec4_type = glsl_type::get_instance(GLSL_TYPE_FLOAT, 4, 1);

   bool added = symtab->add_function(name, f);
   assert(added);

   generate_function_instance(f, name, instructions, 2, generate_dot,
			      float_type, float_type);
   generate_function_instance(f, name, instructions, 2, generate_dot,
			      float_type, vec2_type);
   generate_function_instance(f, name, instructions, 2, generate_dot,
			      float_type, vec3_type);
   generate_function_instance(f, name, instructions, 2, generate_dot,
			      float_type, vec4_type);
}

static void
generate_any_bvec2(exec_list *instructions,
		   ir_variable **declarations,
		   const glsl_type *type)
{
   ir_dereference *const arg0 = new ir_dereference(declarations[0]);
   ir_rvalue *result;

   (void)type;

   result = new ir_expression(ir_binop_logic_or, glsl_type::bool_type,
			      new ir_swizzle(arg0, 0, 0, 0, 0, 1),
			      new ir_swizzle(arg0, 1, 0, 0, 0, 1));

   ir_instruction *inst = new ir_return(result);
   instructions->push_tail(inst);
}

static void
generate_any_bvec3(exec_list *instructions,
		   ir_variable **declarations,
		   const glsl_type *type)
{
   ir_dereference *const arg0 = new ir_dereference(declarations[0]);
   ir_rvalue *result;

   (void)type;

   result = new ir_expression(ir_binop_logic_or, glsl_type::bool_type,
			      new ir_swizzle(arg0, 0, 0, 0, 0, 1),
			      new ir_swizzle(arg0, 1, 0, 0, 0, 1));
   result = new ir_expression(ir_binop_logic_or, glsl_type::bool_type,
			      result,
			      new ir_swizzle(arg0, 2, 0, 0, 0, 1));

   ir_instruction *inst = new ir_return(result);
   instructions->push_tail(inst);
}

static void
generate_any_bvec4(exec_list *instructions,
		   ir_variable **declarations,
		   const glsl_type *type)
{
   ir_dereference *const arg0 = new ir_dereference(declarations[0]);
   ir_rvalue *result;

   (void)type;

   result = new ir_expression(ir_binop_logic_or, glsl_type::bool_type,
			      new ir_swizzle(arg0, 0, 0, 0, 0, 1),
			      new ir_swizzle(arg0, 1, 0, 0, 0, 1));
   result = new ir_expression(ir_binop_logic_or, glsl_type::bool_type,
			      result,
			      new ir_swizzle(arg0, 2, 0, 0, 0, 1));
   result = new ir_expression(ir_binop_logic_or, glsl_type::bool_type,
			      result,
			      new ir_swizzle(arg0, 3, 0, 0, 0, 1));

   ir_instruction *inst = new ir_return(result);
   instructions->push_tail(inst);
}

static void
generate_all_bvec2(exec_list *instructions,
		   ir_variable **declarations,
		   const glsl_type *type)
{
   ir_dereference *const arg0 = new ir_dereference(declarations[0]);
   ir_rvalue *result;

   (void)type;

   result = new ir_expression(ir_binop_logic_and, glsl_type::bool_type,
			      new ir_swizzle(arg0, 0, 0, 0, 0, 1),
			      new ir_swizzle(arg0, 1, 0, 0, 0, 1));

   ir_instruction *inst = new ir_return(result);
   instructions->push_tail(inst);
}

static void
generate_all_bvec3(exec_list *instructions,
		   ir_variable **declarations,
		   const glsl_type *type)
{
   ir_dereference *const arg0 = new ir_dereference(declarations[0]);
   ir_rvalue *result;

   (void)type;

   result = new ir_expression(ir_binop_logic_and, glsl_type::bool_type,
			      new ir_swizzle(arg0, 0, 0, 0, 0, 1),
			      new ir_swizzle(arg0, 1, 0, 0, 0, 1));
   result = new ir_expression(ir_binop_logic_and, glsl_type::bool_type,
			      result,
			      new ir_swizzle(arg0, 2, 0, 0, 0, 1));

   ir_instruction *inst = new ir_return(result);
   instructions->push_tail(inst);
}

static void
generate_all_bvec4(exec_list *instructions,
		   ir_variable **declarations,
		   const glsl_type *type)
{
   ir_dereference *const arg0 = new ir_dereference(declarations[0]);
   ir_rvalue *result;

   (void)type;

   result = new ir_expression(ir_binop_logic_and, glsl_type::bool_type,
			      new ir_swizzle(arg0, 0, 0, 0, 0, 1),
			      new ir_swizzle(arg0, 1, 0, 0, 0, 1));
   result = new ir_expression(ir_binop_logic_and, glsl_type::bool_type,
			      result,
			      new ir_swizzle(arg0, 2, 0, 0, 0, 1));
   result = new ir_expression(ir_binop_logic_and, glsl_type::bool_type,
			      result,
			      new ir_swizzle(arg0, 3, 0, 0, 0, 1));

   ir_instruction *inst = new ir_return(result);
   instructions->push_tail(inst);
}

static void
generate_not(exec_list *instructions,
	     ir_variable **declarations,
	     const glsl_type *type)
{
   ir_dereference *const arg0 = new ir_dereference(declarations[0]);
   ir_rvalue *result;

   result = new ir_expression(ir_unop_logic_not, type, arg0, NULL);

   ir_instruction *inst = new ir_return(result);
   instructions->push_tail(inst);
}

void
generate_any_functions(glsl_symbol_table *symtab, exec_list *instructions)
{
   const char *name = "any";
   ir_function *const f = new ir_function(name);
   const glsl_type *bvec2_type = glsl_type::get_instance(GLSL_TYPE_BOOL, 2, 1);
   const glsl_type *bvec3_type = glsl_type::get_instance(GLSL_TYPE_BOOL, 3, 1);
   const glsl_type *bvec4_type = glsl_type::get_instance(GLSL_TYPE_BOOL, 4, 1);

   bool added = symtab->add_function(name, f);
   assert(added);

   generate_function_instance(f, name, instructions, 1, generate_any_bvec2,
			      glsl_type::bool_type, bvec2_type);
   generate_function_instance(f, name, instructions, 1, generate_any_bvec3,
			      glsl_type::bool_type, bvec3_type);
   generate_function_instance(f, name, instructions, 1, generate_any_bvec4,
			      glsl_type::bool_type, bvec4_type);
}

void
generate_all_functions(glsl_symbol_table *symtab, exec_list *instructions)
{
   const char *name = "all";
   ir_function *const f = new ir_function(name);
   const glsl_type *bvec2_type = glsl_type::get_instance(GLSL_TYPE_BOOL, 2, 1);
   const glsl_type *bvec3_type = glsl_type::get_instance(GLSL_TYPE_BOOL, 3, 1);
   const glsl_type *bvec4_type = glsl_type::get_instance(GLSL_TYPE_BOOL, 4, 1);

   bool added = symtab->add_function(name, f);
   assert(added);

   generate_function_instance(f, name, instructions, 1, generate_all_bvec2,
			      glsl_type::bool_type, bvec2_type);
   generate_function_instance(f, name, instructions, 1, generate_all_bvec3,
			      glsl_type::bool_type, bvec3_type);
   generate_function_instance(f, name, instructions, 1, generate_all_bvec4,
			      glsl_type::bool_type, bvec4_type);
}

void
generate_not_functions(glsl_symbol_table *symtab, exec_list *instructions)
{
   const char *name = "not";
   ir_function *const f = new ir_function(name);
   const glsl_type *bvec2_type = glsl_type::get_instance(GLSL_TYPE_BOOL, 2, 1);
   const glsl_type *bvec3_type = glsl_type::get_instance(GLSL_TYPE_BOOL, 3, 1);
   const glsl_type *bvec4_type = glsl_type::get_instance(GLSL_TYPE_BOOL, 4, 1);

   bool added = symtab->add_function(name, f);
   assert(added);

   generate_function_instance(f, name, instructions, 1, generate_not,
			      bvec2_type, bvec2_type);
   generate_function_instance(f, name, instructions, 1, generate_not,
			      bvec3_type, bvec3_type);
   generate_function_instance(f, name, instructions, 1, generate_not,
			      bvec4_type, bvec4_type);
}

void
generate_110_functions(glsl_symbol_table *symtab, exec_list *instructions)
{
   make_gentype_function(symtab, instructions, "radians", 1, generate_radians);
   make_gentype_function(symtab, instructions, "degrees", 1, generate_degrees);
   /* FINISHME: sin() */
   /* FINISHME: cos() */
   /* FINISHME: tan() */
   /* FINISHME: asin() */
   /* FINISHME: acos() */
   /* FINISHME: atan(y,x) */
   /* FINISHME: atan(y/x) */
   make_gentype_function(symtab, instructions, "pow", 2, generate_pow);
   make_gentype_function(symtab, instructions, "exp", 1, generate_exp);
   make_gentype_function(symtab, instructions, "log", 1, generate_log);
   make_gentype_function(symtab, instructions, "exp2", 1, generate_exp2);
   make_gentype_function(symtab, instructions, "log2", 1, generate_log2);
   make_gentype_function(symtab, instructions, "sqrt", 1, generate_sqrt);
   make_gentype_function(symtab, instructions, "inversesqrt", 1, generate_rsq);
   make_gentype_function(symtab, instructions, "abs", 1, generate_abs);
   /* FINISHME: sign() */
   make_gentype_function(symtab, instructions, "floor", 1, generate_floor);
   make_gentype_function(symtab, instructions, "ceil", 1, generate_ceil);
   /* FINISHME: fract() */
   /* FINISHME: mod(x, float y) */
   make_gentype_function(symtab, instructions, "mod", 2, generate_mod);
   make_gentype_function(symtab, instructions, "min", 2, generate_min);
   /* FINISHME: min(x, float y) */
   make_gentype_function(symtab, instructions, "max", 2, generate_max);
   /* FINISHME: max(x, float y) */
   /* FINISHME: clamp() */
   /* FINISHME: clamp() */
   make_gentype_function(symtab, instructions, "mix", 3, generate_mix_vec);
   /* FINISHME: mix() */
   /* FINISHME: step() */
   /* FINISHME: step() */
   /* FINISHME: smoothstep() */
   /* FINISHME: smoothstep() */
   /* FINISHME: floor() */
   /* FINISHME: step() */
   generate_length_functions(symtab, instructions);
   /* FINISHME: distance() */
   generate_dot_functions(symtab, instructions);
   /* FINISHME: cross() */
   make_gentype_function(symtab, instructions, "normalize", 1,
			 generate_normalize);
   /* FINISHME: normalize() */
   /* FINISHME: ftransform() */
   /* FINISHME: faceforward() */
   /* FINISHME: reflect() */
   /* FINISHME: refract() */
   /* FINISHME: matrixCompMult() */
   generate_vec_compare_function(symtab, instructions,
				 "lessThan", generate_lessThan, false);
   generate_vec_compare_function(symtab, instructions,
				 "lessThanEqual", generate_lessThanEqual,
				 false);
   generate_vec_compare_function(symtab, instructions,
				 "greaterThan", generate_greaterThan, false);
   generate_vec_compare_function(symtab, instructions,
				 "greaterThanEqual", generate_greaterThanEqual,
				 false);
   generate_vec_compare_function(symtab, instructions,
				 "equal", generate_equal, false);
   generate_vec_compare_function(symtab, instructions,
				 "notEqual", generate_notEqual, false);
   generate_any_functions(symtab, instructions);
   generate_all_functions(symtab, instructions);
   generate_not_functions(symtab, instructions);
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
