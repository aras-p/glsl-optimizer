/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.
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
#include "lp_bld_bitarit.h"
#include "lp_bld_logic.h"
#include "lp_bld_swizzle.h"
#include "lp_bld_pack.h"
#include "lp_bld_flow.h"
#include "lp_bld_gather.h"
#include "lp_bld_format.h"
#include "lp_bld_init.h"
#include "lp_bld_sample.h"
#include "lp_bld_sample_aos.h"
#include "lp_bld_quad.h"


/**
 * Build LLVM code for texture coord wrapping, for nearest filtering,
 * for scaled integer texcoords.
 * \param block_length  is the length of the pixel block along the
 *                      coordinate axis
 * \param coord  the incoming texcoord (s,t,r or q) scaled to the texture size
 * \param length  the texture size along one dimension
 * \param stride  pixel stride along the coordinate axis (in bytes)
 * \param is_pot  if TRUE, length is a power of two
 * \param wrap_mode  one of PIPE_TEX_WRAP_x
 * \param out_offset  byte offset for the wrapped coordinate
 * \param out_i  resulting sub-block pixel coordinate for coord0
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
   struct lp_build_context *int_coord_bld = &bld->int_coord_bld;
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef length_minus_one;

   length_minus_one = lp_build_sub(int_coord_bld, length, int_coord_bld->one);

   switch(wrap_mode) {
   case PIPE_TEX_WRAP_REPEAT:
      if(is_pot)
         coord = LLVMBuildAnd(builder, coord, length_minus_one, "");
      else {
         /* Add a bias to the texcoord to handle negative coords */
         LLVMValueRef bias = lp_build_mul_imm(int_coord_bld, length, 1024);
         coord = LLVMBuildAdd(builder, coord, bias, "");
         coord = LLVMBuildURem(builder, coord, length, "");
      }
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

   lp_build_sample_partial_offset(int_coord_bld, block_length, coord, stride,
                                  out_offset, out_i);
}


/**
 * Build LLVM code for texture coord wrapping, for linear filtering,
 * for scaled integer texcoords.
 * \param block_length  is the length of the pixel block along the
 *                      coordinate axis
 * \param coord0  the incoming texcoord (s,t,r or q) scaled to the texture size
 * \param length  the texture size along one dimension
 * \param stride  pixel stride along the coordinate axis (in bytes)
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
   struct lp_build_context *int_coord_bld = &bld->int_coord_bld;
   LLVMBuilderRef builder = bld->gallivm->builder;
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

   *i0 = int_coord_bld->zero;
   *i1 = int_coord_bld->zero;

   length_minus_one = lp_build_sub(int_coord_bld, length, int_coord_bld->one);

   switch(wrap_mode) {
   case PIPE_TEX_WRAP_REPEAT:
      if (is_pot) {
         coord0 = LLVMBuildAnd(builder, coord0, length_minus_one, "");
      }
      else {
         /* Add a bias to the texcoord to handle negative coords */
         LLVMValueRef bias = lp_build_mul_imm(int_coord_bld, length, 1024);
         coord0 = LLVMBuildAdd(builder, coord0, bias, "");
         coord0 = LLVMBuildURem(builder, coord0, length, "");
      }

      mask = lp_build_compare(bld->gallivm, int_coord_bld->type,
                              PIPE_FUNC_NOTEQUAL, coord0, length_minus_one);

      *offset0 = lp_build_mul(int_coord_bld, coord0, stride);
      *offset1 = LLVMBuildAnd(builder,
                              lp_build_add(int_coord_bld, *offset0, stride),
                              mask, "");
      break;

   case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
      lmask = lp_build_compare(int_coord_bld->gallivm, int_coord_bld->type,
                               PIPE_FUNC_GEQUAL, coord0, int_coord_bld->zero);
      umask = lp_build_compare(int_coord_bld->gallivm, int_coord_bld->type,
                               PIPE_FUNC_LESS, coord0, length_minus_one);

      coord0 = lp_build_select(int_coord_bld, lmask, coord0, int_coord_bld->zero);
      coord0 = lp_build_select(int_coord_bld, umask, coord0, length_minus_one);

      mask = LLVMBuildAnd(builder, lmask, umask, "");

      *offset0 = lp_build_mul(int_coord_bld, coord0, stride);
      *offset1 = lp_build_add(int_coord_bld,
                              *offset0,
                              LLVMBuildAnd(builder, stride, mask, ""));
      break;

   case PIPE_TEX_WRAP_CLAMP:
   case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
   case PIPE_TEX_WRAP_MIRROR_REPEAT:
   case PIPE_TEX_WRAP_MIRROR_CLAMP:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
   case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
   default:
      assert(0);
      *offset0 = int_coord_bld->zero;
      *offset1 = int_coord_bld->zero;
      break;
   }
}


/**
 * Sample a single texture image with nearest sampling.
 * If sampling a cube texture, r = cube face in [0,5].
 * Return filtered color as two vectors of 16-bit fixed point values.
 */
static void
lp_build_sample_image_nearest(struct lp_build_sample_context *bld,
                              LLVMValueRef int_size,
                              LLVMValueRef row_stride_vec,
                              LLVMValueRef img_stride_vec,
                              LLVMValueRef data_ptr,
                              LLVMValueRef s,
                              LLVMValueRef t,
                              LLVMValueRef r,
                              LLVMValueRef *colors_lo,
                              LLVMValueRef *colors_hi)
{
   const unsigned dims = bld->dims;
   LLVMBuilderRef builder = bld->gallivm->builder;
   struct lp_build_context i32, h16, u8n;
   LLVMTypeRef i32_vec_type, h16_vec_type, u8n_vec_type;
   LLVMValueRef i32_c8;
   LLVMValueRef width_vec, height_vec, depth_vec;
   LLVMValueRef s_ipart, t_ipart = NULL, r_ipart = NULL;
   LLVMValueRef x_stride;
   LLVMValueRef x_offset, offset;
   LLVMValueRef x_subcoord, y_subcoord, z_subcoord;

   lp_build_context_init(&i32, bld->gallivm, lp_type_int_vec(32));
   lp_build_context_init(&h16, bld->gallivm, lp_type_ufixed(16));
   lp_build_context_init(&u8n, bld->gallivm, lp_type_unorm(8));

   i32_vec_type = lp_build_vec_type(bld->gallivm, i32.type);
   h16_vec_type = lp_build_vec_type(bld->gallivm, h16.type);
   u8n_vec_type = lp_build_vec_type(bld->gallivm, u8n.type);

   lp_build_extract_image_sizes(bld,
                                bld->int_size_type,
                                bld->int_coord_type,
                                int_size,
                                &width_vec,
                                &height_vec,
                                &depth_vec);

   if (bld->static_state->normalized_coords) {
      LLVMValueRef scaled_size;
      LLVMValueRef flt_size;

      /* scale size by 256 (8 fractional bits) */
      scaled_size = lp_build_shl_imm(&bld->int_size_bld, int_size, 8);

      flt_size = lp_build_int_to_float(&bld->float_size_bld, scaled_size);

      lp_build_unnormalized_coords(bld, flt_size, &s, &t, &r);
   }
   else {
      /* scale coords by 256 (8 fractional bits) */
      s = lp_build_mul_imm(&bld->coord_bld, s, 256);
      if (dims >= 2)
         t = lp_build_mul_imm(&bld->coord_bld, t, 256);
      if (dims >= 3)
         r = lp_build_mul_imm(&bld->coord_bld, r, 256);
   }

   /* convert float to int */
   s = LLVMBuildFPToSI(builder, s, i32_vec_type, "");
   if (dims >= 2)
      t = LLVMBuildFPToSI(builder, t, i32_vec_type, "");
   if (dims >= 3)
      r = LLVMBuildFPToSI(builder, r, i32_vec_type, "");

   /* compute floor (shift right 8) */
   i32_c8 = lp_build_const_int_vec(bld->gallivm, i32.type, 8);
   s_ipart = LLVMBuildAShr(builder, s, i32_c8, "");
   if (dims >= 2)
      t_ipart = LLVMBuildAShr(builder, t, i32_c8, "");
   if (dims >= 3)
      r_ipart = LLVMBuildAShr(builder, r, i32_c8, "");

   /* get pixel, row, image strides */
   x_stride = lp_build_const_vec(bld->gallivm,
                                 bld->int_coord_bld.type,
                                 bld->format_desc->block.bits/8);

   /* Do texcoord wrapping, compute texel offset */
   lp_build_sample_wrap_nearest_int(bld,
                                    bld->format_desc->block.width,
                                    s_ipart, width_vec, x_stride,
                                    bld->static_state->pot_width,
                                    bld->static_state->wrap_s,
                                    &x_offset, &x_subcoord);
   offset = x_offset;
   if (dims >= 2) {
      LLVMValueRef y_offset;
      lp_build_sample_wrap_nearest_int(bld,
                                       bld->format_desc->block.height,
                                       t_ipart, height_vec, row_stride_vec,
                                       bld->static_state->pot_height,
                                       bld->static_state->wrap_t,
                                       &y_offset, &y_subcoord);
      offset = lp_build_add(&bld->int_coord_bld, offset, y_offset);
      if (dims >= 3) {
         LLVMValueRef z_offset;
         lp_build_sample_wrap_nearest_int(bld,
                                          1, /* block length (depth) */
                                          r_ipart, depth_vec, img_stride_vec,
                                          bld->static_state->pot_height,
                                          bld->static_state->wrap_r,
                                          &z_offset, &z_subcoord);
         offset = lp_build_add(&bld->int_coord_bld, offset, z_offset);
      }
      else if (bld->static_state->target == PIPE_TEXTURE_CUBE) {
         LLVMValueRef z_offset;
         /* The r coord is the cube face in [0,5] */
         z_offset = lp_build_mul(&bld->int_coord_bld, r, img_stride_vec);
         offset = lp_build_add(&bld->int_coord_bld, offset, z_offset);
      }
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
   {
      LLVMValueRef rgba8;

      if (util_format_is_rgba8_variant(bld->format_desc)) {
         /*
          * Given the format is a rgba8, just read the pixels as is,
          * without any swizzling. Swizzling will be done later.
          */
         rgba8 = lp_build_gather(bld->gallivm,
                                 bld->texel_type.length,
                                 bld->format_desc->block.bits,
                                 bld->texel_type.width,
                                 data_ptr, offset);

         rgba8 = LLVMBuildBitCast(builder, rgba8, u8n_vec_type, "");
      }
      else {
         rgba8 = lp_build_fetch_rgba_aos(bld->gallivm,
                                         bld->format_desc,
                                         u8n.type,
                                         data_ptr, offset,
                                         x_subcoord,
                                         y_subcoord);
      }

      /* Expand one 4*rgba8 to two 2*rgba16 */
      lp_build_unpack2(bld->gallivm, u8n.type, h16.type,
                       rgba8,
                       colors_lo, colors_hi);
   }
}


/**
 * Sample a single texture image with (bi-)(tri-)linear sampling.
 * Return filtered color as two vectors of 16-bit fixed point values.
 */
static void
lp_build_sample_image_linear(struct lp_build_sample_context *bld,
                             LLVMValueRef int_size,
                             LLVMValueRef row_stride_vec,
                             LLVMValueRef img_stride_vec,
                             LLVMValueRef data_ptr,
                             LLVMValueRef s,
                             LLVMValueRef t,
                             LLVMValueRef r,
                             LLVMValueRef *colors_lo,
                             LLVMValueRef *colors_hi)
{
   const unsigned dims = bld->dims;
   LLVMBuilderRef builder = bld->gallivm->builder;
   struct lp_build_context i32, h16, u8n;
   LLVMTypeRef i32_vec_type, h16_vec_type, u8n_vec_type;
   LLVMValueRef i32_c8, i32_c128, i32_c255;
   LLVMValueRef width_vec, height_vec, depth_vec;
   LLVMValueRef s_ipart, s_fpart, s_fpart_lo, s_fpart_hi;
   LLVMValueRef t_ipart = NULL, t_fpart = NULL, t_fpart_lo = NULL, t_fpart_hi = NULL;
   LLVMValueRef r_ipart = NULL, r_fpart = NULL, r_fpart_lo = NULL, r_fpart_hi = NULL;
   LLVMValueRef x_stride, y_stride, z_stride;
   LLVMValueRef x_offset0, x_offset1;
   LLVMValueRef y_offset0, y_offset1;
   LLVMValueRef z_offset0, z_offset1;
   LLVMValueRef offset[2][2][2]; /* [z][y][x] */
   LLVMValueRef x_subcoord[2], y_subcoord[2], z_subcoord[2];
   LLVMValueRef neighbors_lo[2][2][2]; /* [z][y][x] */
   LLVMValueRef neighbors_hi[2][2][2]; /* [z][y][x] */
   LLVMValueRef packed_lo, packed_hi;
   unsigned x, y, z;
   unsigned i, j, k;
   unsigned numj, numk;

   lp_build_context_init(&i32, bld->gallivm, lp_type_int_vec(32));
   lp_build_context_init(&h16, bld->gallivm, lp_type_ufixed(16));
   lp_build_context_init(&u8n, bld->gallivm, lp_type_unorm(8));

   i32_vec_type = lp_build_vec_type(bld->gallivm, i32.type);
   h16_vec_type = lp_build_vec_type(bld->gallivm, h16.type);
   u8n_vec_type = lp_build_vec_type(bld->gallivm, u8n.type);

   lp_build_extract_image_sizes(bld,
                                bld->int_size_type,
                                bld->int_coord_type,
                                int_size,
                                &width_vec,
                                &height_vec,
                                &depth_vec);

   if (bld->static_state->normalized_coords) {
      LLVMValueRef scaled_size;
      LLVMValueRef flt_size;

      /* scale size by 256 (8 fractional bits) */
      scaled_size = lp_build_shl_imm(&bld->int_size_bld, int_size, 8);

      flt_size = lp_build_int_to_float(&bld->float_size_bld, scaled_size);

      lp_build_unnormalized_coords(bld, flt_size, &s, &t, &r);
   }
   else {
      /* scale coords by 256 (8 fractional bits) */
      s = lp_build_mul_imm(&bld->coord_bld, s, 256);
      if (dims >= 2)
         t = lp_build_mul_imm(&bld->coord_bld, t, 256);
      if (dims >= 3)
         r = lp_build_mul_imm(&bld->coord_bld, r, 256);
   }

   /* convert float to int */
   s = LLVMBuildFPToSI(builder, s, i32_vec_type, "");
   if (dims >= 2)
      t = LLVMBuildFPToSI(builder, t, i32_vec_type, "");
   if (dims >= 3)
      r = LLVMBuildFPToSI(builder, r, i32_vec_type, "");

   /* subtract 0.5 (add -128) */
   i32_c128 = lp_build_const_int_vec(bld->gallivm, i32.type, -128);
   s = LLVMBuildAdd(builder, s, i32_c128, "");
   if (dims >= 2) {
      t = LLVMBuildAdd(builder, t, i32_c128, "");
   }
   if (dims >= 3) {
      r = LLVMBuildAdd(builder, r, i32_c128, "");
   }

   /* compute floor (shift right 8) */
   i32_c8 = lp_build_const_int_vec(bld->gallivm, i32.type, 8);
   s_ipart = LLVMBuildAShr(builder, s, i32_c8, "");
   if (dims >= 2)
      t_ipart = LLVMBuildAShr(builder, t, i32_c8, "");
   if (dims >= 3)
      r_ipart = LLVMBuildAShr(builder, r, i32_c8, "");

   /* compute fractional part (AND with 0xff) */
   i32_c255 = lp_build_const_int_vec(bld->gallivm, i32.type, 255);
   s_fpart = LLVMBuildAnd(builder, s, i32_c255, "");
   if (dims >= 2)
      t_fpart = LLVMBuildAnd(builder, t, i32_c255, "");
   if (dims >= 3)
      r_fpart = LLVMBuildAnd(builder, r, i32_c255, "");

   /* get pixel, row and image strides */
   x_stride = lp_build_const_vec(bld->gallivm, bld->int_coord_bld.type,
                                 bld->format_desc->block.bits/8);
   y_stride = row_stride_vec;
   z_stride = img_stride_vec;

   /* do texcoord wrapping and compute texel offsets */
   lp_build_sample_wrap_linear_int(bld,
                                   bld->format_desc->block.width,
                                   s_ipart, width_vec, x_stride,
                                   bld->static_state->pot_width,
                                   bld->static_state->wrap_s,
                                   &x_offset0, &x_offset1,
                                   &x_subcoord[0], &x_subcoord[1]);
   for (z = 0; z < 2; z++) {
      for (y = 0; y < 2; y++) {
         offset[z][y][0] = x_offset0;
         offset[z][y][1] = x_offset1;
      }
   }

   if (dims >= 2) {
      lp_build_sample_wrap_linear_int(bld,
                                      bld->format_desc->block.height,
                                      t_ipart, height_vec, y_stride,
                                      bld->static_state->pot_height,
                                      bld->static_state->wrap_t,
                                      &y_offset0, &y_offset1,
                                      &y_subcoord[0], &y_subcoord[1]);

      for (z = 0; z < 2; z++) {
         for (x = 0; x < 2; x++) {
            offset[z][0][x] = lp_build_add(&bld->int_coord_bld,
                                           offset[z][0][x], y_offset0);
            offset[z][1][x] = lp_build_add(&bld->int_coord_bld,
                                           offset[z][1][x], y_offset1);
         }
      }
   }

   if (dims >= 3) {
      lp_build_sample_wrap_linear_int(bld,
                                      bld->format_desc->block.height,
                                      r_ipart, depth_vec, z_stride,
                                      bld->static_state->pot_depth,
                                      bld->static_state->wrap_r,
                                      &z_offset0, &z_offset1,
                                      &z_subcoord[0], &z_subcoord[1]);
      for (y = 0; y < 2; y++) {
         for (x = 0; x < 2; x++) {
            offset[0][y][x] = lp_build_add(&bld->int_coord_bld,
                                           offset[0][y][x], z_offset0);
            offset[1][y][x] = lp_build_add(&bld->int_coord_bld,
                                           offset[1][y][x], z_offset1);
         }
      }
   }
   else if (bld->static_state->target == PIPE_TEXTURE_CUBE) {
      LLVMValueRef z_offset;
      z_offset = lp_build_mul(&bld->int_coord_bld, r, img_stride_vec);
      for (y = 0; y < 2; y++) {
         for (x = 0; x < 2; x++) {
            /* The r coord is the cube face in [0,5] */
            offset[0][y][x] = lp_build_add(&bld->int_coord_bld,
                                           offset[0][y][x], z_offset);
         }
      }
   }

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
   if (dims >= 2)
      t_fpart = LLVMBuildBitCast(builder, t_fpart, h16_vec_type, "");
   if (dims >= 3)
      r_fpart = LLVMBuildBitCast(builder, r_fpart, h16_vec_type, "");

   {
      LLVMTypeRef elem_type = LLVMInt32TypeInContext(bld->gallivm->context);
      LLVMValueRef shuffles_lo[LP_MAX_VECTOR_LENGTH];
      LLVMValueRef shuffles_hi[LP_MAX_VECTOR_LENGTH];
      LLVMValueRef shuffle_lo;
      LLVMValueRef shuffle_hi;

      for (j = 0; j < h16.type.length; j += 4) {
#ifdef PIPE_ARCH_LITTLE_ENDIAN
         unsigned subindex = 0;
#else
         unsigned subindex = 1;
#endif
         LLVMValueRef index;

         index = LLVMConstInt(elem_type, j/2 + subindex, 0);
         for (i = 0; i < 4; ++i)
            shuffles_lo[j + i] = index;

         index = LLVMConstInt(elem_type, h16.type.length/2 + j/2 + subindex, 0);
         for (i = 0; i < 4; ++i)
            shuffles_hi[j + i] = index;
      }

      shuffle_lo = LLVMConstVector(shuffles_lo, h16.type.length);
      shuffle_hi = LLVMConstVector(shuffles_hi, h16.type.length);

      s_fpart_lo = LLVMBuildShuffleVector(builder, s_fpart, h16.undef,
                                          shuffle_lo, "");
      s_fpart_hi = LLVMBuildShuffleVector(builder, s_fpart, h16.undef,
                                          shuffle_hi, "");
      if (dims >= 2) {
         t_fpart_lo = LLVMBuildShuffleVector(builder, t_fpart, h16.undef,
                                             shuffle_lo, "");
         t_fpart_hi = LLVMBuildShuffleVector(builder, t_fpart, h16.undef,
                                             shuffle_hi, "");
      }
      if (dims >= 3) {
         r_fpart_lo = LLVMBuildShuffleVector(builder, r_fpart, h16.undef,
                                             shuffle_lo, "");
         r_fpart_hi = LLVMBuildShuffleVector(builder, r_fpart, h16.undef,
                                             shuffle_hi, "");
      }
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
   numj = 1 + (dims >= 2);
   numk = 1 + (dims >= 3);

   for (k = 0; k < numk; k++) {
      for (j = 0; j < numj; j++) {
         for (i = 0; i < 2; i++) {
            LLVMValueRef rgba8;

            if (util_format_is_rgba8_variant(bld->format_desc)) {
               /*
                * Given the format is a rgba8, just read the pixels as is,
                * without any swizzling. Swizzling will be done later.
                */
               rgba8 = lp_build_gather(bld->gallivm,
                                       bld->texel_type.length,
                                       bld->format_desc->block.bits,
                                       bld->texel_type.width,
                                       data_ptr, offset[k][j][i]);

               rgba8 = LLVMBuildBitCast(builder, rgba8, u8n_vec_type, "");
            }
            else {
               rgba8 = lp_build_fetch_rgba_aos(bld->gallivm,
                                               bld->format_desc,
                                               u8n.type,
                                               data_ptr, offset[k][j][i],
                                               x_subcoord[i],
                                               y_subcoord[j]);
            }

            /* Expand one 4*rgba8 to two 2*rgba16 */
            lp_build_unpack2(bld->gallivm, u8n.type, h16.type,
                             rgba8,
                             &neighbors_lo[k][j][i], &neighbors_hi[k][j][i]);
         }
      }
   }

   /*
    * Linear interpolation with 8.8 fixed point.
    */
   if (dims == 1) {
      /* 1-D lerp */
      packed_lo = lp_build_lerp(&h16,
				s_fpart_lo,
				neighbors_lo[0][0][0],
				neighbors_lo[0][0][1]);

      packed_hi = lp_build_lerp(&h16,
				s_fpart_hi,
				neighbors_hi[0][0][0],
				neighbors_hi[0][0][1]);
   }
   else {
      /* 2-D lerp */
      packed_lo = lp_build_lerp_2d(&h16,
				   s_fpart_lo, t_fpart_lo,
				   neighbors_lo[0][0][0],
				   neighbors_lo[0][0][1],
				   neighbors_lo[0][1][0],
				   neighbors_lo[0][1][1]);

      packed_hi = lp_build_lerp_2d(&h16,
				   s_fpart_hi, t_fpart_hi,
				   neighbors_hi[0][0][0],
				   neighbors_hi[0][0][1],
				   neighbors_hi[0][1][0],
				   neighbors_hi[0][1][1]);

      if (dims >= 3) {
	 LLVMValueRef packed_lo2, packed_hi2;

	 /* lerp in the second z slice */
	 packed_lo2 = lp_build_lerp_2d(&h16,
				       s_fpart_lo, t_fpart_lo,
				       neighbors_lo[1][0][0],
				       neighbors_lo[1][0][1],
				       neighbors_lo[1][1][0],
				       neighbors_lo[1][1][1]);

	 packed_hi2 = lp_build_lerp_2d(&h16,
				       s_fpart_hi, t_fpart_hi,
				       neighbors_hi[1][0][0],
				       neighbors_hi[1][0][1],
				       neighbors_hi[1][1][0],
				       neighbors_hi[1][1][1]);
	 /* interp between two z slices */
	 packed_lo = lp_build_lerp(&h16, r_fpart_lo,
				   packed_lo, packed_lo2);
	 packed_hi = lp_build_lerp(&h16, r_fpart_hi,
				   packed_hi, packed_hi2);
      }
   }

   *colors_lo = packed_lo;
   *colors_hi = packed_hi;
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
                       LLVMValueRef ilevel0,
                       LLVMValueRef ilevel1,
                       LLVMValueRef lod_fpart,
                       LLVMValueRef colors_lo_var,
                       LLVMValueRef colors_hi_var)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef size0;
   LLVMValueRef size1;
   LLVMValueRef row_stride0_vec;
   LLVMValueRef row_stride1_vec;
   LLVMValueRef img_stride0_vec;
   LLVMValueRef img_stride1_vec;
   LLVMValueRef data_ptr0;
   LLVMValueRef data_ptr1;
   LLVMValueRef colors0_lo, colors0_hi;
   LLVMValueRef colors1_lo, colors1_hi;

   /* sample the first mipmap level */
   lp_build_mipmap_level_sizes(bld, ilevel0,
                               &size0,
                               &row_stride0_vec, &img_stride0_vec);
   data_ptr0 = lp_build_get_mipmap_level(bld, ilevel0);
   if (img_filter == PIPE_TEX_FILTER_NEAREST) {
      lp_build_sample_image_nearest(bld,
                                    size0,
                                    row_stride0_vec, img_stride0_vec,
                                    data_ptr0, s, t, r,
                                    &colors0_lo, &colors0_hi);
   }
   else {
      assert(img_filter == PIPE_TEX_FILTER_LINEAR);
      lp_build_sample_image_linear(bld,
                                   size0,
                                   row_stride0_vec, img_stride0_vec,
                                   data_ptr0, s, t, r,
                                   &colors0_lo, &colors0_hi);
   }

   /* Store the first level's colors in the output variables */
   LLVMBuildStore(builder, colors0_lo, colors_lo_var);
   LLVMBuildStore(builder, colors0_hi, colors_hi_var);

   if (mip_filter == PIPE_TEX_MIPFILTER_LINEAR) {
      LLVMValueRef h16_scale = lp_build_const_float(bld->gallivm, 256.0);
      LLVMTypeRef i32_type = LLVMIntTypeInContext(bld->gallivm->context, 32);
      struct lp_build_if_state if_ctx;
      LLVMValueRef need_lerp;

      lod_fpart = LLVMBuildFMul(builder, lod_fpart, h16_scale, "");
      lod_fpart = LLVMBuildFPToSI(builder, lod_fpart, i32_type, "lod_fpart.fixed16");

      /* need_lerp = lod_fpart > 0 */
      need_lerp = LLVMBuildICmp(builder, LLVMIntSGT,
                                lod_fpart, LLVMConstNull(i32_type),
                                "need_lerp");

      lp_build_if(&if_ctx, bld->gallivm, need_lerp);
      {
         struct lp_build_context h16_bld;

         lp_build_context_init(&h16_bld, bld->gallivm, lp_type_ufixed(16));

         /* sample the second mipmap level */
         lp_build_mipmap_level_sizes(bld, ilevel1,
                                     &size1,
                                     &row_stride1_vec, &img_stride1_vec);
         data_ptr1 = lp_build_get_mipmap_level(bld, ilevel1);
         if (img_filter == PIPE_TEX_FILTER_NEAREST) {
            lp_build_sample_image_nearest(bld,
                                          size1,
                                          row_stride1_vec, img_stride1_vec,
                                          data_ptr1, s, t, r,
                                          &colors1_lo, &colors1_hi);
         }
         else {
            lp_build_sample_image_linear(bld,
                                         size1,
                                         row_stride1_vec, img_stride1_vec,
                                         data_ptr1, s, t, r,
                                         &colors1_lo, &colors1_hi);
         }

         /* interpolate samples from the two mipmap levels */

         lod_fpart = LLVMBuildTrunc(builder, lod_fpart, h16_bld.elem_type, "");
         lod_fpart = lp_build_broadcast_scalar(&h16_bld, lod_fpart);

#if HAVE_LLVM == 0x208
         /* This is a work-around for a bug in LLVM 2.8.
          * Evidently, something goes wrong in the construction of the
          * lod_fpart short[8] vector.  Adding this no-effect shuffle seems
          * to force the vector to be properly constructed.
          * Tested with mesa-demos/src/tests/mipmap_limits.c (press t, f).
          */
         {
            LLVMValueRef shuffles[8], shuffle;
            int i;
            assert(h16_bld.type.length <= Elements(shuffles));
            for (i = 0; i < h16_bld.type.length; i++)
               shuffles[i] = lp_build_const_int32(bld->gallivm, 2 * (i & 1));
            shuffle = LLVMConstVector(shuffles, h16_bld.type.length);
            lod_fpart = LLVMBuildShuffleVector(builder,
                                               lod_fpart, lod_fpart,
                                               shuffle, "");
         }
#endif

         colors0_lo = lp_build_lerp(&h16_bld, lod_fpart,
                                    colors0_lo, colors1_lo);
         colors0_hi = lp_build_lerp(&h16_bld, lod_fpart,
                                    colors0_hi, colors1_hi);

         LLVMBuildStore(builder, colors0_lo, colors_lo_var);
         LLVMBuildStore(builder, colors0_hi, colors_hi_var);
      }
      lp_build_endif(&if_ctx);
   }
}



/**
 * Texture sampling in AoS format.  Used when sampling common 32-bit/texel
 * formats.  1D/2D/3D/cube texture supported.  All mipmap sampling modes
 * but only limited texture coord wrap modes.
 */
void
lp_build_sample_aos(struct lp_build_sample_context *bld,
                    unsigned unit,
                    LLVMValueRef s,
                    LLVMValueRef t,
                    LLVMValueRef r,
                    const LLVMValueRef *ddx,
                    const LLVMValueRef *ddy,
                    LLVMValueRef lod_bias, /* optional */
                    LLVMValueRef explicit_lod, /* optional */
                    LLVMValueRef texel_out[4])
{
   struct lp_build_context *int_bld = &bld->int_bld;
   LLVMBuilderRef builder = bld->gallivm->builder;
   const unsigned mip_filter = bld->static_state->min_mip_filter;
   const unsigned min_filter = bld->static_state->min_img_filter;
   const unsigned mag_filter = bld->static_state->mag_img_filter;
   const unsigned dims = bld->dims;
   LLVMValueRef lod_ipart = NULL, lod_fpart = NULL;
   LLVMValueRef ilevel0, ilevel1 = NULL;
   LLVMValueRef packed, packed_lo, packed_hi;
   LLVMValueRef unswizzled[4];
   LLVMValueRef face_ddx[4], face_ddy[4];
   struct lp_build_context h16_bld;
   LLVMValueRef first_level;
   LLVMValueRef i32t_zero = lp_build_const_int32(bld->gallivm, 0);

   /* we only support the common/simple wrap modes at this time */
   assert(lp_is_simple_wrap_mode(bld->static_state->wrap_s));
   if (dims >= 2)
      assert(lp_is_simple_wrap_mode(bld->static_state->wrap_t));
   if (dims >= 3)
      assert(lp_is_simple_wrap_mode(bld->static_state->wrap_r));


   /* make 16-bit fixed-pt builder context */
   lp_build_context_init(&h16_bld, bld->gallivm, lp_type_ufixed(16));

   /* cube face selection, compute pre-face coords, etc. */
   if (bld->static_state->target == PIPE_TEXTURE_CUBE) {
      LLVMValueRef face, face_s, face_t;
      lp_build_cube_lookup(bld, s, t, r, &face, &face_s, &face_t);
      s = face_s; /* vec */
      t = face_t; /* vec */
      /* use 'r' to indicate cube face */
      r = lp_build_broadcast_scalar(&bld->int_coord_bld, face); /* vec */

      /* recompute ddx, ddy using the new (s,t) face texcoords */
      face_ddx[0] = lp_build_scalar_ddx(&bld->coord_bld, s);
      face_ddx[1] = lp_build_scalar_ddx(&bld->coord_bld, t);
      face_ddx[2] = NULL;
      face_ddx[3] = NULL;
      face_ddy[0] = lp_build_scalar_ddy(&bld->coord_bld, s);
      face_ddy[1] = lp_build_scalar_ddy(&bld->coord_bld, t);
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
      lp_build_lod_selector(bld, unit, ddx, ddy,
                            lod_bias, explicit_lod,
                            mip_filter,
                            &lod_ipart, &lod_fpart);
   } else {
      lod_ipart = i32t_zero;
   }

   /*
    * Compute integer mipmap level(s) to fetch texels from: ilevel0, ilevel1
    */
   switch (mip_filter) {
   default:
      assert(0 && "bad mip_filter value in lp_build_sample_aos()");
      /* fall-through */
   case PIPE_TEX_MIPFILTER_NONE:
      /* always use mip level 0 */
      if (bld->static_state->target == PIPE_TEXTURE_CUBE) {
         /* XXX this is a work-around for an apparent bug in LLVM 2.7.
          * We should be able to set ilevel0 = const(0) but that causes
          * bad x86 code to be emitted.
          */
         assert(lod_ipart);
         lp_build_nearest_mip_level(bld, unit, lod_ipart, &ilevel0);
      }
      else {
         first_level = bld->dynamic_state->first_level(bld->dynamic_state,
                                                       bld->gallivm, unit);
         ilevel0 = first_level;
      }
      break;
   case PIPE_TEX_MIPFILTER_NEAREST:
      assert(lod_ipart);
      lp_build_nearest_mip_level(bld, unit, lod_ipart, &ilevel0);
      break;
   case PIPE_TEX_MIPFILTER_LINEAR:
      assert(lod_ipart);
      assert(lod_fpart);
      lp_build_linear_mip_levels(bld, unit,
                                 lod_ipart, &lod_fpart,
                                 &ilevel0, &ilevel1);
      break;
   }

   /*
    * Get/interpolate texture colors.
    */

   packed_lo = lp_build_alloca(bld->gallivm, h16_bld.vec_type, "packed_lo");
   packed_hi = lp_build_alloca(bld->gallivm, h16_bld.vec_type, "packed_hi");

   if (min_filter == mag_filter) {
      /* no need to distinquish between minification and magnification */
      lp_build_sample_mipmap(bld,
                             min_filter, mip_filter,
                             s, t, r,
                             ilevel0, ilevel1, lod_fpart,
                             packed_lo, packed_hi);
   }
   else {
      /* Emit conditional to choose min image filter or mag image filter
       * depending on the lod being > 0 or <= 0, respectively.
       */
      struct lp_build_if_state if_ctx;
      LLVMValueRef minify;

      /* minify = lod >= 0.0 */
      minify = LLVMBuildICmp(builder, LLVMIntSGE,
                             lod_ipart, int_bld->zero, "");

      lp_build_if(&if_ctx, bld->gallivm, minify);
      {
         /* Use the minification filter */
         lp_build_sample_mipmap(bld,
                                min_filter, mip_filter,
                                s, t, r,
                                ilevel0, ilevel1, lod_fpart,
                                packed_lo, packed_hi);
      }
      lp_build_else(&if_ctx);
      {
         /* Use the magnification filter */
         lp_build_sample_mipmap(bld, 
                                mag_filter, PIPE_TEX_MIPFILTER_NONE,
                                s, t, r,
                                ilevel0, NULL, NULL,
                                packed_lo, packed_hi);
      }
      lp_build_endif(&if_ctx);
   }

   /*
    * combine the values stored in 'packed_lo' and 'packed_hi' variables
    * into 'packed'
    */
   packed = lp_build_pack2(bld->gallivm,
                           h16_bld.type, lp_type_unorm(8),
                           LLVMBuildLoad(builder, packed_lo, ""),
                           LLVMBuildLoad(builder, packed_hi, ""));

   /*
    * Convert to SoA and swizzle.
    */
   lp_build_rgba8_to_f32_soa(bld->gallivm,
                             bld->texel_type,
                             packed, unswizzled);

   if (util_format_is_rgba8_variant(bld->format_desc)) {
      lp_build_format_swizzle_soa(bld->format_desc,
                                  &bld->texel_bld,
                                  unswizzled, texel_out);
   }
   else {
      texel_out[0] = unswizzled[0];
      texel_out[1] = unswizzled[1];
      texel_out[2] = unswizzled[2];
      texel_out[3] = unswizzled[3];
   }
}
