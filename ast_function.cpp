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

#include <cstdio>
#include "glsl_symbol_table.h"
#include "ast.h"
#include "glsl_types.h"
#include "ir.h"

static unsigned
process_parameters(exec_list *instructions, exec_list *actual_parameters,
		   simple_node *parameters,
		   struct _mesa_glsl_parse_state *state)
{
   simple_node *const first = parameters;
   unsigned count = 0;

   if (first != NULL) {
      simple_node *ptr = first;
      do {
	 ir_instruction *const result =
	    ((ast_node *) ptr)->hir(instructions, state);
	 ptr = ptr->next;

	 actual_parameters->push_tail(result);
	 count++;
      } while (ptr != first);
   }

   return count;
}


static ir_rvalue *
process_call(exec_list *instructions, ir_function *f,
	     YYLTYPE *loc, exec_list *actual_parameters,
	     struct _mesa_glsl_parse_state *state)
{
   const ir_function_signature *sig =
      f->matching_signature(actual_parameters);

   /* The instructions param will be used when the FINISHMEs below are done */
   (void) instructions;

   if (sig != NULL) {
      /* FINISHME: The list of actual parameters needs to be modified to
       * FINISHME: include any necessary conversions.
       */
      return new ir_call(sig, actual_parameters);
   } else {
      /* FINISHME: Log a better error message here.  G++ will show the types
       * FINISHME: of the actual parameters and the set of candidate
       * FINISHME: functions.  A different error should also be logged when
       * FINISHME: multiple functions match.
       */
      _mesa_glsl_error(loc, state, "no matching function for call to `%s'",
		       f->name);
      return ir_call::get_error_instruction();
   }
}


static ir_rvalue *
match_function_by_name(exec_list *instructions, const char *name,
		       YYLTYPE *loc, simple_node *parameters,
		       struct _mesa_glsl_parse_state *state)
{
   ir_function *f = state->symbols->get_function(name);

   if (f == NULL) {
      _mesa_glsl_error(loc, state, "function `%s' undeclared", name);
      return ir_call::get_error_instruction();
   }

   /* Once we've determined that the function being called might exist,
    * process the parameters.
    */
   exec_list actual_parameters;
   process_parameters(instructions, &actual_parameters, parameters, state);

   /* After processing the function's actual parameters, try to find an
    * overload of the function that matches.
    */
   return process_call(instructions, f, loc, &actual_parameters, state);
}


/**
 * Perform automatic type conversion of constructor parameters
 */
static ir_rvalue *
convert_component(ir_rvalue *src, const glsl_type *desired_type)
{
   const unsigned a = desired_type->base_type;
   const unsigned b = src->type->base_type;

   if (src->type->is_error())
      return src;

   assert(a <= GLSL_TYPE_BOOL);
   assert(b <= GLSL_TYPE_BOOL);

   if ((a == b) || (src->type->is_integer() && desired_type->is_integer()))
      return src;

   switch (a) {
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
      if (b == GLSL_TYPE_FLOAT)
	 return new ir_expression(ir_unop_f2i, desired_type, src, NULL);
      else {
	 assert(b == GLSL_TYPE_BOOL);
	 assert(!"FINISHME: Convert bool to int / uint.");
      }
   case GLSL_TYPE_FLOAT:
      switch (b) {
      case GLSL_TYPE_UINT:
	 return new ir_expression(ir_unop_u2f, desired_type, src, NULL);
      case GLSL_TYPE_INT:
	 return new ir_expression(ir_unop_i2f, desired_type, src, NULL);
      case GLSL_TYPE_BOOL:
	 return new ir_expression(ir_unop_b2f, desired_type, src, NULL);
      }
      break;
   case GLSL_TYPE_BOOL: {
      int z = 0;
      ir_constant *const zero = new ir_constant(src->type, &z);

      return new ir_expression(ir_binop_nequal, desired_type, src, zero);
   }
   }

   assert(!"Should not get here.");
   return NULL;
}


/**
 * Dereference a specific component from a scalar, vector, or matrix
 */
static ir_rvalue *
dereference_component(ir_rvalue *src, unsigned component)
{
   assert(component < src->type->components());

   if (src->type->is_scalar()) {
      return src;
   } else if (src->type->is_vector()) {
      return new ir_swizzle(src, component, 0, 0, 0, 1);
   } else {
      assert(src->type->is_matrix());

      /* Dereference a row of the matrix, then call this function again to get
       * a specific element from that row.
       */
      const int c = component / src->type->column_type()->vector_elements;
      const int r = component % src->type->column_type()->vector_elements;
      ir_constant *const col_index = new ir_constant(glsl_type::int_type, &c);
      ir_dereference *const col = new ir_dereference(src, col_index);

      col->type = src->type->column_type();

      return dereference_component(col, r);
   }

   assert(!"Should not get here.");
   return NULL;
}


static ir_rvalue *
process_array_constructor(exec_list *instructions,
			  const glsl_type *constructor_type,
			  YYLTYPE *loc, simple_node *parameters,
			  struct _mesa_glsl_parse_state *state)
{
   /* Array constructors come in two forms: sized and unsized.  Sized array
    * constructors look like 'vec4[2](a, b)', where 'a' and 'b' are vec4
    * variables.  In this case the number of parameters must exactly match the
    * specified size of the array.
    *
    * Unsized array constructors look like 'vec4[](a, b)', where 'a' and 'b'
    * are vec4 variables.  In this case the size of the array being constructed
    * is determined by the number of parameters.
    *
    * From page 52 (page 58 of the PDF) of the GLSL 1.50 spec:
    *
    *    "There must be exactly the same number of arguments as the size of
    *    the array being constructed. If no size is present in the
    *    constructor, then the array is explicitly sized to the number of
    *    arguments provided. The arguments are assigned in order, starting at
    *    element 0, to the elements of the constructed array. Each argument
    *    must be the same type as the element type of the array, or be a type
    *    that can be converted to the element type of the array according to
    *    Section 4.1.10 "Implicit Conversions.""
    */
   exec_list actual_parameters;
   const unsigned parameter_count =
      process_parameters(instructions, &actual_parameters, parameters, state);

   if ((parameter_count == 0)
       || ((constructor_type->length != 0)
	   && (constructor_type->length != parameter_count))) {
      const unsigned min_param = (constructor_type->length == 0)
	 ? 1 : constructor_type->length;

      _mesa_glsl_error(loc, state, "array constructor must have %s %u "
		       "parameter%s",
		       (constructor_type->length != 0) ? "at least" : "exactly",
		       min_param, (min_param <= 1) ? "" : "s");
      return ir_call::get_error_instruction();
   }

   if (constructor_type->length == 0) {
      constructor_type =
	 glsl_type::get_array_instance(constructor_type->get_base_type(),
				       parameter_count);
      assert(constructor_type != NULL);
      assert(constructor_type->length == parameter_count);
   }

   ir_function *f = state->symbols->get_function(constructor_type->name);

   /* If the constructor for this type of array does not exist, generate the
    * prototype and add it to the symbol table.  The code will be generated
    * later.
    */
   if (f == NULL) {
      f = constructor_type->generate_constructor_prototype(state->symbols);
   }

   ir_rvalue *const r =
      process_call(instructions, f, loc, &actual_parameters, state);

   assert(r != NULL);
   assert(r->type->is_error() || (r->type == constructor_type));

   return r;
}


ir_rvalue *
ast_function_expression::hir(exec_list *instructions,
			     struct _mesa_glsl_parse_state *state)
{
   /* There are three sorts of function calls.
    *
    * 1. contstructors - The first subexpression is an ast_type_specifier.
    * 2. methods - Only the .length() method of array types.
    * 3. functions - Calls to regular old functions.
    *
    * Method calls are actually detected when the ast_field_selection
    * expression is handled.
    */
   if (is_constructor()) {
      const ast_type_specifier *type = (ast_type_specifier *) subexpressions[0];
      YYLTYPE loc = type->get_location();
      const char *name;

      const glsl_type *const constructor_type = type->glsl_type(& name, state);


      /* Constructors for samplers are illegal.
       */
      if (constructor_type->is_sampler()) {
	 _mesa_glsl_error(& loc, state, "cannot construct sampler type `%s'",
			  constructor_type->name);
	 return ir_call::get_error_instruction();
      }

      if (constructor_type->is_array()) {
	 if (state->language_version <= 110) {
	    _mesa_glsl_error(& loc, state,
			     "array constructors forbidden in GLSL 1.10");
	    return ir_call::get_error_instruction();
	 }

	 return process_array_constructor(instructions, constructor_type,
					  & loc, subexpressions[1], state);
      }

      /* There are two kinds of constructor call.  Constructors for built-in
       * language types, such as mat4 and vec2, are free form.  The only
       * requirement is that the parameters must provide enough values of the
       * correct scalar type.  Constructors for arrays and structures must
       * have the exact number of parameters with matching types in the
       * correct order.  These constructors follow essentially the same type
       * matching rules as functions.
       */
      if (constructor_type->is_numeric() || constructor_type->is_boolean()) {
	 /* Constructing a numeric type has a couple steps.  First all values
	  * passed to the constructor are broken into individual parameters
	  * and type converted to the base type of the thing being constructed.
	  *
	  * At that point we have some number of values that match the base
	  * type of the thing being constructed.  Now the constructor can be
	  * treated like a function call.  Each numeric type has a small set
	  * of constructor functions.  The set of new parameters will either
	  * match one of those functions or the original constructor is
	  * invalid.
	  */
	 const glsl_type *const base_type = constructor_type->get_base_type();

	 /* Total number of components of the type being constructed.
	  */
	 const unsigned type_components = constructor_type->components();

	 /* Number of components from parameters that have actually been
	  * consumed.  This is used to perform several kinds of error checking.
	  */
	 unsigned components_used = 0;

	 unsigned matrix_parameters = 0;
	 unsigned nonmatrix_parameters = 0;
	 exec_list actual_parameters;
	 simple_node *const first = subexpressions[1];

	 assert(first != NULL);

	 if (first != NULL) {
	    simple_node *ptr = first;
	    do {
	       ir_rvalue *const result =
		  ((ast_node *) ptr)->hir(instructions, state)->as_rvalue();
	       ptr = ptr->next;

	       /* From page 50 (page 56 of the PDF) of the GLSL 1.50 spec:
		*
		*    "It is an error to provide extra arguments beyond this
		*    last used argument."
		*/
	       if (components_used >= type_components) {
		  _mesa_glsl_error(& loc, state, "too many parameters to `%s' "
				   "constructor",
				   constructor_type->name);
		  return ir_call::get_error_instruction();
	       }

	       if (!result->type->is_numeric() && !result->type->is_boolean()) {
		  _mesa_glsl_error(& loc, state, "cannot construct `%s' from a "
				   "non-numeric data type",
				   constructor_type->name);
		  return ir_call::get_error_instruction();
	       }

	       /* Count the number of matrix and nonmatrix parameters.  This
		* is used below to enforce some of the constructor rules.
		*/
	       if (result->type->is_matrix())
		  matrix_parameters++;
	       else
		  nonmatrix_parameters++;


	       /* Process each of the components of the parameter.  Dereference
		* each component individually, perform any type conversions, and
		* add it to the parameter list for the constructor.
		*/
	       for (unsigned i = 0; i < result->type->components(); i++) {
		  if (components_used >= type_components)
		     break;

		  ir_rvalue *const component =
		     convert_component(dereference_component(result, i),
				       base_type);

		  /* All cases that could result in component->type being the
		   * error type should have already been caught above.
		   */
		  assert(component->type == base_type);

		  /* Don't actually generate constructor calls for scalars.
		   * Instead, do the usual component selection and conversion,
		   * and return the single component.
		   */
		  if (constructor_type->is_scalar())
		     return component;

		  actual_parameters.push_tail(component);
		  components_used++;
	       }
	    } while (ptr != first);
	 }

	 /* From page 28 (page 34 of the PDF) of the GLSL 1.10 spec:
	  *
	  *    "It is an error to construct matrices from other matrices. This
	  *    is reserved for future use."
	  */
	 if ((state->language_version <= 110) && (matrix_parameters > 0)
	     && constructor_type->is_matrix()) {
	    _mesa_glsl_error(& loc, state, "cannot construct `%s' from a "
			     "matrix in GLSL 1.10",
			     constructor_type->name);
	    return ir_call::get_error_instruction();
	 }

	 /* From page 50 (page 56 of the PDF) of the GLSL 1.50 spec:
	  *
	  *    "If a matrix argument is given to a matrix constructor, it is
	  *    an error to have any other arguments."
	  */
	 if ((matrix_parameters > 0)
	     && ((matrix_parameters + nonmatrix_parameters) > 1)
	     && constructor_type->is_matrix()) {
	    _mesa_glsl_error(& loc, state, "for matrix `%s' constructor, "
			     "matrix must be only parameter",
			     constructor_type->name);
	    return ir_call::get_error_instruction();
	 }

	 /* From page 28 (page 34 of the PDF) of the GLSL 1.10 spec:
	  *
	  *    "In these cases, there must be enough components provided in the
	  *    arguments to provide an initializer for every component in the
	  *    constructed value."
	  */
	 if ((components_used < type_components) && (components_used != 1)) {
	    _mesa_glsl_error(& loc, state, "too few components to construct "
			     "`%s'",
			     constructor_type->name);
	    return ir_call::get_error_instruction();
	 }

	 ir_function *f = state->symbols->get_function(constructor_type->name);
	 if (f == NULL) {
	    _mesa_glsl_error(& loc, state, "no constructor for type `%s'",
			     constructor_type->name);
	    return ir_call::get_error_instruction();
	 }

	 const ir_function_signature *sig =
	    f->matching_signature(& actual_parameters);
	 if (sig != NULL) {
	    return new ir_call(sig, & actual_parameters);
	 } else {
	    /* FINISHME: Log a better error message here.  G++ will show the
	     * FINSIHME: types of the actual parameters and the set of
	     * FINSIHME: candidate functions.  A different error should also be
	     * FINSIHME: logged when multiple functions match.
	     */
	    _mesa_glsl_error(& loc, state, "no matching constructor for `%s'",
			     constructor_type->name);
	    return ir_call::get_error_instruction();
	 }
      }

      return ir_call::get_error_instruction();
   } else {
      const ast_expression *id = subexpressions[0];
      YYLTYPE loc = id->get_location();

      return match_function_by_name(instructions, 
				    id->primary_expression.identifier, & loc,
				    subexpressions[1], state);
   }

   return ir_call::get_error_instruction();
}
