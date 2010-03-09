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


#include "util/u_memory.h"
#include "util/u_debug.h"
#include "util/u_math.h"
#include "util/u_string.h"
#include "util/u_cpu_detect.h"

#include "lp_bld_type.h"
#include "lp_bld_const.h"
#include "lp_bld_intr.h"
#include "lp_bld_logic.h"
#include "lp_bld_pack.h"
#include "lp_bld_debug.h"
#include "lp_bld_arit.h"


/**
 * Generate min(a, b)
 * No checks for special case values of a or b = 1 or 0 are done.
 */
static LLVMValueRef
lp_build_min_simple(struct lp_build_context *bld,
                    LLVMValueRef a,
                    LLVMValueRef b)
{
   const struct lp_type type = bld->type;
   const char *intrinsic = NULL;
   LLVMValueRef cond;

   /* TODO: optimize the constant case */

   if(type.width * type.length == 128) {
      if(type.floating) {
         if(type.width == 32 && util_cpu_caps.has_sse)
            intrinsic = "llvm.x86.sse.min.ps";
         if(type.width == 64 && util_cpu_caps.has_sse2)
            intrinsic = "llvm.x86.sse2.min.pd";
      }
      else {
         if(type.width == 8 && !type.sign && util_cpu_caps.has_sse2)
            intrinsic = "llvm.x86.sse2.pminu.b";
         if(type.width == 8 && type.sign && util_cpu_caps.has_sse4_1)
            intrinsic = "llvm.x86.sse41.pminsb";
         if(type.width == 16 && !type.sign && util_cpu_caps.has_sse4_1)
            intrinsic = "llvm.x86.sse41.pminuw";
         if(type.width == 16 && type.sign && util_cpu_caps.has_sse2)
            intrinsic = "llvm.x86.sse2.pmins.w";
         if(type.width == 32 && !type.sign && util_cpu_caps.has_sse4_1)
            intrinsic = "llvm.x86.sse41.pminud";
         if(type.width == 32 && type.sign && util_cpu_caps.has_sse4_1)
            intrinsic = "llvm.x86.sse41.pminsd";
      }
   }

   if(intrinsic)
      return lp_build_intrinsic_binary(bld->builder, intrinsic, lp_build_vec_type(bld->type), a, b);

   cond = lp_build_cmp(bld, PIPE_FUNC_LESS, a, b);
   return lp_build_select(bld, cond, a, b);
}


/**
 * Generate max(a, b)
 * No checks for special case values of a or b = 1 or 0 are done.
 */
static LLVMValueRef
lp_build_max_simple(struct lp_build_context *bld,
                    LLVMValueRef a,
                    LLVMValueRef b)
{
   const struct lp_type type = bld->type;
   const char *intrinsic = NULL;
   LLVMValueRef cond;

   /* TODO: optimize the constant case */

   if(type.width * type.length == 128) {
      if(type.floating) {
         if(type.width == 32 && util_cpu_caps.has_sse)
            intrinsic = "llvm.x86.sse.max.ps";
         if(type.width == 64 && util_cpu_caps.has_sse2)
            intrinsic = "llvm.x86.sse2.max.pd";
      }
      else {
         if(type.width == 8 && !type.sign && util_cpu_caps.has_sse2)
            intrinsic = "llvm.x86.sse2.pmaxu.b";
         if(type.width == 8 && type.sign && util_cpu_caps.has_sse4_1)
            intrinsic = "llvm.x86.sse41.pmaxsb";
         if(type.width == 16 && !type.sign && util_cpu_caps.has_sse4_1)
            intrinsic = "llvm.x86.sse41.pmaxuw";
         if(type.width == 16 && type.sign && util_cpu_caps.has_sse2)
            intrinsic = "llvm.x86.sse2.pmaxs.w";
         if(type.width == 32 && !type.sign && util_cpu_caps.has_sse4_1)
            intrinsic = "llvm.x86.sse41.pmaxud";
         if(type.width == 32 && type.sign && util_cpu_caps.has_sse4_1)
            intrinsic = "llvm.x86.sse41.pmaxsd";
      }
   }

   if(intrinsic)
      return lp_build_intrinsic_binary(bld->builder, intrinsic, lp_build_vec_type(bld->type), a, b);

   cond = lp_build_cmp(bld, PIPE_FUNC_GREATER, a, b);
   return lp_build_select(bld, cond, a, b);
}


/**
 * Generate 1 - a, or ~a depending on bld->type.
 */
LLVMValueRef
lp_build_comp(struct lp_build_context *bld,
              LLVMValueRef a)
{
   const struct lp_type type = bld->type;

   if(a == bld->one)
      return bld->zero;
   if(a == bld->zero)
      return bld->one;

   if(type.norm && !type.floating && !type.fixed && !type.sign) {
      if(LLVMIsConstant(a))
         return LLVMConstNot(a);
      else
         return LLVMBuildNot(bld->builder, a, "");
   }

   if(LLVMIsConstant(a))
      return LLVMConstSub(bld->one, a);
   else
      return LLVMBuildSub(bld->builder, bld->one, a, "");
}


/**
 * Generate a + b
 */
LLVMValueRef
lp_build_add(struct lp_build_context *bld,
             LLVMValueRef a,
             LLVMValueRef b)
{
   const struct lp_type type = bld->type;
   LLVMValueRef res;

   if(a == bld->zero)
      return b;
   if(b == bld->zero)
      return a;
   if(a == bld->undef || b == bld->undef)
      return bld->undef;

   if(bld->type.norm) {
      const char *intrinsic = NULL;

      if(a == bld->one || b == bld->one)
        return bld->one;

      if(util_cpu_caps.has_sse2 &&
         type.width * type.length == 128 &&
         !type.floating && !type.fixed) {
         if(type.width == 8)
            intrinsic = type.sign ? "llvm.x86.sse2.padds.b" : "llvm.x86.sse2.paddus.b";
         if(type.width == 16)
            intrinsic = type.sign ? "llvm.x86.sse2.padds.w" : "llvm.x86.sse2.paddus.w";
      }
   
      if(intrinsic)
         return lp_build_intrinsic_binary(bld->builder, intrinsic, lp_build_vec_type(bld->type), a, b);
   }

   if(LLVMIsConstant(a) && LLVMIsConstant(b))
      res = LLVMConstAdd(a, b);
   else
      res = LLVMBuildAdd(bld->builder, a, b, "");

   /* clamp to ceiling of 1.0 */
   if(bld->type.norm && (bld->type.floating || bld->type.fixed))
      res = lp_build_min_simple(bld, res, bld->one);

   /* XXX clamp to floor of -1 or 0??? */

   return res;
}


/**
 * Generate a - b
 */
LLVMValueRef
lp_build_sub(struct lp_build_context *bld,
             LLVMValueRef a,
             LLVMValueRef b)
{
   const struct lp_type type = bld->type;
   LLVMValueRef res;

   if(b == bld->zero)
      return a;
   if(a == bld->undef || b == bld->undef)
      return bld->undef;
   if(a == b)
      return bld->zero;

   if(bld->type.norm) {
      const char *intrinsic = NULL;

      if(b == bld->one)
        return bld->zero;

      if(util_cpu_caps.has_sse2 &&
         type.width * type.length == 128 &&
         !type.floating && !type.fixed) {
         if(type.width == 8)
            intrinsic = type.sign ? "llvm.x86.sse2.psubs.b" : "llvm.x86.sse2.psubus.b";
         if(type.width == 16)
            intrinsic = type.sign ? "llvm.x86.sse2.psubs.w" : "llvm.x86.sse2.psubus.w";
      }
   
      if(intrinsic)
         return lp_build_intrinsic_binary(bld->builder, intrinsic, lp_build_vec_type(bld->type), a, b);
   }

   if(LLVMIsConstant(a) && LLVMIsConstant(b))
      res = LLVMConstSub(a, b);
   else
      res = LLVMBuildSub(bld->builder, a, b, "");

   if(bld->type.norm && (bld->type.floating || bld->type.fixed))
      res = lp_build_max_simple(bld, res, bld->zero);

   return res;
}


/**
 * Normalized 8bit multiplication.
 *
 * - alpha plus one
 *
 *     makes the following approximation to the division (Sree)
 *    
 *       a*b/255 ~= (a*(b + 1)) >> 256
 *    
 *     which is the fastest method that satisfies the following OpenGL criteria
 *    
 *       0*0 = 0 and 255*255 = 255
 *
 * - geometric series
 *
 *     takes the geometric series approximation to the division
 *
 *       t/255 = (t >> 8) + (t >> 16) + (t >> 24) ..
 *
 *     in this case just the first two terms to fit in 16bit arithmetic
 *
 *       t/255 ~= (t + (t >> 8)) >> 8
 *
 *     note that just by itself it doesn't satisfies the OpenGL criteria, as
 *     255*255 = 254, so the special case b = 255 must be accounted or roundoff
 *     must be used
 *
 * - geometric series plus rounding
 *
 *     when using a geometric series division instead of truncating the result
 *     use roundoff in the approximation (Jim Blinn)
 *
 *       t/255 ~= (t + (t >> 8) + 0x80) >> 8
 *
 *     achieving the exact results
 *
 * @sa Alvy Ray Smith, Image Compositing Fundamentals, Tech Memo 4, Aug 15, 1995, 
 *     ftp://ftp.alvyray.com/Acrobat/4_Comp.pdf
 * @sa Michael Herf, The "double blend trick", May 2000, 
 *     http://www.stereopsis.com/doubleblend.html
 */
static LLVMValueRef
lp_build_mul_u8n(LLVMBuilderRef builder,
                 struct lp_type i16_type,
                 LLVMValueRef a, LLVMValueRef b)
{
   LLVMValueRef c8;
   LLVMValueRef ab;

   c8 = lp_build_int_const_scalar(i16_type, 8);
   
#if 0
   
   /* a*b/255 ~= (a*(b + 1)) >> 256 */
   b = LLVMBuildAdd(builder, b, lp_build_int_const_scalar(i16_type, 1), "");
   ab = LLVMBuildMul(builder, a, b, "");

#else
   
   /* ab/255 ~= (ab + (ab >> 8) + 0x80) >> 8 */
   ab = LLVMBuildMul(builder, a, b, "");
   ab = LLVMBuildAdd(builder, ab, LLVMBuildLShr(builder, ab, c8, ""), "");
   ab = LLVMBuildAdd(builder, ab, lp_build_int_const_scalar(i16_type, 0x80), "");

#endif
   
   ab = LLVMBuildLShr(builder, ab, c8, "");

   return ab;
}


/**
 * Generate a * b
 */
LLVMValueRef
lp_build_mul(struct lp_build_context *bld,
             LLVMValueRef a,
             LLVMValueRef b)
{
   const struct lp_type type = bld->type;
   LLVMValueRef shift;
   LLVMValueRef res;

   if(a == bld->zero)
      return bld->zero;
   if(a == bld->one)
      return b;
   if(b == bld->zero)
      return bld->zero;
   if(b == bld->one)
      return a;
   if(a == bld->undef || b == bld->undef)
      return bld->undef;

   if(!type.floating && !type.fixed && type.norm) {
      if(type.width == 8) {
         struct lp_type i16_type = lp_wider_type(type);
         LLVMValueRef al, ah, bl, bh, abl, abh, ab;

         lp_build_unpack2(bld->builder, type, i16_type, a, &al, &ah);
         lp_build_unpack2(bld->builder, type, i16_type, b, &bl, &bh);

         /* PMULLW, PSRLW, PADDW */
         abl = lp_build_mul_u8n(bld->builder, i16_type, al, bl);
         abh = lp_build_mul_u8n(bld->builder, i16_type, ah, bh);

         ab = lp_build_pack2(bld->builder, i16_type, type, abl, abh);
         
         return ab;
      }

      /* FIXME */
      assert(0);
   }

   if(type.fixed)
      shift = lp_build_int_const_scalar(type, type.width/2);
   else
      shift = NULL;

   if(LLVMIsConstant(a) && LLVMIsConstant(b)) {
      res =  LLVMConstMul(a, b);
      if(shift) {
         if(type.sign)
            res = LLVMConstAShr(res, shift);
         else
            res = LLVMConstLShr(res, shift);
      }
   }
   else {
      res = LLVMBuildMul(bld->builder, a, b, "");
      if(shift) {
         if(type.sign)
            res = LLVMBuildAShr(bld->builder, res, shift, "");
         else
            res = LLVMBuildLShr(bld->builder, res, shift, "");
      }
   }

   return res;
}


/**
 * Small vector x scale multiplication optimization.
 */
LLVMValueRef
lp_build_mul_imm(struct lp_build_context *bld,
                 LLVMValueRef a,
                 int b)
{
   LLVMValueRef factor;

   if(b == 0)
      return bld->zero;

   if(b == 1)
      return a;

   if(b == -1)
      return LLVMBuildNeg(bld->builder, a, "");

   if(b == 2 && bld->type.floating)
      return lp_build_add(bld, a, a);

   if(util_is_pot(b)) {
      unsigned shift = ffs(b) - 1;

      if(bld->type.floating) {
#if 0
         /*
          * Power of two multiplication by directly manipulating the mantissa.
          *
          * XXX: This might not be always faster, it will introduce a small error
          * for multiplication by zero, and it will produce wrong results
          * for Inf and NaN.
          */
         unsigned mantissa = lp_mantissa(bld->type);
         factor = lp_build_int_const_scalar(bld->type, (unsigned long long)shift << mantissa);
         a = LLVMBuildBitCast(bld->builder, a, lp_build_int_vec_type(bld->type), "");
         a = LLVMBuildAdd(bld->builder, a, factor, "");
         a = LLVMBuildBitCast(bld->builder, a, lp_build_vec_type(bld->type), "");
         return a;
#endif
      }
      else {
         factor = lp_build_const_scalar(bld->type, shift);
         return LLVMBuildShl(bld->builder, a, factor, "");
      }
   }

   factor = lp_build_const_scalar(bld->type, (double)b);
   return lp_build_mul(bld, a, factor);
}


/**
 * Generate a / b
 */
LLVMValueRef
lp_build_div(struct lp_build_context *bld,
             LLVMValueRef a,
             LLVMValueRef b)
{
   const struct lp_type type = bld->type;

   if(a == bld->zero)
      return bld->zero;
   if(a == bld->one)
      return lp_build_rcp(bld, b);
   if(b == bld->zero)
      return bld->undef;
   if(b == bld->one)
      return a;
   if(a == bld->undef || b == bld->undef)
      return bld->undef;

   if(LLVMIsConstant(a) && LLVMIsConstant(b))
      return LLVMConstFDiv(a, b);

   if(util_cpu_caps.has_sse && type.width == 32 && type.length == 4)
      return lp_build_mul(bld, a, lp_build_rcp(bld, b));

   return LLVMBuildFDiv(bld->builder, a, b, "");
}


/**
 * Linear interpolation.
 *
 * This also works for integer values with a few caveats.
 *
 * @sa http://www.stereopsis.com/doubleblend.html
 */
LLVMValueRef
lp_build_lerp(struct lp_build_context *bld,
              LLVMValueRef x,
              LLVMValueRef v0,
              LLVMValueRef v1)
{
   LLVMValueRef delta;
   LLVMValueRef res;

   delta = lp_build_sub(bld, v1, v0);

   res = lp_build_mul(bld, x, delta);

   res = lp_build_add(bld, v0, res);

   if(bld->type.fixed)
      /* XXX: This step is necessary for lerping 8bit colors stored on 16bits,
       * but it will be wrong for other uses. Basically we need a more
       * powerful lp_type, capable of further distinguishing the values
       * interpretation from the value storage. */
      res = LLVMBuildAnd(bld->builder, res, lp_build_int_const_scalar(bld->type, (1 << bld->type.width/2) - 1), "");

   return res;
}


LLVMValueRef
lp_build_lerp_2d(struct lp_build_context *bld,
                 LLVMValueRef x,
                 LLVMValueRef y,
                 LLVMValueRef v00,
                 LLVMValueRef v01,
                 LLVMValueRef v10,
                 LLVMValueRef v11)
{
   LLVMValueRef v0 = lp_build_lerp(bld, x, v00, v01);
   LLVMValueRef v1 = lp_build_lerp(bld, x, v10, v11);
   return lp_build_lerp(bld, y, v0, v1);
}


/**
 * Generate min(a, b)
 * Do checks for special cases.
 */
LLVMValueRef
lp_build_min(struct lp_build_context *bld,
             LLVMValueRef a,
             LLVMValueRef b)
{
   if(a == bld->undef || b == bld->undef)
      return bld->undef;

   if(a == b)
      return a;

   if(bld->type.norm) {
      if(a == bld->zero || b == bld->zero)
         return bld->zero;
      if(a == bld->one)
         return b;
      if(b == bld->one)
         return a;
   }

   return lp_build_min_simple(bld, a, b);
}


/**
 * Generate max(a, b)
 * Do checks for special cases.
 */
LLVMValueRef
lp_build_max(struct lp_build_context *bld,
             LLVMValueRef a,
             LLVMValueRef b)
{
   if(a == bld->undef || b == bld->undef)
      return bld->undef;

   if(a == b)
      return a;

   if(bld->type.norm) {
      if(a == bld->one || b == bld->one)
         return bld->one;
      if(a == bld->zero)
         return b;
      if(b == bld->zero)
         return a;
   }

   return lp_build_max_simple(bld, a, b);
}


/**
 * Generate clamp(a, min, max)
 * Do checks for special cases.
 */
LLVMValueRef
lp_build_clamp(struct lp_build_context *bld,
               LLVMValueRef a,
               LLVMValueRef min,
               LLVMValueRef max)
{
   a = lp_build_min(bld, a, max);
   a = lp_build_max(bld, a, min);
   return a;
}


/**
 * Generate abs(a)
 */
LLVMValueRef
lp_build_abs(struct lp_build_context *bld,
             LLVMValueRef a)
{
   const struct lp_type type = bld->type;
   LLVMTypeRef vec_type = lp_build_vec_type(type);

   if(!type.sign)
      return a;

   if(type.floating) {
      /* Mask out the sign bit */
      LLVMTypeRef int_vec_type = lp_build_int_vec_type(type);
      unsigned long long absMask = ~(1ULL << (type.width - 1));
      LLVMValueRef mask = lp_build_int_const_scalar(type, ((unsigned long long) absMask));
      a = LLVMBuildBitCast(bld->builder, a, int_vec_type, "");
      a = LLVMBuildAnd(bld->builder, a, mask, "");
      a = LLVMBuildBitCast(bld->builder, a, vec_type, "");
      return a;
   }

   if(type.width*type.length == 128 && util_cpu_caps.has_ssse3) {
      switch(type.width) {
      case 8:
         return lp_build_intrinsic_unary(bld->builder, "llvm.x86.ssse3.pabs.b.128", vec_type, a);
      case 16:
         return lp_build_intrinsic_unary(bld->builder, "llvm.x86.ssse3.pabs.w.128", vec_type, a);
      case 32:
         return lp_build_intrinsic_unary(bld->builder, "llvm.x86.ssse3.pabs.d.128", vec_type, a);
      }
   }

   return lp_build_max(bld, a, LLVMBuildNeg(bld->builder, a, ""));
}


LLVMValueRef
lp_build_negate(struct lp_build_context *bld,
                LLVMValueRef a)
{
   return LLVMBuildNeg(bld->builder, a, "");
}


LLVMValueRef
lp_build_sgn(struct lp_build_context *bld,
             LLVMValueRef a)
{
   const struct lp_type type = bld->type;
   LLVMTypeRef vec_type = lp_build_vec_type(type);
   LLVMValueRef cond;
   LLVMValueRef res;

   /* Handle non-zero case */
   if(!type.sign) {
      /* if not zero then sign must be positive */
      res = bld->one;
   }
   else if(type.floating) {
      /* Take the sign bit and add it to 1 constant */
      LLVMTypeRef int_vec_type = lp_build_int_vec_type(type);
      LLVMValueRef mask = lp_build_int_const_scalar(type, (unsigned long long)1 << (type.width - 1));
      LLVMValueRef sign;
      LLVMValueRef one;
      sign = LLVMBuildBitCast(bld->builder, a, int_vec_type, "");
      sign = LLVMBuildAnd(bld->builder, sign, mask, "");
      one = LLVMConstBitCast(bld->one, int_vec_type);
      res = LLVMBuildOr(bld->builder, sign, one, "");
      res = LLVMBuildBitCast(bld->builder, res, vec_type, "");
   }
   else
   {
      LLVMValueRef minus_one = lp_build_const_scalar(type, -1.0);
      cond = lp_build_cmp(bld, PIPE_FUNC_GREATER, a, bld->zero);
      res = lp_build_select(bld, cond, bld->one, minus_one);
   }

   /* Handle zero */
   cond = lp_build_cmp(bld, PIPE_FUNC_EQUAL, a, bld->zero);
   res = lp_build_select(bld, cond, bld->zero, bld->one);

   return res;
}


/**
 * Set the sign of float vector 'a' according to 'sign'.
 * If sign==0, return abs(a).
 * If sign==1, return -abs(a);
 * Other values for sign produce undefined results.
 */
LLVMValueRef
lp_build_set_sign(struct lp_build_context *bld,
                  LLVMValueRef a, LLVMValueRef sign)
{
   const struct lp_type type = bld->type;
   LLVMTypeRef int_vec_type = lp_build_int_vec_type(type);
   LLVMTypeRef vec_type = lp_build_vec_type(type);
   LLVMValueRef shift = lp_build_int_const_scalar(type, type.width - 1);
   LLVMValueRef mask = lp_build_int_const_scalar(type,
                             ~((unsigned long long) 1 << (type.width - 1)));
   LLVMValueRef val, res;

   assert(type.floating);

   /* val = reinterpret_cast<int>(a) */
   val = LLVMBuildBitCast(bld->builder, a, int_vec_type, "");
   /* val = val & mask */
   val = LLVMBuildAnd(bld->builder, val, mask, "");
   /* sign = sign << shift */
   sign = LLVMBuildShl(bld->builder, sign, shift, "");
   /* res = val | sign */
   res = LLVMBuildOr(bld->builder, val, sign, "");
   /* res = reinterpret_cast<float>(res) */
   res = LLVMBuildBitCast(bld->builder, res, vec_type, "");

   return res;
}


/**
 * Convert vector of int to vector of float.
 */
LLVMValueRef
lp_build_int_to_float(struct lp_build_context *bld,
                      LLVMValueRef a)
{
   const struct lp_type type = bld->type;

   assert(type.floating);
   /*assert(lp_check_value(type, a));*/

   {
      LLVMTypeRef vec_type = lp_build_vec_type(type);
      /*LLVMTypeRef int_vec_type = lp_build_int_vec_type(type);*/
      LLVMValueRef res;
      res = LLVMBuildSIToFP(bld->builder, a, vec_type, "");
      return res;
   }
}



enum lp_build_round_sse41_mode
{
   LP_BUILD_ROUND_SSE41_NEAREST = 0,
   LP_BUILD_ROUND_SSE41_FLOOR = 1,
   LP_BUILD_ROUND_SSE41_CEIL = 2,
   LP_BUILD_ROUND_SSE41_TRUNCATE = 3
};


static INLINE LLVMValueRef
lp_build_round_sse41(struct lp_build_context *bld,
                     LLVMValueRef a,
                     enum lp_build_round_sse41_mode mode)
{
   const struct lp_type type = bld->type;
   LLVMTypeRef vec_type = lp_build_vec_type(type);
   const char *intrinsic;

   assert(type.floating);
   assert(type.width*type.length == 128);
   assert(lp_check_value(type, a));
   assert(util_cpu_caps.has_sse4_1);

   switch(type.width) {
   case 32:
      intrinsic = "llvm.x86.sse41.round.ps";
      break;
   case 64:
      intrinsic = "llvm.x86.sse41.round.pd";
      break;
   default:
      assert(0);
      return bld->undef;
   }

   return lp_build_intrinsic_binary(bld->builder, intrinsic, vec_type, a,
                                    LLVMConstInt(LLVMInt32Type(), mode, 0));
}


LLVMValueRef
lp_build_trunc(struct lp_build_context *bld,
               LLVMValueRef a)
{
   const struct lp_type type = bld->type;

   assert(type.floating);
   assert(lp_check_value(type, a));

   if(util_cpu_caps.has_sse4_1)
      return lp_build_round_sse41(bld, a, LP_BUILD_ROUND_SSE41_TRUNCATE);
   else {
      LLVMTypeRef vec_type = lp_build_vec_type(type);
      LLVMTypeRef int_vec_type = lp_build_int_vec_type(type);
      LLVMValueRef res;
      res = LLVMBuildFPToSI(bld->builder, a, int_vec_type, "");
      res = LLVMBuildSIToFP(bld->builder, res, vec_type, "");
      return res;
   }
}


LLVMValueRef
lp_build_round(struct lp_build_context *bld,
               LLVMValueRef a)
{
   const struct lp_type type = bld->type;

   assert(type.floating);
   assert(lp_check_value(type, a));

   if(util_cpu_caps.has_sse4_1)
      return lp_build_round_sse41(bld, a, LP_BUILD_ROUND_SSE41_NEAREST);
   else {
      LLVMTypeRef vec_type = lp_build_vec_type(type);
      LLVMValueRef res;
      res = lp_build_iround(bld, a);
      res = LLVMBuildSIToFP(bld->builder, res, vec_type, "");
      return res;
   }
}


LLVMValueRef
lp_build_floor(struct lp_build_context *bld,
               LLVMValueRef a)
{
   const struct lp_type type = bld->type;

   assert(type.floating);

   if(util_cpu_caps.has_sse4_1)
      return lp_build_round_sse41(bld, a, LP_BUILD_ROUND_SSE41_FLOOR);
   else {
      LLVMTypeRef vec_type = lp_build_vec_type(type);
      LLVMValueRef res;
      res = lp_build_ifloor(bld, a);
      res = LLVMBuildSIToFP(bld->builder, res, vec_type, "");
      return res;
   }
}


LLVMValueRef
lp_build_ceil(struct lp_build_context *bld,
              LLVMValueRef a)
{
   const struct lp_type type = bld->type;

   assert(type.floating);
   assert(lp_check_value(type, a));

   if(util_cpu_caps.has_sse4_1)
      return lp_build_round_sse41(bld, a, LP_BUILD_ROUND_SSE41_CEIL);
   else {
      LLVMTypeRef vec_type = lp_build_vec_type(type);
      LLVMValueRef res;
      res = lp_build_iceil(bld, a);
      res = LLVMBuildSIToFP(bld->builder, res, vec_type, "");
      return res;
   }
}


/**
 * Return fractional part of 'a' computed as a - floor(f)
 * Typically used in texture coord arithmetic.
 */
LLVMValueRef
lp_build_fract(struct lp_build_context *bld,
               LLVMValueRef a)
{
   assert(bld->type.floating);
   return lp_build_sub(bld, a, lp_build_floor(bld, a));
}


/**
 * Convert to integer, through whichever rounding method that's fastest,
 * typically truncating toward zero.
 */
LLVMValueRef
lp_build_itrunc(struct lp_build_context *bld,
                LLVMValueRef a)
{
   const struct lp_type type = bld->type;
   LLVMTypeRef int_vec_type = lp_build_int_vec_type(type);

   assert(type.floating);
   assert(lp_check_value(type, a));

   return LLVMBuildFPToSI(bld->builder, a, int_vec_type, "");
}


LLVMValueRef
lp_build_iround(struct lp_build_context *bld,
                LLVMValueRef a)
{
   const struct lp_type type = bld->type;
   LLVMTypeRef int_vec_type = lp_build_int_vec_type(type);
   LLVMValueRef res;

   assert(type.floating);
   assert(lp_check_value(type, a));

   if(util_cpu_caps.has_sse4_1) {
      res = lp_build_round_sse41(bld, a, LP_BUILD_ROUND_SSE41_NEAREST);
   }
   else {
      LLVMTypeRef vec_type = lp_build_vec_type(type);
      LLVMValueRef mask = lp_build_int_const_scalar(type, (unsigned long long)1 << (type.width - 1));
      LLVMValueRef sign;
      LLVMValueRef half;

      /* get sign bit */
      sign = LLVMBuildBitCast(bld->builder, a, int_vec_type, "");
      sign = LLVMBuildAnd(bld->builder, sign, mask, "");

      /* sign * 0.5 */
      half = lp_build_const_scalar(type, 0.5);
      half = LLVMBuildBitCast(bld->builder, half, int_vec_type, "");
      half = LLVMBuildOr(bld->builder, sign, half, "");
      half = LLVMBuildBitCast(bld->builder, half, vec_type, "");

      res = LLVMBuildAdd(bld->builder, a, half, "");
   }

   res = LLVMBuildFPToSI(bld->builder, res, int_vec_type, "");

   return res;
}


/**
 * Convert float[] to int[] with floor().
 */
LLVMValueRef
lp_build_ifloor(struct lp_build_context *bld,
                LLVMValueRef a)
{
   const struct lp_type type = bld->type;
   LLVMTypeRef int_vec_type = lp_build_int_vec_type(type);
   LLVMValueRef res;

   assert(type.floating);
   assert(lp_check_value(type, a));

   if(util_cpu_caps.has_sse4_1) {
      res = lp_build_round_sse41(bld, a, LP_BUILD_ROUND_SSE41_FLOOR);
   }
   else {
      /* Take the sign bit and add it to 1 constant */
      LLVMTypeRef vec_type = lp_build_vec_type(type);
      unsigned mantissa = lp_mantissa(type);
      LLVMValueRef mask = lp_build_int_const_scalar(type, (unsigned long long)1 << (type.width - 1));
      LLVMValueRef sign;
      LLVMValueRef offset;

      /* sign = a < 0 ? ~0 : 0 */
      sign = LLVMBuildBitCast(bld->builder, a, int_vec_type, "");
      sign = LLVMBuildAnd(bld->builder, sign, mask, "");
      sign = LLVMBuildAShr(bld->builder, sign, lp_build_int_const_scalar(type, type.width - 1), "");
      lp_build_name(sign, "floor.sign");

      /* offset = -0.99999(9)f */
      offset = lp_build_const_scalar(type, -(double)(((unsigned long long)1 << mantissa) - 1)/((unsigned long long)1 << mantissa));
      offset = LLVMConstBitCast(offset, int_vec_type);

      /* offset = a < 0 ? -0.99999(9)f : 0.0f */
      offset = LLVMBuildAnd(bld->builder, offset, sign, "");
      offset = LLVMBuildBitCast(bld->builder, offset, vec_type, "");
      lp_build_name(offset, "floor.offset");

      res = LLVMBuildAdd(bld->builder, a, offset, "");
      lp_build_name(res, "floor.res");
   }

   res = LLVMBuildFPToSI(bld->builder, res, int_vec_type, "");
   lp_build_name(res, "floor");

   return res;
}


LLVMValueRef
lp_build_iceil(struct lp_build_context *bld,
               LLVMValueRef a)
{
   const struct lp_type type = bld->type;
   LLVMTypeRef int_vec_type = lp_build_int_vec_type(type);
   LLVMValueRef res;

   assert(type.floating);
   assert(lp_check_value(type, a));

   if(util_cpu_caps.has_sse4_1) {
      res = lp_build_round_sse41(bld, a, LP_BUILD_ROUND_SSE41_CEIL);
   }
   else {
      assert(0);
      res = bld->undef;
   }

   res = LLVMBuildFPToSI(bld->builder, res, int_vec_type, "");

   return res;
}


LLVMValueRef
lp_build_sqrt(struct lp_build_context *bld,
              LLVMValueRef a)
{
   const struct lp_type type = bld->type;
   LLVMTypeRef vec_type = lp_build_vec_type(type);
   char intrinsic[32];

   /* TODO: optimize the constant case */
   /* TODO: optimize the constant case */

   assert(type.floating);
   util_snprintf(intrinsic, sizeof intrinsic, "llvm.sqrt.v%uf%u", type.length, type.width);

   return lp_build_intrinsic_unary(bld->builder, intrinsic, vec_type, a);
}


LLVMValueRef
lp_build_rcp(struct lp_build_context *bld,
             LLVMValueRef a)
{
   const struct lp_type type = bld->type;

   if(a == bld->zero)
      return bld->undef;
   if(a == bld->one)
      return bld->one;
   if(a == bld->undef)
      return bld->undef;

   assert(type.floating);

   if(LLVMIsConstant(a))
      return LLVMConstFDiv(bld->one, a);

   if(util_cpu_caps.has_sse && type.width == 32 && type.length == 4)
      /* FIXME: improve precision */
      return lp_build_intrinsic_unary(bld->builder, "llvm.x86.sse.rcp.ps", lp_build_vec_type(type), a);

   return LLVMBuildFDiv(bld->builder, bld->one, a, "");
}


/**
 * Generate 1/sqrt(a)
 */
LLVMValueRef
lp_build_rsqrt(struct lp_build_context *bld,
               LLVMValueRef a)
{
   const struct lp_type type = bld->type;

   assert(type.floating);

   if(util_cpu_caps.has_sse && type.width == 32 && type.length == 4)
      return lp_build_intrinsic_unary(bld->builder, "llvm.x86.sse.rsqrt.ps", lp_build_vec_type(type), a);

   return lp_build_rcp(bld, lp_build_sqrt(bld, a));
}


/**
 * Generate cos(a)
 */
LLVMValueRef
lp_build_cos(struct lp_build_context *bld,
              LLVMValueRef a)
{
   const struct lp_type type = bld->type;
   LLVMTypeRef vec_type = lp_build_vec_type(type);
   char intrinsic[32];

   /* TODO: optimize the constant case */

   assert(type.floating);
   util_snprintf(intrinsic, sizeof intrinsic, "llvm.cos.v%uf%u", type.length, type.width);

   return lp_build_intrinsic_unary(bld->builder, intrinsic, vec_type, a);
}


/**
 * Generate sin(a)
 */
LLVMValueRef
lp_build_sin(struct lp_build_context *bld,
              LLVMValueRef a)
{
   const struct lp_type type = bld->type;
   LLVMTypeRef vec_type = lp_build_vec_type(type);
   char intrinsic[32];

   /* TODO: optimize the constant case */

   assert(type.floating);
   util_snprintf(intrinsic, sizeof intrinsic, "llvm.sin.v%uf%u", type.length, type.width);

   return lp_build_intrinsic_unary(bld->builder, intrinsic, vec_type, a);
}


/**
 * Generate pow(x, y)
 */
LLVMValueRef
lp_build_pow(struct lp_build_context *bld,
             LLVMValueRef x,
             LLVMValueRef y)
{
   /* TODO: optimize the constant case */
   if(LLVMIsConstant(x) && LLVMIsConstant(y))
      debug_printf("%s: inefficient/imprecise constant arithmetic\n",
                   __FUNCTION__);

   return lp_build_exp2(bld, lp_build_mul(bld, lp_build_log2(bld, x), y));
}


/**
 * Generate exp(x)
 */
LLVMValueRef
lp_build_exp(struct lp_build_context *bld,
             LLVMValueRef x)
{
   /* log2(e) = 1/log(2) */
   LLVMValueRef log2e = lp_build_const_scalar(bld->type, 1.4426950408889634);

   return lp_build_mul(bld, log2e, lp_build_exp2(bld, x));
}


/**
 * Generate log(x)
 */
LLVMValueRef
lp_build_log(struct lp_build_context *bld,
             LLVMValueRef x)
{
   /* log(2) */
   LLVMValueRef log2 = lp_build_const_scalar(bld->type, 0.69314718055994529);

   return lp_build_mul(bld, log2, lp_build_exp2(bld, x));
}


#define EXP_POLY_DEGREE 3
#define LOG_POLY_DEGREE 5


/**
 * Generate polynomial.
 * Ex:  coeffs[0] + x * coeffs[1] + x^2 * coeffs[2].
 */
static LLVMValueRef
lp_build_polynomial(struct lp_build_context *bld,
                    LLVMValueRef x,
                    const double *coeffs,
                    unsigned num_coeffs)
{
   const struct lp_type type = bld->type;
   LLVMValueRef res = NULL;
   unsigned i;

   /* TODO: optimize the constant case */
   if(LLVMIsConstant(x))
      debug_printf("%s: inefficient/imprecise constant arithmetic\n",
                   __FUNCTION__);

   for (i = num_coeffs; i--; ) {
      LLVMValueRef coeff = lp_build_const_scalar(type, coeffs[i]);
      if(res)
         res = lp_build_add(bld, coeff, lp_build_mul(bld, x, res));
      else
         res = coeff;
   }

   if(res)
      return res;
   else
      return bld->undef;
}


/**
 * Minimax polynomial fit of 2**x, in range [-0.5, 0.5[
 */
const double lp_build_exp2_polynomial[] = {
#if EXP_POLY_DEGREE == 5
   9.9999994e-1, 6.9315308e-1, 2.4015361e-1, 5.5826318e-2, 8.9893397e-3, 1.8775767e-3
#elif EXP_POLY_DEGREE == 4
   1.0000026, 6.9300383e-1, 2.4144275e-1, 5.2011464e-2, 1.3534167e-2
#elif EXP_POLY_DEGREE == 3
   9.9992520e-1, 6.9583356e-1, 2.2606716e-1, 7.8024521e-2
#elif EXP_POLY_DEGREE == 2
   1.0017247, 6.5763628e-1, 3.3718944e-1
#else
#error
#endif
};


void
lp_build_exp2_approx(struct lp_build_context *bld,
                     LLVMValueRef x,
                     LLVMValueRef *p_exp2_int_part,
                     LLVMValueRef *p_frac_part,
                     LLVMValueRef *p_exp2)
{
   const struct lp_type type = bld->type;
   LLVMTypeRef vec_type = lp_build_vec_type(type);
   LLVMTypeRef int_vec_type = lp_build_int_vec_type(type);
   LLVMValueRef ipart = NULL;
   LLVMValueRef fpart = NULL;
   LLVMValueRef expipart = NULL;
   LLVMValueRef expfpart = NULL;
   LLVMValueRef res = NULL;

   if(p_exp2_int_part || p_frac_part || p_exp2) {
      /* TODO: optimize the constant case */
      if(LLVMIsConstant(x))
         debug_printf("%s: inefficient/imprecise constant arithmetic\n",
                      __FUNCTION__);

      assert(type.floating && type.width == 32);

      x = lp_build_min(bld, x, lp_build_const_scalar(type,  129.0));
      x = lp_build_max(bld, x, lp_build_const_scalar(type, -126.99999));

      /* ipart = int(x - 0.5) */
      ipart = LLVMBuildSub(bld->builder, x, lp_build_const_scalar(type, 0.5f), "");
      ipart = LLVMBuildFPToSI(bld->builder, ipart, int_vec_type, "");

      /* fpart = x - ipart */
      fpart = LLVMBuildSIToFP(bld->builder, ipart, vec_type, "");
      fpart = LLVMBuildSub(bld->builder, x, fpart, "");
   }

   if(p_exp2_int_part || p_exp2) {
      /* expipart = (float) (1 << ipart) */
      expipart = LLVMBuildAdd(bld->builder, ipart, lp_build_int_const_scalar(type, 127), "");
      expipart = LLVMBuildShl(bld->builder, expipart, lp_build_int_const_scalar(type, 23), "");
      expipart = LLVMBuildBitCast(bld->builder, expipart, vec_type, "");
   }

   if(p_exp2) {
      expfpart = lp_build_polynomial(bld, fpart, lp_build_exp2_polynomial,
                                     Elements(lp_build_exp2_polynomial));

      res = LLVMBuildMul(bld->builder, expipart, expfpart, "");
   }

   if(p_exp2_int_part)
      *p_exp2_int_part = expipart;

   if(p_frac_part)
      *p_frac_part = fpart;

   if(p_exp2)
      *p_exp2 = res;
}


LLVMValueRef
lp_build_exp2(struct lp_build_context *bld,
              LLVMValueRef x)
{
   LLVMValueRef res;
   lp_build_exp2_approx(bld, x, NULL, NULL, &res);
   return res;
}


/**
 * Minimax polynomial fit of log2(x)/(x - 1), for x in range [1, 2[
 * These coefficients can be generate with
 * http://www.boost.org/doc/libs/1_36_0/libs/math/doc/sf_and_dist/html/math_toolkit/toolkit/internals2/minimax.html
 */
const double lp_build_log2_polynomial[] = {
#if LOG_POLY_DEGREE == 6
   3.11578814719469302614, -3.32419399085241980044, 2.59883907202499966007, -1.23152682416275988241, 0.318212422185251071475, -0.0344359067839062357313
#elif LOG_POLY_DEGREE == 5
   2.8882704548164776201, -2.52074962577807006663, 1.48116647521213171641, -0.465725644288844778798, 0.0596515482674574969533
#elif LOG_POLY_DEGREE == 4
   2.61761038894603480148, -1.75647175389045657003, 0.688243882994381274313, -0.107254423828329604454
#elif LOG_POLY_DEGREE == 3
   2.28330284476918490682, -1.04913055217340124191, 0.204446009836232697516
#else
#error
#endif
};


/**
 * See http://www.devmaster.net/forums/showthread.php?p=43580
 */
void
lp_build_log2_approx(struct lp_build_context *bld,
                     LLVMValueRef x,
                     LLVMValueRef *p_exp,
                     LLVMValueRef *p_floor_log2,
                     LLVMValueRef *p_log2)
{
   const struct lp_type type = bld->type;
   LLVMTypeRef vec_type = lp_build_vec_type(type);
   LLVMTypeRef int_vec_type = lp_build_int_vec_type(type);

   LLVMValueRef expmask = lp_build_int_const_scalar(type, 0x7f800000);
   LLVMValueRef mantmask = lp_build_int_const_scalar(type, 0x007fffff);
   LLVMValueRef one = LLVMConstBitCast(bld->one, int_vec_type);

   LLVMValueRef i = NULL;
   LLVMValueRef exp = NULL;
   LLVMValueRef mant = NULL;
   LLVMValueRef logexp = NULL;
   LLVMValueRef logmant = NULL;
   LLVMValueRef res = NULL;

   if(p_exp || p_floor_log2 || p_log2) {
      /* TODO: optimize the constant case */
      if(LLVMIsConstant(x))
         debug_printf("%s: inefficient/imprecise constant arithmetic\n",
                      __FUNCTION__);

      assert(type.floating && type.width == 32);

      i = LLVMBuildBitCast(bld->builder, x, int_vec_type, "");

      /* exp = (float) exponent(x) */
      exp = LLVMBuildAnd(bld->builder, i, expmask, "");
   }

   if(p_floor_log2 || p_log2) {
      logexp = LLVMBuildLShr(bld->builder, exp, lp_build_int_const_scalar(type, 23), "");
      logexp = LLVMBuildSub(bld->builder, logexp, lp_build_int_const_scalar(type, 127), "");
      logexp = LLVMBuildSIToFP(bld->builder, logexp, vec_type, "");
   }

   if(p_log2) {
      /* mant = (float) mantissa(x) */
      mant = LLVMBuildAnd(bld->builder, i, mantmask, "");
      mant = LLVMBuildOr(bld->builder, mant, one, "");
      mant = LLVMBuildBitCast(bld->builder, mant, vec_type, "");

      logmant = lp_build_polynomial(bld, mant, lp_build_log2_polynomial,
                                    Elements(lp_build_log2_polynomial));

      /* This effectively increases the polynomial degree by one, but ensures that log2(1) == 0*/
      logmant = LLVMBuildMul(bld->builder, logmant, LLVMBuildSub(bld->builder, mant, bld->one, ""), "");

      res = LLVMBuildAdd(bld->builder, logmant, logexp, "");
   }

   if(p_exp)
      *p_exp = exp;

   if(p_floor_log2)
      *p_floor_log2 = logexp;

   if(p_log2)
      *p_log2 = res;
}


LLVMValueRef
lp_build_log2(struct lp_build_context *bld,
              LLVMValueRef x)
{
   LLVMValueRef res;
   lp_build_log2_approx(bld, x, NULL, NULL, &res);
   return res;
}
