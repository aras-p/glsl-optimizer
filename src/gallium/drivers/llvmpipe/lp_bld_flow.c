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
   LP_BUILD_FLOW_SCOPE,
   LP_BUILD_FLOW_SKIP,
   LP_BUILD_FLOW_IF
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

   LLVMValueRef *phi;  /**< array [num_variables] */
};


/**
 * if/else/endif.
 */
struct lp_build_flow_if
{
   unsigned num_variables;

   LLVMValueRef *phi;  /**< array [num_variables] */

   LLVMValueRef condition;
   LLVMBasicBlockRef entry_block, true_block, false_block, merge_block;
};


/**
 * Union of all possible flow constructs' data
 */
union lp_build_flow_construct_data
{
   struct lp_build_flow_scope scope;
   struct lp_build_flow_skip skip;
   struct lp_build_flow_if ifthen;
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


/**
 * Begin/push a new flow control construct, such as a loop, skip block
 * or variable scope.
 */
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


/**
 * Return the current/top flow control construct on the stack.
 * \param kind  the expected type of the top-most construct
 */
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


/**
 * End/pop the current/top flow control construct on the stack.
 * \param kind  the expected type of the top-most construct
 */
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

   scope = &lp_build_flow_push(flow, LP_BUILD_FLOW_SCOPE)->scope;
   if(!scope)
      return;

   scope->num_variables = 0;
}


/**
 * Declare a variable.
 *
 * A variable is a named entity which can have different LLVMValueRef's at
 * different points of the program. This is relevant for control flow because
 * when there are multiple branches to a same location we need to replace
 * the variable's value with a Phi function as explained in
 * http://en.wikipedia.org/wiki/Static_single_assignment_form .
 *
 * We keep track of variables by keeping around a pointer to where they're
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

   scope = &lp_build_flow_peek(flow, LP_BUILD_FLOW_SCOPE)->scope;
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

   scope = &lp_build_flow_pop(flow, LP_BUILD_FLOW_SCOPE)->scope;
   if(!scope)
      return;

   assert(flow->num_variables >= scope->num_variables);
   if(flow->num_variables < scope->num_variables) {
      flow->num_variables = 0;
      return;
   }

   flow->num_variables -= scope->num_variables;
}


/**
 * Note: this function has no dependencies on the flow code and could
 * be used elsewhere.
 */
static LLVMBasicBlockRef
lp_build_insert_new_block(LLVMBuilderRef builder, const char *name)
{
   LLVMBasicBlockRef current_block;
   LLVMBasicBlockRef next_block;
   LLVMBasicBlockRef new_block;

   /* get current basic block */
   current_block = LLVMGetInsertBlock(builder);

   /* check if there's another block after this one */
   next_block = LLVMGetNextBasicBlock(current_block);
   if (next_block) {
      /* insert the new block before the next block */
      new_block = LLVMInsertBasicBlock(next_block, name);
   }
   else {
      /* append new block after current block */
      LLVMValueRef function = LLVMGetBasicBlockParent(current_block);
      new_block = LLVMAppendBasicBlock(function, name);
   }

   return new_block;
}


static LLVMBasicBlockRef
lp_build_flow_insert_block(struct lp_build_flow_context *flow)
{
   return lp_build_insert_new_block(flow->builder, "");
}


/**
 * Begin a "skip" block.  Inside this block we can test a condition and
 * skip to the end of the block if the condition is false.
 */
void
lp_build_flow_skip_begin(struct lp_build_flow_context *flow)
{
   struct lp_build_flow_skip *skip;
   LLVMBuilderRef builder;
   unsigned i;

   skip = &lp_build_flow_push(flow, LP_BUILD_FLOW_SKIP)->skip;
   if(!skip)
      return;

   /* create new basic block */
   skip->block = lp_build_flow_insert_block(flow);

   skip->num_variables = flow->num_variables;
   if(!skip->num_variables) {
      skip->phi = NULL;
      return;
   }

   /* Allocate a Phi node for each variable in this skip scope */
   skip->phi = MALLOC(skip->num_variables * sizeof *skip->phi);
   if(!skip->phi) {
      skip->num_variables = 0;
      return;
   }

   builder = LLVMCreateBuilder();
   LLVMPositionBuilderAtEnd(builder, skip->block);

   /* create a Phi node for each variable */
   for(i = 0; i < skip->num_variables; ++i)
      skip->phi[i] = LLVMBuildPhi(builder, LLVMTypeOf(*flow->variables[i]), "");

   LLVMDisposeBuilder(builder);
}


/**
 * Insert code to test a condition and branch to the end of the current
 * skip block if the condition is true.
 */
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

   /* for each variable, update the Phi node with a (variable, block) pair */
   for(i = 0; i < skip->num_variables; ++i) {
      assert(*flow->variables[i]);
      LLVMAddIncoming(skip->phi[i], flow->variables[i], &current_block, 1);
   }

   /* if cond is true, goto skip->block, else goto new_block */
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

   /* add (variable, block) tuples to the phi nodes */
   for(i = 0; i < skip->num_variables; ++i) {
      assert(*flow->variables[i]);
      LLVMAddIncoming(skip->phi[i], flow->variables[i], &current_block, 1);
      *flow->variables[i] = skip->phi[i];
   }

   /* goto block */
   LLVMBuildBr(flow->builder, skip->block);
   LLVMPositionBuilderAtEnd(flow->builder, skip->block);

   FREE(skip->phi);
}


/**
 * Check if the mask predicate is zero.  If so, jump to the end of the block.
 */
static void
lp_build_mask_check(struct lp_build_mask_context *mask)
{
   LLVMBuilderRef builder = mask->flow->builder;
   LLVMValueRef cond;

   /* cond = (mask == 0) */
   cond = LLVMBuildICmp(builder,
                        LLVMIntEQ,
                        LLVMBuildBitCast(builder, mask->value, mask->reg_type, ""),
                        LLVMConstNull(mask->reg_type),
                        "");

   /* if cond, goto end of block */
   lp_build_flow_skip_cond_break(mask->flow, cond);
}


/**
 * Begin a section of code which is predicated on a mask.
 * \param mask  the mask context, initialized here
 * \param flow  the flow context
 * \param type  the type of the mask
 * \param value  storage for the mask
 */
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


/**
 * Update boolean mask with given value (bitwise AND).
 * Typically used to update the quad's pixel alive/killed mask
 * after depth testing, alpha testing, TGSI_OPCODE_KIL, etc.
 */
void
lp_build_mask_update(struct lp_build_mask_context *mask,
                     LLVMValueRef value)
{
   mask->value = LLVMBuildAnd( mask->flow->builder, mask->value, value, "");

   lp_build_mask_check(mask);
}


/**
 * End section of code which is predicated on a mask.
 */
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



/*
  Example of if/then/else building:

     int x;
     if (cond) {
        x = 1 + 2;
     }
     else {
        x = 2 + 3;
     }

  Is built with:

     LLVMValueRef x = LLVMGetUndef();  // or something else

     flow = lp_build_flow_create(builder);

        lp_build_flow_scope_begin(flow);

           // x needs a phi node
           lp_build_flow_scope_declare(flow, &x);

           lp_build_if(ctx, flow, builder, cond);
              x = LLVMAdd(1, 2);
           lp_build_else(ctx);
              x = LLVMAdd(2, 3);
           lp_build_endif(ctx);

        lp_build_flow_scope_end(flow);

     lp_build_flow_destroy(flow);
 */



/**
 * Begin an if/else/endif construct.
 */
void
lp_build_if(struct lp_build_if_state *ctx,
            struct lp_build_flow_context *flow,
            LLVMBuilderRef builder,
            LLVMValueRef condition)
{
   LLVMBasicBlockRef block = LLVMGetInsertBlock(builder);
   struct lp_build_flow_if *ifthen;
   unsigned i;

   memset(ctx, 0, sizeof(*ctx));
   ctx->builder = builder;
   ctx->flow = flow;

   /* push/create new scope */
   ifthen = &lp_build_flow_push(flow, LP_BUILD_FLOW_IF)->ifthen;
   assert(ifthen);

   ifthen->num_variables = flow->num_variables;
   ifthen->condition = condition;
   ifthen->entry_block = block;

   /* create a Phi node for each variable in this flow scope */
   ifthen->phi = MALLOC(ifthen->num_variables * sizeof(*ifthen->phi));
   if (!ifthen->phi) {
      ifthen->num_variables = 0;
      return;
   }

   /* create endif/merge basic block for the phi functions */
   ifthen->merge_block = lp_build_insert_new_block(builder, "endif-block");
   LLVMPositionBuilderAtEnd(builder, ifthen->merge_block);

   /* create a phi node for each variable */
   for (i = 0; i < flow->num_variables; i++) {
      ifthen->phi[i] = LLVMBuildPhi(builder, LLVMTypeOf(*flow->variables[i]), "");

      /* add add the initial value of the var from the entry block */
      LLVMAddIncoming(ifthen->phi[i], flow->variables[i], &ifthen->entry_block, 1);
   }

   /* create/insert true_block before merge_block */
   ifthen->true_block = LLVMInsertBasicBlock(ifthen->merge_block, "if-true-block");

   /* successive code goes into the true block */
   LLVMPositionBuilderAtEnd(builder, ifthen->true_block);
}


/**
 * Begin else-part of a conditional
 */
void
lp_build_else(struct lp_build_if_state *ctx)
{
   struct lp_build_flow_context *flow = ctx->flow;
   struct lp_build_flow_if *ifthen;
   unsigned i;

   ifthen = &lp_build_flow_peek(flow, LP_BUILD_FLOW_IF)->ifthen;
   assert(ifthen);

   /* for each variable, update the Phi node with a (variable, block) pair */
   LLVMPositionBuilderAtEnd(ctx->builder, ifthen->merge_block);
   for (i = 0; i < flow->num_variables; i++) {
      assert(*flow->variables[i]);
      LLVMAddIncoming(ifthen->phi[i], flow->variables[i], &ifthen->true_block, 1);
   }

   /* create/insert false_block before the merge block */
   ifthen->false_block = LLVMInsertBasicBlock(ifthen->merge_block, "if-false-block");

   /* successive code goes into the else block */
   LLVMPositionBuilderAtEnd(ctx->builder, ifthen->false_block);
}


/**
 * End a conditional.
 */
void
lp_build_endif(struct lp_build_if_state *ctx)
{
   struct lp_build_flow_context *flow = ctx->flow;
   struct lp_build_flow_if *ifthen;
   unsigned i;

   ifthen = &lp_build_flow_pop(flow, LP_BUILD_FLOW_IF)->ifthen;
   assert(ifthen);

   if (ifthen->false_block) {
      LLVMPositionBuilderAtEnd(ctx->builder, ifthen->merge_block);
      /* for each variable, update the Phi node with a (variable, block) pair */
      for (i = 0; i < flow->num_variables; i++) {
         assert(*flow->variables[i]);
         LLVMAddIncoming(ifthen->phi[i], flow->variables[i], &ifthen->false_block, 1);

         /* replace the variable ref with the phi function */
         *flow->variables[i] = ifthen->phi[i];
      }
   }
   else {
      /* no else clause */
      LLVMPositionBuilderAtEnd(ctx->builder, ifthen->merge_block);
      for (i = 0; i < flow->num_variables; i++) {
         assert(*flow->variables[i]);
         LLVMAddIncoming(ifthen->phi[i], flow->variables[i], &ifthen->true_block, 1);

         /* replace the variable ref with the phi function */
         *flow->variables[i] = ifthen->phi[i];
      }
   }

   FREE(ifthen->phi);

   /***
    *** Now patch in the various branch instructions.
    ***/

   /* Insert the conditional branch instruction at the end of entry_block */
   LLVMPositionBuilderAtEnd(ctx->builder, ifthen->entry_block);
   if (ifthen->false_block) {
      /* we have an else clause */
      LLVMBuildCondBr(ctx->builder, ifthen->condition,
                      ifthen->true_block, ifthen->false_block);
   }
   else {
      /* no else clause */
      LLVMBuildCondBr(ctx->builder, ifthen->condition,
                      ifthen->true_block, ifthen->merge_block);
   }

   /* Append an unconditional Br(anch) instruction on the true_block */
   LLVMPositionBuilderAtEnd(ctx->builder, ifthen->true_block);
   LLVMBuildBr(ctx->builder, ifthen->merge_block);
   if (ifthen->false_block) {
      /* Append an unconditional Br(anch) instruction on the false_block */
      LLVMPositionBuilderAtEnd(ctx->builder, ifthen->false_block);
      LLVMBuildBr(ctx->builder, ifthen->merge_block);
   }


   /* Resume building code at end of the ifthen->merge_block */
   LLVMPositionBuilderAtEnd(ctx->builder, ifthen->merge_block);
}
