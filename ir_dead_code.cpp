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

#define NULL 0
#include "ir.h"
#include "ir_visitor.h"
#include "ir_expression_flattening.h"
#include "glsl_types.h"

class variable_entry : public exec_node
{
public:
   variable_entry(ir_variable *var)
   {
      this->var = var;
      assign = NULL;
      referenced = false;
      declaration = false;
   }

   ir_variable *var; /* The key: the variable's pointer. */
   ir_assignment *assign; /* An assignment to the variable, if any */
   bool referenced; /* If the variable has ever been referenced. */
   bool declaration; /* If the variable had a decl in the instruction stream */
};

class ir_dead_code_visitor : public ir_visitor {
public:

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

   variable_entry *get_variable_entry(ir_variable *var);

   bool (*predicate)(ir_instruction *ir);
   ir_instruction *base_ir;

   /* List of variable_entry */
   exec_list variable_list;
};

variable_entry *
ir_dead_code_visitor::get_variable_entry(ir_variable *var)
{
   assert(var);
   foreach_iter(exec_list_iterator, iter, this->variable_list) {
      variable_entry *entry = (variable_entry *)iter.get();
      if (entry->var == var)
	 return entry;
   }

   variable_entry *entry = new variable_entry(var);
   this->variable_list.push_tail(entry);
   return entry;
}

void
find_dead_code(exec_list *instructions, ir_dead_code_visitor *v)
{
   foreach_iter(exec_list_iterator, iter, *instructions) {
      ir_instruction *ir = (ir_instruction *)iter.get();

      ir->accept(v);
   }
}

void
ir_dead_code_visitor::visit(ir_variable *ir)
{
   variable_entry *entry = this->get_variable_entry(ir);
   if (entry) {
      entry->declaration = true;
   }
}


void
ir_dead_code_visitor::visit(ir_loop *ir)
{
   find_dead_code(&ir->body_instructions, this);
   if (ir->from)
      ir->from->accept(this);
   if (ir->to)
      ir->to->accept(this);
   if (ir->increment)
      ir->increment->accept(this);
}

void
ir_dead_code_visitor::visit(ir_loop_jump *ir)
{
   (void) ir;
}


void
ir_dead_code_visitor::visit(ir_function_signature *ir)
{
   find_dead_code(&ir->body, this);
}

void
ir_dead_code_visitor::visit(ir_function *ir)
{
   (void) ir;
}

void
ir_dead_code_visitor::visit(ir_expression *ir)
{
   unsigned int operand;

   for (operand = 0; operand < ir->get_num_operands(); operand++) {
      ir->operands[operand]->accept(this);
   }
}


void
ir_dead_code_visitor::visit(ir_swizzle *ir)
{
   ir->val->accept(this);
}


void
ir_dead_code_visitor::visit(ir_dereference *ir)
{
   ir_variable *var;

   if (ir->mode == ir_dereference::ir_reference_array) {
      ir->selector.array_index->accept(this);
   }

   var = ir->var->as_variable();
   if (var) {
      variable_entry *entry = this->get_variable_entry(var);
      entry->referenced = true;
   } else {
      ir->var->accept(this);
   }
}

void
ir_dead_code_visitor::visit(ir_assignment *ir)
{
   ir_instruction *lhs = ir->lhs;

   /* Walk through the LHS and mark references for variables used in
    * array indices but not for the assignment dereference.
    */
   while (lhs) {
      if (lhs->as_variable())
	 break;

      ir_dereference *deref = lhs->as_dereference();
      if (deref) {
	 if (deref->mode == ir_dereference::ir_reference_array)
	    deref->selector.array_index->accept(this);
	 lhs = deref->var;
      } else {
	 ir_swizzle *swiz = lhs->as_swizzle();

	 lhs = swiz->val;
      }
   }

   ir->rhs->accept(this);
   if (ir->condition)
      ir->condition->accept(this);

   variable_entry *entry;
   entry = this->get_variable_entry(lhs->as_variable());
   if (entry) {
      if (entry->assign == NULL)
	 entry->assign = ir;
   }
}


void
ir_dead_code_visitor::visit(ir_constant *ir)
{
   (void) ir;
}


void
ir_dead_code_visitor::visit(ir_call *ir)
{
   foreach_iter(exec_list_iterator, iter, *ir) {
      ir_rvalue *param = (ir_rvalue *)iter.get();

      /* FINISHME: handle out values. */
      param->accept(this);
   }

   /* Ignore the callee.  Function bodies will get handled when they're
    * encountered at the top level instruction stream and spawn their
    * own dead code visitor.
    */
}


void
ir_dead_code_visitor::visit(ir_return *ir)
{
   ir_rvalue *val = ir->get_value();

   if (val)
      val->accept(this);
}


void
ir_dead_code_visitor::visit(ir_if *ir)
{
   ir->condition->accept(this);

   find_dead_code(&ir->then_instructions, this);
   find_dead_code(&ir->else_instructions, this);
}

/**
 * Do a dead code pass over instructions and everything that instructions
 * references.
 *
 * Note that this will remove assignments to globals, so it is not suitable
 * for usage on an unlinked instruction stream.
 */
bool
do_dead_code(exec_list *instructions)
{
   ir_dead_code_visitor v;
   bool progress = false;

   find_dead_code(instructions, &v);

   foreach_iter(exec_list_iterator, iter, v.variable_list) {
      variable_entry *entry = (variable_entry *)iter.get();

      if (entry->referenced || !entry->declaration)
	 continue;

      if (entry->assign) {
	 /* Remove a single dead assignment to the variable we found.
	  * Don't do so if it's a shader output, though.
	  */
	 if (!entry->var->shader_out) {
	    entry->assign->remove();
	    progress = true;
	 }
      } else {
	 /* If there are no assignments or references to the variable left,
	  * then we can remove its declaration.
	  */
	 entry->var->remove();
	 progress = true;
      }
   }
   return progress;
}

/**
 * Does a dead code pass on the functions present in the instruction stream.
 *
 * This is suitable for use while the program is not linked, as it will
 * ignore variable declarations (and the assignments to them) for variables
 * with global scope.
 */
bool
do_dead_code_unlinked(exec_list *instructions)
{
   bool progress = false;

   foreach_iter(exec_list_iterator, iter, *instructions) {
      ir_instruction *ir = (ir_instruction *)iter.get();
      ir_function *f = ir->as_function();
      if (f) {
	 foreach_iter(exec_list_iterator, sigiter, *f) {
	    ir_function_signature *sig =
	       (ir_function_signature *) sigiter.get();
	    if (do_dead_code(&sig->body))
	       progress = true;
	 }
      }
   }

   return progress;
}
