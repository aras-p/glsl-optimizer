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
#include "util/u_debug_dump.h"
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

   /** Integer coordinates */
   struct lp_type int_coord_type;
   struct lp_build_context int_coord_bld;

   /** Output texels type and build context */
   struct lp_type texel_type;
   struct lp_build_context texel_bld;
};


static void
lp_build_sample_texel_soa(struct lp_build_sample_context *bld,
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
lp_build_sample_packed(struct lp_build_sample_context *bld,
                       LLVMValueRef x,
                       LLVMValueRef y,
                       LLVMValueRef y_stride,
                       LLVMValueRef data_ptr)
{
   LLVMValueRef offset;

   offset = lp_build_sample_offset(&bld->int_coord_bld,
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
      _debug_printf("llvmpipe: failed to translate texture wrap mode %s\n",
                    debug_dump_tex_wrap(wrap_mode, TRUE));
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
   lp_build_name(x, "tex.x.floor");
   lp_build_name(y, "tex.y.floor");

   x = lp_build_sample_wrap(bld, x, width,  bld->static_state->pot_width,  bld->static_state->wrap_s);
   y = lp_build_sample_wrap(bld, y, height, bld->static_state->pot_height, bld->static_state->wrap_t);
   lp_build_name(x, "tex.x.wrapped");
   lp_build_name(y, "tex.y.wrapped");

   lp_build_sample_texel_soa(bld, x, y, stride, data_ptr, texel);
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

   lp_build_sample_texel_soa(bld, x0, y0, stride, data_ptr, neighbors[0][0]);
   lp_build_sample_texel_soa(bld, x1, y0, stride, data_ptr, neighbors[0][1]);
   lp_build_sample_texel_soa(bld, x0, y1, stride, data_ptr, neighbors[1][0]);
   lp_build_sample_texel_soa(bld, x1, y1, stride, data_ptr, neighbors[1][1]);

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

   s = lp_build_mul_imm(&bld->coord_bld, s, 256);
   t = lp_build_mul_imm(&bld->coord_bld, t, 256);

   s = LLVMBuildFPToSI(builder, s, i32_vec_type, "");
   t = LLVMBuildFPToSI(builder, t, i32_vec_type, "");

   i32_c128 = lp_build_int_const_scalar(i32.type, -128);
   s = LLVMBuildAdd(builder, s, i32_c128, "");
   t = LLVMBuildAdd(builder, t, i32_c128, "");

   i32_c8 = lp_build_int_const_scalar(i32.type, 8);
   s_ipart = LLVMBuildAShr(builder, s, i32_c8, "");
   t_ipart = LLVMBuildAShr(builder, t, i32_c8, "");

   i32_c255 = lp_build_int_const_scalar(i32.type, 255);
   s_fpart = LLVMBuildAnd(builder, s, i32_c255, "");
   t_fpart = LLVMBuildAnd(builder, t, i32_c255, "");

   x0 = s_ipart;
   y0 = t_ipart;

   x0 = lp_build_sample_wrap(bld, x0, width,  bld->static_state->pot_width,  bld->static_state->wrap_s);
   y0 = lp_build_sample_wrap(bld, y0, height, bld->static_state->pot_height, bld->static_state->wrap_t);

   x1 = lp_build_add(&bld->int_coord_bld, x0, bld->int_coord_bld.one);
   y1 = lp_build_add(&bld->int_coord_bld, y0, bld->int_coord_bld.one);

   x1 = lp_build_sample_wrap(bld, x1, width,  bld->static_state->pot_width,  bld->static_state->wrap_s);
   y1 = lp_build_sample_wrap(bld, y1, height, bld->static_state->pot_height, bld->static_state->wrap_t);

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
      if(lp_format_is_rgba8(bld.format_desc))
         lp_build_sample_2d_linear_aos(&bld, s, t, width, height, stride, data_ptr, texel);
      else
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
