/*
 * Copyright © 2013 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "brw_fs.h"
#include "brw_cfg.h"

/** @file brw_fs_sel_peephole.cpp
 *
 * This file contains the opt_peephole_sel() optimization pass that replaces
 * MOV instructions to the same destination in the "then" and "else" bodies of
 * an if statement with SEL instructions.
 */

/* Four MOVs seems to be pretty typical, so I picked the next power of two in
 * the hopes that it would handle almost anything possible in a single
 * pass.
 */
#define MAX_MOVS 8 /**< The maximum number of MOVs to attempt to match. */

/**
 * Scans forwards from an IF counting consecutive MOV instructions in the
 * "then" and "else" blocks of the if statement.
 *
 * A pointer to the fs_inst* for IF is passed as the <if_inst> argument. The
 * function stores pointers to the MOV instructions in the <then_mov> and
 * <else_mov> arrays.
 *
 * \return the minimum number of MOVs found in the two branches or zero if
 *         an error occurred.
 *
 * E.g.:
 *                  IF ...
 *    then_mov[0] = MOV g4, ...
 *    then_mov[1] = MOV g5, ...
 *    then_mov[2] = MOV g6, ...
 *                  ELSE ...
 *    else_mov[0] = MOV g4, ...
 *    else_mov[1] = MOV g5, ...
 *    else_mov[2] = MOV g7, ...
 *                  ENDIF
 *    returns 3.
 */
static int
count_movs_from_if(fs_inst *then_mov[MAX_MOVS], fs_inst *else_mov[MAX_MOVS],
                   fs_inst *if_inst, fs_inst *else_inst)
{
   fs_inst *m = if_inst;

   assert(m->opcode == BRW_OPCODE_IF);
   m = (fs_inst *) m->next;

   int then_movs = 0;
   while (then_movs < MAX_MOVS && m->opcode == BRW_OPCODE_MOV) {
      then_mov[then_movs] = m;
      m = (fs_inst *) m->next;
      then_movs++;
   }

   m = (fs_inst *) else_inst->next;

   int else_movs = 0;
   while (else_movs < MAX_MOVS && m->opcode == BRW_OPCODE_MOV) {
      else_mov[else_movs] = m;
      m = (fs_inst *) m->next;
      else_movs++;
   }

   return MIN2(then_movs, else_movs);
}

/**
 * Try to replace IF/MOV+/ELSE/MOV+/ENDIF with SEL.
 *
 * Many GLSL shaders contain the following pattern:
 *
 *    x = condition ? foo : bar
 *
 * or
 *
 *    if (...) a.xyzw = foo.xyzw;
 *    else     a.xyzw = bar.xyzw;
 *
 * The compiler emits an ir_if tree for this, since each subexpression might be
 * a complex tree that could have side-effects or short-circuit logic.
 *
 * However, the common case is to simply select one of two constants or
 * variable values---which is exactly what SEL is for.  In this case, the
 * assembly looks like:
 *
 *    (+f0) IF
 *    MOV dst src0
 *    ...
 *    ELSE
 *    MOV dst src1
 *    ...
 *    ENDIF
 *
 * where each pair of MOVs to a common destination and can be easily translated
 * into
 *
 *    (+f0) SEL dst src0 src1
 *
 * If src0 is an immediate value, we promote it to a temporary GRF.
 */
bool
fs_visitor::opt_peephole_sel()
{
   bool progress = false;

   cfg_t cfg(&instructions);

   for (int b = 0; b < cfg.num_blocks; b++) {
      bblock_t *block = cfg.blocks[b];

      /* IF instructions, by definition, can only be found at the ends of
       * basic blocks.
       */
      fs_inst *if_inst = (fs_inst *) block->end;
      if (if_inst->opcode != BRW_OPCODE_IF)
         continue;

      if (!block->else_inst)
         continue;

      fs_inst *else_inst = (fs_inst *) block->else_inst;
      assert(else_inst->opcode == BRW_OPCODE_ELSE);

      fs_inst *else_mov[MAX_MOVS] = { NULL };
      fs_inst *then_mov[MAX_MOVS] = { NULL };

      int movs = count_movs_from_if(then_mov, else_mov, if_inst, else_inst);

      if (movs == 0)
         continue;

      fs_inst *sel_inst[MAX_MOVS] = { NULL };
      fs_inst *mov_imm_inst[MAX_MOVS] = { NULL };

      /* Generate SEL instructions for pairs of MOVs to a common destination. */
      for (int i = 0; i < movs; i++) {
         if (!then_mov[i] || !else_mov[i])
            break;

         /* Check that the MOVs are the right form. */
         if (!then_mov[i]->dst.equals(else_mov[i]->dst) ||
             then_mov[i]->is_partial_write() ||
             else_mov[i]->is_partial_write()) {
            movs = i;
            break;
         }

         /* Check that source types for mov operations match. */
         if (then_mov[i]->src[0].type != else_mov[i]->src[0].type) {
            movs = i;
            break;
         }

         if (!then_mov[i]->src[0].equals(else_mov[i]->src[0])) {
            /* Only the last source register can be a constant, so if the MOV
             * in the "then" clause uses a constant, we need to put it in a
             * temporary.
             */
            fs_reg src0(then_mov[i]->src[0]);
            if (src0.file == IMM) {
               src0 = fs_reg(this, glsl_type::float_type);
               src0.type = then_mov[i]->src[0].type;
               mov_imm_inst[i] = MOV(src0, then_mov[i]->src[0]);
            }

            sel_inst[i] = SEL(then_mov[i]->dst, src0, else_mov[i]->src[0]);

            if (brw->gen == 6 && if_inst->conditional_mod) {
               /* For Sandybridge with IF with embedded comparison */
               sel_inst[i]->predicate = BRW_PREDICATE_NORMAL;
            } else {
               /* Separate CMP and IF instructions */
               sel_inst[i]->predicate = if_inst->predicate;
               sel_inst[i]->predicate_inverse = if_inst->predicate_inverse;
            }
         } else {
            sel_inst[i] = MOV(then_mov[i]->dst, then_mov[i]->src[0]);
         }
      }

      if (movs == 0)
         continue;

      /* Emit a CMP if our IF used the embedded comparison */
      if (brw->gen == 6 && if_inst->conditional_mod) {
         fs_inst *cmp_inst = CMP(reg_null_d, if_inst->src[0], if_inst->src[1],
                                 if_inst->conditional_mod);
         if_inst->insert_before(cmp_inst);
      }

      for (int i = 0; i < movs; i++) {
         if (mov_imm_inst[i])
            if_inst->insert_before(mov_imm_inst[i]);
         if_inst->insert_before(sel_inst[i]);

         then_mov[i]->remove();
         else_mov[i]->remove();
      }

      progress = true;
   }

   if (progress)
      invalidate_live_intervals();

   return progress;
}
