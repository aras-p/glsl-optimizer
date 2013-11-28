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
   if (ir->normative_bound < 0)
      return visit_continue;

   exec_list new_instructions;
   ir_factory f(&new_instructions, ralloc_parent(ir));

   /* Before the loop, declare the counter and initialize it to zero. */
   ir_variable *counter = f.make_temp(glsl_type::uint_type, "counter");
   f.emit(assign(counter, f.constant(0u)));
   ir->insert_before(&new_instructions);

   /* At the top of the loop, compare the counter to normative_bound, and
    * break if the comparison succeeds.
    */
   ir_loop_jump *brk = new(f.mem_ctx) ir_loop_jump(ir_loop_jump::jump_break);
   ir_if *if_inst = if_tree(gequal(counter,
                                   f.constant((unsigned) ir->normative_bound)),
                            brk);
   ir->body_instructions.push_head(if_inst);

   /* At the bottom of the loop, increment the counter. */
   ir->body_instructions.push_tail(assign(counter,
                                          add(counter, f.constant(1u))));

   /* Since we've explicitly added instructions to terminate the loop, we no
    * longer need it to have a normative bound.
    */
   ir->normative_bound = -1;

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
