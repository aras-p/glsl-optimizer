/*
 * Copyright © 2013 Intel Corporation
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

#include "ir.h"

/**
 * Helper for checking equality when one instruction might be NULL, since you
 * can't access a's vtable in that case.
 */
static bool
possibly_null_equals(ir_instruction *a, ir_instruction *b)
{
   if (!a || !b)
      return !a && !b;

   return a->equals(b);
}

/**
 * The base equality function: Return not equal for anything we don't know
 * about.
 */
bool
ir_instruction::equals(ir_instruction *ir)
{
   return false;
}

bool
ir_constant::equals(ir_instruction *ir)
{
   const ir_constant *other = ir->as_constant();
   if (!other)
      return false;

   if (type != other->type)
      return false;

   for (unsigned i = 0; i < type->components(); i++) {
      if (value.u[i] != other->value.u[i])
         return false;
   }

   return true;
}

bool
ir_dereference_variable::equals(ir_instruction *ir)
{
   const ir_dereference_variable *other = ir->as_dereference_variable();
   if (!other)
      return false;

   return var == other->var;
}

bool
ir_dereference_array::equals(ir_instruction *ir)
{
   const ir_dereference_array *other = ir->as_dereference_array();
   if (!other)
      return false;

   if (type != other->type)
      return false;

   if (!array->equals(other->array))
      return false;

   if (!array_index->equals(other->array_index))
      return false;

   return true;
}

bool
ir_swizzle::equals(ir_instruction *ir)
{
   const ir_swizzle *other = ir->as_swizzle();
   if (!other)
      return false;

   if (type != other->type)
      return false;

   if (mask.x != other->mask.x ||
       mask.y != other->mask.y ||
       mask.z != other->mask.z ||
       mask.w != other->mask.w) {
      return false;
   }

   return val->equals(other->val);
}

bool
ir_texture::equals(ir_instruction *ir)
{
   const ir_texture *other = ir->as_texture();
   if (!other)
      return false;

   if (type != other->type)
      return false;

   if (op != other->op)
      return false;

   if (!possibly_null_equals(coordinate, other->coordinate))
      return false;

#if 0 // Note: glsl optimizer removed projector & shadow_comparitor fields
   if (!possibly_null_equals(projector, other->projector))
      return false;

   if (!possibly_null_equals(shadow_comparitor, other->shadow_comparitor))
      return false;
#endif

   if (!possibly_null_equals(offset, other->offset))
      return false;

   if (!sampler->equals(other->sampler))
      return false;

   switch (op) {
   case ir_tex:
   case ir_lod:
   case ir_query_levels:
      break;
   case ir_txb:
      if (!lod_info.bias->equals(other->lod_info.bias))
         return false;
      break;
   case ir_txl:
   case ir_txf:
   case ir_txs:
      if (!lod_info.lod->equals(other->lod_info.lod))
         return false;
      break;
   case ir_txd:
      if (!lod_info.grad.dPdx->equals(other->lod_info.grad.dPdx) ||
          !lod_info.grad.dPdy->equals(other->lod_info.grad.dPdy))
         return false;
      break;
   case ir_txf_ms:
      if (!lod_info.sample_index->equals(other->lod_info.sample_index))
         return false;
      break;
   case ir_tg4:
      if (!lod_info.component->equals(other->lod_info.component))
         return false;
      break;
   default:
      assert(!"Unrecognized texture op");
   }

   return true;
}

bool
ir_expression::equals(ir_instruction *ir)
{
   const ir_expression *other = ir->as_expression();
   if (!other)
      return false;

   if (type != other->type)
      return false;

   if (operation != other->operation)
      return false;

   for (unsigned i = 0; i < get_num_operands(); i++) {
      if (!operands[i]->equals(other->operands[i]))
         return false;
   }

   return true;
}
