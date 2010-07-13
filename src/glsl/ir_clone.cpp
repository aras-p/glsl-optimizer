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
extern "C" {
#include "hash_table.h"
}

/**
 * Duplicate an IR variable
 *
 * \note
 * This will probably be made \c virtual and moved to the base class
 * eventually.
 */
ir_variable *
ir_variable::clone(struct hash_table *ht) const
{
   void *ctx = talloc_parent(this);
   ir_variable *var = new(ctx) ir_variable(type, name);

   var->max_array_access = this->max_array_access;
   var->read_only = this->read_only;
   var->centroid = this->centroid;
   var->invariant = this->invariant;
   var->shader_in = this->shader_in;
   var->shader_out = this->shader_out;
   var->mode = this->mode;
   var->interpolation = this->interpolation;
   var->array_lvalue = this->array_lvalue;
   var->location = this->location;
   var->warn_extension = this->warn_extension;

   if (this->constant_value)
      var->constant_value = this->constant_value->clone(ht);

   if (ht) {
      hash_table_insert(ht, var, (void *)const_cast<ir_variable *>(this));
   }

   return var;
}

ir_swizzle *
ir_swizzle::clone(struct hash_table *ht) const
{
   void *ctx = talloc_parent(this);
   return new(ctx) ir_swizzle(this->val->clone(ht), this->mask);
}

ir_return *
ir_return::clone(struct hash_table *ht) const
{
   void *ctx = talloc_parent(this);
   ir_rvalue *new_value = NULL;

   if (this->value)
      new_value = this->value->clone(ht);

   return new(ctx) ir_return(new_value);
}

ir_discard *
ir_discard::clone(struct hash_table *ht) const
{
   void *ctx = talloc_parent(this);
   ir_rvalue *new_condition = NULL;

   if (this->condition != NULL)
      new_condition = this->condition->clone(ht);

   return new(ctx) ir_discard(new_condition);
}

ir_loop_jump *
ir_loop_jump::clone(struct hash_table *ht) const
{
   void *ctx = talloc_parent(this);
   (void)ht;

   return new(ctx) ir_loop_jump(this->mode);
}

ir_if *
ir_if::clone(struct hash_table *ht) const
{
   void *ctx = talloc_parent(this);
   ir_if *new_if = new(ctx) ir_if(this->condition->clone(ht));

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

ir_loop *
ir_loop::clone(struct hash_table *ht) const
{
   void *ctx = talloc_parent(this);
   ir_loop *new_loop = new(ctx) ir_loop();

   if (this->from)
      new_loop->from = this->from->clone(ht);
   if (this->to)
      new_loop->to = this->to->clone(ht);
   if (this->increment)
      new_loop->increment = this->increment->clone(ht);
   new_loop->counter = counter;

   foreach_iter(exec_list_iterator, iter, this->body_instructions) {
      ir_instruction *ir = (ir_instruction *)iter.get();
      new_loop->body_instructions.push_tail(ir->clone(ht));
   }

   return new_loop;
}

ir_call *
ir_call::clone(struct hash_table *ht) const
{
   void *ctx = talloc_parent(this);
   exec_list new_parameters;

   foreach_iter(exec_list_iterator, iter, this->actual_parameters) {
      ir_instruction *ir = (ir_instruction *)iter.get();
      new_parameters.push_tail(ir->clone(ht));
   }

   return new(ctx) ir_call(this->callee, &new_parameters);
}

ir_expression *
ir_expression::clone(struct hash_table *ht) const
{
   void *ctx = talloc_parent(this);
   ir_rvalue *op[2] = {NULL, NULL};
   unsigned int i;

   for (i = 0; i < get_num_operands(); i++) {
      op[i] = this->operands[i]->clone(ht);
   }

   return new(ctx) ir_expression(this->operation, this->type, op[0], op[1]);
}

ir_dereference_variable *
ir_dereference_variable::clone(struct hash_table *ht) const
{
   void *ctx = talloc_parent(this);
   ir_variable *new_var;

   if (ht) {
      new_var = (ir_variable *)hash_table_find(ht, this->var);
      if (!new_var)
	 new_var = this->var;
   } else {
      new_var = this->var;
   }

   return new(ctx) ir_dereference_variable(new_var);
}

ir_dereference_array *
ir_dereference_array::clone(struct hash_table *ht) const
{
   void *ctx = talloc_parent(this);
   return new(ctx) ir_dereference_array(this->array->clone(ht),
					this->array_index->clone(ht));
}

ir_dereference_record *
ir_dereference_record::clone(struct hash_table *ht) const
{
   void *ctx = talloc_parent(this);
   return new(ctx) ir_dereference_record(this->record->clone(ht),
					 this->field);
}

ir_texture *
ir_texture::clone(struct hash_table *ht) const
{
   void *ctx = talloc_parent(this);
   ir_texture *new_tex = new(ctx) ir_texture(this->op);

   new_tex->sampler = this->sampler->clone(ht);
   new_tex->coordinate = this->coordinate->clone(ht);
   if (this->projector)
      new_tex->projector = this->projector->clone(ht);
   if (this->shadow_comparitor) {
      new_tex->shadow_comparitor = this->shadow_comparitor->clone(ht);
   }

   for (int i = 0; i < 3; i++)
      new_tex->offsets[i] = this->offsets[i];

   switch (this->op) {
   case ir_tex:
      break;
   case ir_txb:
      new_tex->lod_info.bias = this->lod_info.bias->clone(ht);
      break;
   case ir_txl:
   case ir_txf:
      new_tex->lod_info.lod = this->lod_info.lod->clone(ht);
      break;
   case ir_txd:
      new_tex->lod_info.grad.dPdx = this->lod_info.grad.dPdx->clone(ht);
      new_tex->lod_info.grad.dPdy = this->lod_info.grad.dPdy->clone(ht);
      break;
   }

   return new_tex;
}

ir_assignment *
ir_assignment::clone(struct hash_table *ht) const
{
   ir_rvalue *new_condition = NULL;

   if (this->condition)
      new_condition = this->condition->clone(ht);

   void *ctx = talloc_parent(this);
   return new(ctx) ir_assignment(this->lhs->clone(ht),
				 this->rhs->clone(ht),
				 new_condition);
}

ir_function *
ir_function::clone(struct hash_table *ht) const
{
   void *mem_ctx = talloc_parent(this);
   ir_function *copy = new(mem_ctx) ir_function(this->name);

   foreach_list_const(node, &this->signatures) {
      const ir_function_signature *const sig =
	 (const ir_function_signature *const) node;

      ir_function_signature *sig_copy = sig->clone(ht);
      copy->add_signature(sig_copy);

      if (ht != NULL)
	 hash_table_insert(ht, sig_copy,
			   (void *)const_cast<ir_function_signature *>(sig));
   }

   return copy;
}

ir_function_signature *
ir_function_signature::clone(struct hash_table *ht) const
{
   void *mem_ctx = talloc_parent(this);
   ir_function_signature *copy =
      new(mem_ctx) ir_function_signature(this->return_type);

   copy->is_defined = this->is_defined;
   copy->is_built_in = this->is_built_in;

   /* Clone the parameter list.
    */
   foreach_list_const(node, &this->parameters) {
      const ir_variable *const param = (const ir_variable *) node;

      assert(const_cast<ir_variable *>(param)->as_variable() != NULL);

      ir_variable *const param_copy = param->clone(ht);
      copy->parameters.push_tail(param_copy);
   }

   /* Clone the instruction list.
    */
   foreach_list_const(node, &this->body) {
      const ir_instruction *const inst = (const ir_instruction *) node;

      ir_instruction *const inst_copy = inst->clone(ht);
      copy->body.push_tail(inst_copy);
   }

   return copy;
}

ir_constant *
ir_constant::clone(struct hash_table *ht) const
{
   void *ctx = talloc_parent(this);
   (void)ht;

   switch (this->type->base_type) {
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_BOOL:
      return new(ctx) ir_constant(this->type, &this->value);

   case GLSL_TYPE_STRUCT: {
      ir_constant *c = new(ctx) ir_constant;

      c->type = this->type;
      for (exec_node *node = this->components.head
	      ; !node->is_tail_sentinal()
	      ; node = node->next) {
	 ir_constant *const orig = (ir_constant *) node;

	 c->components.push_tail(orig->clone(NULL));
      }

      return c;
   }

   default:
      assert(!"Should not get here."); break;
      return NULL;
   }
}


class fixup_ir_call_visitor : public ir_hierarchical_visitor {
public:
   fixup_ir_call_visitor(struct hash_table *ht)
   {
      this->ht = ht;
   }

   virtual ir_visitor_status visit_enter(ir_call *ir)
   {
      /* Try to find the function signature referenced by the ir_call in the
       * table.  If it is found, replace it with the value from the table.
       */
      const ir_function_signature *const sig =
	 (ir_function_signature *) hash_table_find(this->ht, ir->get_callee());
      if (sig != NULL)
	 ir->set_callee(sig);

      /* Since this may be used before function call parameters are flattened,
       * the children also need to be processed.
       */
      return visit_continue;
   }

private:
   struct hash_table *ht;
};


static void
fixup_function_calls(struct hash_table *ht, exec_list *instructions)
{
   fixup_ir_call_visitor v(ht);
   v.run(instructions);
}


void
clone_ir_list(exec_list *out, const exec_list *in)
{
   struct hash_table *ht =
      hash_table_ctor(0, hash_table_pointer_hash, hash_table_pointer_compare);

   foreach_list_const(node, in) {
      const ir_instruction *const original = (ir_instruction *) node;
      ir_instruction *copy = original->clone(ht);

      out->push_tail(copy);
   }

   /* Make a pass over the cloned tree to fix up ir_call nodes to point to the
    * cloned ir_function_signature nodes.  This cannot be done automatically
    * during cloning because the ir_call might be a forward reference (i.e.,
    * the function signature that it references may not have been cloned yet).
    */
   fixup_function_calls(ht, out);

   hash_table_dtor(ht);
}
