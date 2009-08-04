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
 * Helper functions for constant building.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#include "util/u_debug.h"

#include "lp_bld_type.h"
#include "lp_bld_const.h"


LLVMValueRef
lp_build_undef(union lp_type type)
{
   LLVMTypeRef vec_type = lp_build_vec_type(type);
   return LLVMGetUndef(vec_type);
}
               

LLVMValueRef
lp_build_zero(union lp_type type)
{
   LLVMTypeRef vec_type = lp_build_vec_type(type);
   return LLVMConstNull(vec_type);
}
               

LLVMValueRef
lp_build_one(union lp_type type)
{
   LLVMTypeRef elem_type;
   LLVMValueRef elems[LP_MAX_VECTOR_LENGTH];
   unsigned i;

   assert(type.length <= LP_MAX_VECTOR_LENGTH);

   elem_type = lp_build_elem_type(type);

   if(type.floating)
      elems[0] = LLVMConstReal(elem_type, 1.0);
   else if(type.fixed)
      elems[0] = LLVMConstInt(elem_type, 1LL << (type.width/2), 0);
   else if(!type.norm)
      elems[0] = LLVMConstInt(elem_type, 1, 0);
   else {
      /* special case' -- 1.0 for normalized types is more easily attained if
       * we start with a vector consisting of all bits set */
      LLVMTypeRef vec_type = LLVMVectorType(elem_type, type.length);
      LLVMValueRef vec = LLVMConstAllOnes(vec_type);

      if(type.sign)
         vec = LLVMConstLShr(vec, LLVMConstInt(LLVMInt32Type(), 1, 0));

      return vec;
   }

   for(i = 1; i < type.length; ++i)
      elems[i] = elems[0];

   return LLVMConstVector(elems, type.length);
}
               

LLVMValueRef
lp_build_const_aos(union lp_type type, 
                   double r, double g, double b, double a, 
                   const unsigned char *swizzle)
{
   const unsigned char default_swizzle[4] = {0, 1, 2, 3};
   LLVMTypeRef elem_type;
   LLVMValueRef elems[LP_MAX_VECTOR_LENGTH];
   unsigned i;

   assert(type.length % 4 == 0);
   assert(type.length <= LP_MAX_VECTOR_LENGTH);

   elem_type = lp_build_elem_type(type);

   if(swizzle == NULL)
      swizzle = default_swizzle;

   if(type.floating) {
      elems[swizzle[0]] = LLVMConstReal(elem_type, r);
      elems[swizzle[1]] = LLVMConstReal(elem_type, g);
      elems[swizzle[2]] = LLVMConstReal(elem_type, b);
      elems[swizzle[3]] = LLVMConstReal(elem_type, a);
   }
   else {
      unsigned shift;
      long long llscale;
      double dscale;

      if(type.fixed)
         shift = type.width/2;
      else if(type.norm)
         shift = type.sign ? type.width - 1 : type.width;
      else
         shift = 0;

      llscale = (long long)1 << shift;
      dscale = (double)llscale;
      assert((long long)dscale == llscale);

      elems[swizzle[0]] = LLVMConstInt(elem_type, r*dscale + 0.5, 0);
      elems[swizzle[1]] = LLVMConstInt(elem_type, g*dscale + 0.5, 0);
      elems[swizzle[2]] = LLVMConstInt(elem_type, b*dscale + 0.5, 0);
      elems[swizzle[3]] = LLVMConstInt(elem_type, a*dscale + 0.5, 0);
   }

   for(i = 4; i < type.length; ++i)
      elems[i] = elems[i % 4];

   return LLVMConstVector(elems, type.length);
}


LLVMValueRef
lp_build_const_shift(union lp_type type,
                     int c)
{
   LLVMTypeRef elem_type = LLVMIntType(type.width);
   LLVMValueRef elems[LP_MAX_VECTOR_LENGTH];
   unsigned i;

   assert(type.length <= LP_MAX_VECTOR_LENGTH);

   for(i = 0; i < type.length; ++i)
      elems[i] = LLVMConstInt(elem_type, c, 0);

   return LLVMConstVector(elems, type.length);
}


LLVMValueRef
lp_build_const_mask_aos(union lp_type type,
                        boolean cond[4])
{
   LLVMTypeRef elem_type = LLVMIntType(type.width);
   LLVMValueRef masks[LP_MAX_VECTOR_LENGTH];
   unsigned i, j;

   assert(type.length <= LP_MAX_VECTOR_LENGTH);

   for(j = 0; j < type.length; j += 4)
      for(i = 0; i < 4; ++i)
         masks[j + i] = LLVMConstInt(elem_type, cond[i] ? ~0 : 0, 0);

   return LLVMConstVector(masks, type.length);
}
