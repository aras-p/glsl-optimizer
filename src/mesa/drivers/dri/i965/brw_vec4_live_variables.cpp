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
#include "brw_vec4_live_variables.h"

using namespace brw;

/** @file brw_vec4_live_variables.cpp
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
 *
 * We independently track each channel of a vec4.  This is because we need to
 * be able to recognize a sequence like:
 *
 * ...
 * DP4 tmp.x a b;
 * DP4 tmp.y c d;
 * MUL result.xy tmp.xy e.xy
 * ...
 *
 * as having tmp live only across that sequence (assuming it's used nowhere
 * else), because it's a common pattern.  A more conservative approach that
 * doesn't get tmp marked a deffed in this block will tend to result in
 * spilling.
 */
void
vec4_live_variables::setup_def_use()
{
   int ip = 0;

   for (int b = 0; b < cfg->num_blocks; b++) {
      bblock_t *block = cfg->blocks[b];

      assert(ip == block->start_ip);
      if (b > 0)
	 assert(cfg->blocks[b - 1]->end_ip == ip - 1);

      for (vec4_instruction *inst = (vec4_instruction *)block->start;
	   inst != block->end->next;
	   inst = (vec4_instruction *)inst->next) {

	 /* Set use[] for this instruction */
	 for (unsigned int i = 0; i < 3; i++) {
	    if (inst->src[i].file == GRF) {
	       int reg = inst->src[i].reg;

               for (int j = 0; j < 4; j++) {
                  int c = BRW_GET_SWZ(inst->src[i].swizzle, j);
                  if (!bd[b].def[reg * 4 + c])
                     bd[b].use[reg * 4 + c] = true;
               }
	    }
	 }

	 /* Check for unconditional writes to whole registers. These
	  * are the things that screen off preceding definitions of a
	  * variable, and thus qualify for being in def[].
	  */
	 if (inst->dst.file == GRF &&
	     v->virtual_grf_sizes[inst->dst.reg] == 1 &&
	     !inst->predicate) {
            for (int c = 0; c < 4; c++) {
               if (inst->dst.writemask & (1 << c)) {
                  int reg = inst->dst.reg;
                  if (!bd[b].use[reg * 4 + c])
                     bd[b].def[reg * 4 + c] = true;
               }
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
vec4_live_variables::compute_live_variables()
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
	    bblock_link *link = (bblock_link *)block_node;
	    bblock_t *block = link->block;

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

vec4_live_variables::vec4_live_variables(vec4_visitor *v, cfg_t *cfg)
   : v(v), cfg(cfg)
{
   mem_ctx = ralloc_context(cfg->mem_ctx);

   num_vars = v->virtual_grf_count * 4;
   bd = rzalloc_array(mem_ctx, struct block_data, cfg->num_blocks);

   for (int i = 0; i < cfg->num_blocks; i++) {
      bd[i].def = rzalloc_array(mem_ctx, bool, num_vars);
      bd[i].use = rzalloc_array(mem_ctx, bool, num_vars);
      bd[i].livein = rzalloc_array(mem_ctx, bool, num_vars);
      bd[i].liveout = rzalloc_array(mem_ctx, bool, num_vars);
   }

   setup_def_use();
   compute_live_variables();
}

vec4_live_variables::~vec4_live_variables()
{
   ralloc_free(mem_ctx);
}

#define MAX_INSTRUCTION (1 << 30)

/**
 * Computes a conservative start/end of the live intervals for each virtual GRF.
 *
 * We could expose per-channel live intervals to the consumer based on the
 * information we computed in vec4_live_variables, except that our only
 * current user is virtual_grf_interferes().  So we instead union the
 * per-channel ranges into a per-vgrf range for virtual_grf_def[] and
 * virtual_grf_use[].
 *
 * We could potentially have virtual_grf_interferes() do the test per-channel,
 * which would let some interesting register allocation occur (particularly on
 * code-generated GLSL sequences from the Cg compiler which does register
 * allocation at the GLSL level and thus reuses components of the variable
 * with distinct lifetimes).  But right now the complexity of doing so doesn't
 * seem worth it, since having virtual_grf_interferes() be cheap is important
 * for register allocation performance.
 */
void
vec4_visitor::calculate_live_intervals()
{
   if (this->live_intervals_valid)
      return;

   int *def = ralloc_array(mem_ctx, int, this->virtual_grf_count);
   int *use = ralloc_array(mem_ctx, int, this->virtual_grf_count);
   ralloc_free(this->virtual_grf_def);
   ralloc_free(this->virtual_grf_use);
   this->virtual_grf_def = def;
   this->virtual_grf_use = use;

   for (int i = 0; i < this->virtual_grf_count; i++) {
      def[i] = MAX_INSTRUCTION;
      use[i] = -1;
   }

   /* Start by setting up the intervals with no knowledge of control
    * flow.
    */
   int ip = 0;
   foreach_list(node, &this->instructions) {
      vec4_instruction *inst = (vec4_instruction *)node;

      for (unsigned int i = 0; i < 3; i++) {
	 if (inst->src[i].file == GRF) {
	    int reg = inst->src[i].reg;

	    use[reg] = ip;
	 }
      }

      if (inst->dst.file == GRF) {
         int reg = inst->dst.reg;

         def[reg] = MIN2(def[reg], ip);
      }

      ip++;
   }

   /* Now, extend those intervals using our analysis of control flow.
    *
    * The control flow-aware analysis was done at a channel level, while at
    * this point we're distilling it down to vgrfs.
    */
   cfg_t cfg(this);
   vec4_live_variables livevars(this, &cfg);

   for (int b = 0; b < cfg.num_blocks; b++) {
      for (int i = 0; i < livevars.num_vars; i++) {
	 if (livevars.bd[b].livein[i]) {
	    def[i / 4] = MIN2(def[i / 4], cfg.blocks[b]->start_ip);
	    use[i / 4] = MAX2(use[i / 4], cfg.blocks[b]->start_ip);
	 }

	 if (livevars.bd[b].liveout[i]) {
	    def[i / 4] = MIN2(def[i / 4], cfg.blocks[b]->end_ip);
	    use[i / 4] = MAX2(use[i / 4], cfg.blocks[b]->end_ip);
	 }
      }
   }

   this->live_intervals_valid = true;

   /* Note in the non-control-flow code above, that we only take def[] as the
    * first store, and use[] as the last use.  We use this in dead code
    * elimination, to determine when a store never gets used.  However, we
    * also use these arrays to answer the virtual_grf_interferes() question
    * (live interval analysis), which is used for register coalescing and
    * register allocation.
    *
    * So, there's a conflict over what the array should mean: if use[]
    * considers a def after the last use, then the dead code elimination pass
    * never does anything (and it's an important pass!).  But if we don't
    * include dead code, then virtual_grf_interferes() lies and we'll do
    * horrible things like coalesce the register that is dead-code-written
    * into another register that was live across the dead write (causing the
    * use of the second register to take the dead write's source value instead
    * of the coalesced MOV's source value).
    *
    * To resolve the conflict, immediately after calculating live intervals,
    * detect dead code, nuke it, and if we changed anything, calculate again
    * before returning to the caller.  Now we happen to produce def[] and
    * use[] arrays that will work for virtual_grf_interferes().
    */
   if (dead_code_eliminate())
      calculate_live_intervals();
}

bool
vec4_visitor::virtual_grf_interferes(int a, int b)
{
   int a_def = this->virtual_grf_def[a], a_use = this->virtual_grf_use[a];
   int b_def = this->virtual_grf_def[b], b_use = this->virtual_grf_use[b];

   /* If there's dead code (def but not use), it would break our test
    * unless we consider it used.
    */
   if ((a_use == -1 && a_def != MAX_INSTRUCTION) ||
       (b_use == -1 && b_def != MAX_INSTRUCTION)) {
      return true;
   }

   int start = MAX2(a_def, b_def);
   int end = MIN2(a_use, b_use);

   return start < end;
}
