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

#include "ir.h"

namespace ir_builder {

/**
 * This little class exists to let the helper expression generators
 * take either an ir_rvalue * or an ir_variable * to be automatically
 * dereferenced, while still providing compile-time type checking.
 *
 * You don't have to explicitly call the constructor -- C++ will see
 * that you passed an ir_variable, and silently call the
 * operand(ir_variable *var) constructor behind your back.
 */
class operand {
public:
   operand(ir_rvalue *val)
      : val(val)
   {
   }

   operand(ir_variable *var)
   {
      void *mem_ctx = ralloc_parent(var);
      val = new(mem_ctx) ir_dereference_variable(var);
   }

   ir_rvalue *val;
};

ir_expression *expr(ir_expression_operation op, operand a, operand b);
ir_expression *add(operand a, operand b);
ir_expression *sub(operand a, operand b);
ir_expression *mul(operand a, operand b);
ir_expression *dot(operand a, operand b);
ir_expression *saturate(operand a);

} /* namespace ir_builder */
