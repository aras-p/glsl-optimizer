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
 * @author Brian Paul <brianp@vmware.com>
 */

#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "pipe/p_shader_tokens.h"
#include "util/u_debug.h"
#include "util/u_dump.h"
#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_format.h"
#include "util/u_cpu_detect.h"
#include "util/u_format_rgb9e5.h"
#include "lp_bld_debug.h"
#include "lp_bld_type.h"
#include "lp_bld_const.h"
#include "lp_bld_conv.h"
#include "lp_bld_arit.h"
#include "lp_bld_bitarit.h"
#include "lp_bld_logic.h"
#include "lp_bld_printf.h"
#include "lp_bld_swizzle.h"
#include "lp_bld_flow.h"
#include "lp_bld_gather.h"
#include "lp_bld_format.h"
#include "lp_bld_sample.h"
#include "lp_bld_sample_aos.h"
#include "lp_bld_struct.h"
#include "lp_bld_quad.h"
#include "lp_bld_pack.h"


/**
 * Generate code to fetch a texel from a texture at int coords (x, y, z).
 * The computation depends on whether the texture is 1D, 2D or 3D.
 * The result, texel, will be float vectors:
 *   texel[0] = red values
 *   texel[1] = green values
 *   texel[2] = blue values
 *   texel[3] = alpha values
 */
static void
lp_build_sample_texel_soa(struct lp_build_sample_context *bld,
                          LLVMValueRef width,
                          LLVMValueRef height,
                          LLVMValueRef depth,
                          LLVMValueRef x,
                          LLVMValueRef y,
                          LLVMValueRef z,
                          LLVMValueRef y_stride,
                          LLVMValueRef z_stride,
                          LLVMValueRef data_ptr,
                          LLVMValueRef mipoffsets,
                          LLVMValueRef texel_out[4])
{
   const struct lp_static_sampler_state *static_state = bld->static_sampler_state;
   const unsigned dims = bld->dims;
   struct lp_build_context *int_coord_bld = &bld->int_coord_bld;
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef offset;
   LLVMValueRef i, j;
   LLVMValueRef use_border = NULL;

   /* use_border = x < 0 || x >= width || y < 0 || y >= height */
   if (lp_sampler_wrap_mode_uses_border_color(static_state->wrap_s,
                                              static_state->min_img_filter,
                                              static_state->mag_img_filter)) {
      LLVMValueRef b1, b2;
      b1 = lp_build_cmp(int_coord_bld, PIPE_FUNC_LESS, x, int_coord_bld->zero);
      b2 = lp_build_cmp(int_coord_bld, PIPE_FUNC_GEQUAL, x, width);
      use_border = LLVMBuildOr(builder, b1, b2, "b1_or_b2");
   }

   if (dims >= 2 &&
       lp_sampler_wrap_mode_uses_border_color(static_state->wrap_t,
                                              static_state->min_img_filter,
                                              static_state->mag_img_filter)) {
      LLVMValueRef b1, b2;
      b1 = lp_build_cmp(int_coord_bld, PIPE_FUNC_LESS, y, int_coord_bld->zero);
      b2 = lp_build_cmp(int_coord_bld, PIPE_FUNC_GEQUAL, y, height);
      if (use_border) {
         use_border = LLVMBuildOr(builder, use_border, b1, "ub_or_b1");
         use_border = LLVMBuildOr(builder, use_border, b2, "ub_or_b2");
      }
      else {
         use_border = LLVMBuildOr(builder, b1, b2, "b1_or_b2");
      }
   }

   if (dims == 3 &&
       lp_sampler_wrap_mode_uses_border_color(static_state->wrap_r,
                                              static_state->min_img_filter,
                                              static_state->mag_img_filter)) {
      LLVMValueRef b1, b2;
      b1 = lp_build_cmp(int_coord_bld, PIPE_FUNC_LESS, z, int_coord_bld->zero);
      b2 = lp_build_cmp(int_coord_bld, PIPE_FUNC_GEQUAL, z, depth);
      if (use_border) {
         use_border = LLVMBuildOr(builder, use_border, b1, "ub_or_b1");
         use_border = LLVMBuildOr(builder, use_border, b2, "ub_or_b2");
      }
      else {
         use_border = LLVMBuildOr(builder, b1, b2, "b1_or_b2");
      }
   }

   /* convert x,y,z coords to linear offset from start of texture, in bytes */
   lp_build_sample_offset(&bld->int_coord_bld,
                          bld->format_desc,
                          x, y, z, y_stride, z_stride,
                          &offset, &i, &j);
   if (mipoffsets) {
      offset = lp_build_add(&bld->int_coord_bld, offset, mipoffsets);
   }

   if (use_border) {
      /* If we can sample the border color, it means that texcoords may
       * lie outside the bounds of the texture image.  We need to do
       * something to prevent reading out of bounds and causing a segfault.
       *
       * Simply AND the texture coords with !use_border.  This will cause
       * coords which are out of bounds to become zero.  Zero's guaranteed
       * to be inside the texture image.
       */
      offset = lp_build_andnot(&bld->int_coord_bld, offset, use_border);
   }

   lp_build_fetch_rgba_soa(bld->gallivm,
                           bld->format_desc,
                           bld->texel_type,
                           data_ptr, offset,
                           i, j,
                           texel_out);

   /*
    * Note: if we find an app which frequently samples the texture border
    * we might want to implement a true conditional here to avoid sampling
    * the texture whenever possible (since that's quite a bit of code).
    * Ex:
    *   if (use_border) {
    *      texel = border_color;
    *   }
    *   else {
    *      texel = sample_texture(coord);
    *   }
    * As it is now, we always sample the texture, then selectively replace
    * the texel color results with the border color.
    */

   if (use_border) {
      /* select texel color or border color depending on use_border. */
      const struct util_format_description *format_desc = bld->format_desc;
      int chan;
      struct lp_type border_type = bld->texel_type;
      border_type.length = 4;
      /*
       * Only replace channels which are actually present. The others should
       * get optimized away eventually by sampler_view swizzle anyway but it's
       * easier too.
       */
      for (chan = 0; chan < 4; chan++) {
         unsigned chan_s;
         /* reverse-map channel... */
         for (chan_s = 0; chan_s < 4; chan_s++) {
            if (chan_s == format_desc->swizzle[chan]) {
               break;
            }
         }
         if (chan_s <= 3) {
            /* use the already clamped color */
            LLVMValueRef idx = lp_build_const_int32(bld->gallivm, chan);
            LLVMValueRef border_chan;

            border_chan = lp_build_extract_broadcast(bld->gallivm,
                                                     border_type,
                                                     bld->texel_type,
                                                     bld->border_color_clamped,
                                                     idx);
            texel_out[chan] = lp_build_select(&bld->texel_bld, use_border,
                                              border_chan, texel_out[chan]);
         }
      }
   }
}


/**
 * Helper to compute the mirror function for the PIPE_WRAP_MIRROR modes.
 */
static LLVMValueRef
lp_build_coord_mirror(struct lp_build_sample_context *bld,
                      LLVMValueRef coord)
{
   struct lp_build_context *coord_bld = &bld->coord_bld;
   struct lp_build_context *int_coord_bld = &bld->int_coord_bld;
   LLVMValueRef fract, flr, isOdd;

   lp_build_ifloor_fract(coord_bld, coord, &flr, &fract);

   /* isOdd = flr & 1 */
   isOdd = LLVMBuildAnd(bld->gallivm->builder, flr, int_coord_bld->one, "");

   /* make coord positive or negative depending on isOdd */
   coord = lp_build_set_sign(coord_bld, fract, isOdd);

   /* convert isOdd to float */
   isOdd = lp_build_int_to_float(coord_bld, isOdd);

   /* add isOdd to coord */
   coord = lp_build_add(coord_bld, coord, isOdd);

   return coord;
}


/**
 * Helper to compute the first coord and the weight for
 * linear wrap repeat npot textures
 */
void
lp_build_coord_repeat_npot_linear(struct lp_build_sample_context *bld,
                                  LLVMValueRef coord_f,
                                  LLVMValueRef length_i,
                                  LLVMValueRef length_f,
                                  LLVMValueRef *coord0_i,
                                  LLVMValueRef *weight_f)
{
   struct lp_build_context *coord_bld = &bld->coord_bld;
   struct lp_build_context *int_coord_bld = &bld->int_coord_bld;
   LLVMValueRef half = lp_build_const_vec(bld->gallivm, coord_bld->type, 0.5);
   LLVMValueRef length_minus_one = lp_build_sub(int_coord_bld, length_i,
                                                int_coord_bld->one);
   LLVMValueRef mask;
   /* wrap with normalized floats is just fract */
   coord_f = lp_build_fract(coord_bld, coord_f);
   /* mul by size and subtract 0.5 */
   coord_f = lp_build_mul(coord_bld, coord_f, length_f);
   coord_f = lp_build_sub(coord_bld, coord_f, half);
   /*
    * we avoided the 0.5/length division before the repeat wrap,
    * now need to fix up edge cases with selects
    */
   /* convert to int, compute lerp weight */
   lp_build_ifloor_fract(coord_bld, coord_f, coord0_i, weight_f);
   mask = lp_build_compare(int_coord_bld->gallivm, int_coord_bld->type,
                           PIPE_FUNC_LESS, *coord0_i, int_coord_bld->zero);
   *coord0_i = lp_build_select(int_coord_bld, mask, length_minus_one, *coord0_i);
}


/**
 * Build LLVM code for texture wrap mode for linear filtering.
 * \param x0_out  returns first integer texcoord
 * \param x1_out  returns second integer texcoord
 * \param weight_out  returns linear interpolation weight
 */
static void
lp_build_sample_wrap_linear(struct lp_build_sample_context *bld,
                            LLVMValueRef coord,
                            LLVMValueRef length,
                            LLVMValueRef length_f,
                            LLVMValueRef offset,
                            boolean is_pot,
                            unsigned wrap_mode,
                            LLVMValueRef *x0_out,
                            LLVMValueRef *x1_out,
                            LLVMValueRef *weight_out)
{
   struct lp_build_context *coord_bld = &bld->coord_bld;
   struct lp_build_context *int_coord_bld = &bld->int_coord_bld;
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef half = lp_build_const_vec(bld->gallivm, coord_bld->type, 0.5);
   LLVMValueRef length_minus_one = lp_build_sub(int_coord_bld, length, int_coord_bld->one);
   LLVMValueRef coord0, coord1, weight;

   switch(wrap_mode) {
   case PIPE_TEX_WRAP_REPEAT:
      if (is_pot) {
         /* mul by size and subtract 0.5 */
         coord = lp_build_mul(coord_bld, coord, length_f);
         coord = lp_build_sub(coord_bld, coord, half);
         if (offset) {
            offset = lp_build_int_to_float(coord_bld, offset);
            coord = lp_build_add(coord_bld, coord, offset);
         }
         /* convert to int, compute lerp weight */
         lp_build_ifloor_fract(coord_bld, coord, &coord0, &weight);
         coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);
         /* repeat wrap */
         coord0 = LLVMBuildAnd(builder, coord0, length_minus_one, "");
         coord1 = LLVMBuildAnd(builder, coord1, length_minus_one, "");
      }
      else {
         LLVMValueRef mask;
         if (offset) {
            offset = lp_build_int_to_float(coord_bld, offset);
            offset = lp_build_div(coord_bld, offset, length_f);
            coord = lp_build_add(coord_bld, coord, offset);
         }
         lp_build_coord_repeat_npot_linear(bld, coord,
                                           length, length_f,
                                           &coord0, &weight);
         mask = lp_build_compare(int_coord_bld->gallivm, int_coord_bld->type,
                                 PIPE_FUNC_NOTEQUAL, coord0, length_minus_one);
         coord1 = LLVMBuildAnd(builder,
                               lp_build_add(int_coord_bld, coord0, int_coord_bld->one),
                               mask, "");
      }
      break;

   case PIPE_TEX_WRAP_CLAMP:
      if (bld->static_sampler_state->normalized_coords) {
         /* scale coord to length */
         coord = lp_build_mul(coord_bld, coord, length_f);
      }
      if (offset) {
         offset = lp_build_int_to_float(coord_bld, offset);
         coord = lp_build_add(coord_bld, coord, offset);
      }

      /* clamp to [0, length] */
      coord = lp_build_clamp(coord_bld, coord, coord_bld->zero, length_f);

      coord = lp_build_sub(coord_bld, coord, half);

      /* convert to int, compute lerp weight */
      lp_build_ifloor_fract(coord_bld, coord, &coord0, &weight);
      coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);
      break;

   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      {
         struct lp_build_context abs_coord_bld = bld->coord_bld;
         abs_coord_bld.type.sign = FALSE;

         if (bld->static_sampler_state->normalized_coords) {
            /* mul by tex size */
            coord = lp_build_mul(coord_bld, coord, length_f);
         }
         if (offset) {
            offset = lp_build_int_to_float(coord_bld, offset);
            coord = lp_build_add(coord_bld, coord, offset);
         }

         /* clamp to length max */
         coord = lp_build_min(coord_bld, coord, length_f);
         /* subtract 0.5 */
         coord = lp_build_sub(coord_bld, coord, half);
         /* clamp to [0, length - 0.5] */
         coord = lp_build_max(coord_bld, coord, coord_bld->zero);
         /* convert to int, compute lerp weight */
         lp_build_ifloor_fract(&abs_coord_bld, coord, &coord0, &weight);
         coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);
         /* coord1 = min(coord1, length-1) */
         coord1 = lp_build_min(int_coord_bld, coord1, length_minus_one);
         break;
      }

   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      if (bld->static_sampler_state->normalized_coords) {
         /* scale coord to length */
         coord = lp_build_mul(coord_bld, coord, length_f);
      }
      if (offset) {
         offset = lp_build_int_to_float(coord_bld, offset);
         coord = lp_build_add(coord_bld, coord, offset);
      }
      /* was: clamp to [-0.5, length + 0.5], then sub 0.5 */
      /* can skip clamp (though might not work for very large coord values */
      coord = lp_build_sub(coord_bld, coord, half);
      /* convert to int, compute lerp weight */
      lp_build_ifloor_fract(coord_bld, coord, &coord0, &weight);
      coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);
      break;

   case PIPE_TEX_WRAP_MIRROR_REPEAT:
      /* compute mirror function */
      coord = lp_build_coord_mirror(bld, coord);

      /* scale coord to length */
      coord = lp_build_mul(coord_bld, coord, length_f);
      coord = lp_build_sub(coord_bld, coord, half);
      if (offset) {
         offset = lp_build_int_to_float(coord_bld, offset);
         coord = lp_build_add(coord_bld, coord, offset);
      }

      /* convert to int, compute lerp weight */
      lp_build_ifloor_fract(coord_bld, coord, &coord0, &weight);
      coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);

      /* coord0 = max(coord0, 0) */
      coord0 = lp_build_max(int_coord_bld, coord0, int_coord_bld->zero);
      /* coord1 = min(coord1, length-1) */
      coord1 = lp_build_min(int_coord_bld, coord1, length_minus_one);
      break;

   case PIPE_TEX_WRAP_MIRROR_CLAMP:
      if (bld->static_sampler_state->normalized_coords) {
         /* scale coord to length */
         coord = lp_build_mul(coord_bld, coord, length_f);
      }
      if (offset) {
         offset = lp_build_int_to_float(coord_bld, offset);
         coord = lp_build_add(coord_bld, coord, offset);
      }
      coord = lp_build_abs(coord_bld, coord);

      /* clamp to [0, length] */
      coord = lp_build_min(coord_bld, coord, length_f);

      coord = lp_build_sub(coord_bld, coord, half);

      /* convert to int, compute lerp weight */
      lp_build_ifloor_fract(coord_bld, coord, &coord0, &weight);
      coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);
      break;

   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
      {
         struct lp_build_context abs_coord_bld = bld->coord_bld;
         abs_coord_bld.type.sign = FALSE;

         if (bld->static_sampler_state->normalized_coords) {
            /* scale coord to length */
            coord = lp_build_mul(coord_bld, coord, length_f);
         }
         if (offset) {
            offset = lp_build_int_to_float(coord_bld, offset);
            coord = lp_build_add(coord_bld, coord, offset);
         }
         coord = lp_build_abs(coord_bld, coord);

         /* clamp to length max */
         coord = lp_build_min(coord_bld, coord, length_f);
         /* subtract 0.5 */
         coord = lp_build_sub(coord_bld, coord, half);
         /* clamp to [0, length - 0.5] */
         coord = lp_build_max(coord_bld, coord, coord_bld->zero);

         /* convert to int, compute lerp weight */
         lp_build_ifloor_fract(&abs_coord_bld, coord, &coord0, &weight);
         coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);
         /* coord1 = min(coord1, length-1) */
         coord1 = lp_build_min(int_coord_bld, coord1, length_minus_one);
      }
      break;

   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
      {
         if (bld->static_sampler_state->normalized_coords) {
            /* scale coord to length */
            coord = lp_build_mul(coord_bld, coord, length_f);
         }
         if (offset) {
            offset = lp_build_int_to_float(coord_bld, offset);
            coord = lp_build_add(coord_bld, coord, offset);
         }
         coord = lp_build_abs(coord_bld, coord);

         /* was: clamp to [-0.5, length + 0.5] then sub 0.5 */
         /* skip clamp - always positive, and other side
            only potentially matters for very large coords */
         coord = lp_build_sub(coord_bld, coord, half);

         /* convert to int, compute lerp weight */
         lp_build_ifloor_fract(coord_bld, coord, &coord0, &weight);
         coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);
      }
      break;

   default:
      assert(0);
      coord0 = NULL;
      coord1 = NULL;
      weight = NULL;
   }

   *x0_out = coord0;
   *x1_out = coord1;
   *weight_out = weight;
}


/**
 * Build LLVM code for texture wrap mode for nearest filtering.
 * \param coord  the incoming texcoord (nominally in [0,1])
 * \param length  the texture size along one dimension, as int vector
 * \param length_f  the texture size along one dimension, as float vector
 * \param offset  texel offset along one dimension (as int vector)
 * \param is_pot  if TRUE, length is a power of two
 * \param wrap_mode  one of PIPE_TEX_WRAP_x
 */
static LLVMValueRef
lp_build_sample_wrap_nearest(struct lp_build_sample_context *bld,
                             LLVMValueRef coord,
                             LLVMValueRef length,
                             LLVMValueRef length_f,
                             LLVMValueRef offset,
                             boolean is_pot,
                             unsigned wrap_mode)
{
   struct lp_build_context *coord_bld = &bld->coord_bld;
   struct lp_build_context *int_coord_bld = &bld->int_coord_bld;
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef length_minus_one = lp_build_sub(int_coord_bld, length, int_coord_bld->one);
   LLVMValueRef icoord;
   
   switch(wrap_mode) {
   case PIPE_TEX_WRAP_REPEAT:
      if (is_pot) {
         coord = lp_build_mul(coord_bld, coord, length_f);
         icoord = lp_build_ifloor(coord_bld, coord);
         if (offset) {
            icoord = lp_build_add(int_coord_bld, icoord, offset);
         }
         icoord = LLVMBuildAnd(builder, icoord, length_minus_one, "");
      }
      else {
          if (offset) {
             offset = lp_build_int_to_float(coord_bld, offset);
             offset = lp_build_div(coord_bld, offset, length_f);
             coord = lp_build_add(coord_bld, coord, offset);
          }
          /* take fraction, unnormalize */
          coord = lp_build_fract_safe(coord_bld, coord);
          coord = lp_build_mul(coord_bld, coord, length_f);
          icoord = lp_build_itrunc(coord_bld, coord);
      }
      break;

   case PIPE_TEX_WRAP_CLAMP:
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      if (bld->static_sampler_state->normalized_coords) {
         /* scale coord to length */
         coord = lp_build_mul(coord_bld, coord, length_f);
      }

      /* floor */
      /* use itrunc instead since we clamp to 0 anyway */
      icoord = lp_build_itrunc(coord_bld, coord);
      if (offset) {
         icoord = lp_build_add(int_coord_bld, icoord, offset);
      }

      /* clamp to [0, length - 1]. */
      icoord = lp_build_clamp(int_coord_bld, icoord, int_coord_bld->zero,
                              length_minus_one);
      break;

   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      if (bld->static_sampler_state->normalized_coords) {
         /* scale coord to length */
         coord = lp_build_mul(coord_bld, coord, length_f);
      }
      /* no clamp necessary, border masking will handle this */
      icoord = lp_build_ifloor(coord_bld, coord);
      if (offset) {
         icoord = lp_build_add(int_coord_bld, icoord, offset);
      }
      break;

   case PIPE_TEX_WRAP_MIRROR_REPEAT:
      if (offset) {
         offset = lp_build_int_to_float(coord_bld, offset);
         offset = lp_build_div(coord_bld, offset, length_f);
         coord = lp_build_add(coord_bld, coord, offset);
      }
      /* compute mirror function */
      coord = lp_build_coord_mirror(bld, coord);

      /* scale coord to length */
      assert(bld->static_sampler_state->normalized_coords);
      coord = lp_build_mul(coord_bld, coord, length_f);

      /* itrunc == ifloor here */
      icoord = lp_build_itrunc(coord_bld, coord);

      /* clamp to [0, length - 1] */
      icoord = lp_build_min(int_coord_bld, icoord, length_minus_one);
      break;

   case PIPE_TEX_WRAP_MIRROR_CLAMP:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
      if (bld->static_sampler_state->normalized_coords) {
         /* scale coord to length */
         coord = lp_build_mul(coord_bld, coord, length_f);
      }
      if (offset) {
         offset = lp_build_int_to_float(coord_bld, offset);
         coord = lp_build_add(coord_bld, coord, offset);
      }
      coord = lp_build_abs(coord_bld, coord);

      /* itrunc == ifloor here */
      icoord = lp_build_itrunc(coord_bld, coord);

      /* clamp to [0, length - 1] */
      icoord = lp_build_min(int_coord_bld, icoord, length_minus_one);
      break;

   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
      if (bld->static_sampler_state->normalized_coords) {
         /* scale coord to length */
         coord = lp_build_mul(coord_bld, coord, length_f);
      }
      if (offset) {
         offset = lp_build_int_to_float(coord_bld, offset);
         coord = lp_build_add(coord_bld, coord, offset);
      }
      coord = lp_build_abs(coord_bld, coord);

      /* itrunc == ifloor here */
      icoord = lp_build_itrunc(coord_bld, coord);
      break;

   default:
      assert(0);
      icoord = NULL;
   }

   return icoord;
}


/**
 * Do shadow test/comparison.
 * \param p shadow ref value
 * \param texel  the texel to compare against
 */
static LLVMValueRef
lp_build_sample_comparefunc(struct lp_build_sample_context *bld,
                            LLVMValueRef p,
                            LLVMValueRef texel)
{
   struct lp_build_context *texel_bld = &bld->texel_bld;
   LLVMValueRef res;

   if (0) {
      //lp_build_print_value(bld->gallivm, "shadow cmp coord", p);
      lp_build_print_value(bld->gallivm, "shadow cmp texel", texel);
   }

   /* result = (p FUNC texel) ? 1 : 0 */
   /*
    * honor d3d10 floating point rules here, which state that comparisons
    * are ordered except NOT_EQUAL which is unordered.
    */
   if (bld->static_sampler_state->compare_func != PIPE_FUNC_NOTEQUAL) {
      res = lp_build_cmp_ordered(texel_bld, bld->static_sampler_state->compare_func,
                                 p, texel);
   }
   else {
      res = lp_build_cmp(texel_bld, bld->static_sampler_state->compare_func,
                         p, texel);
   }
   return res;
}


/**
 * Generate code to sample a mipmap level with nearest filtering.
 * If sampling a cube texture, r = cube face in [0,5].
 */
static void
lp_build_sample_image_nearest(struct lp_build_sample_context *bld,
                              LLVMValueRef size,
                              LLVMValueRef row_stride_vec,
                              LLVMValueRef img_stride_vec,
                              LLVMValueRef data_ptr,
                              LLVMValueRef mipoffsets,
                              LLVMValueRef *coords,
                              const LLVMValueRef *offsets,
                              LLVMValueRef colors_out[4])
{
   const unsigned dims = bld->dims;
   LLVMValueRef width_vec;
   LLVMValueRef height_vec;
   LLVMValueRef depth_vec;
   LLVMValueRef flt_size;
   LLVMValueRef flt_width_vec;
   LLVMValueRef flt_height_vec;
   LLVMValueRef flt_depth_vec;
   LLVMValueRef x, y = NULL, z = NULL;

   lp_build_extract_image_sizes(bld,
                                &bld->int_size_bld,
                                bld->int_coord_type,
                                size,
                                &width_vec, &height_vec, &depth_vec);

   flt_size = lp_build_int_to_float(&bld->float_size_bld, size);

   lp_build_extract_image_sizes(bld,
                                &bld->float_size_bld,
                                bld->coord_type,
                                flt_size,
                                &flt_width_vec, &flt_height_vec, &flt_depth_vec);

   /*
    * Compute integer texcoords.
    */
   x = lp_build_sample_wrap_nearest(bld, coords[0], width_vec,
                                    flt_width_vec, offsets[0],
                                    bld->static_texture_state->pot_width,
                                    bld->static_sampler_state->wrap_s);
   lp_build_name(x, "tex.x.wrapped");

   if (dims >= 2) {
      y = lp_build_sample_wrap_nearest(bld, coords[1], height_vec,
                                       flt_height_vec, offsets[1],
                                       bld->static_texture_state->pot_height,
                                       bld->static_sampler_state->wrap_t);
      lp_build_name(y, "tex.y.wrapped");

      if (dims == 3) {
         z = lp_build_sample_wrap_nearest(bld, coords[2], depth_vec,
                                          flt_depth_vec, offsets[2],
                                          bld->static_texture_state->pot_depth,
                                          bld->static_sampler_state->wrap_r);
         lp_build_name(z, "tex.z.wrapped");
      }
   }
   if (bld->static_texture_state->target == PIPE_TEXTURE_CUBE ||
       bld->static_texture_state->target == PIPE_TEXTURE_1D_ARRAY ||
       bld->static_texture_state->target == PIPE_TEXTURE_2D_ARRAY) {
      z = coords[2];
      lp_build_name(z, "tex.z.layer");
   }

   /*
    * Get texture colors.
    */
   lp_build_sample_texel_soa(bld,
                             width_vec, height_vec, depth_vec,
                             x, y, z,
                             row_stride_vec, img_stride_vec,
                             data_ptr, mipoffsets, colors_out);

   if (bld->static_sampler_state->compare_mode != PIPE_TEX_COMPARE_NONE) {
      LLVMValueRef cmpval;
      cmpval = lp_build_sample_comparefunc(bld, coords[4], colors_out[0]);
      /* this is really just a AND 1.0, cmpval but llvm is clever enough */
      colors_out[0] = lp_build_select(&bld->texel_bld, cmpval,
                                      bld->texel_bld.one, bld->texel_bld.zero);
      colors_out[1] = colors_out[2] = colors_out[3] = colors_out[0];
   }

}


/**
 * Like a lerp, but inputs are 0/~0 masks, so can simplify slightly.
 */
static LLVMValueRef
lp_build_masklerp(struct lp_build_context *bld,
                 LLVMValueRef weight,
                 LLVMValueRef mask0,
                 LLVMValueRef mask1)
{
   struct gallivm_state *gallivm = bld->gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef weight2;

   weight2 = lp_build_sub(bld, bld->one, weight);
   weight = LLVMBuildBitCast(builder, weight,
                              lp_build_int_vec_type(gallivm, bld->type), "");
   weight2 = LLVMBuildBitCast(builder, weight2,
                              lp_build_int_vec_type(gallivm, bld->type), "");
   weight = LLVMBuildAnd(builder, weight, mask1, "");
   weight2 = LLVMBuildAnd(builder, weight2, mask0, "");
   weight = LLVMBuildBitCast(builder, weight, bld->vec_type, "");
   weight2 = LLVMBuildBitCast(builder, weight2, bld->vec_type, "");
   return lp_build_add(bld, weight, weight2);
}

/**
 * Like a 2d lerp, but inputs are 0/~0 masks, so can simplify slightly.
 */
static LLVMValueRef
lp_build_masklerp2d(struct lp_build_context *bld,
                    LLVMValueRef weight0,
                    LLVMValueRef weight1,
                    LLVMValueRef mask00,
                    LLVMValueRef mask01,
                    LLVMValueRef mask10,
                    LLVMValueRef mask11)
{
   LLVMValueRef val0 = lp_build_masklerp(bld, weight0, mask00, mask01);
   LLVMValueRef val1 = lp_build_masklerp(bld, weight0, mask10, mask11);
   return lp_build_lerp(bld, weight1, val0, val1, 0);
}

/*
 * this is a bit excessive code for something OpenGL just recommends
 * but does not require.
 */
#define ACCURATE_CUBE_CORNERS 1

/**
 * Generate code to sample a mipmap level with linear filtering.
 * If sampling a cube texture, r = cube face in [0,5].
 * If linear_mask is present, only pixels having their mask set
 * will receive linear filtering, the rest will use nearest.
 */
static void
lp_build_sample_image_linear(struct lp_build_sample_context *bld,
                             LLVMValueRef size,
                             LLVMValueRef linear_mask,
                             LLVMValueRef row_stride_vec,
                             LLVMValueRef img_stride_vec,
                             LLVMValueRef data_ptr,
                             LLVMValueRef mipoffsets,
                             LLVMValueRef *coords,
                             const LLVMValueRef *offsets,
                             LLVMValueRef colors_out[4])
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   struct lp_build_context *ivec_bld = &bld->int_coord_bld;
   struct lp_build_context *coord_bld = &bld->coord_bld;
   const unsigned dims = bld->dims;
   LLVMValueRef width_vec;
   LLVMValueRef height_vec;
   LLVMValueRef depth_vec;
   LLVMValueRef flt_size;
   LLVMValueRef flt_width_vec;
   LLVMValueRef flt_height_vec;
   LLVMValueRef flt_depth_vec;
   LLVMValueRef fall_off[4], have_corners;
   LLVMValueRef z1 = NULL;
   LLVMValueRef z00 = NULL, z01 = NULL, z10 = NULL, z11 = NULL;
   LLVMValueRef x00 = NULL, x01 = NULL, x10 = NULL, x11 = NULL;
   LLVMValueRef y00 = NULL, y01 = NULL, y10 = NULL, y11 = NULL;
   LLVMValueRef s_fpart, t_fpart = NULL, r_fpart = NULL;
   LLVMValueRef xs[4], ys[4], zs[4];
   LLVMValueRef neighbors[2][2][4];
   int chan, texel_index;
   boolean seamless_cube_filter, accurate_cube_corners;

   seamless_cube_filter = bld->static_texture_state->target == PIPE_TEXTURE_CUBE &&
                          bld->static_sampler_state->seamless_cube_map;
   accurate_cube_corners = ACCURATE_CUBE_CORNERS && seamless_cube_filter;

   lp_build_extract_image_sizes(bld,
                                &bld->int_size_bld,
                                bld->int_coord_type,
                                size,
                                &width_vec, &height_vec, &depth_vec);

   flt_size = lp_build_int_to_float(&bld->float_size_bld, size);

   lp_build_extract_image_sizes(bld,
                                &bld->float_size_bld,
                                bld->coord_type,
                                flt_size,
                                &flt_width_vec, &flt_height_vec, &flt_depth_vec);

   /*
    * Compute integer texcoords.
    */

   if (!seamless_cube_filter) {
      lp_build_sample_wrap_linear(bld, coords[0], width_vec,
                                  flt_width_vec, offsets[0],
                                  bld->static_texture_state->pot_width,
                                  bld->static_sampler_state->wrap_s,
                                  &x00, &x01, &s_fpart);
      lp_build_name(x00, "tex.x0.wrapped");
      lp_build_name(x01, "tex.x1.wrapped");
      x10 = x00;
      x11 = x01;

      if (dims >= 2) {
         lp_build_sample_wrap_linear(bld, coords[1], height_vec,
                                     flt_height_vec, offsets[1],
                                     bld->static_texture_state->pot_height,
                                     bld->static_sampler_state->wrap_t,
                                     &y00, &y10, &t_fpart);
         lp_build_name(y00, "tex.y0.wrapped");
         lp_build_name(y10, "tex.y1.wrapped");
         y01 = y00;
         y11 = y10;

         if (dims == 3) {
            lp_build_sample_wrap_linear(bld, coords[2], depth_vec,
                                        flt_depth_vec, offsets[2],
                                        bld->static_texture_state->pot_depth,
                                        bld->static_sampler_state->wrap_r,
                                        &z00, &z1, &r_fpart);
            z01 = z10 = z11 = z00;
            lp_build_name(z00, "tex.z0.wrapped");
            lp_build_name(z1, "tex.z1.wrapped");
         }
      }
      if (bld->static_texture_state->target == PIPE_TEXTURE_CUBE ||
          bld->static_texture_state->target == PIPE_TEXTURE_1D_ARRAY ||
          bld->static_texture_state->target == PIPE_TEXTURE_2D_ARRAY) {
         z00 = z01 = z10 = z11 = z1 = coords[2];  /* cube face or layer */
         lp_build_name(z00, "tex.z0.layer");
         lp_build_name(z1, "tex.z1.layer");
      }
   }
   else {
      struct lp_build_if_state edge_if;
      LLVMTypeRef int1t;
      LLVMValueRef new_faces[4], new_xcoords[4][2], new_ycoords[4][2];
      LLVMValueRef coord, have_edge, have_corner;
      LLVMValueRef fall_off_ym_notxm, fall_off_ym_notxp, fall_off_x, fall_off_y;
      LLVMValueRef fall_off_yp_notxm, fall_off_yp_notxp;
      LLVMValueRef x0, x1, y0, y1, y0_clamped, y1_clamped;
      LLVMValueRef face = coords[2];
      LLVMValueRef half = lp_build_const_vec(bld->gallivm, coord_bld->type, 0.5f);
      LLVMValueRef length_minus_one = lp_build_sub(ivec_bld, width_vec, ivec_bld->one);
      /* XXX drop height calcs. Could (should) do this without seamless filtering too */
      height_vec = width_vec;
      flt_height_vec = flt_width_vec;

      /* XXX the overflow logic is actually sort of duplicated with trilinear,
       * since an overflow in one mip should also have a corresponding overflow
       * in another.
       */
      /* should always have normalized coords, and offsets are undefined */
      assert(bld->static_sampler_state->normalized_coords);
      coord = lp_build_mul(coord_bld, coords[0], flt_width_vec);
      /* instead of clamp, build mask if overflowed */
      coord = lp_build_sub(coord_bld, coord, half);
      /* convert to int, compute lerp weight */
      /* not ideal with AVX (and no AVX2) */
      lp_build_ifloor_fract(coord_bld, coord, &x0, &s_fpart);
      x1 = lp_build_add(ivec_bld, x0, ivec_bld->one);
      coord = lp_build_mul(coord_bld, coords[1], flt_height_vec);
      coord = lp_build_sub(coord_bld, coord, half);
      lp_build_ifloor_fract(coord_bld, coord, &y0, &t_fpart);
      y1 = lp_build_add(ivec_bld, y0, ivec_bld->one);

      fall_off[0] = lp_build_cmp(ivec_bld, PIPE_FUNC_LESS, x0, ivec_bld->zero);
      fall_off[1] = lp_build_cmp(ivec_bld, PIPE_FUNC_GREATER, x1, length_minus_one);
      fall_off[2] = lp_build_cmp(ivec_bld, PIPE_FUNC_LESS, y0, ivec_bld->zero);
      fall_off[3] = lp_build_cmp(ivec_bld, PIPE_FUNC_GREATER, y1, length_minus_one);

      fall_off_x = lp_build_or(ivec_bld, fall_off[0], fall_off[1]);
      fall_off_y = lp_build_or(ivec_bld, fall_off[2], fall_off[3]);
      have_edge = lp_build_or(ivec_bld, fall_off_x, fall_off_y);
      have_edge = lp_build_any_true_range(ivec_bld, ivec_bld->type.length, have_edge);

      /* needed for accurate corner filtering branch later, rely on 0 init */
      int1t = LLVMInt1TypeInContext(bld->gallivm->context);
      have_corners = lp_build_alloca(bld->gallivm, int1t, "have_corner");

      for (texel_index = 0; texel_index < 4; texel_index++) {
         xs[texel_index] = lp_build_alloca(bld->gallivm, ivec_bld->vec_type, "xs");
         ys[texel_index] = lp_build_alloca(bld->gallivm, ivec_bld->vec_type, "ys");
         zs[texel_index] = lp_build_alloca(bld->gallivm, ivec_bld->vec_type, "zs");
      }

      lp_build_if(&edge_if, bld->gallivm, have_edge);

      have_corner = lp_build_and(ivec_bld, fall_off_x, fall_off_y);
      have_corner = lp_build_any_true_range(ivec_bld, ivec_bld->type.length, have_corner);
      LLVMBuildStore(builder, have_corner, have_corners);

      /*
       * Need to feed clamped values here for cheap corner handling,
       * but only for y coord (as when falling off both edges we only
       * fall off the x one) - this should be sufficient.
       */
      y0_clamped = lp_build_max(ivec_bld, y0, ivec_bld->zero);
      y1_clamped = lp_build_min(ivec_bld, y1, length_minus_one);

      /*
       * Get all possible new coords.
       */
      lp_build_cube_new_coords(ivec_bld, face,
                               x0, x1, y0_clamped, y1_clamped,
                               length_minus_one,
                               new_faces, new_xcoords, new_ycoords);

      /* handle fall off x-, x+ direction */
      /* determine new coords, face (not both fall_off vars can be true at same time) */
      x00 = lp_build_select(ivec_bld, fall_off[0], new_xcoords[0][0], x0);
      y00 = lp_build_select(ivec_bld, fall_off[0], new_ycoords[0][0], y0_clamped);
      x10 = lp_build_select(ivec_bld, fall_off[0], new_xcoords[0][1], x0);
      y10 = lp_build_select(ivec_bld, fall_off[0], new_ycoords[0][1], y1_clamped);
      x01 = lp_build_select(ivec_bld, fall_off[1], new_xcoords[1][0], x1);
      y01 = lp_build_select(ivec_bld, fall_off[1], new_ycoords[1][0], y0_clamped);
      x11 = lp_build_select(ivec_bld, fall_off[1], new_xcoords[1][1], x1);
      y11 = lp_build_select(ivec_bld, fall_off[1], new_ycoords[1][1], y1_clamped);

      z00 = z10 = lp_build_select(ivec_bld, fall_off[0], new_faces[0], face);
      z01 = z11 = lp_build_select(ivec_bld, fall_off[1], new_faces[1], face);

      /* handle fall off y-, y+ direction */
      /*
       * Cheap corner logic: just hack up things so a texel doesn't fall
       * off both sides (which means filter weights will be wrong but we'll only
       * use valid texels in the filter).
       * This means however (y) coords must additionally be clamped (see above).
       * This corner handling should be fully OpenGL (but not d3d10) compliant.
       */
      fall_off_ym_notxm = lp_build_andnot(ivec_bld, fall_off[2], fall_off[0]);
      fall_off_ym_notxp = lp_build_andnot(ivec_bld, fall_off[2], fall_off[1]);
      fall_off_yp_notxm = lp_build_andnot(ivec_bld, fall_off[3], fall_off[0]);
      fall_off_yp_notxp = lp_build_andnot(ivec_bld, fall_off[3], fall_off[1]);

      x00 = lp_build_select(ivec_bld, fall_off_ym_notxm, new_xcoords[2][0], x00);
      y00 = lp_build_select(ivec_bld, fall_off_ym_notxm, new_ycoords[2][0], y00);
      x01 = lp_build_select(ivec_bld, fall_off_ym_notxp, new_xcoords[2][1], x01);
      y01 = lp_build_select(ivec_bld, fall_off_ym_notxp, new_ycoords[2][1], y01);
      x10 = lp_build_select(ivec_bld, fall_off_yp_notxm, new_xcoords[3][0], x10);
      y10 = lp_build_select(ivec_bld, fall_off_yp_notxm, new_ycoords[3][0], y10);
      x11 = lp_build_select(ivec_bld, fall_off_yp_notxp, new_xcoords[3][1], x11);
      y11 = lp_build_select(ivec_bld, fall_off_yp_notxp, new_ycoords[3][1], y11);

      z00 = lp_build_select(ivec_bld, fall_off_ym_notxm, new_faces[2], z00);
      z01 = lp_build_select(ivec_bld, fall_off_ym_notxp, new_faces[2], z01);
      z10 = lp_build_select(ivec_bld, fall_off_yp_notxm, new_faces[3], z10);
      z11 = lp_build_select(ivec_bld, fall_off_yp_notxp, new_faces[3], z11);

      LLVMBuildStore(builder, x00, xs[0]);
      LLVMBuildStore(builder, x01, xs[1]);
      LLVMBuildStore(builder, x10, xs[2]);
      LLVMBuildStore(builder, x11, xs[3]);
      LLVMBuildStore(builder, y00, ys[0]);
      LLVMBuildStore(builder, y01, ys[1]);
      LLVMBuildStore(builder, y10, ys[2]);
      LLVMBuildStore(builder, y11, ys[3]);
      LLVMBuildStore(builder, z00, zs[0]);
      LLVMBuildStore(builder, z01, zs[1]);
      LLVMBuildStore(builder, z10, zs[2]);
      LLVMBuildStore(builder, z11, zs[3]);

      lp_build_else(&edge_if);

      LLVMBuildStore(builder, x0, xs[0]);
      LLVMBuildStore(builder, x1, xs[1]);
      LLVMBuildStore(builder, x0, xs[2]);
      LLVMBuildStore(builder, x1, xs[3]);
      LLVMBuildStore(builder, y0, ys[0]);
      LLVMBuildStore(builder, y0, ys[1]);
      LLVMBuildStore(builder, y1, ys[2]);
      LLVMBuildStore(builder, y1, ys[3]);
      LLVMBuildStore(builder, face, zs[0]);
      LLVMBuildStore(builder, face, zs[1]);
      LLVMBuildStore(builder, face, zs[2]);
      LLVMBuildStore(builder, face, zs[3]);

      lp_build_endif(&edge_if);

      x00 = LLVMBuildLoad(builder, xs[0], "");
      x01 = LLVMBuildLoad(builder, xs[1], "");
      x10 = LLVMBuildLoad(builder, xs[2], "");
      x11 = LLVMBuildLoad(builder, xs[3], "");
      y00 = LLVMBuildLoad(builder, ys[0], "");
      y01 = LLVMBuildLoad(builder, ys[1], "");
      y10 = LLVMBuildLoad(builder, ys[2], "");
      y11 = LLVMBuildLoad(builder, ys[3], "");
      z00 = LLVMBuildLoad(builder, zs[0], "");
      z01 = LLVMBuildLoad(builder, zs[1], "");
      z10 = LLVMBuildLoad(builder, zs[2], "");
      z11 = LLVMBuildLoad(builder, zs[3], "");
   }

   if (linear_mask) {
      /*
       * Whack filter weights into place. Whatever texel had more weight is
       * the one which should have been selected by nearest filtering hence
       * just use 100% weight for it.
       */
      struct lp_build_context *c_bld = &bld->coord_bld;
      LLVMValueRef w1_mask, w1_weight;
      LLVMValueRef half = lp_build_const_vec(bld->gallivm, c_bld->type, 0.5f);

      w1_mask = lp_build_cmp(c_bld, PIPE_FUNC_GREATER, s_fpart, half);
      /* this select is really just a "and" */
      w1_weight = lp_build_select(c_bld, w1_mask, c_bld->one, c_bld->zero);
      s_fpart = lp_build_select(c_bld, linear_mask, s_fpart, w1_weight);
      if (dims >= 2) {
         w1_mask = lp_build_cmp(c_bld, PIPE_FUNC_GREATER, t_fpart, half);
         w1_weight = lp_build_select(c_bld, w1_mask, c_bld->one, c_bld->zero);
         t_fpart = lp_build_select(c_bld, linear_mask, t_fpart, w1_weight);
         if (dims == 3) {
            w1_mask = lp_build_cmp(c_bld, PIPE_FUNC_GREATER, r_fpart, half);
            w1_weight = lp_build_select(c_bld, w1_mask, c_bld->one, c_bld->zero);
            r_fpart = lp_build_select(c_bld, linear_mask, r_fpart, w1_weight);
         }
      }
   }

   /*
    * Get texture colors.
    */
   /* get x0/x1 texels */
   lp_build_sample_texel_soa(bld,
                             width_vec, height_vec, depth_vec,
                             x00, y00, z00,
                             row_stride_vec, img_stride_vec,
                             data_ptr, mipoffsets, neighbors[0][0]);
   lp_build_sample_texel_soa(bld,
                             width_vec, height_vec, depth_vec,
                             x01, y01, z01,
                             row_stride_vec, img_stride_vec,
                             data_ptr, mipoffsets, neighbors[0][1]);

   if (dims == 1) {
      if (bld->static_sampler_state->compare_mode == PIPE_TEX_COMPARE_NONE) {
         /* Interpolate two samples from 1D image to produce one color */
         for (chan = 0; chan < 4; chan++) {
            colors_out[chan] = lp_build_lerp(&bld->texel_bld, s_fpart,
                                             neighbors[0][0][chan],
                                             neighbors[0][1][chan],
                                             0);
         }
      }
      else {
         LLVMValueRef cmpval0, cmpval1;
         cmpval0 = lp_build_sample_comparefunc(bld, coords[4], neighbors[0][0][0]);
         cmpval1 = lp_build_sample_comparefunc(bld, coords[4], neighbors[0][1][0]);
         /* simplified lerp, AND mask with weight and add */
         colors_out[0] = lp_build_masklerp(&bld->texel_bld, s_fpart,
                                           cmpval0, cmpval1);
         colors_out[1] = colors_out[2] = colors_out[3] = colors_out[0];
      }
   }
   else {
      /* 2D/3D texture */
      struct lp_build_if_state corner_if;
      LLVMValueRef colors0[4], colorss[4];

      /* get x0/x1 texels at y1 */
      lp_build_sample_texel_soa(bld,
                                width_vec, height_vec, depth_vec,
                                x10, y10, z10,
                                row_stride_vec, img_stride_vec,
                                data_ptr, mipoffsets, neighbors[1][0]);
      lp_build_sample_texel_soa(bld,
                                width_vec, height_vec, depth_vec,
                                x11, y11, z11,
                                row_stride_vec, img_stride_vec,
                                data_ptr, mipoffsets, neighbors[1][1]);

      /*
       * To avoid having to duplicate linear_mask / fetch code use
       * another branch (with corner condition though edge would work
       * as well) here.
       */
      if (accurate_cube_corners) {
         LLVMValueRef w00, w01, w10, w11, wx0, wy0;
         LLVMValueRef c_weight, c00, c01, c10, c11;
         LLVMValueRef have_corner, one_third, tmp;

         colorss[0] = lp_build_alloca(bld->gallivm, coord_bld->vec_type, "cs");
         colorss[1] = lp_build_alloca(bld->gallivm, coord_bld->vec_type, "cs");
         colorss[2] = lp_build_alloca(bld->gallivm, coord_bld->vec_type, "cs");
         colorss[3] = lp_build_alloca(bld->gallivm, coord_bld->vec_type, "cs");

         have_corner = LLVMBuildLoad(builder, have_corners, "");

         lp_build_if(&corner_if, bld->gallivm, have_corner);

         /*
          * we can't use standard 2d lerp as we need per-element weight
          * in case of corners, so just calculate bilinear result as
          * w00*s00 + w01*s01 + w10*s10 + w11*s11.
          * (This is actually less work than using 2d lerp, 7 vs. 9 instructions,
          * however calculating the weights needs another 6, so actually probably
          * not slower than 2d lerp only for 4 channels as weights only need
          * to be calculated once - of course fixing the weights has additional cost.)
          */
         wx0 = lp_build_sub(coord_bld, coord_bld->one, s_fpart);
         wy0 = lp_build_sub(coord_bld, coord_bld->one, t_fpart);
         w00 = lp_build_mul(coord_bld, wx0, wy0);
         w01 = lp_build_mul(coord_bld, s_fpart, wy0);
         w10 = lp_build_mul(coord_bld, wx0, t_fpart);
         w11 = lp_build_mul(coord_bld, s_fpart, t_fpart);

         /* find corner weight */
         c00 = lp_build_and(ivec_bld, fall_off[0], fall_off[2]);
         c_weight = lp_build_select(coord_bld, c00, w00, coord_bld->zero);
         c01 = lp_build_and(ivec_bld, fall_off[1], fall_off[2]);
         c_weight = lp_build_select(coord_bld, c01, w01, c_weight);
         c10 = lp_build_and(ivec_bld, fall_off[0], fall_off[3]);
         c_weight = lp_build_select(coord_bld, c10, w10, c_weight);
         c11 = lp_build_and(ivec_bld, fall_off[1], fall_off[3]);
         c_weight = lp_build_select(coord_bld, c11, w11, c_weight);

         /*
          * add 1/3 of the corner weight to each of the 3 other samples
          * and null out corner weight
          */
         one_third = lp_build_const_vec(bld->gallivm, coord_bld->type, 1.0f/3.0f);
         c_weight = lp_build_mul(coord_bld, c_weight, one_third);
         w00 = lp_build_add(coord_bld, w00, c_weight);
         c00 = LLVMBuildBitCast(builder, c00, coord_bld->vec_type, "");
         w00 = lp_build_andnot(coord_bld, w00, c00);
         w01 = lp_build_add(coord_bld, w01, c_weight);
         c01 = LLVMBuildBitCast(builder, c01, coord_bld->vec_type, "");
         w01 = lp_build_andnot(coord_bld, w01, c01);
         w10 = lp_build_add(coord_bld, w10, c_weight);
         c10 = LLVMBuildBitCast(builder, c10, coord_bld->vec_type, "");
         w10 = lp_build_andnot(coord_bld, w10, c10);
         w11 = lp_build_add(coord_bld, w11, c_weight);
         c11 = LLVMBuildBitCast(builder, c11, coord_bld->vec_type, "");
         w11 = lp_build_andnot(coord_bld, w11, c11);

         if (bld->static_sampler_state->compare_mode == PIPE_TEX_COMPARE_NONE) {
            for (chan = 0; chan < 4; chan++) {
               colors0[chan] = lp_build_mul(coord_bld, w00, neighbors[0][0][chan]);
               tmp = lp_build_mul(coord_bld, w01, neighbors[0][1][chan]);
               colors0[chan] = lp_build_add(coord_bld, tmp, colors0[chan]);
               tmp = lp_build_mul(coord_bld, w10, neighbors[1][0][chan]);
               colors0[chan] = lp_build_add(coord_bld, tmp, colors0[chan]);
               tmp = lp_build_mul(coord_bld, w11, neighbors[1][1][chan]);
               colors0[chan] = lp_build_add(coord_bld, tmp, colors0[chan]);
            }
         }
         else {
            LLVMValueRef cmpval00, cmpval01, cmpval10, cmpval11;
            cmpval00 = lp_build_sample_comparefunc(bld, coords[4], neighbors[0][0][0]);
            cmpval01 = lp_build_sample_comparefunc(bld, coords[4], neighbors[0][1][0]);
            cmpval10 = lp_build_sample_comparefunc(bld, coords[4], neighbors[1][0][0]);
            cmpval11 = lp_build_sample_comparefunc(bld, coords[4], neighbors[1][1][0]);
            /* inputs to interpolation are just masks so just add masked weights together */
            cmpval00 = LLVMBuildBitCast(builder, cmpval00, coord_bld->vec_type, "");
            cmpval01 = LLVMBuildBitCast(builder, cmpval01, coord_bld->vec_type, "");
            cmpval10 = LLVMBuildBitCast(builder, cmpval10, coord_bld->vec_type, "");
            cmpval11 = LLVMBuildBitCast(builder, cmpval11, coord_bld->vec_type, "");
            colors0[0] = lp_build_and(coord_bld, w00, cmpval00);
            tmp = lp_build_and(coord_bld, w01, cmpval01);
            colors0[0] = lp_build_add(coord_bld, tmp, colors0[0]);
            tmp = lp_build_and(coord_bld, w10, cmpval10);
            colors0[0] = lp_build_add(coord_bld, tmp, colors0[0]);
            tmp = lp_build_and(coord_bld, w11, cmpval11);
            colors0[0] = lp_build_add(coord_bld, tmp, colors0[0]);
            colors0[1] = colors0[2] = colors0[3] = colors0[0];
         }

         LLVMBuildStore(builder, colors0[0], colorss[0]);
         LLVMBuildStore(builder, colors0[1], colorss[1]);
         LLVMBuildStore(builder, colors0[2], colorss[2]);
         LLVMBuildStore(builder, colors0[3], colorss[3]);

         lp_build_else(&corner_if);
      }

      if (bld->static_sampler_state->compare_mode == PIPE_TEX_COMPARE_NONE) {
         /* Bilinear interpolate the four samples from the 2D image / 3D slice */
         for (chan = 0; chan < 4; chan++) {
            colors0[chan] = lp_build_lerp_2d(&bld->texel_bld,
                                             s_fpart, t_fpart,
                                             neighbors[0][0][chan],
                                             neighbors[0][1][chan],
                                             neighbors[1][0][chan],
                                             neighbors[1][1][chan],
                                             0);
         }
      }
      else {
         LLVMValueRef cmpval00, cmpval01, cmpval10, cmpval11;
         cmpval00 = lp_build_sample_comparefunc(bld, coords[4], neighbors[0][0][0]);
         cmpval01 = lp_build_sample_comparefunc(bld, coords[4], neighbors[0][1][0]);
         cmpval10 = lp_build_sample_comparefunc(bld, coords[4], neighbors[1][0][0]);
         cmpval11 = lp_build_sample_comparefunc(bld, coords[4], neighbors[1][1][0]);
         colors0[0] = lp_build_masklerp2d(&bld->texel_bld, s_fpart, t_fpart,
                                          cmpval00, cmpval01, cmpval10, cmpval11);
         colors0[1] = colors0[2] = colors0[3] = colors0[0];
      }

      if (accurate_cube_corners) {
         LLVMBuildStore(builder, colors0[0], colorss[0]);
         LLVMBuildStore(builder, colors0[1], colorss[1]);
         LLVMBuildStore(builder, colors0[2], colorss[2]);
         LLVMBuildStore(builder, colors0[3], colorss[3]);

         lp_build_endif(&corner_if);

         colors0[0] = LLVMBuildLoad(builder, colorss[0], "");
         colors0[1] = LLVMBuildLoad(builder, colorss[1], "");
         colors0[2] = LLVMBuildLoad(builder, colorss[2], "");
         colors0[3] = LLVMBuildLoad(builder, colorss[3], "");
      }

      if (dims == 3) {
         LLVMValueRef neighbors1[2][2][4];
         LLVMValueRef colors1[4];

         /* get x0/x1/y0/y1 texels at z1 */
         lp_build_sample_texel_soa(bld,
                                   width_vec, height_vec, depth_vec,
                                   x00, y00, z1,
                                   row_stride_vec, img_stride_vec,
                                   data_ptr, mipoffsets, neighbors1[0][0]);
         lp_build_sample_texel_soa(bld,
                                   width_vec, height_vec, depth_vec,
                                   x01, y01, z1,
                                   row_stride_vec, img_stride_vec,
                                   data_ptr, mipoffsets, neighbors1[0][1]);
         lp_build_sample_texel_soa(bld,
                                   width_vec, height_vec, depth_vec,
                                   x10, y10, z1,
                                   row_stride_vec, img_stride_vec,
                                   data_ptr, mipoffsets, neighbors1[1][0]);
         lp_build_sample_texel_soa(bld,
                                   width_vec, height_vec, depth_vec,
                                   x11, y11, z1,
                                   row_stride_vec, img_stride_vec,
                                   data_ptr, mipoffsets, neighbors1[1][1]);

         if (bld->static_sampler_state->compare_mode == PIPE_TEX_COMPARE_NONE) {
            /* Bilinear interpolate the four samples from the second Z slice */
            for (chan = 0; chan < 4; chan++) {
               colors1[chan] = lp_build_lerp_2d(&bld->texel_bld,
                                                s_fpart, t_fpart,
                                                neighbors1[0][0][chan],
                                                neighbors1[0][1][chan],
                                                neighbors1[1][0][chan],
                                                neighbors1[1][1][chan],
                                                0);
            }
            /* Linearly interpolate the two samples from the two 3D slices */
            for (chan = 0; chan < 4; chan++) {
               colors_out[chan] = lp_build_lerp(&bld->texel_bld,
                                                r_fpart,
                                                colors0[chan], colors1[chan],
                                                0);
            }
         }
         else {
            LLVMValueRef cmpval00, cmpval01, cmpval10, cmpval11;
            cmpval00 = lp_build_sample_comparefunc(bld, coords[4], neighbors[0][0][0]);
            cmpval01 = lp_build_sample_comparefunc(bld, coords[4], neighbors[0][1][0]);
            cmpval10 = lp_build_sample_comparefunc(bld, coords[4], neighbors[1][0][0]);
            cmpval11 = lp_build_sample_comparefunc(bld, coords[4], neighbors[1][1][0]);
            colors1[0] = lp_build_masklerp2d(&bld->texel_bld, s_fpart, t_fpart,
                                             cmpval00, cmpval01, cmpval10, cmpval11);
            /* Linearly interpolate the two samples from the two 3D slices */
            colors_out[0] = lp_build_lerp(&bld->texel_bld,
                                             r_fpart,
                                             colors0[0], colors1[0],
                                             0);
            colors_out[1] = colors_out[2] = colors_out[3] = colors_out[0];
         }
      }
      else {
         /* 2D tex */
         for (chan = 0; chan < 4; chan++) {
            colors_out[chan] = colors0[chan];
         }
      }
   }
}


/**
 * Sample the texture/mipmap using given image filter and mip filter.
 * ilevel0 and ilevel1 indicate the two mipmap levels to sample
 * from (vectors or scalars).
 * If we're using nearest miplevel sampling the '1' values will be null/unused.
 */
static void
lp_build_sample_mipmap(struct lp_build_sample_context *bld,
                       unsigned img_filter,
                       unsigned mip_filter,
                       LLVMValueRef *coords,
                       const LLVMValueRef *offsets,
                       LLVMValueRef ilevel0,
                       LLVMValueRef ilevel1,
                       LLVMValueRef lod_fpart,
                       LLVMValueRef *colors_out)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef size0 = NULL;
   LLVMValueRef size1 = NULL;
   LLVMValueRef row_stride0_vec = NULL;
   LLVMValueRef row_stride1_vec = NULL;
   LLVMValueRef img_stride0_vec = NULL;
   LLVMValueRef img_stride1_vec = NULL;
   LLVMValueRef data_ptr0 = NULL;
   LLVMValueRef data_ptr1 = NULL;
   LLVMValueRef mipoff0 = NULL;
   LLVMValueRef mipoff1 = NULL;
   LLVMValueRef colors0[4], colors1[4];
   unsigned chan;

   /* sample the first mipmap level */
   lp_build_mipmap_level_sizes(bld, ilevel0,
                               &size0,
                               &row_stride0_vec, &img_stride0_vec);
   if (bld->num_mips == 1) {
      data_ptr0 = lp_build_get_mipmap_level(bld, ilevel0);
   }
   else {
      /* This path should work for num_lods 1 too but slightly less efficient */
      data_ptr0 = bld->base_ptr;
      mipoff0 = lp_build_get_mip_offsets(bld, ilevel0);
   }
   if (img_filter == PIPE_TEX_FILTER_NEAREST) {
      lp_build_sample_image_nearest(bld, size0,
                                    row_stride0_vec, img_stride0_vec,
                                    data_ptr0, mipoff0, coords, offsets,
                                    colors0);
   }
   else {
      assert(img_filter == PIPE_TEX_FILTER_LINEAR);
      lp_build_sample_image_linear(bld, size0, NULL,
                                   row_stride0_vec, img_stride0_vec,
                                   data_ptr0, mipoff0, coords, offsets,
                                   colors0);
   }

   /* Store the first level's colors in the output variables */
   for (chan = 0; chan < 4; chan++) {
       LLVMBuildStore(builder, colors0[chan], colors_out[chan]);
   }

   if (mip_filter == PIPE_TEX_MIPFILTER_LINEAR) {
      struct lp_build_if_state if_ctx;
      LLVMValueRef need_lerp;

      /* need_lerp = lod_fpart > 0 */
      if (bld->num_lods == 1) {
         need_lerp = LLVMBuildFCmp(builder, LLVMRealUGT,
                                   lod_fpart, bld->lodf_bld.zero,
                                   "need_lerp");
      }
      else {
         /*
          * We'll do mip filtering if any of the quads (or individual
          * pixel in case of per-pixel lod) need it.
          * It might be better to split the vectors here and only fetch/filter
          * quads which need it (if there's one lod per quad).
          */
         need_lerp = lp_build_compare(bld->gallivm, bld->lodf_bld.type,
                                      PIPE_FUNC_GREATER,
                                      lod_fpart, bld->lodf_bld.zero);
         need_lerp = lp_build_any_true_range(&bld->lodi_bld, bld->num_lods, need_lerp);
      }

      lp_build_if(&if_ctx, bld->gallivm, need_lerp);
      {
         /*
          * We unfortunately need to clamp lod_fpart here since we can get
          * negative values which would screw up filtering if not all
          * lod_fpart values have same sign.
          */
         lod_fpart = lp_build_max(&bld->lodf_bld, lod_fpart,
                                  bld->lodf_bld.zero);
         /* sample the second mipmap level */
         lp_build_mipmap_level_sizes(bld, ilevel1,
                                     &size1,
                                     &row_stride1_vec, &img_stride1_vec);
         if (bld->num_mips == 1) {
            data_ptr1 = lp_build_get_mipmap_level(bld, ilevel1);
         }
         else {
            data_ptr1 = bld->base_ptr;
            mipoff1 = lp_build_get_mip_offsets(bld, ilevel1);
         }
         if (img_filter == PIPE_TEX_FILTER_NEAREST) {
            lp_build_sample_image_nearest(bld, size1,
                                          row_stride1_vec, img_stride1_vec,
                                          data_ptr1, mipoff1, coords, offsets,
                                          colors1);
         }
         else {
            lp_build_sample_image_linear(bld, size1, NULL,
                                         row_stride1_vec, img_stride1_vec,
                                         data_ptr1, mipoff1, coords, offsets,
                                         colors1);
         }

         /* interpolate samples from the two mipmap levels */

         if (bld->num_lods != bld->coord_type.length)
            lod_fpart = lp_build_unpack_broadcast_aos_scalars(bld->gallivm,
                                                              bld->lodf_bld.type,
                                                              bld->texel_bld.type,
                                                              lod_fpart);

         for (chan = 0; chan < 4; chan++) {
            colors0[chan] = lp_build_lerp(&bld->texel_bld, lod_fpart,
                                          colors0[chan], colors1[chan],
                                          0);
            LLVMBuildStore(builder, colors0[chan], colors_out[chan]);
         }
      }
      lp_build_endif(&if_ctx);
   }
}


/**
 * Sample the texture/mipmap using given mip filter, and using
 * both nearest and linear filtering at the same time depending
 * on linear_mask.
 * lod can be per quad but linear_mask is always per pixel.
 * ilevel0 and ilevel1 indicate the two mipmap levels to sample
 * from (vectors or scalars).
 * If we're using nearest miplevel sampling the '1' values will be null/unused.
 */
static void
lp_build_sample_mipmap_both(struct lp_build_sample_context *bld,
                            LLVMValueRef linear_mask,
                            unsigned mip_filter,
                            LLVMValueRef *coords,
                            const LLVMValueRef *offsets,
                            LLVMValueRef ilevel0,
                            LLVMValueRef ilevel1,
                            LLVMValueRef lod_fpart,
                            LLVMValueRef lod_positive,
                            LLVMValueRef *colors_out)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef size0 = NULL;
   LLVMValueRef size1 = NULL;
   LLVMValueRef row_stride0_vec = NULL;
   LLVMValueRef row_stride1_vec = NULL;
   LLVMValueRef img_stride0_vec = NULL;
   LLVMValueRef img_stride1_vec = NULL;
   LLVMValueRef data_ptr0 = NULL;
   LLVMValueRef data_ptr1 = NULL;
   LLVMValueRef mipoff0 = NULL;
   LLVMValueRef mipoff1 = NULL;
   LLVMValueRef colors0[4], colors1[4];
   unsigned chan;

   /* sample the first mipmap level */
   lp_build_mipmap_level_sizes(bld, ilevel0,
                               &size0,
                               &row_stride0_vec, &img_stride0_vec);
   if (bld->num_mips == 1) {
      data_ptr0 = lp_build_get_mipmap_level(bld, ilevel0);
   }
   else {
      /* This path should work for num_lods 1 too but slightly less efficient */
      data_ptr0 = bld->base_ptr;
      mipoff0 = lp_build_get_mip_offsets(bld, ilevel0);
   }

   lp_build_sample_image_linear(bld, size0, linear_mask,
                                row_stride0_vec, img_stride0_vec,
                                data_ptr0, mipoff0, coords, offsets,
                                colors0);

   /* Store the first level's colors in the output variables */
   for (chan = 0; chan < 4; chan++) {
       LLVMBuildStore(builder, colors0[chan], colors_out[chan]);
   }

   if (mip_filter == PIPE_TEX_MIPFILTER_LINEAR) {
      struct lp_build_if_state if_ctx;
      LLVMValueRef need_lerp;

      /*
       * We'll do mip filtering if any of the quads (or individual
       * pixel in case of per-pixel lod) need it.
       * Note using lod_positive here not lod_fpart since it may be the same
       * condition as that used in the outer "if" in the caller hence llvm
       * should be able to merge the branches in this case.
       */
      need_lerp = lp_build_any_true_range(&bld->lodi_bld, bld->num_lods, lod_positive);

      lp_build_if(&if_ctx, bld->gallivm, need_lerp);
      {
         /*
          * We unfortunately need to clamp lod_fpart here since we can get
          * negative values which would screw up filtering if not all
          * lod_fpart values have same sign.
          */
         lod_fpart = lp_build_max(&bld->lodf_bld, lod_fpart,
                                  bld->lodf_bld.zero);
         /* sample the second mipmap level */
         lp_build_mipmap_level_sizes(bld, ilevel1,
                                     &size1,
                                     &row_stride1_vec, &img_stride1_vec);
         if (bld->num_mips == 1) {
            data_ptr1 = lp_build_get_mipmap_level(bld, ilevel1);
         }
         else {
            data_ptr1 = bld->base_ptr;
            mipoff1 = lp_build_get_mip_offsets(bld, ilevel1);
         }

         lp_build_sample_image_linear(bld, size1, linear_mask,
                                      row_stride1_vec, img_stride1_vec,
                                      data_ptr1, mipoff1, coords, offsets,
                                      colors1);

         /* interpolate samples from the two mipmap levels */

         if (bld->num_lods != bld->coord_type.length)
            lod_fpart = lp_build_unpack_broadcast_aos_scalars(bld->gallivm,
                                                              bld->lodf_bld.type,
                                                              bld->texel_bld.type,
                                                              lod_fpart);

         for (chan = 0; chan < 4; chan++) {
            colors0[chan] = lp_build_lerp(&bld->texel_bld, lod_fpart,
                                          colors0[chan], colors1[chan],
                                          0);
            LLVMBuildStore(builder, colors0[chan], colors_out[chan]);
         }
      }
      lp_build_endif(&if_ctx);
   }
}


/**
 * Build (per-coord) layer value.
 * Either clamp layer to valid values or fill in optional out_of_bounds
 * value and just return value unclamped.
 */
static LLVMValueRef
lp_build_layer_coord(struct lp_build_sample_context *bld,
                     unsigned texture_unit,
                     LLVMValueRef layer,
                     LLVMValueRef *out_of_bounds)
{
   LLVMValueRef num_layers;
   struct lp_build_context *int_coord_bld = &bld->int_coord_bld;

   num_layers = bld->dynamic_state->depth(bld->dynamic_state,
                                          bld->gallivm, texture_unit);

   if (out_of_bounds) {
      LLVMValueRef out1, out;
      num_layers = lp_build_broadcast_scalar(int_coord_bld, num_layers);
      out = lp_build_cmp(int_coord_bld, PIPE_FUNC_LESS, layer, int_coord_bld->zero);
      out1 = lp_build_cmp(int_coord_bld, PIPE_FUNC_GEQUAL, layer, num_layers);
      *out_of_bounds = lp_build_or(int_coord_bld, out, out1);
      return layer;
   }
   else {
      LLVMValueRef maxlayer;
      maxlayer = lp_build_sub(&bld->int_bld, num_layers, bld->int_bld.one);
      maxlayer = lp_build_broadcast_scalar(int_coord_bld, maxlayer);
      return lp_build_clamp(int_coord_bld, layer, int_coord_bld->zero, maxlayer);
   }
}


/**
 * Calculate cube face, lod, mip levels.
 */
static void
lp_build_sample_common(struct lp_build_sample_context *bld,
                       unsigned texture_index,
                       unsigned sampler_index,
                       LLVMValueRef *coords,
                       const struct lp_derivatives *derivs, /* optional */
                       LLVMValueRef lod_bias, /* optional */
                       LLVMValueRef explicit_lod, /* optional */
                       LLVMValueRef *lod_pos_or_zero,
                       LLVMValueRef *lod_fpart,
                       LLVMValueRef *ilevel0,
                       LLVMValueRef *ilevel1)
{
   const unsigned mip_filter = bld->static_sampler_state->min_mip_filter;
   const unsigned min_filter = bld->static_sampler_state->min_img_filter;
   const unsigned mag_filter = bld->static_sampler_state->mag_img_filter;
   const unsigned target = bld->static_texture_state->target;
   LLVMValueRef first_level, cube_rho = NULL;
   LLVMValueRef lod_ipart = NULL;
   struct lp_derivatives cube_derivs;

   /*
   printf("%s mip %d  min %d  mag %d\n", __FUNCTION__,
          mip_filter, min_filter, mag_filter);
   */

   /*
    * Choose cube face, recompute texcoords for the chosen face and
    * compute rho here too (as it requires transform of derivatives).
    */
   if (target == PIPE_TEXTURE_CUBE) {
      boolean need_derivs;
      need_derivs = ((min_filter != mag_filter ||
                      mip_filter != PIPE_TEX_MIPFILTER_NONE) &&
                      !bld->static_sampler_state->min_max_lod_equal &&
                      !explicit_lod);
      lp_build_cube_lookup(bld, coords, derivs, &cube_rho, &cube_derivs, need_derivs);
      derivs = &cube_derivs;
   }
   else if (target == PIPE_TEXTURE_1D_ARRAY ||
            target == PIPE_TEXTURE_2D_ARRAY) {
      coords[2] = lp_build_iround(&bld->coord_bld, coords[2]);
      coords[2] = lp_build_layer_coord(bld, texture_index, coords[2], NULL);
   }

   if (bld->static_sampler_state->compare_mode != PIPE_TEX_COMPARE_NONE) {
      /*
       * Clamp p coords to [0,1] for fixed function depth texture format here.
       * Technically this is not entirely correct for unorm depth as the ref value
       * should be converted to the depth format (quantization!) and comparison
       * then done in texture format. This would actually help performance (since
       * only need to do it once and could save the per-sample conversion of texels
       * to floats instead), but it would need more messy code (would need to push
       * at least some bits down to actual fetch so conversion could be skipped,
       * and would have ugly interaction with border color, would need to convert
       * border color to that format too or do some other tricks to make it work).
       */
      const struct util_format_description *format_desc = bld->format_desc;
      unsigned chan_type;
      /* not entirely sure we couldn't end up with non-valid swizzle here */
      chan_type = format_desc->swizzle[0] <= UTIL_FORMAT_SWIZZLE_W ?
                     format_desc->channel[format_desc->swizzle[0]].type :
                     UTIL_FORMAT_TYPE_FLOAT;
      if (chan_type != UTIL_FORMAT_TYPE_FLOAT) {
         coords[4] = lp_build_clamp(&bld->coord_bld, coords[4],
                                    bld->coord_bld.zero, bld->coord_bld.one);
      }
   }

   /*
    * Compute the level of detail (float).
    */
   if (min_filter != mag_filter ||
       mip_filter != PIPE_TEX_MIPFILTER_NONE) {
      /* Need to compute lod either to choose mipmap levels or to
       * distinguish between minification/magnification with one mipmap level.
       */
      lp_build_lod_selector(bld, texture_index, sampler_index,
                            coords[0], coords[1], coords[2], cube_rho,
                            derivs, lod_bias, explicit_lod,
                            mip_filter,
                            &lod_ipart, lod_fpart, lod_pos_or_zero);
   } else {
      lod_ipart = bld->lodi_bld.zero;
      *lod_pos_or_zero = bld->lodi_bld.zero;
   }

   if (bld->num_lods != bld->num_mips) {
      /* only makes sense if there's just a single mip level */
      assert(bld->num_mips == 1);
      lod_ipart = lp_build_extract_range(bld->gallivm, lod_ipart, 0, 1);
   }

   /*
    * Compute integer mipmap level(s) to fetch texels from: ilevel0, ilevel1
    */
   switch (mip_filter) {
   default:
      assert(0 && "bad mip_filter value in lp_build_sample_soa()");
      /* fall-through */
   case PIPE_TEX_MIPFILTER_NONE:
      /* always use mip level 0 */
      if (HAVE_LLVM == 0x0207 && target == PIPE_TEXTURE_CUBE) {
         /* XXX this is a work-around for an apparent bug in LLVM 2.7.
          * We should be able to set ilevel0 = const(0) but that causes
          * bad x86 code to be emitted.
          */
         assert(lod_ipart);
         lp_build_nearest_mip_level(bld, texture_index, lod_ipart, ilevel0, NULL);
      }
      else {
         first_level = bld->dynamic_state->first_level(bld->dynamic_state,
                                                       bld->gallivm, texture_index);
         first_level = lp_build_broadcast_scalar(&bld->leveli_bld, first_level);
         *ilevel0 = first_level;
      }
      break;
   case PIPE_TEX_MIPFILTER_NEAREST:
      assert(lod_ipart);
      lp_build_nearest_mip_level(bld, texture_index, lod_ipart, ilevel0, NULL);
      break;
   case PIPE_TEX_MIPFILTER_LINEAR:
      assert(lod_ipart);
      assert(*lod_fpart);
      lp_build_linear_mip_levels(bld, texture_index,
                                 lod_ipart, lod_fpart,
                                 ilevel0, ilevel1);
      break;
   }
}

static void
lp_build_clamp_border_color(struct lp_build_sample_context *bld,
                            unsigned sampler_unit)
{
   struct gallivm_state *gallivm = bld->gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef border_color_ptr =
      bld->dynamic_state->border_color(bld->dynamic_state,
                                        gallivm, sampler_unit);
   LLVMValueRef border_color;
   const struct util_format_description *format_desc = bld->format_desc;
   struct lp_type vec4_type = bld->texel_type;
   struct lp_build_context vec4_bld;
   LLVMValueRef min_clamp = NULL;
   LLVMValueRef max_clamp = NULL;

   /*
    * For normalized format need to clamp border color (technically
    * probably should also quantize the data). Really sucks doing this
    * here but can't avoid at least for now since this is part of
    * sampler state and texture format is part of sampler_view state.
    * GL expects also expects clamping for uint/sint formats too so
    * do that as well (d3d10 can't end up here with uint/sint since it
    * only supports them with ld).
    */
   vec4_type.length = 4;
   lp_build_context_init(&vec4_bld, gallivm, vec4_type);

   /*
    * Vectorized clamping of border color. Loading is a bit of a hack since
    * we just cast the pointer to float array to pointer to vec4
    * (int or float).
    */
   border_color_ptr = lp_build_array_get_ptr(gallivm, border_color_ptr,
                                             lp_build_const_int32(gallivm, 0));
   border_color_ptr = LLVMBuildBitCast(builder, border_color_ptr,
                                       LLVMPointerType(vec4_bld.vec_type, 0), "");
   border_color = LLVMBuildLoad(builder, border_color_ptr, "");
   /* we don't have aligned type in the dynamic state unfortunately */
   lp_set_load_alignment(border_color, 4);

   /*
    * Instead of having some incredibly complex logic which will try to figure out
    * clamping necessary for each channel, simply use the first channel, and treat
    * mixed signed/unsigned normalized formats specially.
    * (Mixed non-normalized, which wouldn't work at all here, do not exist for a
    * good reason.)
    */
   if (format_desc->layout == UTIL_FORMAT_LAYOUT_PLAIN) {
      int chan;
      /* d/s needs special handling because both present means just sampling depth */
      if (util_format_is_depth_and_stencil(format_desc->format)) {
         chan = format_desc->swizzle[0];
      }
      else {
         chan = util_format_get_first_non_void_channel(format_desc->format);
      }
      if (chan >= 0 && chan <= UTIL_FORMAT_SWIZZLE_W) {
         unsigned chan_type = format_desc->channel[chan].type;
         unsigned chan_norm = format_desc->channel[chan].normalized;
         unsigned chan_pure = format_desc->channel[chan].pure_integer;
         if (chan_type == UTIL_FORMAT_TYPE_SIGNED) {
            if (chan_norm) {
               min_clamp = lp_build_const_vec(gallivm, vec4_type, -1.0F);
               max_clamp = vec4_bld.one;
            }
            else if (chan_pure) {
               /*
                * Border color was stored as int, hence need min/max clamp
                * only if chan has less than 32 bits..
                */
               unsigned chan_size = format_desc->channel[chan].size;
               if (chan_size < 32) {
                  min_clamp = lp_build_const_int_vec(gallivm, vec4_type,
                                                     0 - (1 << (chan_size - 1)));
                  max_clamp = lp_build_const_int_vec(gallivm, vec4_type,
                                                     (1 << (chan_size - 1)) - 1);
               }
            }
            /* TODO: no idea about non-pure, non-normalized! */
         }
         else if (chan_type == UTIL_FORMAT_TYPE_UNSIGNED) {
            if (chan_norm) {
               min_clamp = vec4_bld.zero;
               max_clamp = vec4_bld.one;
            }
            /*
             * Need a ugly hack here, because we don't have Z32_FLOAT_X8X24
             * we use Z32_FLOAT_S8X24 to imply sampling depth component
             * and ignoring stencil, which will blow up here if we try to
             * do a uint clamp in a float texel build...
             * And even if we had that format, mesa st also thinks using z24s8
             * means depth sampling ignoring stencil.
             */
            else if (chan_pure) {
               /*
                * Border color was stored as uint, hence never need min
                * clamp, and only need max clamp if chan has less than 32 bits.
                */
               unsigned chan_size = format_desc->channel[chan].size;
               if (chan_size < 32) {
                  max_clamp = lp_build_const_int_vec(gallivm, vec4_type,
                                                     (1 << chan_size) - 1);
               }
               /* TODO: no idea about non-pure, non-normalized! */
            }
         }
         else if (chan_type == UTIL_FORMAT_TYPE_FIXED) {
            /* TODO: I have no idea what clamp this would need if any! */
         }
      }
      /* mixed plain formats (or different pure size) */
      switch (format_desc->format) {
      case PIPE_FORMAT_B10G10R10A2_UINT:
      case PIPE_FORMAT_R10G10B10A2_UINT:
      {
         unsigned max10 = (1 << 10) - 1;
         max_clamp = lp_build_const_aos(gallivm, vec4_type, max10, max10,
                                        max10, (1 << 2) - 1, NULL);
      }
         break;
      case PIPE_FORMAT_R10SG10SB10SA2U_NORM:
         min_clamp = lp_build_const_aos(gallivm, vec4_type, -1.0F, -1.0F,
                                        -1.0F, 0.0F, NULL);
         max_clamp = vec4_bld.one;
         break;
      case PIPE_FORMAT_R8SG8SB8UX8U_NORM:
      case PIPE_FORMAT_R5SG5SB6U_NORM:
         min_clamp = lp_build_const_aos(gallivm, vec4_type, -1.0F, -1.0F,
                                        0.0F, 0.0F, NULL);
         max_clamp = vec4_bld.one;
         break;
      default:
         break;
      }
   }
   else {
      /* cannot figure this out from format description */
      if (format_desc->layout == UTIL_FORMAT_LAYOUT_S3TC) {
         /* s3tc formats are always unorm */
         min_clamp = vec4_bld.zero;
         max_clamp = vec4_bld.one;
      }
      else if (format_desc->layout == UTIL_FORMAT_LAYOUT_RGTC ||
               format_desc->layout == UTIL_FORMAT_LAYOUT_ETC) {
         switch (format_desc->format) {
         case PIPE_FORMAT_RGTC1_UNORM:
         case PIPE_FORMAT_RGTC2_UNORM:
         case PIPE_FORMAT_LATC1_UNORM:
         case PIPE_FORMAT_LATC2_UNORM:
         case PIPE_FORMAT_ETC1_RGB8:
            min_clamp = vec4_bld.zero;
            max_clamp = vec4_bld.one;
            break;
         case PIPE_FORMAT_RGTC1_SNORM:
         case PIPE_FORMAT_RGTC2_SNORM:
         case PIPE_FORMAT_LATC1_SNORM:
         case PIPE_FORMAT_LATC2_SNORM:
            min_clamp = lp_build_const_vec(gallivm, vec4_type, -1.0F);
            max_clamp = vec4_bld.one;
            break;
         default:
            assert(0);
            break;
         }
      }
      /*
       * all others from subsampled/other group, though we don't care
       * about yuv (and should not have any from zs here)
       */
      else if (format_desc->colorspace != UTIL_FORMAT_COLORSPACE_YUV){
         switch (format_desc->format) {
         case PIPE_FORMAT_R8G8_B8G8_UNORM:
         case PIPE_FORMAT_G8R8_G8B8_UNORM:
         case PIPE_FORMAT_G8R8_B8R8_UNORM:
         case PIPE_FORMAT_R8G8_R8B8_UNORM:
         case PIPE_FORMAT_R1_UNORM: /* doesn't make sense but ah well */
            min_clamp = vec4_bld.zero;
            max_clamp = vec4_bld.one;
            break;
         case PIPE_FORMAT_R8G8Bx_SNORM:
            min_clamp = lp_build_const_vec(gallivm, vec4_type, -1.0F);
            max_clamp = vec4_bld.one;
            break;
            /*
             * Note smallfloat formats usually don't need clamping
             * (they still have infinite range) however this is not
             * true for r11g11b10 and r9g9b9e5, which can't represent
             * negative numbers (and additionally r9g9b9e5 can't represent
             * very large numbers). d3d10 seems happy without clamping in
             * this case, but gl spec is pretty clear: "for floating
             * point and integer formats, border values are clamped to
             * the representable range of the format" so do that here.
             */
         case PIPE_FORMAT_R11G11B10_FLOAT:
            min_clamp = vec4_bld.zero;
            break;
         case PIPE_FORMAT_R9G9B9E5_FLOAT:
            min_clamp = vec4_bld.zero;
            max_clamp = lp_build_const_vec(gallivm, vec4_type, MAX_RGB9E5);
            break;
         default:
            assert(0);
            break;
         }
      }
   }

   if (min_clamp) {
      border_color = lp_build_max(&vec4_bld, border_color, min_clamp);
   }
   if (max_clamp) {
      border_color = lp_build_min(&vec4_bld, border_color, max_clamp);
   }

   bld->border_color_clamped = border_color;
}


/**
 * General texture sampling codegen.
 * This function handles texture sampling for all texture targets (1D,
 * 2D, 3D, cube) and all filtering modes.
 */
static void
lp_build_sample_general(struct lp_build_sample_context *bld,
                        unsigned sampler_unit,
                        LLVMValueRef *coords,
                        const LLVMValueRef *offsets,
                        LLVMValueRef lod_positive,
                        LLVMValueRef lod_fpart,
                        LLVMValueRef ilevel0,
                        LLVMValueRef ilevel1,
                        LLVMValueRef *colors_out)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   const struct lp_static_sampler_state *sampler_state = bld->static_sampler_state;
   const unsigned mip_filter = sampler_state->min_mip_filter;
   const unsigned min_filter = sampler_state->min_img_filter;
   const unsigned mag_filter = sampler_state->mag_img_filter;
   LLVMValueRef texels[4];
   unsigned chan;

   /* if we need border color, (potentially) clamp it now */
   if (lp_sampler_wrap_mode_uses_border_color(sampler_state->wrap_s,
                                              min_filter,
                                              mag_filter) ||
       (bld->dims > 1 &&
           lp_sampler_wrap_mode_uses_border_color(sampler_state->wrap_t,
                                                  min_filter,
                                                  mag_filter)) ||
       (bld->dims > 2 &&
           lp_sampler_wrap_mode_uses_border_color(sampler_state->wrap_r,
                                                  min_filter,
                                                  mag_filter))) {
      lp_build_clamp_border_color(bld, sampler_unit);
   }


   /*
    * Get/interpolate texture colors.
    */

   for (chan = 0; chan < 4; ++chan) {
     texels[chan] = lp_build_alloca(bld->gallivm, bld->texel_bld.vec_type, "");
     lp_build_name(texels[chan], "sampler%u_texel_%c_var", sampler_unit, "xyzw"[chan]);
   }

   if (min_filter == mag_filter) {
      /* no need to distinguish between minification and magnification */
      lp_build_sample_mipmap(bld, min_filter, mip_filter,
                             coords, offsets,
                             ilevel0, ilevel1, lod_fpart,
                             texels);
   }
   else {
      /*
       * Could also get rid of the if-logic and always use mipmap_both, both
       * for the single lod and multi-lod case if nothing really uses this.
       */
      if (bld->num_lods == 1) {
         /* Emit conditional to choose min image filter or mag image filter
          * depending on the lod being > 0 or <= 0, respectively.
          */
         struct lp_build_if_state if_ctx;

         lod_positive = LLVMBuildTrunc(builder, lod_positive,
                                       LLVMInt1TypeInContext(bld->gallivm->context), "");

         lp_build_if(&if_ctx, bld->gallivm, lod_positive);
         {
            /* Use the minification filter */
            lp_build_sample_mipmap(bld, min_filter, mip_filter,
                                   coords, offsets,
                                   ilevel0, ilevel1, lod_fpart,
                                   texels);
         }
         lp_build_else(&if_ctx);
         {
            /* Use the magnification filter */
            lp_build_sample_mipmap(bld, mag_filter, PIPE_TEX_MIPFILTER_NONE,
                                   coords, offsets,
                                   ilevel0, NULL, NULL,
                                   texels);
         }
         lp_build_endif(&if_ctx);
      }
      else {
         LLVMValueRef need_linear, linear_mask;
         unsigned mip_filter_for_nearest;
         struct lp_build_if_state if_ctx;

         if (min_filter == PIPE_TEX_FILTER_LINEAR) {
            linear_mask = lod_positive;
            mip_filter_for_nearest = PIPE_TEX_MIPFILTER_NONE;
         }
         else {
            linear_mask = lp_build_not(&bld->lodi_bld, lod_positive);
            mip_filter_for_nearest = mip_filter;
         }
         need_linear = lp_build_any_true_range(&bld->lodi_bld, bld->num_lods,
                                               linear_mask);

         if (bld->num_lods != bld->coord_type.length) {
            linear_mask = lp_build_unpack_broadcast_aos_scalars(bld->gallivm,
                                                                bld->lodi_type,
                                                                bld->int_coord_type,
                                                                linear_mask);
         }

         lp_build_if(&if_ctx, bld->gallivm, need_linear);
         {
            /*
             * Do sampling with both filters simultaneously. This means using
             * a linear filter and doing some tricks (with weights) for the pixels
             * which need nearest filter.
             * Note that it's probably rare some pixels need nearest and some
             * linear filter but the fixups required for the nearest pixels
             * aren't all that complicated so just always run a combined path
             * if at least some pixels require linear.
             */
            lp_build_sample_mipmap_both(bld, linear_mask, mip_filter,
                                        coords, offsets,
                                        ilevel0, ilevel1,
                                        lod_fpart, lod_positive,
                                        texels);
         }
         lp_build_else(&if_ctx);
         {
            /*
             * All pixels require just nearest filtering, which is way
             * cheaper than linear, hence do a separate path for that.
             */
            lp_build_sample_mipmap(bld, PIPE_TEX_FILTER_NEAREST,
                                   mip_filter_for_nearest,
                                   coords, offsets,
                                   ilevel0, ilevel1, lod_fpart,
                                   texels);
         }
         lp_build_endif(&if_ctx);
      }
   }

   for (chan = 0; chan < 4; ++chan) {
     colors_out[chan] = LLVMBuildLoad(builder, texels[chan], "");
     lp_build_name(colors_out[chan], "sampler%u_texel_%c", sampler_unit, "xyzw"[chan]);
   }
}


/**
 * Texel fetch function.
 * In contrast to general sampling there is no filtering, no coord minification,
 * lod (if any) is always explicit uint, coords are uints (in terms of texel units)
 * directly to be applied to the selected mip level (after adding texel offsets).
 * This function handles texel fetch for all targets where texel fetch is supported
 * (no cube maps, but 1d, 2d, 3d are supported, arrays and buffers should be too).
 */
static void
lp_build_fetch_texel(struct lp_build_sample_context *bld,
                     unsigned texture_unit,
                     const LLVMValueRef *coords,
                     LLVMValueRef explicit_lod,
                     const LLVMValueRef *offsets,
                     LLVMValueRef *colors_out)
{
   struct lp_build_context *perquadi_bld = &bld->lodi_bld;
   struct lp_build_context *int_coord_bld = &bld->int_coord_bld;
   unsigned dims = bld->dims, chan;
   unsigned target = bld->static_texture_state->target;
   boolean out_of_bound_ret_zero = TRUE;
   LLVMValueRef size, ilevel;
   LLVMValueRef row_stride_vec = NULL, img_stride_vec = NULL;
   LLVMValueRef x = coords[0], y = coords[1], z = coords[2];
   LLVMValueRef width, height, depth, i, j;
   LLVMValueRef offset, out_of_bounds, out1;

   out_of_bounds = int_coord_bld->zero;

   if (explicit_lod && bld->static_texture_state->target != PIPE_BUFFER) {
      if (bld->num_mips != int_coord_bld->type.length) {
         ilevel = lp_build_pack_aos_scalars(bld->gallivm, int_coord_bld->type,
                                            perquadi_bld->type, explicit_lod, 0);
      }
      else {
         ilevel = explicit_lod;
      }
      lp_build_nearest_mip_level(bld, texture_unit, ilevel, &ilevel,
                                 out_of_bound_ret_zero ? &out_of_bounds : NULL);
   }
   else {
      assert(bld->num_mips == 1);
      if (bld->static_texture_state->target != PIPE_BUFFER) {
         ilevel = bld->dynamic_state->first_level(bld->dynamic_state,
                                                  bld->gallivm, texture_unit);
      }
      else {
         ilevel = lp_build_const_int32(bld->gallivm, 0);
      }
   }
   lp_build_mipmap_level_sizes(bld, ilevel,
                               &size,
                               &row_stride_vec, &img_stride_vec);
   lp_build_extract_image_sizes(bld, &bld->int_size_bld, int_coord_bld->type,
                                size, &width, &height, &depth);

   if (target == PIPE_TEXTURE_1D_ARRAY ||
       target == PIPE_TEXTURE_2D_ARRAY) {
      if (out_of_bound_ret_zero) {
         z = lp_build_layer_coord(bld, texture_unit, z, &out1);
         out_of_bounds = lp_build_or(int_coord_bld, out_of_bounds, out1);
      }
      else {
         z = lp_build_layer_coord(bld, texture_unit, z, NULL);
      }
   }

   /* This is a lot like border sampling */
   if (offsets[0]) {
      /*
       * coords are really unsigned, offsets are signed, but I don't think
       * exceeding 31 bits is possible
       */
      x = lp_build_add(int_coord_bld, x, offsets[0]);
   }
   out1 = lp_build_cmp(int_coord_bld, PIPE_FUNC_LESS, x, int_coord_bld->zero);
   out_of_bounds = lp_build_or(int_coord_bld, out_of_bounds, out1);
   out1 = lp_build_cmp(int_coord_bld, PIPE_FUNC_GEQUAL, x, width);
   out_of_bounds = lp_build_or(int_coord_bld, out_of_bounds, out1);

   if (dims >= 2) {
      if (offsets[1]) {
         y = lp_build_add(int_coord_bld, y, offsets[1]);
      }
      out1 = lp_build_cmp(int_coord_bld, PIPE_FUNC_LESS, y, int_coord_bld->zero);
      out_of_bounds = lp_build_or(int_coord_bld, out_of_bounds, out1);
      out1 = lp_build_cmp(int_coord_bld, PIPE_FUNC_GEQUAL, y, height);
      out_of_bounds = lp_build_or(int_coord_bld, out_of_bounds, out1);

      if (dims >= 3) {
         if (offsets[2]) {
            z = lp_build_add(int_coord_bld, z, offsets[2]);
         }
         out1 = lp_build_cmp(int_coord_bld, PIPE_FUNC_LESS, z, int_coord_bld->zero);
         out_of_bounds = lp_build_or(int_coord_bld, out_of_bounds, out1);
         out1 = lp_build_cmp(int_coord_bld, PIPE_FUNC_GEQUAL, z, depth);
         out_of_bounds = lp_build_or(int_coord_bld, out_of_bounds, out1);
      }
   }

   lp_build_sample_offset(int_coord_bld,
                          bld->format_desc,
                          x, y, z, row_stride_vec, img_stride_vec,
                          &offset, &i, &j);

   if (bld->static_texture_state->target != PIPE_BUFFER) {
      offset = lp_build_add(int_coord_bld, offset,
                            lp_build_get_mip_offsets(bld, ilevel));
   }

   offset = lp_build_andnot(int_coord_bld, offset, out_of_bounds);

   lp_build_fetch_rgba_soa(bld->gallivm,
                           bld->format_desc,
                           bld->texel_type,
                           bld->base_ptr, offset,
                           i, j,
                           colors_out);

   if (out_of_bound_ret_zero) {
      /*
       * Only needed for ARB_robust_buffer_access_behavior and d3d10.
       * Could use min/max above instead of out-of-bounds comparisons
       * if we don't care about the result returned for out-of-bounds.
       */
      for (chan = 0; chan < 4; chan++) {
         colors_out[chan] = lp_build_select(&bld->texel_bld, out_of_bounds,
                                            bld->texel_bld.zero, colors_out[chan]);
      }
   }
}


/**
 * Just set texels to white instead of actually sampling the texture.
 * For debugging.
 */
void
lp_build_sample_nop(struct gallivm_state *gallivm,
                    struct lp_type type,
                    const LLVMValueRef *coords,
                    LLVMValueRef texel_out[4])
{
   LLVMValueRef one = lp_build_one(gallivm, type);
   unsigned chan;

   for (chan = 0; chan < 4; chan++) {
      texel_out[chan] = one;
   }  
}


/**
 * Build texture sampling code.
 * 'texel' will return a vector of four LLVMValueRefs corresponding to
 * R, G, B, A.
 * \param type  vector float type to use for coords, etc.
 * \param is_fetch  if this is a texel fetch instruction.
 * \param derivs  partial derivatives of (s,t,r,q) with respect to x and y
 */
void
lp_build_sample_soa(struct gallivm_state *gallivm,
                    const struct lp_static_texture_state *static_texture_state,
                    const struct lp_static_sampler_state *static_sampler_state,
                    struct lp_sampler_dynamic_state *dynamic_state,
                    struct lp_type type,
                    boolean is_fetch,
                    unsigned texture_index,
                    unsigned sampler_index,
                    const LLVMValueRef *coords,
                    const LLVMValueRef *offsets,
                    const struct lp_derivatives *derivs, /* optional */
                    LLVMValueRef lod_bias, /* optional */
                    LLVMValueRef explicit_lod, /* optional */
                    enum lp_sampler_lod_property lod_property,
                    LLVMValueRef texel_out[4])
{
   unsigned target = static_texture_state->target;
   unsigned dims = texture_dims(target);
   unsigned num_quads = type.length / 4;
   unsigned mip_filter, min_img_filter, mag_img_filter, i;
   struct lp_build_sample_context bld;
   struct lp_static_sampler_state derived_sampler_state = *static_sampler_state;
   LLVMTypeRef i32t = LLVMInt32TypeInContext(gallivm->context);
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef tex_width, newcoords[5];

   if (0) {
      enum pipe_format fmt = static_texture_state->format;
      debug_printf("Sample from %s\n", util_format_name(fmt));
   }

   if (static_texture_state->format == PIPE_FORMAT_NONE) {
      /*
       * If there's nothing bound, format is NONE, and we must return
       * all zero as mandated by d3d10 in this case.
       */
      unsigned chan;
      LLVMValueRef zero = lp_build_const_vec(gallivm, type, 0.0F);
      for (chan = 0; chan < 4; chan++) {
         texel_out[chan] = zero;
      }
      return;
   }

   assert(type.floating);

   /* Setup our build context */
   memset(&bld, 0, sizeof bld);
   bld.gallivm = gallivm;
   bld.static_sampler_state = &derived_sampler_state;
   bld.static_texture_state = static_texture_state;
   bld.dynamic_state = dynamic_state;
   bld.format_desc = util_format_description(static_texture_state->format);
   bld.dims = dims;

   bld.vector_width = lp_type_width(type);

   bld.float_type = lp_type_float(32);
   bld.int_type = lp_type_int(32);
   bld.coord_type = type;
   bld.int_coord_type = lp_int_type(type);
   bld.float_size_in_type = lp_type_float(32);
   bld.float_size_in_type.length = dims > 1 ? 4 : 1;
   bld.int_size_in_type = lp_int_type(bld.float_size_in_type);
   bld.texel_type = type;

   /* always using the first channel hopefully should be safe,
    * if not things WILL break in other places anyway.
    */
   if (bld.format_desc->colorspace == UTIL_FORMAT_COLORSPACE_RGB &&
       bld.format_desc->channel[0].pure_integer) {
      if (bld.format_desc->channel[0].type == UTIL_FORMAT_TYPE_SIGNED) {
         bld.texel_type = lp_type_int_vec(type.width, type.width * type.length);
      }
      else if (bld.format_desc->channel[0].type == UTIL_FORMAT_TYPE_UNSIGNED) {
         bld.texel_type = lp_type_uint_vec(type.width, type.width * type.length);
      }
   }
   else if (util_format_has_stencil(bld.format_desc) &&
       !util_format_has_depth(bld.format_desc)) {
      /* for stencil only formats, sample stencil (uint) */
      bld.texel_type = lp_type_int_vec(type.width, type.width * type.length);
   }

   if (!static_texture_state->level_zero_only) {
      derived_sampler_state.min_mip_filter = static_sampler_state->min_mip_filter;
   } else {
      derived_sampler_state.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   }
   mip_filter = derived_sampler_state.min_mip_filter;

   if (0) {
      debug_printf("  .min_mip_filter = %u\n", derived_sampler_state.min_mip_filter);
   }

   if (static_texture_state->target == PIPE_TEXTURE_CUBE ||
       static_texture_state->target == PIPE_TEXTURE_CUBE_ARRAY)
   {
      /*
       * Seamless filtering ignores wrap modes.
       * Setting to CLAMP_TO_EDGE is correct for nearest filtering, for
       * bilinear it's not correct but way better than using for instance repeat.
       * Note we even set this for non-seamless. Technically GL allows any wrap
       * mode, which made sense when supporting true borders (can get seamless
       * effect with border and CLAMP_TO_BORDER), but gallium doesn't support
       * borders and d3d9 requires wrap modes to be ignored and it's a pain to fix
       * up the sampler state (as it makes it texture dependent).
       */
      derived_sampler_state.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      derived_sampler_state.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   }

   min_img_filter = derived_sampler_state.min_img_filter;
   mag_img_filter = derived_sampler_state.mag_img_filter;


   /*
    * This is all a bit complicated different paths are chosen for performance
    * reasons.
    * Essentially, there can be 1 lod per element, 1 lod per quad or 1 lod for
    * everything (the last two options are equivalent for 4-wide case).
    * If there's per-quad lod but we split to 4-wide so we can use AoS, per-quad
    * lod is calculated then the lod value extracted afterwards so making this
    * case basically the same as far as lod handling is concerned for the
    * further sample/filter code as the 1 lod for everything case.
    * Different lod handling mostly shows up when building mipmap sizes
    * (lp_build_mipmap_level_sizes() and friends) and also in filtering
    * (getting the fractional part of the lod to the right texels).
    */

   /*
    * There are other situations where at least the multiple int lods could be
    * avoided like min and max lod being equal.
    */
   bld.num_mips = bld.num_lods = 1;

   if ((gallivm_debug & GALLIVM_DEBUG_NO_QUAD_LOD) &&
       (gallivm_debug & GALLIVM_DEBUG_NO_RHO_APPROX) &&
       (static_texture_state->target == PIPE_TEXTURE_CUBE) &&
       (!is_fetch && mip_filter != PIPE_TEX_MIPFILTER_NONE)) {
      /*
       * special case for using per-pixel lod even for implicit lod,
       * which is generally never required (ok by APIs) except to please
       * some (somewhat broken imho) tests (because per-pixel face selection
       * can cause derivatives to be different for pixels outside the primitive
       * due to the major axis division even if pre-project derivatives are
       * looking normal).
       */
      bld.num_mips = type.length;
      bld.num_lods = type.length;
   }
   else if (lod_property == LP_SAMPLER_LOD_PER_ELEMENT ||
       (explicit_lod || lod_bias || derivs)) {
      if ((is_fetch && target != PIPE_BUFFER) ||
          (!is_fetch && mip_filter != PIPE_TEX_MIPFILTER_NONE)) {
         bld.num_mips = type.length;
         bld.num_lods = type.length;
      }
      else if (!is_fetch && min_img_filter != mag_img_filter) {
         bld.num_mips = 1;
         bld.num_lods = type.length;
      }
   }
   /* TODO: for true scalar_lod should only use 1 lod value */
   else if ((is_fetch && explicit_lod && target != PIPE_BUFFER) ||
            (!is_fetch && mip_filter != PIPE_TEX_MIPFILTER_NONE)) {
      bld.num_mips = num_quads;
      bld.num_lods = num_quads;
   }
   else if (!is_fetch && min_img_filter != mag_img_filter) {
      bld.num_mips = 1;
      bld.num_lods = num_quads;
   }


   bld.lodf_type = type;
   /* we want native vector size to be able to use our intrinsics */
   if (bld.num_lods != type.length) {
      /* TODO: this currently always has to be per-quad or per-element */
      bld.lodf_type.length = type.length > 4 ? ((type.length + 15) / 16) * 4 : 1;
   }
   bld.lodi_type = lp_int_type(bld.lodf_type);
   bld.levelf_type = bld.lodf_type;
   if (bld.num_mips == 1) {
      bld.levelf_type.length = 1;
   }
   bld.leveli_type = lp_int_type(bld.levelf_type);
   bld.float_size_type = bld.float_size_in_type;
   /* Note: size vectors may not be native. They contain minified w/h/d/_ values,
    * with per-element lod that is w0/h0/d0/_/w1/h1/d1_/... so up to 8x4f32 */
   if (bld.num_mips > 1) {
      bld.float_size_type.length = bld.num_mips == type.length ?
                                      bld.num_mips * bld.float_size_in_type.length :
                                      type.length;
   }
   bld.int_size_type = lp_int_type(bld.float_size_type);

   lp_build_context_init(&bld.float_bld, gallivm, bld.float_type);
   lp_build_context_init(&bld.float_vec_bld, gallivm, type);
   lp_build_context_init(&bld.int_bld, gallivm, bld.int_type);
   lp_build_context_init(&bld.coord_bld, gallivm, bld.coord_type);
   lp_build_context_init(&bld.int_coord_bld, gallivm, bld.int_coord_type);
   lp_build_context_init(&bld.int_size_in_bld, gallivm, bld.int_size_in_type);
   lp_build_context_init(&bld.float_size_in_bld, gallivm, bld.float_size_in_type);
   lp_build_context_init(&bld.int_size_bld, gallivm, bld.int_size_type);
   lp_build_context_init(&bld.float_size_bld, gallivm, bld.float_size_type);
   lp_build_context_init(&bld.texel_bld, gallivm, bld.texel_type);
   lp_build_context_init(&bld.levelf_bld, gallivm, bld.levelf_type);
   lp_build_context_init(&bld.leveli_bld, gallivm, bld.leveli_type);
   lp_build_context_init(&bld.lodf_bld, gallivm, bld.lodf_type);
   lp_build_context_init(&bld.lodi_bld, gallivm, bld.lodi_type);

   /* Get the dynamic state */
   tex_width = dynamic_state->width(dynamic_state, gallivm, texture_index);
   bld.row_stride_array = dynamic_state->row_stride(dynamic_state, gallivm, texture_index);
   bld.img_stride_array = dynamic_state->img_stride(dynamic_state, gallivm, texture_index);
   bld.base_ptr = dynamic_state->base_ptr(dynamic_state, gallivm, texture_index);
   bld.mip_offsets = dynamic_state->mip_offsets(dynamic_state, gallivm, texture_index);
   /* Note that mip_offsets is an array[level] of offsets to texture images */

   /* width, height, depth as single int vector */
   if (dims <= 1) {
      bld.int_size = tex_width;
   }
   else {
      bld.int_size = LLVMBuildInsertElement(builder, bld.int_size_in_bld.undef,
                                            tex_width, LLVMConstInt(i32t, 0, 0), "");
      if (dims >= 2) {
         LLVMValueRef tex_height =
            dynamic_state->height(dynamic_state, gallivm, texture_index);
         bld.int_size = LLVMBuildInsertElement(builder, bld.int_size,
                                               tex_height, LLVMConstInt(i32t, 1, 0), "");
         if (dims >= 3) {
            LLVMValueRef tex_depth =
               dynamic_state->depth(dynamic_state, gallivm, texture_index);
            bld.int_size = LLVMBuildInsertElement(builder, bld.int_size,
                                                  tex_depth, LLVMConstInt(i32t, 2, 0), "");
         }
      }
   }

   for (i = 0; i < 5; i++) {
      newcoords[i] = coords[i];
   }

   if (0) {
      /* For debug: no-op texture sampling */
      lp_build_sample_nop(gallivm,
                          bld.texel_type,
                          newcoords,
                          texel_out);
   }

   else if (is_fetch) {
      lp_build_fetch_texel(&bld, texture_index, newcoords,
                           explicit_lod, offsets,
                           texel_out);
   }

   else {
      LLVMValueRef lod_fpart = NULL, lod_positive = NULL;
      LLVMValueRef ilevel0 = NULL, ilevel1 = NULL;
      boolean use_aos = util_format_fits_8unorm(bld.format_desc) &&
                        /* not sure this is strictly needed or simply impossible */
                        derived_sampler_state.compare_mode == PIPE_TEX_COMPARE_NONE &&
                        lp_is_simple_wrap_mode(derived_sampler_state.wrap_s);

      use_aos &= bld.num_lods <= num_quads ||
                 derived_sampler_state.min_img_filter ==
                    derived_sampler_state.mag_img_filter;
      if (dims > 1) {
         use_aos &= lp_is_simple_wrap_mode(derived_sampler_state.wrap_t);
         if (dims > 2) {
            use_aos &= lp_is_simple_wrap_mode(derived_sampler_state.wrap_r);
         }
      }
      if (static_texture_state->target == PIPE_TEXTURE_CUBE &&
          derived_sampler_state.seamless_cube_map &&
          (derived_sampler_state.min_img_filter == PIPE_TEX_FILTER_LINEAR ||
           derived_sampler_state.mag_img_filter == PIPE_TEX_FILTER_LINEAR)) {
         /* theoretically possible with AoS filtering but not implemented (complex!) */
         use_aos = 0;
      }

      if ((gallivm_debug & GALLIVM_DEBUG_PERF) &&
          !use_aos && util_format_fits_8unorm(bld.format_desc)) {
         debug_printf("%s: using floating point linear filtering for %s\n",
                      __FUNCTION__, bld.format_desc->short_name);
         debug_printf("  min_img %d  mag_img %d  mip %d  target %d  seamless %d"
                      "  wraps %d  wrapt %d  wrapr %d\n",
                      derived_sampler_state.min_img_filter,
                      derived_sampler_state.mag_img_filter,
                      derived_sampler_state.min_mip_filter,
                      static_texture_state->target,
                      derived_sampler_state.seamless_cube_map,
                      derived_sampler_state.wrap_s,
                      derived_sampler_state.wrap_t,
                      derived_sampler_state.wrap_r);
      }

      lp_build_sample_common(&bld, texture_index, sampler_index,
                             newcoords,
                             derivs, lod_bias, explicit_lod,
                             &lod_positive, &lod_fpart,
                             &ilevel0, &ilevel1);

      /*
       * we only try 8-wide sampling with soa as it appears to
       * be a loss with aos with AVX (but it should work, except
       * for conformance if min_filter != mag_filter if num_lods > 1).
       * (It should be faster if we'd support avx2)
       */
      if (num_quads == 1 || !use_aos) {
         if (use_aos) {
            /* do sampling/filtering with fixed pt arithmetic */
            lp_build_sample_aos(&bld, sampler_index,
                                newcoords[0], newcoords[1],
                                newcoords[2],
                                offsets, lod_positive, lod_fpart,
                                ilevel0, ilevel1,
                                texel_out);
         }

         else {
            lp_build_sample_general(&bld, sampler_index,
                                    newcoords, offsets,
                                    lod_positive, lod_fpart,
                                    ilevel0, ilevel1,
                                    texel_out);
         }
      }
      else {
         unsigned j;
         struct lp_build_sample_context bld4;
         struct lp_type type4 = type;
         unsigned i;
         LLVMValueRef texelout4[4];
         LLVMValueRef texelouttmp[4][LP_MAX_VECTOR_LENGTH/16];

         type4.length = 4;

         /* Setup our build context */
         memset(&bld4, 0, sizeof bld4);
         bld4.gallivm = bld.gallivm;
         bld4.static_texture_state = bld.static_texture_state;
         bld4.static_sampler_state = bld.static_sampler_state;
         bld4.dynamic_state = bld.dynamic_state;
         bld4.format_desc = bld.format_desc;
         bld4.dims = bld.dims;
         bld4.row_stride_array = bld.row_stride_array;
         bld4.img_stride_array = bld.img_stride_array;
         bld4.base_ptr = bld.base_ptr;
         bld4.mip_offsets = bld.mip_offsets;
         bld4.int_size = bld.int_size;

         bld4.vector_width = lp_type_width(type4);

         bld4.float_type = lp_type_float(32);
         bld4.int_type = lp_type_int(32);
         bld4.coord_type = type4;
         bld4.int_coord_type = lp_int_type(type4);
         bld4.float_size_in_type = lp_type_float(32);
         bld4.float_size_in_type.length = dims > 1 ? 4 : 1;
         bld4.int_size_in_type = lp_int_type(bld4.float_size_in_type);
         bld4.texel_type = bld.texel_type;
         bld4.texel_type.length = 4;

         bld4.num_mips = bld4.num_lods = 1;
         if ((gallivm_debug & GALLIVM_DEBUG_NO_QUAD_LOD) &&
             (gallivm_debug & GALLIVM_DEBUG_NO_RHO_APPROX) &&
             (static_texture_state->target == PIPE_TEXTURE_CUBE) &&
             (!is_fetch && mip_filter != PIPE_TEX_MIPFILTER_NONE)) {
            bld4.num_mips = type4.length;
            bld4.num_lods = type4.length;
         }
         if (lod_property == LP_SAMPLER_LOD_PER_ELEMENT &&
             (explicit_lod || lod_bias || derivs)) {
            if ((is_fetch && target != PIPE_BUFFER) ||
                (!is_fetch && mip_filter != PIPE_TEX_MIPFILTER_NONE)) {
               bld4.num_mips = type4.length;
               bld4.num_lods = type4.length;
            }
            else if (!is_fetch && min_img_filter != mag_img_filter) {
               bld4.num_mips = 1;
               bld4.num_lods = type4.length;
            }
         }

         /* we want native vector size to be able to use our intrinsics */
         bld4.lodf_type = type4;
         if (bld4.num_lods != type4.length) {
            bld4.lodf_type.length = 1;
         }
         bld4.lodi_type = lp_int_type(bld4.lodf_type);
         bld4.levelf_type = type4;
         if (bld4.num_mips != type4.length) {
            bld4.levelf_type.length = 1;
         }
         bld4.leveli_type = lp_int_type(bld4.levelf_type);
         bld4.float_size_type = bld4.float_size_in_type;
         if (bld4.num_mips > 1) {
            bld4.float_size_type.length = bld4.num_mips == type4.length ?
                                            bld4.num_mips * bld4.float_size_in_type.length :
                                            type4.length;
         }
         bld4.int_size_type = lp_int_type(bld4.float_size_type);

         lp_build_context_init(&bld4.float_bld, gallivm, bld4.float_type);
         lp_build_context_init(&bld4.float_vec_bld, gallivm, type4);
         lp_build_context_init(&bld4.int_bld, gallivm, bld4.int_type);
         lp_build_context_init(&bld4.coord_bld, gallivm, bld4.coord_type);
         lp_build_context_init(&bld4.int_coord_bld, gallivm, bld4.int_coord_type);
         lp_build_context_init(&bld4.int_size_in_bld, gallivm, bld4.int_size_in_type);
         lp_build_context_init(&bld4.float_size_in_bld, gallivm, bld4.float_size_in_type);
         lp_build_context_init(&bld4.int_size_bld, gallivm, bld4.int_size_type);
         lp_build_context_init(&bld4.float_size_bld, gallivm, bld4.float_size_type);
         lp_build_context_init(&bld4.texel_bld, gallivm, bld4.texel_type);
         lp_build_context_init(&bld4.levelf_bld, gallivm, bld4.levelf_type);
         lp_build_context_init(&bld4.leveli_bld, gallivm, bld4.leveli_type);
         lp_build_context_init(&bld4.lodf_bld, gallivm, bld4.lodf_type);
         lp_build_context_init(&bld4.lodi_bld, gallivm, bld4.lodi_type);

         for (i = 0; i < num_quads; i++) {
            LLVMValueRef s4, t4, r4;
            LLVMValueRef lod_positive4, lod_fpart4 = NULL;
            LLVMValueRef ilevel04, ilevel14 = NULL;
            LLVMValueRef offsets4[4] = { NULL };
            unsigned num_lods = bld4.num_lods;

            s4 = lp_build_extract_range(gallivm, newcoords[0], 4*i, 4);
            t4 = lp_build_extract_range(gallivm, newcoords[1], 4*i, 4);
            r4 = lp_build_extract_range(gallivm, newcoords[2], 4*i, 4);

            if (offsets[0]) {
               offsets4[0] = lp_build_extract_range(gallivm, offsets[0], 4*i, 4);
               if (dims > 1) {
                  offsets4[1] = lp_build_extract_range(gallivm, offsets[1], 4*i, 4);
                  if (dims > 2) {
                     offsets4[2] = lp_build_extract_range(gallivm, offsets[2], 4*i, 4);
                  }
               }
            }
            lod_positive4 = lp_build_extract_range(gallivm, lod_positive, num_lods * i, num_lods);
            ilevel04 = bld.num_mips == 1 ? ilevel0 :
                          lp_build_extract_range(gallivm, ilevel0, num_lods * i, num_lods);
            if (mip_filter == PIPE_TEX_MIPFILTER_LINEAR) {
               ilevel14 = lp_build_extract_range(gallivm, ilevel1, num_lods * i, num_lods);
               lod_fpart4 = lp_build_extract_range(gallivm, lod_fpart, num_lods * i, num_lods);
            }

            if (use_aos) {
               /* do sampling/filtering with fixed pt arithmetic */
               lp_build_sample_aos(&bld4, sampler_index,
                                   s4, t4, r4, offsets4,
                                   lod_positive4, lod_fpart4,
                                   ilevel04, ilevel14,
                                   texelout4);
            }

            else {
               /* this path is currently unreachable and hence might break easily... */
               LLVMValueRef newcoords4[5];
               newcoords4[0] = s4;
               newcoords4[1] = t4;
               newcoords4[2] = r4;
               newcoords4[3] = lp_build_extract_range(gallivm, newcoords[3], 4*i, 4);
               newcoords4[4] = lp_build_extract_range(gallivm, newcoords[4], 4*i, 4);

               lp_build_sample_general(&bld4, sampler_index,
                                       newcoords4, offsets4,
                                       lod_positive4, lod_fpart4,
                                       ilevel04, ilevel14,
                                       texelout4);
            }
            for (j = 0; j < 4; j++) {
               texelouttmp[j][i] = texelout4[j];
            }
         }

         for (j = 0; j < 4; j++) {
            texel_out[j] = lp_build_concat(gallivm, texelouttmp[j], type4, num_quads);
         }
      }
   }

   if (target != PIPE_BUFFER) {
      apply_sampler_swizzle(&bld, texel_out);
   }

   /*
    * texel type can be a (32bit) int/uint (for pure int formats only),
    * however we are expected to always return floats (storage is untyped).
    */
   if (!bld.texel_type.floating) {
      unsigned chan;
      for (chan = 0; chan < 4; chan++) {
         texel_out[chan] = LLVMBuildBitCast(builder, texel_out[chan],
                                            lp_build_vec_type(gallivm, type), "");
      }
   }
}

void
lp_build_size_query_soa(struct gallivm_state *gallivm,
                        const struct lp_static_texture_state *static_state,
                        struct lp_sampler_dynamic_state *dynamic_state,
                        struct lp_type int_type,
                        unsigned texture_unit,
                        unsigned target,
                        boolean is_sviewinfo,
                        enum lp_sampler_lod_property lod_property,
                        LLVMValueRef explicit_lod,
                        LLVMValueRef *sizes_out)
{
   LLVMValueRef lod, level, size;
   LLVMValueRef first_level = NULL;
   int dims, i;
   boolean has_array;
   unsigned num_lods = 1;
   struct lp_build_context bld_int_vec4;

   if (static_state->format == PIPE_FORMAT_NONE) {
      /*
       * If there's nothing bound, format is NONE, and we must return
       * all zero as mandated by d3d10 in this case.
       */
      unsigned chan;
      LLVMValueRef zero = lp_build_const_vec(gallivm, int_type, 0.0F);
      for (chan = 0; chan < 4; chan++) {
         sizes_out[chan] = zero;
      }
      return;
   }

   /*
    * Do some sanity verification about bound texture and shader dcl target.
    * Not entirely sure what's possible but assume array/non-array
    * always compatible (probably not ok for OpenGL but d3d10 has no
    * distinction of arrays at the resource level).
    * Everything else looks bogus (though not entirely sure about rect/2d).
    * Currently disabled because it causes assertion failures if there's
    * nothing bound (or rather a dummy texture, not that this case would
    * return the right values).
    */
   if (0 && static_state->target != target) {
      if (static_state->target == PIPE_TEXTURE_1D)
         assert(target == PIPE_TEXTURE_1D_ARRAY);
      else if (static_state->target == PIPE_TEXTURE_1D_ARRAY)
         assert(target == PIPE_TEXTURE_1D);
      else if (static_state->target == PIPE_TEXTURE_2D)
         assert(target == PIPE_TEXTURE_2D_ARRAY);
      else if (static_state->target == PIPE_TEXTURE_2D_ARRAY)
         assert(target == PIPE_TEXTURE_2D);
      else if (static_state->target == PIPE_TEXTURE_CUBE)
         assert(target == PIPE_TEXTURE_CUBE_ARRAY);
      else if (static_state->target == PIPE_TEXTURE_CUBE_ARRAY)
         assert(target == PIPE_TEXTURE_CUBE);
      else
         assert(0);
   }

   dims = texture_dims(target);

   switch (target) {
   case PIPE_TEXTURE_1D_ARRAY:
   case PIPE_TEXTURE_2D_ARRAY:
      has_array = TRUE;
      break;
   default:
      has_array = FALSE;
      break;
   }

   assert(!int_type.floating);

   lp_build_context_init(&bld_int_vec4, gallivm, lp_type_int_vec(32, 128));

   if (explicit_lod) {
      /* FIXME: this needs to honor per-element lod */
      lod = LLVMBuildExtractElement(gallivm->builder, explicit_lod, lp_build_const_int32(gallivm, 0), "");
      first_level = dynamic_state->first_level(dynamic_state, gallivm, texture_unit);
      level = LLVMBuildAdd(gallivm->builder, lod, first_level, "level");
      lod = lp_build_broadcast_scalar(&bld_int_vec4, level);
   } else {
      lod = bld_int_vec4.zero;
   }

   size = bld_int_vec4.undef;

   size = LLVMBuildInsertElement(gallivm->builder, size,
                                 dynamic_state->width(dynamic_state, gallivm, texture_unit),
                                 lp_build_const_int32(gallivm, 0), "");

   if (dims >= 2) {
      size = LLVMBuildInsertElement(gallivm->builder, size,
                                    dynamic_state->height(dynamic_state, gallivm, texture_unit),
                                    lp_build_const_int32(gallivm, 1), "");
   }

   if (dims >= 3) {
      size = LLVMBuildInsertElement(gallivm->builder, size,
                                    dynamic_state->depth(dynamic_state, gallivm, texture_unit),
                                    lp_build_const_int32(gallivm, 2), "");
   }

   size = lp_build_minify(&bld_int_vec4, size, lod, TRUE);

   if (has_array)
      size = LLVMBuildInsertElement(gallivm->builder, size,
                                    dynamic_state->depth(dynamic_state, gallivm, texture_unit),
                                    lp_build_const_int32(gallivm, dims), "");

   /*
    * d3d10 requires zero for x/y/z values (but not w, i.e. mip levels)
    * if level is out of bounds (note this can't cover unbound texture
    * here, which also requires returning zero).
    */
   if (explicit_lod && is_sviewinfo) {
      LLVMValueRef last_level, out, out1;
      struct lp_build_context leveli_bld;

      /* everything is scalar for now */
      lp_build_context_init(&leveli_bld, gallivm, lp_type_int_vec(32, 32));
      last_level = dynamic_state->last_level(dynamic_state, gallivm, texture_unit);

      out = lp_build_cmp(&leveli_bld, PIPE_FUNC_LESS, level, first_level);
      out1 = lp_build_cmp(&leveli_bld, PIPE_FUNC_GREATER, level, last_level);
      out = lp_build_or(&leveli_bld, out, out1);
      if (num_lods == 1) {
         out = lp_build_broadcast_scalar(&bld_int_vec4, out);
      }
      else {
         /* TODO */
         assert(0);
      }
      size = lp_build_andnot(&bld_int_vec4, size, out);
   }
   for (i = 0; i < dims + (has_array ? 1 : 0); i++) {
      sizes_out[i] = lp_build_extract_broadcast(gallivm, bld_int_vec4.type, int_type,
                                                size,
                                                lp_build_const_int32(gallivm, i));
   }
   if (is_sviewinfo) {
      for (; i < 4; i++) {
         sizes_out[i] = lp_build_const_vec(gallivm, int_type, 0.0);
      }
   }

   /*
    * if there's no explicit_lod (buffers, rects) queries requiring nr of
    * mips would be illegal.
    */
   if (is_sviewinfo && explicit_lod) {
      struct lp_build_context bld_int_scalar;
      LLVMValueRef num_levels;
      lp_build_context_init(&bld_int_scalar, gallivm, lp_type_int(32));

      if (static_state->level_zero_only) {
         num_levels = bld_int_scalar.one;
      }
      else {
         LLVMValueRef last_level;

         last_level = dynamic_state->last_level(dynamic_state, gallivm, texture_unit);
         num_levels = lp_build_sub(&bld_int_scalar, last_level, first_level);
         num_levels = lp_build_add(&bld_int_scalar, num_levels, bld_int_scalar.one);
      }
      sizes_out[3] = lp_build_broadcast(gallivm, lp_build_vec_type(gallivm, int_type),
                                        num_levels);
   }
}
