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

/** @file brw_dead_control_flow.cpp
 *
 * This file implements the dead control flow elimination optimization pass.
 */

#include "brw_shader.h"
#include "brw_cfg.h"

/* Look for and eliminate dead control flow:
 *
 *   - if/endif
 *   - if/else/endif
 */
bool
dead_control_flow_eliminate(backend_visitor *v)
{
   bool progress = false;

   cfg_t cfg(&v->instructions);

   for (int b = 0; b < cfg.num_blocks; b++) {
      bblock_t *block = cfg.blocks[b];
      bool found = false;

      /* ENDIF instructions, by definition, can only be found at the start of
       * basic blocks.
       */
      backend_instruction *endif_inst = block->start;
      if (endif_inst->opcode != BRW_OPCODE_ENDIF)
         continue;

      backend_instruction *if_inst = NULL, *else_inst = NULL;
      backend_instruction *prev_inst = (backend_instruction *) endif_inst->prev;
      if (prev_inst->opcode == BRW_OPCODE_IF) {
         if_inst = prev_inst;
         found = true;
      } else if (prev_inst->opcode == BRW_OPCODE_ELSE) {
         else_inst = prev_inst;

         prev_inst = (backend_instruction *) prev_inst->prev;
         if (prev_inst->opcode == BRW_OPCODE_IF) {
            if_inst = prev_inst;
            found = true;
         }
      }

      if (found) {
         if_inst->remove();
         if (else_inst)
            else_inst->remove();
         endif_inst->remove();
         progress = true;
      }
   }

   if (progress)
      v->invalidate_live_intervals();

   return progress;
}
