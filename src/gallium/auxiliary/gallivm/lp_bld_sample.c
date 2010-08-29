/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/**
 * @file
 * Texture sampling -- common code.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */

#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include "lp_bld_debug.h"
#include "lp_bld_const.h"
#include "lp_bld_arit.h"
#include "lp_bld_type.h"
#include "lp_bld_sample.h"


/**
 * Initialize lp_sampler_static_state object with the gallium sampler
 * and texture state.
 * The former is considered to be static and the later dynamic.
 */
void
lp_sampler_static_state(struct lp_sampler_static_state *state,
                        const struct pipe_sampler_view *view,
                        const struct pipe_sampler_state *sampler)
{
   const struct pipe_resource *texture = view->texture;

   memset(state, 0, sizeof *state);

   if(!texture)
      return;

   if(!sampler)
      return;

   /*
    * We don't copy sampler state over unless it is actually enabled, to avoid
    * spurious recompiles, as the sampler static state is part of the shader
    * key.
    *
    * Ideally the state tracker or cso_cache module would make all state
    * canonical, but until that happens it's better to be safe than sorry here.
    *
    * XXX: Actually there's much more than can be done here, especially
    * regarding 1D/2D/3D/CUBE textures, wrap modes, etc.
    */

   state->format            = view->format;
   state->swizzle_r         = view->swizzle_r;
   state->swizzle_g         = view->swizzle_g;
   state->swizzle_b         = view->swizzle_b;
   state->swizzle_a         = view->swizzle_a;

   state->target            = texture->target;
   state->pot_width         = util_is_power_of_two(texture->width0);
   state->pot_height        = util_is_power_of_two(texture->height0);
   state->pot_depth         = util_is_power_of_two(texture->depth0);

   state->wrap_s            = sampler->wrap_s;
   state->wrap_t            = sampler->wrap_t;
   state->wrap_r            = sampler->wrap_r;
   state->min_img_filter    = sampler->min_img_filter;
   state->mag_img_filter    = sampler->mag_img_filter;
   if (view->last_level) {
      state->min_mip_filter = sampler->min_mip_filter;
   } else {
      state->min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   }

   state->compare_mode      = sampler->compare_mode;
   if (sampler->compare_mode != PIPE_TEX_COMPARE_NONE) {
      state->compare_func   = sampler->compare_func;
   }

   state->normalized_coords = sampler->normalized_coords;
   state->lod_bias          = sampler->lod_bias;
   if (!view->last_level &&
       sampler->min_img_filter == sampler->mag_img_filter) {
      state->min_lod        = 0.0f;
      state->max_lod        = 0.0f;
   } else {
      state->min_lod        = MAX2(sampler->min_lod, 0.0f);
      state->max_lod        = sampler->max_lod;
   }
   state->border_color[0]   = sampler->border_color[0];
   state->border_color[1]   = sampler->border_color[1];
   state->border_color[2]   = sampler->border_color[2];
   state->border_color[3]   = sampler->border_color[3];

   /*
    * FIXME: Handle the remainder of pipe_sampler_view.
    */
}


/**
 * Compute the offset of a pixel block.
 *
 * x, y, z, y_stride, z_stride are vectors, and they refer to pixels.
 *
 * Returns the relative offset and i,j sub-block coordinates
 */
void
lp_build_sample_offset(struct lp_build_context *bld,
                       const struct util_format_description *format_desc,
                       LLVMValueRef x,
                       LLVMValueRef y,
                       LLVMValueRef z,
                       LLVMValueRef y_stride,
                       LLVMValueRef z_stride,
                       LLVMValueRef *out_offset,
                       LLVMValueRef *out_i,
                       LLVMValueRef *out_j)
{
   LLVMValueRef x_stride;
   LLVMValueRef offset;
   LLVMValueRef i;
   LLVMValueRef j;

   /*
    * Describe the coordinates in terms of pixel blocks.
    *
    * TODO: pixel blocks are power of two. LLVM should convert rem/div to
    * bit arithmetic. Verify this.
    */

   if (format_desc->block.width == 1) {
      i = bld->zero;
   }
   else {
      LLVMValueRef block_width = lp_build_const_int_vec(bld->type, format_desc->block.width);
      i = LLVMBuildURem(bld->builder, x, block_width, "");
      x = LLVMBuildUDiv(bld->builder, x, block_width, "");
   }

   if (format_desc->block.height == 1) {
      j = bld->zero;
   }
   else {
      LLVMValueRef block_height = lp_build_const_int_vec(bld->type, format_desc->block.height);
      j = LLVMBuildURem(bld->builder, y, block_height, "");
      y = LLVMBuildUDiv(bld->builder, y, block_height, "");
   }

   x_stride = lp_build_const_vec(bld->type, format_desc->block.bits/8);
   offset = lp_build_mul(bld, x, x_stride);

   if (y && y_stride) {
      LLVMValueRef y_offset = lp_build_mul(bld, y, y_stride);
      offset = lp_build_add(bld, offset, y_offset);
   }

   if (z && z_stride) {
      LLVMValueRef z_offset = lp_build_mul(bld, z, z_stride);
      offset = lp_build_add(bld, offset, z_offset);
   }

   *out_offset = offset;
   *out_i = i;
   *out_j = j;
}
