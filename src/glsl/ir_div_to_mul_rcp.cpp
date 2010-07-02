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
 * \file ir_div_to_mul_rcp.cpp
 *
 * Breaks an ir_unop_div expression down to op0 * (rcp(op1)).
 *
 * Many GPUs don't have a divide instruction (945 and 965 included),
 * but they do have an RCP instruction to compute an approximate
 * reciprocal.  By breaking the operation down, constant reciprocals
 * can get constant folded.
 */

#include "ir.h"

class ir_div_to_mul_rcp_visitor : public ir_hierarchical_visitor {
public:
   ir_div_to_mul_rcp_visitor()
   {
      this->made_progress = false;
   }

   ir_visitor_status visit_leave(ir_expression *);

   bool made_progress;
};

bool
do_div_to_mul_rcp(exec_list *instructions)
{
   ir_div_to_mul_rcp_visitor v;

   visit_list_elements(&v, instructions);
   return v.made_progress;
}

ir_visitor_status
ir_div_to_mul_rcp_visitor::visit_leave(ir_expression *ir)
{
   if (ir->operation != ir_binop_div)
      return visit_continue;

   /* New expression for the 1.0 / op1 */
   ir_rvalue *expr;
   expr = new(ir) ir_expression(ir_unop_rcp,
				ir->operands[1]->type,
				ir->operands[1],
				NULL);

   /* op0 / op1 -> op0 * (1.0 / op1) */
   ir->operation = ir_binop_mul;
   ir->operands[1] = expr;
   this->made_progress = true;

   return visit_continue;
}
