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

/**
 * \file ir_explog_to_explog2.cpp
 *
 * Many GPUs don't have a base e log or exponent instruction, but they
 * do have base 2 versions, so this pass converts exp and log to exp2
 * and log2 operations.
 */

#include "main/core.h" /* for log2f on MSVC */
#include "ir.h"
#include "glsl_types.h"

class ir_explog_to_explog2_visitor : public ir_hierarchical_visitor {
public:
   ir_explog_to_explog2_visitor()
   {
      this->progress = false;
   }

   ir_visitor_status visit_leave(ir_expression *);

   bool progress;
};

bool
do_explog_to_explog2(exec_list *instructions)
{
   ir_explog_to_explog2_visitor v;

   visit_list_elements(&v, instructions);
   return v.progress;
}

ir_visitor_status
ir_explog_to_explog2_visitor::visit_leave(ir_expression *ir)
{
   if (ir->operation == ir_unop_exp) {
      void *mem_ctx = talloc_parent(ir);
      ir_constant *log2_e = new(mem_ctx) ir_constant(log2f(M_E));

      ir->operation = ir_unop_exp2;
      ir->operands[0] = new(mem_ctx) ir_expression(ir_binop_mul,
						   ir->operands[0]->type,
						   ir->operands[0],
						   log2_e);
      this->progress = true;
   }

   if (ir->operation == ir_unop_log) {
      void *mem_ctx = talloc_parent(ir);

      ir->operation = ir_binop_mul;
      ir->operands[0] = new(mem_ctx) ir_expression(ir_unop_log2,
						   ir->operands[0]->type,
						   ir->operands[0],
						   NULL);
      ir->operands[1] = new(mem_ctx) ir_constant(1.0f / log2f(M_E));
      this->progress = true;
   }

   return visit_continue;
}
