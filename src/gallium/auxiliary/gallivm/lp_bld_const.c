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

#include <float.h>

#include "util/u_debug.h"

#include "lp_bld_type.h"
#include "lp_bld_const.h"


unsigned
lp_mantissa(struct lp_type type)
{
   assert(type.floating);

   if(type.floating) {
      switch(type.width) {
      case 32:
         return 23;
      case 64:
         return 53;
      default:
         assert(0);
         return 0;
      }
   }
   else {
      if(type.sign)
         return type.width - 1;
      else
         return type.width;
   }
}


/**
 * Shift of the unity.
 *
 * Same as lp_const_scale(), but in terms of shifts.
 */
unsigned
lp_const_shift(struct lp_type type)
{
   if(type.floating)
      return 0;
   else if(type.fixed)
      return type.width/2;
   else if(type.norm)
      return type.sign ? type.width - 1 : type.width;
   else
      return 0;
}


unsigned
lp_const_offset(struct lp_type type)
{
   if(type.floating || type.fixed)
      return 0;
   else if(type.norm)
      return 1;
   else
      return 0;
}


/**
 * Scaling factor between the LLVM native value and its interpretation.
 *
 * This is 1.0 for all floating types and unnormalized integers, and something
 * else for the fixed points types and normalized integers.
 */
double
lp_const_scale(struct lp_type type)
{
   unsigned long long llscale;
   double dscale;

   llscale = (unsigned long long)1 << lp_const_shift(type);
   llscale -= lp_const_offset(type);
   dscale = (double)llscale;
   assert((unsigned long long)dscale == llscale);

   return dscale;
}


/**
 * Minimum value representable by the type.
 */
double
lp_const_min(struct lp_type type)
{
   unsigned bits;

   if(!type.sign)
      return 0.0;

   if(type.norm)
      return -1.0;

   if (type.floating) {
      switch(type.width) {
      case 32:
         return -FLT_MAX;
      case 64:
         return -DBL_MAX;
      default:
         assert(0);
         return 0.0;
      }
   }

   if(type.fixed)
      /* FIXME: consider the fractional bits? */
      bits = type.width / 2 - 1;
   else
      bits = type.width - 1;

   return (double)-((long long)1 << bits);
}


/**
 * Maximum value representable by the type.
 */
double
lp_const_max(struct lp_type type)
{
   unsigned bits;

   if(type.norm)
      return 1.0;

   if (type.floating) {
      switch(type.width) {
      case 32:
         return FLT_MAX;
      case 64:
         return DBL_MAX;
      default:
         assert(0);
         return 0.0;
      }
   }

   if(type.fixed)
      bits = type.width / 2;
   else
      bits = type.width;

   if(type.sign)
      bits -= 1;

   return (double)(((unsigned long long)1 << bits) - 1);
}


double
lp_const_eps(struct lp_type type)
{
   if (type.floating) {
      switch(type.width) {
      case 32:
         return FLT_EPSILON;
      case 64:
         return DBL_EPSILON;
      default:
         assert(0);
         return 0.0;
      }
   }
   else {
      double scale = lp_const_scale(type);
      return 1.0/scale;
   }
}


LLVMValueRef
lp_build_undef(struct lp_type type)
{
   LLVMTypeRef vec_type = lp_build_vec_type(type);
   return LLVMGetUndef(vec_type);
}
               

LLVMValueRef
lp_build_zero(struct lp_type type)
{
   LLVMTypeRef vec_type = lp_build_vec_type(type);
   return LLVMConstNull(vec_type);
}
               

LLVMValueRef
lp_build_one(struct lp_type type)
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
   else if(type.sign)
      elems[0] = LLVMConstInt(elem_type, (1LL << (type.width - 1)) - 1, 0);
   else {
      /* special case' -- 1.0 for normalized types is more easily attained if
       * we start with a vector consisting of all bits set */
      LLVMTypeRef vec_type = LLVMVectorType(elem_type, type.length);
      LLVMValueRef vec = LLVMConstAllOnes(vec_type);

#if 0
      if(type.sign)
         /* TODO: Unfortunately this caused "Tried to create a shift operation
          * on a non-integer type!" */
         vec = LLVMConstLShr(vec, lp_build_int_const_scalar(type, 1));
#endif

      return vec;
   }

   for(i = 1; i < type.length; ++i)
      elems[i] = elems[0];

   return LLVMConstVector(elems, type.length);
}
               

LLVMValueRef
lp_build_const_scalar(struct lp_type type,
                      double val)
{
   LLVMTypeRef elem_type = lp_build_elem_type(type);
   LLVMValueRef elems[LP_MAX_VECTOR_LENGTH];
   unsigned i;

   assert(type.length <= LP_MAX_VECTOR_LENGTH);

   if(type.floating) {
      elems[0] = LLVMConstReal(elem_type, val);
   }
   else {
      double dscale = lp_const_scale(type);

      elems[0] = LLVMConstInt(elem_type, val*dscale + 0.5, 0);
   }

   for(i = 1; i < type.length; ++i)
      elems[i] = elems[0];

   return LLVMConstVector(elems, type.length);
}


LLVMValueRef
lp_build_int_const_scalar(struct lp_type type,
                          long long val)
{
   LLVMTypeRef elem_type = lp_build_int_elem_type(type);
   LLVMValueRef elems[LP_MAX_VECTOR_LENGTH];
   unsigned i;

   assert(type.length <= LP_MAX_VECTOR_LENGTH);

   for(i = 0; i < type.length; ++i)
      elems[i] = LLVMConstInt(elem_type, val, type.sign ? 1 : 0);

   return LLVMConstVector(elems, type.length);
}


LLVMValueRef
lp_build_const_aos(struct lp_type type, 
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
      double dscale = lp_const_scale(type);

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
lp_build_const_mask_aos(struct lp_type type,
                        const boolean cond[4])
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
