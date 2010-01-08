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
 * Helper functions for packing/unpacking.
 *
 * Pack/unpacking is necessary for conversion between types of different
 * bit width.
 *
 * They are also commonly used when an computation needs higher
 * precision for the intermediate values. For example, if one needs the
 * function:
 *
 *   c = compute(a, b);
 *
 * to use more precision for intermediate results then one should implement it
 * as:
 *
 *   LLVMValueRef
 *   compute(LLVMBuilderRef builder struct lp_type type, LLVMValueRef a, LLVMValueRef b)
 *   {
 *      struct lp_type wide_type = lp_wider_type(type);
 *      LLVMValueRef al, ah, bl, bh, cl, ch, c;
 *
 *      lp_build_unpack2(builder, type, wide_type, a, &al, &ah);
 *      lp_build_unpack2(builder, type, wide_type, b, &bl, &bh);
 *
 *      cl = compute_half(al, bl);
 *      ch = compute_half(ah, bh);
 *
 *      c = lp_build_pack2(bld->builder, wide_type, type, cl, ch);
 *
 *      return c;
 *   }
 *
 * where compute_half() would do the computation for half the elements with
 * twice the precision.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#include "util/u_debug.h"
#include "util/u_math.h"
#include "util/u_cpu_detect.h"

#include "lp_bld_type.h"
#include "lp_bld_const.h"
#include "lp_bld_intr.h"
#include "lp_bld_arit.h"
#include "lp_bld_pack.h"


/**
 * Build shuffle vectors that match PUNPCKLxx and PUNPCKHxx instructions.
 */
static LLVMValueRef
lp_build_const_unpack_shuffle(unsigned n, unsigned lo_hi)
{
   LLVMValueRef elems[LP_MAX_VECTOR_LENGTH];
   unsigned i, j;

   assert(n <= LP_MAX_VECTOR_LENGTH);
   assert(lo_hi < 2);

   /* TODO: cache results in a static table */

   for(i = 0, j = lo_hi*n/2; i < n; i += 2, ++j) {
      elems[i + 0] = LLVMConstInt(LLVMInt32Type(), 0 + j, 0);
      elems[i + 1] = LLVMConstInt(LLVMInt32Type(), n + j, 0);
   }

   return LLVMConstVector(elems, n);
}


/**
 * Build shuffle vectors that match PACKxx instructions.
 */
static LLVMValueRef
lp_build_const_pack_shuffle(unsigned n)
{
   LLVMValueRef elems[LP_MAX_VECTOR_LENGTH];
   unsigned i;

   assert(n <= LP_MAX_VECTOR_LENGTH);

   /* TODO: cache results in a static table */

   for(i = 0; i < n; ++i)
      elems[i] = LLVMConstInt(LLVMInt32Type(), 2*i, 0);

   return LLVMConstVector(elems, n);
}


/**
 * Interleave vector elements.
 *
 * Matches the PUNPCKLxx and PUNPCKHxx SSE instructions.
 */
LLVMValueRef
lp_build_interleave2(LLVMBuilderRef builder,
                     struct lp_type type,
                     LLVMValueRef a,
                     LLVMValueRef b,
                     unsigned lo_hi)
{
   LLVMValueRef shuffle;

   shuffle = lp_build_const_unpack_shuffle(type.length, lo_hi);

   return LLVMBuildShuffleVector(builder, a, b, shuffle, "");
}


/**
 * Double the bit width.
 *
 * This will only change the number of bits the values are represented, not the
 * values themselves.
 */
void
lp_build_unpack2(LLVMBuilderRef builder,
                 struct lp_type src_type,
                 struct lp_type dst_type,
                 LLVMValueRef src,
                 LLVMValueRef *dst_lo,
                 LLVMValueRef *dst_hi)
{
   LLVMValueRef msb;
   LLVMTypeRef dst_vec_type;

   assert(!src_type.floating);
   assert(!dst_type.floating);
   assert(dst_type.width == src_type.width * 2);
   assert(dst_type.length * 2 == src_type.length);

   if(dst_type.sign && src_type.sign) {
      /* Replicate the sign bit in the most significant bits */
      msb = LLVMBuildAShr(builder, src, lp_build_int_const_scalar(src_type, src_type.width - 1), "");
   }
   else
      /* Most significant bits always zero */
      msb = lp_build_zero(src_type);

   /* Interleave bits */
   if(util_cpu_caps.little_endian) {
      *dst_lo = lp_build_interleave2(builder, src_type, src, msb, 0);
      *dst_hi = lp_build_interleave2(builder, src_type, src, msb, 1);
   }
   else {
      *dst_lo = lp_build_interleave2(builder, src_type, msb, src, 0);
      *dst_hi = lp_build_interleave2(builder, src_type, msb, src, 1);
   }

   /* Cast the result into the new type (twice as wide) */

   dst_vec_type = lp_build_vec_type(dst_type);

   *dst_lo = LLVMBuildBitCast(builder, *dst_lo, dst_vec_type, "");
   *dst_hi = LLVMBuildBitCast(builder, *dst_hi, dst_vec_type, "");
}


/**
 * Expand the bit width.
 *
 * This will only change the number of bits the values are represented, not the
 * values themselves.
 */
void
lp_build_unpack(LLVMBuilderRef builder,
                struct lp_type src_type,
                struct lp_type dst_type,
                LLVMValueRef src,
                LLVMValueRef *dst, unsigned num_dsts)
{
   unsigned num_tmps;
   unsigned i;

   /* Register width must remain constant */
   assert(src_type.width * src_type.length == dst_type.width * dst_type.length);

   /* We must not loose or gain channels. Only precision */
   assert(src_type.length == dst_type.length * num_dsts);

   num_tmps = 1;
   dst[0] = src;

   while(src_type.width < dst_type.width) {
      struct lp_type tmp_type = src_type;

      tmp_type.width *= 2;
      tmp_type.length /= 2;

      for(i = num_tmps; i--; ) {
         lp_build_unpack2(builder, src_type, tmp_type, dst[i], &dst[2*i + 0], &dst[2*i + 1]);
      }

      src_type = tmp_type;

      num_tmps *= 2;
   }

   assert(num_tmps == num_dsts);
}


/**
 * Non-interleaved pack.
 *
 * This will move values as
 *
 *   lo =   __ l0 __ l1 __ l2 __..  __ ln
 *   hi =   __ h0 __ h1 __ h2 __..  __ hn
 *   res =  l0 l1 l2 .. ln h0 h1 h2 .. hn
 *
 * This will only change the number of bits the values are represented, not the
 * values themselves.
 *
 * It is assumed the values are already clamped into the destination type range.
 * Values outside that range will produce undefined results. Use
 * lp_build_packs2 instead.
 */
LLVMValueRef
lp_build_pack2(LLVMBuilderRef builder,
               struct lp_type src_type,
               struct lp_type dst_type,
               LLVMValueRef lo,
               LLVMValueRef hi)
{
   LLVMTypeRef src_vec_type = lp_build_vec_type(src_type);
   LLVMTypeRef dst_vec_type = lp_build_vec_type(dst_type);
   LLVMValueRef shuffle;
   LLVMValueRef res;

   dst_vec_type = lp_build_vec_type(dst_type);

   assert(!src_type.floating);
   assert(!dst_type.floating);
   assert(src_type.width == dst_type.width * 2);
   assert(src_type.length * 2 == dst_type.length);

   if(util_cpu_caps.has_sse2 && src_type.width * src_type.length == 128) {
      switch(src_type.width) {
      case 32:
         if(dst_type.sign) {
            res = lp_build_intrinsic_binary(builder, "llvm.x86.sse2.packssdw.128", src_vec_type, lo, hi);
         }
         else {
            if (util_cpu_caps.has_sse4_1) {
               /* PACKUSDW is the only instrinsic with a consistent signature */
               return lp_build_intrinsic_binary(builder, "llvm.x86.sse41.packusdw", dst_vec_type, lo, hi);
            }
            else {
               assert(0);
               return LLVMGetUndef(dst_vec_type);
            }
         }
         break;

      case 16:
         if(dst_type.sign)
            res = lp_build_intrinsic_binary(builder, "llvm.x86.sse2.packsswb.128", src_vec_type, lo, hi);
         else
            res = lp_build_intrinsic_binary(builder, "llvm.x86.sse2.packuswb.128", src_vec_type, lo, hi);
         break;

      default:
         assert(0);
         return LLVMGetUndef(dst_vec_type);
         break;
      }

      res = LLVMBuildBitCast(builder, res, dst_vec_type, "");
      return res;
   }

   lo = LLVMBuildBitCast(builder, lo, dst_vec_type, "");
   hi = LLVMBuildBitCast(builder, hi, dst_vec_type, "");

   shuffle = lp_build_const_pack_shuffle(dst_type.length);

   res = LLVMBuildShuffleVector(builder, lo, hi, shuffle, "");

   return res;
}



/**
 * Non-interleaved pack and saturate.
 *
 * Same as lp_build_pack2 but will saturate values so that they fit into the
 * destination type.
 */
LLVMValueRef
lp_build_packs2(LLVMBuilderRef builder,
                struct lp_type src_type,
                struct lp_type dst_type,
                LLVMValueRef lo,
                LLVMValueRef hi)
{
   boolean clamp;

   assert(!src_type.floating);
   assert(!dst_type.floating);
   assert(src_type.sign == dst_type.sign);
   assert(src_type.width == dst_type.width * 2);
   assert(src_type.length * 2 == dst_type.length);

   clamp = TRUE;

   /* All X86 SSE non-interleaved pack instructions take signed inputs and
    * saturate them, so no need to clamp for those cases. */
   if(util_cpu_caps.has_sse2 &&
      src_type.width * src_type.length == 128 &&
      src_type.sign)
      clamp = FALSE;

   if(clamp) {
      struct lp_build_context bld;
      unsigned dst_bits = dst_type.sign ? dst_type.width - 1 : dst_type.width;
      LLVMValueRef dst_max = lp_build_int_const_scalar(src_type, ((unsigned long long)1 << dst_bits) - 1);
      lp_build_context_init(&bld, builder, src_type);
      lo = lp_build_min(&bld, lo, dst_max);
      hi = lp_build_min(&bld, hi, dst_max);
      /* FIXME: What about lower bound? */
   }

   return lp_build_pack2(builder, src_type, dst_type, lo, hi);
}


/**
 * Truncate the bit width.
 *
 * TODO: Handle saturation consistently.
 */
LLVMValueRef
lp_build_pack(LLVMBuilderRef builder,
              struct lp_type src_type,
              struct lp_type dst_type,
              boolean clamped,
              const LLVMValueRef *src, unsigned num_srcs)
{
   LLVMValueRef (*pack2)(LLVMBuilderRef builder,
                         struct lp_type src_type,
                         struct lp_type dst_type,
                         LLVMValueRef lo,
                         LLVMValueRef hi);
   LLVMValueRef tmp[LP_MAX_VECTOR_LENGTH];
   unsigned i;


   /* Register width must remain constant */
   assert(src_type.width * src_type.length == dst_type.width * dst_type.length);

   /* We must not loose or gain channels. Only precision */
   assert(src_type.length * num_srcs == dst_type.length);

   if(clamped)
      pack2 = &lp_build_pack2;
   else
      pack2 = &lp_build_packs2;

   for(i = 0; i < num_srcs; ++i)
      tmp[i] = src[i];

   while(src_type.width > dst_type.width) {
      struct lp_type tmp_type = src_type;

      tmp_type.width /= 2;
      tmp_type.length *= 2;

      /* Take in consideration the sign changes only in the last step */
      if(tmp_type.width == dst_type.width)
         tmp_type.sign = dst_type.sign;

      num_srcs /= 2;

      for(i = 0; i < num_srcs; ++i)
         tmp[i] = pack2(builder, src_type, tmp_type, tmp[2*i + 0], tmp[2*i + 1]);

      src_type = tmp_type;
   }

   assert(num_srcs == 1);

   return tmp[0];
}
