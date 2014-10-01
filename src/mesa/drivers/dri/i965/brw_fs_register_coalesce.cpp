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
#include "brw_cfg.h"
#include "brw_fs_live_variables.h"

static bool
is_nop_mov(const fs_inst *inst)
{
   if (inst->opcode == SHADER_OPCODE_LOAD_PAYLOAD) {
      fs_reg dst = inst->dst;
      for (int i = 0; i < inst->sources; i++) {
         dst.reg_offset = i;
         if (!dst.equals(inst->src[i])) {
            return false;
         }
      }
      return true;
   } else if (inst->opcode == BRW_OPCODE_MOV) {
      return inst->dst.equals(inst->src[0]);
   }

   return false;
}

static bool
is_copy_payload(const fs_visitor *v, const fs_inst *inst)
{
   if (v->virtual_grf_sizes[inst->src[0].reg] != inst->regs_written)
      return false;

   fs_reg reg = inst->src[0];

   for (int i = 0; i < inst->sources; i++)
      if (!inst->src[i].equals(offset(reg, i)))
         return false;

   return true;
}

static bool
is_coalesce_candidate(const fs_visitor *v, const fs_inst *inst)
{
   if ((inst->opcode != BRW_OPCODE_MOV &&
        inst->opcode != SHADER_OPCODE_LOAD_PAYLOAD) ||
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

   if (v->virtual_grf_sizes[inst->src[0].reg] >
       v->virtual_grf_sizes[inst->dst.reg])
      return false;

   if (inst->opcode == SHADER_OPCODE_LOAD_PAYLOAD) {
      if (!is_copy_payload(v, inst)) {
         return false;
      }
   }

   return true;
}

static bool
can_coalesce_vars(brw::fs_live_variables *live_intervals,
                  const cfg_t *cfg, const fs_inst *inst,
                  int var_to, int var_from)
{
   if (!live_intervals->vars_interfere(var_from, var_to))
      return true;

   int start_to = live_intervals->start[var_to];
   int end_to = live_intervals->end[var_to];
   int start_from = live_intervals->start[var_from];
   int end_from = live_intervals->end[var_from];

   /* Variables interfere and one line range isn't a subset of the other. */
   if ((end_to > end_from && start_from < start_to) ||
       (end_from > end_to && start_to < start_from))
      return false;

   int start_ip = MIN2(start_to, start_from);
   int scan_ip = -1;

   foreach_block_and_inst(block, fs_inst, scan_inst, cfg) {
      scan_ip++;

      if (scan_ip < start_ip)
         continue;

      if (scan_inst->is_control_flow())
         return false;

      if (scan_ip <= live_intervals->start[var_to])
         continue;

      if (scan_ip > live_intervals->end[var_to])
         return true;

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
   int reg_to_offset[MAX_VGRF_SIZE];
   fs_inst *mov[MAX_VGRF_SIZE];
   int var_to[MAX_VGRF_SIZE];
   int var_from[MAX_VGRF_SIZE];

   foreach_block_and_inst(block, fs_inst, inst, cfg) {
      if (!is_coalesce_candidate(this, inst))
         continue;

      if (is_nop_mov(inst)) {
         inst->opcode = BRW_OPCODE_NOP;
         progress = true;
         continue;
      }

      if (reg_from != inst->src[0].reg) {
         reg_from = inst->src[0].reg;

         src_size = virtual_grf_sizes[inst->src[0].reg];
         assert(src_size <= MAX_VGRF_SIZE);

         assert(inst->src[0].width % 8 == 0);
         channels_remaining = src_size;
         memset(mov, 0, sizeof(mov));

         reg_to = inst->dst.reg;
      }

      if (reg_to != inst->dst.reg)
         continue;

      if (inst->opcode == SHADER_OPCODE_LOAD_PAYLOAD) {
         for (int i = 0; i < src_size; i++) {
            reg_to_offset[i] = i;
         }
         mov[0] = inst;
         channels_remaining -= inst->regs_written;
      } else {
         const int offset = inst->src[0].reg_offset;
         if (mov[offset]) {
            /* This is the second time that this offset in the register has
             * been set.  This means, in particular, that inst->dst was
             * live before this instruction and that the live ranges of
             * inst->dst and inst->src[0] overlap and we can't coalesce the
             * two variables.  Let's ensure that doesn't happen.
             */
            channels_remaining = -1;
            continue;
         }
         reg_to_offset[offset] = inst->dst.reg_offset;
         if (inst->src[0].width == 16)
            reg_to_offset[offset + 1] = inst->dst.reg_offset + 1;
         mov[offset] = inst;
         channels_remaining -= inst->regs_written;
      }

      if (channels_remaining)
         continue;

      bool can_coalesce = true;
      for (int i = 0; i < src_size; i++) {
         if (reg_to_offset[i] != reg_to_offset[0] + i) {
            /* Registers are out-of-order. */
            can_coalesce = false;
            reg_from = -1;
            break;
         }

         var_to[i] = live_intervals->var_from_vgrf[reg_to] + reg_to_offset[i];
         var_from[i] = live_intervals->var_from_vgrf[reg_from] + i;

         if (!can_coalesce_vars(live_intervals, cfg, inst,
                                var_to[i], var_from[i])) {
            can_coalesce = false;
            reg_from = -1;
            break;
         }
      }

      if (!can_coalesce)
         continue;

      progress = true;
      bool was_load_payload = inst->opcode == SHADER_OPCODE_LOAD_PAYLOAD;

      for (int i = 0; i < src_size; i++) {
         if (mov[i]) {
            mov[i]->opcode = BRW_OPCODE_NOP;
            mov[i]->conditional_mod = BRW_CONDITIONAL_NONE;
            mov[i]->dst = reg_undef;
            for (int j = 0; j < mov[i]->sources; j++) {
               mov[i]->src[j] = reg_undef;
            }
         }
      }

      foreach_block_and_inst(block, fs_inst, scan_inst, cfg) {
         for (int i = 0; i < src_size; i++) {
            if (mov[i] || was_load_payload) {
               if (scan_inst->dst.file == GRF &&
                   scan_inst->dst.reg == reg_from &&
                   scan_inst->dst.reg_offset == i) {
                  scan_inst->dst.reg = reg_to;
                  scan_inst->dst.reg_offset = reg_to_offset[i];
               }
               for (int j = 0; j < scan_inst->sources; j++) {
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
      foreach_block_and_inst_safe (block, backend_instruction, inst, cfg) {
         if (inst->opcode == BRW_OPCODE_NOP) {
            inst->remove(block);
         }
      }

      invalidate_live_intervals();
   }

   return progress;
}
