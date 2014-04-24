/**************************************************************************
 * 
 * Copyright 2009 VMware, Inc.
 * Copyright 2007-2008 VMware, Inc.
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
 * TGSI to LLVM IR translation -- SoA.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 *
 * Based on tgsi_sse2.c code written by Michal Krol, Keith Whitwell,
 * Brian Paul, and others.
 */

#include "pipe/p_config.h"
#include "pipe/p_shader_tokens.h"
#include "util/u_debug.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_exec.h"
#include "tgsi/tgsi_info.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"
#include "tgsi/tgsi_scan.h"
#include "tgsi/tgsi_strings.h"
#include "lp_bld_tgsi_action.h"
#include "lp_bld_type.h"
#include "lp_bld_const.h"
#include "lp_bld_arit.h"
#include "lp_bld_bitarit.h"
#include "lp_bld_gather.h"
#include "lp_bld_init.h"
#include "lp_bld_logic.h"
#include "lp_bld_swizzle.h"
#include "lp_bld_flow.h"
#include "lp_bld_quad.h"
#include "lp_bld_tgsi.h"
#include "lp_bld_limits.h"
#include "lp_bld_debug.h"
#include "lp_bld_printf.h"
#include "lp_bld_sample.h"
#include "lp_bld_struct.h"

/* SM 4.0 says that subroutines can nest 32 deep and 
 * we need one more for our main function */
#define LP_MAX_NUM_FUNCS 33

#define DUMP_GS_EMITS 0

/*
 * If non-zero, the generated LLVM IR will print intermediate results on every TGSI
 * instruction.
 *
 * TODO:
 * - take execution masks in consideration
 * - debug control-flow instructions
 */
#define DEBUG_EXECUTION 0


/*
 * Emit code to print a register value.
 */
static void
emit_dump_reg(struct gallivm_state *gallivm,
              unsigned file,
              unsigned index,
              unsigned chan,
              LLVMValueRef value)
{
   char buf[32];

   util_snprintf(buf, sizeof buf, "    %s[%u].%c = ",
                 tgsi_file_name(file),
                 index, "xyzw"[chan]);

   lp_build_print_value(gallivm, buf, value);
}

/*
 * Return the context for the current function.
 * (always 'main', if shader doesn't do any function calls)
 */
static INLINE struct function_ctx *
func_ctx(struct lp_exec_mask *mask)
{
   assert(mask->function_stack_size > 0);
   assert(mask->function_stack_size <= LP_MAX_NUM_FUNCS);
   return &mask->function_stack[mask->function_stack_size - 1];
}

/*
 * Returns true if we're in a loop.
 * It's global, meaning that it returns true even if there's
 * no loop inside the current function, but we were inside
 * a loop inside another function, from which this one was called.
 */
static INLINE boolean
mask_has_loop(struct lp_exec_mask *mask)
{
   int i;
   for (i = mask->function_stack_size - 1; i >= 0; --i) {
      const struct function_ctx *ctx = &mask->function_stack[i];
      if (ctx->loop_stack_size > 0)
         return TRUE;
   }
   return FALSE;
}

/*
 * Returns true if we're inside a switch statement.
 * It's global, meaning that it returns true even if there's
 * no switch in the current function, but we were inside
 * a switch inside another function, from which this one was called.
 */
static INLINE boolean
mask_has_switch(struct lp_exec_mask *mask)
{
   int i;
   for (i = mask->function_stack_size - 1; i >= 0; --i) {
      const struct function_ctx *ctx = &mask->function_stack[i];
      if (ctx->switch_stack_size > 0)
         return TRUE;
   }
   return FALSE;
}

/*
 * Returns true if we're inside a conditional.
 * It's global, meaning that it returns true even if there's
 * no conditional in the current function, but we were inside
 * a conditional inside another function, from which this one was called.
 */
static INLINE boolean
mask_has_cond(struct lp_exec_mask *mask)
{
   int i;
   for (i = mask->function_stack_size - 1; i >= 0; --i) {
      const struct function_ctx *ctx = &mask->function_stack[i];
      if (ctx->cond_stack_size > 0)
         return TRUE;
   }
   return FALSE;
}


/*
 * Initialize a function context at the specified index.
 */
static void
lp_exec_mask_function_init(struct lp_exec_mask *mask, int function_idx)
{
   LLVMTypeRef int_type = LLVMInt32TypeInContext(mask->bld->gallivm->context);
   LLVMBuilderRef builder = mask->bld->gallivm->builder;
   struct function_ctx *ctx =  &mask->function_stack[function_idx];

   ctx->cond_stack_size = 0;
   ctx->loop_stack_size = 0;
   ctx->switch_stack_size = 0;

   if (function_idx == 0) {
      ctx->ret_mask = mask->ret_mask;
   }

   ctx->loop_limiter = lp_build_alloca(mask->bld->gallivm,
                                       int_type, "looplimiter");
   LLVMBuildStore(
      builder,
      LLVMConstInt(int_type, LP_MAX_TGSI_LOOP_ITERATIONS, false),
      ctx->loop_limiter);
}

static void lp_exec_mask_init(struct lp_exec_mask *mask, struct lp_build_context *bld)
{
   mask->bld = bld;
   mask->has_mask = FALSE;
   mask->ret_in_main = FALSE;
   /* For the main function */
   mask->function_stack_size = 1;

   mask->int_vec_type = lp_build_int_vec_type(bld->gallivm, mask->bld->type);
   mask->exec_mask = mask->ret_mask = mask->break_mask = mask->cont_mask =
         mask->cond_mask = mask->switch_mask =
         LLVMConstAllOnes(mask->int_vec_type);

   mask->function_stack = CALLOC(LP_MAX_NUM_FUNCS,
                                 sizeof(mask->function_stack[0]));
   lp_exec_mask_function_init(mask, 0);
}

static void
lp_exec_mask_fini(struct lp_exec_mask *mask)
{
   FREE(mask->function_stack);
}

static void lp_exec_mask_update(struct lp_exec_mask *mask)
{
   LLVMBuilderRef builder = mask->bld->gallivm->builder;
   boolean has_loop_mask = mask_has_loop(mask);
   boolean has_cond_mask = mask_has_cond(mask);
   boolean has_switch_mask = mask_has_switch(mask);
   boolean has_ret_mask = mask->function_stack_size > 1 ||
         mask->ret_in_main;

   if (has_loop_mask) {
      /*for loops we need to update the entire mask at runtime */
      LLVMValueRef tmp;
      assert(mask->break_mask);
      tmp = LLVMBuildAnd(builder,
                         mask->cont_mask,
                         mask->break_mask,
                         "maskcb");
      mask->exec_mask = LLVMBuildAnd(builder,
                                     mask->cond_mask,
                                     tmp,
                                     "maskfull");
   } else
      mask->exec_mask = mask->cond_mask;

   if (has_switch_mask) {
      mask->exec_mask = LLVMBuildAnd(builder,
                                     mask->exec_mask,
                                     mask->switch_mask,
                                     "switchmask");
   }

   if (has_ret_mask) {
      mask->exec_mask = LLVMBuildAnd(builder,
                                     mask->exec_mask,
                                     mask->ret_mask,
                                     "callmask");
   }

   mask->has_mask = (has_cond_mask ||
                     has_loop_mask ||
                     has_switch_mask ||
                     has_ret_mask);
}

static void lp_exec_mask_cond_push(struct lp_exec_mask *mask,
                                   LLVMValueRef val)
{
   LLVMBuilderRef builder = mask->bld->gallivm->builder;
   struct function_ctx *ctx = func_ctx(mask);

   if (ctx->cond_stack_size >= LP_MAX_TGSI_NESTING) {
      ctx->cond_stack_size++;
      return;
   }
   if (ctx->cond_stack_size == 0 && mask->function_stack_size == 1) {
      assert(mask->cond_mask == LLVMConstAllOnes(mask->int_vec_type));
   }
   ctx->cond_stack[ctx->cond_stack_size++] = mask->cond_mask;
   assert(LLVMTypeOf(val) == mask->int_vec_type);
   mask->cond_mask = LLVMBuildAnd(builder,
                                  mask->cond_mask,
                                  val,
                                  "");
   lp_exec_mask_update(mask);
}

static void lp_exec_mask_cond_invert(struct lp_exec_mask *mask)
{
   LLVMBuilderRef builder = mask->bld->gallivm->builder;
   struct function_ctx *ctx = func_ctx(mask);
   LLVMValueRef prev_mask;
   LLVMValueRef inv_mask;

   assert(ctx->cond_stack_size);
   if (ctx->cond_stack_size >= LP_MAX_TGSI_NESTING)
      return;
   prev_mask = ctx->cond_stack[ctx->cond_stack_size - 1];
   if (ctx->cond_stack_size == 1 && mask->function_stack_size == 1) {
      assert(prev_mask == LLVMConstAllOnes(mask->int_vec_type));
   }

   inv_mask = LLVMBuildNot(builder, mask->cond_mask, "");

   mask->cond_mask = LLVMBuildAnd(builder,
                                  inv_mask,
                                  prev_mask, "");
   lp_exec_mask_update(mask);
}

static void lp_exec_mask_cond_pop(struct lp_exec_mask *mask)
{
   struct function_ctx *ctx = func_ctx(mask);
   assert(ctx->cond_stack_size);
   --ctx->cond_stack_size;
   if (ctx->cond_stack_size >= LP_MAX_TGSI_NESTING)
      return;
   mask->cond_mask = ctx->cond_stack[ctx->cond_stack_size];
   lp_exec_mask_update(mask);
}

static void lp_exec_bgnloop(struct lp_exec_mask *mask)
{
   LLVMBuilderRef builder = mask->bld->gallivm->builder;
   struct function_ctx *ctx = func_ctx(mask);

   if (ctx->loop_stack_size >= LP_MAX_TGSI_NESTING) {
      ++ctx->loop_stack_size;
      return;
   }

   ctx->break_type_stack[ctx->loop_stack_size + ctx->switch_stack_size] =
      ctx->break_type;
   ctx->break_type = LP_EXEC_MASK_BREAK_TYPE_LOOP;

   ctx->loop_stack[ctx->loop_stack_size].loop_block = ctx->loop_block;
   ctx->loop_stack[ctx->loop_stack_size].cont_mask = mask->cont_mask;
   ctx->loop_stack[ctx->loop_stack_size].break_mask = mask->break_mask;
   ctx->loop_stack[ctx->loop_stack_size].break_var = ctx->break_var;
   ++ctx->loop_stack_size;

   ctx->break_var = lp_build_alloca(mask->bld->gallivm, mask->int_vec_type, "");
   LLVMBuildStore(builder, mask->break_mask, ctx->break_var);

   ctx->loop_block = lp_build_insert_new_block(mask->bld->gallivm, "bgnloop");

   LLVMBuildBr(builder, ctx->loop_block);
   LLVMPositionBuilderAtEnd(builder, ctx->loop_block);

   mask->break_mask = LLVMBuildLoad(builder, ctx->break_var, "");

   lp_exec_mask_update(mask);
}

static void lp_exec_break(struct lp_exec_mask *mask,
                          struct lp_build_tgsi_context * bld_base)
{
   LLVMBuilderRef builder = mask->bld->gallivm->builder;
   struct function_ctx *ctx = func_ctx(mask);

   if (ctx->break_type == LP_EXEC_MASK_BREAK_TYPE_LOOP) {
      LLVMValueRef exec_mask = LLVMBuildNot(builder,
                                            mask->exec_mask,
                                            "break");

      mask->break_mask = LLVMBuildAnd(builder,
                                      mask->break_mask,
                                      exec_mask, "break_full");
   }
   else {
      unsigned opcode = bld_base->instructions[bld_base->pc + 1].Instruction.Opcode;
      boolean break_always = (opcode == TGSI_OPCODE_ENDSWITCH ||
                              opcode == TGSI_OPCODE_CASE);


      if (ctx->switch_in_default) {
         /*
          * stop default execution but only if this is an unconditional switch.
          * (The condition here is not perfect since dead code after break is
          * allowed but should be sufficient since false negatives are just
          * unoptimized - so we don't have to pre-evaluate that).
          */
         if(break_always && ctx->switch_pc) {
            bld_base->pc = ctx->switch_pc;
            return;
         }
      }

      if (break_always) {
         mask->switch_mask = LLVMConstNull(mask->bld->int_vec_type);
      }
      else {
         LLVMValueRef exec_mask = LLVMBuildNot(builder,
                                               mask->exec_mask,
                                               "break");
         mask->switch_mask = LLVMBuildAnd(builder,
                                          mask->switch_mask,
                                          exec_mask, "break_switch");
      }
   }

   lp_exec_mask_update(mask);
}

static void lp_exec_break_condition(struct lp_exec_mask *mask,
                                    LLVMValueRef cond)
{
   LLVMBuilderRef builder = mask->bld->gallivm->builder;
   struct function_ctx *ctx = func_ctx(mask);
   LLVMValueRef cond_mask = LLVMBuildAnd(builder,
                                         mask->exec_mask,
                                         cond, "cond_mask");
   cond_mask = LLVMBuildNot(builder, cond_mask, "break_cond");

   if (ctx->break_type == LP_EXEC_MASK_BREAK_TYPE_LOOP) {
      mask->break_mask = LLVMBuildAnd(builder,
                                      mask->break_mask,
                                      cond_mask, "breakc_full");
   }
   else {
      mask->switch_mask = LLVMBuildAnd(builder,
                                       mask->switch_mask,
                                       cond_mask, "breakc_switch");
   }

   lp_exec_mask_update(mask);
}

static void lp_exec_continue(struct lp_exec_mask *mask)
{
   LLVMBuilderRef builder = mask->bld->gallivm->builder;
   LLVMValueRef exec_mask = LLVMBuildNot(builder,
                                         mask->exec_mask,
                                         "");

   mask->cont_mask = LLVMBuildAnd(builder,
                                  mask->cont_mask,
                                  exec_mask, "");

   lp_exec_mask_update(mask);
}


static void lp_exec_endloop(struct gallivm_state *gallivm,
                            struct lp_exec_mask *mask)
{
   LLVMBuilderRef builder = mask->bld->gallivm->builder;
   struct function_ctx *ctx = func_ctx(mask);
   LLVMBasicBlockRef endloop;
   LLVMTypeRef int_type = LLVMInt32TypeInContext(mask->bld->gallivm->context);
   LLVMTypeRef reg_type = LLVMIntTypeInContext(gallivm->context,
                                               mask->bld->type.width *
                                               mask->bld->type.length);
   LLVMValueRef i1cond, i2cond, icond, limiter;

   assert(mask->break_mask);

   
   assert(ctx->loop_stack_size);
   if (ctx->loop_stack_size > LP_MAX_TGSI_NESTING) {
      --ctx->loop_stack_size;
      return;
   }

   /*
    * Restore the cont_mask, but don't pop
    */
   mask->cont_mask = ctx->loop_stack[ctx->loop_stack_size - 1].cont_mask;
   lp_exec_mask_update(mask);

   /*
    * Unlike the continue mask, the break_mask must be preserved across loop
    * iterations
    */
   LLVMBuildStore(builder, mask->break_mask, ctx->break_var);

   /* Decrement the loop limiter */
   limiter = LLVMBuildLoad(builder, ctx->loop_limiter, "");

   limiter = LLVMBuildSub(
      builder,
      limiter,
      LLVMConstInt(int_type, 1, false),
      "");

   LLVMBuildStore(builder, limiter, ctx->loop_limiter);

   /* i1cond = (mask != 0) */
   i1cond = LLVMBuildICmp(
      builder,
      LLVMIntNE,
      LLVMBuildBitCast(builder, mask->exec_mask, reg_type, ""),
      LLVMConstNull(reg_type), "i1cond");

   /* i2cond = (looplimiter > 0) */
   i2cond = LLVMBuildICmp(
      builder,
      LLVMIntSGT,
      limiter,
      LLVMConstNull(int_type), "i2cond");

   /* if( i1cond && i2cond ) */
   icond = LLVMBuildAnd(builder, i1cond, i2cond, "");

   endloop = lp_build_insert_new_block(mask->bld->gallivm, "endloop");

   LLVMBuildCondBr(builder,
                   icond, ctx->loop_block, endloop);

   LLVMPositionBuilderAtEnd(builder, endloop);

   assert(ctx->loop_stack_size);
   --ctx->loop_stack_size;
   mask->cont_mask = ctx->loop_stack[ctx->loop_stack_size].cont_mask;
   mask->break_mask = ctx->loop_stack[ctx->loop_stack_size].break_mask;
   ctx->loop_block = ctx->loop_stack[ctx->loop_stack_size].loop_block;
   ctx->break_var = ctx->loop_stack[ctx->loop_stack_size].break_var;
   ctx->break_type = ctx->break_type_stack[ctx->loop_stack_size +
         ctx->switch_stack_size];

   lp_exec_mask_update(mask);
}

static void lp_exec_switch(struct lp_exec_mask *mask,
                           LLVMValueRef switchval)
{
   struct function_ctx *ctx = func_ctx(mask);

   if (ctx->switch_stack_size >= LP_MAX_TGSI_NESTING ||
       ctx->loop_stack_size > LP_MAX_TGSI_NESTING) {
      ctx->switch_stack_size++;
      return;
   }

   ctx->break_type_stack[ctx->loop_stack_size + ctx->switch_stack_size] =
      ctx->break_type;
   ctx->break_type = LP_EXEC_MASK_BREAK_TYPE_SWITCH;

   ctx->switch_stack[ctx->switch_stack_size].switch_mask = mask->switch_mask;
   ctx->switch_stack[ctx->switch_stack_size].switch_val = ctx->switch_val;
   ctx->switch_stack[ctx->switch_stack_size].switch_mask_default = ctx->switch_mask_default;
   ctx->switch_stack[ctx->switch_stack_size].switch_in_default = ctx->switch_in_default;
   ctx->switch_stack[ctx->switch_stack_size].switch_pc = ctx->switch_pc;
   ctx->switch_stack_size++;

   mask->switch_mask = LLVMConstNull(mask->int_vec_type);
   ctx->switch_val = switchval;
   ctx->switch_mask_default = LLVMConstNull(mask->int_vec_type);
   ctx->switch_in_default = false;
   ctx->switch_pc = 0;

   lp_exec_mask_update(mask);
}

static void lp_exec_endswitch(struct lp_exec_mask *mask,
                              struct lp_build_tgsi_context * bld_base)
{
   LLVMBuilderRef builder = mask->bld->gallivm->builder;
   struct function_ctx *ctx = func_ctx(mask);

   if (ctx->switch_stack_size > LP_MAX_TGSI_NESTING) {
      ctx->switch_stack_size--;
      return;
   }

   /* check if there's deferred default if so do it now */
   if (ctx->switch_pc && !ctx->switch_in_default) {
      LLVMValueRef prevmask, defaultmask;
      unsigned tmp_pc;
      prevmask = ctx->switch_stack[ctx->switch_stack_size - 1].switch_mask;
      defaultmask = LLVMBuildNot(builder, ctx->switch_mask_default, "sw_default_mask");
      mask->switch_mask = LLVMBuildAnd(builder, prevmask, defaultmask, "sw_mask");
      ctx->switch_in_default = true;

      lp_exec_mask_update(mask);

      assert(bld_base->instructions[ctx->switch_pc - 1].Instruction.Opcode ==
             TGSI_OPCODE_DEFAULT);

      tmp_pc = bld_base->pc;
      bld_base->pc = ctx->switch_pc;
      /*
       * re-purpose switch_pc to point to here again, since we stop execution of
       * the deferred default after next break.
       */
      ctx->switch_pc = tmp_pc - 1;

      return;
   }

   else if (ctx->switch_pc && ctx->switch_in_default) {
      assert(bld_base->pc == ctx->switch_pc + 1);
   }

   ctx->switch_stack_size--;
   mask->switch_mask = ctx->switch_stack[ctx->switch_stack_size].switch_mask;
   ctx->switch_val = ctx->switch_stack[ctx->switch_stack_size].switch_val;
   ctx->switch_mask_default = ctx->switch_stack[ctx->switch_stack_size].switch_mask_default;
   ctx->switch_in_default = ctx->switch_stack[ctx->switch_stack_size].switch_in_default;
   ctx->switch_pc = ctx->switch_stack[ctx->switch_stack_size].switch_pc;

   ctx->break_type = ctx->break_type_stack[ctx->loop_stack_size + ctx->switch_stack_size];

   lp_exec_mask_update(mask);
}

static void lp_exec_case(struct lp_exec_mask *mask,
                         LLVMValueRef caseval)
{
   LLVMBuilderRef builder = mask->bld->gallivm->builder;
   struct function_ctx *ctx = func_ctx(mask);

   LLVMValueRef casemask, prevmask;

   if (ctx->switch_stack_size > LP_MAX_TGSI_NESTING) {
      return;
   }

   /* skipping case mask evaluation here is NOT optional (not in all cases anyway). */
   if (!ctx->switch_in_default) {
      prevmask = ctx->switch_stack[ctx->switch_stack_size - 1].switch_mask;
      casemask = lp_build_cmp(mask->bld, PIPE_FUNC_EQUAL, caseval, ctx->switch_val);
      ctx->switch_mask_default = LLVMBuildOr(builder, casemask,
                                             ctx->switch_mask_default, "sw_default_mask");
      casemask = LLVMBuildOr(builder, casemask, mask->switch_mask, "");
      mask->switch_mask = LLVMBuildAnd(builder, casemask, prevmask, "sw_mask");

      lp_exec_mask_update(mask);
   }
}

/*
 * Analyse default statement in a switch.
 * \return true if default is last statement, false otherwise
 * \param default_pc_start contains pc of instruction to jump to
 *                         if default wasn't last but there's no
 *                         fallthrough into default.
 */
static boolean default_analyse_is_last(struct lp_exec_mask *mask,
                                       struct lp_build_tgsi_context * bld_base,
                                       int *default_pc_start)
{
   unsigned pc = bld_base->pc;
   struct function_ctx *ctx = func_ctx(mask);
   unsigned curr_switch_stack = ctx->switch_stack_size;

   if (ctx->switch_stack_size > LP_MAX_TGSI_NESTING) {
      return false;
   }

   /* skip over case statements which are together with default */
   while (bld_base->instructions[pc].Instruction.Opcode == TGSI_OPCODE_CASE) {
      pc++;
   }

   while (pc != -1 && pc < bld_base->num_instructions) {
      unsigned opcode = bld_base->instructions[pc].Instruction.Opcode;
      switch (opcode) {
      case TGSI_OPCODE_CASE:
         if (curr_switch_stack == ctx->switch_stack_size) {
            *default_pc_start = pc - 1;
            return false;
         }
         break;
      case TGSI_OPCODE_SWITCH:
         curr_switch_stack++;
         break;
      case TGSI_OPCODE_ENDSWITCH:
         if (curr_switch_stack == ctx->switch_stack_size) {
            *default_pc_start = pc - 1;
            return true;
         }
         curr_switch_stack--;
         break;
      }
      pc++;
   }
   /* should never arrive here */
   assert(0);
   return true;
}

static void lp_exec_default(struct lp_exec_mask *mask,
                            struct lp_build_tgsi_context * bld_base)
{
   LLVMBuilderRef builder = mask->bld->gallivm->builder;
   struct function_ctx *ctx = func_ctx(mask);

   int default_exec_pc;
   boolean default_is_last;

   if (ctx->switch_stack_size > LP_MAX_TGSI_NESTING) {
      return;
   }

   /*
    * This is a messy opcode, because it may not be always at the end and
    * there can be fallthrough in and out of it.
    */

   default_is_last = default_analyse_is_last(mask, bld_base, &default_exec_pc);
   /*
    * If it is last statement in switch (note that case statements appearing
    * "at the same time" as default don't change that) everything is just fine,
    * update switch mask and go on. This means we can handle default with
    * fallthrough INTO it without overhead, if it is last.
    */
   if (default_is_last) {
      LLVMValueRef prevmask, defaultmask;
      prevmask = ctx->switch_stack[ctx->switch_stack_size - 1].switch_mask;
      defaultmask = LLVMBuildNot(builder, ctx->switch_mask_default, "sw_default_mask");
      defaultmask = LLVMBuildOr(builder, defaultmask, mask->switch_mask, "");
      mask->switch_mask = LLVMBuildAnd(builder, prevmask, defaultmask, "sw_mask");
      ctx->switch_in_default = true;

      lp_exec_mask_update(mask);
   }
   else {
      /*
       * Technically, "case" immediately before default isn't really a
       * fallthrough, however we still have to count them as such as we
       * already have updated the masks.
       * If that happens in practice could add a switch optimizer pass
       * which just gets rid of all case statements appearing together with
       * default (or could do switch analysis at switch start time instead).
       */
      unsigned opcode = bld_base->instructions[bld_base->pc - 1].Instruction.Opcode;
      boolean ft_into = (opcode != TGSI_OPCODE_BRK &&
                         opcode != TGSI_OPCODE_SWITCH);
      /*
       * If it is not last statement and there was no fallthrough into it,
       * we record the PC and continue execution at next case (again, those
       * case encountered at the same time don't count). At endswitch
       * time, we update switchmask, and go back executing the code we skipped
       * until the next break (possibly re-executing some code with changed mask
       * if there was a fallthrough out of default).
       * Finally, if it is not last statement and there was a fallthrough into it,
       * do the same as with the former case, except instead of skipping the code
       * just execute it without updating the mask, then go back and re-execute.
       */
      ctx->switch_pc = bld_base->pc;
      if (!ft_into) {
         bld_base->pc = default_exec_pc;
      }
   }
}


/* stores val into an address pointed to by dst_ptr.
 * mask->exec_mask is used to figure out which bits of val
 * should be stored into the address
 * (0 means don't store this bit, 1 means do store).
 */
static void lp_exec_mask_store(struct lp_exec_mask *mask,
                               struct lp_build_context *bld_store,
                               LLVMValueRef pred,
                               LLVMValueRef val,
                               LLVMValueRef dst_ptr)
{
   LLVMBuilderRef builder = mask->bld->gallivm->builder;

   assert(lp_check_value(bld_store->type, val));
   assert(LLVMGetTypeKind(LLVMTypeOf(dst_ptr)) == LLVMPointerTypeKind);
   assert(LLVMGetElementType(LLVMTypeOf(dst_ptr)) == LLVMTypeOf(val));

   /* Mix the predicate and execution mask */
   if (mask->has_mask) {
      if (pred) {
         pred = LLVMBuildAnd(builder, pred, mask->exec_mask, "");
      } else {
         pred = mask->exec_mask;
      }
   }

   if (pred) {
      LLVMValueRef res, dst;

      dst = LLVMBuildLoad(builder, dst_ptr, "");
      res = lp_build_select(bld_store, pred, val, dst);
      LLVMBuildStore(builder, res, dst_ptr);
   } else
      LLVMBuildStore(builder, val, dst_ptr);
}

static void lp_exec_mask_call(struct lp_exec_mask *mask,
                              int func,
                              int *pc)
{
   if (mask->function_stack_size >= LP_MAX_NUM_FUNCS) {
      return;
   }

   lp_exec_mask_function_init(mask, mask->function_stack_size);
   mask->function_stack[mask->function_stack_size].pc = *pc;
   mask->function_stack[mask->function_stack_size].ret_mask = mask->ret_mask;
   mask->function_stack_size++;
   *pc = func;
}

static void lp_exec_mask_ret(struct lp_exec_mask *mask, int *pc)
{
   LLVMBuilderRef builder = mask->bld->gallivm->builder;
   struct function_ctx *ctx = func_ctx(mask);
   LLVMValueRef exec_mask;

   if (ctx->cond_stack_size == 0 &&
       ctx->loop_stack_size == 0 &&
       ctx->switch_stack_size == 0 &&
       mask->function_stack_size == 1) {
      /* returning from main() */
      *pc = -1;
      return;
   }

   if (mask->function_stack_size == 1) {
      /*
       * This requires special handling since we need to ensure
       * we don't drop the mask even if we have no call stack
       * (e.g. after a ret in a if clause after the endif)
       */
      mask->ret_in_main = TRUE;
   }

   exec_mask = LLVMBuildNot(builder,
                            mask->exec_mask,
                            "ret");

   mask->ret_mask = LLVMBuildAnd(builder,
                                 mask->ret_mask,
                                 exec_mask, "ret_full");

   lp_exec_mask_update(mask);
}

static void lp_exec_mask_bgnsub(struct lp_exec_mask *mask)
{
}

static void lp_exec_mask_endsub(struct lp_exec_mask *mask, int *pc)
{
   struct function_ctx *ctx;

   assert(mask->function_stack_size > 1);
   assert(mask->function_stack_size <= LP_MAX_NUM_FUNCS);

   ctx = func_ctx(mask);
   mask->function_stack_size--;

   *pc = ctx->pc;
   mask->ret_mask = ctx->ret_mask;

   lp_exec_mask_update(mask);
}


static LLVMValueRef
get_file_ptr(struct lp_build_tgsi_soa_context *bld,
             unsigned file,
             unsigned index,
             unsigned chan)
{
   LLVMBuilderRef builder = bld->bld_base.base.gallivm->builder;
   LLVMValueRef (*array_of_vars)[TGSI_NUM_CHANNELS];
   LLVMValueRef var_of_array;

   switch (file) {
   case TGSI_FILE_TEMPORARY:
      array_of_vars = bld->temps;
      var_of_array = bld->temps_array;
      break;
   case TGSI_FILE_OUTPUT:
      array_of_vars = bld->outputs;
      var_of_array = bld->outputs_array;
      break;
   default:
      assert(0);
      return NULL;
   }

   assert(chan < 4);

   if (bld->indirect_files & (1 << file)) {
      LLVMValueRef lindex = lp_build_const_int32(bld->bld_base.base.gallivm, index * 4 + chan);
      return LLVMBuildGEP(builder, var_of_array, &lindex, 1, "");
   }
   else {
      assert(index <= bld->bld_base.info->file_max[file]);
      return array_of_vars[index][chan];
   }
}


/**
 * Return pointer to a temporary register channel (src or dest).
 * Note that indirect addressing cannot be handled here.
 * \param index  which temporary register
 * \param chan  which channel of the temp register.
 */
LLVMValueRef
lp_get_temp_ptr_soa(struct lp_build_tgsi_soa_context *bld,
             unsigned index,
             unsigned chan)
{
   return get_file_ptr(bld, TGSI_FILE_TEMPORARY, index, chan);
}

/**
 * Return pointer to a output register channel (src or dest).
 * Note that indirect addressing cannot be handled here.
 * \param index  which output register
 * \param chan  which channel of the output register.
 */
LLVMValueRef
lp_get_output_ptr(struct lp_build_tgsi_soa_context *bld,
               unsigned index,
               unsigned chan)
{
   return get_file_ptr(bld, TGSI_FILE_OUTPUT, index, chan);
}

/*
 * If we have indirect addressing in outputs copy our alloca array
 * to the outputs slots specified by the caller to make sure
 * our outputs are delivered consistently via the same interface.
 */
static void
gather_outputs(struct lp_build_tgsi_soa_context * bld)
{
   if ((bld->indirect_files & (1 << TGSI_FILE_OUTPUT))) {
      unsigned index, chan;
      assert(bld->bld_base.info->num_outputs <=
             bld->bld_base.info->file_max[TGSI_FILE_OUTPUT] + 1);
      for (index = 0; index < bld->bld_base.info->num_outputs; ++index) {
         for (chan = 0; chan < TGSI_NUM_CHANNELS; ++chan) {
            bld->outputs[index][chan] = lp_get_output_ptr(bld, index, chan);
         }
      }
   }
}

/**
 * Gather vector.
 * XXX the lp_build_gather() function should be capable of doing this
 * with a little work.
 */
static LLVMValueRef
build_gather(struct lp_build_context *bld,
             LLVMValueRef base_ptr,
             LLVMValueRef indexes,
             LLVMValueRef *overflow_mask)
{
   LLVMBuilderRef builder = bld->gallivm->builder;
   LLVMValueRef res = bld->undef;
   unsigned i;
   LLVMValueRef temp_ptr;

   if (overflow_mask) {
      temp_ptr = lp_build_alloca(
         bld->gallivm,
         lp_build_vec_type(bld->gallivm, bld->type), "");
   }

   /*
    * Loop over elements of index_vec, load scalar value, insert it into 'res'.
    */
   for (i = 0; i < bld->type.length; i++) {
      LLVMValueRef ii = lp_build_const_int32(bld->gallivm, i);
      LLVMValueRef index = LLVMBuildExtractElement(builder,
                                                   indexes, ii, "");
      LLVMValueRef scalar_ptr, scalar;
      LLVMValueRef overflow;
      struct lp_build_if_state if_ctx;

      /*
       * overflow_mask is a boolean vector telling us which channels
       * in the vector overflowed. We use the overflow behavior for
       * constant buffers which is defined as:
       * Out of bounds access to constant buffer returns 0 in all
       * componenets. Out of bounds behavior is always with respect
       * to the size of the buffer bound at that slot.
       */
      if (overflow_mask) {
         overflow = LLVMBuildExtractElement(builder, *overflow_mask,
                                            ii, "");
         lp_build_if(&if_ctx, bld->gallivm, overflow);
         {
            LLVMValueRef val = LLVMBuildLoad(builder, temp_ptr, "");
            val = LLVMBuildInsertElement(
               builder, val,
               LLVMConstNull(LLVMFloatTypeInContext(bld->gallivm->context)),
               ii, "");
            LLVMBuildStore(builder, val, temp_ptr);
         }
         lp_build_else(&if_ctx);
         {
            LLVMValueRef val = LLVMBuildLoad(builder, temp_ptr, "");

            scalar_ptr = LLVMBuildGEP(builder, base_ptr,
                                      &index, 1, "gather_ptr");
            scalar = LLVMBuildLoad(builder, scalar_ptr, "");

            val = LLVMBuildInsertElement(builder, val, scalar, ii, "");

            LLVMBuildStore(builder, val, temp_ptr);
         }
         lp_build_endif(&if_ctx);
      } else {
         scalar_ptr = LLVMBuildGEP(builder, base_ptr,
                                   &index, 1, "gather_ptr");
         scalar = LLVMBuildLoad(builder, scalar_ptr, "");

         res = LLVMBuildInsertElement(builder, res, scalar, ii, "");
      }
   }

   if (overflow_mask) {
      res = LLVMBuildLoad(builder, temp_ptr, "gather_val");
   }

   return res;
}


/**
 * Scatter/store vector.
 */
static void
emit_mask_scatter(struct lp_build_tgsi_soa_context *bld,
                  LLVMValueRef base_ptr,
                  LLVMValueRef indexes,
                  LLVMValueRef values,
                  struct lp_exec_mask *mask,
                  LLVMValueRef pred)
{
   struct gallivm_state *gallivm = bld->bld_base.base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   unsigned i;

   /* Mix the predicate and execution mask */
   if (mask->has_mask) {
      if (pred) {
         pred = LLVMBuildAnd(builder, pred, mask->exec_mask, "");
      }
      else {
         pred = mask->exec_mask;
      }
   }

   /*
    * Loop over elements of index_vec, store scalar value.
    */
   for (i = 0; i < bld->bld_base.base.type.length; i++) {
      LLVMValueRef ii = lp_build_const_int32(gallivm, i);
      LLVMValueRef index = LLVMBuildExtractElement(builder, indexes, ii, "");
      LLVMValueRef scalar_ptr = LLVMBuildGEP(builder, base_ptr, &index, 1, "scatter_ptr");
      LLVMValueRef val = LLVMBuildExtractElement(builder, values, ii, "scatter_val");
      LLVMValueRef scalar_pred = pred ?
         LLVMBuildExtractElement(builder, pred, ii, "scatter_pred") : NULL;

      if (0)
         lp_build_printf(gallivm, "scatter %d: val %f at %d %p\n",
                         ii, val, index, scalar_ptr);

      if (scalar_pred) {
         LLVMValueRef real_val, dst_val;
         dst_val = LLVMBuildLoad(builder, scalar_ptr, "");
         real_val = lp_build_select(&bld->elem_bld, scalar_pred, val, dst_val);
         LLVMBuildStore(builder, real_val, scalar_ptr);
      }
      else {
         LLVMBuildStore(builder, val, scalar_ptr);
      }
   }
}


/**
 * Read the current value of the ADDR register, convert the floats to
 * ints, add the base index and return the vector of offsets.
 * The offsets will be used to index into the constant buffer or
 * temporary register file.
 */
static LLVMValueRef
get_indirect_index(struct lp_build_tgsi_soa_context *bld,
                   unsigned reg_file, unsigned reg_index,
                   const struct tgsi_ind_register *indirect_reg)
{
   LLVMBuilderRef builder = bld->bld_base.base.gallivm->builder;
   struct lp_build_context *uint_bld = &bld->bld_base.uint_bld;
   /* always use X component of address register */
   unsigned swizzle = indirect_reg->Swizzle;
   LLVMValueRef base;
   LLVMValueRef rel;
   LLVMValueRef max_index;
   LLVMValueRef index;

   assert(bld->indirect_files & (1 << reg_file));

   base = lp_build_const_int_vec(bld->bld_base.base.gallivm, uint_bld->type, reg_index);

   assert(swizzle < 4);
   switch (indirect_reg->File) {
   case TGSI_FILE_ADDRESS:
      rel = LLVMBuildLoad(builder,
                          bld->addr[indirect_reg->Index][swizzle],
                          "load addr reg");
      /* ADDR LLVM values already have LLVM integer type. */
      break;
   case TGSI_FILE_TEMPORARY:
      rel = lp_get_temp_ptr_soa(bld, indirect_reg->Index, swizzle);
      rel = LLVMBuildLoad(builder, rel, "load temp reg");
      /* TEMP LLVM values always have LLVM float type, but for indirection, the
       * value actually stored is expected to be an integer */
      rel = LLVMBuildBitCast(builder, rel, uint_bld->vec_type, "");
      break;
   default:
      assert(0);
      rel = uint_bld->zero;
   }

   index = lp_build_add(uint_bld, base, rel);

   /*
    * emit_fetch_constant handles constant buffer overflow so this code
    * is pointless for them.
    * Furthermore the D3D10 spec in section 6.5 says:
    * If the constant buffer bound to a slot is larger than the size
    * declared in the shader for that slot, implementations are allowed
    * to return incorrect data (not necessarily 0) for indices that are
    * larger than the declared size but smaller than the buffer size.
    */
   if (reg_file != TGSI_FILE_CONSTANT) {
      max_index = lp_build_const_int_vec(bld->bld_base.base.gallivm,
                                         uint_bld->type,
                                         bld->bld_base.info->file_max[reg_file]);

      assert(!uint_bld->type.sign);
      index = lp_build_min(uint_bld, index, max_index);
   }

   return index;
}

static struct lp_build_context *
stype_to_fetch(struct lp_build_tgsi_context * bld_base,
	       enum tgsi_opcode_type stype)
{
   struct lp_build_context *bld_fetch;

   switch (stype) {
   case TGSI_TYPE_FLOAT:
   case TGSI_TYPE_UNTYPED:
      bld_fetch = &bld_base->base;
      break;
   case TGSI_TYPE_UNSIGNED:
      bld_fetch = &bld_base->uint_bld;
      break;
   case TGSI_TYPE_SIGNED:
      bld_fetch = &bld_base->int_bld;
      break;
   case TGSI_TYPE_VOID:
   case TGSI_TYPE_DOUBLE:
   default:
      assert(0);
      bld_fetch = NULL;
      break;
   }
   return bld_fetch;
}

static LLVMValueRef
get_soa_array_offsets(struct lp_build_context *uint_bld,
                      LLVMValueRef indirect_index,
                      unsigned chan_index,
                      boolean need_perelement_offset)
{
   struct gallivm_state *gallivm = uint_bld->gallivm;
   LLVMValueRef chan_vec =
      lp_build_const_int_vec(uint_bld->gallivm, uint_bld->type, chan_index);
   LLVMValueRef length_vec =
      lp_build_const_int_vec(gallivm, uint_bld->type, uint_bld->type.length);
   LLVMValueRef index_vec;

   /* index_vec = (indirect_index * 4 + chan_index) * length + offsets */
   index_vec = lp_build_shl_imm(uint_bld, indirect_index, 2);
   index_vec = lp_build_add(uint_bld, index_vec, chan_vec);
   index_vec = lp_build_mul(uint_bld, index_vec, length_vec);

   if (need_perelement_offset) {
      LLVMValueRef pixel_offsets;
      int i;
     /* build pixel offset vector: {0, 1, 2, 3, ...} */
      pixel_offsets = uint_bld->undef;
      for (i = 0; i < uint_bld->type.length; i++) {
         LLVMValueRef ii = lp_build_const_int32(gallivm, i);
         pixel_offsets = LLVMBuildInsertElement(gallivm->builder, pixel_offsets,
                                                ii, ii, "");
      }
      index_vec = lp_build_add(uint_bld, index_vec, pixel_offsets);
   }
   return index_vec;
}

static LLVMValueRef
emit_fetch_constant(
   struct lp_build_tgsi_context * bld_base,
   const struct tgsi_full_src_register * reg,
   enum tgsi_opcode_type stype,
   unsigned swizzle)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);
   struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   struct lp_build_context *uint_bld = &bld_base->uint_bld;
   unsigned dimension = 0;
   LLVMValueRef dimension_index;
   LLVMValueRef consts_ptr;
   LLVMValueRef num_consts;
   LLVMValueRef res;

   /* XXX: Handle fetching xyzw components as a vector */
   assert(swizzle != ~0);

   if (reg->Register.Dimension) {
      assert(!reg->Dimension.Indirect);
      dimension = reg->Dimension.Index;
      assert(dimension < LP_MAX_TGSI_CONST_BUFFERS);
   }

   dimension_index = lp_build_const_int32(gallivm, dimension);
   consts_ptr =
      lp_build_array_get(gallivm, bld->consts_ptr, dimension_index);
   num_consts =
      lp_build_array_get(gallivm, bld->const_sizes_ptr, dimension_index);

   if (reg->Register.Indirect) {
      LLVMValueRef indirect_index;
      LLVMValueRef swizzle_vec =
         lp_build_const_int_vec(gallivm, uint_bld->type, swizzle);
      LLVMValueRef index_vec;  /* index into the const buffer */
      LLVMValueRef overflow_mask;

      indirect_index = get_indirect_index(bld,
                                          reg->Register.File,
                                          reg->Register.Index,
                                          &reg->Indirect);

      /* All fetches are from the same constant buffer, so
       * we need to propagate the size to a vector to do a
       * vector comparison */
      num_consts = lp_build_broadcast_scalar(uint_bld, num_consts);
      /* Construct a boolean vector telling us which channels
       * overflow the bound constant buffer */
      overflow_mask = LLVMBuildICmp(builder, LLVMIntUGE,
                                    indirect_index,
                                    num_consts, "");

      /* index_vec = indirect_index * 4 + swizzle */
      index_vec = lp_build_shl_imm(uint_bld, indirect_index, 2);
      index_vec = lp_build_add(uint_bld, index_vec, swizzle_vec);

      /* Gather values from the constant buffer */
      res = build_gather(&bld_base->base, consts_ptr, index_vec,
                         &overflow_mask);
   }
   else {
      LLVMValueRef index;  /* index into the const buffer */
      LLVMValueRef scalar, scalar_ptr;

      index = lp_build_const_int32(gallivm, reg->Register.Index * 4 + swizzle);

      scalar_ptr = LLVMBuildGEP(builder, consts_ptr,
                                &index, 1, "");
      scalar = LLVMBuildLoad(builder, scalar_ptr, "");
      res = lp_build_broadcast_scalar(&bld_base->base, scalar);
   }

   if (stype == TGSI_TYPE_SIGNED || stype == TGSI_TYPE_UNSIGNED) {
      struct lp_build_context *bld_fetch = stype_to_fetch(bld_base, stype);
      res = LLVMBuildBitCast(builder, res, bld_fetch->vec_type, "");
   }

   return res;
}

static LLVMValueRef
emit_fetch_immediate(
   struct lp_build_tgsi_context * bld_base,
   const struct tgsi_full_src_register * reg,
   enum tgsi_opcode_type stype,
   unsigned swizzle)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);
   struct gallivm_state *gallivm = bld->bld_base.base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef res = NULL;

   if (bld->use_immediates_array || reg->Register.Indirect) {
      LLVMValueRef imms_array;
      LLVMTypeRef fptr_type;

      /* cast imms_array pointer to float* */
      fptr_type = LLVMPointerType(LLVMFloatTypeInContext(gallivm->context), 0);
      imms_array = LLVMBuildBitCast(builder, bld->imms_array, fptr_type, "");

      if (reg->Register.Indirect) {
         LLVMValueRef indirect_index;
         LLVMValueRef index_vec;  /* index into the immediate register array */

         indirect_index = get_indirect_index(bld,
                                             reg->Register.File,
                                             reg->Register.Index,
                                             &reg->Indirect);
         /*
          * Unlike for other reg classes, adding pixel offsets is unnecessary -
          * immediates are stored as full vectors (FIXME??? - might be better
          * to store them the same as constants) but all elements are the same
          * in any case.
          */
         index_vec = get_soa_array_offsets(&bld_base->uint_bld,
                                           indirect_index,
                                           swizzle,
                                           FALSE);

         /* Gather values from the immediate register array */
         res = build_gather(&bld_base->base, imms_array, index_vec, NULL);
      } else {
         LLVMValueRef lindex = lp_build_const_int32(gallivm,
                                        reg->Register.Index * 4 + swizzle);
         LLVMValueRef imms_ptr =  LLVMBuildGEP(builder,
                                                bld->imms_array, &lindex, 1, "");
         res = LLVMBuildLoad(builder, imms_ptr, "");
      }
   }
   else {
      res = bld->immediates[reg->Register.Index][swizzle];
   }

   if (stype == TGSI_TYPE_UNSIGNED) {
      res = LLVMBuildBitCast(builder, res, bld_base->uint_bld.vec_type, "");
   } else if (stype == TGSI_TYPE_SIGNED) {
      res = LLVMBuildBitCast(builder, res, bld_base->int_bld.vec_type, "");
   }
   return res;
}

static LLVMValueRef
emit_fetch_input(
   struct lp_build_tgsi_context * bld_base,
   const struct tgsi_full_src_register * reg,
   enum tgsi_opcode_type stype,
   unsigned swizzle)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);
   struct gallivm_state *gallivm = bld->bld_base.base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef res;

   if (reg->Register.Indirect) {
      LLVMValueRef indirect_index;
      LLVMValueRef index_vec;  /* index into the input reg array */
      LLVMValueRef inputs_array;
      LLVMTypeRef fptr_type;

      indirect_index = get_indirect_index(bld,
                                          reg->Register.File,
                                          reg->Register.Index,
                                          &reg->Indirect);

      index_vec = get_soa_array_offsets(&bld_base->uint_bld,
                                        indirect_index,
                                        swizzle,
                                        TRUE);

      /* cast inputs_array pointer to float* */
      fptr_type = LLVMPointerType(LLVMFloatTypeInContext(gallivm->context), 0);
      inputs_array = LLVMBuildBitCast(builder, bld->inputs_array, fptr_type, "");

      /* Gather values from the input register array */
      res = build_gather(&bld_base->base, inputs_array, index_vec, NULL);
   } else {
      if (bld->indirect_files & (1 << TGSI_FILE_INPUT)) {
         LLVMValueRef lindex = lp_build_const_int32(gallivm,
                                        reg->Register.Index * 4 + swizzle);
         LLVMValueRef input_ptr =  LLVMBuildGEP(builder,
                                                bld->inputs_array, &lindex, 1, "");
         res = LLVMBuildLoad(builder, input_ptr, "");
      }
      else {
         res = bld->inputs[reg->Register.Index][swizzle];
      }
   }

   assert(res);

   if (stype == TGSI_TYPE_UNSIGNED) {
      res = LLVMBuildBitCast(builder, res, bld_base->uint_bld.vec_type, "");
   } else if (stype == TGSI_TYPE_SIGNED) {
      res = LLVMBuildBitCast(builder, res, bld_base->int_bld.vec_type, "");
   }

   return res;
}


static LLVMValueRef
emit_fetch_gs_input(
   struct lp_build_tgsi_context * bld_base,
   const struct tgsi_full_src_register * reg,
   enum tgsi_opcode_type stype,
   unsigned swizzle)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);
   struct gallivm_state *gallivm = bld->bld_base.base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef attrib_index = NULL;
   LLVMValueRef vertex_index = NULL;
   LLVMValueRef swizzle_index = lp_build_const_int32(gallivm, swizzle);
   LLVMValueRef res;

   if (reg->Register.Indirect) {
      attrib_index = get_indirect_index(bld,
                                        reg->Register.File,
                                        reg->Register.Index,
                                        &reg->Indirect);
   } else {
      attrib_index = lp_build_const_int32(gallivm, reg->Register.Index);
   }
   
   if (reg->Dimension.Indirect) {
      vertex_index = get_indirect_index(bld,
                                        reg->Register.File,
                                        reg->Dimension.Index,
                                        &reg->DimIndirect);
   } else {
      vertex_index = lp_build_const_int32(gallivm, reg->Dimension.Index);
   }

   res = bld->gs_iface->fetch_input(bld->gs_iface, bld_base,
                                    reg->Dimension.Indirect,
                                    vertex_index,
                                    reg->Register.Indirect,
                                    attrib_index,
                                    swizzle_index);

   assert(res);

   if (stype == TGSI_TYPE_UNSIGNED) {
      res = LLVMBuildBitCast(builder, res, bld_base->uint_bld.vec_type, "");
   } else if (stype == TGSI_TYPE_SIGNED) {
      res = LLVMBuildBitCast(builder, res, bld_base->int_bld.vec_type, "");
   }

   return res;
}

static LLVMValueRef
emit_fetch_temporary(
   struct lp_build_tgsi_context * bld_base,
   const struct tgsi_full_src_register * reg,
   enum tgsi_opcode_type stype,
   unsigned swizzle)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);
   struct gallivm_state *gallivm = bld->bld_base.base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef res;

   if (reg->Register.Indirect) {
      LLVMValueRef indirect_index;
      LLVMValueRef index_vec;  /* index into the temp reg array */
      LLVMValueRef temps_array;
      LLVMTypeRef fptr_type;

      indirect_index = get_indirect_index(bld,
                                          reg->Register.File,
                                          reg->Register.Index,
                                          &reg->Indirect);

      index_vec = get_soa_array_offsets(&bld_base->uint_bld,
                                        indirect_index,
                                        swizzle,
                                        TRUE);

      /* cast temps_array pointer to float* */
      fptr_type = LLVMPointerType(LLVMFloatTypeInContext(gallivm->context), 0);
      temps_array = LLVMBuildBitCast(builder, bld->temps_array, fptr_type, "");

      /* Gather values from the temporary register array */
      res = build_gather(&bld_base->base, temps_array, index_vec, NULL);
   }
   else {
      LLVMValueRef temp_ptr;
      temp_ptr = lp_get_temp_ptr_soa(bld, reg->Register.Index, swizzle);
      res = LLVMBuildLoad(builder, temp_ptr, "");
   }

   if (stype == TGSI_TYPE_SIGNED || stype == TGSI_TYPE_UNSIGNED) {
      struct lp_build_context *bld_fetch = stype_to_fetch(bld_base, stype);
      res = LLVMBuildBitCast(builder, res, bld_fetch->vec_type, "");
   }

   return res;
}

static LLVMValueRef
emit_fetch_system_value(
   struct lp_build_tgsi_context * bld_base,
   const struct tgsi_full_src_register * reg,
   enum tgsi_opcode_type stype,
   unsigned swizzle)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);
   struct gallivm_state *gallivm = bld->bld_base.base.gallivm;
   const struct tgsi_shader_info *info = bld->bld_base.info;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef res;
   enum tgsi_opcode_type atype; // Actual type of the value

   assert(!reg->Register.Indirect);

   switch (info->system_value_semantic_name[reg->Register.Index]) {
   case TGSI_SEMANTIC_INSTANCEID:
      res = lp_build_broadcast_scalar(&bld_base->uint_bld, bld->system_values.instance_id);
      atype = TGSI_TYPE_UNSIGNED;
      break;

   case TGSI_SEMANTIC_VERTEXID:
      res = bld->system_values.vertex_id;
      atype = TGSI_TYPE_UNSIGNED;
      break;

   case TGSI_SEMANTIC_PRIMID:
      res = bld->system_values.prim_id;
      atype = TGSI_TYPE_UNSIGNED;
      break;

   default:
      assert(!"unexpected semantic in emit_fetch_system_value");
      res = bld_base->base.zero;
      atype = TGSI_TYPE_FLOAT;
      break;
   }

   if (atype != stype) {
      if (stype == TGSI_TYPE_FLOAT) {
         res = LLVMBuildBitCast(builder, res, bld_base->base.vec_type, "");
      } else if (stype == TGSI_TYPE_UNSIGNED) {
         res = LLVMBuildBitCast(builder, res, bld_base->uint_bld.vec_type, "");
      } else if (stype == TGSI_TYPE_SIGNED) {
         res = LLVMBuildBitCast(builder, res, bld_base->int_bld.vec_type, "");
      }
   }

   return res;
}

/**
 * Register fetch with derivatives.
 */
static void
emit_fetch_deriv(
   struct lp_build_tgsi_soa_context *bld,
   LLVMValueRef src,
   LLVMValueRef *res,
   LLVMValueRef *ddx,
   LLVMValueRef *ddy)
{
   if(res)
      *res = src;

   /* TODO: use interpolation coeffs for inputs */

   if(ddx)
      *ddx = lp_build_ddx(&bld->bld_base.base, src);

   if(ddy)
      *ddy = lp_build_ddy(&bld->bld_base.base, src);
}


/**
 * Predicate.
 */
static void
emit_fetch_predicate(
   struct lp_build_tgsi_soa_context *bld,
   const struct tgsi_full_instruction *inst,
   LLVMValueRef *pred)
{
   LLVMBuilderRef builder = bld->bld_base.base.gallivm->builder;
   unsigned index;
   unsigned char swizzles[4];
   LLVMValueRef unswizzled[4] = {NULL, NULL, NULL, NULL};
   LLVMValueRef value;
   unsigned chan;

   if (!inst->Instruction.Predicate) {
      TGSI_FOR_EACH_CHANNEL( chan ) {
         pred[chan] = NULL;
      }
      return;
   }

   swizzles[0] = inst->Predicate.SwizzleX;
   swizzles[1] = inst->Predicate.SwizzleY;
   swizzles[2] = inst->Predicate.SwizzleZ;
   swizzles[3] = inst->Predicate.SwizzleW;

   index = inst->Predicate.Index;
   assert(index < LP_MAX_TGSI_PREDS);

   TGSI_FOR_EACH_CHANNEL( chan ) {
      unsigned swizzle = swizzles[chan];

      /*
       * Only fetch the predicate register channels that are actually listed
       * in the swizzles
       */
      if (!unswizzled[swizzle]) {
         value = LLVMBuildLoad(builder,
                               bld->preds[index][swizzle], "");

         /*
          * Convert the value to an integer mask.
          *
          * TODO: Short-circuit this comparison -- a D3D setp_xx instructions
          * is needlessly causing two comparisons due to storing the intermediate
          * result as float vector instead of an integer mask vector.
          */
         value = lp_build_compare(bld->bld_base.base.gallivm,
                                  bld->bld_base.base.type,
                                  PIPE_FUNC_NOTEQUAL,
                                  value,
                                  bld->bld_base.base.zero);
         if (inst->Predicate.Negate) {
            value = LLVMBuildNot(builder, value, "");
         }

         unswizzled[swizzle] = value;
      } else {
         value = unswizzled[swizzle];
      }

      pred[chan] = value;
   }
}


/**
 * Register store.
 */
static void
emit_store_chan(
   struct lp_build_tgsi_context *bld_base,
   const struct tgsi_full_instruction *inst,
   unsigned index,
   unsigned chan_index,
   LLVMValueRef pred,
   LLVMValueRef value)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);
   struct gallivm_state *gallivm = bld_base->base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   const struct tgsi_full_dst_register *reg = &inst->Dst[index];
   struct lp_build_context *float_bld = &bld_base->base;
   struct lp_build_context *int_bld = &bld_base->int_bld;
   LLVMValueRef indirect_index = NULL;
   enum tgsi_opcode_type dtype = tgsi_opcode_infer_dst_type(inst->Instruction.Opcode);

   /*
    * Apply saturation.
    *
    * It is always assumed to be float.
    */
   switch( inst->Instruction.Saturate ) {
   case TGSI_SAT_NONE:
      break;

   case TGSI_SAT_ZERO_ONE:
      assert(dtype == TGSI_TYPE_FLOAT ||
             dtype == TGSI_TYPE_UNTYPED);
      value = LLVMBuildBitCast(builder, value, float_bld->vec_type, "");
      value = lp_build_clamp_zero_one_nanzero(float_bld, value);
      break;

   case TGSI_SAT_MINUS_PLUS_ONE:
      assert(dtype == TGSI_TYPE_FLOAT ||
             dtype == TGSI_TYPE_UNTYPED);
      value = LLVMBuildBitCast(builder, value, float_bld->vec_type, "");
      /* This will give -1.0 for NaN which is probably not what we want. */
      value = lp_build_max_ext(float_bld, value,
                               lp_build_const_vec(gallivm, float_bld->type, -1.0),
                               GALLIVM_NAN_RETURN_OTHER_SECOND_NONNAN);
      value = lp_build_min(float_bld, value, float_bld->one);
      break;

   default:
      assert(0);
   }

   if (reg->Register.Indirect) {
      indirect_index = get_indirect_index(bld,
                                          reg->Register.File,
                                          reg->Register.Index,
                                          &reg->Indirect);
   } else {
      assert(reg->Register.Index <=
                             bld_base->info->file_max[reg->Register.File]);
   }

   if (DEBUG_EXECUTION) {
      emit_dump_reg(gallivm, reg->Register.File, reg->Register.Index, chan_index, value);
   }

   switch( reg->Register.File ) {
   case TGSI_FILE_OUTPUT:
      /* Outputs are always stored as floats */
      value = LLVMBuildBitCast(builder, value, float_bld->vec_type, "");

      if (reg->Register.Indirect) {
         LLVMValueRef index_vec;  /* indexes into the output registers */
         LLVMValueRef outputs_array;
         LLVMTypeRef fptr_type;

         index_vec = get_soa_array_offsets(&bld_base->uint_bld,
                                           indirect_index,
                                           chan_index,
                                           TRUE);

         fptr_type = LLVMPointerType(LLVMFloatTypeInContext(gallivm->context), 0);
         outputs_array = LLVMBuildBitCast(builder, bld->outputs_array, fptr_type, "");

         /* Scatter store values into output registers */
         emit_mask_scatter(bld, outputs_array, index_vec, value,
                           &bld->exec_mask, pred);
      }
      else {
         LLVMValueRef out_ptr = lp_get_output_ptr(bld, reg->Register.Index,
                                                  chan_index);
         lp_exec_mask_store(&bld->exec_mask, float_bld, pred, value, out_ptr);
      }
      break;

   case TGSI_FILE_TEMPORARY:
      /* Temporaries are always stored as floats */
      value = LLVMBuildBitCast(builder, value, float_bld->vec_type, "");

      if (reg->Register.Indirect) {
         LLVMValueRef index_vec;  /* indexes into the temp registers */
         LLVMValueRef temps_array;
         LLVMTypeRef fptr_type;

         index_vec = get_soa_array_offsets(&bld_base->uint_bld,
                                           indirect_index,
                                           chan_index,
                                           TRUE);

         fptr_type = LLVMPointerType(LLVMFloatTypeInContext(gallivm->context), 0);
         temps_array = LLVMBuildBitCast(builder, bld->temps_array, fptr_type, "");

         /* Scatter store values into temp registers */
         emit_mask_scatter(bld, temps_array, index_vec, value,
                           &bld->exec_mask, pred);
      }
      else {
         LLVMValueRef temp_ptr;
         temp_ptr = lp_get_temp_ptr_soa(bld, reg->Register.Index, chan_index);
         lp_exec_mask_store(&bld->exec_mask, float_bld, pred, value, temp_ptr);
      }
      break;

   case TGSI_FILE_ADDRESS:
      assert(dtype == TGSI_TYPE_SIGNED);
      assert(LLVMTypeOf(value) == int_bld->vec_type);
      value = LLVMBuildBitCast(builder, value, int_bld->vec_type, "");
      lp_exec_mask_store(&bld->exec_mask, int_bld, pred, value,
                         bld->addr[reg->Register.Index][chan_index]);
      break;

   case TGSI_FILE_PREDICATE:
      assert(LLVMTypeOf(value) == float_bld->vec_type);
      value = LLVMBuildBitCast(builder, value, float_bld->vec_type, "");
      lp_exec_mask_store(&bld->exec_mask, float_bld, pred, value,
                         bld->preds[reg->Register.Index][chan_index]);
      break;

   default:
      assert( 0 );
   }

   (void)dtype;
}

/*
 * Called at the beginning of the translation of each TGSI instruction, to
 * emit some debug code.
 */
static void
emit_debug(
   struct lp_build_tgsi_context * bld_base,
   const struct tgsi_full_instruction * inst,
   const struct tgsi_opcode_info * info)

{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   if (DEBUG_EXECUTION) {
      /*
       * Dump the TGSI instruction.
       */

      struct gallivm_state *gallivm = bld_base->base.gallivm;
      char buf[512];
      buf[0] = '$';
      buf[1] = ' ';
      tgsi_dump_instruction_str(inst, bld_base->pc, &buf[2], sizeof buf - 2);
      lp_build_printf(gallivm, buf);

      /* Dump the execution mask.
       */
      if (bld->exec_mask.has_mask) {
         lp_build_print_value(gallivm, "    mask = ", bld->exec_mask.exec_mask);
      }
   }
}

static void
emit_store(
   struct lp_build_tgsi_context * bld_base,
   const struct tgsi_full_instruction * inst,
   const struct tgsi_opcode_info * info,
   LLVMValueRef dst[4])

{
   unsigned chan_index;
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   if(info->num_dst) {
      LLVMValueRef pred[TGSI_NUM_CHANNELS];

      emit_fetch_predicate( bld, inst, pred );

      TGSI_FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
         emit_store_chan(bld_base, inst, 0, chan_index, pred[chan_index], dst[chan_index]);
      }
   }
}

static unsigned
tgsi_to_pipe_tex_target(unsigned tgsi_target)
{
   switch (tgsi_target) {
   case TGSI_TEXTURE_BUFFER:
      return PIPE_BUFFER;
   case TGSI_TEXTURE_1D:
   case TGSI_TEXTURE_SHADOW1D:
      return PIPE_TEXTURE_1D;
   case TGSI_TEXTURE_2D:
   case TGSI_TEXTURE_SHADOW2D:
   case TGSI_TEXTURE_2D_MSAA:
      return PIPE_TEXTURE_2D;
   case TGSI_TEXTURE_3D:
      return PIPE_TEXTURE_3D;
   case TGSI_TEXTURE_CUBE:
   case TGSI_TEXTURE_SHADOWCUBE:
      return PIPE_TEXTURE_CUBE;
   case TGSI_TEXTURE_RECT:
   case TGSI_TEXTURE_SHADOWRECT:
      return PIPE_TEXTURE_RECT;
   case TGSI_TEXTURE_1D_ARRAY:
   case TGSI_TEXTURE_SHADOW1D_ARRAY:
      return PIPE_TEXTURE_1D_ARRAY;
   case TGSI_TEXTURE_2D_ARRAY:
   case TGSI_TEXTURE_SHADOW2D_ARRAY:
   case TGSI_TEXTURE_2D_ARRAY_MSAA:
      return PIPE_TEXTURE_2D_ARRAY;
   case TGSI_TEXTURE_CUBE_ARRAY:
   case TGSI_TEXTURE_SHADOWCUBE_ARRAY:
      return PIPE_TEXTURE_CUBE_ARRAY;
   default:
      assert(0);
      return PIPE_BUFFER;
   }
}


static enum lp_sampler_lod_property
lp_build_lod_property(
   struct lp_build_tgsi_context *bld_base,
   const struct tgsi_full_instruction *inst,
   unsigned src_op)
{
   const struct tgsi_full_src_register *reg = &inst->Src[src_op];
   enum lp_sampler_lod_property lod_property;

   /*
    * Not much we can do here. We could try catching inputs declared
    * with constant interpolation but not sure it's worth it - since for
    * TEX opcodes as well as FETCH/LD the lod comes from same reg as
    * the coords, so it could only work for SAMPLE/TXQ/SVIEWINFO), just
    * like the constant/immediate recognition below.
    * What seems to be of more value would be to recognize temps holding
    * broadcasted scalars but no way we can do it.
    * Tried asking llvm but without any success (using LLVMIsConstant
    * even though this isn't exactly what we'd need), even as simple as
    * IMM[0] UINT32 (0,-1,0,0)
    * MOV TEMP[0] IMM[0].yyyy
    * SVIEWINFO TEMP[1], TEMP[0].xxxx, SVIEWINFO[0]
    * doesn't work.
    * This means there's ZERO chance this will ever catch a scalar lod
    * with traditional tex opcodes as well as texel fetches, since the lod
    * comes from the same reg as coords (except some test shaders using
    * constant coords maybe).
    * There's at least hope for sample opcodes as well as size queries.
    */
   if (reg->Register.File == TGSI_FILE_CONSTANT ||
       reg->Register.File == TGSI_FILE_IMMEDIATE) {
      lod_property = LP_SAMPLER_LOD_SCALAR;
   }
   else if (bld_base->info->processor == TGSI_PROCESSOR_FRAGMENT) {
      if (gallivm_debug & GALLIVM_DEBUG_NO_QUAD_LOD) {
         lod_property = LP_SAMPLER_LOD_PER_ELEMENT;
      }
      else {
         lod_property = LP_SAMPLER_LOD_PER_QUAD;
      }
   }
   else {
      /* never use scalar (per-quad) lod the results are just too wrong. */
      lod_property = LP_SAMPLER_LOD_PER_ELEMENT;
   }
   return lod_property;
}


/**
 * High-level instruction translators.
 */

static void
emit_tex( struct lp_build_tgsi_soa_context *bld,
          const struct tgsi_full_instruction *inst,
          enum lp_build_tex_modifier modifier,
          LLVMValueRef *texel)
{
   unsigned unit;
   LLVMValueRef lod_bias, explicit_lod;
   LLVMValueRef oow = NULL;
   LLVMValueRef coords[5];
   LLVMValueRef offsets[3] = { NULL };
   struct lp_derivatives derivs;
   struct lp_derivatives *deriv_ptr = NULL;
   enum lp_sampler_lod_property lod_property = LP_SAMPLER_LOD_SCALAR;
   unsigned num_derivs, num_offsets, i;
   unsigned shadow_coord = 0;
   unsigned layer_coord = 0;

   if (!bld->sampler) {
      _debug_printf("warning: found texture instruction but no sampler generator supplied\n");
      for (i = 0; i < 4; i++) {
         texel[i] = bld->bld_base.base.undef;
      }
      return;
   }

   switch (inst->Texture.Texture) {
   case TGSI_TEXTURE_1D_ARRAY:
      layer_coord = 1;
      /* fallthrough */
   case TGSI_TEXTURE_1D:
      num_offsets = 1;
      num_derivs = 1;
      break;
   case TGSI_TEXTURE_2D_ARRAY:
      layer_coord = 2;
      /* fallthrough */
   case TGSI_TEXTURE_2D:
   case TGSI_TEXTURE_RECT:
      num_offsets = 2;
      num_derivs = 2;
      break;
   case TGSI_TEXTURE_SHADOW1D_ARRAY:
      layer_coord = 1;
      /* fallthrough */
   case TGSI_TEXTURE_SHADOW1D:
      shadow_coord = 2;
      num_offsets = 1;
      num_derivs = 1;
      break;
   case TGSI_TEXTURE_SHADOW2D_ARRAY:
      layer_coord = 2;
      shadow_coord = 3;
      num_offsets = 2;
      num_derivs = 2;
      break;
   case TGSI_TEXTURE_SHADOW2D:
   case TGSI_TEXTURE_SHADOWRECT:
      shadow_coord = 2;
      num_offsets = 2;
      num_derivs = 2;
      break;
   case TGSI_TEXTURE_CUBE:
      num_offsets = 2;
      num_derivs = 3;
      break;
   case TGSI_TEXTURE_3D:
      num_offsets = 3;
      num_derivs = 3;
      break;
   case TGSI_TEXTURE_SHADOWCUBE:
      shadow_coord = 3;
      num_offsets = 2;
      num_derivs = 3;
      break;
   case TGSI_TEXTURE_CUBE_ARRAY:
   case TGSI_TEXTURE_SHADOWCUBE_ARRAY:
   case TGSI_TEXTURE_2D_MSAA:
   case TGSI_TEXTURE_2D_ARRAY_MSAA:
   default:
      assert(0);
      return;
   }

   /* Note lod and especially projected are illegal in a LOT of cases */
   if (modifier == LP_BLD_TEX_MODIFIER_LOD_BIAS ||
       modifier == LP_BLD_TEX_MODIFIER_EXPLICIT_LOD) {
      LLVMValueRef lod = lp_build_emit_fetch(&bld->bld_base, inst, 0, 3);
      if (modifier == LP_BLD_TEX_MODIFIER_LOD_BIAS) {
         lod_bias = lod;
         explicit_lod = NULL;
      }
      else if (modifier == LP_BLD_TEX_MODIFIER_EXPLICIT_LOD) {
         lod_bias = NULL;
         explicit_lod = lod;
      }
      lod_property = lp_build_lod_property(&bld->bld_base, inst, 0);
   }
   else {
      lod_bias = NULL;
      explicit_lod = NULL;
   }

   if (modifier == LP_BLD_TEX_MODIFIER_PROJECTED) {
      oow = lp_build_emit_fetch(&bld->bld_base, inst, 0, 3);
      oow = lp_build_rcp(&bld->bld_base.base, oow);
   }

   for (i = 0; i < num_derivs; i++) {
      coords[i] = lp_build_emit_fetch(&bld->bld_base, inst, 0, i);
      if (modifier == LP_BLD_TEX_MODIFIER_PROJECTED)
         coords[i] = lp_build_mul(&bld->bld_base.base, coords[i], oow);
   }
   for (i = num_derivs; i < 5; i++) {
      coords[i] = bld->bld_base.base.undef;
   }

   /* Layer coord always goes into 3rd slot, except for cube map arrays */
   if (layer_coord) {
      coords[2] = lp_build_emit_fetch(&bld->bld_base, inst, 0, layer_coord);
      if (modifier == LP_BLD_TEX_MODIFIER_PROJECTED)
         coords[2] = lp_build_mul(&bld->bld_base.base, coords[2], oow);
   }
   /* Shadow coord occupies always 5th slot. */
   if (shadow_coord) {
      coords[4] = lp_build_emit_fetch(&bld->bld_base, inst, 0, shadow_coord);
      if (modifier == LP_BLD_TEX_MODIFIER_PROJECTED)
         coords[4] = lp_build_mul(&bld->bld_base.base, coords[4], oow);
   }

   if (modifier == LP_BLD_TEX_MODIFIER_EXPLICIT_DERIV) {
      unsigned dim;
      for (dim = 0; dim < num_derivs; ++dim) {
         derivs.ddx[dim] = lp_build_emit_fetch(&bld->bld_base, inst, 1, dim);
         derivs.ddy[dim] = lp_build_emit_fetch(&bld->bld_base, inst, 2, dim);
      }
      deriv_ptr = &derivs;
      unit = inst->Src[3].Register.Index;
      /*
       * could also check all src regs if constant but I doubt such
       * cases exist in practice.
       */
      if (bld->bld_base.info->processor == TGSI_PROCESSOR_FRAGMENT) {
         if (gallivm_debug & GALLIVM_DEBUG_NO_QUAD_LOD) {
            lod_property = LP_SAMPLER_LOD_PER_ELEMENT;
         }
         else {
            lod_property = LP_SAMPLER_LOD_PER_QUAD;
         }
      }
      else {
         lod_property = LP_SAMPLER_LOD_PER_ELEMENT;
      }
   } else {
      unit = inst->Src[1].Register.Index;
   }

   /* some advanced gather instructions (txgo) would require 4 offsets */
   if (inst->Texture.NumOffsets == 1) {
      unsigned dim;
      for (dim = 0; dim < num_offsets; dim++) {
         offsets[dim] = lp_build_emit_fetch_texoffset(&bld->bld_base, inst, 0, dim);
      }
   }

   bld->sampler->emit_fetch_texel(bld->sampler,
                                  bld->bld_base.base.gallivm,
                                  bld->bld_base.base.type,
                                  FALSE,
                                  unit, unit,
                                  coords,
                                  offsets,
                                  deriv_ptr,
                                  lod_bias, explicit_lod, lod_property,
                                  texel);
}

static void
emit_sample(struct lp_build_tgsi_soa_context *bld,
            const struct tgsi_full_instruction *inst,
            enum lp_build_tex_modifier modifier,
            boolean compare,
            LLVMValueRef *texel)
{
   struct gallivm_state *gallivm = bld->bld_base.base.gallivm;
   unsigned texture_unit, sampler_unit;
   LLVMValueRef lod_bias, explicit_lod;
   LLVMValueRef coords[5];
   LLVMValueRef offsets[3] = { NULL };
   struct lp_derivatives derivs;
   struct lp_derivatives *deriv_ptr = NULL;
   enum lp_sampler_lod_property lod_property = LP_SAMPLER_LOD_SCALAR;

   unsigned num_offsets, num_derivs, i;
   unsigned layer_coord = 0;

   if (!bld->sampler) {
      _debug_printf("warning: found texture instruction but no sampler generator supplied\n");
      for (i = 0; i < 4; i++) {
         texel[i] = bld->bld_base.base.undef;
      }
      return;
   }

   /*
    * unlike old-style tex opcodes the texture/sampler indices
    * always come from src1 and src2 respectively.
    */
   texture_unit = inst->Src[1].Register.Index;
   sampler_unit = inst->Src[2].Register.Index;

   /*
    * Note inst->Texture.Texture will contain the number of offsets,
    * however the target information is NOT there and comes from the
    * declared sampler views instead.
    */
   switch (bld->sv[texture_unit].Resource) {
   case TGSI_TEXTURE_1D:
      num_offsets = 1;
      num_derivs = 1;
      break;
   case TGSI_TEXTURE_1D_ARRAY:
      layer_coord = 1;
      num_offsets = 1;
      num_derivs = 1;
      break;
   case TGSI_TEXTURE_2D:
   case TGSI_TEXTURE_RECT:
      num_offsets = 2;
      num_derivs = 2;
      break;
   case TGSI_TEXTURE_2D_ARRAY:
      layer_coord = 2;
      num_offsets = 2;
      num_derivs = 2;
      break;
   case TGSI_TEXTURE_CUBE:
      num_offsets = 2;
      num_derivs = 3;
      break;
   case TGSI_TEXTURE_3D:
      num_offsets = 3;
      num_derivs = 3;
      break;
   case TGSI_TEXTURE_CUBE_ARRAY:
      layer_coord = 3;
      num_offsets = 2;
      num_derivs = 3;
      break;
   default:
      assert(0);
      return;
   }

   if (modifier == LP_BLD_TEX_MODIFIER_LOD_BIAS ||
       modifier == LP_BLD_TEX_MODIFIER_EXPLICIT_LOD) {
      LLVMValueRef lod = lp_build_emit_fetch(&bld->bld_base, inst, 3, 0);
      if (modifier == LP_BLD_TEX_MODIFIER_LOD_BIAS) {
         lod_bias = lod;
         explicit_lod = NULL;
      }
      else if (modifier == LP_BLD_TEX_MODIFIER_EXPLICIT_LOD) {
         lod_bias = NULL;
         explicit_lod = lod;
      }
      lod_property = lp_build_lod_property(&bld->bld_base, inst, 0);
   }
   else if (modifier == LP_BLD_TEX_MODIFIER_LOD_ZERO) {
      lod_bias = NULL;
      /* XXX might be better to explicitly pass the level zero information */
      explicit_lod = lp_build_const_vec(gallivm, bld->bld_base.base.type, 0.0F);
   }
   else {
      lod_bias = NULL;
      explicit_lod = NULL;
   }

   for (i = 0; i < num_derivs; i++) {
      coords[i] = lp_build_emit_fetch(&bld->bld_base, inst, 0, i);
   }
   for (i = num_derivs; i < 5; i++) {
      coords[i] = bld->bld_base.base.undef;
   }

   /* Layer coord always goes into 3rd slot, except for cube map arrays */
   if (layer_coord) {
      if (layer_coord == 3)
         coords[3] = lp_build_emit_fetch(&bld->bld_base, inst, 0, layer_coord);
      else
         coords[2] = lp_build_emit_fetch(&bld->bld_base, inst, 0, layer_coord);
   }
   /* Shadow coord occupies always 5th slot. */
   if (compare) {
      coords[4] = lp_build_emit_fetch(&bld->bld_base, inst, 3, 0);
   }

   if (modifier == LP_BLD_TEX_MODIFIER_EXPLICIT_DERIV) {
      unsigned dim;
      for (dim = 0; dim < num_derivs; ++dim) {
         derivs.ddx[dim] = lp_build_emit_fetch(&bld->bld_base, inst, 3, dim);
         derivs.ddy[dim] = lp_build_emit_fetch(&bld->bld_base, inst, 4, dim);
      }
      deriv_ptr = &derivs;
      /*
       * could also check all src regs if constant but I doubt such
       * cases exist in practice.
       */
      if (bld->bld_base.info->processor == TGSI_PROCESSOR_FRAGMENT) {
         if (gallivm_debug & GALLIVM_DEBUG_NO_QUAD_LOD) {
            lod_property = LP_SAMPLER_LOD_PER_ELEMENT;
         }
         else {
            lod_property = LP_SAMPLER_LOD_PER_QUAD;
         }
      }
      else {
         lod_property = LP_SAMPLER_LOD_PER_ELEMENT;
      }
   }

   /* some advanced gather instructions (txgo) would require 4 offsets */
   if (inst->Texture.NumOffsets == 1) {
      unsigned dim;
      for (dim = 0; dim < num_offsets; dim++) {
         offsets[dim] = lp_build_emit_fetch_texoffset(&bld->bld_base, inst, 0, dim);
      }
   }

   bld->sampler->emit_fetch_texel(bld->sampler,
                                  bld->bld_base.base.gallivm,
                                  bld->bld_base.base.type,
                                  FALSE,
                                  texture_unit, sampler_unit,
                                  coords,
                                  offsets,
                                  deriv_ptr,
                                  lod_bias, explicit_lod, lod_property,
                                  texel);

   if (inst->Src[1].Register.SwizzleX != PIPE_SWIZZLE_RED ||
       inst->Src[1].Register.SwizzleY != PIPE_SWIZZLE_GREEN ||
       inst->Src[1].Register.SwizzleZ != PIPE_SWIZZLE_BLUE ||
       inst->Src[1].Register.SwizzleW != PIPE_SWIZZLE_ALPHA) {
      unsigned char swizzles[4];
      swizzles[0] = inst->Src[1].Register.SwizzleX;
      swizzles[1] = inst->Src[1].Register.SwizzleY;
      swizzles[2] = inst->Src[1].Register.SwizzleZ;
      swizzles[3] = inst->Src[1].Register.SwizzleW;

      lp_build_swizzle_soa_inplace(&bld->bld_base.base, texel, swizzles);
   }
}

static void
emit_fetch_texels( struct lp_build_tgsi_soa_context *bld,
                   const struct tgsi_full_instruction *inst,
                   LLVMValueRef *texel,
                   boolean is_samplei)
{
   unsigned unit, target;
   LLVMValueRef coord_undef = LLVMGetUndef(bld->bld_base.base.int_vec_type);
   LLVMValueRef explicit_lod = NULL;
   LLVMValueRef coords[3];
   LLVMValueRef offsets[3] = { NULL };
   enum lp_sampler_lod_property lod_property = LP_SAMPLER_LOD_SCALAR;
   unsigned dims, i;
   unsigned layer_coord = 0;

   if (!bld->sampler) {
      _debug_printf("warning: found texture instruction but no sampler generator supplied\n");
      for (i = 0; i < 4; i++) {
         texel[i] = coord_undef;
      }
      return;
   }

   unit = inst->Src[1].Register.Index;

   if (is_samplei) {
      target = bld->sv[unit].Resource;
   }
   else {
      target = inst->Texture.Texture;
   }

   switch (target) {
   case TGSI_TEXTURE_1D:
   case TGSI_TEXTURE_BUFFER:
      dims = 1;
      break;
   case TGSI_TEXTURE_1D_ARRAY:
      layer_coord = 1;
      dims = 1;
      break;
   case TGSI_TEXTURE_2D:
   case TGSI_TEXTURE_RECT:
      dims = 2;
      break;
   case TGSI_TEXTURE_2D_ARRAY:
      layer_coord = 2;
      dims = 2;
      break;
   case TGSI_TEXTURE_3D:
      dims = 3;
      break;
   default:
      assert(0);
      return;
   }

   /* always have lod except for buffers ? */
   if (target != TGSI_TEXTURE_BUFFER) {
      explicit_lod = lp_build_emit_fetch(&bld->bld_base, inst, 0, 3);
      lod_property = lp_build_lod_property(&bld->bld_base, inst, 0);
   }

   for (i = 0; i < dims; i++) {
      coords[i] = lp_build_emit_fetch(&bld->bld_base, inst, 0, i);
   }
   for (i = dims; i < 3; i++) {
      coords[i] = coord_undef;
   }
   if (layer_coord)
      coords[2] = lp_build_emit_fetch(&bld->bld_base, inst, 0, layer_coord);

   if (inst->Texture.NumOffsets == 1) {
      unsigned dim;
      for (dim = 0; dim < dims; dim++) {
         offsets[dim] = lp_build_emit_fetch_texoffset(&bld->bld_base, inst, 0, dim);
      }
   }

   bld->sampler->emit_fetch_texel(bld->sampler,
                                  bld->bld_base.base.gallivm,
                                  bld->bld_base.base.type,
                                  TRUE,
                                  unit, unit,
                                  coords,
                                  offsets,
                                  NULL,
                                  NULL, explicit_lod, lod_property,
                                  texel);

   if (is_samplei &&
       (inst->Src[1].Register.SwizzleX != PIPE_SWIZZLE_RED ||
        inst->Src[1].Register.SwizzleY != PIPE_SWIZZLE_GREEN ||
        inst->Src[1].Register.SwizzleZ != PIPE_SWIZZLE_BLUE ||
        inst->Src[1].Register.SwizzleW != PIPE_SWIZZLE_ALPHA)) {
      unsigned char swizzles[4];
      swizzles[0] = inst->Src[1].Register.SwizzleX;
      swizzles[1] = inst->Src[1].Register.SwizzleY;
      swizzles[2] = inst->Src[1].Register.SwizzleZ;
      swizzles[3] = inst->Src[1].Register.SwizzleW;

      lp_build_swizzle_soa_inplace(&bld->bld_base.base, texel, swizzles);
   }
}

static void
emit_size_query( struct lp_build_tgsi_soa_context *bld,
                 const struct tgsi_full_instruction *inst,
                 LLVMValueRef *sizes_out,
                 boolean is_sviewinfo)
{
   LLVMValueRef explicit_lod;
   enum lp_sampler_lod_property lod_property;
   unsigned has_lod;
   unsigned i;
   unsigned unit = inst->Src[1].Register.Index;
   unsigned target, pipe_target;

   if (is_sviewinfo) {
      target = bld->sv[unit].Resource;
   }
   else {
      target = inst->Texture.Texture;
   }
   switch (target) {
   case TGSI_TEXTURE_BUFFER:
   case TGSI_TEXTURE_RECT:
   case TGSI_TEXTURE_SHADOWRECT:
      has_lod = 0;
      break;
   default:
      has_lod = 1;
      break;
   }

   if (!bld->sampler) {
      _debug_printf("warning: found texture query instruction but no sampler generator supplied\n");
      for (i = 0; i < 4; i++)
         sizes_out[i] = bld->bld_base.int_bld.undef;
      return;
   }

   if (has_lod) {
      explicit_lod = lp_build_emit_fetch(&bld->bld_base, inst, 0, 0);
      lod_property = lp_build_lod_property(&bld->bld_base, inst, 0);
   }
   else {
      explicit_lod = NULL;
      lod_property = LP_SAMPLER_LOD_SCALAR;
   }


   pipe_target = tgsi_to_pipe_tex_target(target);

   bld->sampler->emit_size_query(bld->sampler,
                                 bld->bld_base.base.gallivm,
                                 bld->bld_base.int_bld.type,
                                 unit, pipe_target,
                                 is_sviewinfo,
                                 lod_property,
                                 explicit_lod,
                                 sizes_out);
}

static boolean
near_end_of_shader(struct lp_build_tgsi_soa_context *bld,
		   int pc)
{
   int i;

   for (i = 0; i < 5; i++) {
      unsigned opcode;

      if (pc + i >= bld->bld_base.info->num_instructions)
	 return TRUE;

      opcode = bld->bld_base.instructions[pc + i].Instruction.Opcode;

      if (opcode == TGSI_OPCODE_END)
	 return TRUE;

      if (opcode == TGSI_OPCODE_TEX ||
	  opcode == TGSI_OPCODE_TXP ||
	  opcode == TGSI_OPCODE_TXD ||
	  opcode == TGSI_OPCODE_TXB ||
	  opcode == TGSI_OPCODE_TXL ||
	  opcode == TGSI_OPCODE_TXF ||
	  opcode == TGSI_OPCODE_TXQ ||
	  opcode == TGSI_OPCODE_CAL ||
	  opcode == TGSI_OPCODE_CALLNZ ||
	  opcode == TGSI_OPCODE_IF ||
          opcode == TGSI_OPCODE_UIF ||
	  opcode == TGSI_OPCODE_BGNLOOP ||
	  opcode == TGSI_OPCODE_SWITCH)
	 return FALSE;
   }

   return TRUE;
}



/**
 * Kill fragment if any of the src register values are negative.
 */
static void
emit_kill_if(
   struct lp_build_tgsi_soa_context *bld,
   const struct tgsi_full_instruction *inst,
   int pc)
{
   LLVMBuilderRef builder = bld->bld_base.base.gallivm->builder;
   const struct tgsi_full_src_register *reg = &inst->Src[0];
   LLVMValueRef terms[TGSI_NUM_CHANNELS];
   LLVMValueRef mask;
   unsigned chan_index;

   memset(&terms, 0, sizeof terms);

   TGSI_FOR_EACH_CHANNEL( chan_index ) {
      unsigned swizzle;

      /* Unswizzle channel */
      swizzle = tgsi_util_get_full_src_register_swizzle( reg, chan_index );

      /* Check if the component has not been already tested. */
      assert(swizzle < TGSI_NUM_CHANNELS);
      if( !terms[swizzle] )
         /* TODO: change the comparison operator instead of setting the sign */
         terms[swizzle] =  lp_build_emit_fetch(&bld->bld_base, inst, 0, chan_index );
   }

   mask = NULL;
   TGSI_FOR_EACH_CHANNEL( chan_index ) {
      if(terms[chan_index]) {
         LLVMValueRef chan_mask;

         /*
          * If term < 0 then mask = 0 else mask = ~0.
          */
         chan_mask = lp_build_cmp(&bld->bld_base.base, PIPE_FUNC_GEQUAL, terms[chan_index], bld->bld_base.base.zero);

         if(mask)
            mask = LLVMBuildAnd(builder, mask, chan_mask, "");
         else
            mask = chan_mask;
      }
   }

   if (bld->exec_mask.has_mask) {
      LLVMValueRef invmask;
      invmask = LLVMBuildNot(builder, bld->exec_mask.exec_mask, "kilp");
      mask = LLVMBuildOr(builder, mask, invmask, "");
   }

   lp_build_mask_update(bld->mask, mask);
   if (!near_end_of_shader(bld, pc))
      lp_build_mask_check(bld->mask);
}


/**
 * Unconditional fragment kill.
 * The only predication is the execution mask which will apply if
 * we're inside a loop or conditional.
 */
static void
emit_kill(struct lp_build_tgsi_soa_context *bld,
          int pc)
{
   LLVMBuilderRef builder = bld->bld_base.base.gallivm->builder;
   LLVMValueRef mask;

   /* For those channels which are "alive", disable fragment shader
    * execution.
    */
   if (bld->exec_mask.has_mask) {
      mask = LLVMBuildNot(builder, bld->exec_mask.exec_mask, "kilp");
   }
   else {
      LLVMValueRef zero = LLVMConstNull(bld->bld_base.base.int_vec_type);
      mask = zero;
   }

   lp_build_mask_update(bld->mask, mask);

   if (!near_end_of_shader(bld, pc))
      lp_build_mask_check(bld->mask);
}


/**
 * Emit code which will dump the value of all the temporary registers
 * to stdout.
 */
static void
emit_dump_file(struct lp_build_tgsi_soa_context *bld,
               unsigned file)
{
   const struct tgsi_shader_info *info = bld->bld_base.info;
   struct gallivm_state *gallivm = bld->bld_base.base.gallivm;
   LLVMBuilderRef builder = gallivm->builder;
   LLVMValueRef reg_ptr;
   int index;
   int max_index = info->file_max[file];

   /*
    * Some register files, particularly constants, can be very large,
    * and dumping everything could make this unusably slow.
    */
   max_index = MIN2(max_index, 32);

   for (index = 0; index <= max_index; index++) {
      LLVMValueRef res;
      unsigned mask;
      int chan;

      if (index < 8 * sizeof(unsigned) &&
          (info->file_mask[file] & (1 << index)) == 0)  {
         /* This was not declared.*/
         continue;
      }

      if (file == TGSI_FILE_INPUT) {
         mask = info->input_usage_mask[index];
      } else {
         mask = TGSI_WRITEMASK_XYZW;
      }

      for (chan = 0; chan < 4; chan++) {
         if ((mask & (1 << chan)) == 0) {
            /* This channel is not used.*/
            continue;
         }

         if (file == TGSI_FILE_CONSTANT) {
            struct tgsi_full_src_register reg;
            memset(&reg, 0, sizeof reg);
            reg.Register.File = file;
            reg.Register.Index = index;
            reg.Register.SwizzleX = 0;
            reg.Register.SwizzleY = 1;
            reg.Register.SwizzleZ = 2;
            reg.Register.SwizzleW = 3;

            res = bld->bld_base.emit_fetch_funcs[file](&bld->bld_base, &reg, TGSI_TYPE_FLOAT, chan);
            if (!res) {
               continue;
            }
         } else if (file == TGSI_FILE_INPUT) {
            res = bld->inputs[index][chan];
            if (!res) {
               continue;
            }
         } else if (file == TGSI_FILE_TEMPORARY) {
            reg_ptr = lp_get_temp_ptr_soa(bld, index, chan);
            assert(reg_ptr);
            res = LLVMBuildLoad(builder, reg_ptr, "");
         } else if (file == TGSI_FILE_OUTPUT) {
            reg_ptr = lp_get_output_ptr(bld, index, chan);
            assert(reg_ptr);
            res = LLVMBuildLoad(builder, reg_ptr, "");
         } else {
            assert(0);
            continue;
         }

         emit_dump_reg(gallivm, file, index, chan, res);
      }
   }
}



void
lp_emit_declaration_soa(
   struct lp_build_tgsi_context *bld_base,
   const struct tgsi_full_declaration *decl)
{
   struct lp_build_tgsi_soa_context *bld = lp_soa_context(bld_base);
   struct gallivm_state *gallivm = bld->bld_base.base.gallivm;
   LLVMTypeRef vec_type = bld->bld_base.base.vec_type;
   const unsigned first = decl->Range.First;
   const unsigned last = decl->Range.Last;
   unsigned idx, i;

   for (idx = first; idx <= last; ++idx) {
      assert(last <= bld->bld_base.info->file_max[decl->Declaration.File]);
      switch (decl->Declaration.File) {
      case TGSI_FILE_TEMPORARY:
         if (!(bld->indirect_files & (1 << TGSI_FILE_TEMPORARY))) {
            assert(idx < LP_MAX_INLINED_TEMPS);
            for (i = 0; i < TGSI_NUM_CHANNELS; i++)
               bld->temps[idx][i] = lp_build_alloca(gallivm, vec_type, "temp");
         }
         break;

      case TGSI_FILE_OUTPUT:
         if (!(bld->indirect_files & (1 << TGSI_FILE_OUTPUT))) {
            for (i = 0; i < TGSI_NUM_CHANNELS; i++)
               bld->outputs[idx][i] = lp_build_alloca(gallivm,
                                                      vec_type, "output");
         }
         break;

      case TGSI_FILE_ADDRESS:
	 /* ADDR registers are only allocated with an integer LLVM IR type,
	  * as they are guaranteed to always have integers.
	  * XXX: Not sure if this exception is worthwhile (or the whole idea of
	  * an ADDR register for that matter).
	  */
         assert(idx < LP_MAX_TGSI_ADDRS);
         for (i = 0; i < TGSI_NUM_CHANNELS; i++)
            bld->addr[idx][i] = lp_build_alloca(gallivm, bld_base->base.int_vec_type, "addr");
         break;

      case TGSI_FILE_PREDICATE:
         assert(idx < LP_MAX_TGSI_PREDS);
         for (i = 0; i < TGSI_NUM_CHANNELS; i++)
            bld->preds[idx][i] = lp_build_alloca(gallivm, vec_type,
                                                 "predicate");
         break;

      case TGSI_FILE_SAMPLER_VIEW:
         /*
          * The target stored here MUST match whatever there actually
          * is in the set sampler views (what about return type?).
          */
         assert(idx < PIPE_MAX_SHADER_SAMPLER_VIEWS);
         bld->sv[idx] = decl->SamplerView;
         break;

      default:
         /* don't need to declare other vars */
         break;
      }
   }
}


void lp_emit_immediate_soa(
   struct lp_build_tgsi_context *bld_base,
   const struct tgsi_full_immediate *imm)
{
   struct lp_build_tgsi_soa_context *bld = lp_soa_context(bld_base);
   struct gallivm_state * gallivm = bld_base->base.gallivm;
   LLVMValueRef imms[4];
   unsigned i;
   const uint size = imm->Immediate.NrTokens - 1;
   assert(size <= 4);
   switch (imm->Immediate.DataType) {
   case TGSI_IMM_FLOAT32:
      for( i = 0; i < size; ++i )
         imms[i] =
               lp_build_const_vec(gallivm, bld_base->base.type, imm->u[i].Float);

      break;
   case TGSI_IMM_UINT32:
      for( i = 0; i < size; ++i ) {
         LLVMValueRef tmp = lp_build_const_vec(gallivm, bld_base->uint_bld.type, imm->u[i].Uint);
         imms[i] = LLVMConstBitCast(tmp, bld_base->base.vec_type);
      }

      break;
   case TGSI_IMM_INT32:
      for( i = 0; i < size; ++i ) {
         LLVMValueRef tmp = lp_build_const_vec(gallivm, bld_base->int_bld.type, imm->u[i].Int);
         imms[i] = LLVMConstBitCast(tmp, bld_base->base.vec_type);
      }

      break;
   }
   for( i = size; i < 4; ++i )
      imms[i] = bld_base->base.undef;

   if (bld->use_immediates_array) {
      unsigned index = bld->num_immediates;
      struct gallivm_state *gallivm = bld->bld_base.base.gallivm;
      LLVMBuilderRef builder = gallivm->builder;

      assert(bld->indirect_files & (1 << TGSI_FILE_IMMEDIATE));
      for (i = 0; i < 4; ++i ) {
         LLVMValueRef lindex = lp_build_const_int32(
                  bld->bld_base.base.gallivm, index * 4 + i);
         LLVMValueRef imm_ptr = LLVMBuildGEP(builder,
                                             bld->imms_array, &lindex, 1, "");
         LLVMBuildStore(builder, imms[i], imm_ptr);
      }
   } else {
      /* simply copy the immediate values into the next immediates[] slot */
      unsigned i;
      const uint size = imm->Immediate.NrTokens - 1;
      assert(size <= 4);
      assert(bld->num_immediates < LP_MAX_INLINED_IMMEDIATES);

      for(i = 0; i < 4; ++i )
         bld->immediates[bld->num_immediates][i] = imms[i];

      if (bld->indirect_files & (1 << TGSI_FILE_IMMEDIATE)) {
         unsigned index = bld->num_immediates;
         struct gallivm_state *gallivm = bld->bld_base.base.gallivm;
         LLVMBuilderRef builder = gallivm->builder;
         for (i = 0; i < 4; ++i ) {
            LLVMValueRef lindex = lp_build_const_int32(
                     bld->bld_base.base.gallivm, index * 4 + i);
            LLVMValueRef imm_ptr = LLVMBuildGEP(builder,
                                                bld->imms_array, &lindex, 1, "");
            LLVMBuildStore(builder,
                           bld->immediates[index][i],
                           imm_ptr);
         }
      }
   }

   bld->num_immediates++;
}

static void
ddx_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   emit_fetch_deriv(bld, emit_data->args[0], NULL,
                    &emit_data->output[emit_data->chan], NULL);
}

static void
ddy_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   emit_fetch_deriv(bld, emit_data->args[0], NULL, NULL,
                    &emit_data->output[emit_data->chan]);
}

static void
kill_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   emit_kill(bld, bld_base->pc - 1);
}

static void
kill_if_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   emit_kill_if(bld, emit_data->inst, bld_base->pc - 1);
}

static void
tex_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   emit_tex(bld, emit_data->inst, LP_BLD_TEX_MODIFIER_NONE, emit_data->output);
}

static void
txb_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   emit_tex(bld, emit_data->inst, LP_BLD_TEX_MODIFIER_LOD_BIAS,
            emit_data->output);
}

static void
txd_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   emit_tex(bld, emit_data->inst, LP_BLD_TEX_MODIFIER_EXPLICIT_DERIV,
            emit_data->output);
}

static void
txl_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   emit_tex(bld, emit_data->inst, LP_BLD_TEX_MODIFIER_EXPLICIT_LOD,
            emit_data->output);
}

static void
txp_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   emit_tex(bld, emit_data->inst, LP_BLD_TEX_MODIFIER_PROJECTED,
            emit_data->output);
}

static void
txq_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   emit_size_query(bld, emit_data->inst, emit_data->output, FALSE);
}

static void
txf_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   emit_fetch_texels(bld, emit_data->inst, emit_data->output, FALSE);
}

static void
sample_i_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   emit_fetch_texels(bld, emit_data->inst, emit_data->output, TRUE);
}

static void
sample_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   emit_sample(bld, emit_data->inst, LP_BLD_TEX_MODIFIER_NONE,
               FALSE, emit_data->output);
}

static void
sample_b_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   emit_sample(bld, emit_data->inst, LP_BLD_TEX_MODIFIER_LOD_BIAS,
               FALSE, emit_data->output);
}

static void
sample_c_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   emit_sample(bld, emit_data->inst, LP_BLD_TEX_MODIFIER_NONE,
               TRUE, emit_data->output);
}

static void
sample_c_lz_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   emit_sample(bld, emit_data->inst, LP_BLD_TEX_MODIFIER_LOD_ZERO,
               TRUE, emit_data->output);
}

static void
sample_d_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   emit_sample(bld, emit_data->inst, LP_BLD_TEX_MODIFIER_EXPLICIT_DERIV,
               FALSE, emit_data->output);
}

static void
sample_l_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   emit_sample(bld, emit_data->inst, LP_BLD_TEX_MODIFIER_EXPLICIT_LOD,
               FALSE, emit_data->output);
}

static void
sviewinfo_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   emit_size_query(bld, emit_data->inst, emit_data->output, TRUE);
}

static LLVMValueRef
mask_vec(struct lp_build_tgsi_context *bld_base)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);
   LLVMBuilderRef builder = bld->bld_base.base.gallivm->builder;
   struct lp_exec_mask *exec_mask = &bld->exec_mask;

   if (!exec_mask->has_mask) {
      return lp_build_mask_value(bld->mask);
   }
   return LLVMBuildAnd(builder, lp_build_mask_value(bld->mask),
                       exec_mask->exec_mask, "");
}

static void
increment_vec_ptr_by_mask(struct lp_build_tgsi_context * bld_base,
                          LLVMValueRef ptr,
                          LLVMValueRef mask)
{
   LLVMBuilderRef builder = bld_base->base.gallivm->builder;
   LLVMValueRef current_vec = LLVMBuildLoad(builder, ptr, "");

   current_vec = LLVMBuildSub(builder, current_vec, mask, "");

   LLVMBuildStore(builder, current_vec, ptr);
}

static void
clear_uint_vec_ptr_from_mask(struct lp_build_tgsi_context * bld_base,
                             LLVMValueRef ptr,
                             LLVMValueRef mask)
{
   LLVMBuilderRef builder = bld_base->base.gallivm->builder;
   LLVMValueRef current_vec = LLVMBuildLoad(builder, ptr, "");

   current_vec = lp_build_select(&bld_base->uint_bld,
                                 mask,
                                 bld_base->uint_bld.zero,
                                 current_vec);

   LLVMBuildStore(builder, current_vec, ptr);
}

static LLVMValueRef
clamp_mask_to_max_output_vertices(struct lp_build_tgsi_soa_context * bld,
                                  LLVMValueRef current_mask_vec,
                                  LLVMValueRef total_emitted_vertices_vec)
{
   LLVMBuilderRef builder = bld->bld_base.base.gallivm->builder;
   struct lp_build_context *int_bld = &bld->bld_base.int_bld;
   LLVMValueRef max_mask = lp_build_cmp(int_bld, PIPE_FUNC_LESS,
                                        total_emitted_vertices_vec,
                                        bld->max_output_vertices_vec);

   return LLVMBuildAnd(builder, current_mask_vec, max_mask, "");
}

static void
emit_vertex(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);
   LLVMBuilderRef builder = bld->bld_base.base.gallivm->builder;

   if (bld->gs_iface->emit_vertex) {
      LLVMValueRef mask = mask_vec(bld_base);
      LLVMValueRef total_emitted_vertices_vec =
         LLVMBuildLoad(builder, bld->total_emitted_vertices_vec_ptr, "");
      mask = clamp_mask_to_max_output_vertices(bld, mask,
                                               total_emitted_vertices_vec);
      gather_outputs(bld);
      bld->gs_iface->emit_vertex(bld->gs_iface, &bld->bld_base,
                                 bld->outputs,
                                 total_emitted_vertices_vec);
      increment_vec_ptr_by_mask(bld_base, bld->emitted_vertices_vec_ptr,
                                mask);
      increment_vec_ptr_by_mask(bld_base, bld->total_emitted_vertices_vec_ptr,
                                mask);
#if DUMP_GS_EMITS
      lp_build_print_value(bld->bld_base.base.gallivm,
                           " +++ emit vertex masked ones = ",
                           mask);
      lp_build_print_value(bld->bld_base.base.gallivm,
                           " +++ emit vertex emitted = ",
                           total_emitted_vertices_vec);
#endif
   }
}


static void
end_primitive_masked(struct lp_build_tgsi_context * bld_base,
                     LLVMValueRef mask)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);
   LLVMBuilderRef builder = bld->bld_base.base.gallivm->builder;

   if (bld->gs_iface->end_primitive) {
      struct lp_build_context *uint_bld = &bld_base->uint_bld;
      LLVMValueRef emitted_vertices_vec =
         LLVMBuildLoad(builder, bld->emitted_vertices_vec_ptr, "");
      LLVMValueRef emitted_prims_vec =
         LLVMBuildLoad(builder, bld->emitted_prims_vec_ptr, "");

      LLVMValueRef emitted_mask = lp_build_cmp(uint_bld, PIPE_FUNC_NOTEQUAL,
                                               emitted_vertices_vec,
                                               uint_bld->zero);
      /* We need to combine the current execution mask with the mask
         telling us which, if any, execution slots actually have
         unemitted primitives, this way we make sure that end_primitives
         executes only on the paths that have unflushed vertices */
      mask = LLVMBuildAnd(builder, mask, emitted_mask, "");

      bld->gs_iface->end_primitive(bld->gs_iface, &bld->bld_base,
                                   emitted_vertices_vec,
                                   emitted_prims_vec);

#if DUMP_GS_EMITS
      lp_build_print_value(bld->bld_base.base.gallivm,
                           " +++ end prim masked ones = ",
                           mask);
      lp_build_print_value(bld->bld_base.base.gallivm,
                           " +++ end prim emitted verts1 = ",
                           emitted_vertices_vec);
      lp_build_print_value(bld->bld_base.base.gallivm,
                           " +++ end prim emitted prims1 = ",
                           LLVMBuildLoad(builder,
                                         bld->emitted_prims_vec_ptr, ""));
#endif
      increment_vec_ptr_by_mask(bld_base, bld->emitted_prims_vec_ptr,
                                mask);
      clear_uint_vec_ptr_from_mask(bld_base, bld->emitted_vertices_vec_ptr,
                                   mask);
#if DUMP_GS_EMITS
      lp_build_print_value(bld->bld_base.base.gallivm,
                           " +++ end prim emitted verts2 = ",
                           LLVMBuildLoad(builder,
                                         bld->emitted_vertices_vec_ptr, ""));
#endif
   }

}

static void
end_primitive(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   if (bld->gs_iface->end_primitive) {
      LLVMValueRef mask = mask_vec(bld_base);
      end_primitive_masked(bld_base, mask);
   }
}

static void
cal_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   lp_exec_mask_call(&bld->exec_mask, emit_data->inst->Label.Label,
                     &bld_base->pc);
}

static void
ret_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   lp_exec_mask_ret(&bld->exec_mask, &bld_base->pc);
}

static void
brk_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   lp_exec_break(&bld->exec_mask, bld_base);
}

static void
breakc_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);
   LLVMBuilderRef builder = bld_base->base.gallivm->builder;
   struct lp_build_context *uint_bld = &bld_base->uint_bld;
   LLVMValueRef unsigned_cond = 
      LLVMBuildBitCast(builder, emit_data->args[0], uint_bld->vec_type, "");
   LLVMValueRef cond = lp_build_cmp(uint_bld, PIPE_FUNC_NOTEQUAL,
                                    unsigned_cond,
                                    uint_bld->zero);

   lp_exec_break_condition(&bld->exec_mask, cond);
}

static void
if_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   LLVMValueRef tmp;
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   tmp = lp_build_cmp(&bld_base->base, PIPE_FUNC_NOTEQUAL,
                      emit_data->args[0], bld->bld_base.base.zero);
   lp_exec_mask_cond_push(&bld->exec_mask, tmp);
}

static void
uif_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   LLVMValueRef tmp;
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);
   struct lp_build_context *uint_bld = &bld_base->uint_bld;

   tmp = lp_build_cmp(uint_bld, PIPE_FUNC_NOTEQUAL,
                      emit_data->args[0], uint_bld->zero);
   lp_exec_mask_cond_push(&bld->exec_mask, tmp);
}

static void
case_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   lp_exec_case(&bld->exec_mask, emit_data->args[0]);
}

static void
default_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   lp_exec_default(&bld->exec_mask, bld_base);
}

static void
switch_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   lp_exec_switch(&bld->exec_mask, emit_data->args[0]);
}

static void
endswitch_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   lp_exec_endswitch(&bld->exec_mask, bld_base);
}

static void
bgnloop_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   lp_exec_bgnloop(&bld->exec_mask);
}

static void
bgnsub_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   lp_exec_mask_bgnsub(&bld->exec_mask);
}

static void
else_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   lp_exec_mask_cond_invert(&bld->exec_mask);
}

static void
endif_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   lp_exec_mask_cond_pop(&bld->exec_mask);
}

static void
endloop_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   lp_exec_endloop(bld_base->base.gallivm, &bld->exec_mask);
}

static void
endsub_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   lp_exec_mask_endsub(&bld->exec_mask, &bld_base->pc);
}

static void
cont_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   lp_exec_continue(&bld->exec_mask);
}

/* XXX: Refactor and move it to lp_bld_tgsi_action.c
 *
 * XXX: What do the comments about xmm registers mean?  Maybe they are left over
 * from old code, but there is no garauntee that LLVM will use those registers
 * for this code.
 *
 * XXX: There should be no calls to lp_build_emit_fetch in this function.  This
 * should be handled by the emit_data->fetch_args function. */
static void
nrm_emit(
   const struct lp_build_tgsi_action * action,
   struct lp_build_tgsi_context * bld_base,
   struct lp_build_emit_data * emit_data)
{
   LLVMValueRef tmp0, tmp1;
   LLVMValueRef tmp4 = NULL;
   LLVMValueRef tmp5 = NULL;
   LLVMValueRef tmp6 = NULL;
   LLVMValueRef tmp7 = NULL;
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);

   uint dims = (emit_data->inst->Instruction.Opcode == TGSI_OPCODE_NRM) ? 3 : 4;

  if (TGSI_IS_DST0_CHANNEL_ENABLED(emit_data->inst, TGSI_CHAN_X) ||
      TGSI_IS_DST0_CHANNEL_ENABLED(emit_data->inst, TGSI_CHAN_Y) ||
      TGSI_IS_DST0_CHANNEL_ENABLED(emit_data->inst, TGSI_CHAN_Z) ||
      (TGSI_IS_DST0_CHANNEL_ENABLED(emit_data->inst, TGSI_CHAN_W) && dims == 4)) {

      /* NOTE: Cannot use xmm regs 2/3 here (see emit_rsqrt() above). */

      /* xmm4 = src.x */
      /* xmm0 = src.x * src.x */
      tmp0 = lp_build_emit_fetch(&bld->bld_base, emit_data->inst, 0, TGSI_CHAN_X);
      if (TGSI_IS_DST0_CHANNEL_ENABLED(emit_data->inst, TGSI_CHAN_X)) {
         tmp4 = tmp0;
      }
      tmp0 = lp_build_mul( &bld->bld_base.base, tmp0, tmp0);

      /* xmm5 = src.y */
      /* xmm0 = xmm0 + src.y * src.y */
      tmp1 = lp_build_emit_fetch(&bld->bld_base, emit_data->inst, 0, TGSI_CHAN_Y);
      if (TGSI_IS_DST0_CHANNEL_ENABLED(emit_data->inst, TGSI_CHAN_Y)) {
         tmp5 = tmp1;
      }
      tmp1 = lp_build_mul( &bld->bld_base.base, tmp1, tmp1);
      tmp0 = lp_build_add( &bld->bld_base.base, tmp0, tmp1);

      /* xmm6 = src.z */
      /* xmm0 = xmm0 + src.z * src.z */
      tmp1 = lp_build_emit_fetch(&bld->bld_base, emit_data->inst, 0, TGSI_CHAN_Z);
      if (TGSI_IS_DST0_CHANNEL_ENABLED(emit_data->inst, TGSI_CHAN_Z)) {
         tmp6 = tmp1;
      }
      tmp1 = lp_build_mul( &bld->bld_base.base, tmp1, tmp1);
      tmp0 = lp_build_add( &bld->bld_base.base, tmp0, tmp1);

      if (dims == 4) {
         /* xmm7 = src.w */
         /* xmm0 = xmm0 + src.w * src.w */
         tmp1 = lp_build_emit_fetch(&bld->bld_base, emit_data->inst, 0, TGSI_CHAN_W);
         if (TGSI_IS_DST0_CHANNEL_ENABLED(emit_data->inst, TGSI_CHAN_W)) {
            tmp7 = tmp1;
         }
         tmp1 = lp_build_mul( &bld->bld_base.base, tmp1, tmp1);
         tmp0 = lp_build_add( &bld->bld_base.base, tmp0, tmp1);
      }
      /* xmm1 = 1 / sqrt(xmm0) */
      tmp1 = lp_build_rsqrt( &bld->bld_base.base, tmp0);
       /* dst.x = xmm1 * src.x */
      if (TGSI_IS_DST0_CHANNEL_ENABLED(emit_data->inst, TGSI_CHAN_X)) {
         emit_data->output[TGSI_CHAN_X] = lp_build_mul( &bld->bld_base.base, tmp4, tmp1);
      }
      /* dst.y = xmm1 * src.y */
      if (TGSI_IS_DST0_CHANNEL_ENABLED(emit_data->inst, TGSI_CHAN_Y)) {
         emit_data->output[TGSI_CHAN_Y] = lp_build_mul( &bld->bld_base.base, tmp5, tmp1);
      }

      /* dst.z = xmm1 * src.z */
      if (TGSI_IS_DST0_CHANNEL_ENABLED(emit_data->inst, TGSI_CHAN_Z)) {
         emit_data->output[TGSI_CHAN_Z] = lp_build_mul( &bld->bld_base.base, tmp6, tmp1);
      }
      /* dst.w = xmm1 * src.w */
      if (TGSI_IS_DST0_CHANNEL_ENABLED(emit_data->inst, TGSI_CHAN_X) && dims == 4) {
         emit_data->output[TGSI_CHAN_W] = lp_build_mul( &bld->bld_base.base, tmp7, tmp1);
      }
   }

   /* dst.w = 1.0 */
   if (TGSI_IS_DST0_CHANNEL_ENABLED(emit_data->inst, TGSI_CHAN_W) && dims == 3) {
       emit_data->output[TGSI_CHAN_W] = bld->bld_base.base.one;
   }
}

static void emit_prologue(struct lp_build_tgsi_context * bld_base)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);
   struct gallivm_state * gallivm = bld_base->base.gallivm;

   if (bld->indirect_files & (1 << TGSI_FILE_TEMPORARY)) {
      LLVMValueRef array_size =
         lp_build_const_int32(gallivm,
                         bld_base->info->file_max[TGSI_FILE_TEMPORARY] * 4 + 4);
      bld->temps_array = lp_build_array_alloca(gallivm,
                                              bld_base->base.vec_type, array_size,
                                              "temp_array");
   }

   if (bld->indirect_files & (1 << TGSI_FILE_OUTPUT)) {
      LLVMValueRef array_size =
         lp_build_const_int32(gallivm,
                            bld_base->info->file_max[TGSI_FILE_OUTPUT] * 4 + 4);
      bld->outputs_array = lp_build_array_alloca(gallivm,
                                                bld_base->base.vec_type, array_size,
                                                "output_array");
   }

   if (bld->indirect_files & (1 << TGSI_FILE_IMMEDIATE)) {
      LLVMValueRef array_size =
         lp_build_const_int32(gallivm,
                         bld_base->info->file_max[TGSI_FILE_IMMEDIATE] * 4 + 4);
      bld->imms_array = lp_build_array_alloca(gallivm,
                                              bld_base->base.vec_type, array_size,
                                              "imms_array");
   }

   /* If we have indirect addressing in inputs we need to copy them into
    * our alloca array to be able to iterate over them */
   if (bld->indirect_files & (1 << TGSI_FILE_INPUT) && !bld->gs_iface) {
      unsigned index, chan;
      LLVMTypeRef vec_type = bld_base->base.vec_type;
      LLVMValueRef array_size = lp_build_const_int32(gallivm,
            bld_base->info->file_max[TGSI_FILE_INPUT]*4 + 4);
      bld->inputs_array = lp_build_array_alloca(gallivm,
                                               vec_type, array_size,
                                               "input_array");

      assert(bld_base->info->num_inputs
                        <= bld_base->info->file_max[TGSI_FILE_INPUT] + 1);

      for (index = 0; index < bld_base->info->num_inputs; ++index) {
         for (chan = 0; chan < TGSI_NUM_CHANNELS; ++chan) {
            LLVMValueRef lindex =
               lp_build_const_int32(gallivm, index * 4 + chan);
            LLVMValueRef input_ptr =
               LLVMBuildGEP(gallivm->builder, bld->inputs_array,
                            &lindex, 1, "");
            LLVMValueRef value = bld->inputs[index][chan];
            if (value)
               LLVMBuildStore(gallivm->builder, value, input_ptr);
         }
      }
   }

   if (bld->gs_iface) {
      struct lp_build_context *uint_bld = &bld->bld_base.uint_bld;
      bld->emitted_prims_vec_ptr =
         lp_build_alloca(gallivm,
                         uint_bld->vec_type,
                         "emitted_prims_ptr");
      bld->emitted_vertices_vec_ptr =
         lp_build_alloca(gallivm,
                         uint_bld->vec_type,
                         "emitted_vertices_ptr");
      bld->total_emitted_vertices_vec_ptr =
         lp_build_alloca(gallivm,
                         uint_bld->vec_type,
                         "total_emitted_vertices_ptr");

      LLVMBuildStore(gallivm->builder, uint_bld->zero,
                     bld->emitted_prims_vec_ptr);
      LLVMBuildStore(gallivm->builder, uint_bld->zero,
                     bld->emitted_vertices_vec_ptr);
      LLVMBuildStore(gallivm->builder, uint_bld->zero,
                     bld->total_emitted_vertices_vec_ptr);
   }

   if (DEBUG_EXECUTION) {
      lp_build_printf(gallivm, "\n");
      emit_dump_file(bld, TGSI_FILE_CONSTANT);
      if (!bld->gs_iface)
         emit_dump_file(bld, TGSI_FILE_INPUT);
   }
}

static void emit_epilogue(struct lp_build_tgsi_context * bld_base)
{
   struct lp_build_tgsi_soa_context * bld = lp_soa_context(bld_base);
   LLVMBuilderRef builder = bld_base->base.gallivm->builder;

   if (DEBUG_EXECUTION) {
      /* for debugging */
      if (0) {
         emit_dump_file(bld, TGSI_FILE_TEMPORARY);
      }
      emit_dump_file(bld, TGSI_FILE_OUTPUT);
      lp_build_printf(bld_base->base.gallivm, "\n");
   }

   /* If we have indirect addressing in outputs we need to copy our alloca array
    * to the outputs slots specified by the caller */
   if (bld->gs_iface) {
      LLVMValueRef total_emitted_vertices_vec;
      LLVMValueRef emitted_prims_vec;
      /* implicit end_primitives, needed in case there are any unflushed
         vertices in the cache. Note must not call end_primitive here
         since the exec_mask is not valid at this point. */
      end_primitive_masked(bld_base, lp_build_mask_value(bld->mask));
      
      total_emitted_vertices_vec =
         LLVMBuildLoad(builder, bld->total_emitted_vertices_vec_ptr, "");
      emitted_prims_vec =
         LLVMBuildLoad(builder, bld->emitted_prims_vec_ptr, "");

      bld->gs_iface->gs_epilogue(bld->gs_iface,
                                 &bld->bld_base,
                                 total_emitted_vertices_vec,
                                 emitted_prims_vec);
   } else {
      gather_outputs(bld);
   }
}

void
lp_build_tgsi_soa(struct gallivm_state *gallivm,
                  const struct tgsi_token *tokens,
                  struct lp_type type,
                  struct lp_build_mask_context *mask,
                  LLVMValueRef consts_ptr,
                  LLVMValueRef const_sizes_ptr,
                  const struct lp_bld_tgsi_system_values *system_values,
                  const LLVMValueRef (*inputs)[TGSI_NUM_CHANNELS],
                  LLVMValueRef (*outputs)[TGSI_NUM_CHANNELS],
                  struct lp_build_sampler_soa *sampler,
                  const struct tgsi_shader_info *info,
                  const struct lp_build_tgsi_gs_iface *gs_iface)
{
   struct lp_build_tgsi_soa_context bld;

   struct lp_type res_type;

   assert(type.length <= LP_MAX_VECTOR_LENGTH);
   memset(&res_type, 0, sizeof res_type);
   res_type.width = type.width;
   res_type.length = type.length;
   res_type.sign = 1;

   /* Setup build context */
   memset(&bld, 0, sizeof bld);
   lp_build_context_init(&bld.bld_base.base, gallivm, type);
   lp_build_context_init(&bld.bld_base.uint_bld, gallivm, lp_uint_type(type));
   lp_build_context_init(&bld.bld_base.int_bld, gallivm, lp_int_type(type));
   lp_build_context_init(&bld.elem_bld, gallivm, lp_elem_type(type));
   bld.mask = mask;
   bld.inputs = inputs;
   bld.outputs = outputs;
   bld.consts_ptr = consts_ptr;
   bld.const_sizes_ptr = const_sizes_ptr;
   bld.sampler = sampler;
   bld.bld_base.info = info;
   bld.indirect_files = info->indirect_files;

   /*
    * If the number of temporaries is rather large then we just
    * allocate them as an array right from the start and treat
    * like indirect temporaries.
    */
   if (info->file_max[TGSI_FILE_TEMPORARY] >= LP_MAX_INLINED_TEMPS) {
      bld.indirect_files |= (1 << TGSI_FILE_TEMPORARY);
   }
   /*
    * For performance reason immediates are always backed in a static
    * array, but if their number is too great, we have to use just
    * a dynamically allocated array.
    */
   bld.use_immediates_array =
         (info->file_max[TGSI_FILE_IMMEDIATE] >= LP_MAX_INLINED_IMMEDIATES);
   if (bld.use_immediates_array) {
      bld.indirect_files |= (1 << TGSI_FILE_IMMEDIATE);
   }


   bld.bld_base.soa = TRUE;
   bld.bld_base.emit_debug = emit_debug;
   bld.bld_base.emit_fetch_funcs[TGSI_FILE_CONSTANT] = emit_fetch_constant;
   bld.bld_base.emit_fetch_funcs[TGSI_FILE_IMMEDIATE] = emit_fetch_immediate;
   bld.bld_base.emit_fetch_funcs[TGSI_FILE_INPUT] = emit_fetch_input;
   bld.bld_base.emit_fetch_funcs[TGSI_FILE_TEMPORARY] = emit_fetch_temporary;
   bld.bld_base.emit_fetch_funcs[TGSI_FILE_SYSTEM_VALUE] = emit_fetch_system_value;
   bld.bld_base.emit_store = emit_store;

   bld.bld_base.emit_declaration = lp_emit_declaration_soa;
   bld.bld_base.emit_immediate = lp_emit_immediate_soa;

   bld.bld_base.emit_prologue = emit_prologue;
   bld.bld_base.emit_epilogue = emit_epilogue;

   /* Set opcode actions */
   lp_set_default_actions_cpu(&bld.bld_base);

   bld.bld_base.op_actions[TGSI_OPCODE_BGNLOOP].emit = bgnloop_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_BGNSUB].emit = bgnsub_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_BRK].emit = brk_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_BREAKC].emit = breakc_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_CAL].emit = cal_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_CASE].emit = case_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_CONT].emit = cont_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_DDX].emit = ddx_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_DDY].emit = ddy_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_DEFAULT].emit = default_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_ELSE].emit = else_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_ENDIF].emit = endif_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_ENDLOOP].emit = endloop_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_ENDSUB].emit = endsub_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_ENDSWITCH].emit = endswitch_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_IF].emit = if_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_UIF].emit = uif_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_KILL_IF].emit = kill_if_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_KILL].emit = kill_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_NRM].emit = nrm_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_NRM4].emit = nrm_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_RET].emit = ret_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_SWITCH].emit = switch_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_TEX].emit = tex_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_TXB].emit = txb_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_TXD].emit = txd_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_TXL].emit = txl_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_TXP].emit = txp_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_TXQ].emit = txq_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_TXF].emit = txf_emit;
   /* DX10 sampling ops */
   bld.bld_base.op_actions[TGSI_OPCODE_SAMPLE].emit = sample_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_SAMPLE_B].emit = sample_b_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_SAMPLE_C].emit = sample_c_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_SAMPLE_C_LZ].emit = sample_c_lz_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_SAMPLE_D].emit = sample_d_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_SAMPLE_I].emit = sample_i_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_SAMPLE_L].emit = sample_l_emit;
   bld.bld_base.op_actions[TGSI_OPCODE_SVIEWINFO].emit = sviewinfo_emit;

   if (gs_iface) {
      /* There's no specific value for this because it should always
       * be set, but apps using ext_geometry_shader4 quite often
       * were forgetting so we're using MAX_VERTEX_VARYING from
       * that spec even though we could debug_assert if it's not
       * set, but that's a lot uglier. */
      uint max_output_vertices = 32;
      uint i = 0;
      /* inputs are always indirect with gs */
      bld.indirect_files |= (1 << TGSI_FILE_INPUT);
      bld.gs_iface = gs_iface;
      bld.bld_base.emit_fetch_funcs[TGSI_FILE_INPUT] = emit_fetch_gs_input;
      bld.bld_base.op_actions[TGSI_OPCODE_EMIT].emit = emit_vertex;
      bld.bld_base.op_actions[TGSI_OPCODE_ENDPRIM].emit = end_primitive;

      for (i = 0; i < info->num_properties; ++i) {
         if (info->properties[i].name ==
             TGSI_PROPERTY_GS_MAX_OUTPUT_VERTICES) {
            max_output_vertices = info->properties[i].data[0];
         }
      }
      bld.max_output_vertices_vec =
         lp_build_const_int_vec(gallivm, bld.bld_base.int_bld.type,
                                max_output_vertices);
   }

   lp_exec_mask_init(&bld.exec_mask, &bld.bld_base.int_bld);

   bld.system_values = *system_values;

   lp_build_tgsi_llvm(&bld.bld_base, tokens);

   if (0) {
      LLVMBasicBlockRef block = LLVMGetInsertBlock(gallivm->builder);
      LLVMValueRef function = LLVMGetBasicBlockParent(block);
      debug_printf("11111111111111111111111111111 \n");
      tgsi_dump(tokens, 0);
      lp_debug_dump_value(function);
      debug_printf("2222222222222222222222222222 \n");
   }

   if (0) {
      LLVMModuleRef module = LLVMGetGlobalParent(
         LLVMGetBasicBlockParent(LLVMGetInsertBlock(gallivm->builder)));
      LLVMDumpModule(module);

   }
   lp_exec_mask_fini(&bld.exec_mask);
}
