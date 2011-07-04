/*
 * Copyright © 2010 Intel Corporation
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

extern "C" {

#include <sys/types.h>

#include "main/macros.h"
#include "main/shaderobj.h"
#include "main/uniforms.h"
#include "program/prog_parameter.h"
#include "program/prog_print.h"
#include "program/prog_optimize.h"
#include "program/register_allocate.h"
#include "program/sampler.h"
#include "program/hash_table.h"
#include "brw_context.h"
#include "brw_eu.h"
#include "brw_wm.h"
}
#include "brw_fs.h"
#include "../glsl/glsl_types.h"
#include "../glsl/ir_optimization.h"
#include "../glsl/ir_print_visitor.h"

static void
assign_reg(int *reg_hw_locations, fs_reg *reg, int reg_width)
{
   if (reg->file == GRF && reg->reg != 0) {
      assert(reg->reg_offset >= 0);
      reg->hw_reg = reg_hw_locations[reg->reg] + reg->reg_offset * reg_width;
      reg->reg = 0;
   }
}

void
fs_visitor::assign_regs_trivial()
{
   int last_grf = 0;
   int hw_reg_mapping[this->virtual_grf_next];
   int i;
   int reg_width = c->dispatch_width / 8;

   hw_reg_mapping[0] = 0;
   /* Note that compressed instructions require alignment to 2 registers. */
   hw_reg_mapping[1] = ALIGN(this->first_non_payload_grf, reg_width);
   for (i = 2; i < this->virtual_grf_next; i++) {
      hw_reg_mapping[i] = (hw_reg_mapping[i - 1] +
			   this->virtual_grf_sizes[i - 1] * reg_width);
   }
   last_grf = hw_reg_mapping[i - 1] + (this->virtual_grf_sizes[i - 1] *
				       reg_width);

   foreach_iter(exec_list_iterator, iter, this->instructions) {
      fs_inst *inst = (fs_inst *)iter.get();

      assign_reg(hw_reg_mapping, &inst->dst, reg_width);
      assign_reg(hw_reg_mapping, &inst->src[0], reg_width);
      assign_reg(hw_reg_mapping, &inst->src[1], reg_width);
   }

   if (last_grf >= BRW_MAX_GRF) {
      fail("Ran out of regs on trivial allocator (%d/%d)\n",
	   last_grf, BRW_MAX_GRF);
   }

   this->grf_used = last_grf + reg_width;
}

bool
fs_visitor::assign_regs()
{
   /* Most of this allocation was written for a reg_width of 1
    * (dispatch_width == 8).  In extending to 16-wide, the code was
    * left in place and it was converted to have the hardware
    * registers it's allocating be contiguous physical pairs of regs
    * for reg_width == 2.
    */
   int reg_width = c->dispatch_width / 8;
   int hw_reg_mapping[this->virtual_grf_next + 1];
   int first_assigned_grf = ALIGN(this->first_non_payload_grf, reg_width);
   int base_reg_count = (BRW_MAX_GRF - first_assigned_grf) / reg_width;
   int class_sizes[base_reg_count];
   int class_count = 0;
   int aligned_pair_class = -1;

   calculate_live_intervals();

   /* Set up the register classes.
    *
    * The base registers store a scalar value.  For texture samples,
    * we get virtual GRFs composed of 4 contiguous hw register.  For
    * structures and arrays, we store them as contiguous larger things
    * than that, though we should be able to do better most of the
    * time.
    */
   class_sizes[class_count++] = 1;
   if (brw->has_pln && intel->gen < 6) {
      /* Always set up the (unaligned) pairs for gen5, so we can find
       * them for making the aligned pair class.
       */
      class_sizes[class_count++] = 2;
   }
   for (int r = 1; r < this->virtual_grf_next; r++) {
      int i;

      for (i = 0; i < class_count; i++) {
	 if (class_sizes[i] == this->virtual_grf_sizes[r])
	    break;
      }
      if (i == class_count) {
	 if (this->virtual_grf_sizes[r] >= base_reg_count) {
	    fail("Object too large to register allocate.\n");
	 }

	 class_sizes[class_count++] = this->virtual_grf_sizes[r];
      }
   }

   int ra_reg_count = 0;
   int class_base_reg[class_count];
   int class_reg_count[class_count];
   int classes[class_count + 1];

   for (int i = 0; i < class_count; i++) {
      class_base_reg[i] = ra_reg_count;
      class_reg_count[i] = base_reg_count - (class_sizes[i] - 1);
      ra_reg_count += class_reg_count[i];
   }

   struct ra_regs *regs = ra_alloc_reg_set(ra_reg_count);
   for (int i = 0; i < class_count; i++) {
      classes[i] = ra_alloc_reg_class(regs);

      for (int i_r = 0; i_r < class_reg_count[i]; i_r++) {
	 ra_class_add_reg(regs, classes[i], class_base_reg[i] + i_r);
      }

      /* Add conflicts between our contiguous registers aliasing
       * base regs and other register classes' contiguous registers
       * that alias base regs, or the base regs themselves for classes[0].
       */
      for (int c = 0; c <= i; c++) {
	 for (int i_r = 0; i_r < class_reg_count[i]; i_r++) {
	    for (int c_r = MAX2(0, i_r - (class_sizes[c] - 1));
		 c_r < MIN2(class_reg_count[c], i_r + class_sizes[i]);
		 c_r++) {

	       if (0) {
		  printf("%d/%d conflicts %d/%d\n",
			 class_sizes[i], first_assigned_grf + i_r,
			 class_sizes[c], first_assigned_grf + c_r);
	       }

	       ra_add_reg_conflict(regs,
				   class_base_reg[i] + i_r,
				   class_base_reg[c] + c_r);
	    }
	 }
      }
   }

   /* Add a special class for aligned pairs, which we'll put delta_x/y
    * in on gen5 so that we can do PLN.
    */
   if (brw->has_pln && reg_width == 1 && intel->gen < 6) {
      int reg_count = (base_reg_count - 1) / 2;
      int unaligned_pair_class = 1;
      assert(class_sizes[unaligned_pair_class] == 2);

      aligned_pair_class = class_count;
      classes[aligned_pair_class] = ra_alloc_reg_class(regs);
      class_sizes[aligned_pair_class] = 2;
      class_base_reg[aligned_pair_class] = 0;
      class_reg_count[aligned_pair_class] = 0;
      int start = (first_assigned_grf & 1) ? 1 : 0;

      for (int i = 0; i < reg_count; i++) {
	 ra_class_add_reg(regs, classes[aligned_pair_class],
			  class_base_reg[unaligned_pair_class] + i * 2 + start);
      }
      class_count++;
   }

   ra_set_finalize(regs);

   struct ra_graph *g = ra_alloc_interference_graph(regs,
						    this->virtual_grf_next);
   /* Node 0 is just a placeholder to keep virtual_grf[] mapping 1:1
    * with nodes.
    */
   ra_set_node_class(g, 0, classes[0]);

   for (int i = 1; i < this->virtual_grf_next; i++) {
      for (int c = 0; c < class_count; c++) {
	 if (class_sizes[c] == this->virtual_grf_sizes[i]) {
	    if (aligned_pair_class >= 0 &&
		this->delta_x.reg == i) {
	       ra_set_node_class(g, i, classes[aligned_pair_class]);
	    } else {
	       ra_set_node_class(g, i, classes[c]);
	    }
	    break;
	 }
      }

      for (int j = 1; j < i; j++) {
	 if (virtual_grf_interferes(i, j)) {
	    ra_add_node_interference(g, i, j);
	 }
      }
   }

   if (!ra_allocate_no_spills(g)) {
      /* Failed to allocate registers.  Spill a reg, and the caller will
       * loop back into here to try again.
       */
      int reg = choose_spill_reg(g);

      if (reg == -1) {
	 fail("no register to spill\n");
      } else if (intel->gen >= 7) {
	 fail("no spilling support on gen7 yet\n");
      } else if (c->dispatch_width == 16) {
	 fail("no spilling support on 16-wide yet\n");
      } else {
	 spill_reg(reg);
      }


      ralloc_free(g);
      ralloc_free(regs);

      return false;
   }

   /* Get the chosen virtual registers for each node, and map virtual
    * regs in the register classes back down to real hardware reg
    * numbers.
    */
   this->grf_used = first_assigned_grf;
   hw_reg_mapping[0] = 0; /* unused */
   for (int i = 1; i < this->virtual_grf_next; i++) {
      int reg = ra_get_node_reg(g, i);
      int hw_reg = -1;

      for (int c = 0; c < class_count; c++) {
	 if (reg >= class_base_reg[c] &&
	     reg < class_base_reg[c] + class_reg_count[c]) {
	    hw_reg = reg - class_base_reg[c];
	    break;
	 }
      }

      assert(hw_reg >= 0);
      hw_reg_mapping[i] = first_assigned_grf + hw_reg * reg_width;
      this->grf_used = MAX2(this->grf_used,
			    hw_reg_mapping[i] + this->virtual_grf_sizes[i] *
			    reg_width);
   }

   foreach_iter(exec_list_iterator, iter, this->instructions) {
      fs_inst *inst = (fs_inst *)iter.get();

      assign_reg(hw_reg_mapping, &inst->dst, reg_width);
      assign_reg(hw_reg_mapping, &inst->src[0], reg_width);
      assign_reg(hw_reg_mapping, &inst->src[1], reg_width);
   }

   ralloc_free(g);
   ralloc_free(regs);

   return true;
}

void
fs_visitor::emit_unspill(fs_inst *inst, fs_reg dst, uint32_t spill_offset)
{
   int size = virtual_grf_sizes[dst.reg];
   dst.reg_offset = 0;

   for (int chan = 0; chan < size; chan++) {
      fs_inst *unspill_inst = new(mem_ctx) fs_inst(FS_OPCODE_UNSPILL,
						   dst);
      dst.reg_offset++;
      unspill_inst->offset = spill_offset + chan * REG_SIZE;
      unspill_inst->ir = inst->ir;
      unspill_inst->annotation = inst->annotation;

      /* Choose a MRF that won't conflict with an MRF that's live across the
       * spill.  Nothing else will make it up to MRF 14/15.
       */
      unspill_inst->base_mrf = 14;
      unspill_inst->mlen = 1; /* header contains offset */
      inst->insert_before(unspill_inst);
   }
}

int
fs_visitor::choose_spill_reg(struct ra_graph *g)
{
   float loop_scale = 1.0;
   float spill_costs[this->virtual_grf_next];
   bool no_spill[this->virtual_grf_next];

   for (int i = 0; i < this->virtual_grf_next; i++) {
      spill_costs[i] = 0.0;
      no_spill[i] = false;
   }

   /* Calculate costs for spilling nodes.  Call it a cost of 1 per
    * spill/unspill we'll have to do, and guess that the insides of
    * loops run 10 times.
    */
   foreach_iter(exec_list_iterator, iter, this->instructions) {
      fs_inst *inst = (fs_inst *)iter.get();

      for (unsigned int i = 0; i < 3; i++) {
	 if (inst->src[i].file == GRF) {
	    int size = virtual_grf_sizes[inst->src[i].reg];
	    spill_costs[inst->src[i].reg] += size * loop_scale;
	 }
      }

      if (inst->dst.file == GRF) {
	 int size = virtual_grf_sizes[inst->dst.reg];
	 spill_costs[inst->dst.reg] += size * loop_scale;
      }

      switch (inst->opcode) {

      case BRW_OPCODE_DO:
	 loop_scale *= 10;
	 break;

      case BRW_OPCODE_WHILE:
	 loop_scale /= 10;
	 break;

      case FS_OPCODE_SPILL:
	 if (inst->src[0].file == GRF)
	    no_spill[inst->src[0].reg] = true;
	 break;

      case FS_OPCODE_UNSPILL:
	 if (inst->dst.file == GRF)
	    no_spill[inst->dst.reg] = true;
	 break;
      }
   }

   for (int i = 0; i < this->virtual_grf_next; i++) {
      if (!no_spill[i])
	 ra_set_node_spill_cost(g, i, spill_costs[i]);
   }

   return ra_get_best_spill_node(g);
}

void
fs_visitor::spill_reg(int spill_reg)
{
   int size = virtual_grf_sizes[spill_reg];
   unsigned int spill_offset = c->last_scratch;
   assert(ALIGN(spill_offset, 16) == spill_offset); /* oword read/write req. */
   c->last_scratch += size * REG_SIZE;

   /* Generate spill/unspill instructions for the objects being
    * spilled.  Right now, we spill or unspill the whole thing to a
    * virtual grf of the same size.  For most instructions, though, we
    * could just spill/unspill the GRF being accessed.
    */
   foreach_iter(exec_list_iterator, iter, this->instructions) {
      fs_inst *inst = (fs_inst *)iter.get();

      for (unsigned int i = 0; i < 3; i++) {
	 if (inst->src[i].file == GRF &&
	     inst->src[i].reg == spill_reg) {
	    inst->src[i].reg = virtual_grf_alloc(size);
	    emit_unspill(inst, inst->src[i], spill_offset);
	 }
      }

      if (inst->dst.file == GRF &&
	  inst->dst.reg == spill_reg) {
	 inst->dst.reg = virtual_grf_alloc(size);

	 /* Since we spill/unspill the whole thing even if we access
	  * just a component, we may need to unspill before the
	  * instruction we're spilling for.
	  */
	 if (size != 1 || inst->predicated) {
	    emit_unspill(inst, inst->dst, spill_offset);
	 }

	 fs_reg spill_src = inst->dst;
	 spill_src.reg_offset = 0;
	 spill_src.abs = false;
	 spill_src.negate = false;
	 spill_src.smear = -1;

	 for (int chan = 0; chan < size; chan++) {
	    fs_inst *spill_inst = new(mem_ctx) fs_inst(FS_OPCODE_SPILL,
						       reg_null_f, spill_src);
	    spill_src.reg_offset++;
	    spill_inst->offset = spill_offset + chan * REG_SIZE;
	    spill_inst->ir = inst->ir;
	    spill_inst->annotation = inst->annotation;
	    spill_inst->base_mrf = 14;
	    spill_inst->mlen = 2; /* header, value */
	    inst->insert_after(spill_inst);
	 }
      }
   }

   this->live_intervals_valid = false;
}
