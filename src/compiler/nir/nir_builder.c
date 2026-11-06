/*
 * Copyright © 2014-2015 Broadcom
 * Copyright © 2021 Google
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

#include "nir_builder.h"
#include "list.h"
#include "util/list.h"
#include "util/ralloc.h"
#include "glsl_types.h"
#include "nir.h"
#include "nir_serialize.h"

nir_builder MUST_CHECK PRINTFLIKE(3, 4)
   nir_builder_init_simple_shader(mesa_shader_stage stage,
                                  const nir_shader_compiler_options *options,
                                  const char *name, ...)
{
   nir_builder b;

   memset(&b, 0, sizeof(b));
   b.shader = nir_shader_create(NULL, stage, options);

   if (name) {
      va_list args;
      va_start(args, name);
      b.shader->info.name = ralloc_vasprintf(b.shader, name, args);
      va_end(args);
   }

   nir_function *func = nir_function_create(b.shader, "main");
   func->is_entrypoint = true;
   b.fp_math_ctrl = nir_fp_fast_math;
   b.impl = nir_function_impl_create(func);
   b.cursor = nir_after_cf_list(&b.impl->body);

   /* Simple shaders are typically internal, e.g. blit shaders */
   b.shader->info.internal = true;

   /* Compute shaders on Vulkan require some workgroup size initialized, pick
    * a safe default value. This relies on merging workgroups for efficiency.
    */
   b.shader->info.workgroup_size[0] = 1;
   b.shader->info.workgroup_size[1] = 1;
   b.shader->info.workgroup_size[2] = 1;

   return b;
}

nir_def *
nir_builder_alu_instr_finish_and_insert(nir_builder *build, nir_alu_instr *instr)
{
   const nir_op_info *op_info = &nir_op_infos[instr->op];

   instr->fp_math_ctrl = nir_op_valid_fp_math_ctrl(instr->op, build->fp_math_ctrl);

   /* Guess the number of components the destination temporary should have
    * based on our input sizes, if it's not fixed for the op.
    */
   unsigned num_components = op_info->output_size;
   if (num_components == 0) {
      for (unsigned i = 0; i < op_info->num_inputs; i++) {
         if (op_info->input_sizes[i] == 0)
            num_components = MAX2(num_components,
                                  instr->src[i].src.ssa->num_components);
      }
   }
   assert(num_components != 0);

   /* Figure out the bitwidth based on the source bitwidth if the instruction
    * is variable-width.
    */
   unsigned bit_size = nir_alu_type_get_type_size(op_info->output_type);
   if (bit_size == 0) {
      for (unsigned i = 0; i < op_info->num_inputs; i++) {
         unsigned src_bit_size = instr->src[i].src.ssa->bit_size;
         if (nir_alu_type_get_type_size(op_info->input_types[i]) == 0) {
            if (bit_size)
               assert(src_bit_size == bit_size);
            else
               bit_size = src_bit_size;
         } else {
            assert(src_bit_size ==
                   nir_alu_type_get_type_size(op_info->input_types[i]));
         }
      }
   }

   /* When in doubt, assume 32. */
   if (bit_size == 0)
      bit_size = 32;

   /* Make sure we don't swizzle from outside of our source vector (like if a
    * scalar value was passed into a multiply with a vector).
    */
   for (unsigned i = 0; i < op_info->num_inputs; i++) {
      for (unsigned j = instr->src[i].src.ssa->num_components;
           j < NIR_MAX_VEC_COMPONENTS; j++) {
         instr->src[i].swizzle[j] = instr->src[i].src.ssa->num_components - 1;
      }
   }

   nir_def_init(&instr->instr, &instr->def, num_components,
                bit_size);

   if (build->constant_fold_alu) {
      nir_def *new_def = nir_try_constant_fold_alu(build, instr);
      if (new_def) {
         nir_instr_free(&instr->instr);
         return new_def;
      }
   }

   nir_builder_instr_insert(build, &instr->instr);

   return &instr->def;
}

nir_def *
nir_build_alu(nir_builder *build, nir_op op, nir_def *src0,
              nir_def *src1, nir_def *src2, nir_def *src3)
{
   nir_alu_instr *instr = nir_alu_instr_create(build->shader, op);
   if (!instr)
      return NULL;

   instr->src[0].src = nir_src_for_ssa(src0);
   if (src1)
      instr->src[1].src = nir_src_for_ssa(src1);
   if (src2)
      instr->src[2].src = nir_src_for_ssa(src2);
   if (src3)
      instr->src[3].src = nir_src_for_ssa(src3);

   return nir_builder_alu_instr_finish_and_insert(build, instr);
}

nir_def *
nir_build_alu1(nir_builder *build, nir_op op, nir_def *src0)
{
   nir_alu_instr *instr = nir_alu_instr_create(build->shader, op);
   if (!instr)
      return NULL;

   instr->src[0].src = nir_src_for_ssa(src0);

   return nir_builder_alu_instr_finish_and_insert(build, instr);
}

nir_def *
nir_build_alu2(nir_builder *build, nir_op op, nir_def *src0,
               nir_def *src1)
{
   nir_alu_instr *instr = nir_alu_instr_create(build->shader, op);
   if (!instr)
      return NULL;

   instr->src[0].src = nir_src_for_ssa(src0);
   instr->src[1].src = nir_src_for_ssa(src1);

   return nir_builder_alu_instr_finish_and_insert(build, instr);
}

nir_def *
nir_build_alu3(nir_builder *build, nir_op op, nir_def *src0,
               nir_def *src1, nir_def *src2)
{
   nir_alu_instr *instr = nir_alu_instr_create(build->shader, op);
   if (!instr)
      return NULL;

   instr->src[0].src = nir_src_for_ssa(src0);
   instr->src[1].src = nir_src_for_ssa(src1);
   instr->src[2].src = nir_src_for_ssa(src2);

   return nir_builder_alu_instr_finish_and_insert(build, instr);
}

nir_def *
nir_build_alu4(nir_builder *build, nir_op op, nir_def *src0,
               nir_def *src1, nir_def *src2, nir_def *src3)
{
   nir_alu_instr *instr = nir_alu_instr_create(build->shader, op);
   if (!instr)
      return NULL;

   instr->src[0].src = nir_src_for_ssa(src0);
   instr->src[1].src = nir_src_for_ssa(src1);
   instr->src[2].src = nir_src_for_ssa(src2);
   instr->src[3].src = nir_src_for_ssa(src3);

   return nir_builder_alu_instr_finish_and_insert(build, instr);
}

/* for the couple special cases with more than 4 src args: */
nir_def *
nir_build_alu_src_arr(nir_builder *build, nir_op op, nir_def **srcs)
{
   const nir_op_info *op_info = &nir_op_infos[op];
   nir_alu_instr *instr = nir_alu_instr_create(build->shader, op);
   if (!instr)
      return NULL;

   for (unsigned i = 0; i < op_info->num_inputs; i++)
      instr->src[i].src = nir_src_for_ssa(srcs[i]);

   return nir_builder_alu_instr_finish_and_insert(build, instr);
}

static inline bool
nir_dim_has_lod(enum glsl_sampler_dim dim)
{
   switch (dim) {
   case GLSL_SAMPLER_DIM_1D:
   case GLSL_SAMPLER_DIM_2D:
   case GLSL_SAMPLER_DIM_3D:
   case GLSL_SAMPLER_DIM_CUBE:
      return true;
   default:
      return false;
   }
}

nir_def *
nir_build_tex_struct(nir_builder *build, nir_texop op, struct nir_tex_builder f)
{
   assert(((f.texture_index || f.texture_offset != NULL || f.texture_heap_offset != NULL) +
           (f.texture_handle != NULL) + (f.texture_deref != NULL)) <= 1 &&
          "one type of texture");

   assert(((f.sampler_index || f.sampler_offset != NULL || f.sampler_heap_offset != NULL) +
           (f.sampler_handle != NULL) + (f.sampler_deref != NULL)) <= 1 &&
          "one type of sampler");

   bool has_texture_src =
      f.texture_offset || f.texture_heap_offset || f.texture_handle || f.texture_deref;

   bool has_sampler_src =
      f.sampler_offset || f.sampler_heap_offset || f.sampler_handle || f.sampler_deref;

   nir_def *lod = f.lod;
   enum glsl_sampler_dim dim = f.dim;
   nir_alu_type dest_type = f.dest_type;
   bool is_array = f.is_array;

   if (f.texture_deref) {
      const glsl_type *type = f.texture_deref->type;
      assert(glsl_type_is_image(type) || glsl_type_is_texture(type) ||
             glsl_type_is_sampler(type));

      dim = glsl_get_sampler_dim(type);
      is_array = glsl_sampler_type_is_array(type);

      dest_type = nir_get_nir_type_for_glsl_base_type(
         glsl_get_sampler_result_type(type));
   }

   /* Fix up the opcode to allow simplified usage. This helps ergonomics. */
   if (op == nir_texop_txf && f.ms_index) {
      op = nir_texop_txf_ms;
   } else if (op == nir_texop_tex && f.lod) {
      op = nir_texop_txl;
   } else if (op == nir_texop_tex && f.bias) {
      op = nir_texop_txb;
   }

   if (lod == NULL && nir_dim_has_lod(dim) &&
       (op == nir_texop_txs || op == nir_texop_txf)) {

      lod = nir_imm_int(build, 0);
   }

   const unsigned num_srcs = has_texture_src + has_sampler_src + !!f.coord +
                             !!f.ms_index + !!lod + !!f.bias +
                             !!f.comparator + !!f.backend1 + !!f.backend2;

   nir_tex_instr *tex = nir_tex_instr_create(build->shader, num_srcs);
   tex->op = op;
   tex->sampler_dim = dim;
   tex->is_array = is_array;
   tex->is_shadow = false;
   tex->is_sparse = f.is_sparse;
   tex->backend_flags = f.backend_flags;
   tex->texture_index = f.texture_index;
   tex->sampler_index = f.sampler_index;
   tex->can_speculate = f.can_speculate;

   switch (op) {
   case nir_texop_txs:
   case nir_texop_texture_samples:
   case nir_texop_query_levels:
   case nir_texop_txf_ms_mcs_intel:
   case nir_texop_fragment_mask_fetch_amd:
   case nir_texop_descriptor_amd:
   case nir_texop_resinfo_intel:
   case nir_texop_sparse_residency_intel:
   case nir_texop_sparse_residency_txf_intel:
      tex->dest_type = nir_type_int32;
      break;
   case nir_texop_lod:
      tex->dest_type = nir_type_float32;
      break;
   case nir_texop_samples_identical:
      tex->dest_type = nir_type_bool1;
      break;
   default:
      assert(!nir_tex_instr_is_query(tex));
      tex->dest_type = dest_type;
      break;
   }

   unsigned i = 0;

   if (f.texture_deref) {
      tex->src[i++] =
         nir_tex_src_for_ssa(nir_tex_src_texture_deref, &f.texture_deref->def);
   } else if (f.texture_handle) {
      tex->src[i++] =
         nir_tex_src_for_ssa(nir_tex_src_texture_handle, f.texture_handle);
   } else if (f.texture_offset) {
      tex->src[i++] =
         nir_tex_src_for_ssa(nir_tex_src_texture_offset, f.texture_offset);
   } else if (f.texture_heap_offset) {
      tex->src[i++] =
         nir_tex_src_for_ssa(nir_tex_src_texture_heap_offset, f.texture_heap_offset);
   }

   if (f.sampler_deref) {
      assert(glsl_type_is_sampler(f.sampler_deref->type));
      tex->src[i++] =
         nir_tex_src_for_ssa(nir_tex_src_sampler_deref, &f.sampler_deref->def);
   } else if (f.sampler_handle) {
      tex->src[i++] =
         nir_tex_src_for_ssa(nir_tex_src_sampler_handle, f.sampler_handle);
   } else if (f.sampler_offset) {
      tex->src[i++] =
         nir_tex_src_for_ssa(nir_tex_src_sampler_offset, f.sampler_offset);
   } else if (f.sampler_heap_offset) {
      tex->src[i++] =
         nir_tex_src_for_ssa(nir_tex_src_sampler_heap_offset, f.sampler_heap_offset);
   }

   if (f.coord) {
      tex->coord_components = f.coord->num_components;

      assert(tex->coord_components ==
             tex->is_array +
                glsl_get_sampler_dim_coordinate_components(tex->sampler_dim));

      tex->src[i++] = nir_tex_src_for_ssa(nir_tex_src_coord, f.coord);
   }

   if (lod) {
      tex->src[i++] = nir_tex_src_for_ssa(nir_tex_src_lod, lod);
   }

   if (f.ms_index) {
      assert(tex->sampler_dim == GLSL_SAMPLER_DIM_MS);
      tex->src[i++] = nir_tex_src_for_ssa(nir_tex_src_ms_index, f.ms_index);
   }

   if (f.comparator) {
      /* Assume 1-component shadow for the builder helper */
      tex->is_shadow = true;
      tex->is_new_style_shadow = true;
      tex->src[i++] = nir_tex_src_for_ssa(nir_tex_src_comparator, f.comparator);
   }

   if (f.backend1)
      tex->src[i++] = nir_tex_src_for_ssa(nir_tex_src_backend1, f.backend1);
   if (f.backend2)
      tex->src[i++] = nir_tex_src_for_ssa(nir_tex_src_backend2, f.backend2);

   assert(i == num_srcs);

   nir_def_init(&tex->instr, &tex->def, nir_tex_instr_dest_size(tex),
                nir_alu_type_get_type_size(tex->dest_type));
   nir_builder_instr_insert(build, &tex->instr);

   return &tex->def;
}

nir_def *
nir_vec_scalars(nir_builder *build, nir_scalar *comp, unsigned num_components)
{
   nir_op op = nir_op_vec(num_components);
   nir_alu_instr *instr = nir_alu_instr_create(build->shader, op);
   if (!instr)
      return NULL;

   for (unsigned i = 0; i < num_components; i++) {
      instr->src[i].src = nir_src_for_ssa(comp[i].def);
      instr->src[i].swizzle[0] = comp[i].comp;
   }
   assert(nir_op_infos[op].valid_fp_math_ctrl == 0);

   /* Note: not reusing nir_builder_alu_instr_finish_and_insert() because it
    * can't re-guess the num_components when num_components == 1 (nir_op_mov).
    */
   nir_def_init(&instr->instr, &instr->def, num_components,
                comp[0].def->bit_size);

   nir_builder_instr_insert(build, &instr->instr);

   return &instr->def;
}

nir_def *
nir_def_rewrite_uses_with_alu_src(nir_builder *build, nir_def *def, nir_alu_src src)
{
   unsigned num_components = def->num_components;

   if (nir_alu_src_is_trivial_ssa(&src, num_components)) {
      nir_def_rewrite_uses(def, src.src.ssa);
      return NULL;
   }

   nir_def *mov = NULL;

   nir_foreach_use_including_if_safe(use, def) {
      if (nir_src_is_if(use) || nir_src_use_instr(use)->type != nir_instr_type_alu) {
         if (!mov)
            mov = nir_mov_alu(build, src, num_components);

         nir_src_rewrite(use, mov);
      } else {
         nir_alu_src *alu_src = container_of(use, nir_alu_src, src);

         for (unsigned i = 0; i < NIR_MAX_VEC_COMPONENTS; i++)
            alu_src->swizzle[i] = src.swizzle[alu_src->swizzle[i]];

         nir_src_rewrite(use, src.src.ssa);
      }
   }

   return mov;
}

/**
 * Get nir_def for an alu src, respecting the nir_alu_src's swizzle.
 */
nir_def *
nir_ssa_for_alu_src(nir_builder *build, nir_alu_instr *instr, unsigned srcn)
{
   if (nir_alu_has_trivial_src(instr, srcn))
      return instr->src[srcn].src.ssa;

   nir_alu_src *src = &instr->src[srcn];
   unsigned num_components = nir_ssa_alu_instr_src_components(instr, srcn);
   return nir_mov_alu(build, *src, num_components);
}

/* Generic builder for system values. */
nir_def *
nir_load_system_value(nir_builder *build, nir_intrinsic_op op, int index,
                      unsigned num_components, unsigned bit_size)
{
   nir_intrinsic_instr *load = nir_intrinsic_instr_create(build->shader, op);
   if (nir_intrinsic_infos[op].dest_components > 0)
      assert(num_components == nir_intrinsic_infos[op].dest_components);
   else
      load->num_components = num_components;
   load->const_index[0] = index;

   nir_def_init(&load->instr, &load->def, num_components, bit_size);
   nir_builder_instr_insert(build, &load->instr);
   return &load->def;
}

void
nir_builder_instr_insert(nir_builder *build, nir_instr *instr)
{
   nir_instr_insert(build->cursor, instr);

   if (unlikely(build->shader->has_debug_info &&
                (build->cursor.option == nir_cursor_before_instr ||
                 build->cursor.option == nir_cursor_after_instr))) {
      nir_instr_debug_info *cursor_info = nir_instr_get_debug_info(build->cursor.instr);
      nir_instr_debug_info *instr_info = nir_instr_get_debug_info(instr);

      if (!instr_info->line)
         instr_info->line = cursor_info->line;
      if (!instr_info->column)
         instr_info->column = cursor_info->column;
      if (!instr_info->spirv_offset)
         instr_info->spirv_offset = cursor_info->spirv_offset;
      if (!instr_info->filename)
         instr_info->filename = cursor_info->filename;
   }

   /* Move the cursor forward. */
   build->cursor = nir_after_instr(instr);
}

void
nir_builder_instr_insert_at_top(nir_builder *build, nir_instr *instr)
{
   nir_cursor top = nir_before_impl(build->impl);
   const bool at_top = build->cursor.block != NULL &&
                       nir_cursors_equal(build->cursor, top);

   nir_instr_insert(top, instr);

   if (at_top)
      build->cursor = nir_after_instr(instr);
}

void
nir_builder_cf_insert(nir_builder *build, nir_cf_node *cf)
{
   nir_cf_node_insert(build->cursor, cf);
}

bool
nir_builder_is_inside_cf(nir_builder *build, nir_cf_node *cf_node)
{
   nir_block *block = nir_cursor_current_block(build->cursor);
   for (nir_cf_node *n = &block->cf_node; n; n = n->parent) {
      if (n == cf_node)
         return true;
   }
   return false;
}

nir_if *
nir_push_if(nir_builder *build, nir_def *condition)
{
   nir_if *nif = nir_if_create(build->impl);
   nif->condition = nir_src_for_ssa(condition);
   nir_builder_cf_insert(build, &nif->cf_node);
   build->cursor = nir_before_cf_list(&nif->then_list);
   return nif;
}

nir_if *
nir_push_else(nir_builder *build, nir_if *nif)
{
   if (nif) {
      assert(nir_builder_is_inside_cf(build, &nif->cf_node));
   } else {
      nir_block *block = nir_cursor_current_block(build->cursor);
      nif = nir_cf_node_as_if(block->cf_node.parent);
   }
   build->cursor = nir_before_cf_list(&nif->else_list);
   return nif;
}

void
nir_pop_if(nir_builder *build, nir_if *nif)
{
   if (nif) {
      assert(nir_builder_is_inside_cf(build, &nif->cf_node));
   } else {
      nir_block *block = nir_cursor_current_block(build->cursor);
      nif = nir_cf_node_as_if(block->cf_node.parent);
   }
   build->cursor = nir_after_cf_node(&nif->cf_node);
}

nir_def *
nir_if_phi(nir_builder *build, nir_def *then_def, nir_def *else_def)
{
   nir_block *block = nir_cursor_current_block(build->cursor);
   nir_if *nif = nir_cf_node_as_if(nir_cf_node_prev(&block->cf_node));

   nir_phi_instr *phi = nir_phi_instr_create(build->shader);
   nir_phi_instr_add_src(phi, nir_if_last_then_block(nif), then_def);
   nir_phi_instr_add_src(phi, nir_if_last_else_block(nif), else_def);

   assert(then_def->num_components == else_def->num_components);
   assert(then_def->bit_size == else_def->bit_size);
   nir_def_init(&phi->instr, &phi->def, then_def->num_components,
                then_def->bit_size);

   nir_builder_instr_insert(build, &phi->instr);

   return &phi->def;
}

nir_loop *
nir_push_loop(nir_builder *build)
{
   nir_loop *loop = nir_loop_create(build->impl);
   nir_builder_cf_insert(build, &loop->cf_node);
   build->cursor = nir_before_cf_list(&loop->body);
   return loop;
}

nir_loop *
nir_push_continue(nir_builder *build, nir_loop *loop)
{
   if (loop) {
      assert(nir_builder_is_inside_cf(build, &loop->cf_node));
   } else {
      nir_block *block = nir_cursor_current_block(build->cursor);
      loop = nir_cf_node_as_loop(block->cf_node.parent);
   }

   assert(nir_loop_has_continue_construct(loop));
   build->cursor = nir_before_cf_list(&loop->continue_list);
   return loop;
}

void
nir_pop_loop(nir_builder *build, nir_loop *loop)
{
   if (loop) {
      assert(nir_builder_is_inside_cf(build, &loop->cf_node));
   } else {
      nir_block *block = nir_cursor_current_block(build->cursor);
      loop = nir_cf_node_as_loop(block->cf_node.parent);
   }
   build->cursor = nir_after_cf_node(&loop->cf_node);
}

nir_def *
nir_compare_func(nir_builder *b, enum compare_func func,
                 nir_def *src0, nir_def *src1)
{
   switch (func) {
   case COMPARE_FUNC_NEVER:
      return nir_imm_int(b, 0);
   case COMPARE_FUNC_ALWAYS:
      return nir_imm_int(b, ~0);
   case COMPARE_FUNC_EQUAL:
      return nir_feq(b, src0, src1);
   case COMPARE_FUNC_NOTEQUAL:
      return nir_fneu(b, src0, src1);
   case COMPARE_FUNC_GREATER:
      return nir_flt(b, src1, src0);
   case COMPARE_FUNC_GEQUAL:
      return nir_fge(b, src0, src1);
   case COMPARE_FUNC_LESS:
      return nir_flt(b, src0, src1);
   case COMPARE_FUNC_LEQUAL:
      return nir_fge(b, src1, src0);
   }
   UNREACHABLE("bad compare func");
}

nir_def *
nir_type_convert(nir_builder *b,
                 nir_def *src,
                 nir_alu_type src_type,
                 nir_alu_type dest_type,
                 nir_rounding_mode rnd)
{
   assert(nir_alu_type_get_type_size(src_type) == 0 ||
          nir_alu_type_get_type_size(src_type) == src->bit_size);

   const nir_alu_type dst_base =
      (nir_alu_type)nir_alu_type_get_base_type(dest_type);

   const nir_alu_type src_base =
      (nir_alu_type)nir_alu_type_get_base_type(src_type);

   /* b2b uses the regular type conversion path, but i2b and f2b are
    * implemented as src != 0.
    */
   if (dst_base == nir_type_bool && src_base != nir_type_bool) {
      nir_op opcode;

      /* For conversions to backend-specific bit-sizes,
       * please use the appropriate instructions directly.
       */
      assert(nir_alu_type_get_type_size(dest_type) == 1);

      if (src_base == nir_type_float) {
         opcode = nir_op_fneu;
      } else {
         assert(src_base == nir_type_int || src_base == nir_type_uint);
         opcode = nir_op_ine;
      }

      return nir_build_alu(b, opcode, src,
                           nir_imm_zero(b, src->num_components, src->bit_size),
                           NULL, NULL);
   } else {
      src_type = (nir_alu_type)(src_type | src->bit_size);

      nir_op opcode =
         nir_type_conversion_op(src_type, dest_type, rnd);
      if (opcode == nir_op_mov)
         return src;

      return nir_build_alu(b, opcode, src, NULL, NULL, NULL);
   }
}

nir_def *
nir_gen_rect_vertices(nir_builder *b, nir_def *z, nir_def *w)
{
   if (!z)
      z = nir_imm_float(b, 0.0);
   if (!w)
      w = nir_imm_float(b, 1.0);

   nir_def *vertex_id;
   if (b->shader->options && b->shader->options->vertex_id_zero_based)
      vertex_id = nir_load_vertex_id_zero_base(b);
   else
      vertex_id = nir_load_vertex_id(b);

   /* vertex 0: -1.0, -1.0
    * vertex 1: -1.0,  1.0
    * vertex 2:  1.0, -1.0
    * vertex 3:  1.0,  1.0
    *
    * so:
    *
    * channel 0 is vertex_id < 2 ? -1.0 :  1.0
    * channel 1 is vertex_id & 1 ?  1.0 : -1.0
    */

   nir_def *c0cmp = nir_ilt_imm(b, vertex_id, 2);
   nir_def *c1cmp = nir_test_mask(b, vertex_id, 1);

   nir_def *comp[4];
   comp[0] = nir_bcsel(b, c0cmp, nir_imm_float(b, -1.0), nir_imm_float(b, 1.0));
   comp[1] = nir_bcsel(b, c1cmp, nir_imm_float(b, 1.0), nir_imm_float(b, -1.0));
   comp[2] = z;
   comp[3] = w;

   return nir_vec(b, comp, 4);
}

/* glsl-optimizer change: no nir_call_serialized() (needs NIR deserialization) */

/* Build frag_coord according to NIR options. This should generate the final
 * lowered form expected by drivers.
 *
 * The purpose of "num_components" is to generate less dead code for the split
 * form if some components are not needed.
 */
nir_def *
nir_build_frag_coord(nir_builder *b, unsigned num_components)
{
   assert(b->shader->info.stage == MESA_SHADER_FRAGMENT);
   assert(num_components && num_components <= 4);

   if (b->shader->options->frag_coord_form & nir_frag_coord_xy_z_w_separate) {
      nir_def *xy;

      if (b->shader->options->frag_coord_form &
          nir_frag_coord_use_pixel_coord) {
         xy = nir_u2f32(b, nir_load_pixel_coord(b));

         if (!b->shader->info.fs.pixel_center_integer)
            xy = nir_fadd_imm(b, nir_u2f32(b, nir_load_pixel_coord(b)), 0.5);
      } else {
         xy = nir_load_frag_coord_xy(b);
      }

      if (num_components <= 2)
         return nir_trim_vector(b, xy, num_components);

      nir_def *z = nir_load_frag_coord_z(b);

      if (num_components == 3)
         return nir_vec3(b, nir_channel(b, xy, 0), nir_channel(b, xy, 1), z);

      nir_def *w;

      if (b->shader->options->frag_coord_form & nir_frag_coord_use_w_rcp)
         w = nir_frcp(b, nir_load_frag_coord_w_rcp(b));
      else
         w = nir_load_frag_coord_w(b);

      return nir_vec4(b, nir_channel(b, xy, 0), nir_channel(b, xy, 1), z, w);
   } else {
      return nir_trim_vector(b, nir_load_frag_coord(b), num_components);
   }
}
