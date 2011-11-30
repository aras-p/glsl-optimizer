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
 * Convenient representation of SIMD types.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#ifndef LP_BLD_TYPE_H
#define LP_BLD_TYPE_H


#include "pipe/p_compiler.h"
#include "gallivm/lp_bld.h"



/**
 * Native SIMD register width.
 *
 * 128 for all architectures we care about.
 */
#define LP_NATIVE_VECTOR_WIDTH 128

/**
 * Several functions can only cope with vectors of length up to this value.
 * You may need to increase that value if you want to represent bigger vectors.
 */
#define LP_MAX_VECTOR_LENGTH 16


/**
 * The LLVM type system can't conveniently express all the things we care about
 * on the types used for intermediate computations, such as signed vs unsigned,
 * normalized values, or fixed point.
 */
struct lp_type {
   /**
    * Floating-point. Cannot be used with fixed. Integer numbers are
    * represented by this zero.
    */
   unsigned floating:1;

   /**
    * Fixed-point. Cannot be used with floating. Integer numbers are
    * represented by this zero.
    */
   unsigned fixed:1;

   /**
    * Whether it can represent negative values or not.
    *
    * If this is not set for floating point, it means that all values are
    * assumed to be positive.
    */
   unsigned sign:1;

   /**
    * Whether values are normalized to fit [0, 1] interval, or [-1, 1]
    * interval for signed types.
    *
    * For integer types it means the representable integer range should be
    * interpreted as the interval above.
    *
    * For floating and fixed point formats it means the values should be
    * clamped to the interval above.
    */
   unsigned norm:1;

   /**
    * Element width.
    *
    * For fixed point values, the fixed point is assumed to be at half the
    * width.
    */
   unsigned width:14;

   /**
    * Vector length.  If length==1, this is a scalar (float/int) type.
    *
    * width*length should be a power of two greater or equal to eight.
    *
    * @sa LP_MAX_VECTOR_LENGTH
    */
   unsigned length:14;
};


/**
 * We need most of the information here in order to correctly and efficiently
 * translate an arithmetic operation into LLVM IR. Putting it here avoids the
 * trouble of passing it as parameters.
 */
struct lp_build_context
{
   struct gallivm_state *gallivm;

   /**
    * This not only describes the input/output LLVM types, but also whether
    * to normalize/clamp the results.
    */
   struct lp_type type;

   /** Same as lp_build_elem_type(type) */
   LLVMTypeRef elem_type;

   /** Same as lp_build_vec_type(type) */
   LLVMTypeRef vec_type;

   /** Same as lp_build_int_elem_type(type) */
   LLVMTypeRef int_elem_type;

   /** Same as lp_build_int_vec_type(type) */
   LLVMTypeRef int_vec_type;

   /** Same as lp_build_undef(type) */
   LLVMValueRef undef;

   /** Same as lp_build_zero(type) */
   LLVMValueRef zero;

   /** Same as lp_build_one(type) */
   LLVMValueRef one;
};


/** Create scalar float type */
static INLINE struct lp_type
lp_type_float(unsigned width)
{
   struct lp_type res_type;

   memset(&res_type, 0, sizeof res_type);
   res_type.floating = TRUE;
   res_type.sign = TRUE;
   res_type.width = width;
   res_type.length = 1;

   return res_type;
}


/** Create vector of float type */
static INLINE struct lp_type
lp_type_float_vec(unsigned width)
{
   struct lp_type res_type;

   memset(&res_type, 0, sizeof res_type);
   res_type.floating = TRUE;
   res_type.sign = TRUE;
   res_type.width = width;
   res_type.length = LP_NATIVE_VECTOR_WIDTH / width;

   return res_type;
}


/** Create scalar int type */
static INLINE struct lp_type
lp_type_int(unsigned width)
{
   struct lp_type res_type;

   memset(&res_type, 0, sizeof res_type);
   res_type.sign = TRUE;
   res_type.width = width;
   res_type.length = 1;

   return res_type;
}


/** Create vector int type */
static INLINE struct lp_type
lp_type_int_vec(unsigned width)
{
   struct lp_type res_type;

   memset(&res_type, 0, sizeof res_type);
   res_type.sign = TRUE;
   res_type.width = width;
   res_type.length = LP_NATIVE_VECTOR_WIDTH / width;

   return res_type;
}


/** Create scalar uint type */
static INLINE struct lp_type
lp_type_uint(unsigned width)
{
   struct lp_type res_type;

   memset(&res_type, 0, sizeof res_type);
   res_type.width = width;
   res_type.length = 1;

   return res_type;
}


/** Create vector uint type */
static INLINE struct lp_type
lp_type_uint_vec(unsigned width)
{
   struct lp_type res_type;

   memset(&res_type, 0, sizeof res_type);
   res_type.width = width;
   res_type.length = LP_NATIVE_VECTOR_WIDTH / width;

   return res_type;
}


static INLINE struct lp_type
lp_type_unorm(unsigned width)
{
   struct lp_type res_type;

   memset(&res_type, 0, sizeof res_type);
   res_type.norm = TRUE;
   res_type.width = width;
   res_type.length = LP_NATIVE_VECTOR_WIDTH / width;

   return res_type;
}


static INLINE struct lp_type
lp_type_fixed(unsigned width)
{
   struct lp_type res_type;

   memset(&res_type, 0, sizeof res_type);
   res_type.sign = TRUE;
   res_type.fixed = TRUE;
   res_type.width = width;
   res_type.length = LP_NATIVE_VECTOR_WIDTH / width;

   return res_type;
}


static INLINE struct lp_type
lp_type_ufixed(unsigned width)
{
   struct lp_type res_type;

   memset(&res_type, 0, sizeof res_type);
   res_type.fixed = TRUE;
   res_type.width = width;
   res_type.length = LP_NATIVE_VECTOR_WIDTH / width;

   return res_type;
}


LLVMTypeRef
lp_build_elem_type(struct gallivm_state *gallivm, struct lp_type type);


LLVMTypeRef
lp_build_vec_type(struct gallivm_state *gallivm, struct lp_type type);


boolean
lp_check_elem_type(struct lp_type type, LLVMTypeRef elem_type);


boolean
lp_check_vec_type(struct lp_type type, LLVMTypeRef vec_type);


boolean
lp_check_value(struct lp_type type, LLVMValueRef val);


LLVMTypeRef
lp_build_int_elem_type(struct gallivm_state *gallivm, struct lp_type type);


LLVMTypeRef
lp_build_int_vec_type(struct gallivm_state *gallivm, struct lp_type type);


LLVMTypeRef
lp_build_int32_vec4_type(struct gallivm_state *gallivm);


static INLINE struct lp_type
lp_float32_vec4_type(void)
{
   struct lp_type type;

   memset(&type, 0, sizeof(type));
   type.floating = TRUE;
   type.sign = TRUE;
   type.norm = FALSE;
   type.width = 32;
   type.length = 4;

   return type;
}


static INLINE struct lp_type
lp_int32_vec4_type(void)
{
   struct lp_type type;

   memset(&type, 0, sizeof(type));
   type.floating = FALSE;
   type.sign = TRUE;
   type.norm = FALSE;
   type.width = 32;
   type.length = 4;

   return type;
}


static INLINE struct lp_type
lp_unorm8_vec4_type(void)
{
   struct lp_type type;

   memset(&type, 0, sizeof(type));
   type.floating = FALSE;
   type.sign = FALSE;
   type.norm = TRUE;
   type.width = 8;
   type.length = 4;

   return type;
}


struct lp_type
lp_elem_type(struct lp_type type);


struct lp_type
lp_uint_type(struct lp_type type);


struct lp_type
lp_int_type(struct lp_type type);


struct lp_type
lp_wider_type(struct lp_type type);


unsigned
lp_sizeof_llvm_type(LLVMTypeRef t);


const char *
lp_typekind_name(LLVMTypeKind t);


void
lp_dump_llvmtype(LLVMTypeRef t);


void
lp_build_context_init(struct lp_build_context *bld,
                      struct gallivm_state *gallivm,
                      struct lp_type type);


unsigned
lp_build_count_instructions(LLVMValueRef function);


#endif /* !LP_BLD_TYPE_H */
