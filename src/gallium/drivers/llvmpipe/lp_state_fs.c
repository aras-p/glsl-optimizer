/**************************************************************************
 * 
 * Copyright 2009 VMware, Inc.
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/**
 * @file
 * Code generate the whole fragment pipeline.
 *
 * The fragment pipeline consists of the following stages:
 * - early depth test
 * - fragment shader
 * - alpha test
 * - depth/stencil test
 * - blending
 *
 * This file has only the glue to assemble the fragment pipeline.  The actual
 * plumbing of converting Gallium state into LLVM IR is done elsewhere, in the
 * lp_bld_*.[ch] files, and in a complete generic and reusable way. Here we
 * muster the LLVM JIT execution engine to create a function that follows an
 * established binary interface and that can be called from C directly.
 *
 * A big source of complexity here is that we often want to run different
 * stages with different precisions and data types and precisions. For example,
 * the fragment shader needs typically to be done in floats, but the
 * depth/stencil test and blending is better done in the type that most closely
 * matches the depth/stencil and color buffer respectively.
 *
 * Since the width of a SIMD vector register stays the same regardless of the
 * element type, different types imply different number of elements, so we must
 * code generate more instances of the stages with larger types to be able to
 * feed/consume the stages with smaller types.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */

#include <limits.h>
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_pointer.h"
#include "util/u_format.h"
#include "util/u_dump.h"
#include "util/u_string.h"
#include "util/u_simple_list.h"
#include "os/os_time.h"
#include "pipe/p_shader_tokens.h"
#include "draw/draw_context.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_scan.h"
#include "tgsi/tgsi_parse.h"
#include "gallivm/lp_bld_type.h"
#include "gallivm/lp_bld_const.h"
#include "gallivm/lp_bld_conv.h"
#include "gallivm/lp_bld_init.h"
#include "gallivm/lp_bld_intr.h"
#include "gallivm/lp_bld_logic.h"
#include "gallivm/lp_bld_tgsi.h"
#include "gallivm/lp_bld_swizzle.h"
#include "gallivm/lp_bld_flow.h"
#include "gallivm/lp_bld_debug.h"
#include "gallivm/lp_bld_arit.h"
#include "gallivm/lp_bld_pack.h"
#include "gallivm/lp_bld_format.h"
#include "gallivm/lp_bld_quad.h"

#include "lp_bld_alpha.h"
#include "lp_bld_blend.h"
#include "lp_bld_depth.h"
#include "lp_bld_interp.h"
#include "lp_context.h"
#include "lp_debug.h"
#include "lp_perf.h"
#include "lp_setup.h"
#include "lp_state.h"
#include "lp_tex_sample.h"
#include "lp_flush.h"
#include "lp_state_fs.h"


/** Fragment shader number (for debugging) */
static unsigned fs_no = 0;


/**
 * Expand the relevant bits of mask_input to a n*4-dword mask for the
 * n*four pixels in n 2x2 quads.  This will set the n*four elements of the
 * quad mask vector to 0 or ~0.
 * Grouping is 01, 23 for 2 quad mode hence only 0 and 2 are valid
 * quad arguments with fs length 8.
 *
 * \param first_quad  which quad(s) of the quad group to test, in [0,3]
 * \param mask_input  bitwise mask for the whole 4x4 stamp
 */
static LLVMValueRef
generate_quad_mask(struct gallivm_state *gallivm,
                   struct lp_type fs_type,
                   unsigned first_quad,
                   LLVMValueRef mask_input) /* int32 */
{
   LLVMBuilderRef builder = gallivm->builder;
   struct lp_type mask_type;
   LLVMTypeRef i32t = LLVMInt32TypeInContext(gallivm->context);
   LLVMValueRef bits[16];
   LLVMValueRef mask;
   int shift, i;

   /*
    * XXX: We'll need a different path for 16 x u8
    */
   assert(fs_type.width == 32);
   assert(fs_type.length <= Elements(bits));
   mask_type = lp_int_type(fs_type);

   /*
    * mask_input >>= (quad * 4)
    */
   switch (first_quad) {
   case 0:
      shift = 0;
      break;
   case 1:
      assert(fs_type.length == 4);
      shift = 2;
      break;
   case 2:
      shift = 8;
      break;
   case 3:
      assert(fs_type.length == 4);
      shift = 10;
      break;
   default:
      assert(0);
      shift = 0;
   }

   mask_input = LLVMBuildLShr(builder,
                              mask_input,
                              LLVMConstInt(i32t, shift, 0),
                              "");

   /*
    * mask = { mask_input & (1 << i), for i in [0,3] }
    */
   mask = lp_build_broadcast(gallivm,
                             lp_build_vec_type(gallivm, mask_type),
                             mask_input);

   for (i = 0; i < fs_type.length / 4; i++) {
      unsigned j = 2 * (i % 2) + (i / 2) * 8;
      bits[4*i + 0] = LLVMConstInt(i32t, 1 << (j + 0), 0);
      bits[4*i + 1] = LLVMConstInt(i32t, 1 << (j + 1), 0);
      bits[4*i + 2] = LLVMConstInt(i32t, 1 << (j + 4), 0);
      bits[4*i + 3] = LLVMConstInt(i32t, 1 << (j + 5), 0);
   }
   mask = LLVMBuildAnd(builder, mask, LLVMConstVector(bits, fs_type.length), "");

   /*
    * mask = mask != 0 ? ~0 : 0
    */
   mask = lp_build_compare(gallivm,
                           mask_type, PIPE_FUNC_NOTEQUAL,
                           mask,
                           lp_build_const_int_vec(gallivm, mask_type, 0));

   return mask;
}


#define EARLY_DEPTH_TEST  0x1
#define LATE_DEPTH_TEST   0x2
#define EARLY_DEPTH_WRITE 0x4
#define LATE_DEPTH_WRITE  0x8

static int
find_output_by_semantic( const struct tgsi_shader_info *info,
			 unsigned semantic,
			 unsigned index )
{
   int i;

   for (i = 0; i < info->num_outputs; i++)
      if (info->output_semantic_name[i] == semantic &&
	  info->output_semantic_index[i] == index)
	 return i;

   return -1;
}


/**
 * Generate the fragment shader, depth/stencil test, and alpha tests.
 * \param i  which quad in the tile, in range [0,3]
 * \param partial_mask  if 1, do mask_input testing
 */
static void
generate_fs(struct gallivm_state *gallivm,
            struct lp_fragment_shader *shader,
            const struct lp_fragment_shader_variant_key *key,
            LLVMBuilderRef builder,
            struct lp_type type,
            LLVMValueRef context_ptr,
            unsigned i,
            struct lp_build_interp_soa_context *interp,
            struct lp_build_sampler_soa *sampler,
            LLVMValueRef *pmask,
            LLVMValueRef (*color)[4],
            LLVMValueRef depth_ptr,
            LLVMValueRef facing,
            unsigned partial_mask,
            LLVMValueRef mask_input,
            LLVMValueRef counter)
{
   const struct util_format_description *zs_format_desc = NULL;
   const struct tgsi_token *tokens = shader->base.tokens;
   LLVMTypeRef vec_type;
   LLVMValueRef consts_ptr;
   LLVMValueRef outputs[PIPE_MAX_SHADER_OUTPUTS][TGSI_NUM_CHANNELS];
   LLVMValueRef z;
   LLVMValueRef zs_value = NULL;
   LLVMValueRef stencil_refs[2];
   struct lp_build_mask_context mask;
   boolean simple_shader = (shader->info.base.file_count[TGSI_FILE_SAMPLER] == 0 &&
                            shader->info.base.num_inputs < 3 &&
                            shader->info.base.num_instructions < 8);
   unsigned attrib;
   unsigned chan;
   unsigned cbuf;
   unsigned depth_mode;
   struct lp_bld_tgsi_system_values system_values;

   memset(&system_values, 0, sizeof(system_values));

   if (key->depth.enabled ||
       key->stencil[0].enabled ||
       key->stencil[1].enabled) {

      zs_format_desc = util_format_description(key->zsbuf_format);
      assert(zs_format_desc);

      if (!shader->info.base.writes_z) {
         if (key->alpha.enabled || shader->info.base.uses_kill)
            /* With alpha test and kill, can do the depth test early
             * and hopefully eliminate some quads.  But need to do a
             * special deferred depth write once the final mask value
             * is known.
             */
            depth_mode = EARLY_DEPTH_TEST | LATE_DEPTH_WRITE;
         else
            depth_mode = EARLY_DEPTH_TEST | EARLY_DEPTH_WRITE;
      }
      else {
         depth_mode = LATE_DEPTH_TEST | LATE_DEPTH_WRITE;
      }

      if (!(key->depth.enabled && key->depth.writemask) &&
          !(key->stencil[0].enabled && key->stencil[0].writemask))
         depth_mode &= ~(LATE_DEPTH_WRITE | EARLY_DEPTH_WRITE);
   }
   else {
      depth_mode = 0;
   }

   assert(i < 4);

   stencil_refs[0] = lp_jit_context_stencil_ref_front_value(gallivm, context_ptr);
   stencil_refs[1] = lp_jit_context_stencil_ref_back_value(gallivm, context_ptr);

   vec_type = lp_build_vec_type(gallivm, type);

   consts_ptr = lp_jit_context_constants(gallivm, context_ptr);

   memset(outputs, 0, sizeof outputs);

   /* Declare the color and z variables */
   for(cbuf = 0; cbuf < key->nr_cbufs; cbuf++) {
      for(chan = 0; chan < TGSI_NUM_CHANNELS; ++chan) {
         color[cbuf][chan] = lp_build_alloca(gallivm, vec_type, "color");
      }
   }

   /* do triangle edge testing */
   if (partial_mask) {
      *pmask = generate_quad_mask(gallivm, type,
                                  i*type.length/4, mask_input);
   }
   else {
      *pmask = lp_build_const_int_vec(gallivm, type, ~0);
   }

   /* 'mask' will control execution based on quad's pixel alive/killed state */
   lp_build_mask_begin(&mask, gallivm, type, *pmask);

   if (!(depth_mode & EARLY_DEPTH_TEST) && !simple_shader)
      lp_build_mask_check(&mask);

   lp_build_interp_soa_update_pos(interp, gallivm, i*type.length/4);
   z = interp->pos[2];

   if (depth_mode & EARLY_DEPTH_TEST) {
      lp_build_depth_stencil_test(gallivm,
                                  &key->depth,
                                  key->stencil,
                                  type,
                                  zs_format_desc,
                                  &mask,
                                  stencil_refs,
                                  z,
                                  depth_ptr, facing,
                                  &zs_value,
                                  !simple_shader);

      if (depth_mode & EARLY_DEPTH_WRITE) {
         lp_build_depth_write(builder, zs_format_desc, depth_ptr, zs_value);
      }
   }

   lp_build_interp_soa_update_inputs(interp, gallivm, i*type.length/4);

   /* Build the actual shader */
   lp_build_tgsi_soa(gallivm, tokens, type, &mask,
                     consts_ptr, &system_values,
                     interp->pos, interp->inputs,
                     outputs, sampler, &shader->info.base);

   /* Alpha test */
   if (key->alpha.enabled) {
      int color0 = find_output_by_semantic(&shader->info.base,
                                           TGSI_SEMANTIC_COLOR,
                                           0);

      if (color0 != -1 && outputs[color0][3]) {
         const struct util_format_description *cbuf_format_desc;
         LLVMValueRef alpha = LLVMBuildLoad(builder, outputs[color0][3], "alpha");
         LLVMValueRef alpha_ref_value;

         alpha_ref_value = lp_jit_context_alpha_ref_value(gallivm, context_ptr);
         alpha_ref_value = lp_build_broadcast(gallivm, vec_type, alpha_ref_value);

         cbuf_format_desc = util_format_description(key->cbuf_format[0]);

         lp_build_alpha_test(gallivm, key->alpha.func, type, cbuf_format_desc,
                             &mask, alpha, alpha_ref_value,
                             (depth_mode & LATE_DEPTH_TEST) != 0);
      }
   }

   /* Late Z test */
   if (depth_mode & LATE_DEPTH_TEST) { 
      int pos0 = find_output_by_semantic(&shader->info.base,
                                         TGSI_SEMANTIC_POSITION,
                                         0);
         
      if (pos0 != -1 && outputs[pos0][2]) {
         z = LLVMBuildLoad(builder, outputs[pos0][2], "output.z");
      }

      lp_build_depth_stencil_test(gallivm,
                                  &key->depth,
                                  key->stencil,
                                  type,
                                  zs_format_desc,
                                  &mask,
                                  stencil_refs,
                                  z,
                                  depth_ptr, facing,
                                  &zs_value,
                                  !simple_shader);
      /* Late Z write */
      if (depth_mode & LATE_DEPTH_WRITE) {
         lp_build_depth_write(builder, zs_format_desc, depth_ptr, zs_value);
      }
   }
   else if ((depth_mode & EARLY_DEPTH_TEST) &&
            (depth_mode & LATE_DEPTH_WRITE))
   {
      /* Need to apply a reduced mask to the depth write.  Reload the
       * depth value, update from zs_value with the new mask value and
       * write that out.
       */
      lp_build_deferred_depth_write(gallivm,
                                    type,
                                    zs_format_desc,
                                    &mask,
                                    depth_ptr,
                                    zs_value);
   }


   /* Color write  */
   for (attrib = 0; attrib < shader->info.base.num_outputs; ++attrib)
   {
      if (shader->info.base.output_semantic_name[attrib] == TGSI_SEMANTIC_COLOR &&
          shader->info.base.output_semantic_index[attrib] < key->nr_cbufs)
      {
         unsigned cbuf = shader->info.base.output_semantic_index[attrib];
         for(chan = 0; chan < TGSI_NUM_CHANNELS; ++chan) {
            if(outputs[attrib][chan]) {
               /* XXX: just initialize outputs to point at colors[] and
                * skip this.
                */
               LLVMValueRef out = LLVMBuildLoad(builder, outputs[attrib][chan], "");
               lp_build_name(out, "color%u.%u.%c", i, attrib, "rgba"[chan]);
               LLVMBuildStore(builder, out, color[cbuf][chan]);
            }
         }
      }
   }

   if (counter)
      lp_build_occlusion_count(gallivm, type,
                               lp_build_mask_value(&mask), counter);

   *pmask = lp_build_mask_end(&mask);
}


/**
 * Generate the fragment shader, depth/stencil test, and alpha tests.
 */
static void
generate_fs_loop(struct gallivm_state *gallivm,
                 struct lp_fragment_shader *shader,
                 const struct lp_fragment_shader_variant_key *key,
                 LLVMBuilderRef builder,
                 struct lp_type type,
                 LLVMValueRef context_ptr,
                 LLVMValueRef num_loop,
                 struct lp_build_interp_soa_context *interp,
                 struct lp_build_sampler_soa *sampler,
                 LLVMValueRef mask_store,
                 LLVMValueRef (*out_color)[4],
                 LLVMValueRef depth_ptr,
                 unsigned depth_bits,
                 LLVMValueRef facing,
                 LLVMValueRef counter)
{
   const struct util_format_description *zs_format_desc = NULL;
   const struct tgsi_token *tokens = shader->base.tokens;
   LLVMTypeRef vec_type;
   LLVMValueRef mask_ptr, mask_val;
   LLVMValueRef consts_ptr;
   LLVMValueRef z;
   LLVMValueRef zs_value = NULL;
   LLVMValueRef stencil_refs[2];
   LLVMValueRef depth_ptr_i;
   LLVMValueRef depth_offset;
   LLVMValueRef outputs[PIPE_MAX_SHADER_OUTPUTS][TGSI_NUM_CHANNELS];
   struct lp_build_for_loop_state loop_state;
   struct lp_build_mask_context mask;
   boolean simple_shader = (shader->info.base.file_count[TGSI_FILE_SAMPLER] == 0 &&
                            shader->info.base.num_inputs < 3 &&
                            shader->info.base.num_instructions < 8);
   unsigned attrib;
   unsigned chan;
   unsigned cbuf;
   unsigned depth_mode;

   struct lp_bld_tgsi_system_values system_values;

   memset(&system_values, 0, sizeof(system_values));

   if (key->depth.enabled ||
       key->stencil[0].enabled ||
       key->stencil[1].enabled) {

      zs_format_desc = util_format_description(key->zsbuf_format);
      assert(zs_format_desc);

      if (!shader->info.base.writes_z) {
         if (key->alpha.enabled || shader->info.base.uses_kill)
            /* With alpha test and kill, can do the depth test early
             * and hopefully eliminate some quads.  But need to do a
             * special deferred depth write once the final mask value
             * is known.
             */
            depth_mode = EARLY_DEPTH_TEST | LATE_DEPTH_WRITE;
         else
            depth_mode = EARLY_DEPTH_TEST | EARLY_DEPTH_WRITE;
      }
      else {
         depth_mode = LATE_DEPTH_TEST | LATE_DEPTH_WRITE;
      }

      if (!(key->depth.enabled && key->depth.writemask) &&
          !(key->stencil[0].enabled && key->stencil[0].writemask))
         depth_mode &= ~(LATE_DEPTH_WRITE | EARLY_DEPTH_WRITE);
   }
   else {
      depth_mode = 0;
   }


   stencil_refs[0] = lp_jit_context_stencil_ref_front_value(gallivm, context_ptr);
   stencil_refs[1] = lp_jit_context_stencil_ref_back_value(gallivm, context_ptr);

   vec_type = lp_build_vec_type(gallivm, type);

   consts_ptr = lp_jit_context_constants(gallivm, context_ptr);

   lp_build_for_loop_begin(&loop_state, gallivm,
                           lp_build_const_int32(gallivm, 0),
                           LLVMIntULT,
                           num_loop,
                           lp_build_const_int32(gallivm, 1));

   mask_ptr = LLVMBuildGEP(builder, mask_store,
                           &loop_state.counter, 1, "mask_ptr");
   mask_val = LLVMBuildLoad(builder, mask_ptr, "");

   depth_offset = LLVMBuildMul(builder, loop_state.counter,
                               lp_build_const_int32(gallivm, depth_bits * type.length),
                               "");

   depth_ptr_i = LLVMBuildGEP(builder, depth_ptr, &depth_offset, 1, "");

   memset(outputs, 0, sizeof outputs);

   for(cbuf = 0; cbuf < key->nr_cbufs; cbuf++) {
      for(chan = 0; chan < TGSI_NUM_CHANNELS; ++chan) {
         out_color[cbuf][chan] = lp_build_array_alloca(gallivm,
                                                       lp_build_vec_type(gallivm,
                                                                         type),
                                                       num_loop, "color");
      }
   }



   /* 'mask' will control execution based on quad's pixel alive/killed state */
   lp_build_mask_begin(&mask, gallivm, type, mask_val);

   if (!(depth_mode & EARLY_DEPTH_TEST) && !simple_shader)
      lp_build_mask_check(&mask);

   lp_build_interp_soa_update_pos_dyn(interp, gallivm, loop_state.counter);
   z = interp->pos[2];

   if (depth_mode & EARLY_DEPTH_TEST) {
      lp_build_depth_stencil_test(gallivm,
                                  &key->depth,
                                  key->stencil,
                                  type,
                                  zs_format_desc,
                                  &mask,
                                  stencil_refs,
                                  z,
                                  depth_ptr_i, facing,
                                  &zs_value,
                                  !simple_shader);

      if (depth_mode & EARLY_DEPTH_WRITE) {
         lp_build_depth_write(builder, zs_format_desc, depth_ptr_i, zs_value);
      }
   }

   lp_build_interp_soa_update_inputs_dyn(interp, gallivm, loop_state.counter);

   /* Build the actual shader */
   lp_build_tgsi_soa(gallivm, tokens, type, &mask,
                     consts_ptr, &system_values,
                     interp->pos, interp->inputs,
                     outputs, sampler, &shader->info.base);

   /* Alpha test */
   if (key->alpha.enabled) {
      int color0 = find_output_by_semantic(&shader->info.base,
                                           TGSI_SEMANTIC_COLOR,
                                           0);

      if (color0 != -1 && outputs[color0][3]) {
         const struct util_format_description *cbuf_format_desc;
         LLVMValueRef alpha = LLVMBuildLoad(builder, outputs[color0][3], "alpha");
         LLVMValueRef alpha_ref_value;

         alpha_ref_value = lp_jit_context_alpha_ref_value(gallivm, context_ptr);
         alpha_ref_value = lp_build_broadcast(gallivm, vec_type, alpha_ref_value);

         cbuf_format_desc = util_format_description(key->cbuf_format[0]);

         lp_build_alpha_test(gallivm, key->alpha.func, type, cbuf_format_desc,
                             &mask, alpha, alpha_ref_value,
                             (depth_mode & LATE_DEPTH_TEST) != 0);
      }
   }

   /* Late Z test */
   if (depth_mode & LATE_DEPTH_TEST) {
      int pos0 = find_output_by_semantic(&shader->info.base,
                                         TGSI_SEMANTIC_POSITION,
                                         0);

      if (pos0 != -1 && outputs[pos0][2]) {
         z = LLVMBuildLoad(builder, outputs[pos0][2], "output.z");
      }

      lp_build_depth_stencil_test(gallivm,
                                  &key->depth,
                                  key->stencil,
                                  type,
                                  zs_format_desc,
                                  &mask,
                                  stencil_refs,
                                  z,
                                  depth_ptr_i, facing,
                                  &zs_value,
                                  !simple_shader);
      /* Late Z write */
      if (depth_mode & LATE_DEPTH_WRITE) {
         lp_build_depth_write(builder, zs_format_desc, depth_ptr_i, zs_value);
      }
   }
   else if ((depth_mode & EARLY_DEPTH_TEST) &&
            (depth_mode & LATE_DEPTH_WRITE))
   {
      /* Need to apply a reduced mask to the depth write.  Reload the
       * depth value, update from zs_value with the new mask value and
       * write that out.
       */
      lp_build_deferred_depth_write(gallivm,
                                    type,
                                    zs_format_desc,
                                    &mask,
                                    depth_ptr_i,
                                    zs_value);
   }


   /* Color write  */
   for (attrib = 0; attrib < shader->info.base.num_outputs; ++attrib)
   {
      if (shader->info.base.output_semantic_name[attrib] == TGSI_SEMANTIC_COLOR &&
          shader->info.base.output_semantic_index[attrib] < key->nr_cbufs)
      {
         unsigned cbuf = shader->info.base.output_semantic_index[attrib];
         for(chan = 0; chan < TGSI_NUM_CHANNELS; ++chan) {
            if(outputs[attrib][chan]) {
               /* XXX: just initialize outputs to point at colors[] and
                * skip this.
                */
               LLVMValueRef out = LLVMBuildLoad(builder, outputs[attrib][chan], "");
               LLVMValueRef color_ptr;
               color_ptr = LLVMBuildGEP(builder, out_color[cbuf][chan],
                                        &loop_state.counter, 1, "");
               lp_build_name(out, "color%u.%c", attrib, "rgba"[chan]);
               LLVMBuildStore(builder, out, color_ptr);
            }
         }
      }
   }

   if (key->occlusion_count) {
      lp_build_name(counter, "counter");
      lp_build_occlusion_count(gallivm, type,
                               lp_build_mask_value(&mask), counter);
   }

   mask_val = lp_build_mask_end(&mask);
   LLVMBuildStore(builder, mask_val, mask_ptr);
   lp_build_for_loop_end(&loop_state);
}


/**
 * This function will reorder pixels from the fragment shader SoA to memory layout AoS
 *
 * Fragment Shader outputs pixels in small 2x2 blocks
 *  e.g. (0, 0), (1, 0), (0, 1), (1, 1) ; (2, 0) ...
 *
 * However in memory pixels are stored in rows
 *  e.g. (0, 0), (1, 0), (2, 0), (3, 0) ; (0, 1) ...
 *
 * @param type            fragment shader type (4x or 8x float)
 * @param num_fs          number of fs_src
 * @param dst_channels    number of output channels
 * @param fs_src          output from fragment shader
 * @param dst             pointer to store result
 * @param pad_inline      is channel padding inline or at end of row
 * @return                the number of dsts
 */
static int
generate_fs_twiddle(struct gallivm_state *gallivm,
                    struct lp_type type,
                    unsigned num_fs,
                    unsigned dst_channels,
                    LLVMValueRef fs_src[][4],
                    LLVMValueRef* dst,
                    bool pad_inline)
{
   LLVMValueRef src[16];

   bool swizzle_pad;
   bool twiddle;
   bool split;

   unsigned pixels = num_fs == 4 ? 1 : 2;
   unsigned reorder_group;
   unsigned src_channels;
   unsigned src_count;
   unsigned i;

   src_channels = dst_channels < 3 ? dst_channels : 4;
   src_count = num_fs * src_channels;

   assert(pixels == 2 || num_fs == 4);
   assert(num_fs * src_channels <= Elements(src));

   /*
    * Transpose from SoA -> AoS
    */
   for (i = 0; i < num_fs; ++i) {
      lp_build_transpose_aos_n(gallivm, type, &fs_src[i][0], src_channels, &src[i * src_channels]);
   }

   /*
    * Pick transformation options
    */
   swizzle_pad = false;
   twiddle = false;
   split = false;
   reorder_group = 0;

   if (dst_channels == 1) {
      twiddle = true;

      if (pixels == 2) {
         split = true;
      }
   } else if (dst_channels == 2) {
      if (pixels == 1) {
         reorder_group = 1;
      }
   } else if (dst_channels > 2) {
      if (pixels == 1) {
         reorder_group = 2;
      } else {
         twiddle = true;
      }

      if (!pad_inline && dst_channels == 3 && pixels > 1) {
         swizzle_pad = true;
      }
   }

   /*
    * Split the src in half
    */
   if (split) {
      for (i = num_fs; i > 0; --i) {
         src[(i - 1)*2 + 1] = lp_build_extract_range(gallivm, src[i - 1], 4, 4);
         src[(i - 1)*2 + 0] = lp_build_extract_range(gallivm, src[i - 1], 0, 4);
      }

      src_count *= 2;
      type.length = 4;
   }

   /*
    * Ensure pixels are in memory order
    */
   if (reorder_group) {
      /* Twiddle pixels by reordering the array, e.g.:
       *
       * src_count =  8 -> 0 2 1 3 4 6 5 7
       * src_count = 16 -> 0 1 4 5 2 3 6 7 8 9 12 13 10 11 14 15
       */
      const unsigned reorder_sw[] = { 0, 2, 1, 3 };

      for (i = 0; i < src_count; ++i) {
         unsigned group = i / reorder_group;
         unsigned block = (group / 4) * 4 * reorder_group;
         unsigned j = block + (reorder_sw[group % 4] * reorder_group) + (i % reorder_group);
         dst[i] = src[j];
      }
   } else if (twiddle) {
      /* Twiddle pixels across elements of array */
      lp_bld_quad_twiddle(gallivm, type, src, src_count, dst);
   } else {
      /* Do nothing */
      memcpy(dst, src, sizeof(LLVMValueRef) * src_count);
   }

   /*
    * Moves any padding between pixels to the end
    * e.g. RGBXRGBX -> RGBRGBXX
    */
   if (swizzle_pad) {
      unsigned char swizzles[16];
      unsigned elems = pixels * dst_channels;

      for (i = 0; i < type.length; ++i) {
         if (i < elems)
            swizzles[i] = i % dst_channels + (i / dst_channels) * 4;
         else
            swizzles[i] = LP_BLD_SWIZZLE_DONTCARE;
      }

      for (i = 0; i < src_count; ++i) {
         dst[i] = lp_build_swizzle_aos_n(gallivm, dst[i], swizzles, type.length, type.length);
      }
   }

   return src_count;
}


/**
 * Load an unswizzled block of pixels from memory
 */
static void
load_unswizzled_block(struct gallivm_state *gallivm,
                      LLVMValueRef base_ptr,
                      LLVMValueRef stride,
                      unsigned block_width,
                      unsigned block_height,
                      LLVMValueRef* dst,
                      struct lp_type dst_type,
                      unsigned dst_count,
                      unsigned dst_alignment)
{
   LLVMBuilderRef builder = gallivm->builder;
   unsigned row_size = dst_count / block_height;
   unsigned i;

   /* Ensure block exactly fits into dst */
   assert((block_width * block_height) % dst_count == 0);

   for (i = 0; i < dst_count; ++i) {
      unsigned x = i % row_size;
      unsigned y = i / row_size;

      LLVMValueRef bx = lp_build_const_int32(gallivm, x * (dst_type.width / 8) * dst_type.length);
      LLVMValueRef by = LLVMBuildMul(builder, lp_build_const_int32(gallivm, y), stride, "");

      LLVMValueRef gep[2];
      LLVMValueRef dst_ptr;

      gep[0] = lp_build_const_int32(gallivm, 0);
      gep[1] = LLVMBuildAdd(builder, bx, by, "");

      dst_ptr = LLVMBuildGEP(builder, base_ptr, gep, 2, "");
      dst_ptr = LLVMBuildBitCast(builder, dst_ptr, LLVMPointerType(lp_build_vec_type(gallivm, dst_type), 0), "");

      dst[i] = LLVMBuildLoad(builder, dst_ptr, "");

      lp_set_load_alignment(dst[i], dst_alignment);
   }
}


/**
 * Store an unswizzled block of pixels to memory
 */
static void
store_unswizzled_block(struct gallivm_state *gallivm,
                       LLVMValueRef base_ptr,
                       LLVMValueRef stride,
                       unsigned block_width,
                       unsigned block_height,
                       LLVMValueRef* src,
                       struct lp_type src_type,
                       unsigned src_count,
                       unsigned src_alignment)
{
   LLVMBuilderRef builder = gallivm->builder;
   unsigned row_size = src_count / block_height;
   unsigned i;

   /* Ensure src exactly fits into block */
   assert((block_width * block_height) % src_count == 0);

   for (i = 0; i < src_count; ++i) {
      unsigned x = i % row_size;
      unsigned y = i / row_size;

      LLVMValueRef bx = lp_build_const_int32(gallivm, x * (src_type.width / 8) * src_type.length);
      LLVMValueRef by = LLVMBuildMul(builder, lp_build_const_int32(gallivm, y), stride, "");

      LLVMValueRef gep[2];
      LLVMValueRef src_ptr;

      gep[0] = lp_build_const_int32(gallivm, 0);
      gep[1] = LLVMBuildAdd(builder, bx, by, "");

      src_ptr = LLVMBuildGEP(builder, base_ptr, gep, 2, "");
      src_ptr = LLVMBuildBitCast(builder, src_ptr, LLVMPointerType(lp_build_vec_type(gallivm, src_type), 0), "");

      src_ptr = LLVMBuildStore(builder, src[i], src_ptr);

      lp_set_store_alignment(src_ptr, src_alignment);
   }
}


/**
 * Checks if a format description is an arithmetic format
 *
 * A format which has irregular channel sizes such as R3_G3_B2 or R5_G6_B5.
 */
static INLINE boolean
is_arithmetic_format(const struct util_format_description *format_desc)
{
   boolean arith = false;
   unsigned i;

   for (i = 0; i < format_desc->nr_channels; ++i) {
      arith |= format_desc->channel[i].size != format_desc->channel[0].size;
      arith |= (format_desc->channel[i].size % 8) != 0;
   }

   return arith;
}


/**
 * Retrieves the type representing the memory layout for a format
 *
 * e.g. RGBA16F = 4x half-float and R3G3B2 = 1x byte
 */
static INLINE void
lp_mem_type_from_format_desc(const struct util_format_description *format_desc,
                             struct lp_type* type)
{
   int i;

   memset(type, 0, sizeof(struct lp_type));
   type->floating = format_desc->channel[0].type == UTIL_FORMAT_TYPE_FLOAT;
   type->fixed    = format_desc->channel[0].type == UTIL_FORMAT_TYPE_FIXED;
   type->sign     = format_desc->channel[0].type != UTIL_FORMAT_TYPE_UNSIGNED;
   type->norm     = format_desc->channel[0].normalized;

   if (is_arithmetic_format(format_desc)) {
      type->width = 0;
      type->length = 1;

      for (i = 0; i < format_desc->nr_channels; ++i) {
         type->width += format_desc->channel[i].size;
      }
   } else {
      type->width = format_desc->channel[0].size;
      type->length = format_desc->nr_channels;
   }
}


/**
 * Retrieves the type for a format which is usable in the blending code.
 *
 * e.g. RGBA16F = 4x float, R3G3B2 = 3x byte
 */
static INLINE void
lp_blend_type_from_format_desc(const struct util_format_description *format_desc,
                               struct lp_type* type)
{
   int i;

   memset(type, 0, sizeof(struct lp_type));
   type->floating = format_desc->channel[0].type == UTIL_FORMAT_TYPE_FLOAT;
   type->fixed    = format_desc->channel[0].type == UTIL_FORMAT_TYPE_FIXED;
   type->sign     = format_desc->channel[0].type != UTIL_FORMAT_TYPE_UNSIGNED;
   type->norm     = format_desc->channel[0].normalized;
   type->width    = format_desc->channel[0].size;
   type->length   = format_desc->nr_channels;

   for (i = 1; i < format_desc->nr_channels; ++i) {
      if (format_desc->channel[i].size > type->width)
         type->width = format_desc->channel[i].size;
   }

   if (type->floating) {
      type->width = 32;
   } else {
      if (type->width <= 8) {
         type->width = 8;
      } else if (type->width <= 16) {
         type->width = 16;
      } else {
         type->width = 32;
      }
   }

   if (is_arithmetic_format(format_desc) && type->length == 3) {
      type->length = 4;
   }
}


/**
 * Scale a normalized value from src_bits to dst_bits
 */
static INLINE LLVMValueRef
scale_bits(struct gallivm_state *gallivm,
           int src_bits,
           int dst_bits,
           LLVMValueRef src,
           struct lp_type src_type)
{
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef result = src;

   if (dst_bits < src_bits) {
      /* Scale down by LShr */
      result = LLVMBuildLShr(builder,
                             src,
                             lp_build_const_int_vec(gallivm, src_type, src_bits - dst_bits),
                             "");
   } else if (dst_bits > src_bits) {
      /* Scale up bits */
      int db = dst_bits - src_bits;

      /* Shift left by difference in bits */
      result = LLVMBuildShl(builder,
                            src,
                            lp_build_const_int_vec(gallivm, src_type, db),
                            "");

      if (db < src_bits) {
         /* Enough bits in src to fill the remainder */
         LLVMValueRef lower = LLVMBuildLShr(builder,
                                            src,
                                            lp_build_const_int_vec(gallivm, src_type, src_bits - db),
                                            "");

         result = LLVMBuildOr(builder, result, lower, "");
      } else if (db > src_bits) {
         /* Need to repeatedly copy src bits to fill remainder in dst */
         unsigned n;

         for (n = src_bits; n < dst_bits; n *= 2) {
            LLVMValueRef shuv = lp_build_const_int_vec(gallivm, src_type, n);

            result = LLVMBuildOr(builder,
                                 result,
                                 LLVMBuildLShr(builder, result, shuv, ""),
                                 "");
         }
      }
   }

   return result;
}


/**
 * Convert from memory format to blending format
 *
 * e.g. GL_R3G3B2 is 1 byte in memory but 3 bytes for blending
 */
static void
convert_to_blend_type(struct gallivm_state *gallivm,
                      const struct util_format_description *src_fmt,
                      struct lp_type src_type,
                      struct lp_type dst_type,
                      LLVMValueRef* src, // and dst
                      unsigned num_srcs)
{
   LLVMValueRef *dst = src;
   LLVMBuilderRef builder = gallivm->builder;
   struct lp_type blend_type;
   struct lp_type mem_type;
   unsigned i, j, k;
   unsigned pixels = 16 / num_srcs;
   bool is_arith;

   lp_mem_type_from_format_desc(src_fmt, &mem_type);
   lp_blend_type_from_format_desc(src_fmt, &blend_type);

   /* Is the format arithmetic */
   is_arith = blend_type.length * blend_type.width != mem_type.width * mem_type.length;
   is_arith &= !(mem_type.width == 16 && mem_type.floating);

   /* Pad if necessary */
   if (!is_arith && src_type.length < dst_type.length) {
      for (i = 0; i < num_srcs; ++i) {
         dst[i] = lp_build_pad_vector(gallivm, src[i], dst_type.length);
      }

      src_type.length = dst_type.length;
   }

   /* Special case for half-floats */
   if (mem_type.width == 16 && mem_type.floating) {
      assert(blend_type.width == 32 && blend_type.floating);
      lp_build_conv_auto(gallivm, src_type, &dst_type, dst, num_srcs, dst);
      is_arith = false;
   }

   if (!is_arith) {
      return;
   }

   src_type.width = blend_type.width * blend_type.length;
   blend_type.length *= pixels;
   src_type.length *= pixels / (src_type.length / mem_type.length);

   for (i = 0; i < num_srcs; ++i) {
      LLVMValueRef chans[4];
      LLVMValueRef res;
      unsigned sa = 0;

      dst[i] = LLVMBuildZExt(builder, src[i], lp_build_vec_type(gallivm, src_type), "");

      for (j = 0; j < src_fmt->nr_channels; ++j) {
         unsigned mask = 0;

         for (k = 0; k < src_fmt->channel[j].size; ++k) {
            mask |= 1 << k;
         }

         /* Extract bits from source */
         chans[j] = LLVMBuildLShr(builder,
                                  dst[i],
                                  lp_build_const_int_vec(gallivm, src_type, sa),
                                  "");

         chans[j] = LLVMBuildAnd(builder,
                                 chans[j],
                                 lp_build_const_int_vec(gallivm, src_type, mask),
                                 "");

         /* Scale bits */
         if (src_type.norm) {
            chans[j] = scale_bits(gallivm, src_fmt->channel[j].size,
                                  blend_type.width, chans[j], src_type);
         }

         /* Insert bits into correct position */
         chans[j] = LLVMBuildShl(builder,
                                 chans[j],
                                 lp_build_const_int_vec(gallivm, src_type, j * blend_type.width),
                                 "");

         sa += src_fmt->channel[j].size;

         if (j == 0) {
            res = chans[j];
         } else {
            res = LLVMBuildOr(builder, res, chans[j], "");
         }
      }

      dst[i] = LLVMBuildBitCast(builder, res, lp_build_vec_type(gallivm, blend_type), "");
   }
}


/**
 * Convert from blending format to memory format
 *
 * e.g. GL_R3G3B2 is 3 bytes for blending but 1 byte in memory
 */
static void
convert_from_blend_type(struct gallivm_state *gallivm,
                        const struct util_format_description *src_fmt,
                        struct lp_type src_type,
                        struct lp_type dst_type,
                        LLVMValueRef* src, // and dst
                        unsigned num_srcs)
{
   LLVMValueRef* dst = src;
   unsigned i, j, k;
   struct lp_type mem_type;
   struct lp_type blend_type;
   LLVMBuilderRef builder = gallivm->builder;
   unsigned pixels = 16 / num_srcs;
   bool is_arith;

   lp_mem_type_from_format_desc(src_fmt, &mem_type);
   lp_blend_type_from_format_desc(src_fmt, &blend_type);

   is_arith = (blend_type.length * blend_type.width != mem_type.width * mem_type.length);

   /* Special case for half-floats */
   if (mem_type.width == 16 && mem_type.floating) {
      int length = dst_type.length;
      assert(blend_type.width == 32 && blend_type.floating);

      dst_type.length = src_type.length;

      lp_build_conv_auto(gallivm, src_type, &dst_type, dst, num_srcs, dst);

      dst_type.length = length;
      is_arith = false;
   }

   /* Remove any padding */
   if (!is_arith && (src_type.length % mem_type.length)) {
      src_type.length -= (src_type.length % mem_type.length);

      for (i = 0; i < num_srcs; ++i) {
         dst[i] = lp_build_extract_range(gallivm, dst[i], 0, src_type.length);
      }
   }

   /* No bit arithmetic to do */
   if (!is_arith) {
      return;
   }

   src_type.length = pixels;
   src_type.width = blend_type.length * blend_type.width;
   dst_type.length = pixels;

   for (i = 0; i < num_srcs; ++i) {
      LLVMValueRef chans[4];
      LLVMValueRef res;
      unsigned sa = 0;

      dst[i] = LLVMBuildBitCast(builder, src[i], lp_build_vec_type(gallivm, src_type), "");

      for (j = 0; j < src_fmt->nr_channels; ++j) {
         unsigned mask = 0;

         assert(blend_type.width > src_fmt->channel[j].size);

         for (k = 0; k < blend_type.width; ++k) {
            mask |= 1 << k;
         }

         /* Extract bits */
         chans[j] = LLVMBuildLShr(builder,
                                  dst[i],
                                  lp_build_const_int_vec(gallivm, src_type, j * blend_type.width),
                                  "");

         chans[j] = LLVMBuildAnd(builder,
                                 chans[j],
                                 lp_build_const_int_vec(gallivm, src_type, mask),
                                 "");

         /* Scale down bits */
         if (src_type.norm) {
            chans[j] = scale_bits(gallivm, blend_type.width,
                                  src_fmt->channel[j].size, chans[j], src_type);
         }

         /* Insert bits */
         chans[j] = LLVMBuildShl(builder,
                                 chans[j],
                                 lp_build_const_int_vec(gallivm, src_type, sa),
                                 "");

         sa += src_fmt->channel[j].size;

         if (j == 0) {
            res = chans[j];
         } else {
            res = LLVMBuildOr(builder, res, chans[j], "");
         }
      }

      assert (dst_type.width != 24);

      dst[i] = LLVMBuildTrunc(builder, res, lp_build_vec_type(gallivm, dst_type), "");
   }
}


/**
 * Generates the blend function for unswizzled colour buffers
 * Also generates the read & write from colour buffer
 */
static void
generate_unswizzled_blend(struct gallivm_state *gallivm,
                          unsigned rt,
                          struct lp_fragment_shader_variant *variant,
                          enum pipe_format out_format,
                          unsigned int num_fs,
                          struct lp_type fs_type,
                          LLVMValueRef* fs_mask,
                          LLVMValueRef fs_out_color[TGSI_NUM_CHANNELS][4],
                          LLVMValueRef context_ptr,
                          LLVMValueRef color_ptr,
                          LLVMValueRef stride,
                          unsigned partial_mask,
                          boolean do_branch)
{
   const unsigned alpha_channel = 3;
   const unsigned block_width = 4;
   const unsigned block_height = 4;
   const unsigned block_size = block_width * block_height;
   const unsigned lp_integer_vector_width = 128;

   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef fs_src[4][TGSI_NUM_CHANNELS];
   LLVMValueRef src_alpha[4 * 4];
   LLVMValueRef src_mask[4 * 4];
   LLVMValueRef src[4 * 4];
   LLVMValueRef dst[4 * 4];
   LLVMValueRef blend_color;
   LLVMValueRef blend_alpha;
   LLVMValueRef i32_zero;
   LLVMValueRef check_mask;

   struct lp_build_mask_context mask_ctx;
   struct lp_type mask_type;
   struct lp_type blend_type;
   struct lp_type alpha_type;
   struct lp_type row_type;
   struct lp_type dst_type;

   unsigned char swizzle[TGSI_NUM_CHANNELS];
   unsigned vector_width;
   unsigned dst_channels;
   unsigned src_channels;
   unsigned dst_count;
   unsigned src_count;
   unsigned i, j;

   const struct util_format_description* out_format_desc = util_format_description(out_format);

   unsigned dst_alignment;

   bool pad_inline = is_arithmetic_format(out_format_desc);
   bool has_alpha = false;

   src_channels = TGSI_NUM_CHANNELS;
   mask_type = lp_int32_vec4_type();
   mask_type.length = fs_type.length;

   /* Compute the alignment of the destination pointer in bytes */
#if 0
   dst_alignment = (block_width * out_format_desc->block.bits + 7)/(out_format_desc->block.width * 8);
#else
   /* FIXME -- currently we're fetching pixels one by one, instead of row by row */
   dst_alignment = (1 * out_format_desc->block.bits + 7)/(out_format_desc->block.width * 8);
#endif
   /* Force power-of-two alignment by extracting only the least-significant-bit */
   dst_alignment = 1 << (ffs(dst_alignment) - 1);
   /* Resource base and stride pointers are aligned to 16 bytes, so that's the maximum alignment we can guarantee */
   dst_alignment = MIN2(dst_alignment, 16);

   /* Do not bother executing code when mask is empty.. */
   if (do_branch) {
      check_mask = LLVMConstNull(lp_build_int_vec_type(gallivm, mask_type));

      for (i = 0; i < num_fs; ++i) {
         check_mask = LLVMBuildOr(builder, check_mask, fs_mask[i], "");
      }

      lp_build_mask_begin(&mask_ctx, gallivm, mask_type, check_mask);
      lp_build_mask_check(&mask_ctx);
   }

   partial_mask |= !variant->opaque;
   i32_zero = lp_build_const_int32(gallivm, 0);

   /* Get type from output format */
   lp_blend_type_from_format_desc(out_format_desc, &row_type);
   lp_mem_type_from_format_desc(out_format_desc, &dst_type);

   row_type.length = fs_type.length;
   vector_width    = dst_type.floating ? lp_native_vector_width : lp_integer_vector_width;

   /* Compute correct swizzle and count channels */
   memset(swizzle, 0xFF, TGSI_NUM_CHANNELS);
   dst_channels = 0;

   for (i = 0; i < TGSI_NUM_CHANNELS; ++i) {
      /* Ensure channel is used */
      if (out_format_desc->swizzle[i] >= TGSI_NUM_CHANNELS) {
         continue;
      }

      /* Ensure not already written to (happens in case with GL_ALPHA) */
      if (swizzle[out_format_desc->swizzle[i]] < TGSI_NUM_CHANNELS) {
         continue;
      }

      /* Ensure we havn't already found all channels */
      if (dst_channels >= out_format_desc->nr_channels) {
         continue;
      }

      swizzle[out_format_desc->swizzle[i]] = i;
      ++dst_channels;

      if (i == alpha_channel) {
         has_alpha = true;
      }
   }

   /* If 3 channels then pad to include alpha for 4 element transpose */
   if (dst_channels == 3 && !has_alpha) {
      swizzle[3] = 3;

      if (out_format_desc->nr_channels == 4) {
         dst_channels = 4;
      }
   }

   /*
    * Load shader output
    */
   for (i = 0; i < num_fs; ++i) {
      /* Always load alpha for use in blending */
      LLVMValueRef alpha = LLVMBuildLoad(builder, fs_out_color[alpha_channel][i], "");

      /* Load each channel */
      for (j = 0; j < dst_channels; ++j) {
         fs_src[i][j] = LLVMBuildLoad(builder, fs_out_color[swizzle[j]][i], "");
      }

      /* If 3 channels then pad to include alpha for 4 element transpose */
      if (dst_channels == 3 && !has_alpha) {
         fs_src[i][3] = alpha;
         swizzle[3] = 3;
      }

      /* We split the row_mask and row_alpha as we want 128bit interleave */
      if (fs_type.length == 8) {
         src_mask[i*2 + 0]  = lp_build_extract_range(gallivm, fs_mask[i], 0, src_channels);
         src_mask[i*2 + 1]  = lp_build_extract_range(gallivm, fs_mask[i], src_channels, src_channels);

         src_alpha[i*2 + 0] = lp_build_extract_range(gallivm, alpha, 0, src_channels);
         src_alpha[i*2 + 1] = lp_build_extract_range(gallivm, alpha, src_channels, src_channels);
      } else {
         src_mask[i] = fs_mask[i];
         src_alpha[i] = alpha;
      }
   }

   if (util_format_is_pure_integer(out_format)) {
      /*
       * In this case fs_type was really ints or uints disguised as floats,
       * fix that up now.
       */
      fs_type.floating = 0;
      fs_type.sign = dst_type.sign;
      for (i = 0; i < num_fs; ++i) {
         for (j = 0; j < dst_channels; ++j) {
            fs_src[i][j] = LLVMBuildBitCast(builder, fs_src[i][j],
                                            lp_build_vec_type(gallivm, fs_type), "");
         }
         if (dst_channels == 3 && !has_alpha) {
            fs_src[i][3] = LLVMBuildBitCast(builder, fs_src[i][3],
                                            lp_build_vec_type(gallivm, fs_type), "");
         }
      }
   }


   /*
    * Pixel twiddle from fragment shader order to memory order
    */
   src_count = generate_fs_twiddle(gallivm, fs_type, num_fs, dst_channels, fs_src, src, pad_inline);
   src_channels = dst_channels < 3 ? dst_channels : 4;
   if (src_count != num_fs * src_channels) {
      unsigned ds = src_count / (num_fs * src_channels);
      row_type.length /= ds;
      fs_type.length = row_type.length;
   }

   blend_type = row_type;
   alpha_type = fs_type;
   alpha_type.length = 4;
   mask_type.length = 4;

   /* Convert src to row_type */
   src_count = lp_build_conv_auto(gallivm, fs_type, &row_type, src, src_count, src);

   /* If the rows are not an SSE vector, combine them to become SSE size! */
   if ((row_type.width * row_type.length) % 128) {
      unsigned bits = row_type.width * row_type.length;
      unsigned combined;

      dst_count = src_count / (vector_width / bits);
      combined = lp_build_concat_n(gallivm, row_type, src, src_count, src, dst_count);

      row_type.length *= combined;
      src_count /= combined;

      bits = row_type.width * row_type.length;
      assert(bits == 128 || bits == 256);
   }


   /*
    * Blend Colour conversion
    */
   blend_color = lp_jit_context_f_blend_color(gallivm, context_ptr);
   blend_color = LLVMBuildPointerCast(builder, blend_color, LLVMPointerType(lp_build_vec_type(gallivm, fs_type), 0), "");
   blend_color = LLVMBuildLoad(builder, LLVMBuildGEP(builder, blend_color, &i32_zero, 1, ""), "");

   /* Convert */
   lp_build_conv(gallivm, fs_type, blend_type, &blend_color, 1, &blend_color, 1);

   /* Extract alpha */
   blend_alpha = lp_build_extract_broadcast(gallivm, blend_type, row_type, blend_color, lp_build_const_int32(gallivm, 3));

   /* Swizzle to appropriate channels, e.g. from RGBA to BGRA BGRA */
   pad_inline &= (dst_channels * (block_size / src_count) * row_type.width) != vector_width;
   if (pad_inline) {
      /* Use all 4 channels e.g. from RGBA RGBA to RGxx RGxx */
      blend_color = lp_build_swizzle_aos_n(gallivm, blend_color, swizzle, TGSI_NUM_CHANNELS, row_type.length);
   } else {
      /* Only use dst_channels e.g. RGBA RGBA to RG RG xxxx */
      blend_color = lp_build_swizzle_aos_n(gallivm, blend_color, swizzle, dst_channels, row_type.length);
   }

   /*
    * Mask conversion
    */
   lp_bld_quad_twiddle(gallivm, mask_type, &src_mask[0], 4, &src_mask[0]);

   if (src_count < block_height) {
      lp_build_concat_n(gallivm, mask_type, src_mask, 4, src_mask, src_count);
   } else if (src_count > block_height) {
      for (i = src_count; i > 0; --i) {
         unsigned pixels = block_size / src_count;
         unsigned idx = i - 1;

         src_mask[idx] = lp_build_extract_range(gallivm, src_mask[(idx * pixels) / 4], (idx * pixels) % 4, pixels);
      }
   }

   assert(mask_type.width == 32);

   for (i = 0; i < src_count; ++i) {
      unsigned pixels = block_size / src_count;
      unsigned pixel_width = row_type.width * dst_channels;

      if (pixel_width == 24) {
         mask_type.width = 8;
         mask_type.length = vector_width / mask_type.width;
      } else {
         mask_type.length = pixels;
         mask_type.width = row_type.width * dst_channels;

         src_mask[i] = LLVMBuildIntCast(builder, src_mask[i], lp_build_int_vec_type(gallivm, mask_type), "");

         mask_type.length *= dst_channels;
         mask_type.width /= dst_channels;
      }

      src_mask[i] = LLVMBuildBitCast(builder, src_mask[i], lp_build_int_vec_type(gallivm, mask_type), "");
      src_mask[i] = lp_build_pad_vector(gallivm, src_mask[i], row_type.length);
   }

   /*
    * Alpha conversion
    */
   if (!has_alpha) {
      unsigned length = row_type.length;
      row_type.length = alpha_type.length;

      /* Twiddle the alpha to match pixels */
      lp_bld_quad_twiddle(gallivm, alpha_type, src_alpha, 4, src_alpha);

      for (i = 0; i < 4; ++i) {
         lp_build_conv(gallivm, alpha_type, row_type, &src_alpha[i], 1, &src_alpha[i], 1);
      }

      alpha_type = row_type;
      row_type.length = length;

      /* If only one channel we can only need the single alpha value per pixel */
      if (src_count == 1) {
         assert(dst_channels == 1);

         lp_build_concat_n(gallivm, alpha_type, src_alpha, 4, src_alpha, src_count);
      } else {
         /* If there are more srcs than rows then we need to split alpha up */
         if (src_count > block_height) {
            for (i = src_count; i > 0; --i) {
               unsigned pixels = block_size / src_count;
               unsigned idx = i - 1;

               src_alpha[idx] = lp_build_extract_range(gallivm, src_alpha[(idx * pixels) / 4], (idx * pixels) % 4, pixels);
            }
         }

         /* If there is a src for each pixel broadcast the alpha across whole row */
         if (src_count == block_size) {
            for (i = 0; i < src_count; ++i) {
               src_alpha[i] = lp_build_broadcast(gallivm, lp_build_vec_type(gallivm, row_type), src_alpha[i]);
            }
         } else {
            unsigned pixels = block_size / src_count;
            unsigned channels = pad_inline ? TGSI_NUM_CHANNELS : dst_channels;
            unsigned alpha_span = 1;
            LLVMValueRef shuffles[LP_MAX_VECTOR_LENGTH];

            /* Check if we need 2 src_alphas for our shuffles */
            if (pixels > alpha_type.length) {
               alpha_span = 2;
            }

            /* Broadcast alpha across all channels, e.g. a1a2 to a1a1a1a1a2a2a2a2 */
            for (j = 0; j < row_type.length; ++j) {
               if (j < pixels * channels) {
                  shuffles[j] = lp_build_const_int32(gallivm, j / channels);
               } else {
                  shuffles[j] = LLVMGetUndef(LLVMInt32TypeInContext(gallivm->context));
               }
            }

            for (i = 0; i < src_count; ++i) {
               unsigned idx1 = i, idx2 = i;

               if (alpha_span > 1){
                  idx1 *= alpha_span;
                  idx2 = idx1 + 1;
               }

               src_alpha[i] = LLVMBuildShuffleVector(builder,
                                                     src_alpha[idx1],
                                                     src_alpha[idx2],
                                                     LLVMConstVector(shuffles, row_type.length),
                                                     "");
            }
         }
      }
   }


   /*
    * Load dst from memory
    */
   if (src_count < block_height) {
      dst_count = block_height;
   } else {
      dst_count = src_count;
   }

   dst_type.length *= 16 / dst_count;

   load_unswizzled_block(gallivm, color_ptr, stride, block_width, block_height,
                         dst, dst_type, dst_count, dst_alignment);


   /*
    * Convert from dst/output format to src/blending format.
    *
    * This is necessary as we can only read 1 row from memory at a time,
    * so the minimum dst_count will ever be at this point is 4.
    *
    * With, for example, R8 format you can have all 16 pixels in a 128 bit vector,
    * this will take the 4 dsts and combine them into 1 src so we can perform blending
    * on all 16 pixels in that single vector at once.
    */
   if (dst_count > src_count) {
      lp_build_concat_n(gallivm, dst_type, dst, 4, dst, src_count);
   }

   /*
    * Blending
    */
   /* XXX this is broken for RGB8 formats -
    * they get expanded from 12 to 16 elements (to include alpha)
    * by convert_to_blend_type then reduced to 15 instead of 12
    * by convert_from_blend_type (a simple fix though breaks A8...).
    * R16G16B16 also crashes differently however something going wrong
    * inside llvm handling npot vector sizes seemingly.
    * It seems some cleanup could be done here (like skipping conversion/blend
    * when not needed).
    */
   convert_to_blend_type(gallivm, out_format_desc, dst_type, row_type, dst, src_count);

   for (i = 0; i < src_count; ++i) {
      dst[i] = lp_build_blend_aos(gallivm,
                                  &variant->key.blend,
                                  out_format,
                                  row_type,
                                  rt,
                                  src[i],
                                  has_alpha ? NULL : src_alpha[i],
                                  dst[i],
                                  partial_mask ? src_mask[i] : NULL,
                                  blend_color,
                                  has_alpha ? NULL : blend_alpha,
                                  swizzle,
                                  pad_inline ? 4 : dst_channels);
   }

   convert_from_blend_type(gallivm, out_format_desc, row_type, dst_type, dst, src_count);

   /* Split the blend rows back to memory rows */
   if (dst_count > src_count) {
      row_type.length = dst_type.length * (dst_count / src_count);

      if (src_count == 1) {
         dst[1] = lp_build_extract_range(gallivm, dst[0], row_type.length / 2, row_type.length / 2);
         dst[0] = lp_build_extract_range(gallivm, dst[0], 0, row_type.length / 2);

         row_type.length /= 2;
         src_count *= 2;
      }

      dst[3] = lp_build_extract_range(gallivm, dst[1], row_type.length / 2, row_type.length / 2);
      dst[2] = lp_build_extract_range(gallivm, dst[1], 0, row_type.length / 2);
      dst[1] = lp_build_extract_range(gallivm, dst[0], row_type.length / 2, row_type.length / 2);
      dst[0] = lp_build_extract_range(gallivm, dst[0], 0, row_type.length / 2);

      row_type.length /= 2;
      src_count *= 2;
   }


   /*
    * Store blend result to memory
    */
   store_unswizzled_block(gallivm, color_ptr, stride, block_width, block_height,
                          dst, dst_type, dst_count, dst_alignment);

   if (do_branch) {
      lp_build_mask_end(&mask_ctx);
   }
}


/**
 * Generate the runtime callable function for the whole fragment pipeline.
 * Note that the function which we generate operates on a block of 16
 * pixels at at time.  The block contains 2x2 quads.  Each quad contains
 * 2x2 pixels.
 */
static void
generate_fragment(struct llvmpipe_context *lp,
                  struct lp_fragment_shader *shader,
                  struct lp_fragment_shader_variant *variant,
                  unsigned partial_mask)
{
   struct gallivm_state *gallivm = variant->gallivm;
   const struct lp_fragment_shader_variant_key *key = &variant->key;
   struct lp_shader_input inputs[PIPE_MAX_SHADER_INPUTS];
   char func_name[256];
   struct lp_type fs_type;
   struct lp_type blend_type;
   LLVMTypeRef fs_elem_type;
   LLVMTypeRef blend_vec_type;
   LLVMTypeRef arg_types[12];
   LLVMTypeRef func_type;
   LLVMTypeRef int32_type = LLVMInt32TypeInContext(gallivm->context);
   LLVMTypeRef int8_type = LLVMInt8TypeInContext(gallivm->context);
   LLVMValueRef context_ptr;
   LLVMValueRef x;
   LLVMValueRef y;
   LLVMValueRef a0_ptr;
   LLVMValueRef dadx_ptr;
   LLVMValueRef dady_ptr;
   LLVMValueRef color_ptr_ptr;
   LLVMValueRef stride_ptr;
   LLVMValueRef depth_ptr;
   LLVMValueRef mask_input;
   LLVMValueRef counter = NULL;
   LLVMBasicBlockRef block;
   LLVMBuilderRef builder;
   struct lp_build_sampler_soa *sampler;
   struct lp_build_interp_soa_context interp;
   LLVMValueRef fs_mask[16 / 4];
   LLVMValueRef fs_out_color[PIPE_MAX_COLOR_BUFS][TGSI_NUM_CHANNELS][16 / 4];
   LLVMValueRef function;
   LLVMValueRef facing;
   const struct util_format_description *zs_format_desc;
   unsigned num_fs;
   unsigned i;
   unsigned chan;
   unsigned cbuf;
   boolean cbuf0_write_all;
   boolean try_loop = TRUE;

   assert(lp_native_vector_width / 32 >= 4);

   /* Adjust color input interpolation according to flatshade state:
    */
   memcpy(inputs, shader->inputs, shader->info.base.num_inputs * sizeof inputs[0]);
   for (i = 0; i < shader->info.base.num_inputs; i++) {
      if (inputs[i].interp == LP_INTERP_COLOR) {
	 if (key->flatshade)
	    inputs[i].interp = LP_INTERP_CONSTANT;
	 else
	    inputs[i].interp = LP_INTERP_PERSPECTIVE;
      }
   }

   /* check if writes to cbuf[0] are to be copied to all cbufs */
   cbuf0_write_all = FALSE;
   for (i = 0;i < shader->info.base.num_properties; i++) {
      if (shader->info.base.properties[i].name ==
          TGSI_PROPERTY_FS_COLOR0_WRITES_ALL_CBUFS) {
         cbuf0_write_all = TRUE;
         break;
      }
   }

   /* TODO: actually pick these based on the fs and color buffer
    * characteristics. */

   memset(&fs_type, 0, sizeof fs_type);
   fs_type.floating = TRUE;      /* floating point values */
   fs_type.sign = TRUE;          /* values are signed */
   fs_type.norm = FALSE;         /* values are not limited to [0,1] or [-1,1] */
   fs_type.width = 32;           /* 32-bit float */
   fs_type.length = MIN2(lp_native_vector_width / 32, 16); /* n*4 elements per vector */
   num_fs = 16 / fs_type.length; /* number of loops per 4x4 stamp */

   memset(&blend_type, 0, sizeof blend_type);
   blend_type.floating = FALSE; /* values are integers */
   blend_type.sign = FALSE;     /* values are unsigned */
   blend_type.norm = TRUE;      /* values are in [0,1] or [-1,1] */
   blend_type.width = 8;        /* 8-bit ubyte values */
   blend_type.length = 16;      /* 16 elements per vector */

   /* 
    * Generate the function prototype. Any change here must be reflected in
    * lp_jit.h's lp_jit_frag_func function pointer type, and vice-versa.
    */

   fs_elem_type = lp_build_elem_type(gallivm, fs_type);

   blend_vec_type = lp_build_vec_type(gallivm, blend_type);

   util_snprintf(func_name, sizeof(func_name), "fs%u_variant%u_%s", 
		 shader->no, variant->no, partial_mask ? "partial" : "whole");

   arg_types[0] = variant->jit_context_ptr_type;       /* context */
   arg_types[1] = int32_type;                          /* x */
   arg_types[2] = int32_type;                          /* y */
   arg_types[3] = int32_type;                          /* facing */
   arg_types[4] = LLVMPointerType(fs_elem_type, 0);    /* a0 */
   arg_types[5] = LLVMPointerType(fs_elem_type, 0);    /* dadx */
   arg_types[6] = LLVMPointerType(fs_elem_type, 0);    /* dady */
   arg_types[7] = LLVMPointerType(LLVMPointerType(blend_vec_type, 0), 0);  /* color */
   arg_types[8] = LLVMPointerType(int8_type, 0);       /* depth */
   arg_types[9] = int32_type;                          /* mask_input */
   arg_types[10] = LLVMPointerType(int32_type, 0);     /* counter */
   arg_types[11] = LLVMPointerType(int32_type, 0);     /* stride */

   func_type = LLVMFunctionType(LLVMVoidTypeInContext(gallivm->context),
                                arg_types, Elements(arg_types), 0);

   function = LLVMAddFunction(gallivm->module, func_name, func_type);
   LLVMSetFunctionCallConv(function, LLVMCCallConv);

   variant->function[partial_mask] = function;

   /* XXX: need to propagate noalias down into color param now we are
    * passing a pointer-to-pointer?
    */
   for(i = 0; i < Elements(arg_types); ++i)
      if(LLVMGetTypeKind(arg_types[i]) == LLVMPointerTypeKind)
         LLVMAddAttribute(LLVMGetParam(function, i), LLVMNoAliasAttribute);

   context_ptr  = LLVMGetParam(function, 0);
   x            = LLVMGetParam(function, 1);
   y            = LLVMGetParam(function, 2);
   facing       = LLVMGetParam(function, 3);
   a0_ptr       = LLVMGetParam(function, 4);
   dadx_ptr     = LLVMGetParam(function, 5);
   dady_ptr     = LLVMGetParam(function, 6);
   color_ptr_ptr = LLVMGetParam(function, 7);
   depth_ptr    = LLVMGetParam(function, 8);
   mask_input   = LLVMGetParam(function, 9);
   stride_ptr   = LLVMGetParam(function, 11);

   lp_build_name(context_ptr, "context");
   lp_build_name(x, "x");
   lp_build_name(y, "y");
   lp_build_name(a0_ptr, "a0");
   lp_build_name(dadx_ptr, "dadx");
   lp_build_name(dady_ptr, "dady");
   lp_build_name(color_ptr_ptr, "color_ptr_ptr");
   lp_build_name(depth_ptr, "depth");
   lp_build_name(mask_input, "mask_input");
   lp_build_name(stride_ptr, "stride_ptr");

   if (key->occlusion_count) {
      counter = LLVMGetParam(function, 10);
      lp_build_name(counter, "counter");
   }

   /*
    * Function body
    */

   block = LLVMAppendBasicBlockInContext(gallivm->context, function, "entry");
   builder = gallivm->builder;
   assert(builder);
   LLVMPositionBuilderAtEnd(builder, block);

   /* code generated texture sampling */
   sampler = lp_llvm_sampler_soa_create(key->state, context_ptr);

   zs_format_desc = util_format_description(key->zsbuf_format);

   if (!try_loop) {
      /*
       * The shader input interpolation info is not explicitely baked in the
       * shader key, but everything it derives from (TGSI, and flatshade) is
       * already included in the shader key.
       */
      lp_build_interp_soa_init(&interp,
                               gallivm,
                               shader->info.base.num_inputs,
                               inputs,
                               builder, fs_type,
                               FALSE,
                               a0_ptr, dadx_ptr, dady_ptr,
                               x, y);

      /* loop over quads in the block */
      for(i = 0; i < num_fs; ++i) {
         LLVMValueRef depth_offset = LLVMConstInt(int32_type,
                                                  i*fs_type.length*zs_format_desc->block.bits/8,
                                                  0);
         LLVMValueRef out_color[PIPE_MAX_COLOR_BUFS][TGSI_NUM_CHANNELS];
         LLVMValueRef depth_ptr_i;

         depth_ptr_i = LLVMBuildGEP(builder, depth_ptr, &depth_offset, 1, "");

         generate_fs(gallivm,
                     shader, key,
                     builder,
                     fs_type,
                     context_ptr,
                     i,
                     &interp,
                     sampler,
                     &fs_mask[i], /* output */
                     out_color,
                     depth_ptr_i,
                     facing,
                     partial_mask,
                     mask_input,
                     counter);

         for (cbuf = 0; cbuf < key->nr_cbufs; cbuf++)
            for (chan = 0; chan < TGSI_NUM_CHANNELS; ++chan)
               fs_out_color[cbuf][chan][i] =
                  out_color[cbuf * !cbuf0_write_all][chan];
      }
   }
   else {
      unsigned depth_bits = zs_format_desc->block.bits/8;
      LLVMValueRef num_loop = lp_build_const_int32(gallivm, num_fs);
      LLVMTypeRef mask_type = lp_build_int_vec_type(gallivm, fs_type);
      LLVMValueRef mask_store = lp_build_array_alloca(gallivm, mask_type,
                                                      num_loop, "mask_store");
      LLVMValueRef color_store[PIPE_MAX_COLOR_BUFS][TGSI_NUM_CHANNELS];

      /*
       * The shader input interpolation info is not explicitely baked in the
       * shader key, but everything it derives from (TGSI, and flatshade) is
       * already included in the shader key.
       */
      lp_build_interp_soa_init(&interp,
                               gallivm,
                               shader->info.base.num_inputs,
                               inputs,
                               builder, fs_type,
                               TRUE,
                               a0_ptr, dadx_ptr, dady_ptr,
                               x, y);

      for (i = 0; i < num_fs; i++) {
         LLVMValueRef mask;
         LLVMValueRef indexi = lp_build_const_int32(gallivm, i);
         LLVMValueRef mask_ptr = LLVMBuildGEP(builder, mask_store,
                                              &indexi, 1, "mask_ptr");

         if (partial_mask) {
            mask = generate_quad_mask(gallivm, fs_type,
                                      i*fs_type.length/4, mask_input);
         }
         else {
            mask = lp_build_const_int_vec(gallivm, fs_type, ~0);
         }
         LLVMBuildStore(builder, mask, mask_ptr);
      }

      generate_fs_loop(gallivm,
                       shader, key,
                       builder,
                       fs_type,
                       context_ptr,
                       num_loop,
                       &interp,
                       sampler,
                       mask_store, /* output */
                       color_store,
                       depth_ptr,
                       depth_bits,
                       facing,
                       counter);

      for (i = 0; i < num_fs; i++) {
         LLVMValueRef indexi = lp_build_const_int32(gallivm, i);
         LLVMValueRef ptr = LLVMBuildGEP(builder, mask_store,
                                         &indexi, 1, "");
         fs_mask[i] = LLVMBuildLoad(builder, ptr, "mask");
         /* This is fucked up need to reorganize things */
         for (cbuf = 0; cbuf < key->nr_cbufs; cbuf++) {
            for (chan = 0; chan < TGSI_NUM_CHANNELS; ++chan) {
               ptr = LLVMBuildGEP(builder,
                                  color_store[cbuf * !cbuf0_write_all][chan],
                                  &indexi, 1, "");
               fs_out_color[cbuf][chan][i] = ptr;
            }
         }
      }
   }

   sampler->destroy(sampler);

   /* Loop over color outputs / color buffers to do blending.
    */
   for(cbuf = 0; cbuf < key->nr_cbufs; cbuf++) {
      LLVMValueRef color_ptr;
      LLVMValueRef stride;
      LLVMValueRef index = lp_build_const_int32(gallivm, cbuf);

      boolean do_branch = ((key->depth.enabled
                            || key->stencil[0].enabled
                            || key->alpha.enabled)
                           && !shader->info.base.uses_kill);

      color_ptr = LLVMBuildLoad(builder,
                                LLVMBuildGEP(builder, color_ptr_ptr, &index, 1, ""),
                                "");

      lp_build_name(color_ptr, "color_ptr%d", cbuf);

      stride = LLVMBuildLoad(builder,
                             LLVMBuildGEP(builder, stride_ptr, &index, 1, ""),
                             "");

      generate_unswizzled_blend(gallivm, cbuf, variant, key->cbuf_format[cbuf],
                                num_fs, fs_type, fs_mask, fs_out_color[cbuf],
                                context_ptr, color_ptr, stride, partial_mask, do_branch);
   }

   LLVMBuildRetVoid(builder);

   gallivm_verify_function(gallivm, function);

   variant->nr_instrs += lp_build_count_instructions(function);
}


static void
dump_fs_variant_key(const struct lp_fragment_shader_variant_key *key)
{
   unsigned i;

   debug_printf("fs variant %p:\n", (void *) key);

   if (key->flatshade) {
      debug_printf("flatshade = 1\n");
   }
   for (i = 0; i < key->nr_cbufs; ++i) {
      debug_printf("cbuf_format[%u] = %s\n", i, util_format_name(key->cbuf_format[i]));
   }
   if (key->depth.enabled) {
      debug_printf("depth.format = %s\n", util_format_name(key->zsbuf_format));
      debug_printf("depth.func = %s\n", util_dump_func(key->depth.func, TRUE));
      debug_printf("depth.writemask = %u\n", key->depth.writemask);
   }

   for (i = 0; i < 2; ++i) {
      if (key->stencil[i].enabled) {
         debug_printf("stencil[%u].func = %s\n", i, util_dump_func(key->stencil[i].func, TRUE));
         debug_printf("stencil[%u].fail_op = %s\n", i, util_dump_stencil_op(key->stencil[i].fail_op, TRUE));
         debug_printf("stencil[%u].zpass_op = %s\n", i, util_dump_stencil_op(key->stencil[i].zpass_op, TRUE));
         debug_printf("stencil[%u].zfail_op = %s\n", i, util_dump_stencil_op(key->stencil[i].zfail_op, TRUE));
         debug_printf("stencil[%u].valuemask = 0x%x\n", i, key->stencil[i].valuemask);
         debug_printf("stencil[%u].writemask = 0x%x\n", i, key->stencil[i].writemask);
      }
   }

   if (key->alpha.enabled) {
      debug_printf("alpha.func = %s\n", util_dump_func(key->alpha.func, TRUE));
   }

   if (key->occlusion_count) {
      debug_printf("occlusion_count = 1\n");
   }

   if (key->blend.logicop_enable) {
      debug_printf("blend.logicop_func = %s\n", util_dump_logicop(key->blend.logicop_func, TRUE));
   }
   else if (key->blend.rt[0].blend_enable) {
      debug_printf("blend.rgb_func = %s\n",   util_dump_blend_func  (key->blend.rt[0].rgb_func, TRUE));
      debug_printf("blend.rgb_src_factor = %s\n",   util_dump_blend_factor(key->blend.rt[0].rgb_src_factor, TRUE));
      debug_printf("blend.rgb_dst_factor = %s\n",   util_dump_blend_factor(key->blend.rt[0].rgb_dst_factor, TRUE));
      debug_printf("blend.alpha_func = %s\n",       util_dump_blend_func  (key->blend.rt[0].alpha_func, TRUE));
      debug_printf("blend.alpha_src_factor = %s\n", util_dump_blend_factor(key->blend.rt[0].alpha_src_factor, TRUE));
      debug_printf("blend.alpha_dst_factor = %s\n", util_dump_blend_factor(key->blend.rt[0].alpha_dst_factor, TRUE));
   }
   debug_printf("blend.colormask = 0x%x\n", key->blend.rt[0].colormask);
   for (i = 0; i < key->nr_samplers; ++i) {
      const struct lp_static_sampler_state *sampler = &key->state[i].sampler_state;
      debug_printf("sampler[%u] = \n", i);
      debug_printf("  .wrap = %s %s %s\n",
                   util_dump_tex_wrap(sampler->wrap_s, TRUE),
                   util_dump_tex_wrap(sampler->wrap_t, TRUE),
                   util_dump_tex_wrap(sampler->wrap_r, TRUE));
      debug_printf("  .min_img_filter = %s\n",
                   util_dump_tex_filter(sampler->min_img_filter, TRUE));
      debug_printf("  .min_mip_filter = %s\n",
                   util_dump_tex_mipfilter(sampler->min_mip_filter, TRUE));
      debug_printf("  .mag_img_filter = %s\n",
                   util_dump_tex_filter(sampler->mag_img_filter, TRUE));
      if (sampler->compare_mode != PIPE_TEX_COMPARE_NONE)
         debug_printf("  .compare_func = %s\n", util_dump_func(sampler->compare_func, TRUE));
      debug_printf("  .normalized_coords = %u\n", sampler->normalized_coords);
      debug_printf("  .min_max_lod_equal = %u\n", sampler->min_max_lod_equal);
      debug_printf("  .lod_bias_non_zero = %u\n", sampler->lod_bias_non_zero);
      debug_printf("  .apply_min_lod = %u\n", sampler->apply_min_lod);
      debug_printf("  .apply_max_lod = %u\n", sampler->apply_max_lod);
   }
   for (i = 0; i < key->nr_sampler_views; ++i) {
      const struct lp_static_texture_state *texture = &key->state[i].texture_state;
      debug_printf("texture[%u] = \n", i);
      debug_printf("  .format = %s\n",
                   util_format_name(texture->format));
      debug_printf("  .target = %s\n",
                   util_dump_tex_target(texture->target, TRUE));
      debug_printf("  .level_zero_only = %u\n",
                   texture->level_zero_only);
      debug_printf("  .pot = %u %u %u\n",
                   texture->pot_width,
                   texture->pot_height,
                   texture->pot_depth);
   }
}


void
lp_debug_fs_variant(const struct lp_fragment_shader_variant *variant)
{
   debug_printf("llvmpipe: Fragment shader #%u variant #%u:\n", 
                variant->shader->no, variant->no);
   tgsi_dump(variant->shader->base.tokens, 0);
   dump_fs_variant_key(&variant->key);
   debug_printf("variant->opaque = %u\n", variant->opaque);
   debug_printf("\n");
}


/**
 * Generate a new fragment shader variant from the shader code and
 * other state indicated by the key.
 */
static struct lp_fragment_shader_variant *
generate_variant(struct llvmpipe_context *lp,
                 struct lp_fragment_shader *shader,
                 const struct lp_fragment_shader_variant_key *key)
{
   struct lp_fragment_shader_variant *variant;
   const struct util_format_description *cbuf0_format_desc;
   boolean fullcolormask;

   variant = CALLOC_STRUCT(lp_fragment_shader_variant);
   if(!variant)
      return NULL;

   variant->gallivm = gallivm_create();
   if (!variant->gallivm) {
      FREE(variant);
      return NULL;
   }

   variant->shader = shader;
   variant->list_item_global.base = variant;
   variant->list_item_local.base = variant;
   variant->no = shader->variants_created++;

   memcpy(&variant->key, key, shader->variant_key_size);

   /*
    * Determine whether we are touching all channels in the color buffer.
    */
   fullcolormask = FALSE;
   if (key->nr_cbufs == 1) {
      cbuf0_format_desc = util_format_description(key->cbuf_format[0]);
      fullcolormask = util_format_colormask_full(cbuf0_format_desc, key->blend.rt[0].colormask);
   }

   variant->opaque =
         !key->blend.logicop_enable &&
         !key->blend.rt[0].blend_enable &&
         fullcolormask &&
         !key->stencil[0].enabled &&
         !key->alpha.enabled &&
         !key->depth.enabled &&
         !shader->info.base.uses_kill
         ? TRUE : FALSE;

   if ((LP_DEBUG & DEBUG_FS) || (gallivm_debug & GALLIVM_DEBUG_IR)) {
      lp_debug_fs_variant(variant);
   }

   lp_jit_init_types(variant);
   
   if (variant->jit_function[RAST_EDGE_TEST] == NULL)
      generate_fragment(lp, shader, variant, RAST_EDGE_TEST);

   if (variant->jit_function[RAST_WHOLE] == NULL) {
      if (variant->opaque) {
         /* Specialized shader, which doesn't need to read the color buffer. */
         generate_fragment(lp, shader, variant, RAST_WHOLE);
      }
   }

   /*
    * Compile everything
    */

   gallivm_compile_module(variant->gallivm);

   if (variant->function[RAST_EDGE_TEST]) {
      variant->jit_function[RAST_EDGE_TEST] = (lp_jit_frag_func)
            gallivm_jit_function(variant->gallivm,
                                 variant->function[RAST_EDGE_TEST]);
   }

   if (variant->function[RAST_WHOLE]) {
         variant->jit_function[RAST_WHOLE] = (lp_jit_frag_func)
               gallivm_jit_function(variant->gallivm,
                                    variant->function[RAST_WHOLE]);
   } else if (!variant->jit_function[RAST_WHOLE]) {
      variant->jit_function[RAST_WHOLE] = variant->jit_function[RAST_EDGE_TEST];
   }

   return variant;
}


static void *
llvmpipe_create_fs_state(struct pipe_context *pipe,
                         const struct pipe_shader_state *templ)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   struct lp_fragment_shader *shader;
   int nr_samplers;
   int nr_sampler_views;
   int i;

   shader = CALLOC_STRUCT(lp_fragment_shader);
   if (!shader)
      return NULL;

   shader->no = fs_no++;
   make_empty_list(&shader->variants);

   /* get/save the summary info for this shader */
   lp_build_tgsi_info(templ->tokens, &shader->info);

   /* we need to keep a local copy of the tokens */
   shader->base.tokens = tgsi_dup_tokens(templ->tokens);

   shader->draw_data = draw_create_fragment_shader(llvmpipe->draw, templ);
   if (shader->draw_data == NULL) {
      FREE((void *) shader->base.tokens);
      FREE(shader);
      return NULL;
   }

   nr_samplers = shader->info.base.file_max[TGSI_FILE_SAMPLER] + 1;
   nr_sampler_views = shader->info.base.file_max[TGSI_FILE_SAMPLER_VIEW] + 1;

   shader->variant_key_size = Offset(struct lp_fragment_shader_variant_key,
                                     state[MAX2(nr_samplers, nr_sampler_views)]);

   for (i = 0; i < shader->info.base.num_inputs; i++) {
      shader->inputs[i].usage_mask = shader->info.base.input_usage_mask[i];
      shader->inputs[i].cyl_wrap = shader->info.base.input_cylindrical_wrap[i];

      switch (shader->info.base.input_interpolate[i]) {
      case TGSI_INTERPOLATE_CONSTANT:
	 shader->inputs[i].interp = LP_INTERP_CONSTANT;
	 break;
      case TGSI_INTERPOLATE_LINEAR:
	 shader->inputs[i].interp = LP_INTERP_LINEAR;
	 break;
      case TGSI_INTERPOLATE_PERSPECTIVE:
	 shader->inputs[i].interp = LP_INTERP_PERSPECTIVE;
	 break;
      case TGSI_INTERPOLATE_COLOR:
	 shader->inputs[i].interp = LP_INTERP_COLOR;
	 break;
      default:
	 assert(0);
	 break;
      }

      switch (shader->info.base.input_semantic_name[i]) {
      case TGSI_SEMANTIC_FACE:
	 shader->inputs[i].interp = LP_INTERP_FACING;
	 break;
      case TGSI_SEMANTIC_POSITION:
	 /* Position was already emitted above
	  */
	 shader->inputs[i].interp = LP_INTERP_POSITION;
	 shader->inputs[i].src_index = 0;
	 continue;
      }

      shader->inputs[i].src_index = i+1;
   }

   if (LP_DEBUG & DEBUG_TGSI) {
      unsigned attrib;
      debug_printf("llvmpipe: Create fragment shader #%u %p:\n",
                   shader->no, (void *) shader);
      tgsi_dump(templ->tokens, 0);
      debug_printf("usage masks:\n");
      for (attrib = 0; attrib < shader->info.base.num_inputs; ++attrib) {
         unsigned usage_mask = shader->info.base.input_usage_mask[attrib];
         debug_printf("  IN[%u].%s%s%s%s\n",
                      attrib,
                      usage_mask & TGSI_WRITEMASK_X ? "x" : "",
                      usage_mask & TGSI_WRITEMASK_Y ? "y" : "",
                      usage_mask & TGSI_WRITEMASK_Z ? "z" : "",
                      usage_mask & TGSI_WRITEMASK_W ? "w" : "");
      }
      debug_printf("\n");
   }

   return shader;
}


static void
llvmpipe_bind_fs_state(struct pipe_context *pipe, void *fs)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);

   if (llvmpipe->fs == fs)
      return;

   llvmpipe->fs = (struct lp_fragment_shader *) fs;

   draw_bind_fragment_shader(llvmpipe->draw,
                             (llvmpipe->fs ? llvmpipe->fs->draw_data : NULL));

   llvmpipe->dirty |= LP_NEW_FS;
}


/**
 * Remove shader variant from two lists: the shader's variant list
 * and the context's variant list.
 */
void
llvmpipe_remove_shader_variant(struct llvmpipe_context *lp,
                               struct lp_fragment_shader_variant *variant)
{
   unsigned i;

   if (gallivm_debug & GALLIVM_DEBUG_IR) {
      debug_printf("llvmpipe: del fs #%u var #%u v created #%u v cached"
                   " #%u v total cached #%u\n",
                   variant->shader->no,
                   variant->no,
                   variant->shader->variants_created,
                   variant->shader->variants_cached,
                   lp->nr_fs_variants);
   }

   /* free all the variant's JIT'd functions */
   for (i = 0; i < Elements(variant->function); i++) {
      if (variant->function[i]) {
         gallivm_free_function(variant->gallivm,
                               variant->function[i],
                               variant->jit_function[i]);
      }
   }

   gallivm_destroy(variant->gallivm);

   /* remove from shader's list */
   remove_from_list(&variant->list_item_local);
   variant->shader->variants_cached--;

   /* remove from context's list */
   remove_from_list(&variant->list_item_global);
   lp->nr_fs_variants--;
   lp->nr_fs_instrs -= variant->nr_instrs;

   FREE(variant);
}


static void
llvmpipe_delete_fs_state(struct pipe_context *pipe, void *fs)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   struct lp_fragment_shader *shader = fs;
   struct lp_fs_variant_list_item *li;

   assert(fs != llvmpipe->fs);

   /*
    * XXX: we need to flush the context until we have some sort of reference
    * counting in fragment shaders as they may still be binned
    * Flushing alone might not sufficient we need to wait on it too.
    */
   llvmpipe_finish(pipe, __FUNCTION__);

   /* Delete all the variants */
   li = first_elem(&shader->variants);
   while(!at_end(&shader->variants, li)) {
      struct lp_fs_variant_list_item *next = next_elem(li);
      llvmpipe_remove_shader_variant(llvmpipe, li->base);
      li = next;
   }

   /* Delete draw module's data */
   draw_delete_fragment_shader(llvmpipe->draw, shader->draw_data);

   assert(shader->variants_cached == 0);
   FREE((void *) shader->base.tokens);
   FREE(shader);
}



static void
llvmpipe_set_constant_buffer(struct pipe_context *pipe,
                             uint shader, uint index,
                             struct pipe_constant_buffer *cb)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   struct pipe_resource *constants = cb ? cb->buffer : NULL;

   assert(shader < PIPE_SHADER_TYPES);
   assert(index < Elements(llvmpipe->constants[shader]));

   /* note: reference counting */
   util_copy_constant_buffer(&llvmpipe->constants[shader][index], cb);

   if (shader == PIPE_SHADER_VERTEX ||
       shader == PIPE_SHADER_GEOMETRY) {
      /* Pass the constants to the 'draw' module */
      const unsigned size = cb ? cb->buffer_size : 0;
      const ubyte *data;

      if (constants) {
         data = (ubyte *) llvmpipe_resource_data(constants);
      }
      else if (cb && cb->user_buffer) {
         data = (ubyte *) cb->user_buffer;
      }
      else {
         data = NULL;
      }

      if (data)
         data += cb->buffer_offset;

      draw_set_mapped_constant_buffer(llvmpipe->draw, shader,
                                      index, data, size);
   }

   llvmpipe->dirty |= LP_NEW_CONSTANTS;

   if (cb && cb->user_buffer) {
      pipe_resource_reference(&constants, NULL);
   }
}


/**
 * Return the blend factor equivalent to a destination alpha of one.
 */
static INLINE unsigned
force_dst_alpha_one(unsigned factor)
{
   switch(factor) {
   case PIPE_BLENDFACTOR_DST_ALPHA:
      return PIPE_BLENDFACTOR_ONE;
   case PIPE_BLENDFACTOR_INV_DST_ALPHA:
      return PIPE_BLENDFACTOR_ZERO;
   case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
      return PIPE_BLENDFACTOR_ZERO;
   }

   return factor;
}


/**
 * We need to generate several variants of the fragment pipeline to match
 * all the combinations of the contributing state atoms.
 *
 * TODO: there is actually no reason to tie this to context state -- the
 * generated code could be cached globally in the screen.
 */
static void
make_variant_key(struct llvmpipe_context *lp,
                 struct lp_fragment_shader *shader,
                 struct lp_fragment_shader_variant_key *key)
{
   unsigned i;

   memset(key, 0, shader->variant_key_size);

   if (lp->framebuffer.zsbuf) {
      if (lp->depth_stencil->depth.enabled) {
         key->zsbuf_format = lp->framebuffer.zsbuf->format;
         memcpy(&key->depth, &lp->depth_stencil->depth, sizeof key->depth);
      }
      if (lp->depth_stencil->stencil[0].enabled) {
         key->zsbuf_format = lp->framebuffer.zsbuf->format;
         memcpy(&key->stencil, &lp->depth_stencil->stencil, sizeof key->stencil);
      }
   }

   /* alpha test only applies if render buffer 0 is non-integer (or does not exist) */
   if (!lp->framebuffer.nr_cbufs ||
       !util_format_is_pure_integer(lp->framebuffer.cbufs[0]->format)) {
      key->alpha.enabled = lp->depth_stencil->alpha.enabled;
   }
   if(key->alpha.enabled)
      key->alpha.func = lp->depth_stencil->alpha.func;
   /* alpha.ref_value is passed in jit_context */

   key->flatshade = lp->rasterizer->flatshade;
   if (lp->active_occlusion_query) {
      key->occlusion_count = TRUE;
   }

   if (lp->framebuffer.nr_cbufs) {
      memcpy(&key->blend, lp->blend, sizeof key->blend);
   }

   key->nr_cbufs = lp->framebuffer.nr_cbufs;

   if (!key->blend.independent_blend_enable) {
      /* we always need independent blend otherwise the fixups below won't work */
      for (i = 1; i < key->nr_cbufs; i++) {
         memcpy(&key->blend.rt[i], &key->blend.rt[0], sizeof(key->blend.rt[0]));
      }
      key->blend.independent_blend_enable = 1;
   }

   for (i = 0; i < lp->framebuffer.nr_cbufs; i++) {
      enum pipe_format format = lp->framebuffer.cbufs[i]->format;
      struct pipe_rt_blend_state *blend_rt = &key->blend.rt[i];
      const struct util_format_description *format_desc;

      key->cbuf_format[i] = format;

      format_desc = util_format_description(format);
      assert(format_desc->colorspace == UTIL_FORMAT_COLORSPACE_RGB ||
             format_desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB);

      /*
       * Mask out color channels not present in the color buffer.
       */
      blend_rt->colormask &= util_format_colormask(format_desc);

      /*
       * Disable blend for integer formats.
       */
      if (util_format_is_pure_integer(format)) {
         blend_rt->blend_enable = 0;
      }

      /*
       * Our swizzled render tiles always have an alpha channel, but the linear
       * render target format often does not, so force here the dst alpha to be
       * one.
       *
       * This is not a mere optimization. Wrong results will be produced if the
       * dst alpha is used, the dst format does not have alpha, and the previous
       * rendering was not flushed from the swizzled to linear buffer. For
       * example, NonPowTwo DCT.
       *
       * TODO: This should be generalized to all channels for better
       * performance, but only alpha causes correctness issues.
       *
       * Also, force rgb/alpha func/factors match, to make AoS blending easier.
       */
      if (format_desc->swizzle[3] > UTIL_FORMAT_SWIZZLE_W ||
          format_desc->swizzle[3] == format_desc->swizzle[0]) {
         blend_rt->rgb_src_factor   = force_dst_alpha_one(blend_rt->rgb_src_factor);
         blend_rt->rgb_dst_factor   = force_dst_alpha_one(blend_rt->rgb_dst_factor);
         blend_rt->alpha_func       = blend_rt->rgb_func;
         blend_rt->alpha_src_factor = blend_rt->rgb_src_factor;
         blend_rt->alpha_dst_factor = blend_rt->rgb_dst_factor;
      }
   }

   /* This value will be the same for all the variants of a given shader:
    */
   key->nr_samplers = shader->info.base.file_max[TGSI_FILE_SAMPLER] + 1;

   for(i = 0; i < key->nr_samplers; ++i) {
      if(shader->info.base.file_mask[TGSI_FILE_SAMPLER] & (1 << i)) {
         lp_sampler_static_sampler_state(&key->state[i].sampler_state,
                                         lp->samplers[PIPE_SHADER_FRAGMENT][i]);
      }
   }

   /*
    * XXX If TGSI_FILE_SAMPLER_VIEW exists assume all texture opcodes
    * are dx10-style? Can't really have mixed opcodes, at least not
    * if we want to skip the holes here (without rescanning tgsi).
    */
   if (shader->info.base.file_max[TGSI_FILE_SAMPLER_VIEW] != -1) {
      key->nr_sampler_views = shader->info.base.file_max[TGSI_FILE_SAMPLER_VIEW] + 1;
      for(i = 0; i < key->nr_sampler_views; ++i) {
         if(shader->info.base.file_mask[TGSI_FILE_SAMPLER_VIEW] & (1 << i)) {
            lp_sampler_static_texture_state(&key->state[i].texture_state,
                                            lp->sampler_views[PIPE_SHADER_FRAGMENT][i]);
         }
      }
   }
   else {
      key->nr_sampler_views = key->nr_samplers;
      for(i = 0; i < key->nr_sampler_views; ++i) {
         if(shader->info.base.file_mask[TGSI_FILE_SAMPLER] & (1 << i)) {
            lp_sampler_static_texture_state(&key->state[i].texture_state,
                                            lp->sampler_views[PIPE_SHADER_FRAGMENT][i]);
         }
      }
   }
}



/**
 * Update fragment shader state.  This is called just prior to drawing
 * something when some fragment-related state has changed.
 */
void 
llvmpipe_update_fs(struct llvmpipe_context *lp)
{
   struct lp_fragment_shader *shader = lp->fs;
   struct lp_fragment_shader_variant_key key;
   struct lp_fragment_shader_variant *variant = NULL;
   struct lp_fs_variant_list_item *li;

   make_variant_key(lp, shader, &key);

   /* Search the variants for one which matches the key */
   li = first_elem(&shader->variants);
   while(!at_end(&shader->variants, li)) {
      if(memcmp(&li->base->key, &key, shader->variant_key_size) == 0) {
         variant = li->base;
         break;
      }
      li = next_elem(li);
   }

   if (variant) {
      /* Move this variant to the head of the list to implement LRU
       * deletion of shader's when we have too many.
       */
      move_to_head(&lp->fs_variants_list, &variant->list_item_global);
   }
   else {
      /* variant not found, create it now */
      int64_t t0, t1, dt;
      unsigned i;
      unsigned variants_to_cull;

      if (0) {
         debug_printf("%u variants,\t%u instrs,\t%u instrs/variant\n",
                      lp->nr_fs_variants,
                      lp->nr_fs_instrs,
                      lp->nr_fs_variants ? lp->nr_fs_instrs / lp->nr_fs_variants : 0);
      }

      /* First, check if we've exceeded the max number of shader variants.
       * If so, free 25% of them (the least recently used ones).
       */
      variants_to_cull = lp->nr_fs_variants >= LP_MAX_SHADER_VARIANTS ? LP_MAX_SHADER_VARIANTS / 4 : 0;

      if (variants_to_cull ||
          lp->nr_fs_instrs >= LP_MAX_SHADER_INSTRUCTIONS) {
         struct pipe_context *pipe = &lp->pipe;

         /*
          * XXX: we need to flush the context until we have some sort of
          * reference counting in fragment shaders as they may still be binned
          * Flushing alone might not be sufficient we need to wait on it too.
          */
         llvmpipe_finish(pipe, __FUNCTION__);

         /*
          * We need to re-check lp->nr_fs_variants because an arbitrarliy large
          * number of shader variants (potentially all of them) could be
          * pending for destruction on flush.
          */

         for (i = 0; i < variants_to_cull || lp->nr_fs_instrs >= LP_MAX_SHADER_INSTRUCTIONS; i++) {
            struct lp_fs_variant_list_item *item;
            if (is_empty_list(&lp->fs_variants_list)) {
               break;
            }
            item = last_elem(&lp->fs_variants_list);
            assert(item);
            assert(item->base);
            llvmpipe_remove_shader_variant(lp, item->base);
         }
      }

      /*
       * Generate the new variant.
       */
      t0 = os_time_get();
      variant = generate_variant(lp, shader, &key);
      t1 = os_time_get();
      dt = t1 - t0;
      LP_COUNT_ADD(llvm_compile_time, dt);
      LP_COUNT_ADD(nr_llvm_compiles, 2);  /* emit vs. omit in/out test */

      llvmpipe_variant_count++;

      /* Put the new variant into the list */
      if (variant) {
         insert_at_head(&shader->variants, &variant->list_item_local);
         insert_at_head(&lp->fs_variants_list, &variant->list_item_global);
         lp->nr_fs_variants++;
         lp->nr_fs_instrs += variant->nr_instrs;
         shader->variants_cached++;
      }
   }

   /* Bind this variant */
   lp_setup_set_fs_variant(lp->setup, variant);
}





void
llvmpipe_init_fs_funcs(struct llvmpipe_context *llvmpipe)
{
   llvmpipe->pipe.create_fs_state = llvmpipe_create_fs_state;
   llvmpipe->pipe.bind_fs_state   = llvmpipe_bind_fs_state;
   llvmpipe->pipe.delete_fs_state = llvmpipe_delete_fs_state;

   llvmpipe->pipe.set_constant_buffer = llvmpipe_set_constant_buffer;
}
