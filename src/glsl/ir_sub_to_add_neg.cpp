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
 * \file ir_sub_to_add_neg.cpp
 *
 * Breaks an ir_binop_sub expression down to add(op0, neg(op1))
 *
 * This simplifies expression reassociation, and for many backends
 * there is no subtract operation separate from adding the negation.
 * For backends with native subtract operations, they will probably
 * want to recognize add(op0, neg(op1)) or the other way around to
 * produce a subtract anyway.
 */

#include "ir.h"

class ir_sub_to_add_neg_visitor : public ir_hierarchical_visitor {
public:
   ir_sub_to_add_neg_visitor()
   {
      this->progress = false;
   }

   ir_visitor_status visit_leave(ir_expression *);

   bool progress;
};

bool
do_sub_to_add_neg(exec_list *instructions)
{
   ir_sub_to_add_neg_visitor v;

   visit_list_elements(&v, instructions);
   return v.progress;
}

ir_visitor_status
ir_sub_to_add_neg_visitor::visit_leave(ir_expression *ir)
{
   if (ir->operation != ir_binop_sub)
      return visit_continue;

   void *mem_ctx = talloc_parent(ir);

   ir->operation = ir_binop_add;
   ir->operands[1] = new(mem_ctx) ir_expression(ir_unop_neg,
						ir->operands[1]->type,
						ir->operands[1],
						NULL);

   this->progress = true;

   return visit_continue;
}
