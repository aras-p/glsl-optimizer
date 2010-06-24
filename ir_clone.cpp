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

#include <string.h>
#include "ir.h"
#include "glsl_types.h"
#include "hash_table.h"

/**
 * Duplicate an IR variable
 *
 * \note
 * This will probably be made \c virtual and moved to the base class
 * eventually.
 */
ir_instruction *
ir_variable::clone(struct hash_table *ht) const
{
   ir_variable *var = new ir_variable(type, name);

   var->max_array_access = this->max_array_access;
   var->read_only = this->read_only;
   var->centroid = this->centroid;
   var->invariant = this->invariant;
   var->mode = this->mode;
   var->interpolation = this->interpolation;

   if (ht) {
      hash_table_insert(ht, var, (void *)const_cast<ir_variable *>(this));
   }

   return var;
}

ir_instruction *
ir_swizzle::clone(struct hash_table *ht) const
{
   return new ir_swizzle((ir_rvalue *)this->val->clone(ht), this->mask);
}

ir_instruction *
ir_return::clone(struct hash_table *ht) const
{
   ir_rvalue *new_value = NULL;

   if (this->value)
      new_value = (ir_rvalue *)this->value->clone(ht);

   return new ir_return(new_value);
}

ir_instruction *
ir_loop_jump::clone(struct hash_table *ht) const
{
   (void)ht;

   return new ir_loop_jump(this->mode);
}

ir_instruction *
ir_if::clone(struct hash_table *ht) const
{
   ir_if *new_if = new ir_if((ir_rvalue *)this->condition->clone(ht));

   foreach_iter(exec_list_iterator, iter, this->then_instructions) {
      ir_instruction *ir = (ir_instruction *)iter.get();
      new_if->then_instructions.push_tail(ir->clone(ht));
   }

   foreach_iter(exec_list_iterator, iter, this->else_instructions) {
      ir_instruction *ir = (ir_instruction *)iter.get();
      new_if->else_instructions.push_tail(ir->clone(ht));
   }

   return new_if;
}

ir_instruction *
ir_loop::clone(struct hash_table *ht) const
{
   ir_loop *new_loop = new ir_loop();

   if (this->from)
      new_loop->from = (ir_rvalue *)this->from->clone(ht);
   if (this->to)
      new_loop->to = (ir_rvalue *)this->to->clone(ht);
   if (this->increment)
      new_loop->increment = (ir_rvalue *)this->increment->clone(ht);
   new_loop->counter = counter;

   foreach_iter(exec_list_iterator, iter, this->body_instructions) {
      ir_instruction *ir = (ir_instruction *)iter.get();
      new_loop->body_instructions.push_tail(ir->clone(ht));
   }

   return new_loop;
}

ir_instruction *
ir_call::clone(struct hash_table *ht) const
{
   exec_list new_parameters;

   foreach_iter(exec_list_iterator, iter, this->actual_parameters) {
      ir_instruction *ir = (ir_instruction *)iter.get();
      new_parameters.push_tail(ir->clone(ht));
   }

   return new ir_call(this->callee, &new_parameters);
}

ir_instruction *
ir_expression::clone(struct hash_table *ht) const
{
   ir_rvalue *op[2] = {NULL, NULL};
   unsigned int i;

   for (i = 0; i < get_num_operands(); i++) {
      op[i] = (ir_rvalue *)this->operands[i]->clone(ht);
   }

   return new ir_expression(this->operation, this->type, op[0], op[1]);
}

ir_instruction *
ir_dereference_variable::clone(struct hash_table *ht) const
{
   ir_variable *new_var;

   if (ht) {
      new_var = (ir_variable *)hash_table_find(ht, this->var);
      if (!new_var)
	 new_var = this->var;
   } else {
      new_var = this->var;
   }

   return new ir_dereference_variable(new_var);
}

ir_instruction *
ir_dereference_array::clone(struct hash_table *ht) const
{
   return new ir_dereference_array((ir_rvalue *)this->array->clone(ht),
				   (ir_rvalue *)this->array_index->clone(ht));
}

ir_instruction *
ir_dereference_record::clone(struct hash_table *ht) const
{
   return new ir_dereference_record((ir_rvalue *)this->record->clone(ht),
				    this->field);
}

ir_instruction *
ir_texture::clone(struct hash_table *ht) const
{
   ir_texture *new_tex = new ir_texture(this->op);

   new_tex->sampler = (ir_dereference *)this->sampler->clone(ht);
   new_tex->coordinate = (ir_rvalue *)this->coordinate->clone(ht);
   if (this->projector)
      new_tex->projector = (ir_rvalue *)this->projector->clone(ht);
   if (this->shadow_comparitor) {
      new_tex->shadow_comparitor =
	 (ir_rvalue *)this->shadow_comparitor->clone(ht);
   }

   for (int i = 0; i < 3; i++)
      new_tex->offsets[i] = this->offsets[i];

   switch (this->op) {
   case ir_tex:
      break;
   case ir_txb:
      new_tex->lod_info.bias = (ir_rvalue *)this->lod_info.bias->clone(ht);
      break;
   case ir_txl:
   case ir_txf:
      new_tex->lod_info.lod = (ir_rvalue *)this->lod_info.lod->clone(ht);
      break;
   case ir_txd:
      new_tex->lod_info.grad.dPdx =
	 (ir_rvalue *)this->lod_info.grad.dPdx->clone(ht);
      new_tex->lod_info.grad.dPdy =
	 (ir_rvalue *)this->lod_info.grad.dPdy->clone(ht);
      break;
   }

   return new_tex;
}

ir_instruction *
ir_assignment::clone(struct hash_table *ht) const
{
   ir_rvalue *new_condition = NULL;

   if (this->condition)
      new_condition = (ir_rvalue *)this->condition->clone(ht);

   return new ir_assignment((ir_rvalue *)this->lhs->clone(ht),
			    (ir_rvalue *)this->rhs->clone(ht),
			    new_condition);
}

ir_instruction *
ir_function::clone(struct hash_table *ht) const
{
   (void)ht;
   /* FINISHME */
   abort();
}

ir_instruction *
ir_function_signature::clone(struct hash_table *ht) const
{
   (void)ht;
   /* FINISHME */
   abort();
}
