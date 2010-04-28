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
#include <string.h>
#include "main/imports.h"
#include "main/simple_list.h"
#include "ir.h"
#include "ir_visitor.h"
#include "glsl_types.h"

ir_assignment::ir_assignment(ir_rvalue *lhs, ir_rvalue *rhs,
			     ir_rvalue *condition)
{
   this->lhs = lhs;
   this->rhs = rhs;
   this->condition = condition;
}


ir_expression::ir_expression(int op, const struct glsl_type *type,
			     ir_rvalue *op0, ir_rvalue *op1)
{
   this->type = type;
   this->operation = ir_expression_operation(op);
   this->operands[0] = op0;
   this->operands[1] = op1;
}

unsigned int
ir_expression::get_num_operands(ir_expression_operation op)
{
/* Update ir_print_visitor.cpp when updating this list. */
   const int num_operands[] = {
      1, /* ir_unop_bit_not */
      1, /* ir_unop_logic_not */
      1, /* ir_unop_neg */
      1, /* ir_unop_abs */
      1, /* ir_unop_rcp */
      1, /* ir_unop_rsq */
      1, /* ir_unop_sqrt */
      1, /* ir_unop_exp */
      1, /* ir_unop_log */
      1, /* ir_unop_exp2 */
      1, /* ir_unop_log2 */
      1, /* ir_unop_f2i */
      1, /* ir_unop_i2f */
      1, /* ir_unop_f2b */
      1, /* ir_unop_b2f */
      1, /* ir_unop_i2b */
      1, /* ir_unop_b2i */
      1, /* ir_unop_u2f */

      1, /* ir_unop_trunc */
      1, /* ir_unop_ceil */
      1, /* ir_unop_floor */

      2, /* ir_binop_add */
      2, /* ir_binop_sub */
      2, /* ir_binop_mul */
      2, /* ir_binop_div */
      2, /* ir_binop_mod */

      2, /* ir_binop_less */
      2, /* ir_binop_greater */
      2, /* ir_binop_lequal */
      2, /* ir_binop_gequal */
      2, /* ir_binop_equal */
      2, /* ir_binop_nequal */

      2, /* ir_binop_lshift */
      2, /* ir_binop_rshift */
      2, /* ir_binop_bit_and */
      2, /* ir_binop_bit_xor */
      2, /* ir_binop_bit_or */

      2, /* ir_binop_logic_and */
      2, /* ir_binop_logic_xor */
      2, /* ir_binop_logic_or */

      2, /* ir_binop_dot */
      2, /* ir_binop_min */
      2, /* ir_binop_max */

      2, /* ir_binop_pow */
   };

   assert(sizeof(num_operands) / sizeof(num_operands[0]) == ir_binop_pow + 1);

   return num_operands[op];
}


ir_constant::ir_constant(const struct glsl_type *type, const void *data)
{
   unsigned size = 0;

   this->type = type;
   switch (type->base_type) {
   case GLSL_TYPE_UINT:  size = sizeof(this->value.u[0]); break;
   case GLSL_TYPE_INT:   size = sizeof(this->value.i[0]); break;
   case GLSL_TYPE_FLOAT: size = sizeof(this->value.f[0]); break;
   case GLSL_TYPE_BOOL:  size = sizeof(this->value.b[0]); break;
   default:
      /* FINISHME: What to do?  Exceptions are not the answer.
       */
      break;
   }

   memcpy(& this->value, data, size * type->components());
}

ir_constant::ir_constant(float f)
{
   this->type = glsl_type::float_type;
   this->value.f[0] = f;
}

ir_constant::ir_constant(unsigned int u)
{
   this->type = glsl_type::uint_type;
   this->value.u[0] = u;
}

ir_constant::ir_constant(int i)
{
   this->type = glsl_type::int_type;
   this->value.i[0] = i;
}

ir_constant::ir_constant(bool b)
{
   this->type = glsl_type::bool_type;
   this->value.b[0] = b;
}


ir_dereference::ir_dereference(ir_instruction *var)
{
   this->mode = ir_reference_variable;
   this->var = var;
   this->type = (var != NULL) ? var->type : glsl_type::error_type;
}


ir_dereference::ir_dereference(ir_instruction *var,
			       ir_rvalue *array_index)
   : mode(ir_reference_array), var(var)
{
   type = glsl_type::error_type;

   if (var != NULL) {
      const glsl_type *const vt = var->type;

      if (vt->is_array()) {
	 type = vt->element_type();
      } else if (vt->is_matrix()) {
	 type = vt->column_type();
      } else if (vt->is_vector()) {
	 type = vt->get_base_type();
      }
   }

   this->selector.array_index = array_index;
}

bool
ir_dereference::is_lvalue()
{
   if (var == NULL)
      return false;

   if (mode == ir_reference_variable) {
      ir_variable *const as_var = var->as_variable();
      if (as_var == NULL)
	 return false;

      if (as_var->type->is_array() && !as_var->array_lvalue)
	 return false;

      return !as_var->read_only;
   } else if (mode == ir_reference_array) {
      /* FINISHME: Walk up the dereference chain and figure out if
       * FINISHME: the variable is read-only.
       */
   }

   return true;
}

ir_swizzle::ir_swizzle(ir_rvalue *val, unsigned x, unsigned y, unsigned z,
		       unsigned w, unsigned count)
   : val(val)
{
   assert((count >= 1) && (count <= 4));

   const unsigned dup_mask = 0
      | ((count > 1) ? ((1U << y) & ((1U << x)                        )) : 0)
      | ((count > 2) ? ((1U << z) & ((1U << x) | (1U << y)            )) : 0)
      | ((count > 3) ? ((1U << w) & ((1U << x) | (1U << y) | (1U << z))) : 0);

   assert(x <= 3);
   assert(y <= 3);
   assert(z <= 3);
   assert(w <= 3);

   mask.x = x;
   mask.y = y;
   mask.z = z;
   mask.w = w;
   mask.num_components = count;
   mask.has_duplicates = dup_mask != 0;

   /* Based on the number of elements in the swizzle and the base type
    * (i.e., float, int, unsigned, or bool) of the vector being swizzled,
    * generate the type of the resulting value.
    */
   type = glsl_type::get_instance(val->type->base_type, mask.num_components, 1);
}

#define X 1
#define R 5
#define S 9
#define I 13

ir_swizzle *
ir_swizzle::create(ir_rvalue *val, const char *str, unsigned vector_length)
{
   /* For each possible swizzle character, this table encodes the value in
    * \c idx_map that represents the 0th element of the vector.  For invalid
    * swizzle characters (e.g., 'k'), a special value is used that will allow
    * detection of errors.
    */
   static const unsigned char base_idx[26] = {
   /* a  b  c  d  e  f  g  h  i  j  k  l  m */
      R, R, I, I, I, I, R, I, I, I, I, I, I,
   /* n  o  p  q  r  s  t  u  v  w  x  y  z */
      I, I, S, S, R, S, S, I, I, X, X, X, X
   };

   /* Each valid swizzle character has an entry in the previous table.  This
    * table encodes the base index encoded in the previous table plus the actual
    * index of the swizzle character.  When processing swizzles, the first
    * character in the string is indexed in the previous table.  Each character
    * in the string is indexed in this table, and the value found there has the
    * value form the first table subtracted.  The result must be on the range
    * [0,3].
    *
    * For example, the string "wzyx" will get X from the first table.  Each of
    * the charcaters will get X+3, X+2, X+1, and X+0 from this table.  After
    * subtraction, the swizzle values are { 3, 2, 1, 0 }.
    *
    * The string "wzrg" will get X from the first table.  Each of the characters
    * will get X+3, X+2, R+0, and R+1 from this table.  After subtraction, the
    * swizzle values are { 3, 2, 4, 5 }.  Since 4 and 5 are outside the range
    * [0,3], the error is detected.
    */
   static const unsigned char idx_map[26] = {
   /* a    b    c    d    e    f    g    h    i    j    k    l    m */
      R+3, R+2, 0,   0,   0,   0,   R+1, 0,   0,   0,   0,   0,   0,
   /* n    o    p    q    r    s    t    u    v    w    x    y    z */
      0,   0,   S+2, S+3, R+0, S+0, S+1, 0,   0,   X+3, X+0, X+1, X+2
   };

   int swiz_idx[4] = { 0, 0, 0, 0 };
   unsigned i;


   /* Validate the first character in the swizzle string and look up the base
    * index value as described above.
    */
   if ((str[0] < 'a') || (str[0] > 'z'))
      return NULL;

   const unsigned base = base_idx[str[0] - 'a'];


   for (i = 0; (i < 4) && (str[i] != '\0'); i++) {
      /* Validate the next character, and, as described above, convert it to a
       * swizzle index.
       */
      if ((str[i] < 'a') || (str[i] > 'z'))
	 return NULL;

      swiz_idx[i] = idx_map[str[i] - 'a'] - base;
      if ((swiz_idx[i] < 0) || (swiz_idx[i] >= (int) vector_length))
	 return NULL;
   }

   if (str[i] != '\0')
	 return NULL;

   return new ir_swizzle(val, swiz_idx[0], swiz_idx[1], swiz_idx[2],
                         swiz_idx[3], i);
}

#undef X
#undef R
#undef S
#undef I


ir_variable::ir_variable(const struct glsl_type *type, const char *name)
   : max_array_access(0), read_only(false), centroid(false), invariant(false),
     mode(ir_var_auto), interpolation(ir_var_smooth)
{
   this->type = type;
   this->name = name;
   this->constant_value = NULL;

   if (type && type->base_type == GLSL_TYPE_SAMPLER)
      this->read_only = true;
}


ir_function_signature::ir_function_signature(const glsl_type *return_type)
   : return_type(return_type), is_defined(false)
{
   /* empty */
}


const char *
ir_function_signature::qualifiers_match(exec_list *params)
{
   exec_list_iterator iter_a = parameters.iterator();
   exec_list_iterator iter_b = params->iterator();

   /* check that the qualifiers match. */
   while (iter_a.has_next()) {
      ir_variable *a = (ir_variable *)iter_a.get();
      ir_variable *b = (ir_variable *)iter_b.get();

      if (a->read_only != b->read_only ||
	  a->interpolation != b->interpolation ||
	  a->centroid != b->centroid) {

	 /* parameter a's qualifiers don't match */
	 return a->name;
      }

      iter_a.next();
      iter_b.next();
   }
   return NULL;
}


ir_function::ir_function(const char *name)
   : name(name)
{
   /* empty */
}


ir_call *
ir_call::get_error_instruction()
{
   ir_call *call = new ir_call;

   call->type = glsl_type::error_type;
   return call;
}

void
visit_exec_list(exec_list *list, ir_visitor *visitor)
{
   foreach_iter(exec_list_iterator, iter, *list) {
      ((ir_instruction *)iter.get())->accept(visitor);
   }
}

