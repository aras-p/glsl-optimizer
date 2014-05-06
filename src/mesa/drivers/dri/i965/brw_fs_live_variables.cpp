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
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#include "brw_cfg.h"
#include "brw_fs_live_variables.h"

using namespace brw;

#define MAX_INSTRUCTION (1 << 30)

/** @file brw_fs_live_variables.cpp
 *
 * Support for calculating liveness information about virtual GRFs.
 *
 * This produces a live interval for each whole virtual GRF.  We could
 * choose to expose per-component live intervals for VGRFs of size > 1,
 * but we currently do not.  It is easier for the consumers of this
 * information to work with whole VGRFs.
 *
 * However, we internally track use/def information at the per-component
 * (reg_offset) level for greater accuracy.  Large VGRFs may be accessed
 * piecemeal over many (possibly non-adjacent) instructions.  In this case,
 * examining a single instruction is insufficient to decide whether a whole
 * VGRF is ultimately used or defined.  Tracking individual components
 * allows us to easily assemble this information.
 *
 * See Muchnick's Advanced Compiler Design and Implementation, section
 * 14.1 (p444).
 */

void
fs_live_variables::setup_one_read(bblock_t *block, fs_inst *inst,
                                  int ip, fs_reg reg)
{
   int var = var_from_vgrf[reg.reg] + reg.reg_offset;
   assert(var < num_vars);

   /* In most cases, a register can be written over safely by the
    * same instruction that is its last use.  For a single
    * instruction, the sources are dereferenced before writing of the
    * destination starts (naturally).  This gets more complicated for
    * simd16, because the instruction:
    *
    * add(16)      g4<1>F      g4<8,8,1>F   g6<8,8,1>F
    *
    * is actually decoded in hardware as:
    *
    * add(8)       g4<1>F      g4<8,8,1>F   g6<8,8,1>F
    * add(8)       g5<1>F      g5<8,8,1>F   g7<8,8,1>F
    *
    * Which is safe.  However, if we have uniform accesses
    * happening, we get into trouble:
    *
    * add(8)       g4<1>F      g4<0,1,0>F   g6<8,8,1>F
    * add(8)       g5<1>F      g4<0,1,0>F   g7<8,8,1>F
    *
    * Now our destination for the first instruction overwrote the
    * second instruction's src0, and we get garbage for those 8
    * pixels.  There's a similar issue for the pre-gen6
    * pixel_x/pixel_y, which are registers of 16-bit values and thus
    * would get stomped by the first decode as well.
    */
   int end_ip = ip;
   if (v->dispatch_width == 16 && (reg.stride == 0 ||
                                   reg.type == BRW_REGISTER_TYPE_UW ||
                                   reg.type == BRW_REGISTER_TYPE_W ||
                                   reg.type == BRW_REGISTER_TYPE_UB ||
                                   reg.type == BRW_REGISTER_TYPE_B)) {
      end_ip++;
   }

   start[var] = MIN2(start[var], ip);
   end[var] = MAX2(end[var], end_ip);

   /* The use[] bitset marks when the block makes use of a variable (VGRF
    * channel) without having completely defined that variable within the
    * block.
    */
   if (!BITSET_TEST(bd[block->block_num].def, var))
      BITSET_SET(bd[block->block_num].use, var);
}

void
fs_live_variables::setup_one_write(bblock_t *block, fs_inst *inst,
                                   int ip, fs_reg reg)
{
   int var = var_from_vgrf[reg.reg] + reg.reg_offset;
   assert(var < num_vars);

   start[var] = MIN2(start[var], ip);
   end[var] = MAX2(end[var], ip);

   /* The def[] bitset marks when an initialization in a block completely
    * screens off previous updates of that variable (VGRF channel).
    */
   if (inst->dst.file == GRF && !inst->is_partial_write()) {
      if (!BITSET_TEST(bd[block->block_num].use, var))
         BITSET_SET(bd[block->block_num].def, var);
   }
}

/**
 * Sets up the use[] and def[] bitsets.
 *
 * The basic-block-level live variable analysis needs to know which
 * variables get used before they're completely defined, and which
 * variables are completely defined before they're used.
 *
 * These are tracked at the per-component level, rather than whole VGRFs.
 */
void
fs_live_variables::setup_def_use()
{
   int ip = 0;

   for (int b = 0; b < cfg->num_blocks; b++) {
      bblock_t *block = cfg->blocks[b];

      assert(ip == block->start_ip);
      if (b > 0)
	 assert(cfg->blocks[b - 1]->end_ip == ip - 1);

      for (fs_inst *inst = (fs_inst *)block->start;
	   inst != block->end->next;
	   inst = (fs_inst *)inst->next) {

	 /* Set use[] for this instruction */
	 for (unsigned int i = 0; i < 3; i++) {
            fs_reg reg = inst->src[i];

            if (reg.file != GRF)
               continue;

            for (int j = 0; j < inst->regs_read(v, i); j++) {
               setup_one_read(block, inst, ip, reg);
               reg.reg_offset++;
            }
	 }

         /* Set def[] for this instruction */
         if (inst->dst.file == GRF) {
            fs_reg reg = inst->dst;
            for (int j = 0; j < inst->regs_written; j++) {
               setup_one_write(block, inst, ip, reg);
               reg.reg_offset++;
            }
	 }

	 ip++;
      }
   }
}

/**
 * The algorithm incrementally sets bits in liveout and livein,
 * propagating it through control flow.  It will eventually terminate
 * because it only ever adds bits, and stops when no bits are added in
 * a pass.
 */
void
fs_live_variables::compute_live_variables()
{
   bool cont = true;

   while (cont) {
      cont = false;

      for (int b = 0; b < cfg->num_blocks; b++) {
	 /* Update livein */
	 for (int i = 0; i < bitset_words; i++) {
            BITSET_WORD new_livein = (bd[b].use[i] |
                                      (bd[b].liveout[i] & ~bd[b].def[i]));
	    if (new_livein & ~bd[b].livein[i]) {
               bd[b].livein[i] |= new_livein;
               cont = true;
	    }
	 }

	 /* Update liveout */
	 foreach_list(block_node, &cfg->blocks[b]->children) {
	    bblock_link *link = (bblock_link *)block_node;
	    bblock_t *block = link->block;

	    for (int i = 0; i < bitset_words; i++) {
               BITSET_WORD new_liveout = (bd[block->block_num].livein[i] &
                                          ~bd[b].liveout[i]);
               if (new_liveout) {
                  bd[b].liveout[i] |= new_liveout;
                  cont = true;
               }
	    }
	 }
      }
   }
}

/**
 * Extend the start/end ranges for each variable to account for the
 * new information calculated from control flow.
 */
void
fs_live_variables::compute_start_end()
{
   for (int b = 0; b < cfg->num_blocks; b++) {
      for (int i = 0; i < num_vars; i++) {
	 if (BITSET_TEST(bd[b].livein, i)) {
	    start[i] = MIN2(start[i], cfg->blocks[b]->start_ip);
	    end[i] = MAX2(end[i], cfg->blocks[b]->start_ip);
	 }

	 if (BITSET_TEST(bd[b].liveout, i)) {
	    start[i] = MIN2(start[i], cfg->blocks[b]->end_ip);
	    end[i] = MAX2(end[i], cfg->blocks[b]->end_ip);
	 }

      }
   }
}

int
fs_live_variables::var_from_reg(fs_reg *reg)
{
   return var_from_vgrf[reg->reg] + reg->reg_offset;
}

fs_live_variables::fs_live_variables(fs_visitor *v, cfg_t *cfg)
   : v(v), cfg(cfg)
{
   mem_ctx = ralloc_context(NULL);

   num_vgrfs = v->virtual_grf_count;
   num_vars = 0;
   var_from_vgrf = rzalloc_array(mem_ctx, int, num_vgrfs);
   for (int i = 0; i < num_vgrfs; i++) {
      var_from_vgrf[i] = num_vars;
      num_vars += v->virtual_grf_sizes[i];
   }

   vgrf_from_var = rzalloc_array(mem_ctx, int, num_vars);
   for (int i = 0; i < num_vgrfs; i++) {
      for (int j = 0; j < v->virtual_grf_sizes[i]; j++) {
         vgrf_from_var[var_from_vgrf[i] + j] = i;
      }
   }

   start = ralloc_array(mem_ctx, int, num_vars);
   end = rzalloc_array(mem_ctx, int, num_vars);
   for (int i = 0; i < num_vars; i++) {
      start[i] = MAX_INSTRUCTION;
      end[i] = -1;
   }

   bd = rzalloc_array(mem_ctx, struct block_data, cfg->num_blocks);

   bitset_words = BITSET_WORDS(num_vars);
   for (int i = 0; i < cfg->num_blocks; i++) {
      bd[i].def = rzalloc_array(mem_ctx, BITSET_WORD, bitset_words);
      bd[i].use = rzalloc_array(mem_ctx, BITSET_WORD, bitset_words);
      bd[i].livein = rzalloc_array(mem_ctx, BITSET_WORD, bitset_words);
      bd[i].liveout = rzalloc_array(mem_ctx, BITSET_WORD, bitset_words);
   }

   setup_def_use();
   compute_live_variables();
   compute_start_end();
}

fs_live_variables::~fs_live_variables()
{
   ralloc_free(mem_ctx);
}

void
fs_visitor::invalidate_live_intervals()
{
   ralloc_free(live_intervals);
   live_intervals = NULL;
}

/**
 * Compute the live intervals for each virtual GRF.
 *
 * This uses the per-component use/def data, but combines it to produce
 * information about whole VGRFs.
 */
void
fs_visitor::calculate_live_intervals()
{
   if (this->live_intervals)
      return;

   int num_vgrfs = this->virtual_grf_count;
   ralloc_free(this->virtual_grf_start);
   ralloc_free(this->virtual_grf_end);
   virtual_grf_start = ralloc_array(mem_ctx, int, num_vgrfs);
   virtual_grf_end = ralloc_array(mem_ctx, int, num_vgrfs);

   for (int i = 0; i < num_vgrfs; i++) {
      virtual_grf_start[i] = MAX_INSTRUCTION;
      virtual_grf_end[i] = -1;
   }

   cfg_t cfg(&instructions);
   this->live_intervals = new(mem_ctx) fs_live_variables(this, &cfg);

   /* Merge the per-component live ranges to whole VGRF live ranges. */
   for (int i = 0; i < live_intervals->num_vars; i++) {
      int vgrf = live_intervals->vgrf_from_var[i];
      virtual_grf_start[vgrf] = MIN2(virtual_grf_start[vgrf],
                                     live_intervals->start[i]);
      virtual_grf_end[vgrf] = MAX2(virtual_grf_end[vgrf],
                                   live_intervals->end[i]);
   }
}

bool
fs_live_variables::vars_interfere(int a, int b)
{
   return !(end[b] <= start[a] ||
            end[a] <= start[b]);
}

bool
fs_visitor::virtual_grf_interferes(int a, int b)
{
   return !(virtual_grf_end[a] <= virtual_grf_start[b] ||
            virtual_grf_end[b] <= virtual_grf_start[a]);
}
