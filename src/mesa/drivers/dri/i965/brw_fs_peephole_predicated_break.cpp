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

/** @file brw_fs_peephole_predicated_break.cpp
 *
 * Loops are often structured as
 *
 * loop:
 *    CMP.f0
 *    (+f0) IF
 *    BREAK
 *    ENDIF
 *    ...
 *    WHILE loop
 *
 * This peephole pass removes the IF and ENDIF instructions and predicates the
 * BREAK, dropping two instructions from the loop body.
 */

bool
fs_visitor::opt_peephole_predicated_break()
{
   bool progress = false;

   cfg_t cfg(&instructions);

   for (int b = 0; b < cfg.num_blocks; b++) {
      bblock_t *block = cfg.blocks[b];

      /* BREAK and CONTINUE instructions, by definition, can only be found at
       * the ends of basic blocks.
       */
      fs_inst *inst = (fs_inst *) block->end;
      if (inst->opcode != BRW_OPCODE_BREAK && inst->opcode != BRW_OPCODE_CONTINUE)
         continue;

      fs_inst *if_inst = (fs_inst *) inst->prev;
      if (if_inst->opcode != BRW_OPCODE_IF)
         continue;

      fs_inst *endif_inst = (fs_inst *) inst->next;
      if (endif_inst->opcode != BRW_OPCODE_ENDIF)
         continue;

      /* For Sandybridge with IF with embedded comparison we need to emit an
       * instruction to set the flag register.
       */
      if (brw->gen == 6 && if_inst->conditional_mod) {
         fs_inst *cmp_inst = CMP(reg_null_d, if_inst->src[0], if_inst->src[1],
                                 if_inst->conditional_mod);
         if_inst->insert_before(cmp_inst);
         inst->predicate = BRW_PREDICATE_NORMAL;
      } else {
         inst->predicate = if_inst->predicate;
         inst->predicate_inverse = if_inst->predicate_inverse;
      }

      if_inst->remove();
      endif_inst->remove();

      /* By removing the ENDIF instruction we removed a basic block. Skip over
       * it for the next iteration.
       */
      b++;

      progress = true;
   }

   if (progress)
      invalidate_live_intervals();

   return progress;
}
