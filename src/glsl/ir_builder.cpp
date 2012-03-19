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

using namespace ir_builder;

namespace ir_builder {

ir_expression *
expr(ir_expression_operation op,
     ir_rvalue *a, ir_rvalue *b)
{
   void *mem_ctx = ralloc_parent(a);

   return new(mem_ctx) ir_expression(op, a, b);
}

ir_expression *add(ir_rvalue *a, ir_rvalue *b)
{
   return expr(ir_binop_add, a, b);
}

ir_expression *sub(ir_rvalue *a, ir_rvalue *b)
{
   return expr(ir_binop_sub, a, b);
}

ir_expression *mul(ir_rvalue *a, ir_rvalue *b)
{
   return expr(ir_binop_mul, a, b);
}

ir_expression *dot(ir_rvalue *a, ir_rvalue *b)
{
   return expr(ir_binop_dot, a, b);
}

ir_expression *
saturate(ir_rvalue *a)
{
   void *mem_ctx = ralloc_parent(a);

   return expr(ir_binop_max,
	       expr(ir_binop_min, a, new(mem_ctx) ir_constant(1.0f)),
	       new(mem_ctx) ir_constant(0.0f));
}

} /* namespace ir_builder */
