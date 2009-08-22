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

#include "util/u_debug.h"

#include "lp_bld_type.h"
#include "lp_bld_flow.h"


void
lp_build_mask_begin(struct lp_build_mask_context *mask,
                    LLVMBuilderRef builder,
                    union lp_type type,
                    LLVMValueRef value)
{
   memset(mask, 0, sizeof *mask);

   mask->builder = builder;
   mask->reg_type = LLVMIntType(type.width * type.length);
   mask->value = value;
}


void
lp_build_mask_update(struct lp_build_mask_context *mask,
                     LLVMValueRef value)
{

   LLVMValueRef cond;
   LLVMBasicBlockRef current_block;
   LLVMBasicBlockRef next_block;
   LLVMBasicBlockRef new_block;

   if(mask->value)
      mask->value = LLVMBuildAnd(mask->builder, mask->value, value, "");
   else
      mask->value = value;

   /* FIXME: disabled until we have proper control flow helpers */
#if 0
   cond = LLVMBuildICmp(mask->builder,
                        LLVMIntEQ,
                        LLVMBuildBitCast(mask->builder, mask->value, mask->reg_type, ""),
                        LLVMConstNull(mask->reg_type),
                        "");

   current_block = LLVMGetInsertBlock(mask->builder);

   if(!mask->skip_block) {
      LLVMValueRef function = LLVMGetBasicBlockParent(current_block);
      mask->skip_block = LLVMAppendBasicBlock(function, "skip");

      mask->phi = LLVMBuildPhi(mask->builder, LLVMTypeOf(mask->value), "");
   }

   next_block = LLVMGetNextBasicBlock(current_block);
   assert(next_block);
   if(next_block) {
      new_block = LLVMInsertBasicBlock(next_block, "");
   }
   else {
      LLVMValueRef function = LLVMGetBasicBlockParent(current_block);
      new_block = LLVMAppendBasicBlock(function, "");
   }

   LLVMAddIncoming(mask->phi, &mask->value, &current_block, 1);
   LLVMBuildCondBr(mask->builder, cond, mask->skip_block, new_block);

   LLVMPositionBuilderAtEnd(mask->builder, new_block);
#endif
}


LLVMValueRef
lp_build_mask_end(struct lp_build_mask_context *mask)
{
   if(mask->skip_block) {
      LLVMBasicBlockRef current_block = LLVMGetInsertBlock(mask->builder);

      LLVMAddIncoming(mask->phi, &mask->value, &current_block, 1);
      LLVMBuildBr(mask->builder, mask->skip_block);

      LLVMPositionBuilderAtEnd(mask->builder, mask->skip_block);

      mask->value = mask->phi;
      mask->phi = NULL;
      mask->skip_block = NULL;
   }

   return mask->value;
}



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

