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
#include "brw_fs_live_variables.h"
#include "brw_cfg.h"

/** @file brw_fs_saturate_propagation.cpp
 */

static bool
opt_saturate_propagation_local(fs_visitor *v, bblock_t *block)
{
   bool progress = false;
   int ip = block->start_ip - 1;

   for (fs_inst *inst = (fs_inst *)block->start;
        inst != block->end->next;
        inst = (fs_inst *) inst->next) {
      ip++;

      if (inst->opcode != BRW_OPCODE_MOV ||
          inst->dst.file != GRF ||
          inst->src[0].file != GRF ||
          inst->src[0].abs ||
          inst->src[0].negate ||
          !inst->saturate)
         continue;

      int src_var = v->live_intervals->var_from_reg(&inst->src[0]);
      int src_end_ip = v->live_intervals->end[src_var];
      if (src_end_ip > ip && !inst->dst.equals(inst->src[0]))
         continue;

      int scan_ip = ip;
      bool interfered = false;
      for (fs_inst *scan_inst = (fs_inst *) inst->prev;
           scan_inst != block->start->prev;
           scan_inst = (fs_inst *) scan_inst->prev) {
         scan_ip--;

         if (scan_inst->dst.file == GRF &&
             scan_inst->dst.reg == inst->src[0].reg &&
             scan_inst->dst.reg_offset == inst->src[0].reg_offset &&
             !scan_inst->is_partial_write()) {
            if (scan_inst->can_do_saturate()) {
               scan_inst->saturate = true;
               inst->saturate = false;
               progress = true;
            }
            break;
         }
         for (int i = 0; i < 3; i++) {
            if (scan_inst->src[i].file == GRF &&
                scan_inst->src[i].reg == inst->src[0].reg &&
                scan_inst->src[i].reg_offset == inst->src[0].reg_offset) {
               interfered = true;
               break;
            }
         }

         if (interfered)
            break;
      }
   }

   return progress;
}

bool
fs_visitor::opt_saturate_propagation()
{
   bool progress = false;

   calculate_live_intervals();

   cfg_t cfg(&instructions);

   for (int b = 0; b < cfg.num_blocks; b++) {
      progress = opt_saturate_propagation_local(this, cfg.blocks[b])
                 || progress;
   }

   if (progress)
      invalidate_live_intervals();

   return progress;
}
