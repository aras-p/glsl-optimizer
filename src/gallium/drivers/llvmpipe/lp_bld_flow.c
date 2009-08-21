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

#include "lp_bld_flow.h"



void
lp_build_loop_begin(LLVMBuilderRef builder,
                    LLVMValueRef start,
                    struct lp_build_loop_state *state)
{
   LLVMBasicBlockRef block = LLVMGetInsertBlock(builder);
   LLVMValueRef function = LLVMGetBasicBlockParent(block);

   state->block = LLVMAppendBasicBlock(function, "loop");

   LLVMBuildBr(builder, state->block);

   LLVMPositionBuilderAtEnd(builder, state->block);

   state->counter = LLVMBuildPhi(builder, LLVMTypeOf(start), "");

   LLVMAddIncoming(state->counter, &start, &block, 1);

}


void
lp_build_loop_end(LLVMBuilderRef builder,
                  LLVMValueRef end,
                  LLVMValueRef step,
                  struct lp_build_loop_state *state)
{
   LLVMBasicBlockRef block = LLVMGetInsertBlock(builder);
   LLVMValueRef function = LLVMGetBasicBlockParent(block);
   LLVMValueRef next;
   LLVMValueRef cond;
   LLVMBasicBlockRef after_block;

   if (!step)
      step = LLVMConstInt(LLVMTypeOf(end), 1, 0);

   next = LLVMBuildAdd(builder, state->counter, step, "");

   cond = LLVMBuildICmp(builder, LLVMIntNE, next, end, "");

   after_block = LLVMAppendBasicBlock(function, "");

   LLVMBuildCondBr(builder, cond, after_block, state->block);

   LLVMAddIncoming(state->counter, &next, &block, 1);

   LLVMPositionBuilderAtEnd(builder, after_block);
}

