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
#include "util/u_dump.h"
#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_format.h"
#include "util/u_cpu_detect.h"
#include "lp_bld_debug.h"
#include "lp_bld_type.h"
#include "lp_bld_const.h"
#include "lp_bld_conv.h"
#include "lp_bld_arit.h"
#include "lp_bld_logic.h"
#include "lp_bld_swizzle.h"
#include "lp_bld_pack.h"
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

   /** Unsigned integer coordinates */
   struct lp_type uint_coord_type;
   struct lp_build_context uint_coord_bld;

   /** Signed integer coordinates */
   struct lp_type int_coord_type;
   struct lp_build_context int_coord_bld;

   /** Output texels type and build context */
   struct lp_type texel_type;
   struct lp_build_context texel_bld;
};


/**
 * Does the given texture wrap mode allow sampling the texture border color?
 * XXX maybe move this into gallium util code.
 */
static boolean
wrap_mode_uses_border_color(unsigned mode)
{
   switch (mode) {
   case PIPE_TEX_WRAP_REPEAT:
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
   case PIPE_TEX_WRAP_MIRROR_REPEAT:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
      return FALSE;
   case PIPE_TEX_WRAP_CLAMP:
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
   case PIPE_TEX_WRAP_MIRROR_CLAMP:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
      return TRUE;
   default:
      assert(0 && "unexpected wrap mode");
      return FALSE;
   }
}



/**
 * Gen code to fetch a texel from a texture at int coords (x, y).
 * The result, texel, will be:
 *   texel[0] = red values
 *   texel[1] = green values
 *   texel[2] = blue values
 *   texel[3] = alpha values
 */
static void
lp_build_sample_texel_soa(struct lp_build_sample_context *bld,
                          LLVMValueRef width,
                          LLVMValueRef height,
                          LLVMValueRef x,
                          LLVMValueRef y,
                          LLVMValueRef y_stride,
                          LLVMValueRef data_ptr,
                          LLVMValueRef *texel)
{
   struct lp_build_context *int_coord_bld = &bld->int_coord_bld;
   LLVMValueRef offset;
   LLVMValueRef packed;
   LLVMValueRef use_border = NULL;

   /* use_border = x < 0 || x >= width || y < 0 || y >= height */
   if (wrap_mode_uses_border_color(bld->static_state->wrap_s)) {
      LLVMValueRef b1, b2;
      b1 = lp_build_cmp(int_coord_bld, PIPE_FUNC_LESS, x, int_coord_bld->zero);
      b2 = lp_build_cmp(int_coord_bld, PIPE_FUNC_GEQUAL, x, width);
      use_border = LLVMBuildOr(bld->builder, b1, b2, "b1_or_b2");
   }

   if (wrap_mode_uses_border_color(bld->static_state->wrap_t)) {
      LLVMValueRef b1, b2;
      b1 = lp_build_cmp(int_coord_bld, PIPE_FUNC_LESS, y, int_coord_bld->zero);
      b2 = lp_build_cmp(int_coord_bld, PIPE_FUNC_GEQUAL, y, height);
      if (use_border) {
         use_border = LLVMBuildOr(bld->builder, use_border, b1, "ub_or_b1");
         use_border = LLVMBuildOr(bld->builder, use_border, b2, "ub_or_b2");
      }
      else {
         use_border = LLVMBuildOr(bld->builder, b1, b2, "b1_or_b2");
      }
   }

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

   /* convert x,y coords to linear offset from start of texture, in bytes */
   offset = lp_build_sample_offset(&bld->uint_coord_bld,
                                   bld->format_desc,
                                   x, y, y_stride,
                                   data_ptr);

   assert(bld->format_desc->block.width == 1);
   assert(bld->format_desc->block.height == 1);
   assert(bld->format_desc->block.bits <= bld->texel_type.width);

   /* gather the texels from the texture */
   packed = lp_build_gather(bld->builder,
                            bld->texel_type.length,
                            bld->format_desc->block.bits,
                            bld->texel_type.width,
                            data_ptr, offset);

   /* convert texels to float rgba */
   lp_build_unpack_rgba_soa(bld->builder,
                            bld->format_desc,
                            bld->texel_type,
                            packed, texel);

   if (use_border) {
      /* select texel color or border color depending on use_border */
      int chan;
      for (chan = 0; chan < 4; chan++) {
         LLVMValueRef border_chan =
            lp_build_const_scalar(bld->texel_type,
                                  bld->static_state->border_color[chan]);
         texel[chan] = lp_build_select(&bld->texel_bld, use_border,
                                       border_chan, texel[chan]);
      }
   }
}


static LLVMValueRef
lp_build_sample_packed(struct lp_build_sample_context *bld,
                       LLVMValueRef x,
                       LLVMValueRef y,
                       LLVMValueRef y_stride,
                       LLVMValueRef data_ptr)
{
   LLVMValueRef offset;

   offset = lp_build_sample_offset(&bld->uint_coord_bld,
                                   bld->format_desc,
                                   x, y, y_stride,
                                   data_ptr);

   assert(bld->format_desc->block.width == 1);
   assert(bld->format_desc->block.height == 1);
   assert(bld->format_desc->block.bits <= bld->texel_type.width);

   return lp_build_gather(bld->builder,
                          bld->texel_type.length,
                          bld->format_desc->block.bits,
                          bld->texel_type.width,
                          data_ptr, offset);
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

   /* fract = coord - floor(coord) */
   fract = lp_build_sub(coord_bld, coord, lp_build_floor(coord_bld, coord));

   /* flr = ifloor(coord); */
   flr = lp_build_ifloor(coord_bld, coord);

   /* isOdd = flr & 1 */
   isOdd = LLVMBuildAnd(bld->builder, flr, int_coord_bld->one, "");

   /* make coord positive or negative depending on isOdd */
   coord = lp_build_set_sign(coord_bld, fract, isOdd);

   /* convert isOdd to float */
   isOdd = lp_build_int_to_float(coord_bld, isOdd);

   /* add isOdd to coord */
   coord = lp_build_add(coord_bld, coord, isOdd);

   return coord;
}


/**
 * We only support a few wrap modes in lp_build_sample_wrap_int() at this time.
 * Return whether the given mode is supported by that function.
 */
static boolean
is_simple_wrap_mode(unsigned mode)
{
   switch (mode) {
   case PIPE_TEX_WRAP_REPEAT:
   case PIPE_TEX_WRAP_CLAMP:
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      return TRUE;
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
   default:
      return FALSE;
   }
}


/**
 * Build LLVM code for texture wrap mode, for scaled integer texcoords.
 * \param coord  the incoming texcoord (s,t,r or q) scaled to the texture size
 * \param length  the texture size along one dimension
 * \param is_pot  if TRUE, length is a power of two
 * \param wrap_mode  one of PIPE_TEX_WRAP_x
 */
static LLVMValueRef
lp_build_sample_wrap_int(struct lp_build_sample_context *bld,
                         LLVMValueRef coord,
                         LLVMValueRef length,
                         boolean is_pot,
                         unsigned wrap_mode)
{
   struct lp_build_context *uint_coord_bld = &bld->uint_coord_bld;
   struct lp_build_context *int_coord_bld = &bld->int_coord_bld;
   LLVMValueRef length_minus_one;

   length_minus_one = lp_build_sub(uint_coord_bld, length, uint_coord_bld->one);

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
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      coord = lp_build_max(int_coord_bld, coord, int_coord_bld->zero);
      coord = lp_build_min(int_coord_bld, coord, length_minus_one);
      break;

   case PIPE_TEX_WRAP_MIRROR_REPEAT:
   case PIPE_TEX_WRAP_MIRROR_CLAMP:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
      /* FIXME */
      _debug_printf("llvmpipe: failed to translate texture wrap mode %s\n",
                    util_dump_tex_wrap(wrap_mode, TRUE));
      coord = lp_build_max(uint_coord_bld, coord, uint_coord_bld->zero);
      coord = lp_build_min(uint_coord_bld, coord, length_minus_one);
      break;

   default:
      assert(0);
   }

   return coord;
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
                            boolean is_pot,
                            unsigned wrap_mode,
                            LLVMValueRef *x0_out,
                            LLVMValueRef *x1_out,
                            LLVMValueRef *weight_out)
{
   struct lp_build_context *coord_bld = &bld->coord_bld;
   struct lp_build_context *int_coord_bld = &bld->int_coord_bld;
   struct lp_build_context *uint_coord_bld = &bld->uint_coord_bld;
   LLVMValueRef two = lp_build_const_scalar(coord_bld->type, 2.0);
   LLVMValueRef half = lp_build_const_scalar(coord_bld->type, 0.5);
   LLVMValueRef length_f = lp_build_int_to_float(coord_bld, length);
   LLVMValueRef length_minus_one = lp_build_sub(uint_coord_bld, length, uint_coord_bld->one);
   LLVMValueRef length_f_minus_one = lp_build_sub(coord_bld, length_f, coord_bld->one);
   LLVMValueRef coord0, coord1, weight;

   switch(wrap_mode) {
   case PIPE_TEX_WRAP_REPEAT:
      /* mul by size and subtract 0.5 */
      coord = lp_build_mul(coord_bld, coord, length_f);
      coord = lp_build_sub(coord_bld, coord, half);
      /* convert to int */
      coord0 = lp_build_ifloor(coord_bld, coord);
      coord1 = lp_build_add(uint_coord_bld, coord0, uint_coord_bld->one);
      /* compute lerp weight */
      weight = lp_build_fract(coord_bld, coord);
      /* repeat wrap */
      if (is_pot) {
         coord0 = LLVMBuildAnd(bld->builder, coord0, length_minus_one, "");
         coord1 = LLVMBuildAnd(bld->builder, coord1, length_minus_one, "");
      }
      else {
         /* Signed remainder won't give the right results for negative
          * dividends but unsigned remainder does.*/
         coord0 = LLVMBuildURem(bld->builder, coord0, length, "");
         coord1 = LLVMBuildURem(bld->builder, coord1, length, "");
      }
      break;

   case PIPE_TEX_WRAP_CLAMP:
      if (bld->static_state->normalized_coords) {
         coord = lp_build_mul(coord_bld, coord, length_f);
      }
      weight = lp_build_fract(coord_bld, coord);
      coord0 = lp_build_clamp(coord_bld, coord, coord_bld->zero,
                              length_f_minus_one);
      coord1 = lp_build_add(coord_bld, coord, coord_bld->one);
      coord1 = lp_build_clamp(coord_bld, coord1, coord_bld->zero,
                              length_f_minus_one);
      coord0 = lp_build_ifloor(coord_bld, coord0);
      coord1 = lp_build_ifloor(coord_bld, coord1);
      break;

   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      if (bld->static_state->normalized_coords) {
         /* clamp to [0,1] */
         coord = lp_build_clamp(coord_bld, coord, coord_bld->zero, coord_bld->one);
         /* mul by tex size and subtract 0.5 */
         coord = lp_build_mul(coord_bld, coord, length_f);
         coord = lp_build_sub(coord_bld, coord, half);
      }
      else {
         LLVMValueRef min, max;
         /* clamp to [0.5, length - 0.5] */
         min = lp_build_const_scalar(coord_bld->type, 0.5F);
         max = lp_build_sub(coord_bld, length_f, min);
         coord = lp_build_clamp(coord_bld, coord, min, max);
      }
      /* compute lerp weight */
      weight = lp_build_fract(coord_bld, coord);
      /* coord0 = floor(coord); */
      coord0 = lp_build_ifloor(coord_bld, coord);
      coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);
      /* coord0 = max(coord0, 0) */
      coord0 = lp_build_max(int_coord_bld, coord0, int_coord_bld->zero);
      /* coord1 = min(coord1, length-1) */
      coord1 = lp_build_min(int_coord_bld, coord1, length_minus_one);
      break;

   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      {
         LLVMValueRef min, max;
         if (bld->static_state->normalized_coords) {
            /* min = -1.0 / (2 * length) = -0.5 / length */
            min = lp_build_mul(coord_bld,
                               lp_build_const_scalar(coord_bld->type, -0.5F),
                               lp_build_rcp(coord_bld, length_f));
            /* max = 1.0 - min */
            max = lp_build_sub(coord_bld, coord_bld->one, min);
            /* coord = clamp(coord, min, max) */
            coord = lp_build_clamp(coord_bld, coord, min, max);
            /* scale coord to length (and sub 0.5?) */
            coord = lp_build_mul(coord_bld, coord, length_f);
            coord = lp_build_sub(coord_bld, coord, half);
         }
         else {
            /* clamp to [-0.5, length + 0.5] */
            min = lp_build_const_scalar(coord_bld->type, -0.5F);
            max = lp_build_sub(coord_bld, length_f, min);
            coord = lp_build_clamp(coord_bld, coord, min, max);
            coord = lp_build_sub(coord_bld, coord, half);
         }
         /* compute lerp weight */
         weight = lp_build_fract(coord_bld, coord);
         /* convert to int */
         coord0 = lp_build_ifloor(coord_bld, coord);
         coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);
      }
      break;

   case PIPE_TEX_WRAP_MIRROR_REPEAT:
      /* compute mirror function */
      coord = lp_build_coord_mirror(bld, coord);

      /* scale coord to length */
      coord = lp_build_mul(coord_bld, coord, length_f);
      coord = lp_build_sub(coord_bld, coord, half);

      /* compute lerp weight */
      weight = lp_build_fract(coord_bld, coord);

      /* convert to int coords */
      coord0 = lp_build_ifloor(coord_bld, coord);
      coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);

      /* coord0 = max(coord0, 0) */
      coord0 = lp_build_max(int_coord_bld, coord0, int_coord_bld->zero);
      /* coord1 = min(coord1, length-1) */
      coord1 = lp_build_min(int_coord_bld, coord1, length_minus_one);
      break;

   case PIPE_TEX_WRAP_MIRROR_CLAMP:
      {
         LLVMValueRef min, max;
         /* min = 1.0 / (2 * length) */
         min = lp_build_rcp(coord_bld, lp_build_mul(coord_bld, two, length_f));
         /* max = 1.0 - min */
         max = lp_build_sub(coord_bld, coord_bld->one, min);

         coord = lp_build_abs(coord_bld, coord);
         coord = lp_build_clamp(coord_bld, coord, min, max);
         coord = lp_build_mul(coord_bld, coord, length_f);
         if(0)coord = lp_build_sub(coord_bld, coord, half);
         weight = lp_build_fract(coord_bld, coord);
         coord0 = lp_build_ifloor(coord_bld, coord);
         coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);
      }
      break;

   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
      {
         LLVMValueRef min, max;
         /* min = 1.0 / (2 * length) */
         min = lp_build_rcp(coord_bld, lp_build_mul(coord_bld, two, length_f));
         /* max = 1.0 - min */
         max = lp_build_sub(coord_bld, coord_bld->one, min);

         coord = lp_build_abs(coord_bld, coord);
         coord = lp_build_clamp(coord_bld, coord, min, max);
         coord = lp_build_mul(coord_bld, coord, length_f);
         coord = lp_build_sub(coord_bld, coord, half);
         weight = lp_build_fract(coord_bld, coord);
         coord0 = lp_build_ifloor(coord_bld, coord);
         coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);
      }
      break;

   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
      {
         LLVMValueRef min, max;
         /* min = -1.0 / (2 * length) = -0.5 / length */
         min = lp_build_mul(coord_bld,
                            lp_build_const_scalar(coord_bld->type, -0.5F),
                            lp_build_rcp(coord_bld, length_f));
         /* max = 1.0 - min */
         max = lp_build_sub(coord_bld, coord_bld->one, min);

         coord = lp_build_abs(coord_bld, coord);
         coord = lp_build_clamp(coord_bld, coord, min, max);
         coord = lp_build_mul(coord_bld, coord, length_f);
         coord = lp_build_sub(coord_bld, coord, half);
         weight = lp_build_fract(coord_bld, coord);
         coord0 = lp_build_ifloor(coord_bld, coord);
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
 * \param length  the texture size along one dimension, as int
 * \param is_pot  if TRUE, length is a power of two
 * \param wrap_mode  one of PIPE_TEX_WRAP_x
 */
static LLVMValueRef
lp_build_sample_wrap_nearest(struct lp_build_sample_context *bld,
                             LLVMValueRef coord,
                             LLVMValueRef length,
                             boolean is_pot,
                             unsigned wrap_mode)
{
   struct lp_build_context *coord_bld = &bld->coord_bld;
   struct lp_build_context *int_coord_bld = &bld->int_coord_bld;
   struct lp_build_context *uint_coord_bld = &bld->uint_coord_bld;
   LLVMValueRef two = lp_build_const_scalar(coord_bld->type, 2.0);
   LLVMValueRef length_f = lp_build_int_to_float(coord_bld, length);
   LLVMValueRef length_minus_one = lp_build_sub(uint_coord_bld, length, uint_coord_bld->one);
   LLVMValueRef length_f_minus_one = lp_build_sub(coord_bld, length_f, coord_bld->one);
   LLVMValueRef icoord;
   
   switch(wrap_mode) {
   case PIPE_TEX_WRAP_REPEAT:
      coord = lp_build_mul(coord_bld, coord, length_f);
      icoord = lp_build_ifloor(coord_bld, coord);
      if (is_pot)
         icoord = LLVMBuildAnd(bld->builder, icoord, length_minus_one, "");
      else
         /* Signed remainder won't give the right results for negative
          * dividends but unsigned remainder does.*/
         icoord = LLVMBuildURem(bld->builder, icoord, length, "");
      break;

   case PIPE_TEX_WRAP_CLAMP:
      /* mul by size */
      if (bld->static_state->normalized_coords) {
         coord = lp_build_mul(coord_bld, coord, length_f);
      }
      /* floor */
      icoord = lp_build_ifloor(coord_bld, coord);
      /* clamp to [0, size-1].  Note: int coord builder type */
      icoord = lp_build_clamp(int_coord_bld, icoord, int_coord_bld->zero,
                              length_minus_one);
      break;

   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      {
         LLVMValueRef min, max;
         if (bld->static_state->normalized_coords) {
            /* min = 1.0 / (2 * length) */
            min = lp_build_rcp(coord_bld, lp_build_mul(coord_bld, two, length_f));
            /* max = length - min */
            max = lp_build_sub(coord_bld, length_f, min);
            /* scale coord to length */
            coord = lp_build_mul(coord_bld, coord, length_f);
         }
         else {
            /* clamp to [0.5, length - 0.5] */
            min = lp_build_const_scalar(coord_bld->type, 0.5F);
            max = lp_build_sub(coord_bld, length_f, min);
         }
         /* coord = clamp(coord, min, max) */
         coord = lp_build_clamp(coord_bld, coord, min, max);
         icoord = lp_build_ifloor(coord_bld, coord);
      }
      break;

   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      /* Note: this is the same as CLAMP_TO_EDGE, except min = -min */
      {
         LLVMValueRef min, max;
         if (bld->static_state->normalized_coords) {
            /* min = -1.0 / (2 * length) = -0.5 / length */
            min = lp_build_mul(coord_bld,
                               lp_build_const_scalar(coord_bld->type, -0.5F),
                               lp_build_rcp(coord_bld, length_f));
            /* max = length - min */
            max = lp_build_sub(coord_bld, length_f, min);
            /* scale coord to length */
            coord = lp_build_mul(coord_bld, coord, length_f);
         }
         else {
            /* clamp to [-0.5, length + 0.5] */
            min = lp_build_const_scalar(coord_bld->type, -0.5F);
            max = lp_build_sub(coord_bld, length_f, min);
         }
         /* coord = clamp(coord, min, max) */
         coord = lp_build_clamp(coord_bld, coord, min, max);
         icoord = lp_build_ifloor(coord_bld, coord);
      }
      break;

   case PIPE_TEX_WRAP_MIRROR_REPEAT:
      {
         LLVMValueRef min, max;
         /* min = 1.0 / (2 * length) */
         min = lp_build_rcp(coord_bld, lp_build_mul(coord_bld, two, length_f));
         /* max = length - min */
         max = lp_build_sub(coord_bld, length_f, min);

         /* compute mirror function */
         coord = lp_build_coord_mirror(bld, coord);

         /* scale coord to length */
         coord = lp_build_mul(coord_bld, coord, length_f);

         /* coord = clamp(coord, min, max) */
         coord = lp_build_clamp(coord_bld, coord, min, max);
         icoord = lp_build_ifloor(coord_bld, coord);
      }
      break;

   case PIPE_TEX_WRAP_MIRROR_CLAMP:
      coord = lp_build_abs(coord_bld, coord);
      coord = lp_build_mul(coord_bld, coord, length_f);
      coord = lp_build_clamp(coord_bld, coord, coord_bld->zero, length_f_minus_one);
      icoord = lp_build_ifloor(coord_bld, coord);
      break;

   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
      {
         LLVMValueRef min, max;
         /* min = 1.0 / (2 * length) */
         min = lp_build_rcp(coord_bld, lp_build_mul(coord_bld, two, length_f));
         /* max = length - min */
         max = lp_build_sub(coord_bld, length_f, min);

         coord = lp_build_abs(coord_bld, coord);
         coord = lp_build_mul(coord_bld, coord, length_f);
         coord = lp_build_clamp(coord_bld, coord, min, max);
         icoord = lp_build_ifloor(coord_bld, coord);
      }
      break;

   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
      {
         LLVMValueRef min, max;
         /* min = 1.0 / (2 * length) */
         min = lp_build_rcp(coord_bld, lp_build_mul(coord_bld, two, length_f));
         min = lp_build_negate(coord_bld, min);
         /* max = length - min */
         max = lp_build_sub(coord_bld, length_f, min);

         coord = lp_build_abs(coord_bld, coord);
         coord = lp_build_mul(coord_bld, coord, length_f);
         coord = lp_build_clamp(coord_bld, coord, min, max);
         icoord = lp_build_ifloor(coord_bld, coord);
      }
      break;

   default:
      assert(0);
      icoord = NULL;
   }

   return icoord;
}


/**
 * Sample 2D texture with nearest filtering.
 */
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
   LLVMValueRef x, y;

   x = lp_build_sample_wrap_nearest(bld, s, width,
                                    bld->static_state->pot_width,
                                    bld->static_state->wrap_s);
   y = lp_build_sample_wrap_nearest(bld, t, height,
                                    bld->static_state->pot_height,
                                    bld->static_state->wrap_t);

   lp_build_name(x, "tex.x.wrapped");
   lp_build_name(y, "tex.y.wrapped");

   lp_build_sample_texel_soa(bld, width, height, x, y, stride, data_ptr, texel);
}


/**
 * Sample 2D texture with bilinear filtering.
 */
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
   LLVMValueRef s_fpart;
   LLVMValueRef t_fpart;
   LLVMValueRef x0, x1;
   LLVMValueRef y0, y1;
   LLVMValueRef neighbors[2][2][4];
   unsigned chan;

   lp_build_sample_wrap_linear(bld, s, width, bld->static_state->pot_width,
                               bld->static_state->wrap_s, &x0, &x1, &s_fpart);
   lp_build_sample_wrap_linear(bld, t, height, bld->static_state->pot_height,
                               bld->static_state->wrap_t, &y0, &y1, &t_fpart);

   lp_build_sample_texel_soa(bld, width, height, x0, y0, stride, data_ptr, neighbors[0][0]);
   lp_build_sample_texel_soa(bld, width, height, x1, y0, stride, data_ptr, neighbors[0][1]);
   lp_build_sample_texel_soa(bld, width, height, x0, y1, stride, data_ptr, neighbors[1][0]);
   lp_build_sample_texel_soa(bld, width, height, x1, y1, stride, data_ptr, neighbors[1][1]);

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
lp_build_rgba8_to_f32_soa(LLVMBuilderRef builder,
                          struct lp_type dst_type,
                          LLVMValueRef packed,
                          LLVMValueRef *rgba)
{
   LLVMValueRef mask = lp_build_int_const_scalar(dst_type, 0xff);
   unsigned chan;

   /* Decode the input vector components */
   for (chan = 0; chan < 4; ++chan) {
      unsigned start = chan*8;
      unsigned stop = start + 8;
      LLVMValueRef input;

      input = packed;

      if(start)
         input = LLVMBuildLShr(builder, input, lp_build_int_const_scalar(dst_type, start), "");

      if(stop < 32)
         input = LLVMBuildAnd(builder, input, mask, "");

      input = lp_build_unsigned_norm_to_float(builder, 8, dst_type, input);

      rgba[chan] = input;
   }
}


static void
lp_build_sample_2d_linear_aos(struct lp_build_sample_context *bld,
                              LLVMValueRef s,
                              LLVMValueRef t,
                              LLVMValueRef width,
                              LLVMValueRef height,
                              LLVMValueRef stride,
                              LLVMValueRef data_ptr,
                              LLVMValueRef *texel)
{
   LLVMBuilderRef builder = bld->builder;
   struct lp_build_context i32, h16, u8n;
   LLVMTypeRef i32_vec_type, h16_vec_type, u8n_vec_type;
   LLVMValueRef i32_c8, i32_c128, i32_c255;
   LLVMValueRef s_ipart, s_fpart, s_fpart_lo, s_fpart_hi;
   LLVMValueRef t_ipart, t_fpart, t_fpart_lo, t_fpart_hi;
   LLVMValueRef x0, x1;
   LLVMValueRef y0, y1;
   LLVMValueRef neighbors[2][2];
   LLVMValueRef neighbors_lo[2][2];
   LLVMValueRef neighbors_hi[2][2];
   LLVMValueRef packed, packed_lo, packed_hi;
   LLVMValueRef unswizzled[4];

   lp_build_context_init(&i32, builder, lp_type_int(32));
   lp_build_context_init(&h16, builder, lp_type_ufixed(16));
   lp_build_context_init(&u8n, builder, lp_type_unorm(8));

   i32_vec_type = lp_build_vec_type(i32.type);
   h16_vec_type = lp_build_vec_type(h16.type);
   u8n_vec_type = lp_build_vec_type(u8n.type);

   if (bld->static_state->normalized_coords) {
      LLVMTypeRef coord_vec_type = lp_build_vec_type(bld->coord_type);
      LLVMValueRef fp_width = LLVMBuildSIToFP(bld->builder, width, coord_vec_type, "");
      LLVMValueRef fp_height = LLVMBuildSIToFP(bld->builder, height, coord_vec_type, "");
      s = lp_build_mul(&bld->coord_bld, s, fp_width);
      t = lp_build_mul(&bld->coord_bld, t, fp_height);
   }

   /* scale coords by 256 (8 fractional bits) */
   s = lp_build_mul_imm(&bld->coord_bld, s, 256);
   t = lp_build_mul_imm(&bld->coord_bld, t, 256);

   /* convert float to int */
   s = LLVMBuildFPToSI(builder, s, i32_vec_type, "");
   t = LLVMBuildFPToSI(builder, t, i32_vec_type, "");

   /* subtract 0.5 (add -128) */
   i32_c128 = lp_build_int_const_scalar(i32.type, -128);
   s = LLVMBuildAdd(builder, s, i32_c128, "");
   t = LLVMBuildAdd(builder, t, i32_c128, "");

   /* compute floor (shift right 8) */
   i32_c8 = lp_build_int_const_scalar(i32.type, 8);
   s_ipart = LLVMBuildAShr(builder, s, i32_c8, "");
   t_ipart = LLVMBuildAShr(builder, t, i32_c8, "");

   /* compute fractional part (AND with 0xff) */
   i32_c255 = lp_build_int_const_scalar(i32.type, 255);
   s_fpart = LLVMBuildAnd(builder, s, i32_c255, "");
   t_fpart = LLVMBuildAnd(builder, t, i32_c255, "");

   x0 = s_ipart;
   y0 = t_ipart;

   x1 = lp_build_add(&bld->int_coord_bld, x0, bld->int_coord_bld.one);
   y1 = lp_build_add(&bld->int_coord_bld, y0, bld->int_coord_bld.one);

   x0 = lp_build_sample_wrap_int(bld, x0, width,  bld->static_state->pot_width,
                                 bld->static_state->wrap_s);
   y0 = lp_build_sample_wrap_int(bld, y0, height, bld->static_state->pot_height,
                                 bld->static_state->wrap_t);

   x1 = lp_build_sample_wrap_int(bld, x1, width,  bld->static_state->pot_width,
                                 bld->static_state->wrap_s);
   y1 = lp_build_sample_wrap_int(bld, y1, height, bld->static_state->pot_height,
                                 bld->static_state->wrap_t);

   /*
    * Transform 4 x i32 in
    *
    *   s_fpart = {s0, s1, s2, s3}
    *
    * into 8 x i16
    *
    *   s_fpart = {00, s0, 00, s1, 00, s2, 00, s3}
    *
    * into two 8 x i16
    *
    *   s_fpart_lo = {s0, s0, s0, s0, s1, s1, s1, s1}
    *   s_fpart_hi = {s2, s2, s2, s2, s3, s3, s3, s3}
    *
    * and likewise for t_fpart. There is no risk of loosing precision here
    * since the fractional parts only use the lower 8bits.
    */

   s_fpart = LLVMBuildBitCast(builder, s_fpart, h16_vec_type, "");
   t_fpart = LLVMBuildBitCast(builder, t_fpart, h16_vec_type, "");

   {
      LLVMTypeRef elem_type = LLVMInt32Type();
      LLVMValueRef shuffles_lo[LP_MAX_VECTOR_LENGTH];
      LLVMValueRef shuffles_hi[LP_MAX_VECTOR_LENGTH];
      LLVMValueRef shuffle_lo;
      LLVMValueRef shuffle_hi;
      unsigned i, j;

      for(j = 0; j < h16.type.length; j += 4) {
         unsigned subindex = util_cpu_caps.little_endian ? 0 : 1;
         LLVMValueRef index;

         index = LLVMConstInt(elem_type, j/2 + subindex, 0);
         for(i = 0; i < 4; ++i)
            shuffles_lo[j + i] = index;

         index = LLVMConstInt(elem_type, h16.type.length/2 + j/2 + subindex, 0);
         for(i = 0; i < 4; ++i)
            shuffles_hi[j + i] = index;
      }

      shuffle_lo = LLVMConstVector(shuffles_lo, h16.type.length);
      shuffle_hi = LLVMConstVector(shuffles_hi, h16.type.length);

      s_fpart_lo = LLVMBuildShuffleVector(builder, s_fpart, h16.undef, shuffle_lo, "");
      t_fpart_lo = LLVMBuildShuffleVector(builder, t_fpart, h16.undef, shuffle_lo, "");
      s_fpart_hi = LLVMBuildShuffleVector(builder, s_fpart, h16.undef, shuffle_hi, "");
      t_fpart_hi = LLVMBuildShuffleVector(builder, t_fpart, h16.undef, shuffle_hi, "");
   }

   /*
    * Fetch the pixels as 4 x 32bit (rgba order might differ):
    *
    *   rgba0 rgba1 rgba2 rgba3
    *
    * bit cast them into 16 x u8
    *
    *   r0 g0 b0 a0 r1 g1 b1 a1 r2 g2 b2 a2 r3 g3 b3 a3
    *
    * unpack them into two 8 x i16:
    *
    *   r0 g0 b0 a0 r1 g1 b1 a1
    *   r2 g2 b2 a2 r3 g3 b3 a3
    *
    * The higher 8 bits of the resulting elements will be zero.
    */

   neighbors[0][0] = lp_build_sample_packed(bld, x0, y0, stride, data_ptr);
   neighbors[0][1] = lp_build_sample_packed(bld, x1, y0, stride, data_ptr);
   neighbors[1][0] = lp_build_sample_packed(bld, x0, y1, stride, data_ptr);
   neighbors[1][1] = lp_build_sample_packed(bld, x1, y1, stride, data_ptr);

   neighbors[0][0] = LLVMBuildBitCast(builder, neighbors[0][0], u8n_vec_type, "");
   neighbors[0][1] = LLVMBuildBitCast(builder, neighbors[0][1], u8n_vec_type, "");
   neighbors[1][0] = LLVMBuildBitCast(builder, neighbors[1][0], u8n_vec_type, "");
   neighbors[1][1] = LLVMBuildBitCast(builder, neighbors[1][1], u8n_vec_type, "");

   lp_build_unpack2(builder, u8n.type, h16.type, neighbors[0][0], &neighbors_lo[0][0], &neighbors_hi[0][0]);
   lp_build_unpack2(builder, u8n.type, h16.type, neighbors[0][1], &neighbors_lo[0][1], &neighbors_hi[0][1]);
   lp_build_unpack2(builder, u8n.type, h16.type, neighbors[1][0], &neighbors_lo[1][0], &neighbors_hi[1][0]);
   lp_build_unpack2(builder, u8n.type, h16.type, neighbors[1][1], &neighbors_lo[1][1], &neighbors_hi[1][1]);

   /*
    * Linear interpolate with 8.8 fixed point.
    */

   packed_lo = lp_build_lerp_2d(&h16,
                                s_fpart_lo, t_fpart_lo,
                                neighbors_lo[0][0],
                                neighbors_lo[0][1],
                                neighbors_lo[1][0],
                                neighbors_lo[1][1]);

   packed_hi = lp_build_lerp_2d(&h16,
                                s_fpart_hi, t_fpart_hi,
                                neighbors_hi[0][0],
                                neighbors_hi[0][1],
                                neighbors_hi[1][0],
                                neighbors_hi[1][1]);

   packed = lp_build_pack2(builder, h16.type, u8n.type, packed_lo, packed_hi);

   /*
    * Convert to SoA and swizzle.
    */

   packed = LLVMBuildBitCast(builder, packed, i32_vec_type, "");

   lp_build_rgba8_to_f32_soa(bld->builder,
                             bld->texel_type,
                             packed, unswizzled);

   lp_build_format_swizzle_soa(bld->format_desc,
                               bld->texel_type, unswizzled,
                               texel);
}


static void
lp_build_sample_compare(struct lp_build_sample_context *bld,
                        LLVMValueRef p,
                        LLVMValueRef *texel)
{
   struct lp_build_context *texel_bld = &bld->texel_bld;
   LLVMValueRef res;
   unsigned chan;

   if(bld->static_state->compare_mode == PIPE_TEX_COMPARE_NONE)
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


static int
texture_dims(enum pipe_texture_target tex)
{
   switch (tex) {
   case PIPE_TEXTURE_1D:
      return 1;
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_CUBE:
      return 2;
   case PIPE_TEXTURE_3D:
      return 3;
   default:
      assert(0 && "bad texture target in texture_dims()");
      return 2;
   }
}


/**
 * Generate code to compute texture level of detail (lambda).
 * \param s  vector of texcoord s values
 * \param t  vector of texcoord t values
 * \param r  vector of texcoord r values
 * \param width  scalar int texture width
 * \param height  scalar int texture height
 * \param depth  scalar int texture depth
 */
static LLVMValueRef
lp_build_lod_selector(struct lp_build_sample_context *bld,
                      LLVMValueRef s,
                      LLVMValueRef t,
                      LLVMValueRef r,
                      LLVMValueRef width,
                      LLVMValueRef height,
                      LLVMValueRef depth)

{
   const int dims = texture_dims(bld->static_state->target);
   struct lp_build_context *coord_bld = &bld->coord_bld;

   LLVMValueRef lod_bias = lp_build_const_scalar(bld->coord_bld.type,
                                                 bld->static_state->lod_bias);
   LLVMValueRef min_lod = lp_build_const_scalar(bld->coord_bld.type,
                                                bld->static_state->min_lod);
   LLVMValueRef max_lod = lp_build_const_scalar(bld->coord_bld.type,
                                                bld->static_state->max_lod);

   LLVMValueRef index0 = LLVMConstInt(LLVMInt32Type(), 0, 0);
   LLVMValueRef index1 = LLVMConstInt(LLVMInt32Type(), 1, 0);
   LLVMValueRef index2 = LLVMConstInt(LLVMInt32Type(), 2, 0);

   LLVMValueRef s0, s1, s2;
   LLVMValueRef t0, t1, t2;
   LLVMValueRef r0, r1, r2;
   LLVMValueRef dsdx, dsdy, dtdx, dtdy, drdx, drdy;
   LLVMValueRef rho, lod;

   /*
    * dsdx = abs(s[1] - s[0]);
    * dsdy = abs(s[2] - s[0]);
    * dtdx = abs(t[1] - t[0]);
    * dtdy = abs(t[2] - t[0]);
    * drdx = abs(r[1] - r[0]);
    * drdy = abs(r[2] - r[0]);
    * XXX we're assuming a four-element quad in 2x2 layout here.
    */
   s0 = LLVMBuildExtractElement(bld->builder, s, index0, "s0");
   s1 = LLVMBuildExtractElement(bld->builder, s, index1, "s1");
   s2 = LLVMBuildExtractElement(bld->builder, s, index2, "s2");
   dsdx = lp_build_abs(coord_bld, lp_build_sub(coord_bld, s1, s0));
   dsdy = lp_build_abs(coord_bld, lp_build_sub(coord_bld, s2, s0));
   if (dims > 1) {
      t0 = LLVMBuildExtractElement(bld->builder, t, index0, "t0");
      t1 = LLVMBuildExtractElement(bld->builder, t, index1, "t1");
      t2 = LLVMBuildExtractElement(bld->builder, t, index2, "t2");
      dtdx = lp_build_abs(coord_bld, lp_build_sub(coord_bld, t1, t0));
      dtdy = lp_build_abs(coord_bld, lp_build_sub(coord_bld, t2, t0));
      if (dims > 2) {
         r0 = LLVMBuildExtractElement(bld->builder, r, index0, "r0");
         r1 = LLVMBuildExtractElement(bld->builder, r, index1, "r1");
         r2 = LLVMBuildExtractElement(bld->builder, r, index2, "r2");
         drdx = lp_build_abs(coord_bld, lp_build_sub(coord_bld, r1, r0));
         drdy = lp_build_abs(coord_bld, lp_build_sub(coord_bld, r2, r0));
      }
   }

   /* Compute rho = max of all partial derivatives scaled by texture size.
    * XXX this can be vectorized somewhat
    */
   rho = lp_build_mul(coord_bld,
                       lp_build_max(coord_bld, dsdx, dsdy),
                       lp_build_int_to_float(coord_bld, width));
   if (dims > 1) {
      LLVMValueRef max;
      max = lp_build_mul(coord_bld,
                         lp_build_max(coord_bld, dtdx, dtdy),
                         lp_build_int_to_float(coord_bld, height));
      rho = lp_build_max(coord_bld, rho, max);
      if (dims > 2) {
         max = lp_build_mul(coord_bld,
                            lp_build_max(coord_bld, drdx, drdy),
                            lp_build_int_to_float(coord_bld, depth));
         rho = lp_build_max(coord_bld, rho, max);
      }
   }

   /* compute lod = log2(rho) */
   lod = lp_build_log2(coord_bld, rho);

   /* add lod bias */
   lod = lp_build_add(coord_bld, lod, lod_bias);

   /* clamp lod */
   lod = lp_build_clamp(coord_bld, lod, min_lod, max_lod);

   return lod;
}


/**
 * For PIPE_TEX_MIPFILTER_NEAREST, convert float LOD to integer
 * mipmap level index.
 * \param lod  scalar float texture level of detail
 * \param level_out  returns integer 
 */
static void
lp_build_nearest_mip_level(struct lp_build_sample_context *bld,
                           unsigned unit,
                           LLVMValueRef lod,
                           LLVMValueRef *level_out)
{
   struct lp_build_context *coord_bld = &bld->coord_bld;
   struct lp_build_context *int_coord_bld = &bld->int_coord_bld;
   LLVMValueRef last_level, level;

   last_level = bld->dynamic_state->last_level(bld->dynamic_state,
                                               bld->builder, unit);

   /* convert float lod to integer */
   level = lp_build_iround(coord_bld, lod);

   /* clamp level to legal range of levels */
   *level_out = lp_build_clamp(int_coord_bld, level,
                               int_coord_bld->zero,
                               last_level);
}


/**
 * For PIPE_TEX_MIPFILTER_LINEAR, convert float LOD to integer to
 * two (adjacent) mipmap level indexes.  Later, we'll sample from those
 * two mipmap levels and interpolate between them.
 */
static void
lp_build_linear_mip_levels(struct lp_build_sample_context *bld,
                           unsigned unit,
                           LLVMValueRef lod,
                           LLVMValueRef *level0_out,
                           LLVMValueRef *level1_out,
                           LLVMValueRef *weight_out)
{
   struct lp_build_context *coord_bld = &bld->coord_bld;
   struct lp_build_context *int_coord_bld = &bld->int_coord_bld;
   LLVMValueRef last_level, level;

   last_level = bld->dynamic_state->last_level(bld->dynamic_state,
                                               bld->builder, unit);

   /* convert float lod to integer */
   level = lp_build_ifloor(coord_bld, lod);

   /* compute level 0 and clamp to legal range of levels */
   *level0_out = lp_build_clamp(int_coord_bld, level,
                                int_coord_bld->zero,
                                last_level);
   /* compute level 1 and clamp to legal range of levels */
   *level1_out = lp_build_add(int_coord_bld, *level0_out, int_coord_bld->one);
   *level1_out = lp_build_min(int_coord_bld, *level1_out, int_coord_bld->zero);

   *weight_out = lp_build_fract(coord_bld, lod);
}



/**
 * Build texture sampling code.
 * 'texel' will return a vector of four LLVMValueRefs corresponding to
 * R, G, B, A.
 */
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
   LLVMValueRef r;

   (void) lp_build_lod_selector;   /* temporary to silence warning */
   (void) lp_build_nearest_mip_level;
   (void) lp_build_linear_mip_levels;

   /* Setup our build context */
   memset(&bld, 0, sizeof bld);
   bld.builder = builder;
   bld.static_state = static_state;
   bld.dynamic_state = dynamic_state;
   bld.format_desc = util_format_description(static_state->format);
   bld.coord_type = type;
   bld.uint_coord_type = lp_uint_type(type);
   bld.int_coord_type = lp_int_type(type);
   bld.texel_type = type;
   lp_build_context_init(&bld.coord_bld, builder, bld.coord_type);
   lp_build_context_init(&bld.uint_coord_bld, builder, bld.uint_coord_type);
   lp_build_context_init(&bld.int_coord_bld, builder, bld.int_coord_type);
   lp_build_context_init(&bld.texel_bld, builder, bld.texel_type);

   /* Get the dynamic state */
   width = dynamic_state->width(dynamic_state, builder, unit);
   height = dynamic_state->height(dynamic_state, builder, unit);
   stride = dynamic_state->stride(dynamic_state, builder, unit);
   data_ptr = dynamic_state->data_ptr(dynamic_state, builder, unit);

   s = coords[0];
   t = coords[1];
   r = coords[2];

   width = lp_build_broadcast_scalar(&bld.uint_coord_bld, width);
   height = lp_build_broadcast_scalar(&bld.uint_coord_bld, height);
   stride = lp_build_broadcast_scalar(&bld.uint_coord_bld, stride);

   if(static_state->target == PIPE_TEXTURE_1D)
      t = bld.coord_bld.zero;

   switch (static_state->min_img_filter) {
   case PIPE_TEX_FILTER_NEAREST:
      lp_build_sample_2d_nearest_soa(&bld, s, t, width, height,
                                     stride, data_ptr, texel);
      break;
   case PIPE_TEX_FILTER_LINEAR:
      if(lp_format_is_rgba8(bld.format_desc) &&
         is_simple_wrap_mode(static_state->wrap_s) &&
         is_simple_wrap_mode(static_state->wrap_t))
         lp_build_sample_2d_linear_aos(&bld, s, t, width, height,
                                       stride, data_ptr, texel);
      else
         lp_build_sample_2d_linear_soa(&bld, s, t, width, height,
                                       stride, data_ptr, texel);
      break;
   default:
      assert(0);
   }

   /* FIXME: respect static_state->min_mip_filter */;
   /* FIXME: respect static_state->mag_img_filter */;

   lp_build_sample_compare(&bld, r, texel);
}
