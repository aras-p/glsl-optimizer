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

#include "glsl_types.h"
#include "ir.h"

int
type_compare(const glsl_type *a, const glsl_type *b)
{
   /* If the types are the same, they trivially match.
    */
   if (a == b)
      return 0;

   switch (a->base_type) {
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_BOOL:
      /* There is no implicit conversion to or from integer types or bool.
       */
      if ((a->is_integer() != b->is_integer())
	  || (a->is_boolean() != b->is_boolean()))
	 return -1;

      /* FALLTHROUGH */

   case GLSL_TYPE_FLOAT:
      if ((a->vector_elements != b->vector_elements)
	  || (a->matrix_columns != b->matrix_columns))
	 return -1;

      return 1;

   case GLSL_TYPE_SAMPLER:
   case GLSL_TYPE_STRUCT:
      /* Samplers and structures must match exactly.
       */
      return -1;

   case GLSL_TYPE_ARRAY:
      if ((b->base_type != GLSL_TYPE_ARRAY)
	  || (a->length != b->length))
	 return -1;

      /* From GLSL 1.50 spec, page 27 (page 33 of the PDF):
       *    "There are no implicit array or structure conversions."
       *
       * If the comparison of the array element types detects that a conversion
       * would be required, the array types do not match.
       */
      return (type_compare(a->fields.array, b->fields.array) == 0) ? 0 : -1;

   case GLSL_TYPE_VOID:
   case GLSL_TYPE_ERROR:
   default:
      /* These are all error conditions.  It is invalid for a parameter to
       * a function to be declared as error, void, or a function.
       */
      return -1;
   }

   /* This point should be unreachable.
    */
   assert(0);
}


static int
parameter_lists_match(const exec_list *list_a, const exec_list *list_b)
{
   const exec_node *node_a = list_a->head;
   const exec_node *node_b = list_b->head;
   int total_score = 0;

   for (/* empty */
	; !node_a->is_tail_sentinel()
	; node_a = node_a->next, node_b = node_b->next) {
      /* If all of the parameters from the other parameter list have been
       * exhausted, the lists have different length and, by definition,
       * do not match.
       */
      if (node_b->is_tail_sentinel())
	 return -1;


      const ir_variable *const param = (ir_variable *) node_a;
      const ir_instruction *const actual = (ir_instruction *) node_b;

      /* Determine whether or not the types match.  If the types are an
       * exact match, the match score is zero.  If the types don't match
       * but the actual parameter can be coerced to the type of the declared
       * parameter, the match score is one.
       */
      int score;
      switch ((enum ir_variable_mode)(param->mode)) {
      case ir_var_auto:
      case ir_var_uniform:
      case ir_var_temporary:
	 /* These are all error conditions.  It is invalid for a parameter to
	  * a function to be declared as auto (not in, out, or inout) or
	  * as uniform.
	  */
	 assert(0);
	 return -1;

      case ir_var_in:
	 score = type_compare(param->type, actual->type);
	 break;

      case ir_var_out:
	 score = type_compare(actual->type, param->type);
	 break;

      case ir_var_inout:
	 /* Since there are no bi-directional automatic conversions (e.g.,
	  * there is int -> float but no float -> int), inout parameters must
	  * be exact matches.
	  */
	 score = (type_compare(actual->type, param->type) == 0) ? 0 : -1;
	 break;

      default:
	 assert(false);
      }

      if (score < 0)
	 return -1;

      total_score += score;
   }

   /* If all of the parameters from the other parameter list have been
    * exhausted, the lists have different length and, by definition, do not
    * match.
    */
   if (!node_b->is_tail_sentinel())
      return -1;

   return total_score;
}


ir_function_signature *
ir_function::matching_signature(const exec_list *actual_parameters)
{
   ir_function_signature *match = NULL;

   foreach_iter(exec_list_iterator, iter, signatures) {
      ir_function_signature *const sig =
	 (ir_function_signature *) iter.get();

      const int score = parameter_lists_match(& sig->parameters,
					      actual_parameters);

      if (score == 0)
	 return sig;

      if (score > 0) {
	 if (match != NULL)
	    return NULL;

	 match = sig;
      }
   }

   return match;
}


static bool
parameter_lists_match_exact(const exec_list *list_a, const exec_list *list_b)
{
   const exec_node *node_a = list_a->head;
   const exec_node *node_b = list_b->head;

   for (/* empty */
	; !node_a->is_tail_sentinel() && !node_b->is_tail_sentinel()
	; node_a = node_a->next, node_b = node_b->next) {
      ir_variable *a = (ir_variable *) node_a;
      ir_variable *b = (ir_variable *) node_b;

      /* If the types of the parameters do not match, the parameters lists
       * are different.
       */
      if (a->type != b->type)
         return false;
   }

   /* Unless both lists are exhausted, they differ in length and, by
    * definition, do not match.
    */
   return (node_a->is_tail_sentinel() == node_b->is_tail_sentinel());
}

ir_function_signature *
ir_function::exact_matching_signature(const exec_list *actual_parameters)
{
   foreach_iter(exec_list_iterator, iter, signatures) {
      ir_function_signature *const sig =
	 (ir_function_signature *) iter.get();

      if (parameter_lists_match_exact(&sig->parameters, actual_parameters))
	 return sig;
   }
   return NULL;
}
