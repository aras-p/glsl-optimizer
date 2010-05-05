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
#include "ir_visit_tree.h"

class ir_tree_visitor : public ir_visitor {
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

   void (*callback)(ir_instruction *ir, void *data);
   void *data;
};

void
ir_tree_visitor::visit(ir_variable *ir)
{
   this->callback(ir, this->data);
}


void
ir_tree_visitor::visit(ir_loop *ir)
{
   this->callback(ir, this->data);

   visit_exec_list(&ir->body_instructions, this);
   if (ir->from)
      ir->from->accept(this);
   if (ir->to)
      ir->to->accept(this);
   if (ir->increment)
      ir->increment->accept(this);
}

void
ir_tree_visitor::visit(ir_loop_jump *ir)
{
   this->callback(ir, this->data);
}


void
ir_tree_visitor::visit(ir_function_signature *ir)
{
   this->callback(ir, this->data);

   visit_exec_list(&ir->body, this);
}

void
ir_tree_visitor::visit(ir_function *ir)
{
   this->callback(ir, this->data);

   /* FINISHME: Do we want to walk into functions? */
}

void
ir_tree_visitor::visit(ir_expression *ir)
{
   unsigned int operand;

   this->callback(ir, this->data);

   for (operand = 0; operand < ir->get_num_operands(); operand++) {
      ir->operands[operand]->accept(this);
   }
}


void
ir_tree_visitor::visit(ir_swizzle *ir)
{
   this->callback(ir, this->data);

   ir->val->accept(this);
}


void
ir_tree_visitor::visit(ir_dereference *ir)
{
   this->callback(ir, this->data);

   if (ir->mode == ir_dereference::ir_reference_array) {
      ir->selector.array_index->accept(this);
   }
   ir->var->accept(this);
}

void
ir_tree_visitor::visit(ir_assignment *ir)
{
   this->callback(ir, this->data);

   ir->lhs->accept(this);
   ir->rhs->accept(this);
   if (ir->condition)
      ir->condition->accept(this);
}


void
ir_tree_visitor::visit(ir_constant *ir)
{
   this->callback(ir, this->data);
}


void
ir_tree_visitor::visit(ir_call *ir)
{
   this->callback(ir, this->data);

   foreach_iter(exec_list_iterator, iter, *ir) {
      ir_rvalue *param = (ir_rvalue *)iter.get();

      param->accept(this);
   }
}


void
ir_tree_visitor::visit(ir_return *ir)
{
   ir_rvalue *val = ir->get_value();

   this->callback(ir, this->data);

   if (val)
      val->accept(this);
}


void
ir_tree_visitor::visit(ir_if *ir)
{
   this->callback(ir, this->data);

   ir->condition->accept(this);

   visit_exec_list(&ir->then_instructions, this);
   visit_exec_list(&ir->else_instructions, this);
}

void ir_visit_tree(ir_instruction *ir,
		   void (*callback)(ir_instruction *ir,
				    void *data),
		   void *data)
{
   ir_tree_visitor v;
   v.callback = callback;
   v.data = data;

   ir->accept(&v);
}

