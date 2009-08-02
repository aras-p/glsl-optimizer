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
 * Helper arithmetic functions.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#ifndef LP_BLD_ARIT_H
#define LP_BLD_ARIT_H


#include <llvm-c/Core.h>  

 
#define LP_MAX_VECTOR_LENGTH 16


/*
 * Types
 */


enum lp_type_kind {
   LP_TYPE_INTEGER = 0,
   LP_TYPE_FLOAT = 1,
   LP_TYPE_FIXED = 2
};


/**
 * The LLVM type system can't conveniently express all the things we care about
 * on the types used for intermediate computations, such as signed vs unsigned,
 * normalized values, or fixed point.
 */
union lp_type {
   struct {
      /** 
       * Integer. floating-point, or fixed point as established by the
       * lp_build_type_kind enum above.
       */
      unsigned kind:2;
      
      /** 
       * Whether it can represent negative values or not.
       *
       * Floating point values 
       */
      unsigned sign:1;

      /**
       * Whether values are normalized to fit [0, 1] interval, or [-1, 1] interval for
       * signed types.
       *
       * For integer types
       *
       * It makes no sense to use this with fixed point values.
       */
      unsigned norm:1;

      /**
       * Element width.
       *
       * For fixed point values, the fixed point is assumed to be at half the width.
       */
      unsigned width:14;

      /** 
       * Vector length.
       *
       * width*length should be a power of two greater or equal to height.
       *
       * Several functions can only cope with vectors of length up to
       * LP_MAX_VECTOR_LENGTH, so you may need to increase that value if you
       * want to represent bigger vectors.
       */
      unsigned length:14;
   };
   uint32_t value;
};


LLVMTypeRef
lp_build_elem_type(union lp_type type);


LLVMTypeRef
lp_build_vec_type(union lp_type type);


boolean
lp_check_elem_type(union lp_type type, LLVMTypeRef elem_type);


boolean
lp_check_vec_type(union lp_type type, LLVMTypeRef vec_type);


/*
 * Constants
 */

LLVMValueRef
lp_build_const_aos(union lp_type type, 
                   double r, double g, double b, double a, 
                   const unsigned char *swizzle);

/*
 * Basic arithmetic
 */

LLVMValueRef
lp_build_add(LLVMBuilderRef builder,
             LLVMValueRef a,
             LLVMValueRef b,
             LLVMValueRef zero);

LLVMValueRef
lp_build_sub(LLVMBuilderRef builder,
             LLVMValueRef a,
             LLVMValueRef b,
             LLVMValueRef zero);

LLVMValueRef
lp_build_mul(LLVMBuilderRef builder,
             LLVMValueRef a,
             LLVMValueRef b,
             LLVMValueRef zero,
             LLVMValueRef one);

LLVMValueRef
lp_build_min(LLVMBuilderRef builder,
             LLVMValueRef a,
             LLVMValueRef b);

LLVMValueRef
lp_build_max(LLVMBuilderRef builder,
             LLVMValueRef a,
             LLVMValueRef b);

/*
 * Satured arithmetic
 */

LLVMValueRef
lp_build_add_sat(LLVMBuilderRef builder,
                 LLVMValueRef a,
                 LLVMValueRef b,
                 LLVMValueRef zero,
                 LLVMValueRef one);

LLVMValueRef
lp_build_sub_sat(LLVMBuilderRef builder,
                 LLVMValueRef a,
                 LLVMValueRef b,
                 LLVMValueRef zero,
                 LLVMValueRef one);

LLVMValueRef
lp_build_min_sat(LLVMBuilderRef builder,
                 LLVMValueRef a,
                 LLVMValueRef b,
                 LLVMValueRef zero,
                 LLVMValueRef one);

LLVMValueRef
lp_build_max_sat(LLVMBuilderRef builder,
                 LLVMValueRef a,
                 LLVMValueRef b,
                 LLVMValueRef zero,
                 LLVMValueRef one);


#endif /* !LP_BLD_ARIT_H */
