/*
 * Copyright © 2014 Intel Corporation
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Connor Abbott (cwabbott0@gmail.com)
 *
 */

#include "nir.h"
#include "nir_builder.h"

/**
 * SSA-based copy propagation
 */

static bool
is_swizzleless_move(nir_alu_instr *instr)
{
   unsigned num_comp = instr->def.num_components;

   if (instr->src[0].src.ssa->num_components != num_comp)
      return false;

   if (instr->op == nir_op_mov) {
      for (unsigned i = 0; i < num_comp; i++) {
         if (instr->src[0].swizzle[i] != i)
            return false;
      }
   } else {
      for (unsigned i = 0; i < num_comp; i++) {
         if (instr->src[i].swizzle[0] != i ||
             instr->src[i].src.ssa != instr->src[0].src.ssa)
            return false;
      }
   }

   return true;
}

static void
merge_vec_and_mov(nir_alu_instr *mov, nir_alu_instr *vec)
{
   nir_builder b = nir_builder_at(nir_after_instr(&mov->instr));

   unsigned num_comp = mov->def.num_components;
   nir_alu_instr *new_vec = nir_alu_instr_create(b.shader, nir_op_vec(num_comp));
   for (unsigned i = 0; i < num_comp; i++)
      new_vec->src[i] = vec->src[mov->src[0].swizzle[i]];

   nir_def *new = nir_builder_alu_instr_finish_and_insert(&b, new_vec);
   nir_def_rewrite_uses(&mov->def, new);

   /* If we remove "mov" and it's the next instruction in the
    * nir_foreach_instr_safe() loop, then we would end copy-propagation early. */
}

static bool
copy_propagate_alu(nir_alu_src *use_of_copy, nir_alu_instr *copy)
{
   nir_def *new_use_src = NULL;
   nir_alu_instr *user = nir_instr_as_alu(nir_src_use_instr(&use_of_copy->src));
   unsigned src_idx = use_of_copy - user->src;
   assert(src_idx < nir_op_infos[user->op].num_inputs);
   unsigned num_use_components = nir_ssa_alu_instr_src_components(user, src_idx);

   if (copy->op == nir_op_mov) {
      new_use_src = copy->src[0].src.ssa;

      /* Propagate the mov swizzle to the use. */
      for (unsigned i = 0; i < num_use_components; i++)
         use_of_copy->swizzle[i] = copy->src[0].swizzle[use_of_copy->swizzle[i]];
   } else {
      /* "copy" is vecN. */
      new_use_src = copy->src[use_of_copy->swizzle[0]].src.ssa;

      for (unsigned i = 1; i < num_use_components; i++) {
         if (copy->src[use_of_copy->swizzle[i]].src.ssa != new_use_src) {
            if (user->op == nir_op_mov) {
               /* When vecN using different defs is followed by mov, they are
                * merged, which becomes a new vecN instruction.
                *
                * Ideally, mov would be merged into the preceding vecN, but
                * since the old vecN can have other uses, we create a new vecN.
                *
                * TODO: This ends up creating duplicated instructions if
                * the mov isn't CSE'd before this pass. It also leaves the mov
                * dead, and the original vecN can end up dead too.
                */
               merge_vec_and_mov(user, copy);
               return true;
            } else {
               /* If the use uses at least 2 components of vecN and those
                * components source at least 2 different defs, it's not
                * possible to propagate that vecN.
                */
               return false;
            }
         }
      }

      /* Propagate vecN (using the same def in all srcs, so it's equivalent to
       * a swizzled mov) to the use.
       */
      for (unsigned i = 0; i < num_use_components; i++)
         use_of_copy->swizzle[i] = copy->src[use_of_copy->swizzle[i]].swizzle[0];
   }

   nir_src_rewrite(&use_of_copy->src, new_use_src);

   return true;
}

static bool
copy_propagate(nir_src *use_of_copy, nir_alu_instr *copy)
{
   if (!is_swizzleless_move(copy))
      return false;

   nir_src_rewrite(use_of_copy, copy->src[0].src.ssa);

   return true;
}

static bool
copy_prop_instr(nir_instr *instr)
{
   if (instr->type != nir_instr_type_alu)
      return false;

   nir_alu_instr *copy = nir_instr_as_alu(instr);

   if (!nir_op_is_vec_or_mov(copy->op))
      return false;

   bool progress = false;

   nir_foreach_use_including_if_safe(src, &copy->def) {
      if (!nir_src_is_if(src) && nir_src_use_instr(src)->type == nir_instr_type_alu)
         progress |= copy_propagate_alu(container_of(src, nir_alu_src, src), copy);
      else
         progress |= copy_propagate(src, copy);
   }

   if (progress && nir_def_is_unused(&copy->def))
      nir_instr_remove(&copy->instr);

   return progress;
}

bool
nir_opt_copy_prop_impl(nir_function_impl *impl)
{
   bool progress = false;

   nir_foreach_block(block, impl) {
      nir_foreach_instr_safe(instr, block) {
         progress |= copy_prop_instr(instr);
      }
   }

   return nir_progress(progress, impl, nir_metadata_control_flow);
}

bool
nir_opt_copy_prop(nir_shader *shader)
{
   bool progress = false;

   nir_foreach_function_impl(impl, shader) {
      if (nir_opt_copy_prop_impl(impl))
         progress = true;
   }

   return progress;
}
