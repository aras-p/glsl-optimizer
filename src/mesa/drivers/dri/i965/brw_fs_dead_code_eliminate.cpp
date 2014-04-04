/*
 * Copyright © 2014 Intel Corporation
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
#include "brw_fs_live_variables.h"
#include "brw_cfg.h"

/** @file brw_fs_dead_code_eliminate.cpp
 *
 * Dataflow-aware dead code elimination.
 *
 * Walks the instruction list from the bottom, removing instructions that
 * have results that both aren't used in later blocks and haven't been read
 * yet in the tail end of this block.
 */

bool
fs_visitor::dead_code_eliminate()
{
   bool progress = false;

   cfg_t cfg(&instructions);

   calculate_live_intervals();

   int num_vars = live_intervals->num_vars;
   BITSET_WORD *live = ralloc_array(NULL, BITSET_WORD, BITSET_WORDS(num_vars));

   for (int b = 0; b < cfg.num_blocks; b++) {
      bblock_t *block = cfg.blocks[b];
      memcpy(live, live_intervals->bd[b].liveout,
             sizeof(BITSET_WORD) * BITSET_WORDS(num_vars));

      for (fs_inst *inst = (fs_inst *)block->end;
           inst != block->start->prev;
           inst = (fs_inst *)inst->prev) {
         if (inst->dst.file == GRF &&
             !inst->has_side_effects() &&
             !inst->writes_flag()) {
            bool result_live = false;

            if (inst->regs_written == 1) {
               int var = live_intervals->var_from_reg(&inst->dst);
               result_live = BITSET_TEST(live, var);
            } else {
               int var = live_intervals->var_from_vgrf[inst->dst.reg];
               for (int i = 0; i < inst->regs_written; i++) {
                  result_live = result_live || BITSET_TEST(live, var + i);
               }
            }

            if (!result_live) {
               progress = true;

               if (inst->writes_accumulator) {
                  inst->dst = fs_reg(retype(brw_null_reg(), inst->dst.type));
               } else {
                  inst->opcode = BRW_OPCODE_NOP;
                  continue;
               }
            }
         }

         if (inst->dst.file == GRF) {
            if (!inst->is_partial_write()) {
               int var = live_intervals->var_from_vgrf[inst->dst.reg];
               for (int i = 0; i < inst->regs_written; i++) {
                  BITSET_CLEAR(live, var + inst->dst.reg_offset + i);
               }
            }
         }

         for (int i = 0; i < 3; i++) {
            if (inst->src[i].file == GRF) {
               int var = live_intervals->var_from_vgrf[inst->src[i].reg];

               for (int j = 0; j < inst->regs_read(this, i); j++) {
                  BITSET_SET(live, var + inst->src[i].reg_offset + j);
               }
            }
         }
      }
   }

   ralloc_free(live);

   if (progress) {
      foreach_list_safe(node, &this->instructions) {
         fs_inst *inst = (fs_inst *)node;

         if (inst->opcode == BRW_OPCODE_NOP) {
            inst->remove();
         }
      }

      invalidate_live_intervals();
   }

   return progress;
}
