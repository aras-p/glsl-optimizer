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
#include "util/u_debug.h"
#include "util/u_dump.h"
#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_format.h"
#include "lp_bld_debug.h"
#include "lp_bld_type.h"
#include "lp_bld_const.h"
#include "lp_bld_conv.h"
#include "lp_bld_arit.h"
#include "lp_bld_logic.h"
#include "lp_bld_swizzle.h"
#include "lp_bld_pack.h"
#include "lp_bld_flow.h"
#include "lp_bld_gather.h"
#include "lp_bld_format.h"
#include "lp_bld_sample.h"
#include "lp_bld_quad.h"


/**
 * Keep all information for sampling code generation in a single place.
 */
struct lp_build_sample_context
{
   LLVMBuilderRef builder;

   const struct lp_sampler_static_state *static_state;

   struct lp_sampler_dynamic_state *dynamic_state;

   const struct util_format_description *format_desc;

   /** regular scalar float type */
   struct lp_type float_type;
   struct lp_build_context float_bld;

   /** regular scalar float type */
   struct lp_type int_type;
   struct lp_build_context int_bld;

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


static LLVMValueRef
lp_build_get_mipmap_level(struct lp_build_sample_context *bld,
                          LLVMValueRef data_array, LLVMValueRef level)
{
   LLVMValueRef indexes[2], data_ptr;
   indexes[0] = LLVMConstInt(LLVMInt32Type(), 0, 0);
   indexes[1] = level;
   data_ptr = LLVMBuildGEP(bld->builder, data_array, indexes, 2, "");
   data_ptr = LLVMBuildLoad(bld->builder, data_ptr, "");
   return data_ptr;
}


static LLVMValueRef
lp_build_get_const_mipmap_level(struct lp_build_sample_context *bld,
                                LLVMValueRef data_array, int level)
{
   LLVMValueRef lvl = LLVMConstInt(LLVMInt32Type(), level, 0);
   return lp_build_get_mipmap_level(bld, data_array, lvl);
}


/**
 * Dereference stride_array[mipmap_level] array to get a stride.
 * Return stride as a vector.
 */
static LLVMValueRef
lp_build_get_level_stride_vec(struct lp_build_sample_context *bld,
                              LLVMValueRef stride_array, LLVMValueRef level)
{
   LLVMValueRef indexes[2], stride;
   indexes[0] = LLVMConstInt(LLVMInt32Type(), 0, 0);
   indexes[1] = level;
   stride = LLVMBuildGEP(bld->builder, stride_array, indexes, 2, "");
   stride = LLVMBuildLoad(bld->builder, stride, "");
   stride = lp_build_broadcast_scalar(&bld->int_coord_bld, stride);
   return stride;
}


/** Dereference stride_array[0] array to get a stride (as vector). */
static LLVMValueRef
lp_build_get_const_level_stride_vec(struct lp_build_sample_context *bld,
                                    LLVMValueRef stride_array, int level)
{
   LLVMValueRef lvl = LLVMConstInt(LLVMInt32Type(), level, 0);
   return lp_build_get_level_stride_vec(bld, stride_array, lvl);
}


static int
texture_dims(enum pipe_texture_target tex)
{
   switch (tex) {
   case PIPE_TEXTURE_1D:
      return 1;
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_RECT:
   case PIPE_TEXTURE_CUBE:
      return 2;
   case PIPE_TEXTURE_3D:
      return 3;
   default:
      assert(0 && "bad texture target in texture_dims()");
      return 2;
   }
}


static void
apply_sampler_swizzle(struct lp_build_sample_context *bld,
                      LLVMValueRef *texel)
{
   unsigned char swizzles[4];

   swizzles[0] = bld->static_state->swizzle_r;
   swizzles[1] = bld->static_state->swizzle_g;
   swizzles[2] = bld->static_state->swizzle_b;
   swizzles[3] = bld->static_state->swizzle_a;

   lp_build_swizzle_soa_inplace(&bld->texel_bld, texel, swizzles);
}



/**
 * Generate code to fetch a texel from a texture at int coords (x, y, z).
 * The computation depends on whether the texture is 1D, 2D or 3D.
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
                          LLVMValueRef depth,
                          LLVMValueRef x,
                          LLVMValueRef y,
                          LLVMValueRef z,
                          LLVMValueRef y_stride,
                          LLVMValueRef z_stride,
                          LLVMValueRef data_ptr,
                          LLVMValueRef texel_out[4])
{
   const int dims = texture_dims(bld->static_state->target);
   struct lp_build_context *int_coord_bld = &bld->int_coord_bld;
   LLVMValueRef offset;
   LLVMValueRef i, j;
   LLVMValueRef use_border = NULL;

   /* use_border = x < 0 || x >= width || y < 0 || y >= height */
   if (wrap_mode_uses_border_color(bld->static_state->wrap_s)) {
      LLVMValueRef b1, b2;
      b1 = lp_build_cmp(int_coord_bld, PIPE_FUNC_LESS, x, int_coord_bld->zero);
      b2 = lp_build_cmp(int_coord_bld, PIPE_FUNC_GEQUAL, x, width);
      use_border = LLVMBuildOr(bld->builder, b1, b2, "b1_or_b2");
   }

   if (dims >= 2 && wrap_mode_uses_border_color(bld->static_state->wrap_t)) {
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

   if (dims == 3 && wrap_mode_uses_border_color(bld->static_state->wrap_r)) {
      LLVMValueRef b1, b2;
      b1 = lp_build_cmp(int_coord_bld, PIPE_FUNC_LESS, z, int_coord_bld->zero);
      b2 = lp_build_cmp(int_coord_bld, PIPE_FUNC_GEQUAL, z, depth);
      if (use_border) {
         use_border = LLVMBuildOr(bld->builder, use_border, b1, "ub_or_b1");
         use_border = LLVMBuildOr(bld->builder, use_border, b2, "ub_or_b2");
      }
      else {
         use_border = LLVMBuildOr(bld->builder, b1, b2, "b1_or_b2");
      }
   }

   /* convert x,y,z coords to linear offset from start of texture, in bytes */
   lp_build_sample_offset(&bld->uint_coord_bld,
                          bld->format_desc,
                          x, y, z, y_stride, z_stride,
                          &offset, &i, &j);

   if (use_border) {
      /* If we can sample the border color, it means that texcoords may
       * lie outside the bounds of the texture image.  We need to do
       * something to prevent reading out of bounds and causing a segfault.
       *
       * Simply AND the texture coords with !use_border.  This will cause
       * coords which are out of bounds to become zero.  Zero's guaranteed
       * to be inside the texture image.
       */
      offset = lp_build_andc(&bld->uint_coord_bld, offset, use_border);
   }

   lp_build_fetch_rgba_soa(bld->builder,
                           bld->format_desc,
                           bld->texel_type,
                           data_ptr, offset,
                           i, j,
                           texel_out);

   apply_sampler_swizzle(bld, texel_out);

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
      /* select texel color or border color depending on use_border */
      int chan;
      for (chan = 0; chan < 4; chan++) {
         LLVMValueRef border_chan =
            lp_build_const_vec(bld->texel_type,
                                  bld->static_state->border_color[chan]);
         texel_out[chan] = lp_build_select(&bld->texel_bld, use_border,
                                           border_chan, texel_out[chan]);
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
 * We only support a few wrap modes in lp_build_sample_wrap_linear_int() at this time.
 * Return whether the given mode is supported by that function.
 */
static boolean
is_simple_wrap_mode(unsigned mode)
{
   switch (mode) {
   case PIPE_TEX_WRAP_REPEAT:
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      return TRUE;
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
 * \param i0  resulting sub-block pixel coordinate for coord0
 */
static void
lp_build_sample_wrap_nearest_int(struct lp_build_sample_context *bld,
                                 unsigned block_length,
                                 LLVMValueRef coord,
                                 LLVMValueRef length,
                                 LLVMValueRef stride,
                                 boolean is_pot,
                                 unsigned wrap_mode,
                                 LLVMValueRef *out_offset,
                                 LLVMValueRef *out_i)
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

   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      coord = lp_build_max(int_coord_bld, coord, int_coord_bld->zero);
      coord = lp_build_min(int_coord_bld, coord, length_minus_one);
      break;

   case PIPE_TEX_WRAP_CLAMP:
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
   case PIPE_TEX_WRAP_MIRROR_REPEAT:
   case PIPE_TEX_WRAP_MIRROR_CLAMP:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
   default:
      assert(0);
   }

   lp_build_sample_partial_offset(uint_coord_bld, block_length, coord, stride,
                                  out_offset, out_i);
}


/**
 * Build LLVM code for texture wrap mode, for scaled integer texcoords.
 * \param coord0  the incoming texcoord (s,t,r or q) scaled to the texture size
 * \param length  the texture size along one dimension
 * \param stride  pixel stride along the coordinate axis
 * \param block_length  is the length of the pixel block along the
 *                      coordinate axis
 * \param is_pot  if TRUE, length is a power of two
 * \param wrap_mode  one of PIPE_TEX_WRAP_x
 * \param offset0  resulting relative offset for coord0
 * \param offset1  resulting relative offset for coord0 + 1
 * \param i0  resulting sub-block pixel coordinate for coord0
 * \param i1  resulting sub-block pixel coordinate for coord0 + 1
 */
static void
lp_build_sample_wrap_linear_int(struct lp_build_sample_context *bld,
                                unsigned block_length,
                                LLVMValueRef coord0,
                                LLVMValueRef length,
                                LLVMValueRef stride,
                                boolean is_pot,
                                unsigned wrap_mode,
                                LLVMValueRef *offset0,
                                LLVMValueRef *offset1,
                                LLVMValueRef *i0,
                                LLVMValueRef *i1)
{
   struct lp_build_context *uint_coord_bld = &bld->uint_coord_bld;
   struct lp_build_context *int_coord_bld = &bld->int_coord_bld;
   LLVMValueRef length_minus_one;
   LLVMValueRef lmask, umask, mask;

   if (block_length != 1) {
      /*
       * If the pixel block covers more than one pixel then there is no easy
       * way to calculate offset1 relative to offset0. Instead, compute them
       * independently.
       */

      LLVMValueRef coord1;

      lp_build_sample_wrap_nearest_int(bld,
                                       block_length,
                                       coord0,
                                       length,
                                       stride,
                                       is_pot,
                                       wrap_mode,
                                       offset0, i0);

      coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);

      lp_build_sample_wrap_nearest_int(bld,
                                       block_length,
                                       coord1,
                                       length,
                                       stride,
                                       is_pot,
                                       wrap_mode,
                                       offset1, i1);

      return;
   }

   /*
    * Scalar pixels -- try to compute offset0 and offset1 with a single stride
    * multiplication.
    */

   *i0 = uint_coord_bld->zero;
   *i1 = uint_coord_bld->zero;

   length_minus_one = lp_build_sub(int_coord_bld, length, int_coord_bld->one);

   switch(wrap_mode) {
   case PIPE_TEX_WRAP_REPEAT:
      if (is_pot) {
         coord0 = LLVMBuildAnd(bld->builder, coord0, length_minus_one, "");
      }
      else {
         /* Signed remainder won't give the right results for negative
          * dividends but unsigned remainder does.*/
         coord0 = LLVMBuildURem(bld->builder, coord0, length, "");
      }

      mask = lp_build_compare(bld->builder, int_coord_bld->type,
                              PIPE_FUNC_NOTEQUAL, coord0, length_minus_one);

      *offset0 = lp_build_mul(uint_coord_bld, coord0, stride);
      *offset1 = LLVMBuildAnd(bld->builder,
                              lp_build_add(uint_coord_bld, *offset0, stride),
                              mask, "");
      break;

   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      lmask = lp_build_compare(int_coord_bld->builder, int_coord_bld->type,
                               PIPE_FUNC_GEQUAL, coord0, int_coord_bld->zero);
      umask = lp_build_compare(int_coord_bld->builder, int_coord_bld->type,
                               PIPE_FUNC_LESS, coord0, length_minus_one);

      coord0 = lp_build_select(int_coord_bld, lmask, coord0, int_coord_bld->zero);
      coord0 = lp_build_select(int_coord_bld, umask, coord0, length_minus_one);

      mask = LLVMBuildAnd(bld->builder, lmask, umask, "");

      *offset0 = lp_build_mul(uint_coord_bld, coord0, stride);
      *offset1 = lp_build_add(uint_coord_bld,
                              *offset0,
                              LLVMBuildAnd(bld->builder, stride, mask, ""));
      break;

   case PIPE_TEX_WRAP_CLAMP:
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
   case PIPE_TEX_WRAP_MIRROR_REPEAT:
   case PIPE_TEX_WRAP_MIRROR_CLAMP:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
   default:
      assert(0);
      *offset0 = uint_coord_bld->zero;
      *offset1 = uint_coord_bld->zero;
      break;
   }
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
   LLVMValueRef half = lp_build_const_vec(coord_bld->type, 0.5);
   LLVMValueRef length_f = lp_build_int_to_float(coord_bld, length);
   LLVMValueRef length_minus_one = lp_build_sub(uint_coord_bld, length, uint_coord_bld->one);
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
         /* scale coord to length */
         coord = lp_build_mul(coord_bld, coord, length_f);
      }

      /* clamp to [0, length] */
      coord = lp_build_clamp(coord_bld, coord, coord_bld->zero, length_f);

      coord = lp_build_sub(coord_bld, coord, half);

      weight = lp_build_fract(coord_bld, coord);
      coord0 = lp_build_ifloor(coord_bld, coord);
      coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);
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
         min = half;
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
            /* scale coord to length */
            coord = lp_build_mul(coord_bld, coord, length_f);
         }
         /* clamp to [-0.5, length + 0.5] */
         min = lp_build_const_vec(coord_bld->type, -0.5F);
         max = lp_build_sub(coord_bld, length_f, min);
         coord = lp_build_clamp(coord_bld, coord, min, max);
         coord = lp_build_sub(coord_bld, coord, half);
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
      coord = lp_build_abs(coord_bld, coord);

      if (bld->static_state->normalized_coords) {
         /* scale coord to length */
         coord = lp_build_mul(coord_bld, coord, length_f);
      }

      /* clamp to [0, length] */
      coord = lp_build_min(coord_bld, coord, length_f);

      coord = lp_build_sub(coord_bld, coord, half);

      weight = lp_build_fract(coord_bld, coord);
      coord0 = lp_build_ifloor(coord_bld, coord);
      coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);
      break;

   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
      {
         LLVMValueRef min, max;

         coord = lp_build_abs(coord_bld, coord);

         if (bld->static_state->normalized_coords) {
            /* scale coord to length */
            coord = lp_build_mul(coord_bld, coord, length_f);
         }

         /* clamp to [0.5, length - 0.5] */
         min = half;
         max = lp_build_sub(coord_bld, length_f, min);
         coord = lp_build_clamp(coord_bld, coord, min, max);

         coord = lp_build_sub(coord_bld, coord, half);

         weight = lp_build_fract(coord_bld, coord);
         coord0 = lp_build_ifloor(coord_bld, coord);
         coord1 = lp_build_add(int_coord_bld, coord0, int_coord_bld->one);
      }
      break;

   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
      {
         LLVMValueRef min, max;

         coord = lp_build_abs(coord_bld, coord);

         if (bld->static_state->normalized_coords) {
            /* scale coord to length */
            coord = lp_build_mul(coord_bld, coord, length_f);
         }

         /* clamp to [-0.5, length + 0.5] */
         min = lp_build_negate(coord_bld, half);
         max = lp_build_sub(coord_bld, length_f, min);
         coord = lp_build_clamp(coord_bld, coord, min, max);

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
   LLVMValueRef length_f = lp_build_int_to_float(coord_bld, length);
   LLVMValueRef length_minus_one = lp_build_sub(uint_coord_bld, length, uint_coord_bld->one);
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
   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      if (bld->static_state->normalized_coords) {
         /* scale coord to length */
         coord = lp_build_mul(coord_bld, coord, length_f);
      }

      /* floor */
      icoord = lp_build_ifloor(coord_bld, coord);

      /* clamp to [0, length - 1]. */
      icoord = lp_build_clamp(int_coord_bld, icoord, int_coord_bld->zero,
                              length_minus_one);
      break;

   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
      /* Note: this is the same as CLAMP_TO_EDGE, except min = -min */
      {
         LLVMValueRef min, max;

         if (bld->static_state->normalized_coords) {
            /* scale coord to length */
            coord = lp_build_mul(coord_bld, coord, length_f);
         }

         icoord = lp_build_ifloor(coord_bld, coord);

         /* clamp to [-1, length] */
         min = lp_build_negate(int_coord_bld, int_coord_bld->one);
         max = length;
         icoord = lp_build_clamp(int_coord_bld, icoord, min, max);
      }
      break;

   case PIPE_TEX_WRAP_MIRROR_REPEAT:
      /* compute mirror function */
      coord = lp_build_coord_mirror(bld, coord);

      /* scale coord to length */
      assert(bld->static_state->normalized_coords);
      coord = lp_build_mul(coord_bld, coord, length_f);

      icoord = lp_build_ifloor(coord_bld, coord);

      /* clamp to [0, length - 1] */
      icoord = lp_build_min(int_coord_bld, icoord, length_minus_one);
      break;

   case PIPE_TEX_WRAP_MIRROR_CLAMP:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
      coord = lp_build_abs(coord_bld, coord);

      if (bld->static_state->normalized_coords) {
         /* scale coord to length */
         coord = lp_build_mul(coord_bld, coord, length_f);
      }

      icoord = lp_build_ifloor(coord_bld, coord);

      /* clamp to [0, length - 1] */
      icoord = lp_build_min(int_coord_bld, icoord, length_minus_one);
      break;

   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
      coord = lp_build_abs(coord_bld, coord);

      if (bld->static_state->normalized_coords) {
         /* scale coord to length */
         coord = lp_build_mul(coord_bld, coord, length_f);
      }

      icoord = lp_build_ifloor(coord_bld, coord);

      /* clamp to [0, length] */
      icoord = lp_build_min(int_coord_bld, icoord, length);
      break;

   default:
      assert(0);
      icoord = NULL;
   }

   return icoord;
}


/**
 * Codegen equivalent for u_minify().
 * Return max(1, base_size >> level);
 */
static LLVMValueRef
lp_build_minify(struct lp_build_sample_context *bld,
                LLVMValueRef base_size,
                LLVMValueRef level)
{
   LLVMValueRef size = LLVMBuildLShr(bld->builder, base_size, level, "minify");
   size = lp_build_max(&bld->int_coord_bld, size, bld->int_coord_bld.one);
   return size;
}


/**
 * Generate code to compute texture level of detail (lambda).
 * \param ddx  partial derivatives of (s, t, r, q) with respect to X
 * \param ddy  partial derivatives of (s, t, r, q) with respect to Y
 * \param lod_bias  optional float vector with the shader lod bias
 * \param explicit_lod  optional float vector with the explicit lod
 * \param width  scalar int texture width
 * \param height  scalar int texture height
 * \param depth  scalar int texture depth
 *
 * XXX: The resulting lod is scalar, so ignore all but the first element of
 * derivatives, lod_bias, etc that are passed by the shader.
 */
static LLVMValueRef
lp_build_lod_selector(struct lp_build_sample_context *bld,
                      const LLVMValueRef ddx[4],
                      const LLVMValueRef ddy[4],
                      LLVMValueRef lod_bias, /* optional */
                      LLVMValueRef explicit_lod, /* optional */
                      LLVMValueRef width,
                      LLVMValueRef height,
                      LLVMValueRef depth)

{
   if (bld->static_state->min_lod == bld->static_state->max_lod) {
      /* User is forcing sampling from a particular mipmap level.
       * This is hit during mipmap generation.
       */
      return LLVMConstReal(LLVMFloatType(), bld->static_state->min_lod);
   }
   else {
      struct lp_build_context *float_bld = &bld->float_bld;
      LLVMValueRef sampler_lod_bias = LLVMConstReal(LLVMFloatType(),
                                                    bld->static_state->lod_bias);
      LLVMValueRef min_lod = LLVMConstReal(LLVMFloatType(),
                                           bld->static_state->min_lod);
      LLVMValueRef max_lod = LLVMConstReal(LLVMFloatType(),
                                           bld->static_state->max_lod);
      LLVMValueRef index0 = LLVMConstInt(LLVMInt32Type(), 0, 0);
      LLVMValueRef lod;

      if (explicit_lod) {
         lod = LLVMBuildExtractElement(bld->builder, explicit_lod,
                                       index0, "");
      }
      else {
         const int dims = texture_dims(bld->static_state->target);
         LLVMValueRef dsdx, dsdy;
         LLVMValueRef dtdx = NULL, dtdy = NULL, drdx = NULL, drdy = NULL;
         LLVMValueRef rho;

         dsdx = LLVMBuildExtractElement(bld->builder, ddx[0], index0, "dsdx");
         dsdx = lp_build_abs(float_bld, dsdx);
         dsdy = LLVMBuildExtractElement(bld->builder, ddy[0], index0, "dsdy");
         dsdy = lp_build_abs(float_bld, dsdy);
         if (dims > 1) {
            dtdx = LLVMBuildExtractElement(bld->builder, ddx[1], index0, "dtdx");
            dtdx = lp_build_abs(float_bld, dtdx);
            dtdy = LLVMBuildExtractElement(bld->builder, ddy[1], index0, "dtdy");
            dtdy = lp_build_abs(float_bld, dtdy);
            if (dims > 2) {
               drdx = LLVMBuildExtractElement(bld->builder, ddx[2], index0, "drdx");
               drdx = lp_build_abs(float_bld, drdx);
               drdy = LLVMBuildExtractElement(bld->builder, ddy[2], index0, "drdy");
               drdy = lp_build_abs(float_bld, drdy);
            }
         }

         /* Compute rho = max of all partial derivatives scaled by texture size.
          * XXX this could be vectorized somewhat
          */
         rho = LLVMBuildFMul(bld->builder,
                            lp_build_max(float_bld, dsdx, dsdy),
                            lp_build_int_to_float(float_bld, width), "");
         if (dims > 1) {
            LLVMValueRef max;
            max = LLVMBuildFMul(bld->builder,
                               lp_build_max(float_bld, dtdx, dtdy),
                               lp_build_int_to_float(float_bld, height), "");
            rho = lp_build_max(float_bld, rho, max);
            if (dims > 2) {
               max = LLVMBuildFMul(bld->builder,
                                  lp_build_max(float_bld, drdx, drdy),
                                  lp_build_int_to_float(float_bld, depth), "");
               rho = lp_build_max(float_bld, rho, max);
            }
         }

         /* compute lod = log2(rho) */
         lod = lp_build_log2(float_bld, rho);

         /* add shader lod bias */
         if (lod_bias) {
            lod_bias = LLVMBuildExtractElement(bld->builder, lod_bias,
                                               index0, "");
            lod = LLVMBuildFAdd(bld->builder, lod, lod_bias, "shader_lod_bias");
         }
      }

      /* add sampler lod bias */
      lod = LLVMBuildFAdd(bld->builder, lod, sampler_lod_bias, "sampler_lod_bias");

      /* clamp lod */
      lod = lp_build_clamp(float_bld, lod, min_lod, max_lod);

      return lod;
   }
}


/**
 * For PIPE_TEX_MIPFILTER_NEAREST, convert float LOD to integer
 * mipmap level index.
 * Note: this is all scalar code.
 * \param lod  scalar float texture level of detail
 * \param level_out  returns integer 
 */
static void
lp_build_nearest_mip_level(struct lp_build_sample_context *bld,
                           unsigned unit,
                           LLVMValueRef lod,
                           LLVMValueRef *level_out)
{
   struct lp_build_context *float_bld = &bld->float_bld;
   struct lp_build_context *int_bld = &bld->int_bld;
   LLVMValueRef last_level, level;

   LLVMValueRef zero = LLVMConstInt(LLVMInt32Type(), 0, 0);

   last_level = bld->dynamic_state->last_level(bld->dynamic_state,
                                               bld->builder, unit);

   /* convert float lod to integer */
   level = lp_build_iround(float_bld, lod);

   /* clamp level to legal range of levels */
   *level_out = lp_build_clamp(int_bld, level, zero, last_level);
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
   struct lp_build_context *float_bld = &bld->float_bld;
   struct lp_build_context *int_bld = &bld->int_bld;
   LLVMValueRef last_level, level;

   last_level = bld->dynamic_state->last_level(bld->dynamic_state,
                                               bld->builder, unit);

   /* convert float lod to integer */
   level = lp_build_ifloor(float_bld, lod);

   /* compute level 0 and clamp to legal range of levels */
   *level0_out = lp_build_clamp(int_bld, level,
                                int_bld->zero,
                                last_level);
   /* compute level 1 and clamp to legal range of levels */
   level = lp_build_add(int_bld, level, int_bld->one);
   *level1_out = lp_build_clamp(int_bld, level,
                                int_bld->zero,
                                last_level);

   *weight_out = lp_build_fract(float_bld, lod);
}


/**
 * Generate code to sample a mipmap level with nearest filtering.
 * If sampling a cube texture, r = cube face in [0,5].
 */
static void
lp_build_sample_image_nearest(struct lp_build_sample_context *bld,
                              LLVMValueRef width_vec,
                              LLVMValueRef height_vec,
                              LLVMValueRef depth_vec,
                              LLVMValueRef row_stride_vec,
                              LLVMValueRef img_stride_vec,
                              LLVMValueRef data_ptr,
                              LLVMValueRef s,
                              LLVMValueRef t,
                              LLVMValueRef r,
                              LLVMValueRef colors_out[4])
{
   const int dims = texture_dims(bld->static_state->target);
   LLVMValueRef x, y, z;

   /*
    * Compute integer texcoords.
    */
   x = lp_build_sample_wrap_nearest(bld, s, width_vec,
                                    bld->static_state->pot_width,
                                    bld->static_state->wrap_s);
   lp_build_name(x, "tex.x.wrapped");

   if (dims >= 2) {
      y = lp_build_sample_wrap_nearest(bld, t, height_vec,
                                       bld->static_state->pot_height,
                                       bld->static_state->wrap_t);
      lp_build_name(y, "tex.y.wrapped");

      if (dims == 3) {
         z = lp_build_sample_wrap_nearest(bld, r, depth_vec,
                                          bld->static_state->pot_height,
                                          bld->static_state->wrap_r);
         lp_build_name(z, "tex.z.wrapped");
      }
      else if (bld->static_state->target == PIPE_TEXTURE_CUBE) {
         z = r;
      }
      else {
         z = NULL;
      }
   }
   else {
      y = z = NULL;
   }

   /*
    * Get texture colors.
    */
   lp_build_sample_texel_soa(bld, width_vec, height_vec, depth_vec,
                             x, y, z,
                             row_stride_vec, img_stride_vec,
                             data_ptr, colors_out);
}


/**
 * Generate code to sample a mipmap level with linear filtering.
 * If sampling a cube texture, r = cube face in [0,5].
 */
static void
lp_build_sample_image_linear(struct lp_build_sample_context *bld,
                             LLVMValueRef width_vec,
                             LLVMValueRef height_vec,
                             LLVMValueRef depth_vec,
                             LLVMValueRef row_stride_vec,
                             LLVMValueRef img_stride_vec,
                             LLVMValueRef data_ptr,
                             LLVMValueRef s,
                             LLVMValueRef t,
                             LLVMValueRef r,
                             LLVMValueRef colors_out[4])
{
   const int dims = texture_dims(bld->static_state->target);
   LLVMValueRef x0, y0, z0, x1, y1, z1;
   LLVMValueRef s_fpart, t_fpart, r_fpart;
   LLVMValueRef neighbors[2][2][4];
   int chan;

   /*
    * Compute integer texcoords.
    */
   lp_build_sample_wrap_linear(bld, s, width_vec,
                               bld->static_state->pot_width,
                               bld->static_state->wrap_s,
                               &x0, &x1, &s_fpart);
   lp_build_name(x0, "tex.x0.wrapped");
   lp_build_name(x1, "tex.x1.wrapped");

   if (dims >= 2) {
      lp_build_sample_wrap_linear(bld, t, height_vec,
                                  bld->static_state->pot_height,
                                  bld->static_state->wrap_t,
                                  &y0, &y1, &t_fpart);
      lp_build_name(y0, "tex.y0.wrapped");
      lp_build_name(y1, "tex.y1.wrapped");

      if (dims == 3) {
         lp_build_sample_wrap_linear(bld, r, depth_vec,
                                     bld->static_state->pot_depth,
                                     bld->static_state->wrap_r,
                                     &z0, &z1, &r_fpart);
         lp_build_name(z0, "tex.z0.wrapped");
         lp_build_name(z1, "tex.z1.wrapped");
      }
      else if (bld->static_state->target == PIPE_TEXTURE_CUBE) {
         z0 = z1 = r;  /* cube face */
         r_fpart = NULL;
      }
      else {
         z0 = z1 = NULL;
         r_fpart = NULL;
      }
   }
   else {
      y0 = y1 = t_fpart = NULL;
      z0 = z1 = r_fpart = NULL;
   }

   /*
    * Get texture colors.
    */
   /* get x0/x1 texels */
   lp_build_sample_texel_soa(bld, width_vec, height_vec, depth_vec,
                             x0, y0, z0,
                             row_stride_vec, img_stride_vec,
                             data_ptr, neighbors[0][0]);
   lp_build_sample_texel_soa(bld, width_vec, height_vec, depth_vec,
                             x1, y0, z0,
                             row_stride_vec, img_stride_vec,
                             data_ptr, neighbors[0][1]);

   if (dims == 1) {
      /* Interpolate two samples from 1D image to produce one color */
      for (chan = 0; chan < 4; chan++) {
         colors_out[chan] = lp_build_lerp(&bld->texel_bld, s_fpart,
                                          neighbors[0][0][chan],
                                          neighbors[0][1][chan]);
      }
   }
   else {
      /* 2D/3D texture */
      LLVMValueRef colors0[4];

      /* get x0/x1 texels at y1 */
      lp_build_sample_texel_soa(bld, width_vec, height_vec, depth_vec,
                                x0, y1, z0,
                                row_stride_vec, img_stride_vec,
                                data_ptr, neighbors[1][0]);
      lp_build_sample_texel_soa(bld, width_vec, height_vec, depth_vec,
                                x1, y1, z0,
                                row_stride_vec, img_stride_vec,
                                data_ptr, neighbors[1][1]);

      /* Bilinear interpolate the four samples from the 2D image / 3D slice */
      for (chan = 0; chan < 4; chan++) {
         colors0[chan] = lp_build_lerp_2d(&bld->texel_bld,
                                          s_fpart, t_fpart,
                                          neighbors[0][0][chan],
                                          neighbors[0][1][chan],
                                          neighbors[1][0][chan],
                                          neighbors[1][1][chan]);
      }

      if (dims == 3) {
         LLVMValueRef neighbors1[2][2][4];
         LLVMValueRef colors1[4];

         /* get x0/x1/y0/y1 texels at z1 */
         lp_build_sample_texel_soa(bld, width_vec, height_vec, depth_vec,
                                   x0, y0, z1,
                                   row_stride_vec, img_stride_vec,
                                   data_ptr, neighbors1[0][0]);
         lp_build_sample_texel_soa(bld, width_vec, height_vec, depth_vec,
                                   x1, y0, z1,
                                   row_stride_vec, img_stride_vec,
                                   data_ptr, neighbors1[0][1]);
         lp_build_sample_texel_soa(bld, width_vec, height_vec, depth_vec,
                                   x0, y1, z1,
                                   row_stride_vec, img_stride_vec,
                                   data_ptr, neighbors1[1][0]);
         lp_build_sample_texel_soa(bld, width_vec, height_vec, depth_vec,
                                   x1, y1, z1,
                                   row_stride_vec, img_stride_vec,
                                   data_ptr, neighbors1[1][1]);

         /* Bilinear interpolate the four samples from the second Z slice */
         for (chan = 0; chan < 4; chan++) {
            colors1[chan] = lp_build_lerp_2d(&bld->texel_bld,
                                             s_fpart, t_fpart,
                                             neighbors1[0][0][chan],
                                             neighbors1[0][1][chan],
                                             neighbors1[1][0][chan],
                                             neighbors1[1][1][chan]);
         }

         /* Linearly interpolate the two samples from the two 3D slices */
         for (chan = 0; chan < 4; chan++) {
            colors_out[chan] = lp_build_lerp(&bld->texel_bld,
                                             r_fpart,
                                             colors0[chan], colors1[chan]);
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


/** Helper used by lp_build_cube_lookup() */
static LLVMValueRef
lp_build_cube_ima(struct lp_build_context *coord_bld, LLVMValueRef coord)
{
   /* ima = -0.5 / abs(coord); */
   LLVMValueRef negHalf = lp_build_const_vec(coord_bld->type, -0.5);
   LLVMValueRef absCoord = lp_build_abs(coord_bld, coord);
   LLVMValueRef ima = lp_build_div(coord_bld, negHalf, absCoord);
   return ima;
}


/**
 * Helper used by lp_build_cube_lookup()
 * \param sign  scalar +1 or -1
 * \param coord  float vector
 * \param ima  float vector
 */
static LLVMValueRef
lp_build_cube_coord(struct lp_build_context *coord_bld,
                    LLVMValueRef sign, int negate_coord,
                    LLVMValueRef coord, LLVMValueRef ima)
{
   /* return negate(coord) * ima * sign + 0.5; */
   LLVMValueRef half = lp_build_const_vec(coord_bld->type, 0.5);
   LLVMValueRef res;

   assert(negate_coord == +1 || negate_coord == -1);

   if (negate_coord == -1) {
      coord = lp_build_negate(coord_bld, coord);
   }

   res = lp_build_mul(coord_bld, coord, ima);
   if (sign) {
      sign = lp_build_broadcast_scalar(coord_bld, sign);
      res = lp_build_mul(coord_bld, res, sign);
   }
   res = lp_build_add(coord_bld, res, half);

   return res;
}


/** Helper used by lp_build_cube_lookup()
 * Return (major_coord >= 0) ? pos_face : neg_face;
 */
static LLVMValueRef
lp_build_cube_face(struct lp_build_sample_context *bld,
                   LLVMValueRef major_coord,
                   unsigned pos_face, unsigned neg_face)
{
   LLVMValueRef cmp = LLVMBuildFCmp(bld->builder, LLVMRealUGE,
                                    major_coord,
                                    bld->float_bld.zero, "");
   LLVMValueRef pos = LLVMConstInt(LLVMInt32Type(), pos_face, 0);
   LLVMValueRef neg = LLVMConstInt(LLVMInt32Type(), neg_face, 0);
   LLVMValueRef res = LLVMBuildSelect(bld->builder, cmp, pos, neg, "");
   return res;
}



/**
 * Generate code to do cube face selection and compute per-face texcoords.
 */
static void
lp_build_cube_lookup(struct lp_build_sample_context *bld,
                     LLVMValueRef s,
                     LLVMValueRef t,
                     LLVMValueRef r,
                     LLVMValueRef *face,
                     LLVMValueRef *face_s,
                     LLVMValueRef *face_t)
{
   struct lp_build_context *float_bld = &bld->float_bld;
   struct lp_build_context *coord_bld = &bld->coord_bld;
   LLVMValueRef rx, ry, rz;
   LLVMValueRef arx, ary, arz;
   LLVMValueRef c25 = LLVMConstReal(LLVMFloatType(), 0.25);
   LLVMValueRef arx_ge_ary, arx_ge_arz;
   LLVMValueRef ary_ge_arx, ary_ge_arz;
   LLVMValueRef arx_ge_ary_arz, ary_ge_arx_arz;
   LLVMValueRef rx_pos, ry_pos, rz_pos;

   assert(bld->coord_bld.type.length == 4);

   /*
    * Use the average of the four pixel's texcoords to choose the face.
    */
   rx = lp_build_mul(float_bld, c25,
                     lp_build_sum_vector(&bld->coord_bld, s));
   ry = lp_build_mul(float_bld, c25,
                     lp_build_sum_vector(&bld->coord_bld, t));
   rz = lp_build_mul(float_bld, c25,
                     lp_build_sum_vector(&bld->coord_bld, r));

   arx = lp_build_abs(float_bld, rx);
   ary = lp_build_abs(float_bld, ry);
   arz = lp_build_abs(float_bld, rz);

   /*
    * Compare sign/magnitude of rx,ry,rz to determine face
    */
   arx_ge_ary = LLVMBuildFCmp(bld->builder, LLVMRealUGE, arx, ary, "");
   arx_ge_arz = LLVMBuildFCmp(bld->builder, LLVMRealUGE, arx, arz, "");
   ary_ge_arx = LLVMBuildFCmp(bld->builder, LLVMRealUGE, ary, arx, "");
   ary_ge_arz = LLVMBuildFCmp(bld->builder, LLVMRealUGE, ary, arz, "");

   arx_ge_ary_arz = LLVMBuildAnd(bld->builder, arx_ge_ary, arx_ge_arz, "");
   ary_ge_arx_arz = LLVMBuildAnd(bld->builder, ary_ge_arx, ary_ge_arz, "");

   rx_pos = LLVMBuildFCmp(bld->builder, LLVMRealUGE, rx, float_bld->zero, "");
   ry_pos = LLVMBuildFCmp(bld->builder, LLVMRealUGE, ry, float_bld->zero, "");
   rz_pos = LLVMBuildFCmp(bld->builder, LLVMRealUGE, rz, float_bld->zero, "");

   {
      struct lp_build_flow_context *flow_ctx;
      struct lp_build_if_state if_ctx;

      flow_ctx = lp_build_flow_create(bld->builder);
      lp_build_flow_scope_begin(flow_ctx);

      *face_s = bld->coord_bld.undef;
      *face_t = bld->coord_bld.undef;
      *face = bld->int_bld.undef;

      lp_build_name(*face_s, "face_s");
      lp_build_name(*face_t, "face_t");
      lp_build_name(*face, "face");

      lp_build_flow_scope_declare(flow_ctx, face_s);
      lp_build_flow_scope_declare(flow_ctx, face_t);
      lp_build_flow_scope_declare(flow_ctx, face);

      lp_build_if(&if_ctx, flow_ctx, bld->builder, arx_ge_ary_arz);
      {
         /* +/- X face */
         LLVMValueRef sign = lp_build_sgn(float_bld, rx);
         LLVMValueRef ima = lp_build_cube_ima(coord_bld, s);
         *face_s = lp_build_cube_coord(coord_bld, sign, +1, r, ima);
         *face_t = lp_build_cube_coord(coord_bld, NULL, +1, t, ima);
         *face = lp_build_cube_face(bld, rx,
                                    PIPE_TEX_FACE_POS_X,
                                    PIPE_TEX_FACE_NEG_X);
      }
      lp_build_else(&if_ctx);
      {
         struct lp_build_flow_context *flow_ctx2;
         struct lp_build_if_state if_ctx2;

         LLVMValueRef face_s2 = bld->coord_bld.undef;
         LLVMValueRef face_t2 = bld->coord_bld.undef;
         LLVMValueRef face2 = bld->int_bld.undef;

         flow_ctx2 = lp_build_flow_create(bld->builder);
         lp_build_flow_scope_begin(flow_ctx2);
         lp_build_flow_scope_declare(flow_ctx2, &face_s2);
         lp_build_flow_scope_declare(flow_ctx2, &face_t2);
         lp_build_flow_scope_declare(flow_ctx2, &face2);

         ary_ge_arx_arz = LLVMBuildAnd(bld->builder, ary_ge_arx, ary_ge_arz, "");

         lp_build_if(&if_ctx2, flow_ctx2, bld->builder, ary_ge_arx_arz);
         {
            /* +/- Y face */
            LLVMValueRef sign = lp_build_sgn(float_bld, ry);
            LLVMValueRef ima = lp_build_cube_ima(coord_bld, t);
            face_s2 = lp_build_cube_coord(coord_bld, NULL, -1, s, ima);
            face_t2 = lp_build_cube_coord(coord_bld, sign, -1, r, ima);
            face2 = lp_build_cube_face(bld, ry,
                                       PIPE_TEX_FACE_POS_Y,
                                       PIPE_TEX_FACE_NEG_Y);
         }
         lp_build_else(&if_ctx2);
         {
            /* +/- Z face */
            LLVMValueRef sign = lp_build_sgn(float_bld, rz);
            LLVMValueRef ima = lp_build_cube_ima(coord_bld, r);
            face_s2 = lp_build_cube_coord(coord_bld, sign, -1, s, ima);
            face_t2 = lp_build_cube_coord(coord_bld, NULL, +1, t, ima);
            face2 = lp_build_cube_face(bld, rz,
                                       PIPE_TEX_FACE_POS_Z,
                                       PIPE_TEX_FACE_NEG_Z);
         }
         lp_build_endif(&if_ctx2);
         lp_build_flow_scope_end(flow_ctx2);
         lp_build_flow_destroy(flow_ctx2);
         *face_s = face_s2;
         *face_t = face_t2;
         *face = face2;
      }

      lp_build_endif(&if_ctx);
      lp_build_flow_scope_end(flow_ctx);
      lp_build_flow_destroy(flow_ctx);
   }
}



/**
 * Sample the texture/mipmap using given image filter and mip filter.
 * data0_ptr and data1_ptr point to the two mipmap levels to sample
 * from.  width0/1_vec, height0/1_vec, depth0/1_vec indicate their sizes.
 * If we're using nearest miplevel sampling the '1' values will be null/unused.
 */
static void
lp_build_sample_mipmap(struct lp_build_sample_context *bld,
                       unsigned img_filter,
                       unsigned mip_filter,
                       LLVMValueRef s,
                       LLVMValueRef t,
                       LLVMValueRef r,
                       LLVMValueRef lod_fpart,
                       LLVMValueRef width0_vec,
                       LLVMValueRef width1_vec,
                       LLVMValueRef height0_vec,
                       LLVMValueRef height1_vec,
                       LLVMValueRef depth0_vec,
                       LLVMValueRef depth1_vec,
                       LLVMValueRef row_stride0_vec,
                       LLVMValueRef row_stride1_vec,
                       LLVMValueRef img_stride0_vec,
                       LLVMValueRef img_stride1_vec,
                       LLVMValueRef data_ptr0,
                       LLVMValueRef data_ptr1,
                       LLVMValueRef *colors_out)
{
   LLVMValueRef colors0[4], colors1[4];
   int chan;

   if (img_filter == PIPE_TEX_FILTER_NEAREST) {
      /* sample the first mipmap level */
      lp_build_sample_image_nearest(bld,
                                    width0_vec, height0_vec, depth0_vec,
                                    row_stride0_vec, img_stride0_vec,
                                    data_ptr0, s, t, r, colors0);

      if (mip_filter == PIPE_TEX_MIPFILTER_LINEAR) {
         /* sample the second mipmap level */
         lp_build_sample_image_nearest(bld,
                                       width1_vec, height1_vec, depth1_vec,
                                       row_stride1_vec, img_stride1_vec,
                                       data_ptr1, s, t, r, colors1);
      }
   }
   else {
      assert(img_filter == PIPE_TEX_FILTER_LINEAR);

      /* sample the first mipmap level */
      lp_build_sample_image_linear(bld,
                                   width0_vec, height0_vec, depth0_vec,
                                   row_stride0_vec, img_stride0_vec,
                                   data_ptr0, s, t, r, colors0);

      if (mip_filter == PIPE_TEX_MIPFILTER_LINEAR) {
         /* sample the second mipmap level */
         lp_build_sample_image_linear(bld,
                                      width1_vec, height1_vec, depth1_vec,
                                      row_stride1_vec, img_stride1_vec,
                                      data_ptr1, s, t, r, colors1);
      }
   }

   if (mip_filter == PIPE_TEX_MIPFILTER_LINEAR) {
      /* interpolate samples from the two mipmap levels */
      for (chan = 0; chan < 4; chan++) {
         colors_out[chan] = lp_build_lerp(&bld->texel_bld, lod_fpart,
                                          colors0[chan], colors1[chan]);
      }
   }
   else {
      /* use first/only level's colors */
      for (chan = 0; chan < 4; chan++) {
         colors_out[chan] = colors0[chan];
      }
   }
}



/**
 * General texture sampling codegen.
 * This function handles texture sampling for all texture targets (1D,
 * 2D, 3D, cube) and all filtering modes.
 */
static void
lp_build_sample_general(struct lp_build_sample_context *bld,
                        unsigned unit,
                        LLVMValueRef s,
                        LLVMValueRef t,
                        LLVMValueRef r,
                        const LLVMValueRef *ddx,
                        const LLVMValueRef *ddy,
                        LLVMValueRef lod_bias, /* optional */
                        LLVMValueRef explicit_lod, /* optional */
                        LLVMValueRef width,
                        LLVMValueRef height,
                        LLVMValueRef depth,
                        LLVMValueRef width_vec,
                        LLVMValueRef height_vec,
                        LLVMValueRef depth_vec,
                        LLVMValueRef row_stride_array,
                        LLVMValueRef img_stride_array,
                        LLVMValueRef data_array,
                        LLVMValueRef *colors_out)
{
   struct lp_build_context *float_bld = &bld->float_bld;
   const unsigned mip_filter = bld->static_state->min_mip_filter;
   const unsigned min_filter = bld->static_state->min_img_filter;
   const unsigned mag_filter = bld->static_state->mag_img_filter;
   const int dims = texture_dims(bld->static_state->target);
   LLVMValueRef lod = NULL, lod_fpart = NULL;
   LLVMValueRef ilevel0, ilevel1 = NULL, ilevel0_vec, ilevel1_vec = NULL;
   LLVMValueRef width0_vec = NULL, height0_vec = NULL, depth0_vec = NULL;
   LLVMValueRef width1_vec = NULL, height1_vec = NULL, depth1_vec = NULL;
   LLVMValueRef row_stride0_vec = NULL, row_stride1_vec = NULL;
   LLVMValueRef img_stride0_vec = NULL, img_stride1_vec = NULL;
   LLVMValueRef data_ptr0, data_ptr1 = NULL;
   LLVMValueRef face_ddx[4], face_ddy[4];

   /*
   printf("%s mip %d  min %d  mag %d\n", __FUNCTION__,
          mip_filter, min_filter, mag_filter);
   */

   /*
    * Choose cube face, recompute texcoords and derivatives for the chosen face.
    */
   if (bld->static_state->target == PIPE_TEXTURE_CUBE) {
      LLVMValueRef face, face_s, face_t;
      lp_build_cube_lookup(bld, s, t, r, &face, &face_s, &face_t);
      s = face_s; /* vec */
      t = face_t; /* vec */
      /* use 'r' to indicate cube face */
      r = lp_build_broadcast_scalar(&bld->int_coord_bld, face); /* vec */

      /* recompute ddx, ddy using the new (s,t) face texcoords */
      face_ddx[0] = lp_build_ddx(&bld->coord_bld, s);
      face_ddx[1] = lp_build_ddx(&bld->coord_bld, t);
      face_ddx[2] = NULL;
      face_ddx[3] = NULL;
      face_ddy[0] = lp_build_ddy(&bld->coord_bld, s);
      face_ddy[1] = lp_build_ddy(&bld->coord_bld, t);
      face_ddy[2] = NULL;
      face_ddy[3] = NULL;
      ddx = face_ddx;
      ddy = face_ddy;
   }

   /*
    * Compute the level of detail (float).
    */
   if (min_filter != mag_filter ||
       mip_filter != PIPE_TEX_MIPFILTER_NONE) {
      /* Need to compute lod either to choose mipmap levels or to
       * distinguish between minification/magnification with one mipmap level.
       */
      lod = lp_build_lod_selector(bld, ddx, ddy,
                                  lod_bias, explicit_lod,
                                  width, height, depth);
   }

   /*
    * Compute integer mipmap level(s) to fetch texels from.
    */
   if (mip_filter == PIPE_TEX_MIPFILTER_NONE) {
      /* always use mip level 0 */
      if (bld->static_state->target == PIPE_TEXTURE_CUBE) {
         /* XXX this is a work-around for an apparent bug in LLVM 2.7.
          * We should be able to set ilevel0 = const(0) but that causes
          * bad x86 code to be emitted.
          */
         lod = lp_build_const_elem(bld->coord_bld.type, 0.0);
         lp_build_nearest_mip_level(bld, unit, lod, &ilevel0);
      }
      else {
         ilevel0 = LLVMConstInt(LLVMInt32Type(), 0, 0);
      }
   }
   else {
      assert(lod);
      if (mip_filter == PIPE_TEX_MIPFILTER_NEAREST) {
         lp_build_nearest_mip_level(bld, unit, lod, &ilevel0);
      }
      else {
         assert(mip_filter == PIPE_TEX_MIPFILTER_LINEAR);
         lp_build_linear_mip_levels(bld, unit, lod, &ilevel0, &ilevel1,
                                    &lod_fpart);
         lod_fpart = lp_build_broadcast_scalar(&bld->coord_bld, lod_fpart);
      }
   }

   /*
    * Convert scalar integer mipmap levels into vectors.
    */
   ilevel0_vec = lp_build_broadcast_scalar(&bld->int_coord_bld, ilevel0);
   if (mip_filter == PIPE_TEX_MIPFILTER_LINEAR)
      ilevel1_vec = lp_build_broadcast_scalar(&bld->int_coord_bld, ilevel1);

   /*
    * Compute width, height at mipmap level 'ilevel0'
    */
   width0_vec = lp_build_minify(bld, width_vec, ilevel0_vec);
   if (dims >= 2) {
      height0_vec = lp_build_minify(bld, height_vec, ilevel0_vec);
      row_stride0_vec = lp_build_get_level_stride_vec(bld, row_stride_array,
                                                      ilevel0);
      if (dims == 3 || bld->static_state->target == PIPE_TEXTURE_CUBE) {
         img_stride0_vec = lp_build_get_level_stride_vec(bld,
                                                         img_stride_array,
                                                         ilevel0);
         if (dims == 3) {
            depth0_vec = lp_build_minify(bld, depth_vec, ilevel0_vec);
         }
      }
   }
   if (mip_filter == PIPE_TEX_MIPFILTER_LINEAR) {
      /* compute width, height, depth for second mipmap level at 'ilevel1' */
      width1_vec = lp_build_minify(bld, width_vec, ilevel1_vec);
      if (dims >= 2) {
         height1_vec = lp_build_minify(bld, height_vec, ilevel1_vec);
         row_stride1_vec = lp_build_get_level_stride_vec(bld, row_stride_array,
                                                         ilevel1);
         if (dims == 3 || bld->static_state->target == PIPE_TEXTURE_CUBE) {
            img_stride1_vec = lp_build_get_level_stride_vec(bld,
                                                            img_stride_array,
                                                            ilevel1);
            if (dims ==3) {
               depth1_vec = lp_build_minify(bld, depth_vec, ilevel1_vec);
            }
         }
      }
   }

   /*
    * Get pointer(s) to image data for mipmap level(s).
    */
   data_ptr0 = lp_build_get_mipmap_level(bld, data_array, ilevel0);
   if (mip_filter == PIPE_TEX_MIPFILTER_LINEAR) {
      data_ptr1 = lp_build_get_mipmap_level(bld, data_array, ilevel1);
   }

   /*
    * Get/interpolate texture colors.
    */
   if (min_filter == mag_filter) {
      /* no need to distinquish between minification and magnification */
      lp_build_sample_mipmap(bld, min_filter, mip_filter, s, t, r, lod_fpart,
                             width0_vec, width1_vec,
                             height0_vec, height1_vec,
                             depth0_vec, depth1_vec,
                             row_stride0_vec, row_stride1_vec,
                             img_stride0_vec, img_stride1_vec,
                             data_ptr0, data_ptr1,
                             colors_out);
   }
   else {
      /* Emit conditional to choose min image filter or mag image filter
       * depending on the lod being >0 or <= 0, respectively.
       */
      struct lp_build_flow_context *flow_ctx;
      struct lp_build_if_state if_ctx;
      LLVMValueRef minify;

      flow_ctx = lp_build_flow_create(bld->builder);
      lp_build_flow_scope_begin(flow_ctx);

      lp_build_flow_scope_declare(flow_ctx, &colors_out[0]);
      lp_build_flow_scope_declare(flow_ctx, &colors_out[1]);
      lp_build_flow_scope_declare(flow_ctx, &colors_out[2]);
      lp_build_flow_scope_declare(flow_ctx, &colors_out[3]);

      /* minify = lod > 0.0 */
      minify = LLVMBuildFCmp(bld->builder, LLVMRealUGE,
                             lod, float_bld->zero, "");

      lp_build_if(&if_ctx, flow_ctx, bld->builder, minify);
      {
         /* Use the minification filter */
         lp_build_sample_mipmap(bld, min_filter, mip_filter,
                                s, t, r, lod_fpart,
                                width0_vec, width1_vec,
                                height0_vec, height1_vec,
                                depth0_vec, depth1_vec,
                                row_stride0_vec, row_stride1_vec,
                                img_stride0_vec, img_stride1_vec,
                                data_ptr0, data_ptr1,
                                colors_out);
      }
      lp_build_else(&if_ctx);
      {
         /* Use the magnification filter */
         lp_build_sample_mipmap(bld, mag_filter, mip_filter,
                                s, t, r, lod_fpart,
                                width0_vec, width1_vec,
                                height0_vec, height1_vec,
                                depth0_vec, depth1_vec,
                                row_stride0_vec, row_stride1_vec,
                                img_stride0_vec, img_stride1_vec,
                                data_ptr0, data_ptr1,
                                colors_out);
      }
      lp_build_endif(&if_ctx);

      lp_build_flow_scope_end(flow_ctx);
      lp_build_flow_destroy(flow_ctx);
   }
}



static void
lp_build_sample_2d_linear_aos(struct lp_build_sample_context *bld,
                              LLVMValueRef s,
                              LLVMValueRef t,
                              LLVMValueRef width,
                              LLVMValueRef height,
                              LLVMValueRef stride_array,
                              LLVMValueRef data_array,
                              LLVMValueRef texel_out[4])
{
   LLVMBuilderRef builder = bld->builder;
   struct lp_build_context i32, h16, u8n;
   LLVMTypeRef i32_vec_type, h16_vec_type, u8n_vec_type;
   LLVMValueRef i32_c8, i32_c128, i32_c255;
   LLVMValueRef s_ipart, s_fpart, s_fpart_lo, s_fpart_hi;
   LLVMValueRef t_ipart, t_fpart, t_fpart_lo, t_fpart_hi;
   LLVMValueRef data_ptr;
   LLVMValueRef x_stride, y_stride;
   LLVMValueRef x_offset0, x_offset1;
   LLVMValueRef y_offset0, y_offset1;
   LLVMValueRef offset[2][2];
   LLVMValueRef x_subcoord[2], y_subcoord[2];
   LLVMValueRef neighbors_lo[2][2];
   LLVMValueRef neighbors_hi[2][2];
   LLVMValueRef packed, packed_lo, packed_hi;
   LLVMValueRef unswizzled[4];
   const unsigned level = 0;
   unsigned i, j;

   assert(bld->static_state->target == PIPE_TEXTURE_2D
         || bld->static_state->target == PIPE_TEXTURE_RECT);
   assert(bld->static_state->min_img_filter == PIPE_TEX_FILTER_LINEAR);
   assert(bld->static_state->mag_img_filter == PIPE_TEX_FILTER_LINEAR);
   assert(bld->static_state->min_mip_filter == PIPE_TEX_MIPFILTER_NONE);

   lp_build_context_init(&i32, builder, lp_type_int_vec(32));
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
   i32_c128 = lp_build_const_int_vec(i32.type, -128);
   s = LLVMBuildAdd(builder, s, i32_c128, "");
   t = LLVMBuildAdd(builder, t, i32_c128, "");

   /* compute floor (shift right 8) */
   i32_c8 = lp_build_const_int_vec(i32.type, 8);
   s_ipart = LLVMBuildAShr(builder, s, i32_c8, "");
   t_ipart = LLVMBuildAShr(builder, t, i32_c8, "");

   /* compute fractional part (AND with 0xff) */
   i32_c255 = lp_build_const_int_vec(i32.type, 255);
   s_fpart = LLVMBuildAnd(builder, s, i32_c255, "");
   t_fpart = LLVMBuildAnd(builder, t, i32_c255, "");

   x_stride = lp_build_const_vec(bld->uint_coord_bld.type,
                                 bld->format_desc->block.bits/8);

   y_stride = lp_build_get_const_level_stride_vec(bld, stride_array, level);

   lp_build_sample_wrap_linear_int(bld,
                                   bld->format_desc->block.width,
                                   s_ipart, width, x_stride,
                                   bld->static_state->pot_width,
                                   bld->static_state->wrap_s,
                                   &x_offset0, &x_offset1,
                                   &x_subcoord[0], &x_subcoord[1]);
   lp_build_sample_wrap_linear_int(bld,
                                   bld->format_desc->block.height,
                                   t_ipart, height, y_stride,
                                   bld->static_state->pot_height,
                                   bld->static_state->wrap_t,
                                   &y_offset0, &y_offset1,
                                   &y_subcoord[0], &y_subcoord[1]);

   offset[0][0] = lp_build_add(&bld->uint_coord_bld, x_offset0, y_offset0);
   offset[0][1] = lp_build_add(&bld->uint_coord_bld, x_offset1, y_offset0);
   offset[1][0] = lp_build_add(&bld->uint_coord_bld, x_offset0, y_offset1);
   offset[1][1] = lp_build_add(&bld->uint_coord_bld, x_offset1, y_offset1);

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

      for(j = 0; j < h16.type.length; j += 4) {
#ifdef PIPE_ARCH_LITTLE_ENDIAN
         unsigned subindex = 0;
#else
         unsigned subindex = 1;
#endif
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
    * get pointer to mipmap level 0 data
    */
   data_ptr = lp_build_get_const_mipmap_level(bld, data_array, level);

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

   for (j = 0; j < 2; ++j) {
      for (i = 0; i < 2; ++i) {
         LLVMValueRef rgba8;

         if (util_format_is_rgba8_variant(bld->format_desc)) {
            /*
             * Given the format is a rgba8, just read the pixels as is,
             * without any swizzling. Swizzling will be done later.
             */
            rgba8 = lp_build_gather(bld->builder,
                                    bld->texel_type.length,
                                    bld->format_desc->block.bits,
                                    bld->texel_type.width,
                                    data_ptr, offset[j][i]);

            rgba8 = LLVMBuildBitCast(builder, rgba8, u8n_vec_type, "");

         }
         else {
            rgba8 = lp_build_fetch_rgba_aos(bld->builder,
                                            bld->format_desc,
                                            u8n.type,
                                            data_ptr, offset[j][i],
                                            x_subcoord[i],
                                            y_subcoord[j]);
         }

         lp_build_unpack2(builder, u8n.type, h16.type,
                          rgba8,
                          &neighbors_lo[j][i], &neighbors_hi[j][i]);
      }
   }

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

   lp_build_rgba8_to_f32_soa(bld->builder,
                             bld->texel_type,
                             packed, unswizzled);

   if (util_format_is_rgba8_variant(bld->format_desc)) {
      lp_build_format_swizzle_soa(bld->format_desc,
                                  &bld->texel_bld,
                                  unswizzled, texel_out);
   } else {
      texel_out[0] = unswizzled[0];
      texel_out[1] = unswizzled[1];
      texel_out[2] = unswizzled[2];
      texel_out[3] = unswizzled[3];
   }

   apply_sampler_swizzle(bld, texel_out);
}


static void
lp_build_sample_compare(struct lp_build_sample_context *bld,
                        LLVMValueRef p,
                        LLVMValueRef texel[4])
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
   res = lp_build_mul(texel_bld, res, lp_build_const_vec(texel_bld->type, 0.25));

   /* XXX returning result for default GL_DEPTH_TEXTURE_MODE = GL_LUMINANCE */
   for(chan = 0; chan < 3; ++chan)
      texel[chan] = res;
   texel[3] = texel_bld->one;
}


/**
 * Just set texels to white instead of actually sampling the texture.
 * For debugging.
 */
static void
lp_build_sample_nop(struct lp_build_sample_context *bld,
                    LLVMValueRef texel_out[4])
{
   struct lp_build_context *texel_bld = &bld->texel_bld;
   unsigned chan;

   for (chan = 0; chan < 4; chan++) {
      /*lp_bld_mov(texel_bld, texel, texel_bld->one);*/
      texel_out[chan] = texel_bld->one;
   }  
}


/**
 * Build texture sampling code.
 * 'texel' will return a vector of four LLVMValueRefs corresponding to
 * R, G, B, A.
 * \param type  vector float type to use for coords, etc.
 * \param ddx  partial derivatives of (s,t,r,q) with respect to x
 * \param ddy  partial derivatives of (s,t,r,q) with respect to y
 */
void
lp_build_sample_soa(LLVMBuilderRef builder,
                    const struct lp_sampler_static_state *static_state,
                    struct lp_sampler_dynamic_state *dynamic_state,
                    struct lp_type type,
                    unsigned unit,
                    unsigned num_coords,
                    const LLVMValueRef *coords,
                    const LLVMValueRef ddx[4],
                    const LLVMValueRef ddy[4],
                    LLVMValueRef lod_bias, /* optional */
                    LLVMValueRef explicit_lod, /* optional */
                    LLVMValueRef texel_out[4])
{
   struct lp_build_sample_context bld;
   LLVMValueRef width, width_vec;
   LLVMValueRef height, height_vec;
   LLVMValueRef depth, depth_vec;
   LLVMValueRef row_stride_array, img_stride_array;
   LLVMValueRef data_array;
   LLVMValueRef s;
   LLVMValueRef t;
   LLVMValueRef r;

   if (0) {
      enum pipe_format fmt = static_state->format;
      debug_printf("Sample from %s\n", util_format_name(fmt));
   }

   assert(type.floating);

   /* Setup our build context */
   memset(&bld, 0, sizeof bld);
   bld.builder = builder;
   bld.static_state = static_state;
   bld.dynamic_state = dynamic_state;
   bld.format_desc = util_format_description(static_state->format);

   bld.float_type = lp_type_float(32);
   bld.int_type = lp_type_int(32);
   bld.coord_type = type;
   bld.uint_coord_type = lp_uint_type(type);
   bld.int_coord_type = lp_int_type(type);
   bld.texel_type = type;

   lp_build_context_init(&bld.float_bld, builder, bld.float_type);
   lp_build_context_init(&bld.int_bld, builder, bld.int_type);
   lp_build_context_init(&bld.coord_bld, builder, bld.coord_type);
   lp_build_context_init(&bld.uint_coord_bld, builder, bld.uint_coord_type);
   lp_build_context_init(&bld.int_coord_bld, builder, bld.int_coord_type);
   lp_build_context_init(&bld.texel_bld, builder, bld.texel_type);

   /* Get the dynamic state */
   width = dynamic_state->width(dynamic_state, builder, unit);
   height = dynamic_state->height(dynamic_state, builder, unit);
   depth = dynamic_state->depth(dynamic_state, builder, unit);
   row_stride_array = dynamic_state->row_stride(dynamic_state, builder, unit);
   img_stride_array = dynamic_state->img_stride(dynamic_state, builder, unit);
   data_array = dynamic_state->data_ptr(dynamic_state, builder, unit);
   /* Note that data_array is an array[level] of pointers to texture images */

   s = coords[0];
   t = coords[1];
   r = coords[2];

   width_vec = lp_build_broadcast_scalar(&bld.uint_coord_bld, width);
   height_vec = lp_build_broadcast_scalar(&bld.uint_coord_bld, height);
   depth_vec = lp_build_broadcast_scalar(&bld.uint_coord_bld, depth);

   if (0) {
      /* For debug: no-op texture sampling */
      lp_build_sample_nop(&bld, texel_out);
   }
   else if (util_format_fits_8unorm(bld.format_desc) &&
            (static_state->target == PIPE_TEXTURE_2D ||
             static_state->target == PIPE_TEXTURE_RECT) &&
            static_state->min_img_filter == PIPE_TEX_FILTER_LINEAR &&
            static_state->mag_img_filter == PIPE_TEX_FILTER_LINEAR &&
            static_state->min_mip_filter == PIPE_TEX_MIPFILTER_NONE &&
            is_simple_wrap_mode(static_state->wrap_s) &&
            is_simple_wrap_mode(static_state->wrap_t)) {
      /* special case */
      lp_build_sample_2d_linear_aos(&bld, s, t, width_vec, height_vec,
                                    row_stride_array, data_array, texel_out);
   }
   else {
      if (gallivm_debug & GALLIVM_DEBUG_PERF &&
          (static_state->min_img_filter != PIPE_TEX_FILTER_NEAREST ||
           static_state->mag_img_filter != PIPE_TEX_FILTER_NEAREST ||
           static_state->min_mip_filter == PIPE_TEX_MIPFILTER_LINEAR) &&
          util_format_fits_8unorm(bld.format_desc)) {
         debug_printf("%s: using floating point linear filtering for %s\n",
                      __FUNCTION__, bld.format_desc->short_name);
      }

      lp_build_sample_general(&bld, unit, s, t, r, ddx, ddy,
                              lod_bias, explicit_lod,
                              width, height, depth,
                              width_vec, height_vec, depth_vec,
                              row_stride_array, img_stride_array,
                              data_array,
                              texel_out);
   }

   lp_build_sample_compare(&bld, r, texel_out);
}
