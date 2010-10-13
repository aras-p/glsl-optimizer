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
 * \file ir_if_to_cond_assign.cpp
 *
 * This attempts to flatten all if statements to conditional
 * assignments for GPUs that don't do control flow.
 *
 * It can't handle other control flow being inside of its block, such
 * as calls or loops.  Hopefully loop unrolling and inlining will take
 * care of those.
 */

#include "glsl_types.h"
#include "ir.h"

class ir_if_to_cond_assign_visitor : public ir_hierarchical_visitor {
public:
   ir_if_to_cond_assign_visitor()
   {
      this->progress = false;
   }

   ir_visitor_status visit_leave(ir_if *);

   bool progress;
};

bool
do_if_to_cond_assign(exec_list *instructions)
{
   ir_if_to_cond_assign_visitor v;

   visit_list_elements(&v, instructions);

   return v.progress;
}

void
check_control_flow(ir_instruction *ir, void *data)
{
   bool *found_control_flow = (bool *)data;
   switch (ir->ir_type) {
   case ir_type_call:
   case ir_type_discard:
   case ir_type_loop:
   case ir_type_loop_jump:
   case ir_type_return:
      *found_control_flow = true;
      break;
   default:
      break;
   }
}

void
move_block_to_cond_assign(void *mem_ctx,
			  ir_if *if_ir, ir_variable *cond_var, bool then)
{
   exec_list *instructions;

   if (then) {
      instructions = &if_ir->then_instructions;
   } else {
      instructions = &if_ir->else_instructions;
   }

   foreach_iter(exec_list_iterator, iter, *instructions) {
      ir_instruction *ir = (ir_instruction *)iter.get();

      if (ir->ir_type == ir_type_assignment) {
	 ir_assignment *assign = (ir_assignment *)ir;
	 ir_rvalue *cond_expr;
	 ir_dereference *deref = new(mem_ctx) ir_dereference_variable(cond_var);

	 if (then) {
	    cond_expr = deref;
	 } else {
	    cond_expr = new(mem_ctx) ir_expression(ir_unop_logic_not,
						   glsl_type::bool_type,
						   deref,
						   NULL);
	 }

	 if (!assign->condition) {
	    assign->condition = cond_expr;
	 } else {
	    assign->condition = new(mem_ctx) ir_expression(ir_binop_logic_and,
							   glsl_type::bool_type,
							   cond_expr,
							   assign->condition);
	 }
      }

      /* Now, move from the if block to the block surrounding it. */
      ir->remove();
      if_ir->insert_before(ir);
   }
}

ir_visitor_status
ir_if_to_cond_assign_visitor::visit_leave(ir_if *ir)
{
   bool found_control_flow = false;
   ir_variable *cond_var;
   ir_assignment *assign;
   ir_dereference_variable *deref;

   /* Check that both blocks don't contain anything we can't support. */
   foreach_iter(exec_list_iterator, then_iter, ir->then_instructions) {
      ir_instruction *then_ir = (ir_instruction *)then_iter.get();
      visit_tree(then_ir, check_control_flow, &found_control_flow);
   }
   foreach_iter(exec_list_iterator, else_iter, ir->else_instructions) {
      ir_instruction *else_ir = (ir_instruction *)else_iter.get();
      visit_tree(else_ir, check_control_flow, &found_control_flow);
   }
   if (found_control_flow)
      return visit_continue;

   void *mem_ctx = talloc_parent(ir);

   /* Store the condition to a variable so the assignment conditions are
    * simpler.
    */
   cond_var = new(mem_ctx) ir_variable(glsl_type::bool_type,
				       "if_to_cond_assign_condition",
				       ir_var_temporary, ir_precision_undefined);
   ir->insert_before(cond_var);

   deref = new(mem_ctx) ir_dereference_variable(cond_var);
   assign = new(mem_ctx) ir_assignment(deref,
				       ir->condition, NULL);
   ir->insert_before(assign);

   /* Now, move all of the instructions out of the if blocks, putting
    * conditions on assignments.
    */
   move_block_to_cond_assign(mem_ctx, ir, cond_var, true);
   move_block_to_cond_assign(mem_ctx, ir, cond_var, false);

   ir->remove();

   this->progress = true;

   return visit_continue;
}
