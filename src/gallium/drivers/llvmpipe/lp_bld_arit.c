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
#include "lp_bld_arit.h"


static LLVMValueRef
lp_build_intrinsic_binary(LLVMBuilderRef builder,
                          const char *name,
                          LLVMValueRef a,
                          LLVMValueRef b)
{
   LLVMModuleRef module = LLVMGetGlobalParent(LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)));
   LLVMValueRef function;
   LLVMValueRef args[2];

   function = LLVMGetNamedFunction(module, name);
   if(!function) {
      LLVMTypeRef type = LLVMTypeOf(a);
      LLVMTypeRef arg_types[2];
      arg_types[0] = type;
      arg_types[1] = type;
      function = LLVMAddFunction(module, name, LLVMFunctionType(type, arg_types, 2, 0));
      LLVMSetFunctionCallConv(function, LLVMCCallConv);
      LLVMSetLinkage(function, LLVMExternalLinkage);
   }
   assert(LLVMIsDeclaration(function));

#ifdef DEBUG
   /* We shouldn't use only constants with intrinsics, as they won't be
    * propagated by LLVM optimization passes.
    */
   if(LLVMIsConstant(a) && LLVMIsConstant(b))
      debug_printf("warning: invoking intrinsic \"%s\" with constants\n");
#endif

   args[0] = a;
   args[1] = b;

   return LLVMBuildCall(builder, function, args, 2, "");
}


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
      if(type.floating)
         if(type.width == 32)
            intrinsic = "llvm.x86.sse.min.ps";
         if(type.width == 64)
            intrinsic = "llvm.x86.sse2.min.pd";
      else {
         if(type.width == 8 && !type.sign)
            intrinsic = "llvm.x86.sse2.pminu.b";
         if(type.width == 16 && type.sign)
            intrinsic = "llvm.x86.sse2.pmins.w";
      }
   }
#endif
   
   if(intrinsic)
      return lp_build_intrinsic_binary(bld->builder, intrinsic, a, b);

   if(type.floating)
      cond = LLVMBuildFCmp(bld->builder, LLVMRealULT, a, b, "");
   else
      cond = LLVMBuildICmp(bld->builder, type.sign ? LLVMIntSLT : LLVMIntULT, a, b, "");
   return LLVMBuildSelect(bld->builder, cond, a, b, "");
}


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
      if(type.floating)
         if(type.width == 32)
            intrinsic = "llvm.x86.sse.max.ps";
         if(type.width == 64)
            intrinsic = "llvm.x86.sse2.max.pd";
      else {
         if(type.width == 8 && !type.sign)
            intrinsic = "llvm.x86.sse2.pmaxu.b";
         if(type.width == 16 && type.sign)
            intrinsic = "llvm.x86.sse2.pmaxs.w";
      }
   }
#endif

   if(intrinsic)
      return lp_build_intrinsic_binary(bld->builder, intrinsic, a, b);

   if(type.floating)
      cond = LLVMBuildFCmp(bld->builder, LLVMRealULT, a, b, "");
   else
      cond = LLVMBuildICmp(bld->builder, type.sign ? LLVMIntSLT : LLVMIntULT, a, b, "");
   return LLVMBuildSelect(bld->builder, cond, b, a, "");
}


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
         return lp_build_intrinsic_binary(bld->builder, intrinsic, a, b);
   }

   if(LLVMIsConstant(a) && LLVMIsConstant(b))
      res = LLVMConstAdd(a, b);
   else
      res = LLVMBuildAdd(bld->builder, a, b, "");

   if(bld->type.norm && (bld->type.floating || bld->type.fixed))
      res = lp_build_min_simple(bld, res, bld->one);

   return res;
}


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
         return lp_build_intrinsic_binary(bld->builder, intrinsic, a, b);
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
         ab = lp_build_intrinsic_binary(bld->builder, "llvm.x86.sse2.packuswb.128" , abl, abh);

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
