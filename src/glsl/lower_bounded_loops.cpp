/*
 * Copyright © 2013 Intel Corporation
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

/** \file
 *
 * This pass converts bounded loops (those whose ir_loop contains non-null
 * values for \c from, \c to, \c increment, and \c counter) into unbounded
 * loops.
 *
 * For instance:
 *
 * (loop (declare () uint i) (constant uint 0) (constant uint 4)
 *       (constant uint 1)
 *   ...loop body...)
 *
 * Is transformed into:
 *
 * (declare () uint i)
 * (assign (x) (var_ref i) (constant uint 0))
 * (loop
 *   (if (expression bool >= (var_ref i) (constant uint 4))
 *       (break)
 *     ())
 *   ...loop body...
 *   (assign (x) (var_ref i)
 *               (expression uint + (var_ref i) (constant uint 1))))
 */

#include "ir_hierarchical_visitor.h"
#include "ir.h"
#include "ir_builder.h"

using namespace ir_builder;

namespace {

class lower_bounded_loops_visitor : public ir_hierarchical_visitor {
public:
   lower_bounded_loops_visitor()
      : progress(false)
   {
   }

   virtual ir_visitor_status visit_leave(ir_loop *ir);

   bool progress;
};

} /* anonymous namespace */


ir_visitor_status
lower_bounded_loops_visitor::visit_leave(ir_loop *ir)
{
   if (ir->counter == NULL)
      return visit_continue;

   exec_list new_instructions;
   ir_factory f(&new_instructions, ralloc_parent(ir));

   /* Before the loop, declare the counter and initialize it to "from". */
   f.emit(ir->counter);
   f.emit(assign(ir->counter, ir->from));
   ir->insert_before(&new_instructions);

   /* At the top of the loop, compare the counter to "to", and break if the
    * comparison succeeds.
    */
   ir_loop_jump *brk = new(f.mem_ctx) ir_loop_jump(ir_loop_jump::jump_break);
   ir_expression_operation cmp = (ir_expression_operation) ir->cmp;
   ir->body_instructions.push_head(if_tree(expr(cmp, ir->counter, ir->to),
                                           brk));

   /* At the bottom of the loop, increment the counter. */
   ir->body_instructions.push_tail(assign(ir->counter,
                                          add(ir->counter, ir->increment)));

   /* NULL out the counter, from, to, and increment variables. */
   ir->counter = NULL;
   ir->from = NULL;
   ir->to = NULL;
   ir->increment = NULL;

   this->progress = true;
   return visit_continue;
}


bool
lower_bounded_loops(exec_list *instructions)
{
   lower_bounded_loops_visitor v;

   visit_list_elements(&v, instructions);

   return v.progress;
}
