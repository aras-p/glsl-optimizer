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

#include "glsl_types.h"
#include "loop_analysis.h"
#include "ir_hierarchical_visitor.h"

class loop_unroll_visitor : public ir_hierarchical_visitor {
public:
   loop_unroll_visitor(loop_state *state)
   {
      this->state = state;
      this->progress = false;
   }

   virtual ir_visitor_status visit_leave(ir_loop *ir);

   loop_state *state;

   bool progress;
};


ir_visitor_status
loop_unroll_visitor::visit_leave(ir_loop *ir)
{
   loop_variable_state *const ls = this->state->get(ir);

   /* If we've entered a loop that hasn't been analyzed, something really,
    * really bad has happened.
    */
   if (ls == NULL) {
      assert(ls != NULL);
      return visit_continue;
   }

   /* Don't try to unroll loops where the number of iterations is not known
    * at compile-time.
    */
   if (ls->max_iterations < 0)
      return visit_continue;

   /* Don't try to unroll loops that have zillions of iterations either.
    */
   if (ls->max_iterations > 32)
      return visit_continue;

   if (ls->num_loop_jumps > 0)
      return visit_continue;

   void *const mem_ctx = talloc_parent(ir);

   for (int i = 0; i < ls->max_iterations; i++) {
      exec_list copy_list;

      copy_list.make_empty();
      clone_ir_list(mem_ctx, &copy_list, &ir->body_instructions);

      ir->insert_before(&copy_list);
   }

   /* The loop has been replaced by the unrolled copies.  Remove the original
    * loop from the IR sequence.
    */
   ir->remove();

   this->progress = true;
   return visit_continue;
}


bool
unroll_loops(exec_list *instructions, loop_state *ls)
{
   loop_unroll_visitor v(ls);

   v.run(instructions);

   return v.progress;
}
