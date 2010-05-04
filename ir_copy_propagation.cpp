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
 * \file ir_dead_code.cpp
 *
 * Eliminates dead assignments and variable declarations from the code.
 */

#include <stdio.h>
#include "ir.h"
#include "ir_visitor.h"
#include "ir_basic_block.h"
#include "ir_copy_propagation.h"
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

class ir_copy_propagation_visitor : public ir_visitor {
public:
   ir_copy_propagation_visitor(exec_list *acp)
   {
      progress = false;
      this->acp = acp;
   }

   /**
    * \name Visit methods
    *
    * As typical for the visitor pattern, there must be one \c visit method for
    * each concrete subclass of \c ir_instruction.  Virtual base classes within
    * the hierarchy should not have \c visit methods.
    */
   /*@{*/
   virtual void visit(ir_variable *);
   virtual void visit(ir_loop *);
   virtual void visit(ir_loop_jump *);
   virtual void visit(ir_function_signature *);
   virtual void visit(ir_function *);
   virtual void visit(ir_expression *);
   virtual void visit(ir_swizzle *);
   virtual void visit(ir_dereference *);
   virtual void visit(ir_assignment *);
   virtual void visit(ir_constant *);
   virtual void visit(ir_call *);
   virtual void visit(ir_return *);
   virtual void visit(ir_if *);
   /*@}*/

   /** List of acp_entry */
   exec_list *acp;
   bool progress;
};


void
ir_copy_propagation_visitor::visit(ir_variable *ir)
{
   (void)ir;
}


void
ir_copy_propagation_visitor::visit(ir_loop *ir)
{
   (void)ir;
}

void
ir_copy_propagation_visitor::visit(ir_loop_jump *ir)
{
   (void) ir;
}


void
ir_copy_propagation_visitor::visit(ir_function_signature *ir)
{
   (void)ir;
}

void
ir_copy_propagation_visitor::visit(ir_function *ir)
{
   (void) ir;
}

void
ir_copy_propagation_visitor::visit(ir_expression *ir)
{
   unsigned int operand;

   for (operand = 0; operand < ir->get_num_operands(); operand++) {
      ir->operands[operand]->accept(this);
   }
}


void
ir_copy_propagation_visitor::visit(ir_swizzle *ir)
{
   ir->val->accept(this);
}

/**
 * Replaces dereferences of ACP RHS variables with ACP LHS variables.
 *
 * This is where the actual copy propagation occurs.  Note that the
 * rewriting of ir_dereference means that the ir_dereference instance
 * must not be shared by multiple IR operations!
 */
void
ir_copy_propagation_visitor::visit(ir_dereference *ir)
{
   ir_variable *var;

   if (ir->mode == ir_dereference::ir_reference_array) {
      ir->selector.array_index->accept(this);
   }

   var = ir->var->as_variable();
   if (var) {
      foreach_iter(exec_list_iterator, iter, *this->acp) {
	 acp_entry *entry = (acp_entry *)iter.get();

	 if (var == entry->lhs) {
	    ir->var = entry->rhs;
	    this->progress = true;
	    break;
	 }
      }
   } else {
      ir->var->accept(this);
   }
}

void
ir_copy_propagation_visitor::visit(ir_assignment *ir)
{
   if (ir->condition)
      ir->condition->accept(this);

   /* Ignores the LHS.  Don't want to rewrite the LHS to point at some
    * other storage!
    */

   ir->rhs->accept(this);
}


void
ir_copy_propagation_visitor::visit(ir_constant *ir)
{
   (void) ir;
}


void
ir_copy_propagation_visitor::visit(ir_call *ir)
{
   (void)ir;

   /* Note, if we were to do copy propagation to parameters of calls, we'd
    * have to be careful about out params.
    */
}


void
ir_copy_propagation_visitor::visit(ir_return *ir)
{
   ir_rvalue *val = ir->get_value();

   if (val)
      val->accept(this);
}


void
ir_copy_propagation_visitor::visit(ir_if *ir)
{
   ir->condition->accept(this);
}

static void
propagate_copies(ir_instruction *ir, exec_list *acp)
{
   ir_copy_propagation_visitor v(acp);

   ir->accept(&v);
}

static void
kill_invalidated_copies(ir_assignment *ir, exec_list *acp)
{
   ir_dereference *lhs_deref = ir->lhs->as_dereference();

   /* Only handle simple dereferences for now. */
   if (lhs_deref &&
       lhs_deref->mode == ir_dereference::ir_reference_variable) {
      ir_variable *var = lhs_deref->var->as_variable();

      foreach_iter(exec_list_iterator, iter, *acp) {
	 acp_entry *entry = (acp_entry *)iter.get();

	 if (entry->lhs == var || entry->rhs == var) {
	    entry->remove();
	 }
      }
   } else {
      /* FINISHME: Only clear out the entries we overwrote here. */
      acp->make_empty();
   }
}

/**
 * Adds an entry to the available copy list if it's a plain assignment
 * of a variable to a variable.
 */
static void
add_copy(ir_assignment *ir, exec_list *acp)
{
   acp_entry *entry;

   if (ir->condition) {
      ir_constant *condition = ir->condition->as_constant();
      if (!condition || !condition->value.b[0])
	 return;
   }

   ir_dereference *lhs_deref = ir->lhs->as_dereference();
   if (!lhs_deref || lhs_deref->mode != ir_dereference::ir_reference_variable)
      return;
   ir_variable *lhs_var = lhs_deref->var->as_variable();

   ir_dereference *rhs_deref = ir->rhs->as_dereference();
   if (!rhs_deref || rhs_deref->mode != ir_dereference::ir_reference_variable)
      return;
   ir_variable *rhs_var = rhs_deref->var->as_variable();

   entry = new acp_entry(lhs_var, rhs_var);
   acp->push_tail(entry);
}

static void
copy_propagation_basic_block(ir_instruction *first,
			     ir_instruction *last)
{
   ir_instruction *ir;
   /* List of avaialble_copy */
   exec_list acp;

   for (ir = first;; ir = (ir_instruction *)ir->next) {
      ir_assignment *ir_assign = ir->as_assignment();

      propagate_copies(ir, &acp);

      if (ir_assign) {
	 kill_invalidated_copies(ir_assign, &acp);

	 add_copy(ir_assign, &acp);
      }
      if (ir == last)
	 break;
   }
}

/**
 * Does a copy propagation pass on the code present in the instruction stream.
 */
bool
do_copy_propagation(exec_list *instructions)
{
   bool progress = false;

   call_for_basic_blocks(instructions, copy_propagation_basic_block);

   /* FINISHME: Return a legit progress value. */
   return progress;
}
