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


union lp_type type;
struct lp_build_context;


/**
 * Complement, i.e., 1 - a.
 */
LLVMValueRef
lp_build_comp(struct lp_build_context *bld,
              LLVMValueRef a);

LLVMValueRef
lp_build_add(struct lp_build_context *bld,
             LLVMValueRef a,
             LLVMValueRef b);

LLVMValueRef
lp_build_sub(struct lp_build_context *bld,
             LLVMValueRef a,
             LLVMValueRef b);

LLVMValueRef
lp_build_mul(struct lp_build_context *bld,
             LLVMValueRef a,
             LLVMValueRef b);

LLVMValueRef
lp_build_div(struct lp_build_context *bld,
             LLVMValueRef a,
             LLVMValueRef b);

LLVMValueRef
lp_build_min(struct lp_build_context *bld,
             LLVMValueRef a,
             LLVMValueRef b);

LLVMValueRef
lp_build_max(struct lp_build_context *bld,
             LLVMValueRef a,
             LLVMValueRef b);

LLVMValueRef
lp_build_abs(struct lp_build_context *bld,
             LLVMValueRef a);

LLVMValueRef
lp_build_sqrt(struct lp_build_context *bld,
              LLVMValueRef a);

LLVMValueRef
lp_build_rcp(struct lp_build_context *bld,
             LLVMValueRef a);

LLVMValueRef
lp_build_rsqrt(struct lp_build_context *bld,
               LLVMValueRef a);

LLVMValueRef
lp_build_cos(struct lp_build_context *bld,
             LLVMValueRef a);

LLVMValueRef
lp_build_sin(struct lp_build_context *bld,
             LLVMValueRef a);

LLVMValueRef
lp_build_pow(struct lp_build_context *bld,
             LLVMValueRef a,
             LLVMValueRef b);

LLVMValueRef
lp_build_exp(struct lp_build_context *bld,
             LLVMValueRef a);

LLVMValueRef
lp_build_log(struct lp_build_context *bld,
             LLVMValueRef a);

LLVMValueRef
lp_build_exp2(struct lp_build_context *bld,
              LLVMValueRef a);

LLVMValueRef
lp_build_log2(struct lp_build_context *bld,
              LLVMValueRef a);

void
lp_build_exp2_approx(struct lp_build_context *bld,
                     LLVMValueRef x,
                     LLVMValueRef *p_exp2_int_part,
                     LLVMValueRef *p_frac_part,
                     LLVMValueRef *p_exp2);

void
lp_build_log2_approx(struct lp_build_context *bld,
                     LLVMValueRef x,
                     LLVMValueRef *p_exp,
                     LLVMValueRef *p_floor_log2,
                     LLVMValueRef *p_log2);

#endif /* !LP_BLD_ARIT_H */
