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
#include "util/u_memory.h"

#include "lp_bld_type.h"
#include "lp_bld_flow.h"


#define LP_BUILD_FLOW_MAX_VARIABLES 32
#define LP_BUILD_FLOW_MAX_DEPTH 32


/**
 * Enumeration of all possible flow constructs.
 */
enum lp_build_flow_construct_kind {
   lP_BUILD_FLOW_SCOPE,
   LP_BUILD_FLOW_SKIP,
};


/**
 * Variable declaration scope.
 */
struct lp_build_flow_scope
{
   /** Number of variables declared in this scope */
   unsigned num_variables;
};


/**
 * Early exit. Useful to skip to the end of a function or block when
 * the execution mask becomes zero or when there is an error condition.
 */
struct lp_build_flow_skip
{
   /** Block to skip to */
   LLVMBasicBlockRef block;

   /** Number of variables declared at the beginning */
   unsigned num_variables;

   LLVMValueRef *phi;
};


/**
 * Union of all possible flow constructs' data
 */
union lp_build_flow_construct_data
{
   struct lp_build_flow_scope scope;
   struct lp_build_flow_skip skip;
};


/**
 * Element of the flow construct stack.
 */
struct lp_build_flow_construct
{
   enum lp_build_flow_construct_kind kind;
   union lp_build_flow_construct_data data;
};


/**
 * All necessary data to generate LLVM control flow constructs.
 *
 * Besides keeping track of the control flow construct themselves we also
 * need to keep track of variables in order to generate SSA Phi values.
 */
struct lp_build_flow_context
{
   LLVMBuilderRef builder;

   /**
    * Control flow stack.
    */
   struct lp_build_flow_construct constructs[LP_BUILD_FLOW_MAX_DEPTH];
   unsigned num_constructs;

   /**
    * Variable stack
    */
   LLVMValueRef *variables[LP_BUILD_FLOW_MAX_VARIABLES];
   unsigned num_variables;
};


struct lp_build_flow_context *
lp_build_flow_create(LLVMBuilderRef builder)
{
   struct lp_build_flow_context *flow;

   flow = CALLOC_STRUCT(lp_build_flow_context);
   if(!flow)
      return NULL;

   flow->builder = builder;

   return flow;
}


void
lp_build_flow_destroy(struct lp_build_flow_context *flow)
{
   assert(flow->num_constructs == 0);
   assert(flow->num_variables == 0);
   FREE(flow);
}


static union lp_build_flow_construct_data *
lp_build_flow_push(struct lp_build_flow_context *flow,
                   enum lp_build_flow_construct_kind kind)
{
   assert(flow->num_constructs < LP_BUILD_FLOW_MAX_DEPTH);
   if(flow->num_constructs >= LP_BUILD_FLOW_MAX_DEPTH)
      return NULL;

   flow->constructs[flow->num_constructs].kind = kind;
   return &flow->constructs[flow->num_constructs++].data;
}


static union lp_build_flow_construct_data *
lp_build_flow_peek(struct lp_build_flow_context *flow,
                   enum lp_build_flow_construct_kind kind)
{
   assert(flow->num_constructs);
   if(!flow->num_constructs)
      return NULL;

   assert(flow->constructs[flow->num_constructs - 1].kind == kind);
   if(flow->constructs[flow->num_constructs - 1].kind != kind)
      return NULL;

   return &flow->constructs[flow->num_constructs - 1].data;
}


static union lp_build_flow_construct_data *
lp_build_flow_pop(struct lp_build_flow_context *flow,
                  enum lp_build_flow_construct_kind kind)
{
   assert(flow->num_constructs);
   if(!flow->num_constructs)
      return NULL;

   assert(flow->constructs[flow->num_constructs - 1].kind == kind);
   if(flow->constructs[flow->num_constructs - 1].kind != kind)
      return NULL;

   return &flow->constructs[--flow->num_constructs].data;
}


/**
 * Begin a variable scope.
 *
 *
 */
void
lp_build_flow_scope_begin(struct lp_build_flow_context *flow)
{
   struct lp_build_flow_scope *scope;

   scope = &lp_build_flow_push(flow, lP_BUILD_FLOW_SCOPE)->scope;
   if(!scope)
      return;

   scope->num_variables = 0;
}


/**
 * Declare a variable.
 *
 * A variable is a named entity which can have different LLVMValueRef's at
 * different points of the program. This is relevant for control flow because
 * when there are mutiple branches to a same location we need to replace
 * the variable's value with a Phi function as explained in
 * http://en.wikipedia.org/wiki/Static_single_assignment_form .
 *
 * We keep track of variables by keeping around a pointer to where their
 * current.
 *
 * There are a few cautions to observe:
 *
 * - Variable's value must not be NULL. If there is no initial value then
 *   LLVMGetUndef() should be used.
 *
 * - Variable's value must be kept up-to-date. If the variable is going to be
 *   modified by a function then a pointer should be passed so that its value
 *   is accurate. Failure to do this will cause some of the variables'
 *   transient values to be lost, leading to wrong results.
 *
 * - A program should be written from top to bottom, by always appending
 *   instructions to the bottom with a single LLVMBuilderRef. Inserting and/or
 *   modifying existing statements will most likely lead to wrong results.
 *
 */
void
lp_build_flow_scope_declare(struct lp_build_flow_context *flow,
                            LLVMValueRef *variable)
{
   struct lp_build_flow_scope *scope;

   scope = &lp_build_flow_peek(flow, lP_BUILD_FLOW_SCOPE)->scope;
   if(!scope)
      return;

   assert(*variable);
   if(!*variable)
      return;

   assert(flow->num_variables < LP_BUILD_FLOW_MAX_VARIABLES);
   if(flow->num_variables >= LP_BUILD_FLOW_MAX_VARIABLES)
      return;

   flow->variables[flow->num_variables++] = variable;
   ++scope->num_variables;
}


void
lp_build_flow_scope_end(struct lp_build_flow_context *flow)
{
   struct lp_build_flow_scope *scope;

   scope = &lp_build_flow_pop(flow, lP_BUILD_FLOW_SCOPE)->scope;
   if(!scope)
      return;

   assert(flow->num_variables >= scope->num_variables);
   if(flow->num_variables < scope->num_variables) {
      flow->num_variables = 0;
      return;
   }

   flow->num_variables -= scope->num_variables;
}


static LLVMBasicBlockRef
lp_build_flow_insert_block(struct lp_build_flow_context *flow)
{
   LLVMBasicBlockRef current_block;
   LLVMBasicBlockRef next_block;
   LLVMBasicBlockRef new_block;

   current_block = LLVMGetInsertBlock(flow->builder);

   next_block = LLVMGetNextBasicBlock(current_block);
   if(next_block) {
      new_block = LLVMInsertBasicBlock(next_block, "");
   }
   else {
      LLVMValueRef function = LLVMGetBasicBlockParent(current_block);
      new_block = LLVMAppendBasicBlock(function, "");
   }

   return new_block;
}

void
lp_build_flow_skip_begin(struct lp_build_flow_context *flow)
{
   struct lp_build_flow_skip *skip;
   LLVMBuilderRef builder;
   unsigned i;

   skip = &lp_build_flow_push(flow, LP_BUILD_FLOW_SKIP)->skip;
   if(!skip)
      return;

   skip->block = lp_build_flow_insert_block(flow);
   skip->num_variables = flow->num_variables;
   if(!skip->num_variables) {
      skip->phi = NULL;
      return;
   }

   skip->phi = MALLOC(skip->num_variables * sizeof *skip->phi);
   if(!skip->phi) {
      skip->num_variables = 0;
      return;
   }

   builder = LLVMCreateBuilder();
   LLVMPositionBuilderAtEnd(builder, skip->block);

   for(i = 0; i < skip->num_variables; ++i)
      skip->phi[i] = LLVMBuildPhi(builder, LLVMTypeOf(*flow->variables[i]), "");

   LLVMDisposeBuilder(builder);
}


void
lp_build_flow_skip_cond_break(struct lp_build_flow_context *flow,
                              LLVMValueRef cond)
{
   struct lp_build_flow_skip *skip;
   LLVMBasicBlockRef current_block;
   LLVMBasicBlockRef new_block;
   unsigned i;

   skip = &lp_build_flow_peek(flow, LP_BUILD_FLOW_SKIP)->skip;
   if(!skip)
      return;

   current_block = LLVMGetInsertBlock(flow->builder);

   new_block = lp_build_flow_insert_block(flow);

   for(i = 0; i < skip->num_variables; ++i) {
      assert(*flow->variables[i]);
      LLVMAddIncoming(skip->phi[i], flow->variables[i], &current_block, 1);
   }

   LLVMBuildCondBr(flow->builder, cond, skip->block, new_block);

   LLVMPositionBuilderAtEnd(flow->builder, new_block);
 }


void
lp_build_flow_skip_end(struct lp_build_flow_context *flow)
{
   struct lp_build_flow_skip *skip;
   LLVMBasicBlockRef current_block;
   unsigned i;

   skip = &lp_build_flow_pop(flow, LP_BUILD_FLOW_SKIP)->skip;
   if(!skip)
      return;

   current_block = LLVMGetInsertBlock(flow->builder);

   for(i = 0; i < skip->num_variables; ++i) {
      assert(*flow->variables[i]);
      LLVMAddIncoming(skip->phi[i], flow->variables[i], &current_block, 1);
      *flow->variables[i] = skip->phi[i];
   }

   LLVMBuildBr(flow->builder, skip->block);
   LLVMPositionBuilderAtEnd(flow->builder, skip->block);

   FREE(skip->phi);
}


static void
lp_build_mask_check(struct lp_build_mask_context *mask)
{
   LLVMBuilderRef builder = mask->flow->builder;
   LLVMValueRef cond;

   cond = LLVMBuildICmp(builder,
                        LLVMIntEQ,
                        LLVMBuildBitCast(builder, mask->value, mask->reg_type, ""),
                        LLVMConstNull(mask->reg_type),
                        "");

   lp_build_flow_skip_cond_break(mask->flow, cond);
}


void
lp_build_mask_begin(struct lp_build_mask_context *mask,
                    struct lp_build_flow_context *flow,
                    struct lp_type type,
                    LLVMValueRef value)
{
   memset(mask, 0, sizeof *mask);

   mask->flow = flow;
   mask->reg_type = LLVMIntType(type.width * type.length);
   mask->value = value;

   lp_build_flow_scope_begin(flow);
   lp_build_flow_scope_declare(flow, &mask->value);
   lp_build_flow_skip_begin(flow);

   lp_build_mask_check(mask);
}


void
lp_build_mask_update(struct lp_build_mask_context *mask,
                     LLVMValueRef value)
{
   mask->value = LLVMBuildAnd( mask->flow->builder, mask->value, value, "");

   lp_build_mask_check(mask);
}


LLVMValueRef
lp_build_mask_end(struct lp_build_mask_context *mask)
{
   lp_build_flow_skip_end(mask->flow);
   lp_build_flow_scope_end(mask->flow);
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

