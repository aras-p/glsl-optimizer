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
#include "lp_bld_conv.h"


static LLVMValueRef
lp_build_trunc(LLVMBuilderRef builder,
               union lp_type src_type,
               union lp_type dst_type,
               LLVMValueRef *src, unsigned num_srcs)
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
            /* FIXME: we only have a packed signed intrinsic */
            packed = lp_build_intrinsic_binary(builder, "llvm.x86.sse2.packssdw.128", tmp_vec_type, lo, hi);
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
              LLVMValueRef *src, unsigned num_srcs,
              LLVMValueRef *dst, unsigned num_dsts)
{
   unsigned i;

   /* Register width must remain constant */
   assert(src_type.width * src_type.length == dst_type.width * dst_type.length);

   /* We must not loose or gain channels. Only precision */
   assert(src_type.length * num_srcs == dst_type.length * num_dsts);

   if(!src_type.norm && dst_type.norm) {
      /* FIXME: clamp */
   }

   if(src_type.floating && !dst_type.floating) {
      double dscale;
      LLVMTypeRef tmp;

      /* Rescale */
      dscale = lp_const_scale(dst_type);
      if (dscale != 1.0) {
         LLVMValueRef scale = lp_build_const_uni(src_type, dscale);
         for(i = 0; i < num_srcs; ++i)
            src[i] = LLVMBuildMul(builder, src[i], scale, "");
      }

      /* Use an equally sized integer for intermediate computations */
      src_type.floating = FALSE;
      tmp = lp_build_vec_type(src_type);
      for(i = 0; i < num_srcs; ++i) {
#if 0
         if(dst_type.sign)
            src[i] = LLVMBuildFPToSI(builder, src[i], tmp, "");
         else
            src[i] = LLVMBuildFPToUI(builder, src[i], tmp, "");
#else
        /* FIXME: there is no SSE counterpart for LLVMBuildFPToUI */
         src[i] = LLVMBuildFPToSI(builder, src[i], tmp, "");
#endif
      }
   }
   else {
      unsigned src_shift = lp_const_shift(src_type);
      unsigned dst_shift = lp_const_shift(dst_type);

      if(src_shift > dst_shift) {
         LLVMValueRef shift = lp_build_int_const_uni(src_type, src_shift - dst_shift);
         for(i = 0; i < num_srcs; ++i)
            if(dst_type.sign)
               src[i] = LLVMBuildAShr(builder, src[i], shift, "");
            else
               src[i] = LLVMBuildLShr(builder, src[i], shift, "");
      }
   }

   if(src_type.width > dst_type.width) {
      assert(num_dsts == 1);
      dst[0] = lp_build_trunc(builder, src_type, dst_type, src, num_srcs);
   }
   else
      assert(0);
}
