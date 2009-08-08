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
 * Helper
 *
 * LLVM IR doesn't support all basic arithmetic operations we care about (most
 * notably min/max and saturated operations), and it is often necessary to
 * resort machine-specific intrinsics directly. The functions here hide all
 * these implementation details from the other modules.
 *
 * We also do simple expressions simplification here. Reasons are:
 * - it is very easy given we have all necessary information readily available
 * - LLVM optimization passes fail to simplify several vector expressions
 * - We often know value constraints which the optimization passes have no way
 *   of knowing, such as when source arguments are known to be in [0, 1] range.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#include "util/u_debug.h"

#include "lp_bld_type.h"
#include "lp_bld_const.h"
#include "lp_bld_intr.h"
#include "lp_bld_arit.h"
#include "lp_bld_conv.h"


/**
 * Build shuffle vectors that match PUNPCKLxx and PUNPCKHxx instructions.
 */
static LLVMValueRef
lp_build_const_expand_shuffle(unsigned n, unsigned lo_hi)
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


static void
lp_build_expand(LLVMBuilderRef builder,
               union lp_type src_type,
               union lp_type dst_type,
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
      union lp_type new_type = src_type;
      LLVMTypeRef new_vec_type;

      new_type.width *= 2;
      new_type.length /= 2;
      new_vec_type = lp_build_vec_type(new_type);

      for(i = num_tmps; i--; ) {
         LLVMValueRef zero;
         LLVMValueRef shuffle_lo;
         LLVMValueRef shuffle_hi;
         LLVMValueRef lo;
         LLVMValueRef hi;

         zero = lp_build_zero(src_type);
         shuffle_lo = lp_build_const_expand_shuffle(src_type.length, 0);
         shuffle_hi = lp_build_const_expand_shuffle(src_type.length, 1);

         /*  PUNPCKLBW, PUNPCKHBW */
         lo = LLVMBuildShuffleVector(builder, dst[i], zero, shuffle_lo, "");
         hi = LLVMBuildShuffleVector(builder, dst[i], zero, shuffle_hi, "");

         dst[2*i + 0] = LLVMBuildBitCast(builder, lo, new_vec_type, "");
         dst[2*i + 1] = LLVMBuildBitCast(builder, hi, new_vec_type, "");
      }

      src_type = new_type;

      num_tmps *= 2;
   }

   assert(num_tmps == num_dsts);
}


static LLVMValueRef
lp_build_trunc(LLVMBuilderRef builder,
               union lp_type src_type,
               union lp_type dst_type,
               const LLVMValueRef *src, unsigned num_srcs)
{
   LLVMValueRef tmp[LP_MAX_VECTOR_LENGTH];
   unsigned i;

   /* Register width must remain constant */
   assert(src_type.width * src_type.length == dst_type.width * dst_type.length);

   /* We must not loose or gain channels. Only precision */
   assert(src_type.length * num_srcs == dst_type.length);

   for(i = 0; i < num_srcs; ++i)
      tmp[i] = src[i];

   while(src_type.width > dst_type.width) {
      LLVMTypeRef tmp_vec_type = lp_build_vec_type(src_type);
      union lp_type new_type = src_type;
      LLVMTypeRef new_vec_type;

      new_type.width /= 2;
      new_type.length *= 2;
      new_vec_type = lp_build_vec_type(new_type);

      for(i = 0; i < num_srcs/2; ++i) {
         LLVMValueRef lo = tmp[2*i + 0];
         LLVMValueRef hi = tmp[2*i + 1];
         LLVMValueRef packed = NULL;

         if(src_type.width == 32) {
#if 0
            if(dst_type.sign)
               packed = lp_build_intrinsic_binary(builder, "llvm.x86.sse2.packssdw.128", tmp_vec_type, lo, hi);
            else {
               /* XXX: PACKUSDW intrinsic is actually the only one with a consistent signature */
               packed = lp_build_intrinsic_binary(builder, "llvm.x86.sse41.packusdw", new_vec_type, lo, hi);
            }
#else
            packed = lp_build_intrinsic_binary(builder, "llvm.x86.sse2.packssdw.128", tmp_vec_type, lo, hi);
#endif
         }
         else if(src_type.width == 16) {
            if(dst_type.sign)
               packed = lp_build_intrinsic_binary(builder, "llvm.x86.sse2.packsswb.128", tmp_vec_type, lo, hi);
            else
               packed = lp_build_intrinsic_binary(builder, "llvm.x86.sse2.packuswb.128", tmp_vec_type, lo, hi);
         }
         else
            assert(0);

         tmp[i] = LLVMBuildBitCast(builder, packed, new_vec_type, "");
      }

      src_type = new_type;

      num_srcs /= 2;
   }

   assert(num_srcs == 1);

   return tmp[0];
}


/**
 * Convert between two SIMD types.
 *
 * Converting between SIMD types of different element width poses a problem:
 * SIMD registers have a fixed number of bits, so different element widths
 * imply different vector lengths. Therefore we must multiplex the multiple
 * incoming sources into a single destination vector, or demux a single incoming
 * vector into multiple vectors.
 */
void
lp_build_conv(LLVMBuilderRef builder,
              union lp_type src_type,
              union lp_type dst_type,
              const LLVMValueRef *src, unsigned num_srcs,
              LLVMValueRef *dst, unsigned num_dsts)
{
   union lp_type tmp_type;
   LLVMValueRef tmp[LP_MAX_VECTOR_LENGTH];
   unsigned num_tmps;
   unsigned i;

   /* Register width must remain constant */
   assert(src_type.width * src_type.length == dst_type.width * dst_type.length);

   /* We must not loose or gain channels. Only precision */
   assert(src_type.length * num_srcs == dst_type.length * num_dsts);

   assert(src_type.length <= LP_MAX_VECTOR_LENGTH);
   assert(dst_type.length <= LP_MAX_VECTOR_LENGTH);

   tmp_type = src_type;
   for(i = 0; i < num_srcs; ++i)
      tmp[i] = src[i];
   num_tmps = num_srcs;

   /*
    * Clamp if necessary
    */

   if(tmp_type.sign != dst_type.sign || tmp_type.norm != dst_type.norm) {
      struct lp_build_context bld;
      lp_build_context_init(&bld, builder, tmp_type);

      if(tmp_type.sign && !dst_type.sign)
         for(i = 0; i < num_tmps; ++i)
            tmp[i] = lp_build_max(&bld, tmp[i], bld.zero);

      if(!tmp_type.norm && dst_type.norm)
         for(i = 0; i < num_tmps; ++i)
            tmp[i] = lp_build_min(&bld, tmp[i], bld.one);
   }

   /*
    * Scale to the narrowest range
    */

   if(dst_type.floating) {
      /* Nothing to do */
   }
   else if(tmp_type.floating) {
      double dst_scale = lp_const_scale(dst_type);
      LLVMTypeRef tmp_vec_type;

      if (dst_scale != 1.0) {
         LLVMValueRef scale = lp_build_const_uni(tmp_type, dst_scale);
         for(i = 0; i < num_tmps; ++i)
            tmp[i] = LLVMBuildMul(builder, tmp[i], scale, "");
      }

      /* Use an equally sized integer for intermediate computations */
      tmp_type.floating = FALSE;
      tmp_vec_type = lp_build_vec_type(tmp_type);
      for(i = 0; i < num_tmps; ++i) {
#if 0
         if(dst_type.sign)
            tmp[i] = LLVMBuildFPToSI(builder, tmp[i], tmp_vec_type, "");
         else
            tmp[i] = LLVMBuildFPToUI(builder, tmp[i], tmp_vec_type, "");
#else
        /* FIXME: there is no SSE counterpart for LLVMBuildFPToUI */
         tmp[i] = LLVMBuildFPToSI(builder, tmp[i], tmp_vec_type, "");
#endif
      }
   }
   else {
      unsigned src_shift = lp_const_shift(src_type);
      unsigned dst_shift = lp_const_shift(dst_type);

      /* FIXME: compensate different offsets too */
      if(src_shift > dst_shift) {
         LLVMValueRef shift = lp_build_int_const_uni(tmp_type, src_shift - dst_shift);
         for(i = 0; i < num_tmps; ++i)
            if(dst_type.sign)
               tmp[i] = LLVMBuildAShr(builder, tmp[i], shift, "");
            else
               tmp[i] = LLVMBuildLShr(builder, tmp[i], shift, "");
      }
   }

   /*
    * Truncate or expand bit width
    */

   assert(!tmp_type.floating || tmp_type.width == dst_type.width);

   if(tmp_type.width > dst_type.width) {
      assert(num_dsts == 1);
      tmp[0] = lp_build_trunc(builder, tmp_type, dst_type, tmp, num_tmps);
      tmp_type.width = dst_type.width;
      tmp_type.length = dst_type.length;
      num_tmps = 1;
   }

   if(tmp_type.width < dst_type.width) {
      assert(num_tmps == 1);
      lp_build_expand(builder, tmp_type, dst_type, tmp[0], tmp, num_dsts);
      tmp_type.width = dst_type.width;
      tmp_type.length = dst_type.length;
      num_tmps = num_dsts;
   }

   assert(tmp_type.width == dst_type.width);
   assert(tmp_type.length == dst_type.length);
   assert(num_tmps == num_dsts);

   /*
    * Scale to the widest range
    */

   if(src_type.floating) {
      /* Nothing to do */
   }
   else if(!src_type.floating && dst_type.floating) {
      double src_scale = lp_const_scale(src_type);
      LLVMTypeRef tmp_vec_type;

      /* Use an equally sized integer for intermediate computations */
      tmp_type.floating = TRUE;
      tmp_type.sign = TRUE;
      tmp_vec_type = lp_build_vec_type(tmp_type);
      for(i = 0; i < num_tmps; ++i) {
 #if 0
         if(dst_type.sign)
            tmp[i] = LLVMBuildSIToFP(builder, tmp[i], tmp_vec_type, "");
         else
            tmp[i] = LLVMBuildUIToFP(builder, tmp[i], tmp_vec_type, "");
 #else
         /* FIXME: there is no SSE counterpart for LLVMBuildUIToFP */
         tmp[i] = LLVMBuildSIToFP(builder, tmp[i], tmp_vec_type, "");
 #endif
       }

       if (src_scale != 1.0) {
          LLVMValueRef scale = lp_build_const_uni(tmp_type, 1.0/src_scale);
          for(i = 0; i < num_tmps; ++i)
             tmp[i] = LLVMBuildMul(builder, tmp[i], scale, "");
       }
    }
    else {
       unsigned src_shift = lp_const_shift(src_type);
       unsigned dst_shift = lp_const_shift(dst_type);

       /* FIXME: compensate different offsets too */
       if(src_shift < dst_shift) {
          LLVMValueRef shift = lp_build_int_const_uni(tmp_type, dst_shift - src_shift);
          for(i = 0; i < num_tmps; ++i)
             tmp[i] = LLVMBuildShl(builder, tmp[i], shift, "");
       }
    }

   for(i = 0; i < num_dsts; ++i)
      dst[i] = tmp[i];
}
