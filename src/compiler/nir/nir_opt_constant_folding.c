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
 */

#include <math.h>
#include "nir.h"
#include "nir_builder.h"
#include "nir_constant_expressions.h"
#include "nir_deref.h"

/*
 * Implements SSA-based constant folding.
 */

struct constant_fold_state {
   bool has_load_constant;
   bool has_indirect_load_const;
};

static nir_def *
const_value_for_alu(nir_builder *b, nir_alu_instr *alu, unsigned bit_size,
                    nir_const_value src[NIR_ALU_MAX_INPUTS][NIR_MAX_VEC_COMPONENTS])
{
   nir_const_value dest[NIR_MAX_VEC_COMPONENTS];
   nir_const_value *srcs[NIR_ALU_MAX_INPUTS];

   memset(dest, 0, sizeof(dest));
   for (unsigned i = 0; i < nir_op_infos[alu->op].num_inputs; ++i)
      srcs[i] = src[i];

   nir_eval_const_opcode(alu->op, dest, NULL, alu->def.num_components,
                         bit_size, srcs,
                         b->shader->info.float_controls_execution_mode);

   return nir_build_imm(b, alu->def.num_components, alu->def.bit_size,
                        dest);
}

static bool
is_bcsel_with_two_constants(nir_alu_instr *bcsel)
{
   return bcsel && bcsel->op == nir_op_bcsel &&
          bcsel->def.num_components == 1 &&
          bcsel->src[0].swizzle[0] == 0 &&
          bcsel->src[0].src.ssa->num_components == 1 &&
          nir_src_is_const(bcsel->src[1].src) &&
          nir_src_is_const(bcsel->src[2].src);
}

static bool
should_fold_bcsel(nir_alu_instr *alu)
{
   /* Don't fold bcsel if the resulting bit size is larger than 32 bit
    * as these commonly require two instructions.
    */
   if (alu->def.bit_size > 32)
      return false;

   /* Don't fight with nir_lower_bit_size. */
   if (alu->op == nir_op_u2u8 || alu->op == nir_op_i2i8 ||
       alu->op == nir_op_u2u16 || alu->op == nir_op_i2i16 ||
       alu->op == nir_op_u2u32 || alu->op == nir_op_i2i32) {
      return false;
   }

   /* Don't fight with nir_lower_load_const_to_scalar. */
   if (nir_op_is_vec_or_mov(alu->op))
      return false;

   /* Make an exception for fneg, because in many cases it can be
    * folded with the next instruction.
    */
   return alu->op != nir_op_fneg;
}

nir_def *
nir_try_constant_fold_alu(nir_builder *b, nir_alu_instr *alu)
{
   nir_const_value src[NIR_ALU_MAX_INPUTS][NIR_MAX_VEC_COMPONENTS];
   nir_def *bcsel = NULL;

   /* In the case that any outputs/inputs have unsized types, then we need to
    * guess the bit-size. In this case, the validator ensures that all
    * bit-sizes match so we can just take the bit-size from first
    * output/input with an unsized type. If all the outputs/inputs are sized
    * then we don't need to guess the bit-size at all because the code we
    * generate for constant opcodes in this case already knows the sizes of
    * the types involved and does not need the provided bit-size for anything
    * (although it still requires to receive a valid bit-size).
    */
   unsigned bit_size = 0;
   if (!nir_alu_type_get_type_size(nir_op_infos[alu->op].output_type))
      bit_size = alu->def.bit_size;

   for (unsigned i = 0; i < nir_op_infos[alu->op].num_inputs; i++) {
      if (bit_size == 0 &&
          !nir_alu_type_get_type_size(nir_op_infos[alu->op].input_types[i]))
         bit_size = alu->src[i].src.ssa->bit_size;

      nir_load_const_instr *load_const = nir_src_as_load_const(alu->src[i].src);

      if (load_const) {
         for (unsigned j = 0; j < nir_ssa_alu_instr_src_components(alu, i); j++)
            src[i][j] = load_const->value[alu->src[i].swizzle[j]];
         continue;
      }

      /* Check if the source is a bcsel with two constants. */
      nir_alu_instr *bcsel_alu = nir_src_as_alu(alu->src[i].src);
      if (should_fold_bcsel(alu) && is_bcsel_with_two_constants(bcsel_alu)) {
         /* If there is multiple bcsel sources, they must use the same condition. */
         if (bcsel && bcsel_alu->src[0].src.ssa != bcsel)
            return false;

         bcsel = bcsel_alu->src[0].src.ssa;

         /* Use first bcsel constant. */
         load_const = nir_src_as_load_const(bcsel_alu->src[1].src);
         for (unsigned j = 0; j < nir_ssa_alu_instr_src_components(alu, i); j++)
            src[i][j] = load_const->value[bcsel_alu->src[1].swizzle[alu->src[i].swizzle[j]]];
      } else {
         return NULL;
      }
   }

   if (bit_size == 0)
      bit_size = 32;

   /* If all sources are constant, we can fold the ALU. */
   if (!bcsel)
      return const_value_for_alu(b, alu, bit_size, src);

   /* At least one source is a bcsel with two constants. Fold the ALU twice
    * and create a new bcsel, selecting between the folded values.
    */
   nir_def *then_const = const_value_for_alu(b, alu, bit_size, src);

   /* Create second bcsel constant. */
   for (unsigned i = 0; i < nir_op_infos[alu->op].num_inputs; i++) {
      nir_alu_instr *bcsel_alu = nir_src_as_alu(alu->src[i].src);
      if (!bcsel_alu)
         continue;

      nir_load_const_instr *load_const = nir_src_as_load_const(bcsel_alu->src[2].src);
      for (unsigned j = 0; j < nir_ssa_alu_instr_src_components(alu, i); j++) {
         src[i][j] = load_const->value[bcsel_alu->src[2].swizzle[alu->src[i].swizzle[j]]];
      }
   }
   nir_def *else_const = const_value_for_alu(b, alu, bit_size, src);

   return nir_bcsel(b, bcsel, then_const, else_const);
}

static nir_const_value *
const_value_for_deref(nir_deref_instr *deref)
{
   if (!nir_deref_mode_is(deref, nir_var_mem_constant))
      return NULL;

   nir_deref_path path;
   nir_deref_path_init(&path, deref, NULL);
   if (path.path[0]->deref_type != nir_deref_type_var)
      goto fail;

   nir_variable *var = path.path[0]->var;
   assert(var->data.mode == nir_var_mem_constant);
   if (var->constant_initializer == NULL)
      goto fail;

   if (var->constant_initializer->is_null_constant) {
      /* Doesn't matter what casts are in the way, it's all zeros */
      nir_deref_path_finish(&path);
      return var->constant_initializer->values;
   }

   nir_constant *c = var->constant_initializer;
   nir_const_value *v = NULL; /* Vector value for array-deref-of-vec */

   for (unsigned i = 1; path.path[i] != NULL; i++) {
      nir_deref_instr *p = path.path[i];
      switch (p->deref_type) {
      case nir_deref_type_var:
         UNREACHABLE("Deref paths can only start with a var deref");

      case nir_deref_type_array: {
         assert(v == NULL);
         if (!nir_src_is_const(p->arr.index))
            goto fail;

         uint64_t idx = nir_src_as_uint(p->arr.index);
         if (c->num_elements > 0) {
            assert(glsl_type_is_array(path.path[i - 1]->type));
            if (idx >= c->num_elements)
               goto fail;
            c = c->elements[idx];
         } else {
            assert(glsl_type_is_vector(path.path[i - 1]->type));
            assert(glsl_type_is_scalar(p->type));
            if (idx >= NIR_MAX_VEC_COMPONENTS)
               goto fail;
            v = &c->values[idx];
         }
         break;
      }

      case nir_deref_type_struct:
         assert(glsl_type_is_struct(path.path[i - 1]->type));
         assert(v == NULL && c->num_elements > 0);
         if (p->strct.index >= c->num_elements)
            goto fail;
         c = c->elements[p->strct.index];
         break;

      default:
         goto fail;
      }
   }

   /* We have to have ended at a vector */
   assert(c->num_elements == 0);
   nir_deref_path_finish(&path);
   return v ? v : c->values;

fail:
   nir_deref_path_finish(&path);
   return NULL;
}

static bool
is_dual_slot_io(nir_intrinsic_instr *intrin)
{
   if (intrin->intrinsic == nir_intrinsic_store_output ||
       intrin->intrinsic == nir_intrinsic_store_per_vertex_output ||
       intrin->intrinsic == nir_intrinsic_store_per_view_output ||
       intrin->intrinsic == nir_intrinsic_store_per_primitive_output) {
      return nir_src_bit_size(intrin->src[0]) == 64 &&
             nir_src_num_components(intrin->src[0]) >= 3;
   }

   return intrin->def.bit_size == 64 &&
          intrin->def.num_components >= 3;
}

static nir_def *
try_fold_intrinsic(nir_builder *b, nir_intrinsic_instr *intrin,
                   struct constant_fold_state *state)
{
   switch (intrin->intrinsic) {
   case nir_intrinsic_demote_if:
   case nir_intrinsic_terminate_if:
      if (nir_src_is_const(intrin->src[0])) {
         if (nir_src_as_bool(intrin->src[0])) {
            nir_intrinsic_op op;
            switch (intrin->intrinsic) {
            case nir_intrinsic_demote_if:
               op = nir_intrinsic_demote;
               break;
            case nir_intrinsic_terminate_if:
               op = nir_intrinsic_terminate;
               break;
            default:
               UNREACHABLE("invalid intrinsic");
            }
            nir_intrinsic_instr *new_instr =
               nir_intrinsic_instr_create(b->shader, op);
            nir_builder_instr_insert(b, &new_instr->instr);
         }
         return NIR_LOWER_INSTR_PROGRESS_REPLACE;
      }
      return NULL;

   case nir_intrinsic_load_deref: {
      nir_deref_instr *deref = nir_src_as_deref(intrin->src[0]);
      nir_const_value *v = const_value_for_deref(deref);
      if (v) {
         return nir_build_imm(b, intrin->def.num_components,
                              intrin->def.bit_size, v);
      }
      return NULL;
   }

   case nir_intrinsic_load_constant: {
      state->has_load_constant = true;

      if (!nir_src_is_const(intrin->src[0])) {
         state->has_indirect_load_const = true;
         return NULL;
      }

      unsigned offset = nir_src_as_uint(intrin->src[0]);
      unsigned base = nir_intrinsic_base(intrin);
      unsigned range = nir_intrinsic_range(intrin);
      assert(base + range <= b->shader->constant_data_size);

      nir_def *val;
      if (offset >= range) {
         val = nir_undef(b, intrin->def.num_components,
                         intrin->def.bit_size);
      } else {
         nir_const_value imm[NIR_MAX_VEC_COMPONENTS];
         memset(imm, 0, sizeof(imm));
         uint8_t *data = (uint8_t *)b->shader->constant_data + base;
         for (unsigned i = 0; i < intrin->num_components; i++) {
            unsigned bytes = intrin->def.bit_size / 8;
            bytes = MIN2(bytes, range - offset);

            memcpy(&imm[i].u64, data + offset, bytes);
            offset += bytes;
         }
         val = nir_build_imm(b, intrin->def.num_components,
                             intrin->def.bit_size, imm);
      }
      return val;
   }

   case nir_intrinsic_ddx:
   case nir_intrinsic_ddx_fine:
   case nir_intrinsic_ddx_coarse:
   case nir_intrinsic_ddy:
   case nir_intrinsic_ddy_fine:
   case nir_intrinsic_ddy_coarse: {
      if (!nir_src_is_const(intrin->src[0]))
         return NULL;

      /* Derivative of a constant is zero, except for NaNs and Infs */
      nir_const_value imm[NIR_MAX_VEC_COMPONENTS];
      unsigned sz = intrin->def.bit_size;

      for (unsigned i = 0; i < intrin->def.num_components; i++) {
         bool finite = isfinite(nir_src_comp_as_float(intrin->src[0], i));
         imm[i] = nir_const_value_for_float(finite ? 0 : NAN, sz);
      }

      return nir_build_imm(b, intrin->def.num_components, sz, imm);
   }

   case nir_intrinsic_vote_any:
   case nir_intrinsic_vote_all:
   case nir_intrinsic_read_invocation:
   case nir_intrinsic_read_first_invocation:
   case nir_intrinsic_as_uniform:
   case nir_intrinsic_shuffle:
   case nir_intrinsic_shuffle_xor:
   case nir_intrinsic_shuffle_up:
   case nir_intrinsic_shuffle_down:
   case nir_intrinsic_quad_broadcast:
   case nir_intrinsic_quad_swap_horizontal:
   case nir_intrinsic_quad_swap_vertical:
   case nir_intrinsic_quad_swap_diagonal:
   case nir_intrinsic_quad_swizzle_amd:
   case nir_intrinsic_masked_swizzle_amd:
      /* All of these have the data payload in the first source.  They may
       * have a second source with a shuffle index but that doesn't matter if
       * the data is constant.
       */
      if (nir_src_is_const(intrin->src[0])) {
         return intrin->src[0].ssa;
      }
      return NULL;

   case nir_intrinsic_vote_feq:
   case nir_intrinsic_vote_ieq:
      if (nir_src_is_const(intrin->src[0])) {
         return nir_imm_true(b);
      }
      return NULL;

   case nir_intrinsic_inverse_ballot: {
      if (!nir_src_is_const(intrin->src[0]))
         return NULL;
      bool constant_true = true;
      bool constant_false = true;
      for (unsigned i = 0; i < nir_src_num_components(intrin->src[0]); i++) {
         int64_t value = nir_src_comp_as_int(intrin->src[0], i);
         constant_true &= value == -1;
         constant_false &= value == 0;
      }
      if (!constant_true && !constant_false)
         return NULL;

      return nir_imm_bool(b, constant_true);
   }

   case nir_intrinsic_ballot_relaxed:
   case nir_intrinsic_ballot: {
      if (!nir_src_is_const(intrin->src[0]) || nir_src_as_bool(intrin->src[0]))
         return NULL;

      return nir_imm_zero(b, intrin->def.num_components, intrin->def.bit_size);
   }

   case nir_intrinsic_load_input:
   case nir_intrinsic_load_per_primitive_input:
   case nir_intrinsic_load_input_vertex:
   case nir_intrinsic_load_per_vertex_input:
   case nir_intrinsic_load_interpolated_input:
   case nir_intrinsic_load_fs_input_interp_deltas:
   case nir_intrinsic_load_output:
   case nir_intrinsic_load_per_vertex_output:
   case nir_intrinsic_load_per_view_output:
   case nir_intrinsic_load_per_primitive_output:
   case nir_intrinsic_store_output:
   case nir_intrinsic_store_per_vertex_output:
   case nir_intrinsic_store_per_view_output:
   case nir_intrinsic_store_per_primitive_output: {
      if (nir_is_input_load(intrin) ?
          b->shader->info.disable_input_offset_src_constant_folding :
          b->shader->info.disable_output_offset_src_constant_folding)
         return NULL;

      nir_io_semantics sem = nir_intrinsic_io_semantics(intrin);

      /* NV_mesh_shader: ignore MS primitive indices. */
      if (b->shader->info.stage == MESA_SHADER_MESH &&
          sem.location == VARYING_SLOT_PRIMITIVE_INDICES &&
          !(b->shader->info.per_primitive_outputs &
            VARYING_BIT_PRIMITIVE_INDICES))
         return NULL;

      nir_src *offset = nir_get_io_offset_src(intrin);

      /* TODO: Better handling of per-view variables here */
      if (!nir_src_is_const(*offset) ||
          nir_intrinsic_io_semantics(intrin).per_view)
         return NULL;

      unsigned off = nir_src_as_uint(*offset);
      bool progress = false;

      if (off) {
         nir_intrinsic_set_base(intrin, nir_intrinsic_base(intrin) + off);

         sem.location += off;
         b->cursor = nir_before_instr(&intrin->instr);
         nir_src_rewrite(offset, nir_imm_int(b, 0));
         progress = true;
      }

      /* non-indirect indexing should reduce num_slots */
      sem.num_slots = is_dual_slot_io(intrin) ? 2 : 1;

      nir_io_semantics original = nir_intrinsic_io_semantics(intrin);
      progress |= memcmp(&original, &sem, sizeof(sem));
      nir_intrinsic_set_io_semantics(intrin, sem);
      return progress ? NIR_LOWER_INSTR_PROGRESS : NULL;
   }

   default:
      return NULL;
   }
}

static bool
try_fold_txb_to_tex(nir_builder *b, nir_tex_instr *tex)
{
   assert(tex->op == nir_texop_txb);

   const int bias_idx = nir_tex_instr_src_index(tex, nir_tex_src_bias);

   /* nir_to_tgsi_lower_tex mangles many kinds of texture instructions,
    * including txb, into invalid states.  It removes the special
    * parameters and appends the values to the texture coordinate.
    */
   if (bias_idx < 0)
      return false;

   if (nir_src_is_const(tex->src[bias_idx].src) &&
       nir_src_as_float(tex->src[bias_idx].src) == 0.0) {
      nir_tex_instr_remove_src(tex, bias_idx);
      tex->op = nir_texop_tex;
      return true;
   }

   return false;
}

static bool
try_fold_txd_to_txl(nir_builder *b, nir_tex_instr *tex)
{
   assert(tex->op == nir_texop_txd);

   const int ddx_idx = nir_tex_instr_src_index(tex, nir_tex_src_ddx);
   const int ddy_idx = nir_tex_instr_src_index(tex, nir_tex_src_ddy);
   const int min_lod_idx = nir_tex_instr_src_index(tex, nir_tex_src_min_lod);

   if (ddx_idx < 0 || ddx_idx < 0)
      return false;

   /* min_lod is applied after the sampler bias is added, so we can't use it as lod */
   if (min_lod_idx >= 0)
      return false;

   if (!nir_src_is_const(tex->src[ddx_idx].src) || !nir_src_is_const(tex->src[ddy_idx].src))
      return false;

   for (unsigned i = 0; i < tex->src[ddx_idx].src.ssa->num_components; i++) {
      if (nir_src_comp_as_float(tex->src[ddx_idx].src, i) != 0.0)
         return false;
   }

   for (unsigned i = 0; i < tex->src[ddy_idx].src.ssa->num_components; i++) {
      if (nir_src_comp_as_float(tex->src[ddy_idx].src, i) != 0.0)
         return false;
   }

   b->cursor = nir_before_instr(&tex->instr);
   nir_steal_tex_src(tex, nir_tex_src_ddx);
   nir_steal_tex_src(tex, nir_tex_src_ddy);

   nir_def *lod = nir_imm_int(b, 0);
   nir_tex_instr_add_src(tex, nir_tex_src_lod, lod);
   tex->op = nir_texop_txl;
   return true;
}

static bool
try_fold_tex_offset(nir_tex_instr *tex, unsigned *index,
                    nir_tex_src_type src_type)
{
   const int src_idx = nir_tex_instr_src_index(tex, src_type);
   if (src_idx < 0)
      return false;

   if (!nir_src_is_const(tex->src[src_idx].src))
      return false;

   *index += nir_src_as_uint(tex->src[src_idx].src);
   nir_tex_instr_remove_src(tex, src_idx);

   return true;
}

static bool
try_fold_texel_offset_src(nir_tex_instr *tex)
{
   int offset_src = nir_tex_instr_src_index(tex, nir_tex_src_offset);
   if (offset_src < 0)
      return false;

   unsigned size = nir_tex_instr_src_size(tex, offset_src);
   nir_tex_src *src = &tex->src[offset_src];

   for (unsigned i = 0; i < size; i++) {
      nir_scalar comp = nir_scalar_resolved(src->src.ssa, i);
      if (!nir_scalar_is_const(comp) || nir_scalar_as_uint(comp) != 0)
         return false;
   }

   nir_tex_instr_remove_src(tex, offset_src);

   return true;
}

static nir_def *
try_fold_tex(nir_builder *b, nir_tex_instr *tex)
{
   bool progress = false;

   progress |= try_fold_tex_offset(tex, &tex->texture_index,
                                   nir_tex_src_texture_offset);
   progress |= try_fold_tex_offset(tex, &tex->sampler_index,
                                   nir_tex_src_sampler_offset);

   /* txb with a bias of constant zero is just tex. */
   if (tex->op == nir_texop_txb)
      progress |= try_fold_txb_to_tex(b, tex);
   /* txd with ddx/ddy of constant zero is just txl. */
   if (tex->op == nir_texop_txd)
      progress |= try_fold_txd_to_txl(b, tex);

   /* tex with a zero offset is just tex. */
   progress |= try_fold_texel_offset_src(tex);

   return progress ? NIR_LOWER_INSTR_PROGRESS : NULL;
}

static nir_def *
try_fold_instr(nir_builder *b, nir_instr *instr, void *_state)
{
   switch (instr->type) {
   case nir_instr_type_alu:
      return nir_try_constant_fold_alu(b, nir_instr_as_alu(instr));
   case nir_instr_type_intrinsic:
      return try_fold_intrinsic(b, nir_instr_as_intrinsic(instr), _state);
   case nir_instr_type_tex:
      return try_fold_tex(b, nir_instr_as_tex(instr));
   default:
      /* Don't know how to constant fold */
      return NULL;
   }
}

bool
nir_opt_constant_folding(nir_shader *shader)
{
   struct constant_fold_state state;
   state.has_load_constant = false;
   state.has_indirect_load_const = false;

   bool progress = nir_shader_lower_instructions(shader, NULL, try_fold_instr,
                                                 &state);

   /* This doesn't free the constant data if there are no constant loads because
    * the data might still be used but the loads have been lowered to load_ubo
    */
   if (state.has_load_constant && !state.has_indirect_load_const &&
       shader->constant_data_size) {
      ralloc_free(shader->constant_data);
      shader->constant_data = NULL;
      shader->constant_data_size = 0;
   }

   return progress;
}
