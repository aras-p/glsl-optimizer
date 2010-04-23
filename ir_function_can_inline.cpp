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
 * \file ir_function_can_inline.cpp
 *
 * Determines if we can inline a function call using ir_function_inlining.cpp.
 *
 * The primary restriction is that we can't return from the function
 * other than as the last instruction.  We could potentially work
 * around this for some constructs by flattening control flow and
 * moving the return to the end, or by using breaks from a do {} while
 * (0) loop surrounding the function body.
 */

#define NULL 0
#include "ir.h"
#include "ir_visitor.h"
#include "ir_function_inlining.h"
#include "ir_expression_flattening.h"
#include "glsl_types.h"

class ir_function_can_inline_visitor : public ir_visitor {
public:
   ir_function_can_inline_visitor()
   {
      this->can_inline = true;
      this->num_returns = 0;
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
   virtual void visit(ir_label *);
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

   bool can_inline;
   int num_returns;
};

void
ir_function_can_inline_visitor::visit(ir_variable *ir)
{
   (void)ir;
}

void
ir_function_can_inline_visitor::visit(ir_label *ir)
{
   (void)ir;
}

void
ir_function_can_inline_visitor::visit(ir_loop *ir)
{
   /* FINISHME: Implement loop cloning in ir_function_inlining.cpp */
   this->can_inline = false;

   if (ir->from)
      ir->from->accept(this);
   if (ir->to)
      ir->to->accept(this);
   if (ir->increment)
      ir->increment->accept(this);

   foreach_iter(exec_list_iterator, iter, ir->body_instructions) {
      ir_instruction *inner_ir = (ir_instruction *)iter.get();
      inner_ir->accept(this);
   }
}

void
ir_function_can_inline_visitor::visit(ir_loop_jump *ir)
{
   (void) ir;
}


void
ir_function_can_inline_visitor::visit(ir_function_signature *ir)
{
   (void)ir;
}


void
ir_function_can_inline_visitor::visit(ir_function *ir)
{
   (void) ir;
}

void
ir_function_can_inline_visitor::visit(ir_expression *ir)
{
   unsigned int operand;

   for (operand = 0; operand < ir->get_num_operands(); operand++) {
      ir->operands[operand]->accept(this);
   }
}


void
ir_function_can_inline_visitor::visit(ir_swizzle *ir)
{
   ir->val->accept(this);
}

void
ir_function_can_inline_visitor::visit(ir_dereference *ir)
{
   ir->var->accept(this);
   if (ir->mode == ir_dereference::ir_reference_array)
      ir->selector.array_index->accept(this);
}

void
ir_function_can_inline_visitor::visit(ir_assignment *ir)
{
   ir->lhs->accept(this);
   ir->rhs->accept(this);
   if (ir->condition)
      ir->condition->accept(this);
}


void
ir_function_can_inline_visitor::visit(ir_constant *ir)
{
   (void)ir;
}


void
ir_function_can_inline_visitor::visit(ir_call *ir)
{
   foreach_iter(exec_list_iterator, iter, *ir) {
      ir_rvalue *param = (ir_rvalue *)iter.get();

      param->accept(this);
   }
}


void
ir_function_can_inline_visitor::visit(ir_return *ir)
{
   ir->get_value()->accept(this);

   this->num_returns++;
}


void
ir_function_can_inline_visitor::visit(ir_if *ir)
{
   /* FINISHME: Implement if cloning in ir_function_inlining.cpp. */
   this->can_inline = false;

   ir->condition->accept(this);

   foreach_iter(exec_list_iterator, iter, ir->then_instructions) {
      ir_instruction *inner_ir = (ir_instruction *)iter.get();
      inner_ir->accept(this);
   }

   foreach_iter(exec_list_iterator, iter, ir->else_instructions) {
      ir_instruction *inner_ir = (ir_instruction *)iter.get();
      inner_ir->accept(this);
   }
}

bool
can_inline(ir_call *call)
{
   ir_function_can_inline_visitor v;
   const ir_function_signature *callee = call->get_callee();

   foreach_iter(exec_list_iterator, iter, callee->body) {
      ir_instruction *ir = (ir_instruction *)iter.get();
      ir->accept(&v);
   }

   ir_instruction *last = (ir_instruction *)callee->body.get_tail();
   if (last && !last->as_return())
      v.num_returns++;

   return v.can_inline && v.num_returns == 1;
}
