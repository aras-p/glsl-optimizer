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
 * Helper functions for swizzling/shuffling.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */


#ifndef LP_BLD_SWIZZLE_H
#define LP_BLD_SWIZZLE_H


#include <llvm-c/Core.h>  


struct lp_type;
struct lp_build_context;


LLVMValueRef
lp_build_broadcast(LLVMBuilderRef builder,
                   LLVMTypeRef vec_type,
                   LLVMValueRef scalar);


LLVMValueRef
lp_build_broadcast_scalar(struct lp_build_context *bld,
                          LLVMValueRef scalar);


/**
 * Broadcast one channel of a vector composed of arrays of XYZW structures into
 * all four channel.
 */
LLVMValueRef
lp_build_broadcast_aos(struct lp_build_context *bld,
                       LLVMValueRef a,
                       unsigned channel);


/**
 * Swizzle a vector consisting of an array of XYZW structs.
 *
 * @param swizzle is the in [0,4[ range.
 */
LLVMValueRef
lp_build_swizzle1_aos(struct lp_build_context *bld,
                      LLVMValueRef a,
                      const unsigned char swizzle[4]);


/**
 * Swizzle two vector consisting of an array of XYZW structs.
 *
 * @param swizzle is the in [0,8[ range. Values in [4,8[ range refer to b.
 */
LLVMValueRef
lp_build_swizzle2_aos(struct lp_build_context *bld,
                      LLVMValueRef a,
                      LLVMValueRef b,
                      const unsigned char swizzle[4]);


#endif /* !LP_BLD_SWIZZLE_H */
