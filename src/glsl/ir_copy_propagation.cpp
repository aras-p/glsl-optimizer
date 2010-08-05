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
 * \file ir_copy_propagation.cpp
 *
 * Moves usage of recently-copied variables to the previous copy of
 * the variable within basic blocks.
 *
 * This should reduce the number of MOV instructions in the generated
 * programs unless copy propagation is also done on the LIR, and may
 * help anyway by triggering other optimizations that live in the HIR.
 */

#include "ir.h"
#include "ir_visitor.h"
#include "ir_basic_block.h"
#include "ir_optimization.h"
#include "glsl_types.h"

class acp_entry : public exec_node
{
public:
   acp_entry(ir_variable *lhs, ir_variable *rhs)
   {
      assert(lhs);
      assert(rhs);
      this->lhs = lhs;
      this->rhs = rhs;
   }

   ir_variable *lhs;
   ir_variable *rhs;
};

class ir_copy_propagation_visitor : public ir_hierarchical_visitor {
public:
   ir_copy_propagation_visitor(exec_list *acp)
   {
      progress = false;
      in_lhs = false;
      this->acp = acp;
   }

   virtual ir_visitor_status visit(class ir_dereference_variable *);
   virtual ir_visitor_status visit_enter(class ir_loop *);
   virtual ir_visitor_status visit_enter(class ir_function_signature *);
   virtual ir_visitor_status visit_enter(class ir_function *);
   virtual ir_visitor_status visit_enter(class ir_assignment *);
   virtual ir_visitor_status visit_enter(class ir_call *);
   virtual ir_visitor_status visit_enter(class ir_if *);

   /** List of acp_entry */
   exec_list *acp;
   bool progress;

   /** Currently in the LHS of an assignment? */
   bool in_lhs;
};


ir_visitor_status
ir_copy_propagation_visitor::visit_enter(ir_loop *ir)
{
   (void)ir;
   return visit_continue_with_parent;
}

ir_visitor_status
ir_copy_propagation_visitor::visit_enter(ir_function_signature *ir)
{
   (void)ir;
   return visit_continue_with_parent;
}

ir_visitor_status
ir_copy_propagation_visitor::visit_enter(ir_assignment *ir)
{
   ir_visitor_status s;

   /* Inline the rest of ir_assignment::accept(ir_hv *v), wrapping the
    * LHS part with setting in_lhs so that we can avoid copy
    * propagating into the LHS.
    *
    * Note that this means we won't copy propagate into the derefs of
    * an array index.  Oh well.
    */
   this->in_lhs = true;
   s = ir->lhs->accept(this);
   this->in_lhs = false;
   if (s != visit_continue)
      return (s == visit_continue_with_parent) ? visit_continue : s;

   s = ir->rhs->accept(this);
   if (s != visit_continue)
      return (s == visit_continue_with_parent) ? visit_continue : s;

   if (ir->condition)
      s = ir->condition->accept(this);

   return (s == visit_stop) ? s : visit_continue_with_parent;
}

ir_visitor_status
ir_copy_propagation_visitor::visit_enter(ir_function *ir)
{
   (void) ir;
   return visit_continue_with_parent;
}

/**
 * Replaces dereferences of ACP RHS variables with ACP LHS variables.
 *
 * This is where the actual copy propagation occurs.  Note that the
 * rewriting of ir_dereference means that the ir_dereference instance
 * must not be shared by multiple IR operations!
 */
ir_visitor_status
ir_copy_propagation_visitor::visit(ir_dereference_variable *ir)
{
   /* Ignores the LHS.  Don't want to rewrite the LHS to point at some
    * other storage!
    */
   if (this->in_lhs) {
      return visit_continue;
   }

   ir_variable *var = ir->variable_referenced();

   foreach_iter(exec_list_iterator, iter, *this->acp) {
      acp_entry *entry = (acp_entry *)iter.get();

      if (var == entry->lhs) {
	 ir->var = entry->rhs;
	 this->progress = true;
	 break;
      }
   }

   return visit_continue;
}


ir_visitor_status
ir_copy_propagation_visitor::visit_enter(ir_call *ir)
{
   (void)ir;

   /* Note, if we were to do copy propagation to parameters of calls, we'd
    * have to be careful about out params.
    */
   return visit_continue_with_parent;
}


ir_visitor_status
ir_copy_propagation_visitor::visit_enter(ir_if *ir)
{
   ir->condition->accept(this);

   /* Do not traverse into the body of the if-statement since that is a
    * different basic block.
    */
   return visit_continue_with_parent;
}

static bool
propagate_copies(ir_instruction *ir, exec_list *acp)
{
   ir_copy_propagation_visitor v(acp);

   ir->accept(&v);

   return v.progress;
}

static void
kill_invalidated_copies(ir_assignment *ir, exec_list *acp)
{
   ir_variable *var = ir->lhs->variable_referenced();
   assert(var != NULL);

   foreach_iter(exec_list_iterator, iter, *acp) {
      acp_entry *entry = (acp_entry *)iter.get();

      if (entry->lhs == var || entry->rhs == var) {
	 entry->remove();
      }
   }
}

/**
 * Adds an entry to the available copy list if it's a plain assignment
 * of a variable to a variable.
 */
static void
add_copy(void *ctx, ir_assignment *ir, exec_list *acp)
{
   acp_entry *entry;

   if (ir->condition) {
      ir_constant *condition = ir->condition->as_constant();
      if (!condition || !condition->value.b[0])
	 return;
   }

   ir_variable *lhs_var = ir->whole_variable_written();
   ir_variable *rhs_var = ir->rhs->whole_variable_referenced();

   if ((lhs_var != NULL) && (rhs_var != NULL)) {
      entry = new(ctx) acp_entry(lhs_var, rhs_var);
      acp->push_tail(entry);
   }
}

static void
copy_propagation_basic_block(ir_instruction *first,
			     ir_instruction *last,
			     void *data)
{
   ir_instruction *ir;
   /* List of avaialble_copy */
   exec_list acp;
   bool *out_progress = (bool *)data;
   bool progress = false;

   void *ctx = talloc_new(NULL);
   for (ir = first;; ir = (ir_instruction *)ir->next) {
      ir_assignment *ir_assign = ir->as_assignment();

      progress = propagate_copies(ir, &acp) || progress;

      if (ir_assign) {
	 kill_invalidated_copies(ir_assign, &acp);

	 add_copy(ctx, ir_assign, &acp);
      }
      if (ir == last)
	 break;
   }
   *out_progress = progress;
   talloc_free(ctx);
}

/**
 * Does a copy propagation pass on the code present in the instruction stream.
 */
bool
do_copy_propagation(exec_list *instructions)
{
   bool progress = false;

   call_for_basic_blocks(instructions, copy_propagation_basic_block, &progress);

   return progress;
}
