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
 * See Muchnick's Advanced Compiler Design and Implementation, section
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

   foreach_block (block, cfg) {
      assert(ip == block->start_ip);
      if (block->num > 0)
	 assert(cfg->blocks[block->num - 1]->end_ip == ip - 1);

      foreach_inst_in_block(vec4_instruction, inst, block) {
	 /* Set use[] for this instruction */
	 for (unsigned int i = 0; i < 3; i++) {
	    if (inst->src[i].file == GRF) {
	       int reg = inst->src[i].reg;

               for (int j = 0; j < 4; j++) {
                  int c = BRW_GET_SWZ(inst->src[i].swizzle, j);
                  if (!BITSET_TEST(bd[block->num].def, reg * 4 + c))
                     BITSET_SET(bd[block->num].use, reg * 4 + c);
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
                  if (!BITSET_TEST(bd[block->num].use, reg * 4 + c))
                     BITSET_SET(bd[block->num].def, reg * 4 + c);
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

      foreach_block (block, cfg) {
	 /* Update livein */
	 for (int i = 0; i < bitset_words; i++) {
            BITSET_WORD new_livein = (bd[block->num].use[i] |
                                      (bd[block->num].liveout[i] &
                                       ~bd[block->num].def[i]));
            if (new_livein & ~bd[block->num].livein[i]) {
               bd[block->num].livein[i] |= new_livein;
               cont = true;
	    }
	 }

	 /* Update liveout */
	 foreach_list_typed(bblock_link, child_link, link, &block->children) {
	    bblock_t *child = child_link->block;

	    for (int i = 0; i < bitset_words; i++) {
               BITSET_WORD new_liveout = (bd[child->num].livein[i] &
                                          ~bd[block->num].liveout[i]);
               if (new_liveout) {
                  bd[block->num].liveout[i] |= new_liveout;
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
   mem_ctx = ralloc_context(NULL);

   num_vars = v->virtual_grf_count * 4;
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
 * per-channel ranges into a per-vgrf range for virtual_grf_start[] and
 * virtual_grf_end[].
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

   int *start = ralloc_array(mem_ctx, int, this->virtual_grf_count * 4);
   int *end = ralloc_array(mem_ctx, int, this->virtual_grf_count * 4);
   ralloc_free(this->virtual_grf_start);
   ralloc_free(this->virtual_grf_end);
   this->virtual_grf_start = start;
   this->virtual_grf_end = end;

   for (int i = 0; i < this->virtual_grf_count * 4; i++) {
      start[i] = MAX_INSTRUCTION;
      end[i] = -1;
   }

   /* Start by setting up the intervals with no knowledge of control
    * flow.
    */
   int ip = 0;
   foreach_block_and_inst(block, vec4_instruction, inst, cfg) {
      for (unsigned int i = 0; i < 3; i++) {
	 if (inst->src[i].file == GRF) {
	    int reg = inst->src[i].reg;

            for (int j = 0; j < 4; j++) {
               int c = BRW_GET_SWZ(inst->src[i].swizzle, j);

               start[reg * 4 + c] = MIN2(start[reg * 4 + c], ip);
               end[reg * 4 + c] = ip;
            }
	 }
      }

      if (inst->dst.file == GRF) {
         int reg = inst->dst.reg;

         for (int c = 0; c < 4; c++) {
            if (inst->dst.writemask & (1 << c)) {
               start[reg * 4 + c] = MIN2(start[reg * 4 + c], ip);
               end[reg * 4 + c] = ip;
            }
         }
      }

      ip++;
   }

   /* Now, extend those intervals using our analysis of control flow.
    *
    * The control flow-aware analysis was done at a channel level, while at
    * this point we're distilling it down to vgrfs.
    */
   vec4_live_variables livevars(this, cfg);

   foreach_block (block, cfg) {
      for (int i = 0; i < livevars.num_vars; i++) {
	 if (BITSET_TEST(livevars.bd[block->num].livein, i)) {
	    start[i] = MIN2(start[i], block->start_ip);
	    end[i] = MAX2(end[i], block->start_ip);
	 }

	 if (BITSET_TEST(livevars.bd[block->num].liveout, i)) {
	    start[i] = MIN2(start[i], block->end_ip);
	    end[i] = MAX2(end[i], block->end_ip);
	 }
      }
   }

   this->live_intervals_valid = true;
}

void
vec4_visitor::invalidate_live_intervals()
{
   live_intervals_valid = false;
}

bool
vec4_visitor::virtual_grf_interferes(int a, int b)
{
   int start_a = MIN2(MIN2(virtual_grf_start[a * 4 + 0],
                           virtual_grf_start[a * 4 + 1]),
                      MIN2(virtual_grf_start[a * 4 + 2],
                           virtual_grf_start[a * 4 + 3]));
   int start_b = MIN2(MIN2(virtual_grf_start[b * 4 + 0],
                           virtual_grf_start[b * 4 + 1]),
                      MIN2(virtual_grf_start[b * 4 + 2],
                           virtual_grf_start[b * 4 + 3]));
   int end_a = MAX2(MAX2(virtual_grf_end[a * 4 + 0],
                         virtual_grf_end[a * 4 + 1]),
                    MAX2(virtual_grf_end[a * 4 + 2],
                         virtual_grf_end[a * 4 + 3]));
   int end_b = MAX2(MAX2(virtual_grf_end[b * 4 + 0],
                         virtual_grf_end[b * 4 + 1]),
                    MAX2(virtual_grf_end[b * 4 + 2],
                         virtual_grf_end[b * 4 + 3]));
   return !(end_a <= start_b ||
            end_b <= start_a);
}
