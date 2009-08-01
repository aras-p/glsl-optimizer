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

#ifndef LP_BLD_ARIT_H
#define LP_BLD_ARIT_H


#include <llvm-c/Core.h>  

 
#define LP_MAX_VECTOR_SIZE 16


/*
 * Constants
 */

LLVMValueRef
lp_build_const_aos(LLVMTypeRef type, 
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
