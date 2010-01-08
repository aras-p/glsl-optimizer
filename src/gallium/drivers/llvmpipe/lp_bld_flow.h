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
 * LLVM control flow build helpers.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */

#ifndef LP_BLD_FLOW_H
#define LP_BLD_FLOW_H


#include <llvm-c/Core.h>  


struct lp_type;


struct lp_build_flow_context;


struct lp_build_flow_context *
lp_build_flow_create(LLVMBuilderRef builder);

void
lp_build_flow_destroy(struct lp_build_flow_context *flow);

void
lp_build_flow_scope_begin(struct lp_build_flow_context *flow);

void
lp_build_flow_scope_declare(struct lp_build_flow_context *flow,
                            LLVMValueRef *variable);

void
lp_build_flow_scope_end(struct lp_build_flow_context *flow);

void
lp_build_flow_skip_begin(struct lp_build_flow_context *flow);

void
lp_build_flow_skip_cond_break(struct lp_build_flow_context *flow,
                              LLVMValueRef cond);

void
lp_build_flow_skip_end(struct lp_build_flow_context *flow);


struct lp_build_mask_context
{
   struct lp_build_flow_context *flow;

   LLVMTypeRef reg_type;

   LLVMValueRef value;
};


void
lp_build_mask_begin(struct lp_build_mask_context *mask,
                    struct lp_build_flow_context *flow,
                    struct lp_type type,
                    LLVMValueRef value);

/**
 * Bitwise AND the mask with the given value, if a previous mask was set.
 */
void
lp_build_mask_update(struct lp_build_mask_context *mask,
                     LLVMValueRef value);

LLVMValueRef
lp_build_mask_end(struct lp_build_mask_context *mask);


/**
 * LLVM's IR doesn't represent for-loops directly. Furthermore it
 * it requires creating code blocks, branches, phi variables, so it
 * requires a fair amount of code.
 *
 * @sa http://www.llvm.org/docs/tutorial/LangImpl5.html#for
 */
struct lp_build_loop_state
{
  LLVMBasicBlockRef block;
  LLVMValueRef counter;
};


void
lp_build_loop_begin(LLVMBuilderRef builder,
                    LLVMValueRef start,
                    struct lp_build_loop_state *state);


void
lp_build_loop_end(LLVMBuilderRef builder,
                  LLVMValueRef end,
                  LLVMValueRef step,
                  struct lp_build_loop_state *state);




struct lp_build_if_state
{
   LLVMBuilderRef builder;
   struct lp_build_flow_context *flow;
};


void
lp_build_if(struct lp_build_if_state *ctx,
            struct lp_build_flow_context *flow,
            LLVMBuilderRef builder,
            LLVMValueRef condition);

void
lp_build_else(struct lp_build_if_state *ctx);

void
lp_build_endif(struct lp_build_if_state *ctx);
              


#endif /* !LP_BLD_FLOW_H */
