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
 * \file ir_if_return.cpp
 *
 * This pass tries to normalize functions to always return from one
 * place by moving around blocks of code in if statements.
 *
 * This helps on hardware with no branching support, and may even be a
 * useful transform on hardware supporting control flow by turning
 * masked returns into normal returns.
 */

#include <string.h>
#include "glsl_types.h"
#include "ir.h"

class ir_if_return_visitor : public ir_hierarchical_visitor {
public:
   ir_if_return_visitor()
   {
      this->progress = false;
   }

   ir_visitor_status visit_enter(ir_function_signature *);
   ir_visitor_status visit_leave(ir_if *);

   ir_visitor_status move_outer_block_inside(ir_instruction *ir,
					     exec_list *inner_block);
   void move_returns_after_block(ir_instruction *ir,
				 ir_return *then_return,
				 ir_return *else_return);
   bool progress;
};

bool
do_if_return(exec_list *instructions)
{
   ir_if_return_visitor v;

   do {
      v.progress = false;
      visit_list_elements(&v, instructions);
   } while (v.progress);

   return v.progress;
}

/**
 * Removes any instructions after a (unconditional) return, since they will
 * never be executed.
 */
static void
truncate_after_instruction(ir_instruction *ir)
{
   if (!ir)
      return;

   while (!ir->get_next()->is_tail_sentinel())
      ((ir_instruction *)ir->get_next())->remove();
}

/**
 * Returns an ir_instruction of the first ir_return in the exec_list, or NULL.
 */
static ir_return *
find_return_in_block(exec_list *instructions)
{
   foreach_iter(exec_list_iterator, iter, *instructions) {
      ir_instruction *ir = (ir_instruction *)iter.get();
      if (ir->ir_type == ir_type_return)
	 return (ir_return *)ir;
   }

   return NULL;
}

void
ir_if_return_visitor::move_returns_after_block(ir_instruction *ir,
					       ir_return *then_return,
					       ir_return *else_return)
{

   if (!then_return->value) {
      then_return->remove();
      else_return->remove();
      ir->insert_after(new(ir) ir_return(NULL));
   } else {
      ir_assignment *assign;
      ir_variable *new_var = new(ir) ir_variable(then_return->value->type,
						 "if_return_tmp",
						 ir_var_temporary);
      ir->insert_before(new_var);

      assign = new(ir) ir_assignment(new(ir) ir_dereference_variable(new_var),
				     then_return->value, NULL);
      then_return->replace_with(assign);

      assign = new(ir) ir_assignment(new(ir) ir_dereference_variable(new_var),
				     else_return->value, NULL);
      else_return->replace_with(assign);

      ir_dereference_variable *deref = new(ir) ir_dereference_variable(new_var);
      ir->insert_after(new(ir) ir_return(deref));
   }
   this->progress = true;
}

ir_visitor_status
ir_if_return_visitor::move_outer_block_inside(ir_instruction *ir,
					      exec_list *inner_block)
{
   if (!ir->get_next()->is_tail_sentinel()) {
      while (!ir->get_next()->is_tail_sentinel()) {
	 ir_instruction *move_ir = (ir_instruction *)ir->get_next();

	 move_ir->remove();
	 inner_block->push_tail(move_ir);
      }

      /* If we move the instructions following ir inside the block, it
       * will confuse the exec_list iteration in the parent that visited
       * us.  So stop the visit at this point.
       */
      return visit_stop;
   } else {
      return visit_continue;
   }
}

/* Normalize a function to always have a return statement at the end.
 *
 * This avoids the ir_if handler needing to know whether it is at the
 * top level of the function to know if there's an implicit return at
 * the end of the outer block.
 */
ir_visitor_status
ir_if_return_visitor::visit_enter(ir_function_signature *ir)
{
   ir_return *ret;

   if (!ir->is_defined)
      return visit_continue_with_parent;
   if (strcmp(ir->function_name(), "main") == 0)
      return visit_continue_with_parent;

   ret = find_return_in_block(&ir->body);

   if (ret) {
      truncate_after_instruction(ret);
   } else {
      if (ir->return_type->is_void()) {
	 ir->body.push_tail(new(ir) ir_return(NULL));
      } else {
	 /* Probably, if we've got a function with a return value
	  * hitting this point, it's something like:
	  *
	  * float reduce_below_half(float val)
	  * {
	  *         while () {
	  *                 if (val >= 0.5)
	  *                         val /= 2.0;
	  *                 else
	  *                         return val;
	  *         }
	  * }
	  *
	  * So we gain a junk return statement of an undefined value
	  * at the end that never gets executed.  However, a backend
	  * using this pass is probably desperate to get rid of
	  * function calls, so go ahead and do it for their sake in
	  * case it fixes apps.
	  */
	 ir_variable *undef = new(ir) ir_variable(ir->return_type,
						  "if_return_undef",
						  ir_var_temporary);
	 ir->body.push_tail(undef);

	 ir_dereference_variable *deref = new(ir) ir_dereference_variable(undef);
	 ir->body.push_tail(new(ir) ir_return(deref));
      }
   }

   return visit_continue;
}

ir_visitor_status
ir_if_return_visitor::visit_leave(ir_if *ir)
{
   ir_return *then_return;
   ir_return *else_return;

   then_return = find_return_in_block(&ir->then_instructions);
   else_return = find_return_in_block(&ir->else_instructions);
   if (!then_return && !else_return)
      return visit_continue;

   /* Trim off any trailing instructions after the return statements
    * on both sides.
    */
   truncate_after_instruction(then_return);
   truncate_after_instruction(else_return);

   /* If both sides return, then we can move the returns to a single
    * one outside the if statement.
    */
   if (then_return && else_return) {
      move_returns_after_block(ir, then_return, else_return);
      return visit_continue;
   }

   /* If only one side returns, then the block of code after the "if"
    * is only executed by the other side, so those instructions don't
    * need to be anywhere but that other side.
    *
    * This will usually pull a return statement up into the other
    * side, so we'll trigger the above case on the next pass.
    */
   if (then_return) {
      return move_outer_block_inside(ir, &ir->else_instructions);
   } else {
      assert(else_return);
      return move_outer_block_inside(ir, &ir->then_instructions);
   }
}
