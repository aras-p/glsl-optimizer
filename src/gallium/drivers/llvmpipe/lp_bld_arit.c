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


#include "pipe/p_state.h"

#include "lp_bld_arit.h"


LLVMValueRef
lp_build_const_aos(LLVMTypeRef type, 
                   double r, double g, double b, double a, 
                   const unsigned char *swizzle)
{
   const unsigned char default_swizzle[4] = {0, 1, 2, 3};
   LLVMTypeRef elem_type;
   unsigned num_elems;
   unsigned elem_width;
   LLVMValueRef elems[LP_MAX_VECTOR_SIZE];
   double scale;
   unsigned i;

   num_elems = LLVMGetVectorSize(type);
   assert(num_elems % 4 == 0);
   assert(num_elems < LP_MAX_VECTOR_SIZE);

   elem_type = LLVMGetElementType(type);

   if(swizzle == NULL)
      swizzle = default_swizzle;

   switch(LLVMGetTypeKind(elem_type)) {
   case LLVMFloatTypeKind:
      for(i = 0; i < num_elems; i += 4) {
         elems[i + swizzle[0]] = LLVMConstReal(elem_type, r);
         elems[i + swizzle[1]] = LLVMConstReal(elem_type, g);
         elems[i + swizzle[2]] = LLVMConstReal(elem_type, b);
         elems[i + swizzle[3]] = LLVMConstReal(elem_type, a);
      }
      break;

   case LLVMIntegerTypeKind:
      elem_width = LLVMGetIntTypeWidth(elem_type);
      assert(elem_width <= 32);
      scale = (double)((1 << elem_width) - 1);
      for(i = 0; i < num_elems; i += 4) {
         elems[i + swizzle[0]] = LLVMConstInt(elem_type, r*scale + 0.5, 0);
         elems[i + swizzle[1]] = LLVMConstInt(elem_type, g*scale + 0.5, 0);
         elems[i + swizzle[2]] = LLVMConstInt(elem_type, b*scale + 0.5, 0);
         elems[i + swizzle[3]] = LLVMConstInt(elem_type, a*scale + 0.5, 0);
      }
      break;

   default:
      assert(0);
      return LLVMGetUndef(type);
   }

   return LLVMConstVector(elems, num_elems);
}
               

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

   args[0] = a;
   args[1] = b;

   return LLVMBuildCall(builder, function, args, 2, "");
}


LLVMValueRef
lp_build_add(LLVMBuilderRef builder,
             LLVMValueRef a,
             LLVMValueRef b,
             LLVMValueRef zero)
{
   if(a == zero)
      return b;
   else if(b == zero)
      return a;
   else if(LLVMIsConstant(a) && LLVMIsConstant(b))
      return LLVMConstAdd(a, b);
   else
      return LLVMBuildAdd(builder, a, b, "");
}


LLVMValueRef
lp_build_sub(LLVMBuilderRef builder,
             LLVMValueRef a,
             LLVMValueRef b,
             LLVMValueRef zero)
{
   if(b == zero)
      return a;
   else if(a == b)
      return zero;
   else if(LLVMIsConstant(a) && LLVMIsConstant(b))
      return LLVMConstSub(a, b);
   else
      return LLVMBuildSub(builder, a, b, "");
}


LLVMValueRef
lp_build_mul(LLVMBuilderRef builder,
             LLVMValueRef a,
             LLVMValueRef b,
             LLVMValueRef zero,
             LLVMValueRef one)
{
   if(a == zero)
      return zero;
   else if(a == one)
      return b;
   else if(b == zero)
      return zero;
   else if(b == one)
      return a;
   else if(LLVMIsConstant(a) && LLVMIsConstant(b))
      return LLVMConstMul(a, b);
   else
      return LLVMBuildMul(builder, a, b, "");
}


LLVMValueRef
lp_build_min(LLVMBuilderRef builder,
             LLVMValueRef a,
             LLVMValueRef b)
{
   /* TODO: optimize the constant case */

#if defined(PIPE_ARCH_X86) || defined(PIPE_ARCH_X86_64)

   return lp_build_intrinsic_binary(builder, "llvm.x86.sse.min.ps", a, b);

#else

   LLVMValueRef cond = LLVMBuildFCmp(values->builder, LLVMRealULT, a, b, "");
   return LLVMBuildSelect(values->builder, cond, a, b, "");

#endif
}


LLVMValueRef
lp_build_max(LLVMBuilderRef builder,
             LLVMValueRef a,
             LLVMValueRef b)
{
   /* TODO: optimize the constant case */

#if defined(PIPE_ARCH_X86) || defined(PIPE_ARCH_X86_64)

   return lp_build_intrinsic_binary(builder, "llvm.x86.sse.max.ps", a, b);

#else

   LLVMValueRef cond = LLVMBuildFCmp(values->builder, LLVMRealULT, a, b, "");
   return LLVMBuildSelect(values->builder, cond, b, a, "");

#endif
}


LLVMValueRef
lp_build_add_sat(LLVMBuilderRef builder,
                 LLVMValueRef a,
                 LLVMValueRef b,
                 LLVMValueRef zero,
                 LLVMValueRef one)
{
   if(a == zero)
      return b;
   else if(b == zero)
      return a;
   else if(a == one || b == one)
      return one;
   else
      return lp_build_min(builder, lp_build_add(builder, a, b, zero), one);
}

LLVMValueRef
lp_build_sub_sat(LLVMBuilderRef builder,
                 LLVMValueRef a,
                 LLVMValueRef b,
                 LLVMValueRef zero,
                 LLVMValueRef one)
{
   if(b == zero)
      return a;
   else if(b == one)
      return zero;
   else
      return lp_build_max(builder, lp_build_sub(builder, a, b, zero), zero);
}

LLVMValueRef
lp_build_min_sat(LLVMBuilderRef builder,
                 LLVMValueRef a,
                 LLVMValueRef b,
                 LLVMValueRef zero,
                 LLVMValueRef one)
{
   if(a == zero || b == zero)
      return zero;
   else if(a == one)
      return b;
   else if(b == one)
      return a;
   else
      return lp_build_min(builder, a, b);
}


LLVMValueRef
lp_build_max_sat(LLVMBuilderRef builder,
                 LLVMValueRef a,
                 LLVMValueRef b,
                 LLVMValueRef zero,
                 LLVMValueRef one)
{
   if(a == zero)
      return b;
   else if(b == zero)
      return a;
   else if(a == one || b == one)
      return one;
   else
      return lp_build_max(builder, a, b);
}
