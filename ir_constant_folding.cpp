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

#define NULL 0
#include "ir.h"
#include "ir_visitor.h"
#include "ir_constant_folding.h"
#include "glsl_types.h"

/**
 * Visitor class for replacing expressions with ir_constant values.
 */

void
ir_constant_folding_visitor::visit(ir_variable *ir)
{
   (void) ir;
}


void
ir_constant_folding_visitor::visit(ir_label *ir)
{
   ir->signature->accept(this);
}


void
ir_constant_folding_visitor::visit(ir_function_signature *ir)
{
   visit_exec_list(&ir->body, this);
}


void
ir_constant_folding_visitor::visit(ir_function *ir)
{
   (void) ir;
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
ir_constant_folding_visitor::visit(ir_swizzle *ir)
{
   ir->val->accept(this);
}


void
ir_constant_folding_visitor::visit(ir_dereference *ir)
{
   if (ir->mode == ir_dereference::ir_reference_array) {
      ir_constant *const_val = ir->selector.array_index->constant_expression_value();
      if (const_val)
	 ir->selector.array_index = const_val;
      else
	 ir->selector.array_index->accept(this);
   }
   ir->var->accept(this);
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
