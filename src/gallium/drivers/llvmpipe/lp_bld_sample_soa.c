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
 * Texture sampling -- SoA.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */

#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "util/u_debug.h"
#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_format.h"
#include "lp_bld_debug.h"
#include "lp_bld_type.h"
#include "lp_bld_const.h"
#include "lp_bld_arit.h"
#include "lp_bld_logic.h"
#include "lp_bld_swizzle.h"
#include "lp_bld_format.h"
#include "lp_bld_sample.h"


/**
 * Keep all information for sampling code generation in a single place.
 */
struct lp_build_sample_context
{
   LLVMBuilderRef builder;

   const struct lp_sampler_static_state *static_state;

   struct lp_sampler_dynamic_state *dynamic_state;

   const struct util_format_description *format_desc;

   /** Incoming coordinates type and build context */
   struct lp_type coord_type;
   struct lp_build_context coord_bld;

   /** Integer coordinates */
   struct lp_type int_coord_type;
   struct lp_build_context int_coord_bld;

   /** Output texels type and build context */
   struct lp_type texel_type;
   struct lp_build_context texel_bld;
};


static void
lp_build_sample_texel(struct lp_build_sample_context *bld,
                      LLVMValueRef x,
                      LLVMValueRef y,
                      LLVMValueRef y_stride,
                      LLVMValueRef data_ptr,
                      LLVMValueRef *texel)
{
   LLVMValueRef offset;
   LLVMValueRef packed;

   offset = lp_build_sample_offset(&bld->int_coord_bld,
                                   bld->format_desc,
                                   x, y, y_stride,
                                   data_ptr);

   assert(bld->format_desc->block.width == 1);
   assert(bld->format_desc->block.height == 1);
   assert(bld->format_desc->block.bits <= bld->texel_type.width);

   packed = lp_build_gather(bld->builder,
                            bld->texel_type.length,
                            bld->format_desc->block.bits,
                            bld->texel_type.width,
                            data_ptr, offset);

   lp_build_unpack_rgba_soa(bld->builder,
                            bld->format_desc,
                            bld->texel_type,
                            packed, texel);
}


static LLVMValueRef
lp_build_sample_wrap(struct lp_build_sample_context *bld,
                     LLVMValueRef coord,
                     LLVMValueRef length,
                     boolean is_pot,
                     unsigned wrap_mode)
{
   struct lp_build_context *int_coord_bld = &bld->int_coord_bld;
   LLVMValueRef length_minus_one;

   length_minus_one = lp_build_sub(int_coord_bld, length, int_coord_bld->one);

   switch(wrap_mode) {
   case PIPE_TEX_WRAP_REPEAT:
      if(is_pot)
         coord = LLVMBuildAnd(bld->builder, coord, length_minus_one, "");
      else
         /* Signed remainder won't give the right results for negative
          * dividends but unsigned remainder does.*/
         coord = LLVMBuildURem(bld->builder, coord, length, "");
      break;

   case PIPE_TEX_WRAP_CLAMP:
      coord = lp_build_max(int_coord_bld, coord, int_coord_bld->zero);
      coord = lp_build_min(int_coord_bld, coord, length_minus_one);
      break;

   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
   case PIPE_TEX_WRAP_MIRROR_REPEAT:
   case PIPE_TEX_WRAP_MIRROR_CLAMP:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
      /* FIXME */
      _debug_printf("warning: failed to translate texture wrap mode %u\n", wrap_mode);
      coord = lp_build_max(int_coord_bld, coord, int_coord_bld->zero);
      coord = lp_build_min(int_coord_bld, coord, length_minus_one);
      break;

   default:
      assert(0);
   }

   return coord;
}


static void
lp_build_sample_2d_nearest_soa(struct lp_build_sample_context *bld,
                               LLVMValueRef s,
                               LLVMValueRef t,
                               LLVMValueRef width,
                               LLVMValueRef height,
                               LLVMValueRef stride,
                               LLVMValueRef data_ptr,
                               LLVMValueRef *texel)
{
   LLVMValueRef x;
   LLVMValueRef y;

   x = lp_build_ifloor(&bld->coord_bld, s);
   y = lp_build_ifloor(&bld->coord_bld, t);

   x = lp_build_sample_wrap(bld, x, width,  bld->static_state->pot_width,  bld->static_state->wrap_s);
   y = lp_build_sample_wrap(bld, y, height, bld->static_state->pot_height, bld->static_state->wrap_t);

   lp_build_sample_texel(bld, x, y, stride, data_ptr, texel);
}


static void
lp_build_sample_2d_linear_soa(struct lp_build_sample_context *bld,
                              LLVMValueRef s,
                              LLVMValueRef t,
                              LLVMValueRef width,
                              LLVMValueRef height,
                              LLVMValueRef stride,
                              LLVMValueRef data_ptr,
                              LLVMValueRef *texel)
{
   LLVMValueRef half;
   LLVMValueRef s_ipart;
   LLVMValueRef t_ipart;
   LLVMValueRef s_fpart;
   LLVMValueRef t_fpart;
   LLVMValueRef x0, x1;
   LLVMValueRef y0, y1;
   LLVMValueRef neighbors[2][2][4];
   unsigned chan;

   half = lp_build_const_scalar(bld->coord_type, 0.5);
   s = lp_build_sub(&bld->coord_bld, s, half);
   t = lp_build_sub(&bld->coord_bld, t, half);

   s_ipart = lp_build_floor(&bld->coord_bld, s);
   t_ipart = lp_build_floor(&bld->coord_bld, t);

   s_fpart = lp_build_sub(&bld->coord_bld, s, s_ipart);
   t_fpart = lp_build_sub(&bld->coord_bld, t, t_ipart);

   x0 = lp_build_itrunc(&bld->coord_bld, s_ipart);
   y0 = lp_build_itrunc(&bld->coord_bld, t_ipart);

   x0 = lp_build_sample_wrap(bld, x0, width,  bld->static_state->pot_width,  bld->static_state->wrap_s);
   y0 = lp_build_sample_wrap(bld, y0, height, bld->static_state->pot_height, bld->static_state->wrap_t);

   x1 = lp_build_add(&bld->int_coord_bld, x0, bld->int_coord_bld.one);
   y1 = lp_build_add(&bld->int_coord_bld, y0, bld->int_coord_bld.one);

   x1 = lp_build_sample_wrap(bld, x1, width,  bld->static_state->pot_width,  bld->static_state->wrap_s);
   y1 = lp_build_sample_wrap(bld, y1, height, bld->static_state->pot_height, bld->static_state->wrap_t);

   lp_build_sample_texel(bld, x0, y0, stride, data_ptr, neighbors[0][0]);
   lp_build_sample_texel(bld, x1, y0, stride, data_ptr, neighbors[0][1]);
   lp_build_sample_texel(bld, x0, y1, stride, data_ptr, neighbors[1][0]);
   lp_build_sample_texel(bld, x1, y1, stride, data_ptr, neighbors[1][1]);

   /* TODO: Don't interpolate missing channels */
   for(chan = 0; chan < 4; ++chan) {
      texel[chan] = lp_build_lerp_2d(&bld->texel_bld,
                                     s_fpart, t_fpart,
                                     neighbors[0][0][chan],
                                     neighbors[0][1][chan],
                                     neighbors[1][0][chan],
                                     neighbors[1][1][chan]);
   }
}


static void
lp_build_sample_compare(struct lp_build_sample_context *bld,
                        LLVMValueRef p,
                        LLVMValueRef *texel)
{
   struct lp_build_context *texel_bld = &bld->texel_bld;
   LLVMValueRef res;
   unsigned chan;

   if(!bld->static_state->compare_mode)
      return;

   /* TODO: Compare before swizzling, to avoid redundant computations */
   res = NULL;
   for(chan = 0; chan < 4; ++chan) {
      LLVMValueRef cmp;
      cmp = lp_build_cmp(texel_bld, bld->static_state->compare_func, p, texel[chan]);
      cmp = lp_build_select(texel_bld, cmp, texel_bld->one, texel_bld->zero);

      if(res)
         res = lp_build_add(texel_bld, res, cmp);
      else
         res = cmp;
   }

   assert(res);
   res = lp_build_mul(texel_bld, res, lp_build_const_scalar(texel_bld->type, 0.25));

   /* XXX returning result for default GL_DEPTH_TEXTURE_MODE = GL_LUMINANCE */
   for(chan = 0; chan < 3; ++chan)
      texel[chan] = res;
   texel[3] = texel_bld->one;
}


void
lp_build_sample_soa(LLVMBuilderRef builder,
                    const struct lp_sampler_static_state *static_state,
                    struct lp_sampler_dynamic_state *dynamic_state,
                    struct lp_type type,
                    unsigned unit,
                    unsigned num_coords,
                    const LLVMValueRef *coords,
                    LLVMValueRef lodbias,
                    LLVMValueRef *texel)
{
   struct lp_build_sample_context bld;
   LLVMValueRef width;
   LLVMValueRef height;
   LLVMValueRef stride;
   LLVMValueRef data_ptr;
   LLVMValueRef s;
   LLVMValueRef t;
   LLVMValueRef p;

   /* Setup our build context */
   memset(&bld, 0, sizeof bld);
   bld.builder = builder;
   bld.static_state = static_state;
   bld.dynamic_state = dynamic_state;
   bld.format_desc = util_format_description(static_state->format);
   bld.coord_type = type;
   bld.int_coord_type = lp_int_type(type);
   bld.texel_type = type;
   lp_build_context_init(&bld.coord_bld, builder, bld.coord_type);
   lp_build_context_init(&bld.int_coord_bld, builder, bld.int_coord_type);
   lp_build_context_init(&bld.texel_bld, builder, bld.texel_type);

   /* Get the dynamic state */
   width = dynamic_state->width(dynamic_state, builder, unit);
   height = dynamic_state->height(dynamic_state, builder, unit);
   stride = dynamic_state->stride(dynamic_state, builder, unit);
   data_ptr = dynamic_state->data_ptr(dynamic_state, builder, unit);

   s = coords[0];
   t = coords[1];
   p = coords[2];

   width = lp_build_broadcast_scalar(&bld.int_coord_bld, width);
   height = lp_build_broadcast_scalar(&bld.int_coord_bld, height);
   stride = lp_build_broadcast_scalar(&bld.int_coord_bld, stride);

   if(static_state->target == PIPE_TEXTURE_1D)
      t = bld.coord_bld.zero;

   if(static_state->normalized_coords) {
      LLVMTypeRef coord_vec_type = lp_build_vec_type(bld.coord_type);
      LLVMValueRef fp_width = LLVMBuildSIToFP(builder, width, coord_vec_type, "");
      LLVMValueRef fp_height = LLVMBuildSIToFP(builder, height, coord_vec_type, "");
      s = lp_build_mul(&bld.coord_bld, s, fp_width);
      t = lp_build_mul(&bld.coord_bld, t, fp_height);
   }

   switch (static_state->min_img_filter) {
   case PIPE_TEX_FILTER_NEAREST:
      lp_build_sample_2d_nearest_soa(&bld, s, t, width, height, stride, data_ptr, texel);
      break;
   case PIPE_TEX_FILTER_LINEAR:
   case PIPE_TEX_FILTER_ANISO:
      lp_build_sample_2d_linear_soa(&bld, s, t, width, height, stride, data_ptr, texel);
      break;
   default:
      assert(0);
   }

   /* FIXME: respect static_state->min_mip_filter */;
   /* FIXME: respect static_state->mag_img_filter */;
   /* FIXME: respect static_state->prefilter */;

   lp_build_sample_compare(&bld, p, texel);
}
