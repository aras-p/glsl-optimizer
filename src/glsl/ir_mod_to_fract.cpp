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
 * \file ir_mod_to_floor.cpp
 *
 * Breaks an ir_unop_mod expression down to (op1 * fract(op0 / op1))
 *
 * Many GPUs don't have a MOD instruction (945 and 965 included), and
 * if we have to break it down like this anyway, it gives an
 * opportunity to do things like constant fold the (1.0 / op1) easily.
 */

#include "ir.h"

class ir_mod_to_fract_visitor : public ir_hierarchical_visitor {
public:
   ir_mod_to_fract_visitor()
   {
      this->made_progress = false;
   }

   ir_visitor_status visit_leave(ir_expression *);

   bool made_progress;
};

bool
do_mod_to_fract(exec_list *instructions)
{
   ir_mod_to_fract_visitor v;

   visit_list_elements(&v, instructions);
   return v.made_progress;
}

ir_visitor_status
ir_mod_to_fract_visitor::visit_leave(ir_expression *ir)
{
   if (ir->operation != ir_binop_mod)
      return visit_continue;

   ir_variable *temp = new(ir) ir_variable(ir->operands[1]->type, "mod_b",
					   ir_var_temporary);
   this->base_ir->insert_before(temp);

   ir_assignment *assign;
   ir_rvalue *expr;

   assign = new(ir) ir_assignment(new(ir) ir_dereference_variable(temp),
				  ir->operands[1], NULL);
   this->base_ir->insert_before(assign);

   expr = new(ir) ir_expression(ir_binop_div,
				ir->operands[0]->type,
				ir->operands[0],
				new(ir) ir_dereference_variable(temp));

   expr = new(ir) ir_expression(ir_unop_fract,
				ir->operands[0]->type,
				expr,
				NULL);

   ir->operation = ir_binop_mul;
   ir->operands[0] = new(ir) ir_dereference_variable(temp);
   ir->operands[1] = expr;
   this->made_progress = true;

   return visit_continue;
}
