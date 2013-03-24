/**************************************************************************
 *
 * Copyright 2013 VMware, Inc.
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
 * Format conversion code for "special" float formats.
 *
 * @author Roland Scheidegger <sroland@vmware.com>
 */


#include "util/u_debug.h"

#include "lp_bld_type.h"
#include "lp_bld_const.h"
#include "lp_bld_arit.h"
#include "lp_bld_bitarit.h"
#include "lp_bld_logic.h"
#include "lp_bld_format.h"


/**
 * Convert float32 to a float-like value with less exponent and mantissa
 * bits. The mantissa is still biased, and the mantissa still has an implied 1,
 * but there's no sign bit.
 *
 * @param src             (vector) float value to convert
 * @param mantissa_bits   the number of mantissa bits
 * @param exponent_bits   the number of exponent bits
 *
 * Unlike float_to_half using accurate method here.
 * This implements round-towards-zero (trunc) hence too large numbers get
 * converted to largest representable number, not infinity.
 * Small numbers may get converted to denorms, depending on normal
 * float denorm handling of the cpu.
 * Note that compared to the references, below, we skip any rounding bias
 * since we do rounding towards zero - OpenGL allows rounding towards zero
 * (though not preferred) and DX10 even seems to require it.
 * Note that this will not do any packing - the value will
 * look like a "rescaled float" (except for Inf/NaN) but be returned
 * as int32.
 *
 * ref http://fgiesen.wordpress.com/2012/03/28/half-to-float-done-quic/
 * ref https://gist.github.com/rygorous/2156668
 */
static LLVMValueRef
lp_build_float_to_smallfloat_nosign(struct gallivm_state *gallivm,
                                    struct lp_type i32_type,
                                    LLVMValueRef src,
                                    unsigned mantissa_bits,
                                    unsigned exponent_bits)
{
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef i32_floatexpmask, i32_smallexpmask, magic, normal;
   LLVMValueRef clamped, tmp, i32_roundmask, small_max, src_abs;
   LLVMValueRef is_nan, is_posinf, is_nan_or_posinf, i32_qnanbit, nan_or_posinf;
   struct lp_type f32_type = lp_type_float_vec(32, 32 * i32_type.length);
   struct lp_build_context f32_bld, i32_bld;
   LLVMValueRef zero = lp_build_const_vec(gallivm, f32_type, 0.0f);

   lp_build_context_init(&f32_bld, gallivm, f32_type);
   lp_build_context_init(&i32_bld, gallivm, i32_type);

   i32_smallexpmask = lp_build_const_int_vec(gallivm, i32_type,
                                             ((1 << exponent_bits) - 1) << 23);
   i32_floatexpmask = lp_build_const_int_vec(gallivm, i32_type, 0xff << 23);

   /* "ordinary" number */
   /* clamp to pos range (can still have sign bit if NaN or negative zero) */
   clamped = lp_build_max(&f32_bld, src, zero);
   clamped = LLVMBuildBitCast(builder, clamped, i32_bld.vec_type, "");
   /* get rid of excess mantissa bits, and while here also potential sign bit */
   i32_roundmask = lp_build_const_int_vec(gallivm, i32_type,
                                          ~((1 << (23 - mantissa_bits)) - 1) |
                                          0x7fffffff);

   tmp = lp_build_and(&i32_bld, clamped, i32_roundmask);
   tmp = LLVMBuildBitCast(builder, tmp, f32_bld.vec_type, "");
   /* bias exponent (and denormalize if necessary) */
   magic = lp_build_const_int_vec(gallivm, i32_type,
                                  ((1 << (exponent_bits - 1)) - 1) << 23);
   magic = LLVMBuildBitCast(builder, magic, f32_bld.vec_type, "");
   normal = lp_build_mul(&f32_bld, tmp, magic);

   /* clamp to max value */
   small_max = lp_build_const_int_vec(gallivm, i32_type,
                                      (((1 << exponent_bits) - 2) << 23) |
                                      (((1 << mantissa_bits) - 1) << (23 - mantissa_bits)));
   small_max = LLVMBuildBitCast(builder, small_max, f32_bld.vec_type, "");
   normal = lp_build_min(&f32_bld, normal, small_max);
   normal = LLVMBuildBitCast(builder, normal, i32_bld.vec_type, "");

   /*
    * handle nan/inf cases
    * a little bit tricky since -Inf -> 0, +Inf -> +Inf, +-Nan -> +Nan
    * Note that on a lucky day, we could simplify this a bit,
    * by just using the max(src, zero) result - this will have -Inf
    * clamped to 0, and MIGHT preserve the NaNs.
    */
   src_abs = lp_build_abs(&f32_bld, src);
   src_abs = LLVMBuildBitCast(builder, src_abs, i32_bld.vec_type, "");
   src = LLVMBuildBitCast(builder, src, i32_bld.vec_type, "");
   is_nan = lp_build_compare(gallivm, i32_type, PIPE_FUNC_GREATER,
                             src_abs, i32_floatexpmask);
   is_posinf = lp_build_compare(gallivm, i32_type, PIPE_FUNC_EQUAL,
                                src, i32_floatexpmask);
   is_nan_or_posinf = lp_build_and(&i32_bld, is_nan, is_posinf);
   /* could also set more mantissa bits but need at least the highest mantissa bit */
   i32_qnanbit = lp_build_const_vec(gallivm, i32_type, 1 << 22);
   /* combine maxexp with qnanbit */
   nan_or_posinf = lp_build_or(&i32_bld, i32_smallexpmask,
                               lp_build_and(&i32_bld, is_nan, i32_qnanbit));

   return lp_build_select(&i32_bld, is_nan_or_posinf, nan_or_posinf, normal);
}


/**
 * Convert rgba float SoA values to packed r11g11b10 values.
 *
 * @param src   SoA float (vector) values to convert.
 */
LLVMValueRef
lp_build_float_to_r11g11b10(struct gallivm_state *gallivm,
                            LLVMValueRef *src)
{
   LLVMValueRef dst, rcomp, bcomp, gcomp, shift, mask;
   struct lp_build_context i32_bld;
   LLVMTypeRef src_type = LLVMTypeOf(*src);
   unsigned src_length = LLVMGetTypeKind(src_type) == LLVMVectorTypeKind ?
                            LLVMGetVectorSize(src_type) : 1;
   struct lp_type i32_type = lp_type_int_vec(32, 32 * src_length);

   lp_build_context_init(&i32_bld, gallivm, i32_type);

   /* "rescale" - this does the actual conversion except the packing */
   rcomp = lp_build_float_to_smallfloat_nosign(gallivm, i32_type, src[0], 6, 5);
   gcomp = lp_build_float_to_smallfloat_nosign(gallivm, i32_type, src[1], 6, 5);
   bcomp = lp_build_float_to_smallfloat_nosign(gallivm, i32_type, src[2], 5, 5);

   /* pack rescaled SoA floats to r11g11b10 AoS values */
   shift = lp_build_const_int_vec(gallivm, i32_type, 23 - 6);
   rcomp = lp_build_shr(&i32_bld, rcomp, shift);

   shift = lp_build_const_int_vec(gallivm, i32_type, 23 - 17);
   mask = lp_build_const_int_vec(gallivm, i32_type, 0x7ff << 11);
   gcomp = lp_build_shr(&i32_bld, gcomp, shift);
   gcomp = lp_build_and(&i32_bld, gcomp, mask);

   shift = lp_build_const_int_vec(gallivm, i32_type, 27 - 23);
   mask = lp_build_const_int_vec(gallivm, i32_type, 0x3ff << 22);
   bcomp = lp_build_shl(&i32_bld, bcomp, shift);
   bcomp = lp_build_and(&i32_bld, bcomp, mask);

   dst = lp_build_or(&i32_bld, rcomp, gcomp);
   return lp_build_or(&i32_bld, dst, bcomp);
}


/**
 * Convert a float-like value with less exponent and mantissa
 * bits than a normal float32 to a float32. The mantissa of
 * the source value is assumed to have an implied 1, and the exponent
 * is biased. There are no negative values.
 * The source value to extract must be in a 32bit int.
 * While this helper is generic, it is only ever going to be useful for
 * r11g11b10 (no other common format exists with the same properties).
 *
 * @param src             (vector) value to convert
 * @param mantissa_bits   the number of mantissa bits
 * @param exponent_bits   the number of exponent bits
 * @param mantissa_start  the bit start position of the packed component
 *
 * ref http://fgiesen.wordpress.com/2012/03/28/half-to-float-done-quic/
 * ref https://gist.github.com/rygorous/2156668
 */
static LLVMValueRef
lp_build_smallfloat_nosign_to_float(struct gallivm_state *gallivm,
                                    struct lp_type f32_type,
                                    LLVMValueRef src,
                                    unsigned mantissa_bits,
                                    unsigned exponent_bits,
                                    unsigned mantissa_start)
{
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef smallexpmask, i32_floatexpmask, magic;
   LLVMValueRef wasinfnan, tmp, res, shift, mask;
   unsigned exponent_start = mantissa_start + mantissa_bits;
   struct lp_type i32_type = lp_type_int_vec(32, 32 * f32_type.length);
   struct lp_build_context f32_bld, i32_bld;

   lp_build_context_init(&f32_bld, gallivm, f32_type);
   lp_build_context_init(&i32_bld, gallivm, i32_type);

   /* extract the component to "float position" */
   if (exponent_start < 23) {
      shift = lp_build_const_int_vec(gallivm, i32_type, 23 - exponent_start);
      src = lp_build_shl(&i32_bld, src, shift);
   }
   else {
      shift = lp_build_const_int_vec(gallivm, i32_type, exponent_start - 23);
      src = lp_build_shr(&i32_bld, src, shift);
   }
   mask = lp_build_const_int_vec(gallivm, i32_type,
                                 ((1 << (mantissa_bits + exponent_bits)) - 1) <<
                                 (23 - mantissa_bits));
   src = lp_build_and(&i32_bld, src, mask);
   src = LLVMBuildBitCast(builder, src, f32_bld.vec_type, "");

   /* now do the actual scaling */
   smallexpmask = lp_build_const_int_vec(gallivm, i32_type,
                                         ((1 << exponent_bits) - 1) << 23);
   i32_floatexpmask = lp_build_const_int_vec(gallivm, i32_type, 0xff << 23);
   /*
    * magic number has exponent new exp bias + (new exp bias - old exp bias),
    * mantissa is 0.
    */
   magic = lp_build_const_int_vec(gallivm, i32_type,
                                  (255 - (1 << (exponent_bits - 1))) << 23);
   magic = LLVMBuildBitCast(builder, magic, f32_bld.vec_type, "");

   /* adjust exponent and fix denorms */
   res = lp_build_mul(&f32_bld, src, magic);

   /*
    * if exp was max (== NaN or Inf) set new exp to max (keep mantissa),
    * so a simple "or" will do (because exp adjust will leave mantissa intact)
    */
   /* use float compare (better for AVX 8-wide / no AVX2 though otherwise should use int) */
   smallexpmask = LLVMBuildBitCast(builder, magic, f32_bld.vec_type, "");
   wasinfnan = lp_build_compare(gallivm, f32_type, PIPE_FUNC_GEQUAL, src, smallexpmask);
   res = LLVMBuildBitCast(builder, res, i32_bld.vec_type, "");
   tmp = lp_build_and(&i32_bld, i32_floatexpmask, wasinfnan);
   res = lp_build_or(&i32_bld, tmp, res);

   return LLVMBuildBitCast(builder, res, f32_bld.vec_type, "");
}


/**
 * Convert packed float format (r11g11b10) value(s) to rgba float SoA values.
 *
 * @param src   packed AoS r11g11b10 values (as (vector) int32)
 * @param dst   pointer to the SoA result values
 */
void
lp_build_r11g11b10_to_float(struct gallivm_state *gallivm,
                            LLVMValueRef src,
                            LLVMValueRef *dst)
{
   LLVMTypeRef src_type = LLVMTypeOf(src);
   unsigned src_length = LLVMGetTypeKind(src_type) == LLVMVectorTypeKind ?
                            LLVMGetVectorSize(src_type) : 1;
   struct lp_type f32_type = lp_type_float_vec(32, 32 * src_length);

   dst[0] = lp_build_smallfloat_nosign_to_float(gallivm, f32_type, src, 6, 5, 0);
   dst[1] = lp_build_smallfloat_nosign_to_float(gallivm, f32_type, src, 6, 5, 11);
   dst[2] = lp_build_smallfloat_nosign_to_float(gallivm, f32_type, src, 5, 5, 22);

   /* Just set alpha to one */
   dst[3] = lp_build_one(gallivm, f32_type);
}


static LLVMValueRef
lp_build_rgb9_to_float_helper(struct gallivm_state *gallivm,
                              struct lp_type f32_type,
                              LLVMValueRef src,
                              LLVMValueRef scale,
                              unsigned mantissa_start)
{
   LLVMValueRef shift, mask;

   struct lp_type i32_type = lp_type_int_vec(32, 32 * f32_type.length);
   struct lp_build_context i32_bld, f32_bld;

   lp_build_context_init(&i32_bld, gallivm, i32_type);
   lp_build_context_init(&f32_bld, gallivm, f32_type);

   /*
    * This is much easier as other weirdo float formats, since
    * there's no sign, no Inf/NaN, and there's nothing special
    * required for normals/denormals neither (as without the implied one
    * for the mantissa for other formats, everything looks like a denormal).
    * So just do (float)comp_bits * scale
    */
   shift = lp_build_const_int_vec(gallivm, i32_type, mantissa_start);
   mask = lp_build_const_int_vec(gallivm, i32_type, 0x1ff);
   src = lp_build_shr(&i32_bld, src, shift);
   src = lp_build_and(&i32_bld, src, mask);
   src = lp_build_int_to_float(&f32_bld, src);
   return lp_build_mul(&f32_bld, src, scale);
}


/**
 * Convert shared exponent format (rgb9e5) value(s) to rgba float SoA values.
 *
 * @param src   packed AoS rgb9e5 values (as (vector) int32)
 * @param dst   pointer to the SoA result values
 */
void
lp_build_rgb9e5_to_float(struct gallivm_state *gallivm,
                         LLVMValueRef src,
                         LLVMValueRef *dst)
{
   LLVMBuilderRef builder = gallivm->builder;
   LLVMTypeRef src_type = LLVMTypeOf(src);
   LLVMValueRef shift, scale, bias, exp;
   unsigned src_length = LLVMGetTypeKind(src_type) == LLVMVectorTypeKind ?
                            LLVMGetVectorSize(src_type) : 1;
   struct lp_type i32_type = lp_type_int_vec(32, 32 * src_length);
   struct lp_type u32_type = lp_type_uint_vec(32, 32 * src_length);
   struct lp_type f32_type = lp_type_float_vec(32, 32 * src_length);
   struct lp_build_context i32_bld, u32_bld, f32_bld;

   lp_build_context_init(&i32_bld, gallivm, i32_type);
   lp_build_context_init(&u32_bld, gallivm, u32_type);
   lp_build_context_init(&f32_bld, gallivm, f32_type);

   /* extract exponent */
   shift = lp_build_const_int_vec(gallivm, i32_type, 27);
   /* this shift needs to be unsigned otherwise need mask */
   exp = lp_build_shr(&u32_bld, src, shift);

   /*
    * scale factor is 2 ^ (exp - bias)
    * (and additionally corrected here for the mantissa bits)
    * not using shift because
    * a) don't have vector shift in a lot of cases
    * b) shift direction changes hence need 2 shifts + conditional
    *    (or rotate instruction which is even more rare (for instance XOP))
    * so use whacky float 2 ^ function instead manipulating exponent
    * (saves us the float conversion at the end too)
    */
   bias = lp_build_const_int_vec(gallivm, i32_type, 127 - (15 + 9));
   scale = lp_build_add(&i32_bld, exp, bias);
   shift = lp_build_const_int_vec(gallivm, i32_type, 23);
   scale = lp_build_shl(&i32_bld, scale, shift);
   scale = LLVMBuildBitCast(builder, scale, f32_bld.vec_type, "");

   dst[0] = lp_build_rgb9_to_float_helper(gallivm, f32_type, src, scale, 0);
   dst[1] = lp_build_rgb9_to_float_helper(gallivm, f32_type, src, scale, 9);
   dst[2] = lp_build_rgb9_to_float_helper(gallivm, f32_type, src, scale, 18);

   /* Just set alpha to one */
   dst[3] = f32_bld.one;
}
