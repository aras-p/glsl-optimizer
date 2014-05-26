/*
 * Copyright © 2012 Intel Corporation
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

/** @file brw_fs_register_coalesce.cpp
 *
 * Implements register coalescing: Checks if the two registers involved in a
 * raw move don't interfere, in which case they can both be stored in the same
 * place and the MOV removed.
 *
 * To do this, all uses of the source of the MOV in the shader are replaced
 * with the destination of the MOV. For example:
 *
 * add vgrf3:F, vgrf1:F, vgrf2:F
 * mov vgrf4:F, vgrf3:F
 * mul vgrf5:F, vgrf5:F, vgrf4:F
 *
 * becomes
 *
 * add vgrf4:F, vgrf1:F, vgrf2:F
 * mul vgrf5:F, vgrf5:F, vgrf4:F
 */

#include "brw_fs.h"
#include "brw_fs_live_variables.h"

static bool
is_nop_mov(const fs_inst *inst)
{
   if (inst->opcode == BRW_OPCODE_MOV) {
      return inst->dst.equals(inst->src[0]);
   }

   return false;
}

static bool
is_coalesce_candidate(const fs_inst *inst, const int *virtual_grf_sizes)
{
   if (inst->opcode != BRW_OPCODE_MOV ||
       inst->is_partial_write() ||
       inst->saturate ||
       inst->src[0].file != GRF ||
       inst->src[0].negate ||
       inst->src[0].abs ||
       !inst->src[0].is_contiguous() ||
       inst->dst.file != GRF ||
       inst->dst.type != inst->src[0].type) {
      return false;
   }

   if (virtual_grf_sizes[inst->src[0].reg] >
       virtual_grf_sizes[inst->dst.reg])
      return false;

   return true;
}

static bool
can_coalesce_vars(brw::fs_live_variables *live_intervals,
                  const exec_list *instructions, const fs_inst *inst,
                  int var_to, int var_from)
{
   if (!live_intervals->vars_interfere(var_from, var_to))
      return true;

   /* We know that the live ranges of A (var_from) and B (var_to)
    * interfere because of the ->vars_interfere() call above. If the end
    * of B's live range is after the end of A's range, then we know two
    * things:
    *  - the start of B's live range must be in A's live range (since we
    *    already know the two ranges interfere, this is the only remaining
    *    possibility)
    *  - the interference isn't of the form we're looking for (where B is
    *    entirely inside A)
    */
   if (live_intervals->end[var_to] > live_intervals->end[var_from])
      return false;

   int scan_ip = -1;

   foreach_list(n, instructions) {
      fs_inst *scan_inst = (fs_inst *)n;
      scan_ip++;

      if (scan_inst->is_control_flow())
         return false;

      if (scan_ip <= live_intervals->start[var_to])
         continue;

      if (scan_ip > live_intervals->end[var_to])
         break;

      if (scan_inst->dst.equals(inst->dst) ||
          scan_inst->dst.equals(inst->src[0]))
         return false;
   }

   return true;
}

bool
fs_visitor::register_coalesce()
{
   bool progress = false;

   calculate_live_intervals();

   int src_size = 0;
   int channels_remaining = 0;
   int reg_from = -1, reg_to = -1;
   int reg_to_offset[MAX_SAMPLER_MESSAGE_SIZE];
   fs_inst *mov[MAX_SAMPLER_MESSAGE_SIZE];
   int var_to[MAX_SAMPLER_MESSAGE_SIZE];
   int var_from[MAX_SAMPLER_MESSAGE_SIZE];

   foreach_list(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      if (!is_coalesce_candidate(inst, virtual_grf_sizes))
         continue;

      if (is_nop_mov(inst)) {
         inst->opcode = BRW_OPCODE_NOP;
         progress = true;
         continue;
      }

      if (reg_from != inst->src[0].reg) {
         reg_from = inst->src[0].reg;

         src_size = virtual_grf_sizes[inst->src[0].reg];
         assert(src_size <= MAX_SAMPLER_MESSAGE_SIZE);

         channels_remaining = src_size;
         memset(mov, 0, sizeof(mov));

         reg_to = inst->dst.reg;
      }

      if (reg_to != inst->dst.reg)
         continue;

      const int offset = inst->src[0].reg_offset;
      reg_to_offset[offset] = inst->dst.reg_offset;
      mov[offset] = inst;
      channels_remaining--;

      if (channels_remaining)
         continue;

      bool can_coalesce = true;
      for (int i = 0; i < src_size; i++) {
         var_to[i] = live_intervals->var_from_vgrf[reg_to] + reg_to_offset[i];
         var_from[i] = live_intervals->var_from_vgrf[reg_from] + i;

         if (!can_coalesce_vars(live_intervals, &instructions, inst,
                                var_to[i], var_from[i])) {
            can_coalesce = false;
            reg_from = -1;
            break;
         }
      }

      if (!can_coalesce)
         continue;

      progress = true;

      for (int i = 0; i < src_size; i++) {
         if (mov[i]) {
            mov[i]->opcode = BRW_OPCODE_NOP;
            mov[i]->conditional_mod = BRW_CONDITIONAL_NONE;
            mov[i]->dst = reg_undef;
            mov[i]->src[0] = reg_undef;
            mov[i]->src[1] = reg_undef;
            mov[i]->src[2] = reg_undef;
         }
      }

      foreach_list(node, &this->instructions) {
         fs_inst *scan_inst = (fs_inst *)node;

         for (int i = 0; i < src_size; i++) {
            if (mov[i]) {
               if (scan_inst->dst.file == GRF &&
                   scan_inst->dst.reg == reg_from &&
                   scan_inst->dst.reg_offset == i) {
                  scan_inst->dst.reg = reg_to;
                  scan_inst->dst.reg_offset = reg_to_offset[i];
               }
               for (int j = 0; j < 3; j++) {
                  if (scan_inst->src[j].file == GRF &&
                      scan_inst->src[j].reg == reg_from &&
                      scan_inst->src[j].reg_offset == i) {
                     scan_inst->src[j].reg = reg_to;
                     scan_inst->src[j].reg_offset = reg_to_offset[i];
                  }
               }
            }
         }
      }

      for (int i = 0; i < src_size; i++) {
         live_intervals->start[var_to[i]] =
            MIN2(live_intervals->start[var_to[i]],
                 live_intervals->start[var_from[i]]);
         live_intervals->end[var_to[i]] =
            MAX2(live_intervals->end[var_to[i]],
                 live_intervals->end[var_from[i]]);
      }
      reg_from = -1;
   }

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
