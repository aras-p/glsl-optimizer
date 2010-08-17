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
 * \file ir_expression_flattening.cpp
 *
 * Takes the leaves of expression trees and makes them dereferences of
 * assignments of the leaves to temporaries, according to a predicate.
 *
 * This is used for automatic function inlining, where we want to take
 * an expression containing a call and move the call out to its own
 * assignment so that we can inline it at the appropriate place in the
 * instruction stream.
 */

#include "ir.h"
#include "ir_visitor.h"
#include "ir_expression_flattening.h"
#include "glsl_types.h"

class ir_expression_flattening_visitor : public ir_hierarchical_visitor {
public:
   ir_expression_flattening_visitor(ir_instruction *base_ir,
				    bool (*predicate)(ir_instruction *ir))
   {
      this->base_ir = base_ir;
      this->predicate = predicate;
   }

   virtual ~ir_expression_flattening_visitor()
   {
      /* empty */
   }

   virtual ir_visitor_status visit_enter(ir_call *);
   virtual ir_visitor_status visit_enter(ir_return *);
   virtual ir_visitor_status visit_enter(ir_function_signature *);
   virtual ir_visitor_status visit_enter(ir_if *);
   virtual ir_visitor_status visit_enter(ir_loop *);
   virtual ir_visitor_status visit_leave(ir_assignment *);
   virtual ir_visitor_status visit_leave(ir_expression *);
   virtual ir_visitor_status visit_leave(ir_swizzle *);

   ir_rvalue *operand_to_temp(ir_rvalue *val);
   bool (*predicate)(ir_instruction *ir);
   ir_instruction *base_ir;
};

void
do_expression_flattening(exec_list *instructions,
			 bool (*predicate)(ir_instruction *ir))
{
   foreach_iter(exec_list_iterator, iter, *instructions) {
      ir_instruction *ir = (ir_instruction *)iter.get();

      ir_expression_flattening_visitor v(ir, predicate);
      ir->accept(&v);
   }
}


ir_rvalue *
ir_expression_flattening_visitor::operand_to_temp(ir_rvalue *ir)
{
   void *ctx = base_ir;
   ir_variable *var;
   ir_assignment *assign;

   if (!this->predicate(ir))
      return ir;

   var = new(ctx) ir_variable(ir->type, "flattening_tmp", ir_var_temporary);
   base_ir->insert_before(var);

   assign = new(ctx) ir_assignment(new(ctx) ir_dereference_variable(var),
				   ir,
				   NULL);
   base_ir->insert_before(assign);

   return new(ctx) ir_dereference_variable(var);
}

ir_visitor_status
ir_expression_flattening_visitor::visit_enter(ir_function_signature *ir)
{
   do_expression_flattening(&ir->body, this->predicate);

   return visit_continue_with_parent;
}

ir_visitor_status
ir_expression_flattening_visitor::visit_enter(ir_loop *ir)
{
   do_expression_flattening(&ir->body_instructions, this->predicate);

   return visit_continue_with_parent;
}

ir_visitor_status
ir_expression_flattening_visitor::visit_enter(ir_if *ir)
{
   ir->condition->accept(this);

   do_expression_flattening(&ir->then_instructions, this->predicate);
   do_expression_flattening(&ir->else_instructions, this->predicate);

   return visit_continue_with_parent;
}

ir_visitor_status
ir_expression_flattening_visitor::visit_leave(ir_expression *ir)
{
   unsigned int operand;

   for (operand = 0; operand < ir->get_num_operands(); operand++) {
      /* If the operand matches the predicate, then we'll assign its
       * value to a temporary and deref the temporary as the operand.
       */
      ir->operands[operand] = operand_to_temp(ir->operands[operand]);
   }

   return visit_continue;
}

ir_visitor_status
ir_expression_flattening_visitor::visit_leave(ir_assignment *ir)
{
   ir->rhs = operand_to_temp(ir->rhs);
   if (ir->condition)
      ir->condition = operand_to_temp(ir->condition);

   return visit_continue;
}

ir_visitor_status
ir_expression_flattening_visitor::visit_leave(ir_swizzle *ir)
{
   if (this->predicate(ir->val)) {
      ir->val = operand_to_temp(ir->val);
   }

   return visit_continue;
}

ir_visitor_status
ir_expression_flattening_visitor::visit_enter(ir_call *ir)
{
   /* Reminder: iterating ir_call iterates its parameters. */
   foreach_iter(exec_list_iterator, iter, *ir) {
      ir_rvalue *ir = (ir_rvalue *)iter.get();
      ir_rvalue *new_ir = operand_to_temp(ir);

      if (new_ir != ir) {
	 ir->replace_with(new_ir);
      }
   }

   return visit_continue;
}


ir_visitor_status
ir_expression_flattening_visitor::visit_enter(ir_return *ir)
{
   if (ir->value)
      ir->value = operand_to_temp(ir->value);
   return visit_continue;
}
