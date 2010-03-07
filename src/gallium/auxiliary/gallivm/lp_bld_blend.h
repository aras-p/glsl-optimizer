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

#ifndef LP_BLD_BLEND_H
#define LP_BLD_BLEND_H


/**
 * @file
 * LLVM IR building helpers interfaces.
 *
 * We use LLVM-C bindings for now. They are not documented, but follow the C++
 * interfaces very closely, and appear to be complete enough for code
 * genration. See
 * http://npcontemplation.blogspot.com/2008/06/secret-of-llvm-c-bindings.html
 * for a standalone example.
 */

#include <llvm-c/Core.h>  
 
#include "pipe/p_format.h"


struct pipe_blend_state;
struct lp_type;
struct lp_build_context;


/**
 * Whether the blending function is commutative or not.
 */
boolean
lp_build_blend_func_commutative(unsigned func);


/**
 * Whether the blending functions are the reverse of each other.
 */
boolean
lp_build_blend_func_reverse(unsigned rgb_func, unsigned alpha_func);


LLVMValueRef
lp_build_blend_func(struct lp_build_context *bld,
                    unsigned func,
                    LLVMValueRef term1,
                    LLVMValueRef term2);


LLVMValueRef
lp_build_blend_aos(LLVMBuilderRef builder,
                   const struct pipe_blend_state *blend,
                   struct lp_type type,
                   LLVMValueRef src,
                   LLVMValueRef dst,
                   LLVMValueRef const_,
                   unsigned alpha_swizzle);


void
lp_build_blend_soa(LLVMBuilderRef builder,
                   const struct pipe_blend_state *blend,
                   struct lp_type type,
                   LLVMValueRef src[4],
                   LLVMValueRef dst[4],
                   LLVMValueRef const_[4],
                   LLVMValueRef res[4]);


/**
 * Apply a logic op.
 *
 * src/dst parameters are packed values. It should work regardless the inputs
 * are scalars, or a vector.
 */
LLVMValueRef
lp_build_logicop(LLVMBuilderRef builder,
                 unsigned logicop_func,
                 LLVMValueRef src,
                 LLVMValueRef dst);


#endif /* !LP_BLD_BLEND_H */
