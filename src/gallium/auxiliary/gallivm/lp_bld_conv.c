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
 * Helper functions for type conversions.
 *
 * We want to use the fastest type for a given computation whenever feasible.
 * The other side of this is that we need to be able convert between several
 * types accurately and efficiently.
 *
 * Conversion between types of different bit width is quite complex since a 
 *
 * To remember there are a few invariants in type conversions:
 *
 * - register width must remain constant:
 *
 *     src_type.width * src_type.length == dst_type.width * dst_type.length
 *
 * - total number of elements must remain constant:
 *
 *     src_type.length * num_srcs == dst_type.length * num_dsts
 *
 * It is not always possible to do the conversion both accurately and
 * efficiently, usually due to lack of adequate machine instructions. In these
 * cases it is important not to cut shortcuts here and sacrifice accuracy, as
 * there this functions can be used anywhere. In the future we might have a
 * precision parameter which can gauge the accuracy vs efficiency compromise,
 * but for now if the data conversion between two stages happens to be the
 * bottleneck, then most likely should just avoid converting at all and run
 * both stages with the same type.
 *
 * Make sure to run lp_test_conv unit test after any change to this file.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#include "util/u_debug.h"
#include "util/u_math.h"

#include "lp_bld_type.h"
#include "lp_bld_const.h"
#include "lp_bld_arit.h"
#include "lp_bld_pack.h"
#include "lp_bld_conv.h"


/**
 * Special case for converting clamped IEEE-754 floats to unsigned norms.
 *
 * The mathematical voodoo below may seem excessive but it is actually
 * paramount we do it this way for several reasons. First, there is no single
 * precision FP to unsigned integer conversion Intel SSE instruction. Second,
 * secondly, even if there was, since the FP's mantissa takes only a fraction
 * of register bits the typically scale and cast approach would require double
 * precision for accurate results, and therefore half the throughput
 *
 * Although the result values can be scaled to an arbitrary bit width specified
 * by dst_width, the actual result type will have the same width.
 */
LLVMValueRef
lp_build_clamped_float_to_unsigned_norm(LLVMBuilderRef builder,
                                        struct lp_type src_type,
                                        unsigned dst_width,
                                        LLVMValueRef src)
{
   LLVMTypeRef int_vec_type = lp_build_int_vec_type(src_type);
   LLVMValueRef res;
   unsigned mantissa;
   unsigned n;
   unsigned long long ubound;
   unsigned long long mask;
   double scale;
   double bias;

   assert(src_type.floating);

   mantissa = lp_mantissa(src_type);

   /* We cannot carry more bits than the mantissa */
   n = MIN2(mantissa, dst_width);

   /* This magic coefficients will make the desired result to appear in the
    * lowest significant bits of the mantissa.
    */
   ubound = ((unsigned long long)1 << n);
   mask = ubound - 1;
   scale = (double)mask/ubound;
   bias = (double)((unsigned long long)1 << (mantissa - n));

   res = LLVMBuildMul(builder, src, lp_build_const_scalar(src_type, scale), "");
   res = LLVMBuildAdd(builder, res, lp_build_const_scalar(src_type, bias), "");
   res = LLVMBuildBitCast(builder, res, int_vec_type, "");

   if(dst_width > n) {
      int shift = dst_width - n;
      res = LLVMBuildShl(builder, res, lp_build_int_const_scalar(src_type, shift), "");

      /* TODO: Fill in the empty lower bits for additional precision? */
      /* YES: this fixes progs/trivial/tri-z-eq.c.
       * Otherwise vertex Z=1.0 values get converted to something like
       * 0xfffffb00 and the test for equality with 0xffffffff fails.
       */
#if 0
      {
         LLVMValueRef msb;
         msb = LLVMBuildLShr(builder, res, lp_build_int_const_scalar(src_type, dst_width - 1), "");
         msb = LLVMBuildShl(builder, msb, lp_build_int_const_scalar(src_type, shift), "");
         msb = LLVMBuildSub(builder, msb, lp_build_int_const_scalar(src_type, 1), "");
         res = LLVMBuildOr(builder, res, msb, "");
      }
#elif 0
      while(shift > 0) {
         res = LLVMBuildOr(builder, res, LLVMBuildLShr(builder, res, lp_build_int_const_scalar(src_type, n), ""), "");
         shift -= n;
         n *= 2;
      }
#endif
   }
   else
      res = LLVMBuildAnd(builder, res, lp_build_int_const_scalar(src_type, mask), "");

   return res;
}


/**
 * Inverse of lp_build_clamped_float_to_unsigned_norm above.
 */
LLVMValueRef
lp_build_unsigned_norm_to_float(LLVMBuilderRef builder,
                                unsigned src_width,
                                struct lp_type dst_type,
                                LLVMValueRef src)
{
   LLVMTypeRef vec_type = lp_build_vec_type(dst_type);
   LLVMTypeRef int_vec_type = lp_build_int_vec_type(dst_type);
   LLVMValueRef bias_;
   LLVMValueRef res;
   unsigned mantissa;
   unsigned n;
   unsigned long long ubound;
   unsigned long long mask;
   double scale;
   double bias;

   mantissa = lp_mantissa(dst_type);

   n = MIN2(mantissa, src_width);

   ubound = ((unsigned long long)1 << n);
   mask = ubound - 1;
   scale = (double)ubound/mask;
   bias = (double)((unsigned long long)1 << (mantissa - n));

   res = src;

   if(src_width > mantissa) {
      int shift = src_width - mantissa;
      res = LLVMBuildLShr(builder, res, lp_build_int_const_scalar(dst_type, shift), "");
   }

   bias_ = lp_build_const_scalar(dst_type, bias);

   res = LLVMBuildOr(builder,
                     res,
                     LLVMBuildBitCast(builder, bias_, int_vec_type, ""), "");

   res = LLVMBuildBitCast(builder, res, vec_type, "");

   res = LLVMBuildSub(builder, res, bias_, "");
   res = LLVMBuildMul(builder, res, lp_build_const_scalar(dst_type, scale), "");

   return res;
}


/**
 * Generic type conversion.
 *
 * TODO: Take a precision argument, or even better, add a new precision member
 * to the lp_type union.
 */
void
lp_build_conv(LLVMBuilderRef builder,
              struct lp_type src_type,
              struct lp_type dst_type,
              const LLVMValueRef *src, unsigned num_srcs,
              LLVMValueRef *dst, unsigned num_dsts)
{
   struct lp_type tmp_type;
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

   if(memcmp(&src_type, &dst_type, sizeof src_type) != 0) {
      struct lp_build_context bld;
      double src_min = lp_const_min(src_type);
      double dst_min = lp_const_min(dst_type);
      double src_max = lp_const_max(src_type);
      double dst_max = lp_const_max(dst_type);
      LLVMValueRef thres;

      lp_build_context_init(&bld, builder, tmp_type);

      if(src_min < dst_min) {
         if(dst_min == 0.0)
            thres = bld.zero;
         else
            thres = lp_build_const_scalar(src_type, dst_min);
         for(i = 0; i < num_tmps; ++i)
            tmp[i] = lp_build_max(&bld, tmp[i], thres);
      }

      if(src_max > dst_max) {
         if(dst_max == 1.0)
            thres = bld.one;
         else
            thres = lp_build_const_scalar(src_type, dst_max);
         for(i = 0; i < num_tmps; ++i)
            tmp[i] = lp_build_min(&bld, tmp[i], thres);
      }
   }

   /*
    * Scale to the narrowest range
    */

   if(dst_type.floating) {
      /* Nothing to do */
   }
   else if(tmp_type.floating) {
      if(!dst_type.fixed && !dst_type.sign && dst_type.norm) {
         for(i = 0; i < num_tmps; ++i) {
            tmp[i] = lp_build_clamped_float_to_unsigned_norm(builder,
                                                             tmp_type,
                                                             dst_type.width,
                                                             tmp[i]);
         }
         tmp_type.floating = FALSE;
      }
      else {
         double dst_scale = lp_const_scale(dst_type);
         LLVMTypeRef tmp_vec_type;

         if (dst_scale != 1.0) {
            LLVMValueRef scale = lp_build_const_scalar(tmp_type, dst_scale);
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
   }
   else {
      unsigned src_shift = lp_const_shift(src_type);
      unsigned dst_shift = lp_const_shift(dst_type);

      /* FIXME: compensate different offsets too */
      if(src_shift > dst_shift) {
         LLVMValueRef shift = lp_build_int_const_scalar(tmp_type, src_shift - dst_shift);
         for(i = 0; i < num_tmps; ++i)
            if(src_type.sign)
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
      tmp[0] = lp_build_pack(builder, tmp_type, dst_type, TRUE, tmp, num_tmps);
      tmp_type.width = dst_type.width;
      tmp_type.length = dst_type.length;
      num_tmps = 1;
   }

   if(tmp_type.width < dst_type.width) {
      assert(num_tmps == 1);
      lp_build_unpack(builder, tmp_type, dst_type, tmp[0], tmp, num_dsts);
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
      if(!src_type.fixed && !src_type.sign && src_type.norm) {
         for(i = 0; i < num_tmps; ++i) {
            tmp[i] = lp_build_unsigned_norm_to_float(builder,
                                                     src_type.width,
                                                     dst_type,
                                                     tmp[i]);
         }
         tmp_type.floating = TRUE;
      }
      else {
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
             LLVMValueRef scale = lp_build_const_scalar(tmp_type, 1.0/src_scale);
             for(i = 0; i < num_tmps; ++i)
                tmp[i] = LLVMBuildMul(builder, tmp[i], scale, "");
          }
      }
    }
    else {
       unsigned src_shift = lp_const_shift(src_type);
       unsigned dst_shift = lp_const_shift(dst_type);

       /* FIXME: compensate different offsets too */
       if(src_shift < dst_shift) {
          LLVMValueRef shift = lp_build_int_const_scalar(tmp_type, dst_shift - src_shift);
          for(i = 0; i < num_tmps; ++i)
             tmp[i] = LLVMBuildShl(builder, tmp[i], shift, "");
       }
    }

   for(i = 0; i < num_dsts; ++i)
      dst[i] = tmp[i];
}


/**
 * Bit mask conversion.
 *
 * This will convert the integer masks that match the given types.
 *
 * The mask values should 0 or -1, i.e., all bits either set to zero or one.
 * Any other value will likely cause in unpredictable results.
 *
 * This is basically a very trimmed down version of lp_build_conv.
 */
void
lp_build_conv_mask(LLVMBuilderRef builder,
                   struct lp_type src_type,
                   struct lp_type dst_type,
                   const LLVMValueRef *src, unsigned num_srcs,
                   LLVMValueRef *dst, unsigned num_dsts)
{
   /* Register width must remain constant */
   assert(src_type.width * src_type.length == dst_type.width * dst_type.length);

   /* We must not loose or gain channels. Only precision */
   assert(src_type.length * num_srcs == dst_type.length * num_dsts);

   /*
    * Drop
    *
    * We assume all values are 0 or -1
    */

   src_type.floating = FALSE;
   src_type.fixed = FALSE;
   src_type.sign = TRUE;
   src_type.norm = FALSE;

   dst_type.floating = FALSE;
   dst_type.fixed = FALSE;
   dst_type.sign = TRUE;
   dst_type.norm = FALSE;

   /*
    * Truncate or expand bit width
    */

   if(src_type.width > dst_type.width) {
      assert(num_dsts == 1);
      dst[0] = lp_build_pack(builder, src_type, dst_type, TRUE, src, num_srcs);
   }
   else if(src_type.width < dst_type.width) {
      assert(num_srcs == 1);
      lp_build_unpack(builder, src_type, dst_type, src[0], dst, num_dsts);
   }
   else {
      assert(num_srcs == num_dsts);
      memcpy(dst, src, num_dsts * sizeof *dst);
   }
}
