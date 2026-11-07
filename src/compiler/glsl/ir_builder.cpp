/*
 * Copyright © 2012 Intel Corporation
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "ir_builder.h"
#include "program/prog_instruction.h"

using namespace ir_builder;

namespace ir_builder {

void
ir_factory::emit(ir_instruction *ir)
{
   instructions->push_tail(ir);
}

ir_variable *
ir_factory::make_temp(const glsl_type *type, const char *name)
{
   ir_variable *var;

   var = new(linalloc) ir_variable(type, name, ir_var_temporary);
   emit(var);

   return var;
}

ir_assignment *
assign(deref lhs, operand rhs)
{
   return assign(lhs, rhs, (1 << lhs.val->type->vector_elements) - 1);
}

ir_assignment *
assign(deref lhs, operand rhs, int writemask)
{
   linear_ctx *linalloc = lhs.val->node_linalloc;

   ir_assignment *assign = new(linalloc) ir_assignment(lhs.val,
                                                      rhs.val,
                                                      writemask);

   return assign;
}

ir_return *
ret(operand retval)
{
   linear_ctx *linalloc = retval.val->node_linalloc;
   return new(linalloc) ir_return(retval.val);
}

ir_swizzle *
swizzle(operand a, int swizzle, int components)
{
   linear_ctx *linalloc = a.val->node_linalloc;

   return new(linalloc) ir_swizzle(a.val,
                                  GET_SWZ(swizzle, 0),
                                  GET_SWZ(swizzle, 1),
                                  GET_SWZ(swizzle, 2),
                                  GET_SWZ(swizzle, 3),
                                  components);
}

ir_swizzle *
swizzle_for_size(operand a, unsigned components)
{
   linear_ctx *linalloc = a.val->node_linalloc;

   if (a.val->type->vector_elements < components)
      components = a.val->type->vector_elements;

   unsigned s[4] = { 0, 1, 2, 3 };
   for (int i = components; i < 4; i++)
      s[i] = components - 1;

   return new(linalloc) ir_swizzle(a.val, s, components);
}

ir_swizzle *
swizzle_x(operand a)
{
   return swizzle(a, SWIZZLE_XXXX, 1);
}

ir_swizzle *
swizzle_y(operand a)
{
   return swizzle(a, SWIZZLE_YYYY, 1);
}

ir_swizzle *
swizzle_z(operand a)
{
   return swizzle(a, SWIZZLE_ZZZZ, 1);
}

ir_swizzle *
swizzle_w(operand a)
{
   return swizzle(a, SWIZZLE_WWWW, 1);
}

ir_expression *
expr(ir_expression_operation op, operand a)
{
   linear_ctx *linalloc = a.val->node_linalloc;

   return new(linalloc) ir_expression(op, a.val);
}

ir_expression *
expr(ir_expression_operation op, operand a, operand b)
{
   linear_ctx *linalloc = a.val->node_linalloc;

   return new(linalloc) ir_expression(op, a.val, b.val);
}

ir_expression *
expr(ir_expression_operation op, operand a, operand b, operand c)
{
   linear_ctx *linalloc = a.val->node_linalloc;

   return new(linalloc) ir_expression(op, a.val, b.val, c.val);
}

ir_expression *add(operand a, operand b)
{
   return expr(ir_binop_add, a, b);
}

ir_expression *sub(operand a, operand b)
{
   return expr(ir_binop_sub, a, b);
}

ir_expression *min2(operand a, operand b)
{
   return expr(ir_binop_min, a, b);
}

ir_expression *max2(operand a, operand b)
{
   return expr(ir_binop_max, a, b);
}

ir_expression *mul(operand a, operand b)
{
   return expr(ir_binop_mul, a, b);
}

ir_expression *div(operand a, operand b)
{
   return expr(ir_binop_div, a, b);
}

ir_expression *carry(operand a, operand b)
{
   return expr(ir_binop_carry, a, b);
}

ir_expression *borrow(operand a, operand b)
{
   return expr(ir_binop_borrow, a, b);
}

ir_expression *trunc(operand a)
{
   return expr(ir_unop_trunc, a);
}

ir_expression *round_even(operand a)
{
   return expr(ir_unop_round_even, a);
}

/* dot for vectors, mul for scalars */
ir_expression *dot(operand a, operand b)
{
   assert(a.val->type == b.val->type);

   if (a.val->type->vector_elements == 1)
      return expr(ir_binop_mul, a, b);

   return expr(ir_binop_dot, a, b);
}

ir_expression*
clamp(operand a, operand b, operand c)
{
   return expr(ir_binop_min, expr(ir_binop_max, a, b), c);
}

ir_expression *
saturate(operand a)
{
   return expr(ir_unop_saturate, a);
}

ir_expression *
abs(operand a)
{
   return expr(ir_unop_abs, a);
}

ir_expression *
neg(operand a)
{
   return expr(ir_unop_neg, a);
}

ir_expression *
sin(operand a)
{
   return expr(ir_unop_sin, a);
}

ir_expression *
cos(operand a)
{
   return expr(ir_unop_cos, a);
}

ir_expression *
exp(operand a)
{
   return expr(ir_unop_exp, a);
}

ir_expression *
rsq(operand a)
{
   return expr(ir_unop_rsq, a);
}

ir_expression *
sqrt(operand a)
{
   return expr(ir_unop_sqrt, a);
}

ir_expression *
log(operand a)
{
   return expr(ir_unop_log, a);
}

ir_expression *
sign(operand a)
{
   return expr(ir_unop_sign, a);
}

ir_expression *
subr_to_int(operand a)
{
   return expr(ir_unop_subroutine_to_int, a);
}

ir_expression*
equal(operand a, operand b)
{
   return expr(ir_binop_equal, a, b);
}

ir_expression*
nequal(operand a, operand b)
{
   return expr(ir_binop_nequal, a, b);
}

ir_expression*
less(operand a, operand b)
{
   return expr(ir_binop_less, a, b);
}

ir_expression*
greater(operand a, operand b)
{
   return expr(ir_binop_less, b, a);
}

ir_expression*
gequal(operand a, operand b)
{
   return expr(ir_binop_gequal, a, b);
}

ir_expression*
logic_not(operand a)
{
   return expr(ir_unop_logic_not, a);
}

ir_expression*
logic_and(operand a, operand b)
{
   return expr(ir_binop_logic_and, a, b);
}

ir_expression*
logic_or(operand a, operand b)
{
   return expr(ir_binop_logic_or, a, b);
}

ir_expression*
bit_not(operand a)
{
   return expr(ir_unop_bit_not, a);
}

ir_expression*
bit_and(operand a, operand b)
{
   return expr(ir_binop_bit_and, a, b);
}

ir_expression*
bit_or(operand a, operand b)
{
   return expr(ir_binop_bit_or, a, b);
}

ir_expression*
lshift(operand a, operand b)
{
   return expr(ir_binop_lshift, a, b);
}

ir_expression*
rshift(operand a, operand b)
{
   return expr(ir_binop_rshift, a, b);
}

ir_expression*
f2i(operand a)
{
   return expr(ir_unop_f2i, a);
}

ir_expression*
bitcast_f2i(operand a)
{
   return expr(ir_unop_bitcast_f2i, a);
}

ir_expression*
i2f(operand a)
{
   return expr(ir_unop_i2f, a);
}

ir_expression*
bitcast_i2f(operand a)
{
   return expr(ir_unop_bitcast_i2f, a);
}

ir_expression*
i2u(operand a)
{
   return expr(ir_unop_i2u, a);
}

ir_expression*
u2i(operand a)
{
   return expr(ir_unop_u2i, a);
}

ir_expression*
f2u(operand a)
{
   return expr(ir_unop_f2u, a);
}

ir_expression*
bitcast_f2u(operand a)
{
   return expr(ir_unop_bitcast_f2u, a);
}

ir_expression*
u2f(operand a)
{
   return expr(ir_unop_u2f, a);
}

ir_expression*
bitcast_u2f(operand a)
{
   return expr(ir_unop_bitcast_u2f, a);
}

ir_expression*
b2i(operand a)
{
   return expr(ir_unop_b2i, a);
}

ir_expression *
b2f(operand a)
{
   return expr(ir_unop_b2f, a);
}

ir_expression*
bitcast_d2i64(operand a)
{
   return expr(ir_unop_bitcast_d2i64, a);
}

ir_expression*
bitcast_d2u64(operand a)
{
   return expr(ir_unop_bitcast_d2u64, a);
}

ir_expression*
bitcast_i642d(operand a)
{
   return expr(ir_unop_bitcast_i642d, a);
}

ir_expression*
bitcast_u642d(operand a)
{
   return expr(ir_unop_bitcast_u642d, a);
}

ir_expression *
interpolate_at_centroid(operand a)
{
   return expr(ir_unop_interpolate_at_centroid, a);
}

ir_expression *
interpolate_at_offset(operand a, operand b)
{
   return expr(ir_binop_interpolate_at_offset, a, b);
}

ir_expression *
interpolate_at_sample(operand a, operand b)
{
   return expr(ir_binop_interpolate_at_sample, a, b);
}

ir_expression *
f2f16(operand a)
{
   return expr(ir_unop_f2f16, a);
}

ir_expression *
f2d(operand a)
{
   return expr(ir_unop_f2d, a);
}

ir_expression *
fma(operand a, operand b, operand c)
{
   return expr(ir_triop_fma, a, b, c);
}

ir_expression *
lrp(operand x, operand y, operand a)
{
   return expr(ir_triop_lrp, x, y, a);
}

ir_expression *
csel(operand a, operand b, operand c)
{
   return expr(ir_triop_csel, a, b, c);
}

ir_expression *
bitfield_extract(operand a, operand b, operand c)
{
   return expr(ir_triop_bitfield_extract, a, b, c);
}

ir_expression *
bitfield_insert(operand a, operand b, operand c, operand d)
{
   linear_ctx *linalloc = a.val->node_linalloc;
   return new(linalloc) ir_expression(ir_quadop_bitfield_insert,
                                     a.val->type, a.val, b.val, c.val, d.val);
}

ir_if*
if_tree(operand condition,
        ir_instruction *then_branch)
{
   assert(then_branch != NULL);

   linear_ctx *linalloc = condition.val->node_linalloc;

   ir_if *result = new(linalloc) ir_if(condition.val);
   result->then_instructions.push_tail(then_branch);
   return result;
}

ir_if*
if_tree(operand condition,
        ir_instruction *then_branch,
        ir_instruction *else_branch)
{
   assert(then_branch != NULL);
   assert(else_branch != NULL);

   linear_ctx *linalloc = condition.val->node_linalloc;

   ir_if *result = new(linalloc) ir_if(condition.val);
   result->then_instructions.push_tail(then_branch);
   result->else_instructions.push_tail(else_branch);
   return result;
}

} /* namespace ir_builder */
