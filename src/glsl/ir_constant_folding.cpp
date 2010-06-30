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
 * \file ir_constant_folding.cpp
 * Replace constant-valued expressions with references to constant values.
 */

#include "ir.h"
#include "ir_visitor.h"
#include "ir_optimization.h"
#include "glsl_types.h"

/**
 * Visitor class for replacing expressions with ir_constant values.
 */

class ir_constant_folding_visitor : public ir_visitor {
public:
   ir_constant_folding_visitor()
   {
      /* empty */
   }

   virtual ~ir_constant_folding_visitor()
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
   virtual void visit(ir_function_signature *);
   virtual void visit(ir_function *);
   virtual void visit(ir_expression *);
   virtual void visit(ir_texture *);
   virtual void visit(ir_swizzle *);
   virtual void visit(ir_dereference_variable *);
   virtual void visit(ir_dereference_array *);
   virtual void visit(ir_dereference_record *);
   virtual void visit(ir_assignment *);
   virtual void visit(ir_constant *);
   virtual void visit(ir_call *);
   virtual void visit(ir_return *);
   virtual void visit(ir_discard *);
   virtual void visit(ir_if *);
   virtual void visit(ir_loop *);
   virtual void visit(ir_loop_jump *);
   /*@}*/
};

void
ir_constant_folding_visitor::visit(ir_variable *ir)
{
   (void) ir;
}


void
ir_constant_folding_visitor::visit(ir_function_signature *ir)
{
   visit_exec_list(&ir->body, this);
}


void
ir_constant_folding_visitor::visit(ir_function *ir)
{
   foreach_iter(exec_list_iterator, iter, *ir) {
      ir_function_signature *const sig = (ir_function_signature *) iter.get();
      sig->accept(this);
   }
}

void
ir_constant_folding_visitor::visit(ir_expression *ir)
{
   ir_constant *op[2];
   unsigned int operand;

   for (operand = 0; operand < ir->get_num_operands(); operand++) {
      op[operand] = ir->operands[operand]->constant_expression_value();
      if (op[operand]) {
	 ir->operands[operand] = op[operand];
      } else {
	 ir->operands[operand]->accept(this);
      }
   }
}


void
ir_constant_folding_visitor::visit(ir_texture *ir)
{
   // FINISHME: Do stuff with texture lookups
   (void) ir;
}


void
ir_constant_folding_visitor::visit(ir_swizzle *ir)
{
   ir->val->accept(this);
}


void
ir_constant_folding_visitor::visit(ir_dereference_variable *ir)
{
   (void) ir;
}


void
ir_constant_folding_visitor::visit(ir_dereference_array *ir)
{
   ir_constant *const_val =
      ir->array_index->constant_expression_value();

   if (const_val)
      ir->array_index = const_val;
   else
      ir->array_index->accept(this);

   ir->array->accept(this);
}


void
ir_constant_folding_visitor::visit(ir_dereference_record *ir)
{
   ir->record->accept(this);
}


void
ir_constant_folding_visitor::visit(ir_assignment *ir)
{
   ir_constant *const_val = ir->rhs->constant_expression_value();
   if (const_val)
      ir->rhs = const_val;
   else
      ir->rhs->accept(this);
}


void
ir_constant_folding_visitor::visit(ir_constant *ir)
{
   (void) ir;
}


void
ir_constant_folding_visitor::visit(ir_call *ir)
{
   (void) ir;
}


void
ir_constant_folding_visitor::visit(ir_return *ir)
{
   (void) ir;
}


void
ir_constant_folding_visitor::visit(ir_discard *ir)
{
   (void) ir;
}


void
ir_constant_folding_visitor::visit(ir_if *ir)
{
   ir_constant *const_val = ir->condition->constant_expression_value();
   if (const_val)
      ir->condition = const_val;
   else
      ir->condition->accept(this);

   visit_exec_list(&ir->then_instructions, this);
   visit_exec_list(&ir->else_instructions, this);
}


void
ir_constant_folding_visitor::visit(ir_loop *ir)
{
   (void) ir;
}


void
ir_constant_folding_visitor::visit(ir_loop_jump *ir)
{
   (void) ir;
}

bool
do_constant_folding(exec_list *instructions)
{
   ir_constant_folding_visitor constant_folding;

   visit_exec_list(instructions, &constant_folding);

   /* FINISHME: Return real progress. */
   return false;
}
