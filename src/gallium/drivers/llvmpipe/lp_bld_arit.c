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
#include "util/u_string.h"

#include "lp_bld_type.h"
#include "lp_bld_const.h"
#include "lp_bld_intr.h"
#include "lp_bld_logic.h"
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
   const union lp_type type = bld->type;
   const char *intrinsic = NULL;
   LLVMValueRef cond;

   /* TODO: optimize the constant case */

#if defined(PIPE_ARCH_X86) || defined(PIPE_ARCH_X86_64)
   if(type.width * type.length == 128) {
      if(type.floating) {
         if(type.width == 32)
            intrinsic = "llvm.x86.sse.min.ps";
         if(type.width == 64)
            intrinsic = "llvm.x86.sse2.min.pd";
      }
      else {
         if(type.width == 8 && !type.sign)
            intrinsic = "llvm.x86.sse2.pminu.b";
         if(type.width == 8 && type.sign)
            intrinsic = "llvm.x86.sse41.pminsb";
         if(type.width == 16 && !type.sign)
            intrinsic = "llvm.x86.sse41.pminuw";
         if(type.width == 16 && type.sign)
            intrinsic = "llvm.x86.sse2.pmins.w";
         if(type.width == 32 && !type.sign)
            intrinsic = "llvm.x86.sse41.pminud";
         if(type.width == 32 && type.sign)
            intrinsic = "llvm.x86.sse41.pminsd";
      }
   }
#endif

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
   const union lp_type type = bld->type;
   const char *intrinsic = NULL;
   LLVMValueRef cond;

   /* TODO: optimize the constant case */

#if defined(PIPE_ARCH_X86) || defined(PIPE_ARCH_X86_64)
   if(type.width * type.length == 128) {
      if(type.floating) {
         if(type.width == 32)
            intrinsic = "llvm.x86.sse.max.ps";
         if(type.width == 64)
            intrinsic = "llvm.x86.sse2.max.pd";
      }
      else {
         if(type.width == 8 && !type.sign)
            intrinsic = "llvm.x86.sse2.pmaxu.b";
         if(type.width == 8 && type.sign)
            intrinsic = "llvm.x86.sse41.pmaxsb";
         if(type.width == 16 && !type.sign)
            intrinsic = "llvm.x86.sse41.pmaxuw";
         if(type.width == 16 && type.sign)
            intrinsic = "llvm.x86.sse2.pmaxs.w";
         if(type.width == 32 && !type.sign)
            intrinsic = "llvm.x86.sse41.pmaxud";
         if(type.width == 32 && type.sign)
            intrinsic = "llvm.x86.sse41.pmaxsd";
      }
   }
#endif

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
   const union lp_type type = bld->type;

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
   const union lp_type type = bld->type;
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

#if defined(PIPE_ARCH_X86) || defined(PIPE_ARCH_X86_64)
      if(type.width * type.length == 128 &&
         !type.floating && !type.fixed) {
         if(type.width == 8)
            intrinsic = type.sign ? "llvm.x86.sse2.padds.b" : "llvm.x86.sse2.paddus.b";
         if(type.width == 16)
            intrinsic = type.sign ? "llvm.x86.sse2.padds.w" : "llvm.x86.sse2.paddus.w";
      }
#endif
   
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
   const union lp_type type = bld->type;
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

#if defined(PIPE_ARCH_X86) || defined(PIPE_ARCH_X86_64)
      if(type.width * type.length == 128 &&
         !type.floating && !type.fixed) {
         if(type.width == 8)
            intrinsic = type.sign ? "llvm.x86.sse2.psubs.b" : "llvm.x86.sse2.psubus.b";
         if(type.width == 16)
            intrinsic = type.sign ? "llvm.x86.sse2.psubs.w" : "llvm.x86.sse2.psubus.w";
      }
#endif
   
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
 * Build shuffle vectors that match PUNPCKLxx and PUNPCKHxx instructions.
 */
static LLVMValueRef 
lp_build_unpack_shuffle(unsigned n, unsigned lo_hi)
{
   LLVMValueRef elems[LP_MAX_VECTOR_LENGTH];
   unsigned i, j;

   assert(n <= LP_MAX_VECTOR_LENGTH);
   assert(lo_hi < 2);

   for(i = 0, j = lo_hi*n/2; i < n; i += 2, ++j) {
      elems[i + 0] = LLVMConstInt(LLVMInt32Type(), 0 + j, 0);
      elems[i + 1] = LLVMConstInt(LLVMInt32Type(), n + j, 0);
   }

   return LLVMConstVector(elems, n);
}


/**
 * Build constant int vector of width 'n' and value 'c'.
 */
static LLVMValueRef 
lp_build_const_vec(LLVMTypeRef type, unsigned n, long long c)
{
   LLVMValueRef elems[LP_MAX_VECTOR_LENGTH];
   unsigned i;

   assert(n <= LP_MAX_VECTOR_LENGTH);

   for(i = 0; i < n; ++i)
      elems[i] = LLVMConstInt(type, c, 0);

   return LLVMConstVector(elems, n);
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
                 LLVMValueRef a, LLVMValueRef b)
{
   static LLVMValueRef c01 = NULL;
   static LLVMValueRef c08 = NULL;
   static LLVMValueRef c80 = NULL;
   LLVMValueRef ab;

   if(!c01) c01 = lp_build_const_vec(LLVMInt16Type(), 8, 0x01);
   if(!c08) c08 = lp_build_const_vec(LLVMInt16Type(), 8, 0x08);
   if(!c80) c80 = lp_build_const_vec(LLVMInt16Type(), 8, 0x80);
   
#if 0
   
   /* a*b/255 ~= (a*(b + 1)) >> 256 */
   b = LLVMBuildAdd(builder, b, c01, "");
   ab = LLVMBuildMul(builder, a, b, "");

#else
   
   /* t/255 ~= (t + (t >> 8) + 0x80) >> 8 */
   ab = LLVMBuildMul(builder, a, b, "");
   ab = LLVMBuildAdd(builder, ab, LLVMBuildLShr(builder, ab, c08, ""), "");
   ab = LLVMBuildAdd(builder, ab, c80, "");

#endif
   
   ab = LLVMBuildLShr(builder, ab, c08, "");

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
   const union lp_type type = bld->type;

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
#if defined(PIPE_ARCH_X86) || defined(PIPE_ARCH_X86_64)
      if(type.width == 8 && type.length == 16) {
         LLVMTypeRef i16x8 = LLVMVectorType(LLVMInt16Type(), 8);
         LLVMTypeRef i8x16 = LLVMVectorType(LLVMInt8Type(), 16);
         static LLVMValueRef ml = NULL;
         static LLVMValueRef mh = NULL;
         LLVMValueRef al, ah, bl, bh;
         LLVMValueRef abl, abh;
         LLVMValueRef ab;
         
         if(!ml) ml = lp_build_unpack_shuffle(16, 0);
         if(!mh) mh = lp_build_unpack_shuffle(16, 1);

         /*  PUNPCKLBW, PUNPCKHBW */
         al = LLVMBuildShuffleVector(bld->builder, a, bld->zero, ml, "");
         bl = LLVMBuildShuffleVector(bld->builder, b, bld->zero, ml, "");
         ah = LLVMBuildShuffleVector(bld->builder, a, bld->zero, mh, "");
         bh = LLVMBuildShuffleVector(bld->builder, b, bld->zero, mh, "");

         /* NOP */
         al = LLVMBuildBitCast(bld->builder, al, i16x8, "");
         bl = LLVMBuildBitCast(bld->builder, bl, i16x8, "");
         ah = LLVMBuildBitCast(bld->builder, ah, i16x8, "");
         bh = LLVMBuildBitCast(bld->builder, bh, i16x8, "");

         /* PMULLW, PSRLW, PADDW */
         abl = lp_build_mul_u8n(bld->builder, al, bl);
         abh = lp_build_mul_u8n(bld->builder, ah, bh);

         /* PACKUSWB */
         ab = lp_build_intrinsic_binary(bld->builder, "llvm.x86.sse2.packuswb.128" , i16x8, abl, abh);

         /* NOP */
         ab = LLVMBuildBitCast(bld->builder, ab, i8x16, "");
         
         return ab;
      }
#endif

      /* FIXME */
      assert(0);
   }

   if(LLVMIsConstant(a) && LLVMIsConstant(b))
      return LLVMConstMul(a, b);

   return LLVMBuildMul(bld->builder, a, b, "");
}


/**
 * Generate a / b
 */
LLVMValueRef
lp_build_div(struct lp_build_context *bld,
             LLVMValueRef a,
             LLVMValueRef b)
{
   const union lp_type type = bld->type;

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

#if defined(PIPE_ARCH_X86) || defined(PIPE_ARCH_X86_64)
   if(type.width == 32 && type.length == 4)
      return lp_build_mul(bld, a, lp_build_rcp(bld, b));
#endif

   return LLVMBuildFDiv(bld->builder, a, b, "");
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
 * Generate abs(a)
 */
LLVMValueRef
lp_build_abs(struct lp_build_context *bld,
             LLVMValueRef a)
{
   const union lp_type type = bld->type;

   if(!type.sign)
      return a;

   /* XXX: is this really necessary? */
#if defined(PIPE_ARCH_X86) || defined(PIPE_ARCH_X86_64)
   if(!type.floating && type.width*type.length == 128) {
      LLVMTypeRef vec_type = lp_build_vec_type(type);
      if(type.width == 8)
         return lp_build_intrinsic_unary(bld->builder, "llvm.x86.ssse3.pabs.b.128", vec_type, a);
      if(type.width == 16)
         return lp_build_intrinsic_unary(bld->builder, "llvm.x86.ssse3.pabs.w.128", vec_type, a);
      if(type.width == 32)
         return lp_build_intrinsic_unary(bld->builder, "llvm.x86.ssse3.pabs.d.128", vec_type, a);
   }
#endif

   return lp_build_max(bld, a, LLVMBuildNeg(bld->builder, a, ""));
}


LLVMValueRef
lp_build_sqrt(struct lp_build_context *bld,
              LLVMValueRef a)
{
   const union lp_type type = bld->type;
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
   const union lp_type type = bld->type;

   if(a == bld->zero)
      return bld->undef;
   if(a == bld->one)
      return bld->one;
   if(a == bld->undef)
      return bld->undef;

   assert(type.floating);

   if(LLVMIsConstant(a))
      return LLVMConstFDiv(bld->one, a);

   /* XXX: is this really necessary? */
#if defined(PIPE_ARCH_X86) || defined(PIPE_ARCH_X86_64)
   if(type.width == 32 && type.length == 4)
      return lp_build_intrinsic_unary(bld->builder, "llvm.x86.sse.rcp.ps", lp_build_vec_type(type), a);
#endif

   return LLVMBuildFDiv(bld->builder, bld->one, a, "");
}


/**
 * Generate 1/sqrt(a)
 */
LLVMValueRef
lp_build_rsqrt(struct lp_build_context *bld,
               LLVMValueRef a)
{
   const union lp_type type = bld->type;

   assert(type.floating);

   /* XXX: is this really necessary? */
#if defined(PIPE_ARCH_X86) || defined(PIPE_ARCH_X86_64)
   if(type.width == 32 && type.length == 4)
      return lp_build_intrinsic_unary(bld->builder, "llvm.x86.sse.rsqrt.ps", lp_build_vec_type(type), a);
#endif

   return lp_build_rcp(bld, lp_build_sqrt(bld, a));
}


/**
 * Generate cos(a)
 */
LLVMValueRef
lp_build_cos(struct lp_build_context *bld,
              LLVMValueRef a)
{
   const union lp_type type = bld->type;
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
   const union lp_type type = bld->type;
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
      debug_printf("%s: inefficient/imprecise constant arithmetic\n");

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
   LLVMValueRef log2 = lp_build_const_scalar(bld->type, 1.4426950408889634);

   return lp_build_mul(bld, log2, lp_build_exp2(bld, x));
}


#define EXP_POLY_DEGREE 3
#define LOG_POLY_DEGREE 5


/**
 * Generate polynomial.
 * Ex:  x^2 * coeffs[0] + x * coeffs[1] + coeffs[2].
 */
static LLVMValueRef
lp_build_polynomial(struct lp_build_context *bld,
                    LLVMValueRef x,
                    const double *coeffs,
                    unsigned num_coeffs)
{
   const union lp_type type = bld->type;
   LLVMValueRef res = NULL;
   unsigned i;

   /* TODO: optimize the constant case */
   if(LLVMIsConstant(x))
      debug_printf("%s: inefficient/imprecise constant arithmetic\n");

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
   const union lp_type type = bld->type;
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
         debug_printf("%s: inefficient/imprecise constant arithmetic\n");

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
   const union lp_type type = bld->type;
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
         debug_printf("%s: inefficient/imprecise constant arithmetic\n");

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
      mant = LLVMBuildSIToFP(bld->builder, mant, vec_type, "");

      logmant = lp_build_polynomial(bld, mant, lp_build_log2_polynomial,
                                    Elements(lp_build_log2_polynomial));

      /* This effectively increases the polynomial degree by one, but ensures that log2(1) == 0*/
      logmant = LLVMBuildMul(bld->builder, logmant, LLVMBuildMul(bld->builder, mant, bld->one, ""), "");

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
