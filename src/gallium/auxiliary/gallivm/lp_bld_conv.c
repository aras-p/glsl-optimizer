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
#include "util/u_half.h"
#include "util/u_cpu_detect.h"

#include "lp_bld_type.h"
#include "lp_bld_const.h"
#include "lp_bld_arit.h"
#include "lp_bld_bitarit.h"
#include "lp_bld_pack.h"
#include "lp_bld_conv.h"
#include "lp_bld_logic.h"
#include "lp_bld_intr.h"
#include "lp_bld_printf.h"
#include "lp_bld_format.h"



/**
 * Converts int16 half-float to float32
 * Note this can be performed in 1 instruction if vcvtph2ps exists (f16c/cvt16)
 * [llvm.x86.vcvtph2ps / _mm_cvtph_ps]
 *
 * @param src           value to convert
 *
 */
LLVMValueRef
lp_build_half_to_float(struct gallivm_state *gallivm,
                       LLVMValueRef src)
{
   LLVMBuilderRef builder = gallivm->builder;
   LLVMTypeRef src_type = LLVMTypeOf(src);
   unsigned src_length = LLVMGetTypeKind(src_type) == LLVMVectorTypeKind ?
                            LLVMGetVectorSize(src_type) : 1;

   struct lp_type f32_type = lp_type_float_vec(32, 32 * src_length);
   struct lp_type i32_type = lp_type_int_vec(32, 32 * src_length);
   LLVMTypeRef int_vec_type = lp_build_vec_type(gallivm, i32_type);
   LLVMValueRef h;

   if (util_cpu_caps.has_f16c &&
       (src_length == 4 || src_length == 8)) {
      const char *intrinsic = NULL;
      if (src_length == 4) {
         src = lp_build_pad_vector(gallivm, src, 8);
         intrinsic = "llvm.x86.vcvtph2ps.128";
      }
      else {
         intrinsic = "llvm.x86.vcvtph2ps.256";
      }
      return lp_build_intrinsic_unary(builder, intrinsic,
                                      lp_build_vec_type(gallivm, f32_type), src);
   }

   /* Convert int16 vector to int32 vector by zero ext (might generate bad code) */
   h = LLVMBuildZExt(builder, src, int_vec_type, "");
   return lp_build_smallfloat_to_float(gallivm, f32_type, h, 10, 5, 0, true);
}


/**
 * Converts float32 to int16 half-float
 * Note this can be performed in 1 instruction if vcvtps2ph exists (f16c/cvt16)
 * [llvm.x86.vcvtps2ph / _mm_cvtps_ph]
 *
 * @param src           value to convert
 *
 * Convert float32 to half floats, preserving Infs and NaNs,
 * with rounding towards zero (trunc).
 */
LLVMValueRef
lp_build_float_to_half(struct gallivm_state *gallivm,
                       LLVMValueRef src)
{
   LLVMBuilderRef builder = gallivm->builder;
   LLVMTypeRef f32_vec_type = LLVMTypeOf(src);
   unsigned length = LLVMGetTypeKind(f32_vec_type) == LLVMVectorTypeKind
                   ? LLVMGetVectorSize(f32_vec_type) : 1;
   struct lp_type i32_type = lp_type_int_vec(32, 32 * length);
   struct lp_type i16_type = lp_type_int_vec(16, 16 * length);
   LLVMValueRef result;

   if (util_cpu_caps.has_f16c &&
       (length == 4 || length == 8)) {
      struct lp_type i168_type = lp_type_int_vec(16, 16 * 8);
      unsigned mode = 3; /* same as LP_BUILD_ROUND_TRUNCATE */
      LLVMTypeRef i32t = LLVMInt32TypeInContext(gallivm->context);
      const char *intrinsic = NULL;
      if (length == 4) {
         intrinsic = "llvm.x86.vcvtps2ph.128";
      }
      else {
         intrinsic = "llvm.x86.vcvtps2ph.256";
      }
      result = lp_build_intrinsic_binary(builder, intrinsic,
                                         lp_build_vec_type(gallivm, i168_type),
                                         src, LLVMConstInt(i32t, mode, 0));
      if (length == 4) {
         result = lp_build_extract_range(gallivm, result, 0, 4);
      }
   }

   else {
      result = lp_build_float_to_smallfloat(gallivm, i32_type, src, 10, 5, 0, true);
      /* Convert int32 vector to int16 vector by trunc (might generate bad code) */
      result = LLVMBuildTrunc(builder, result, lp_build_vec_type(gallivm, i16_type), "");
   }

   /*
    * Debugging code.
    */
   if (0) {
     LLVMTypeRef i32t = LLVMInt32TypeInContext(gallivm->context);
     LLVMTypeRef i16t = LLVMInt16TypeInContext(gallivm->context);
     LLVMTypeRef f32t = LLVMFloatTypeInContext(gallivm->context);
     LLVMValueRef ref_result = LLVMGetUndef(LLVMVectorType(i16t, length));
     unsigned i;

     LLVMTypeRef func_type = LLVMFunctionType(i16t, &f32t, 1, 0);
     LLVMValueRef func = lp_build_const_int_pointer(gallivm, func_to_pointer((func_pointer)util_float_to_half));
     func = LLVMBuildBitCast(builder, func, LLVMPointerType(func_type, 0), "util_float_to_half");

     for (i = 0; i < length; ++i) {
        LLVMValueRef index = LLVMConstInt(i32t, i, 0);
        LLVMValueRef f32 = LLVMBuildExtractElement(builder, src, index, "");
#if 0
        /* XXX: not really supported by backends */
        LLVMValueRef f16 = lp_build_intrinsic_unary(builder, "llvm.convert.to.fp16", i16t, f32);
#else
        LLVMValueRef f16 = LLVMBuildCall(builder, func, &f32, 1, "");
#endif
        ref_result = LLVMBuildInsertElement(builder, ref_result, f16, index, "");
     }

     lp_build_print_value(gallivm, "src  = ", src);
     lp_build_print_value(gallivm, "llvm = ", result);
     lp_build_print_value(gallivm, "util = ", ref_result);
     lp_build_printf(gallivm, "\n");
  }

   return result;
}


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
 *
 * Ex: src = { float, float, float, float }
 * return { i32, i32, i32, i32 } where each value is in [0, 2^dst_width-1].
 */
LLVMValueRef
lp_build_clamped_float_to_unsigned_norm(struct gallivm_state *gallivm,
                                        struct lp_type src_type,
                                        unsigned dst_width,
                                        LLVMValueRef src)
{
   LLVMBuilderRef builder = gallivm->builder;
   LLVMTypeRef int_vec_type = lp_build_int_vec_type(gallivm, src_type);
   LLVMValueRef res;
   unsigned mantissa;

   assert(src_type.floating);
   assert(dst_width <= src_type.width);
   src_type.sign = FALSE;

   mantissa = lp_mantissa(src_type);

   if (dst_width <= mantissa) {
      /*
       * Apply magic coefficients that will make the desired result to appear
       * in the lowest significant bits of the mantissa, with correct rounding.
       *
       * This only works if the destination width fits in the mantissa.
       */

      unsigned long long ubound;
      unsigned long long mask;
      double scale;
      double bias;

      ubound = (1ULL << dst_width);
      mask = ubound - 1;
      scale = (double)mask/ubound;
      bias = (double)(1ULL << (mantissa - dst_width));

      res = LLVMBuildFMul(builder, src, lp_build_const_vec(gallivm, src_type, scale), "");
      /* instead of fadd/and could (with sse2) just use lp_build_iround */
      res = LLVMBuildFAdd(builder, res, lp_build_const_vec(gallivm, src_type, bias), "");
      res = LLVMBuildBitCast(builder, res, int_vec_type, "");
      res = LLVMBuildAnd(builder, res,
                         lp_build_const_int_vec(gallivm, src_type, mask), "");
   }
   else if (dst_width == (mantissa + 1)) {
      /*
       * The destination width matches exactly what can be represented in
       * floating point (i.e., mantissa + 1 bits). Even so correct rounding
       * still needs to be applied (only for numbers in [0.5-1.0] would
       * conversion using truncation after scaling be sufficient).
       */
      double scale;
      struct lp_build_context uf32_bld;

      lp_build_context_init(&uf32_bld, gallivm, src_type);
      scale = (double)((1ULL << dst_width) - 1);

      res = LLVMBuildFMul(builder, src,
                          lp_build_const_vec(gallivm, src_type, scale), "");
      res = lp_build_iround(&uf32_bld, res);
   }
   else {
      /*
       * The destination exceeds what can be represented in the floating point.
       * So multiply by the largest power two we get away with, and when
       * subtract the most significant bit to rescale to normalized values.
       *
       * The largest power of two factor we can get away is
       * (1 << (src_type.width - 1)), because we need to use signed . In theory it
       * should be (1 << (src_type.width - 2)), but IEEE 754 rules states
       * INT_MIN should be returned in FPToSI, which is the correct result for
       * values near 1.0!
       *
       * This means we get (src_type.width - 1) correct bits for values near 0.0,
       * and (mantissa + 1) correct bits for values near 1.0. Equally or more
       * important, we also get exact results for 0.0 and 1.0.
       */

      unsigned n = MIN2(src_type.width - 1, dst_width);

      double scale = (double)(1ULL << n);
      unsigned lshift = dst_width - n;
      unsigned rshift = n;
      LLVMValueRef lshifted;
      LLVMValueRef rshifted;

      res = LLVMBuildFMul(builder, src,
                          lp_build_const_vec(gallivm, src_type, scale), "");
      res = LLVMBuildFPToSI(builder, res, int_vec_type, "");

      /*
       * Align the most significant bit to its final place.
       *
       * This will cause 1.0 to overflow to 0, but the later adjustment will
       * get it right.
       */
      if (lshift) {
         lshifted = LLVMBuildShl(builder, res,
                                 lp_build_const_int_vec(gallivm, src_type,
                                                        lshift), "");
      } else {
         lshifted = res;
      }

      /*
       * Align the most significant bit to the right.
       */
      rshifted =  LLVMBuildLShr(builder, res,
                                lp_build_const_int_vec(gallivm, src_type, rshift),
                                "");

      /*
       * Subtract the MSB to the LSB, therefore re-scaling from
       * (1 << dst_width) to ((1 << dst_width) - 1).
       */

      res = LLVMBuildSub(builder, lshifted, rshifted, "");
   }

   return res;
}


/**
 * Inverse of lp_build_clamped_float_to_unsigned_norm above.
 * Ex: src = { i32, i32, i32, i32 } with values in range [0, 2^src_width-1]
 * return {float, float, float, float} with values in range [0, 1].
 */
LLVMValueRef
lp_build_unsigned_norm_to_float(struct gallivm_state *gallivm,
                                unsigned src_width,
                                struct lp_type dst_type,
                                LLVMValueRef src)
{
   LLVMBuilderRef builder = gallivm->builder;
   LLVMTypeRef vec_type = lp_build_vec_type(gallivm, dst_type);
   LLVMTypeRef int_vec_type = lp_build_int_vec_type(gallivm, dst_type);
   LLVMValueRef bias_;
   LLVMValueRef res;
   unsigned mantissa;
   unsigned n;
   unsigned long long ubound;
   unsigned long long mask;
   double scale;
   double bias;

   assert(dst_type.floating);

   mantissa = lp_mantissa(dst_type);

   if (src_width <= (mantissa + 1)) {
      /*
       * The source width matches fits what can be represented in floating
       * point (i.e., mantissa + 1 bits). So do a straight multiplication
       * followed by casting. No further rounding is necessary.
       */

      scale = 1.0/(double)((1ULL << src_width) - 1);
      res = LLVMBuildSIToFP(builder, src, vec_type, "");
      res = LLVMBuildFMul(builder, res,
                          lp_build_const_vec(gallivm, dst_type, scale), "");
      return res;
   }
   else {
      /*
       * The source width exceeds what can be represented in floating
       * point. So truncate the incoming values.
       */

      n = MIN2(mantissa, src_width);

      ubound = ((unsigned long long)1 << n);
      mask = ubound - 1;
      scale = (double)ubound/mask;
      bias = (double)((unsigned long long)1 << (mantissa - n));

      res = src;

      if (src_width > mantissa) {
         int shift = src_width - mantissa;
         res = LLVMBuildLShr(builder, res,
                             lp_build_const_int_vec(gallivm, dst_type, shift), "");
      }

      bias_ = lp_build_const_vec(gallivm, dst_type, bias);

      res = LLVMBuildOr(builder,
                        res,
                        LLVMBuildBitCast(builder, bias_, int_vec_type, ""), "");

      res = LLVMBuildBitCast(builder, res, vec_type, "");

      res = LLVMBuildFSub(builder, res, bias_, "");
      res = LLVMBuildFMul(builder, res, lp_build_const_vec(gallivm, dst_type, scale), "");
   }

   return res;
}


/**
 * Pick a suitable num_dsts for lp_build_conv to ensure optimal cases are used.
 *
 * Returns the number of dsts created from src
 */
int lp_build_conv_auto(struct gallivm_state *gallivm,
                       struct lp_type src_type,
                       struct lp_type* dst_type,
                       const LLVMValueRef *src,
                       unsigned num_srcs,
                       LLVMValueRef *dst)
{
   int i;
   int num_dsts = num_srcs;

   if (src_type.floating == dst_type->floating &&
       src_type.width == dst_type->width &&
       src_type.length == dst_type->length &&
       src_type.fixed == dst_type->fixed &&
       src_type.norm == dst_type->norm &&
       src_type.sign == dst_type->sign)
      return num_dsts;

   /* Special case 4x4f -> 1x16ub or 2x8f -> 1x16ub
    */
   if (src_type.floating == 1 &&
       src_type.fixed    == 0 &&
       src_type.sign     == 1 &&
       src_type.norm     == 0 &&
       src_type.width    == 32 &&

       dst_type->floating == 0 &&
       dst_type->fixed    == 0 &&
       dst_type->sign     == 0 &&
       dst_type->norm     == 1 &&
       dst_type->width    == 8)
   {
      /* Special case 4x4f --> 1x16ub */
      if (src_type.length == 4 &&
          util_cpu_caps.has_sse2)
      {
         num_dsts = (num_srcs + 3) / 4;
         dst_type->length = num_srcs * 4 >= 16 ? 16 : num_srcs * 4;

         lp_build_conv(gallivm, src_type, *dst_type, src, num_srcs, dst, num_dsts);
         return num_dsts;
      }

      /* Special case 2x8f --> 1x16ub */
      if (src_type.length == 8 &&
          util_cpu_caps.has_avx)
      {
         num_dsts = (num_srcs + 1) / 2;
         dst_type->length = num_srcs * 8 >= 16 ? 16 : num_srcs * 8;

         lp_build_conv(gallivm, src_type, *dst_type, src, num_srcs, dst, num_dsts);
         return num_dsts;
      }
   }

   /* lp_build_resize does not support M:N */
   if (src_type.width == dst_type->width) {
      lp_build_conv(gallivm, src_type, *dst_type, src, num_srcs, dst, num_dsts);
   } else {
      for (i = 0; i < num_srcs; ++i) {
         lp_build_conv(gallivm, src_type, *dst_type, &src[i], 1, &dst[i], 1);
      }
   }

   return num_dsts;
}


/**
 * Generic type conversion.
 *
 * TODO: Take a precision argument, or even better, add a new precision member
 * to the lp_type union.
 */
void
lp_build_conv(struct gallivm_state *gallivm,
              struct lp_type src_type,
              struct lp_type dst_type,
              const LLVMValueRef *src, unsigned num_srcs,
              LLVMValueRef *dst, unsigned num_dsts)
{
   LLVMBuilderRef builder = gallivm->builder;
   struct lp_type tmp_type;
   LLVMValueRef tmp[LP_MAX_VECTOR_LENGTH];
   unsigned num_tmps;
   unsigned i;

   /* We must not loose or gain channels. Only precision */
   assert(src_type.length * num_srcs == dst_type.length * num_dsts);

   assert(src_type.length <= LP_MAX_VECTOR_LENGTH);
   assert(dst_type.length <= LP_MAX_VECTOR_LENGTH);
   assert(num_srcs <= LP_MAX_VECTOR_LENGTH);
   assert(num_dsts <= LP_MAX_VECTOR_LENGTH);

   tmp_type = src_type;
   for(i = 0; i < num_srcs; ++i) {
      assert(lp_check_value(src_type, src[i]));
      tmp[i] = src[i];
   }
   num_tmps = num_srcs;


   /* Special case 4x4f --> 1x16ub, 2x4f -> 1x8ub, 1x4f -> 1x4ub
    */
   if (src_type.floating == 1 &&
       src_type.fixed    == 0 &&
       src_type.sign     == 1 &&
       src_type.norm     == 0 &&
       src_type.width    == 32 &&
       src_type.length   == 4 &&

       dst_type.floating == 0 &&
       dst_type.fixed    == 0 &&
       dst_type.sign     == 0 &&
       dst_type.norm     == 1 &&
       dst_type.width    == 8 &&

       ((dst_type.length == 16 && 4 * num_dsts == num_srcs) ||
        (num_dsts == 1 && dst_type.length * num_srcs == 16 && num_srcs != 3)) &&

       util_cpu_caps.has_sse2)
   {
      struct lp_build_context bld;
      struct lp_type int16_type, int32_type;
      struct lp_type dst_type_ext = dst_type;
      LLVMValueRef const_255f;
      unsigned i, j;

      lp_build_context_init(&bld, gallivm, src_type);

      dst_type_ext.length = 16;
      int16_type = int32_type = dst_type_ext;

      int16_type.width *= 2;
      int16_type.length /= 2;
      int16_type.sign = 1;

      int32_type.width *= 4;
      int32_type.length /= 4;
      int32_type.sign = 1;

      const_255f = lp_build_const_vec(gallivm, src_type, 255.0f);

      for (i = 0; i < num_dsts; ++i, src += 4) {
         LLVMValueRef lo, hi;

         for (j = 0; j < dst_type.length / 4; ++j) {
            tmp[j] = LLVMBuildFMul(builder, src[j], const_255f, "");
            tmp[j] = lp_build_iround(&bld, tmp[j]);
         }

         if (num_srcs == 1) {
            tmp[1] = tmp[0];
         }

         /* relying on clamping behavior of sse2 intrinsics here */
         lo = lp_build_pack2(gallivm, int32_type, int16_type, tmp[0], tmp[1]);

         if (num_srcs < 4) {
            hi = lo;
         }
         else {
            hi = lp_build_pack2(gallivm, int32_type, int16_type, tmp[2], tmp[3]);
         }
         dst[i] = lp_build_pack2(gallivm, int16_type, dst_type_ext, lo, hi);
      }
      if (num_srcs < 4) {
         dst[0] = lp_build_extract_range(gallivm, dst[0], 0, dst_type.length);
      }

      return; 
   }

   /* Special case 2x8f --> 1x16ub, 1x8f ->1x8ub
    */
   else if (src_type.floating == 1 &&
      src_type.fixed    == 0 &&
      src_type.sign     == 1 &&
      src_type.norm     == 0 &&
      src_type.width    == 32 &&
      src_type.length   == 8 &&

      dst_type.floating == 0 &&
      dst_type.fixed    == 0 &&
      dst_type.sign     == 0 &&
      dst_type.norm     == 1 &&
      dst_type.width    == 8 &&

      ((dst_type.length == 16 && 2 * num_dsts == num_srcs) ||
       (num_dsts == 1 && dst_type.length * num_srcs == 8)) &&

      util_cpu_caps.has_avx) {

      struct lp_build_context bld;
      struct lp_type int16_type, int32_type;
      struct lp_type dst_type_ext = dst_type;
      LLVMValueRef const_255f;
      unsigned i;

      lp_build_context_init(&bld, gallivm, src_type);

      dst_type_ext.length = 16;
      int16_type = int32_type = dst_type_ext;

      int16_type.width *= 2;
      int16_type.length /= 2;
      int16_type.sign = 1;

      int32_type.width *= 4;
      int32_type.length /= 4;
      int32_type.sign = 1;

      const_255f = lp_build_const_vec(gallivm, src_type, 255.0f);

      for (i = 0; i < num_dsts; ++i, src += 2) {
         LLVMValueRef lo, hi, a, b;

         a = LLVMBuildFMul(builder, src[0], const_255f, "");
         a = lp_build_iround(&bld, a);
         tmp[0] = lp_build_extract_range(gallivm, a, 0, 4);
         tmp[1] = lp_build_extract_range(gallivm, a, 4, 4);
         /* relying on clamping behavior of sse2 intrinsics here */
         lo = lp_build_pack2(gallivm, int32_type, int16_type, tmp[0], tmp[1]);

         if (num_srcs == 1) {
            hi = lo;
         }
         else {
            b = LLVMBuildFMul(builder, src[1], const_255f, "");
            b = lp_build_iround(&bld, b);
            tmp[2] = lp_build_extract_range(gallivm, b, 0, 4);
            tmp[3] = lp_build_extract_range(gallivm, b, 4, 4);
            hi = lp_build_pack2(gallivm, int32_type, int16_type, tmp[2], tmp[3]);

         }
         dst[i] = lp_build_pack2(gallivm, int16_type, dst_type_ext, lo, hi);
      }

      if (num_srcs == 1) {
         dst[0] = lp_build_extract_range(gallivm, dst[0], 0, dst_type.length);
      }

      return;
   }

   /* Special case -> 16bit half-float
    */
   else if (dst_type.floating && dst_type.width == 16)
   {
      /* Only support src as 32bit float currently */
      assert(src_type.floating && src_type.width == 32);

      for(i = 0; i < num_tmps; ++i)
         dst[i] = lp_build_float_to_half(gallivm, tmp[i]);

      return;
   }

   /* Pre convert half-floats to floats
    */
   else if (src_type.floating && src_type.width == 16)
   {
      for(i = 0; i < num_tmps; ++i)
         tmp[i] = lp_build_half_to_float(gallivm, tmp[i]);

      tmp_type.width = 32;
   }

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

      lp_build_context_init(&bld, gallivm, tmp_type);

      if(src_min < dst_min) {
         if(dst_min == 0.0)
            thres = bld.zero;
         else
            thres = lp_build_const_vec(gallivm, src_type, dst_min);
         for(i = 0; i < num_tmps; ++i)
            tmp[i] = lp_build_max(&bld, tmp[i], thres);
      }

      if(src_max > dst_max) {
         if(dst_max == 1.0)
            thres = bld.one;
         else
            thres = lp_build_const_vec(gallivm, src_type, dst_max);
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
            tmp[i] = lp_build_clamped_float_to_unsigned_norm(gallivm,
                                                             tmp_type,
                                                             dst_type.width,
                                                             tmp[i]);
         }
         tmp_type.floating = FALSE;
      }
      else {
         double dst_scale = lp_const_scale(dst_type);

         if (dst_scale != 1.0) {
            LLVMValueRef scale = lp_build_const_vec(gallivm, tmp_type, dst_scale);
            for(i = 0; i < num_tmps; ++i)
               tmp[i] = LLVMBuildFMul(builder, tmp[i], scale, "");
         }

         /*
          * these functions will use fptosi in some form which won't work
          * with 32bit uint dst. Causes lp_test_conv failures though.
          */
         if (0)
            assert(dst_type.sign || dst_type.width < 32);

         if (dst_type.sign && dst_type.norm && !dst_type.fixed) {
            struct lp_build_context bld;

            lp_build_context_init(&bld, gallivm, tmp_type);
            for(i = 0; i < num_tmps; ++i) {
               tmp[i] = lp_build_iround(&bld, tmp[i]);
            }
            tmp_type.floating = FALSE;
         }
         else {
            LLVMTypeRef tmp_vec_type;

            tmp_type.floating = FALSE;
            tmp_vec_type = lp_build_vec_type(gallivm, tmp_type);
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
   }
   else {
      unsigned src_shift = lp_const_shift(src_type);
      unsigned dst_shift = lp_const_shift(dst_type);
      unsigned src_offset = lp_const_offset(src_type);
      unsigned dst_offset = lp_const_offset(dst_type);
      struct lp_build_context bld;
      lp_build_context_init(&bld, gallivm, tmp_type);

      /* Compensate for different offsets */
      /* sscaled -> unorm and similar would cause negative shift count, skip */
      if (dst_offset > src_offset && src_type.width > dst_type.width && src_shift > 0) {
         for (i = 0; i < num_tmps; ++i) {
            LLVMValueRef shifted;

            shifted = lp_build_shr_imm(&bld, tmp[i], src_shift - 1);
            tmp[i] = LLVMBuildSub(builder, tmp[i], shifted, "");
         }
      }

      if(src_shift > dst_shift) {
         for(i = 0; i < num_tmps; ++i)
            tmp[i] = lp_build_shr_imm(&bld, tmp[i], src_shift - dst_shift);
      }
   }

   /*
    * Truncate or expand bit width
    *
    * No data conversion should happen here, although the sign bits are
    * crucial to avoid bad clamping.
    */

   {
      struct lp_type new_type;

      new_type = tmp_type;
      new_type.sign   = dst_type.sign;
      new_type.width  = dst_type.width;
      new_type.length = dst_type.length;

      lp_build_resize(gallivm, tmp_type, new_type, tmp, num_srcs, tmp, num_dsts);

      tmp_type = new_type;
      num_tmps = num_dsts;
   }

   /*
    * Scale to the widest range
    */

   if(src_type.floating) {
      /* Nothing to do */
   }
   else if(!src_type.floating && dst_type.floating) {
      if(!src_type.fixed && !src_type.sign && src_type.norm) {
         for(i = 0; i < num_tmps; ++i) {
            tmp[i] = lp_build_unsigned_norm_to_float(gallivm,
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
         tmp_vec_type = lp_build_vec_type(gallivm, tmp_type);
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
             LLVMValueRef scale = lp_build_const_vec(gallivm, tmp_type, 1.0/src_scale);
             for(i = 0; i < num_tmps; ++i)
                tmp[i] = LLVMBuildFMul(builder, tmp[i], scale, "");
          }

          /* the formula above will produce value below -1.0 for most negative
           * value but everything seems happy with that hence disable for now */
          if (0 && !src_type.fixed && src_type.norm && src_type.sign) {
             struct lp_build_context bld;

             lp_build_context_init(&bld, gallivm, dst_type);
             for(i = 0; i < num_tmps; ++i) {
                tmp[i] = lp_build_max(&bld, tmp[i],
                                      lp_build_const_vec(gallivm, dst_type, -1.0f));
             }
          }
      }
    }
    else {
       unsigned src_shift = lp_const_shift(src_type);
       unsigned dst_shift = lp_const_shift(dst_type);
       unsigned src_offset = lp_const_offset(src_type);
       unsigned dst_offset = lp_const_offset(dst_type);
       struct lp_build_context bld;
       lp_build_context_init(&bld, gallivm, tmp_type);

       if (src_shift < dst_shift) {
          LLVMValueRef pre_shift[LP_MAX_VECTOR_LENGTH];

          if (dst_shift - src_shift < dst_type.width) {
             for (i = 0; i < num_tmps; ++i) {
                pre_shift[i] = tmp[i];
                tmp[i] = lp_build_shl_imm(&bld, tmp[i], dst_shift - src_shift);
             }
          }
          else {
             /*
              * This happens for things like sscaled -> unorm conversions. Shift
              * counts equal to bit width cause undefined results, so hack around it.
              */
             for (i = 0; i < num_tmps; ++i) {
                pre_shift[i] = tmp[i];
                tmp[i] = lp_build_zero(gallivm, dst_type);
             }
          }

          /* Compensate for different offsets */
          if (dst_offset > src_offset) {
             for (i = 0; i < num_tmps; ++i) {
                tmp[i] = LLVMBuildSub(builder, tmp[i], pre_shift[i], "");
             }
          }
       }
    }

   for(i = 0; i < num_dsts; ++i) {
      dst[i] = tmp[i];
      assert(lp_check_value(dst_type, dst[i]));
   }
}


/**
 * Bit mask conversion.
 *
 * This will convert the integer masks that match the given types.
 *
 * The mask values should 0 or -1, i.e., all bits either set to zero or one.
 * Any other value will likely cause unpredictable results.
 *
 * This is basically a very trimmed down version of lp_build_conv.
 */
void
lp_build_conv_mask(struct gallivm_state *gallivm,
                   struct lp_type src_type,
                   struct lp_type dst_type,
                   const LLVMValueRef *src, unsigned num_srcs,
                   LLVMValueRef *dst, unsigned num_dsts)
{

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

   lp_build_resize(gallivm, src_type, dst_type, src, num_srcs, dst, num_dsts);
}
