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
 * Helper functions for swizzling/shuffling.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#include "util/u_debug.h"

#include "lp_bld_type.h"
#include "lp_bld_const.h"
#include "lp_bld_logic.h"
#include "lp_bld_swizzle.h"


LLVMValueRef
lp_build_broadcast(LLVMBuilderRef builder,
                   LLVMTypeRef vec_type,
                   LLVMValueRef scalar)
{
   const unsigned n = LLVMGetVectorSize(vec_type);
   LLVMValueRef res;
   unsigned i;

   res = LLVMGetUndef(vec_type);
   for(i = 0; i < n; ++i) {
      LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), i, 0);
      res = LLVMBuildInsertElement(builder, res, scalar, index, "");
   }

   return res;
}


LLVMValueRef
lp_build_broadcast_scalar(struct lp_build_context *bld,
                          LLVMValueRef scalar)
{
   const struct lp_type type = bld->type;
   LLVMValueRef res;
   unsigned i;

   res = bld->undef;
   for(i = 0; i < type.length; ++i) {
      LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), i, 0);
      res = LLVMBuildInsertElement(bld->builder, res, scalar, index, "");
   }

   return res;
}


LLVMValueRef
lp_build_broadcast_aos(struct lp_build_context *bld,
                       LLVMValueRef a,
                       unsigned channel)
{
   const struct lp_type type = bld->type;
   const unsigned n = type.length;
   unsigned i, j;

   if(a == bld->undef || a == bld->zero || a == bld->one)
      return a;

   /* XXX: SSE3 has PSHUFB which should be better than bitmasks, but forcing
    * using shuffles here actually causes worst results. More investigation is
    * needed. */
   if (n <= 4) {
      /*
       * Shuffle.
       */
      LLVMTypeRef elem_type = LLVMInt32Type();
      LLVMValueRef shuffles[LP_MAX_VECTOR_LENGTH];

      for(j = 0; j < n; j += 4)
         for(i = 0; i < 4; ++i)
            shuffles[j + i] = LLVMConstInt(elem_type, j + channel, 0);

      return LLVMBuildShuffleVector(bld->builder, a, bld->undef, LLVMConstVector(shuffles, n), "");
   }
   else {
      /*
       * Bit mask and recursive shifts
       *
       *   XYZW XYZW .... XYZW  <= input
       *   0Y00 0Y00 .... 0Y00
       *   YY00 YY00 .... YY00
       *   YYYY YYYY .... YYYY  <= output
       */
      struct lp_type type4 = type;
      const char shifts[4][2] = {
         { 1,  2},
         {-1,  2},
         { 1, -2},
         {-1, -2}
      };
      boolean cond[4];
      unsigned i;

      memset(cond, 0, sizeof cond);
      cond[channel] = 1;

      a = LLVMBuildAnd(bld->builder, a, lp_build_const_mask_aos(type, cond), "");

      type4.width *= 4;
      type4.length /= 4;

      a = LLVMBuildBitCast(bld->builder, a, lp_build_vec_type(type4), "");

      for(i = 0; i < 2; ++i) {
         LLVMValueRef tmp = NULL;
         int shift = shifts[channel][i];

#ifdef PIPE_ARCH_LITTLE_ENDIAN
         shift = -shift;
#endif

         if(shift > 0)
            tmp = LLVMBuildLShr(bld->builder, a, lp_build_int_const_scalar(type4, shift*type.width), "");
         if(shift < 0)
            tmp = LLVMBuildShl(bld->builder, a, lp_build_int_const_scalar(type4, -shift*type.width), "");

         assert(tmp);
         if(tmp)
            a = LLVMBuildOr(bld->builder, a, tmp, "");
      }

      return LLVMBuildBitCast(bld->builder, a, lp_build_vec_type(type), "");
   }
}


LLVMValueRef
lp_build_swizzle1_aos(struct lp_build_context *bld,
                      LLVMValueRef a,
                      const unsigned char swizzle[4])
{
   const unsigned n = bld->type.length;
   unsigned i, j;

   if(a == bld->undef || a == bld->zero || a == bld->one)
      return a;

   if(swizzle[0] == swizzle[1] && swizzle[1] == swizzle[2] && swizzle[2] == swizzle[3])
      return lp_build_broadcast_aos(bld, a, swizzle[0]);

   {
      /*
       * Shuffle.
       */
      LLVMTypeRef elem_type = LLVMInt32Type();
      LLVMValueRef shuffles[LP_MAX_VECTOR_LENGTH];

      for(j = 0; j < n; j += 4)
         for(i = 0; i < 4; ++i)
            shuffles[j + i] = LLVMConstInt(elem_type, j + swizzle[i], 0);

      return LLVMBuildShuffleVector(bld->builder, a, bld->undef, LLVMConstVector(shuffles, n), "");
   }
}


LLVMValueRef
lp_build_swizzle2_aos(struct lp_build_context *bld,
                      LLVMValueRef a,
                      LLVMValueRef b,
                      const unsigned char swizzle[4])
{
   const unsigned n = bld->type.length;
   unsigned i, j;

   if(swizzle[0] < 4 && swizzle[1] < 4 && swizzle[2] < 4 && swizzle[3] < 4)
      return lp_build_swizzle1_aos(bld, a, swizzle);

   if(a == b) {
      unsigned char swizzle1[4];
      swizzle1[0] = swizzle[0] % 4;
      swizzle1[1] = swizzle[1] % 4;
      swizzle1[2] = swizzle[2] % 4;
      swizzle1[3] = swizzle[3] % 4;
      return lp_build_swizzle1_aos(bld, a, swizzle1);
   }

   if(swizzle[0] % 4 == 0 &&
      swizzle[1] % 4 == 1 &&
      swizzle[2] % 4 == 2 &&
      swizzle[3] % 4 == 3) {
      boolean cond[4];
      cond[0] = swizzle[0] / 4;
      cond[1] = swizzle[1] / 4;
      cond[2] = swizzle[2] / 4;
      cond[3] = swizzle[3] / 4;
      return lp_build_select_aos(bld, a, b, cond);
   }

   {
      /*
       * Shuffle.
       */
      LLVMTypeRef elem_type = LLVMInt32Type();
      LLVMValueRef shuffles[LP_MAX_VECTOR_LENGTH];

      for(j = 0; j < n; j += 4)
         for(i = 0; i < 4; ++i)
            shuffles[j + i] = LLVMConstInt(elem_type, j + (swizzle[i] % 4) + (swizzle[i] / 4 * n), 0);

      return LLVMBuildShuffleVector(bld->builder, a, b, LLVMConstVector(shuffles, n), "");
   }
}


