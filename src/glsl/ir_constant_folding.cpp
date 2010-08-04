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
      this->progress = false;
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

   void fold_constant(ir_rvalue **rvalue);

   bool progress;
};

void
ir_constant_folding_visitor::fold_constant(ir_rvalue **rvalue)
{
   if (*rvalue == NULL || (*rvalue)->ir_type == ir_type_constant)
      return;

   ir_constant *constant = (*rvalue)->constant_expression_value();
   if (constant) {
      *rvalue = constant;
      this->progress = true;
   } else {
      (*rvalue)->accept(this);
   }
}

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
   unsigned int operand;

   for (operand = 0; operand < ir->get_num_operands(); operand++) {
      fold_constant(&ir->operands[operand]);
   }
}


void
ir_constant_folding_visitor::visit(ir_texture *ir)
{
   fold_constant(&ir->coordinate);
   fold_constant(&ir->projector);
   fold_constant(&ir->shadow_comparitor);

   switch (ir->op) {
   case ir_tex:
      break;
   case ir_txb:
      fold_constant(&ir->lod_info.bias);
      break;
   case ir_txf:
   case ir_txl:
      fold_constant(&ir->lod_info.lod);
      break;
   case ir_txd:
      fold_constant(&ir->lod_info.grad.dPdx);
      fold_constant(&ir->lod_info.grad.dPdy);
      break;
   }
}


void
ir_constant_folding_visitor::visit(ir_swizzle *ir)
{
   fold_constant(&ir->val);
}


void
ir_constant_folding_visitor::visit(ir_dereference_variable *ir)
{
   (void) ir;
}


void
ir_constant_folding_visitor::visit(ir_dereference_array *ir)
{
   fold_constant(&ir->array_index);
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
   fold_constant(&ir->rhs);

   if (ir->condition) {
      /* If the condition is constant, either remove the condition or
       * remove the never-executed assignment.
       */
      ir_constant *const_val = ir->condition->constant_expression_value();
      if (const_val) {
	 if (const_val->value.b[0])
	    ir->condition = NULL;
	 else
	    ir->remove();
	 this->progress = true;
      }
   }
}


void
ir_constant_folding_visitor::visit(ir_constant *ir)
{
   (void) ir;
}


void
ir_constant_folding_visitor::visit(ir_call *ir)
{
   foreach_iter(exec_list_iterator, iter, *ir) {
      ir_rvalue *param = (ir_rvalue *)iter.get();
      ir_rvalue *new_param = param;
      fold_constant(&new_param);

      if (new_param != param) {
	 param->replace_with(new_param);
      }
   }
}


void
ir_constant_folding_visitor::visit(ir_return *ir)
{
   fold_constant(&ir->value);
}


void
ir_constant_folding_visitor::visit(ir_discard *ir)
{
   (void) ir;
}


void
ir_constant_folding_visitor::visit(ir_if *ir)
{
   fold_constant(&ir->condition);

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

   return constant_folding.progress;
}
