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

#include "brw_fs_cfg.h"
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
 * Sets up the use[] and def[] arrays.
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
      fs_bblock *block = cfg->blocks[b];

      assert(ip == block->start_ip);
      if (b > 0)
	 assert(cfg->blocks[b - 1]->end_ip == ip - 1);

      for (fs_inst *inst = block->start;
	   inst != block->end->next;
	   inst = (fs_inst *)inst->next) {

	 /* Set use[] for this instruction */
	 for (unsigned int i = 0; i < 3; i++) {
	    if (inst->src[i].file == GRF) {
	       int reg = inst->src[i].reg;

	       if (!bd[b].def[reg])
		  bd[b].use[reg] = true;
	    }
	 }

	 /* Check for unconditional writes to whole registers. These
	  * are the things that screen off preceding definitions of a
	  * variable, and thus qualify for being in def[].
	  */
	 if (inst->dst.file == GRF &&
	     inst->regs_written() == v->virtual_grf_sizes[inst->dst.reg] &&
	     !inst->predicated &&
	     !inst->force_uncompressed &&
	     !inst->force_sechalf) {
	    int reg = inst->dst.reg;
	    if (!bd[b].use[reg])
	       bd[b].def[reg] = true;
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
	 for (int i = 0; i < num_vars; i++) {
	    if (bd[b].use[i] || (bd[b].liveout[i] && !bd[b].def[i])) {
	       if (!bd[b].livein[i]) {
		  bd[b].livein[i] = true;
		  cont = true;
	       }
	    }
	 }

	 /* Update liveout */
	 foreach_list(block_node, &cfg->blocks[b]->children) {
	    fs_bblock_link *link = (fs_bblock_link *)block_node;
	    fs_bblock *block = link->block;

	    for (int i = 0; i < num_vars; i++) {
	       if (bd[block->block_num].livein[i] && !bd[b].liveout[i]) {
		  bd[b].liveout[i] = true;
		  cont = true;
	       }
	    }
	 }
      }
   }
}

fs_live_variables::fs_live_variables(fs_visitor *v, fs_cfg *cfg)
   : v(v), cfg(cfg)
{
   mem_ctx = ralloc_context(cfg->mem_ctx);

   num_vars = v->virtual_grf_next;
   bd = rzalloc_array(mem_ctx, struct block_data, cfg->num_blocks);
   vars = rzalloc_array(mem_ctx, struct var, num_vars);

   for (int i = 0; i < cfg->num_blocks; i++) {
      bd[i].def = rzalloc_array(mem_ctx, bool, num_vars);
      bd[i].use = rzalloc_array(mem_ctx, bool, num_vars);
      bd[i].livein = rzalloc_array(mem_ctx, bool, num_vars);
      bd[i].liveout = rzalloc_array(mem_ctx, bool, num_vars);
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
   int num_vars = this->virtual_grf_next;
   int *def = ralloc_array(mem_ctx, int, num_vars);
   int *use = ralloc_array(mem_ctx, int, num_vars);
   int loop_depth = 0;
   int loop_start = 0;

   if (this->live_intervals_valid)
      return;

   for (int i = 0; i < num_vars; i++) {
      def[i] = MAX_INSTRUCTION;
      use[i] = -1;
   }

   int ip = 0;
   foreach_list(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      if (inst->opcode == BRW_OPCODE_DO) {
	 if (loop_depth++ == 0)
	    loop_start = ip;
      } else if (inst->opcode == BRW_OPCODE_WHILE) {
	 loop_depth--;

	 if (loop_depth == 0) {
	    /* Patches up the use of vars marked for being live across
	     * the whole loop.
	     */
	    for (int i = 0; i < num_vars; i++) {
	       if (use[i] == loop_start) {
		  use[i] = ip;
	       }
	    }
	 }
      } else {
	 for (unsigned int i = 0; i < 3; i++) {
	    if (inst->src[i].file == GRF) {
	       int reg = inst->src[i].reg;

	       if (!loop_depth) {
		  use[reg] = ip;
	       } else {
		  def[reg] = MIN2(loop_start, def[reg]);
		  use[reg] = loop_start;

		  /* Nobody else is going to go smash our start to
		   * later in the loop now, because def[reg] now
		   * points before the bb header.
		   */
	       }
	    }
	 }
	 if (inst->dst.file == GRF) {
	    int reg = inst->dst.reg;

	    if (!loop_depth) {
	       def[reg] = MIN2(def[reg], ip);
	    } else {
	       def[reg] = MIN2(def[reg], loop_start);
	    }
	 }
      }

      ip++;
   }

   ralloc_free(this->virtual_grf_def);
   ralloc_free(this->virtual_grf_use);
   this->virtual_grf_def = def;
   this->virtual_grf_use = use;

   this->live_intervals_valid = true;
}

bool
fs_visitor::virtual_grf_interferes(int a, int b)
{
   int start = MAX2(this->virtual_grf_def[a], this->virtual_grf_def[b]);
   int end = MIN2(this->virtual_grf_use[a], this->virtual_grf_use[b]);

   /* We can't handle dead register writes here, without iterating
    * over the whole instruction stream to find every single dead
    * write to that register to compare to the live interval of the
    * other register.  Just assert that dead_code_eliminate() has been
    * called.
    */
   assert((this->virtual_grf_use[a] != -1 ||
	   this->virtual_grf_def[a] == MAX_INSTRUCTION) &&
	  (this->virtual_grf_use[b] != -1 ||
	   this->virtual_grf_def[b] == MAX_INSTRUCTION));

   /* If the register is used to store 16 values of less than float
    * size (only the case for pixel_[xy]), then we can't allocate
    * another dword-sized thing to that register that would be used in
    * the same instruction.  This is because when the GPU decodes (for
    * example):
    *
    * (declare (in ) vec4 gl_FragCoord@0x97766a0)
    * add(16)         g6<1>F          g6<8,8,1>UW     0.5F { align1 compr };
    *
    * it's actually processed as:
    * add(8)         g6<1>F          g6<8,8,1>UW     0.5F { align1 };
    * add(8)         g7<1>F          g6.8<8,8,1>UW   0.5F { align1 sechalf };
    *
    * so our second half values in g6 got overwritten in the first
    * half.
    */
   if (c->dispatch_width == 16 && (this->pixel_x.reg == a ||
				   this->pixel_x.reg == b ||
				   this->pixel_y.reg == a ||
				   this->pixel_y.reg == b)) {
      return start <= end;
   }

   return start < end;
}
