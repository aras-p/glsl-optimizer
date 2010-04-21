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
 * \file ir_function_inlining.cpp
 *
 * Moves constant branches of if statements out to the surrounding
 * instruction stream.
 */

#define NULL 0
#include "ir.h"
#include "ir_visitor.h"
#include "ir_function_inlining.h"
#include "glsl_types.h"

class ir_if_simplification_visitor : public ir_visitor {
public:
   ir_if_simplification_visitor()
   {
      /* empty */
   }

   virtual ~ir_if_simplification_visitor()
   {
      /* empty */
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
};

bool
do_if_simplification(exec_list *instructions)
{
   bool progress = false;

   foreach_iter(exec_list_iterator, iter, *instructions) {
      ir_instruction *ir = (ir_instruction *)iter.get();
      ir_if *conditional = ir->as_if();

      if (conditional) {
	 ir_constant *condition_constant;

	 condition_constant =
	    conditional->condition->constant_expression_value();
	 if (condition_constant) {
	    /* Move the contents of the one branch of the conditional
	     * that matters out.
	     */
	    if (condition_constant->value.b[0]) {
	       foreach_iter(exec_list_iterator, then_iter,
			    conditional->then_instructions) {
		  ir_instruction *then_ir = (ir_instruction *)then_iter.get();
		  ir->insert_before(then_ir);
	       }
	    } else {
	       foreach_iter(exec_list_iterator, else_iter,
			    conditional->else_instructions) {
		  ir_instruction *else_ir = (ir_instruction *)else_iter.get();
		  ir->insert_before(else_ir);
	       }
	    }
	    ir->remove();
	    progress = true;
	    /* It would be nice to move the iterator back up to the point
	     * that we just spliced in contents.
	     */
	 } else {
	    ir_if_simplification_visitor v;
	    ir->accept(&v);
	 }
      } else {
	 ir_if_simplification_visitor v;
	 ir->accept(&v);
      }
   }

   return progress;
}

class variable_remap : public exec_node {
public:
   variable_remap(const ir_variable *old_var, ir_variable *new_var)
      : old_var(old_var), new_var(new_var)
   {
      /* empty */
   }
   const ir_variable *old_var;
   ir_variable *new_var;
};

void
ir_if_simplification_visitor::visit(ir_variable *ir)
{
   (void) ir;
}


void
ir_if_simplification_visitor::visit(ir_loop *ir)
{
   do_if_simplification(&ir->body_instructions);
}

void
ir_if_simplification_visitor::visit(ir_loop_jump *ir)
{
   (void) ir;
}


void
ir_if_simplification_visitor::visit(ir_function_signature *ir)
{
   do_if_simplification(&ir->body);
}


void
ir_if_simplification_visitor::visit(ir_function *ir)
{
   foreach_iter(exec_list_iterator, iter, *ir) {
      ir_function_signature *const sig = (ir_function_signature *) iter.get();
      sig->accept(this);
   }
}

void
ir_if_simplification_visitor::visit(ir_expression *ir)
{
   unsigned int operand;

   for (operand = 0; operand < ir->get_num_operands(); operand++) {
      ir->operands[operand]->accept(this);
   }
}


void
ir_if_simplification_visitor::visit(ir_swizzle *ir)
{
   ir->val->accept(this);
}


void
ir_if_simplification_visitor::visit(ir_dereference *ir)
{
   if (ir->mode == ir_dereference::ir_reference_array) {
      ir->selector.array_index->accept(this);
   }
   ir->var->accept(this);
}

void
ir_if_simplification_visitor::visit(ir_assignment *ir)
{
   ir->rhs->accept(this);
}


void
ir_if_simplification_visitor::visit(ir_constant *ir)
{
   (void) ir;
}


void
ir_if_simplification_visitor::visit(ir_call *ir)
{
   (void) ir;
}


void
ir_if_simplification_visitor::visit(ir_return *ir)
{
   (void) ir;
}


void
ir_if_simplification_visitor::visit(ir_if *ir)
{
   ir->condition->accept(this);

   do_if_simplification(&ir->then_instructions);
   do_if_simplification(&ir->else_instructions);
}
