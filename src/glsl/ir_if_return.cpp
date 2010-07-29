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
 * If a function includes an if statement that returns from both
 * branches, then make the branches write the return val to a temp and
 * return the temp after the if statement.
 *
 * This allows inlinining in the common case of short functions that
 * return one of two values based on a condition.  This helps on
 * hardware with no branching support, and may even be a useful
 * transform on hardware supporting control flow by masked returns
 * with normal returns.
 */

#include "ir.h"

class ir_if_return_visitor : public ir_hierarchical_visitor {
public:
   ir_if_return_visitor()
   {
      this->progress = false;
   }

   ir_visitor_status visit_enter(ir_if *);

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

ir_visitor_status
ir_if_return_visitor::visit_enter(ir_if *ir)
{
   ir_return *then_return = NULL;
   ir_return *else_return = NULL;

   then_return = find_return_in_block(&ir->then_instructions);
   else_return = find_return_in_block(&ir->else_instructions);
   if (!then_return || !else_return)
      return visit_continue;

   /* Trim off any trailing instructions after the return statements
    * on both sides.
    */
   truncate_after_instruction(then_return);
   truncate_after_instruction(else_return);

   this->progress = true;

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

   return visit_continue;
}
