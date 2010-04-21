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
 * \file ir_expression_flattening.cpp
 *
 * Takes the leaves of expression trees and makes them dereferences of
 * assignments of the leaves to temporaries, according to a predicate.
 *
 * This is used for automatic function inlining, where we want to take
 * an expression containing a call and move the call out to its own
 * assignment so that we can inline it at the appropriate place in the
 * instruction stream.
 */

#define NULL 0
#include "ir.h"
#include "ir_visitor.h"
#include "ir_expression_flattening.h"
#include "glsl_types.h"

class ir_expression_flattening_visitor : public ir_visitor {
public:
   ir_expression_flattening_visitor(ir_instruction *base_ir,
				    bool (*predicate)(ir_instruction *ir))
   {
      this->base_ir = base_ir;
      this->predicate = predicate;

      /* empty */
   }

   virtual ~ir_expression_flattening_visitor()
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

   bool (*predicate)(ir_instruction *ir);
   ir_instruction *base_ir;
};

void
do_expression_flattening(exec_list *instructions,
			 bool (*predicate)(ir_instruction *ir))
{
   foreach_iter(exec_list_iterator, iter, *instructions) {
      ir_instruction *ir = (ir_instruction *)iter.get();

      ir_expression_flattening_visitor v(ir, predicate);
      ir->accept(&v);
   }
}

void
ir_expression_flattening_visitor::visit(ir_variable *ir)
{
   (void) ir;
}

void
ir_expression_flattening_visitor::visit(ir_loop *ir)
{
   do_expression_flattening(&ir->body_instructions, this->predicate);
}

void
ir_expression_flattening_visitor::visit(ir_loop_jump *ir)
{
   (void) ir;
}


void
ir_expression_flattening_visitor::visit(ir_function_signature *ir)
{
   do_expression_flattening(&ir->body, this->predicate);
}

void
ir_expression_flattening_visitor::visit(ir_function *ir)
{
   (void) ir;
}

void
ir_expression_flattening_visitor::visit(ir_expression *ir)
{
   unsigned int operand;

   for (operand = 0; operand < ir->get_num_operands(); operand++) {
      ir->operands[operand]->accept(this);

      /* If the operand matches the predicate, then we'll assign its
       * value to a temporary and deref the temporary as the operand.
       */
      if (this->predicate(ir->operands[operand])) {
	 ir_variable *var;
	 ir_assignment *assign;

	 var = new ir_variable(ir->operands[operand]->type, "flattening_tmp");
	 this->base_ir->insert_before(var);

	 assign = new ir_assignment(new ir_dereference(var),
				    ir->operands[operand],
				    NULL);
	 this->base_ir->insert_before(assign);

	 ir->operands[operand] = new ir_dereference(var);
      }
   }
}


void
ir_expression_flattening_visitor::visit(ir_swizzle *ir)
{
   ir->val->accept(this);
}


void
ir_expression_flattening_visitor::visit(ir_dereference *ir)
{
   if (ir->mode == ir_dereference::ir_reference_array) {
      ir->selector.array_index->accept(this);
   }
   ir->var->accept(this);
}

void
ir_expression_flattening_visitor::visit(ir_assignment *ir)
{
   ir->rhs->accept(this);
}


void
ir_expression_flattening_visitor::visit(ir_constant *ir)
{
   (void) ir;
}


void
ir_expression_flattening_visitor::visit(ir_call *ir)
{
   (void) ir;
}


void
ir_expression_flattening_visitor::visit(ir_return *ir)
{
   (void) ir;
}


void
ir_expression_flattening_visitor::visit(ir_if *ir)
{
   ir->condition->accept(this);

   do_expression_flattening(&ir->then_instructions, this->predicate);
   do_expression_flattening(&ir->else_instructions, this->predicate);
}
