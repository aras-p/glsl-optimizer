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

/** @file brw_fs_live_variables.cpp
 *
 * Support for computing at the basic block level which variables
 * (virtual GRFs in our case) are live at entry and exit.
 *
 * See Muchnik's Advanced Compiler Design and Implementation, section
 * 14.1 (p444).
 */

/**
 * Sets up the use[] and def[] bitsets.
 *
 * The basic-block-level live variable analysis needs to know which
 * variables get used before they're completely defined, and which
 * variables are completely defined before they're used.
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
	    if (inst->src[i].file == GRF) {
	       int reg = inst->src[i].reg;

	       if (!BITSET_TEST(bd[b].def, reg))
		  BITSET_SET(bd[b].use, reg);
	    }
	 }

	 /* Check for unconditional writes to whole registers. These
	  * are the things that screen off preceding definitions of a
	  * variable, and thus qualify for being in def[].
	  */
	 if (inst->dst.file == GRF &&
	     inst->regs_written == v->virtual_grf_sizes[inst->dst.reg] &&
	     !inst->is_partial_write()) {
	    int reg = inst->dst.reg;
            if (!BITSET_TEST(bd[b].use, reg))
               BITSET_SET(bd[b].def, reg);
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

fs_live_variables::fs_live_variables(fs_visitor *v, cfg_t *cfg)
   : v(v), cfg(cfg)
{
   mem_ctx = ralloc_context(cfg->mem_ctx);

   num_vars = v->virtual_grf_count;
   bd = rzalloc_array(mem_ctx, struct block_data, cfg->num_blocks);

   bitset_words = BITSET_WORDS(v->virtual_grf_count);
   for (int i = 0; i < cfg->num_blocks; i++) {
      bd[i].def = rzalloc_array(mem_ctx, BITSET_WORD, bitset_words);
      bd[i].use = rzalloc_array(mem_ctx, BITSET_WORD, bitset_words);
      bd[i].livein = rzalloc_array(mem_ctx, BITSET_WORD, bitset_words);
      bd[i].liveout = rzalloc_array(mem_ctx, BITSET_WORD, bitset_words);
   }

   setup_def_use();
   compute_live_variables();
}

fs_live_variables::~fs_live_variables()
{
   ralloc_free(mem_ctx);
}

#define MAX_INSTRUCTION (1 << 30)

void
fs_visitor::calculate_live_intervals()
{
   int num_vars = this->virtual_grf_count;

   if (this->live_intervals_valid)
      return;

   int *start = ralloc_array(mem_ctx, int, num_vars);
   int *end = ralloc_array(mem_ctx, int, num_vars);
   ralloc_free(this->virtual_grf_start);
   ralloc_free(this->virtual_grf_end);
   this->virtual_grf_start = start;
   this->virtual_grf_end = end;

   for (int i = 0; i < num_vars; i++) {
      start[i] = MAX_INSTRUCTION;
      end[i] = -1;
   }

   /* Start by setting up the intervals with no knowledge of control
    * flow.
    */
   int ip = 0;
   foreach_list(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      for (unsigned int i = 0; i < 3; i++) {
	 if (inst->src[i].file == GRF) {
	    int reg = inst->src[i].reg;
            int end_ip = ip;

            /* In most cases, a register can be written over safely by the
             * same instruction that is its last use.  For a single
             * instruction, the sources are dereferenced before writing of the
             * destination starts (naturally).  This gets more complicated for
             * simd16, because the instruction:
             *
             * mov(16)      g4<1>F      g4<8,8,1>F   g6<8,8,1>F
             *
             * is actually decoded in hardware as:
             *
             * mov(8)       g4<1>F      g4<8,8,1>F   g6<8,8,1>F
             * mov(8)       g5<1>F      g5<8,8,1>F   g7<8,8,1>F
             *
             * Which is safe.  However, if we have uniform accesses
             * happening, we get into trouble:
             *
             * mov(8)       g4<1>F      g4<0,1,0>F   g6<8,8,1>F
             * mov(8)       g5<1>F      g4<0,1,0>F   g7<8,8,1>F
             *
             * Now our destination for the first instruction overwrote the
             * second instruction's src0, and we get garbage for those 8
             * pixels.  There's a similar issue for the pre-gen6
             * pixel_x/pixel_y, which are registers of 16-bit values and thus
             * would get stomped by the first decode as well.
             */
            if (dispatch_width == 16 && (inst->src[i].smear >= 0 ||
                                         (this->pixel_x.reg == reg ||
                                          this->pixel_y.reg == reg))) {
               end_ip++;
            }

            start[reg] = MIN2(start[reg], ip);
            end[reg] = MAX2(end[reg], end_ip);
	 }
      }

      if (inst->dst.file == GRF) {
         int reg = inst->dst.reg;

         start[reg] = MIN2(start[reg], ip);
         end[reg] = MAX2(end[reg], ip);
      }

      ip++;
   }

   /* Now, extend those intervals using our analysis of control flow. */
   cfg_t cfg(this);
   fs_live_variables livevars(this, &cfg);

   for (int b = 0; b < cfg.num_blocks; b++) {
      for (int i = 0; i < num_vars; i++) {
	 if (BITSET_TEST(livevars.bd[b].livein, i)) {
	    start[i] = MIN2(start[i], cfg.blocks[b]->start_ip);
	    end[i] = MAX2(end[i], cfg.blocks[b]->start_ip);
	 }

	 if (BITSET_TEST(livevars.bd[b].liveout, i)) {
	    start[i] = MIN2(start[i], cfg.blocks[b]->end_ip);
	    end[i] = MAX2(end[i], cfg.blocks[b]->end_ip);
	 }
      }
   }

   this->live_intervals_valid = true;
}

bool
fs_visitor::virtual_grf_interferes(int a, int b)
{
   return !(virtual_grf_end[a] <= virtual_grf_start[b] ||
            virtual_grf_end[b] <= virtual_grf_start[a]);
}
