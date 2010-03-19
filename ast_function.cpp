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

static ir_instruction *
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
   simple_node *const first = parameters;
   if (first != NULL) {
      simple_node *ptr = first;
      do {
	 ir_instruction *const result =
	    ((ast_node *) ptr)->hir(instructions, state);
	 ptr = ptr->next;

	 actual_parameters.push_tail(result);
      } while (ptr != first);
   }

   /* After processing the function's actual parameters, try to find an
    * overload of the function that matches.
    */
   const ir_function_signature *sig =
      f->matching_signature(& actual_parameters);
   if (sig != NULL) {
      /* FINISHME: The list of actual parameters needs to be modified to
       * FINISHME: include any necessary conversions.
       */
      return new ir_call(sig, & actual_parameters);
   } else {
      /* FINISHME: Log a better error message here.  G++ will show the types
       * FINISHME: of the actual parameters and the set of candidate
       * FINISHME: functions.  A different error should also be logged when
       * FINISHME: multiple functions match.
       */
      _mesa_glsl_error(loc, state, "no matching function for call to `%s'",
		       name);
      return ir_call::get_error_instruction();
   }
}


ir_instruction *
ast_function_expression::hir(exec_list *instructions,
			     struct _mesa_glsl_parse_state *state)
{
   /* There are three sorts of function calls.
    *
    * 1. contstructors - The first subexpression is an ast_type_specifier.
    * 2. methods - Only the .length() method of array types.
    * 3. functions - Calls to regular old functions.
    *
    * There are two kinds of constructor call.  Constructors for built-in
    * language types, such as mat4 and vec2, are free form.  The only
    * requirement is that the parameters must provide enough values of the
    * correct scalar type.  Constructors for arrays and structures must have
    * the exact number of parameters with matching types in the correct order.
    * These constructors follow essentially the same type matching rules as
    * functions.
    *
    * Method calls are actually detected when the ast_field_selection
    * expression is handled.
    */
   if (is_constructor()) {
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
