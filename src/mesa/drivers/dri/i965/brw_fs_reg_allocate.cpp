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

#include "brw_fs.h"
#include "glsl/glsl_types.h"
#include "glsl/ir_optimization.h"
#include "glsl/ir_print_visitor.h"

static void
assign_reg(int *reg_hw_locations, fs_reg *reg, int reg_width)
{
   if (reg->file == GRF) {
      assert(reg->reg_offset >= 0);
      reg->reg = reg_hw_locations[reg->reg] + reg->reg_offset * reg_width;
      reg->reg_offset = 0;
   }
}

void
fs_visitor::assign_regs_trivial()
{
   int hw_reg_mapping[this->virtual_grf_count + 1];
   int i;
   int reg_width = dispatch_width / 8;

   /* Note that compressed instructions require alignment to 2 registers. */
   hw_reg_mapping[0] = ALIGN(this->first_non_payload_grf, reg_width);
   for (i = 1; i <= this->virtual_grf_count; i++) {
      hw_reg_mapping[i] = (hw_reg_mapping[i - 1] +
			   this->virtual_grf_sizes[i - 1] * reg_width);
   }
   this->grf_used = hw_reg_mapping[this->virtual_grf_count];

   foreach_list(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      assign_reg(hw_reg_mapping, &inst->dst, reg_width);
      assign_reg(hw_reg_mapping, &inst->src[0], reg_width);
      assign_reg(hw_reg_mapping, &inst->src[1], reg_width);
      assign_reg(hw_reg_mapping, &inst->src[2], reg_width);
   }

   if (this->grf_used >= max_grf) {
      fail("Ran out of regs on trivial allocator (%d/%d)\n",
	   this->grf_used, max_grf);
   }

}

static void
brw_alloc_reg_set(struct brw_context *brw, int reg_width)
{
   struct intel_context *intel = &brw->intel;
   int base_reg_count = BRW_MAX_GRF / reg_width;
   int index = reg_width - 1;

   /* The registers used to make up almost all values handled in the compiler
    * are a scalar value occupying a single register (or 2 registers in the
    * case of 16-wide, which is handled by dividing base_reg_count by 2 and
    * multiplying allocated register numbers by 2).  Things that were
    * aggregates of scalar values at the GLSL level were split to scalar
    * values by split_virtual_grfs().
    *
    * However, texture SEND messages return a series of contiguous registers.
    * We currently always ask for 4 registers, but we may convert that to use
    * less some day.
    *
    * Additionally, on gen5 we need aligned pairs of registers for the PLN
    * instruction, and on gen4 we need 8 contiguous regs for workaround simd16
    * texturing.
    *
    * So we have a need for classes for 1, 2, 4, and 8 registers currently,
    * and we add in '3' to make indexing the array easier for the common case
    * (since we'll probably want it for texturing later).
    */
   const int class_count = 5;
   const int class_sizes[class_count] = {1, 2, 3, 4, 8};

   /* Compute the total number of registers across all classes. */
   int ra_reg_count = 0;
   for (int i = 0; i < class_count; i++) {
      ra_reg_count += base_reg_count - (class_sizes[i] - 1);
   }

   uint8_t *ra_reg_to_grf = ralloc_array(brw, uint8_t, ra_reg_count);
   struct ra_regs *regs = ra_alloc_reg_set(brw, ra_reg_count);
   if (intel->gen >= 6)
      ra_set_allocate_round_robin(regs);
   int *classes = ralloc_array(brw, int, class_count);
   int aligned_pairs_class = -1;

   /* Now, add the registers to their classes, and add the conflicts
    * between them and the base GRF registers (and also each other).
    */
   int reg = 0;
   int pairs_base_reg = 0;
   int pairs_reg_count = 0;
   for (int i = 0; i < class_count; i++) {
      int class_reg_count = base_reg_count - (class_sizes[i] - 1);
      classes[i] = ra_alloc_reg_class(regs);

      /* Save this off for the aligned pair class at the end. */
      if (class_sizes[i] == 2) {
	 pairs_base_reg = reg;
	 pairs_reg_count = class_reg_count;
      }

      for (int j = 0; j < class_reg_count; j++) {
	 ra_class_add_reg(regs, classes[i], reg);

	 ra_reg_to_grf[reg] = j;

	 for (int base_reg = j;
	      base_reg < j + class_sizes[i];
	      base_reg++) {
	    ra_add_transitive_reg_conflict(regs, base_reg, reg);
	 }

	 reg++;
      }
   }
   assert(reg == ra_reg_count);

   /* Add a special class for aligned pairs, which we'll put delta_x/y
    * in on gen5 so that we can do PLN.
    */
   if (brw->has_pln && reg_width == 1 && intel->gen < 6) {
      aligned_pairs_class = ra_alloc_reg_class(regs);

      for (int i = 0; i < pairs_reg_count; i++) {
	 if ((ra_reg_to_grf[pairs_base_reg + i] & 1) == 0) {
	    ra_class_add_reg(regs, aligned_pairs_class, pairs_base_reg + i);
	 }
      }
   }

   ra_set_finalize(regs, NULL);

   brw->wm.reg_sets[index].regs = regs;
   brw->wm.reg_sets[index].classes = classes;
   brw->wm.reg_sets[index].ra_reg_to_grf = ra_reg_to_grf;
   brw->wm.reg_sets[index].aligned_pairs_class = aligned_pairs_class;
}

void
brw_fs_alloc_reg_sets(struct brw_context *brw)
{
   brw_alloc_reg_set(brw, 1);
   brw_alloc_reg_set(brw, 2);
}

int
count_to_loop_end(fs_inst *do_inst)
{
   int depth = 1;
   int ip = 1;
   for (fs_inst *inst = (fs_inst *)do_inst->next;
        depth > 0;
        inst = (fs_inst *)inst->next) {
      switch (inst->opcode) {
      case BRW_OPCODE_DO:
         depth++;
         break;
      case BRW_OPCODE_WHILE:
         depth--;
         break;
      default:
         break;
      }
      ip++;
   }
   return ip;
}

/**
 * Sets up interference between thread payload registers and the virtual GRFs
 * to be allocated for program temporaries.
 *
 * We want to be able to reallocate the payload for our virtual GRFs, notably
 * because the setup coefficients for a full set of 16 FS inputs takes up 8 of
 * our 128 registers.
 *
 * The layout of the payload registers is:
 *
 * 0..nr_payload_regs-1: fixed function setup (including bary coordinates).
 * nr_payload_regs..nr_payload_regs+curb_read_lengh-1: uniform data
 * nr_payload_regs+curb_read_lengh..first_non_payload_grf-1: setup coefficients.
 *
 * And we have payload_node_count nodes covering these registers in order
 * (note that in 16-wide, a node is two registers).
 */
void
fs_visitor::setup_payload_interference(struct ra_graph *g,
                                       int payload_node_count,
                                       int first_payload_node)
{
   int reg_width = dispatch_width / 8;
   int loop_depth = 0;
   int loop_end_ip = 0;

   int payload_last_use_ip[payload_node_count];
   memset(payload_last_use_ip, 0, sizeof(payload_last_use_ip));
   int ip = 0;
   foreach_list(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      switch (inst->opcode) {
      case BRW_OPCODE_DO:
         loop_depth++;

         /* Since payload regs are deffed only at the start of the shader
          * execution, any uses of the payload within a loop mean the live
          * interval extends to the end of the outermost loop.  Find the ip of
          * the end now.
          */
         if (loop_depth == 1)
            loop_end_ip = ip + count_to_loop_end(inst);
         break;
      case BRW_OPCODE_WHILE:
         loop_depth--;
         break;
      default:
         break;
      }

      int use_ip;
      if (loop_depth > 0)
         use_ip = loop_end_ip;
      else
         use_ip = ip;

      /* Note that UNIFORM args have been turned into FIXED_HW_REG by
       * assign_curbe_setup(), and interpolation uses fixed hardware regs from
       * the start (see interp_reg()).
       */
      for (int i = 0; i < 3; i++) {
         if (inst->src[i].file == FIXED_HW_REG &&
             inst->src[i].fixed_hw_reg.file == BRW_GENERAL_REGISTER_FILE) {
            int node_nr = inst->src[i].fixed_hw_reg.nr / reg_width;
            if (node_nr >= payload_node_count)
               continue;

            payload_last_use_ip[node_nr] = use_ip;
         }
      }

      /* Special case instructions which have extra implied registers used. */
      switch (inst->opcode) {
      case FS_OPCODE_FB_WRITE:
         /* We could omit this for the !inst->header_present case, except that
          * the simulator apparently incorrectly reads from g0/g1 instead of
          * sideband.  It also really freaks out driver developers to see g0
          * used in unusual places, so just always reserve it.
          */
         payload_last_use_ip[0 / reg_width] = use_ip;
         payload_last_use_ip[1 / reg_width] = use_ip;
         break;

      case FS_OPCODE_LINTERP:
         /* On gen6+ in 16-wide, there are 4 adjacent registers (so 2 nodes)
          * used by PLN's sourcing of the deltas, while we list only the first
          * two in the arguments (1 node).  Pre-gen6, the deltas are computed
          * in normal VGRFs.
          */
         if (intel->gen >= 6) {
            int delta_x_arg = 0;
            if (inst->src[delta_x_arg].file == FIXED_HW_REG &&
                inst->src[delta_x_arg].fixed_hw_reg.file ==
                BRW_GENERAL_REGISTER_FILE) {
               int sechalf_node = (inst->src[delta_x_arg].fixed_hw_reg.nr /
                                   reg_width) + 1;
               assert(sechalf_node < payload_node_count);
               payload_last_use_ip[sechalf_node] = use_ip;
            }
         }
         break;

      default:
         break;
      }

      ip++;
   }

   for (int i = 0; i < payload_node_count; i++) {
      /* Mark the payload node as interfering with any virtual grf that is
       * live between the start of the program and our last use of the payload
       * node.
       */
      for (int j = 0; j < this->virtual_grf_count; j++) {
         /* Note that we use a <= comparison, unlike virtual_grf_interferes(),
          * in order to not have to worry about the uniform issue described in
          * calculate_live_intervals().
          */
         if (this->virtual_grf_def[j] <= payload_last_use_ip[i] ||
             this->virtual_grf_use[j] <= payload_last_use_ip[i]) {
            ra_add_node_interference(g, first_payload_node + i, j);
         }
      }
   }

   for (int i = 0; i < payload_node_count; i++) {
      /* Mark each payload node as being allocated to its physical register.
       *
       * The alternative would be to have per-physical-register classes, which
       * would just be silly.
       */
      ra_set_node_reg(g, first_payload_node + i, i);
   }
}

/**
 * Sets interference between virtual GRFs and usage of the high GRFs for SEND
 * messages (treated as MRFs in code generation).
 */
void
fs_visitor::setup_mrf_hack_interference(struct ra_graph *g, int first_mrf_node)
{
   int mrf_count = BRW_MAX_GRF - GEN7_MRF_HACK_START;
   int reg_width = dispatch_width / 8;

   /* Identify all the MRFs used in the program. */
   bool mrf_used[mrf_count];
   memset(mrf_used, 0, sizeof(mrf_used));
   foreach_list(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      if (inst->dst.file == MRF) {
         int reg = inst->dst.reg & ~BRW_MRF_COMPR4;
         mrf_used[reg] = true;
         if (reg_width == 2) {
            if (inst->dst.reg & BRW_MRF_COMPR4) {
               mrf_used[reg + 4] = true;
            } else {
               mrf_used[reg + 1] = true;
            }
         }
      }

      if (inst->mlen > 0) {
	 for (int i = 0; i < implied_mrf_writes(inst); i++) {
            mrf_used[inst->base_mrf + i] = true;
         }
      }
   }

   for (int i = 0; i < mrf_count; i++) {
      /* Mark each payload reg node as being allocated to its physical register.
       *
       * The alternative would be to have per-physical-register classes, which
       * would just be silly.
       */
      ra_set_node_reg(g, first_mrf_node + i,
                      (GEN7_MRF_HACK_START + i) / reg_width);

      /* Since we don't have any live/dead analysis on the MRFs, just mark all
       * that are used as conflicting with all virtual GRFs.
       */
      if (mrf_used[i]) {
         for (int j = 0; j < this->virtual_grf_count; j++) {
            ra_add_node_interference(g, first_mrf_node + i, j);
         }
      }
   }
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
   int reg_width = dispatch_width / 8;
   int hw_reg_mapping[this->virtual_grf_count];
   int payload_node_count = (ALIGN(this->first_non_payload_grf, reg_width) /
                            reg_width);
   int rsi = reg_width - 1; /* Which brw->wm.reg_sets[] to use */
   calculate_live_intervals();

   int node_count = this->virtual_grf_count;
   int first_payload_node = node_count;
   node_count += payload_node_count;
   int first_mrf_hack_node = node_count;
   if (intel->gen >= 7)
      node_count += BRW_MAX_GRF - GEN7_MRF_HACK_START;
   struct ra_graph *g = ra_alloc_interference_graph(brw->wm.reg_sets[rsi].regs,
                                                    node_count);

   for (int i = 0; i < this->virtual_grf_count; i++) {
      int size = this->virtual_grf_sizes[i];
      int c;

      if (size == 8) {
         c = 4;
      } else {
         assert(size >= 1 &&
                size <= 4 &&
                "Register allocation relies on split_virtual_grfs()");
         c = brw->wm.reg_sets[rsi].classes[size - 1];
      }

      /* Special case: on pre-GEN6 hardware that supports PLN, the
       * second operand of a PLN instruction needs to be an
       * even-numbered register, so we have a special register class
       * wm_aligned_pairs_class to handle this case.  pre-GEN6 always
       * uses this->delta_x[BRW_WM_PERSPECTIVE_PIXEL_BARYCENTRIC] as the
       * second operand of a PLN instruction (since it doesn't support
       * any other interpolation modes).  So all we need to do is find
       * that register and set it to the appropriate class.
       */
      if (brw->wm.reg_sets[rsi].aligned_pairs_class >= 0 &&
          this->delta_x[BRW_WM_PERSPECTIVE_PIXEL_BARYCENTRIC].reg == i) {
         c = brw->wm.reg_sets[rsi].aligned_pairs_class;
      }

      ra_set_node_class(g, i, c);

      for (int j = 0; j < i; j++) {
	 if (virtual_grf_interferes(i, j)) {
	    ra_add_node_interference(g, i, j);
	 }
      }
   }

   setup_payload_interference(g, payload_node_count, first_payload_node);
   if (intel->gen >= 7)
      setup_mrf_hack_interference(g, first_mrf_hack_node);

   if (!ra_allocate_no_spills(g)) {
      /* Failed to allocate registers.  Spill a reg, and the caller will
       * loop back into here to try again.
       */
      int reg = choose_spill_reg(g);

      if (reg == -1) {
	 fail("no register to spill\n");
      } else if (dispatch_width == 16) {
	 fail("Failure to register allocate.  Reduce number of live scalar "
              "values to avoid this.");
      } else {
	 spill_reg(reg);
      }


      ralloc_free(g);

      return false;
   }

   /* Get the chosen virtual registers for each node, and map virtual
    * regs in the register classes back down to real hardware reg
    * numbers.
    */
   this->grf_used = payload_node_count * reg_width;
   for (int i = 0; i < this->virtual_grf_count; i++) {
      int reg = ra_get_node_reg(g, i);

      hw_reg_mapping[i] = brw->wm.reg_sets[rsi].ra_reg_to_grf[reg] * reg_width;
      this->grf_used = MAX2(this->grf_used,
			    hw_reg_mapping[i] + this->virtual_grf_sizes[i] *
			    reg_width);
   }

   foreach_list(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      assign_reg(hw_reg_mapping, &inst->dst, reg_width);
      assign_reg(hw_reg_mapping, &inst->src[0], reg_width);
      assign_reg(hw_reg_mapping, &inst->src[1], reg_width);
      assign_reg(hw_reg_mapping, &inst->src[2], reg_width);
   }

   ralloc_free(g);

   return true;
}

void
fs_visitor::emit_unspill(fs_inst *inst, fs_reg dst, uint32_t spill_offset)
{
   fs_inst *unspill_inst = new(mem_ctx) fs_inst(FS_OPCODE_UNSPILL, dst);
   unspill_inst->offset = spill_offset;
   unspill_inst->ir = inst->ir;
   unspill_inst->annotation = inst->annotation;

   /* Choose a MRF that won't conflict with an MRF that's live across the
    * spill.  Nothing else will make it up to MRF 14/15.
    */
   unspill_inst->base_mrf = 14;
   unspill_inst->mlen = 1; /* header contains offset */
   inst->insert_before(unspill_inst);
}

int
fs_visitor::choose_spill_reg(struct ra_graph *g)
{
   float loop_scale = 1.0;
   float spill_costs[this->virtual_grf_count];
   bool no_spill[this->virtual_grf_count];

   for (int i = 0; i < this->virtual_grf_count; i++) {
      spill_costs[i] = 0.0;
      no_spill[i] = false;
   }

   /* Calculate costs for spilling nodes.  Call it a cost of 1 per
    * spill/unspill we'll have to do, and guess that the insides of
    * loops run 10 times.
    */
   foreach_list(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      for (unsigned int i = 0; i < 3; i++) {
	 if (inst->src[i].file == GRF) {
	    spill_costs[inst->src[i].reg] += loop_scale;

            /* Register spilling logic assumes full-width registers; smeared
             * registers have a width of 1 so if we try to spill them we'll
             * generate invalid assembly.  This shouldn't be a problem because
             * smeared registers are only used as short-term temporaries when
             * loading pull constants, so spilling them is unlikely to reduce
             * register pressure anyhow.
             */
            if (inst->src[i].smear >= 0) {
               no_spill[inst->src[i].reg] = true;
            }
	 }
      }

      if (inst->dst.file == GRF) {
	 spill_costs[inst->dst.reg] += inst->regs_written * loop_scale;

         if (inst->dst.smear >= 0) {
            no_spill[inst->dst.reg] = true;
         }
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

      default:
	 break;
      }
   }

   for (int i = 0; i < this->virtual_grf_count; i++) {
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
   foreach_list(node, &this->instructions) {
      fs_inst *inst = (fs_inst *)node;

      for (unsigned int i = 0; i < 3; i++) {
	 if (inst->src[i].file == GRF &&
	     inst->src[i].reg == spill_reg) {
	    inst->src[i].reg = virtual_grf_alloc(1);
	    emit_unspill(inst, inst->src[i],
                         spill_offset + REG_SIZE * inst->src[i].reg_offset);
	 }
      }

      if (inst->dst.file == GRF &&
	  inst->dst.reg == spill_reg) {
         int subset_spill_offset = (spill_offset +
                                    REG_SIZE * inst->dst.reg_offset);
         inst->dst.reg = virtual_grf_alloc(inst->regs_written);
         inst->dst.reg_offset = 0;

	 /* If our write is going to affect just part of the
          * inst->regs_written(), then we need to unspill the destination
          * since we write back out all of the regs_written().
	  */
	 if (inst->predicate || inst->force_uncompressed || inst->force_sechalf) {
            fs_reg unspill_reg = inst->dst;
            for (int chan = 0; chan < inst->regs_written; chan++) {
               emit_unspill(inst, unspill_reg,
                            subset_spill_offset + REG_SIZE * chan);
               unspill_reg.reg_offset++;
            }
	 }

	 fs_reg spill_src = inst->dst;
	 spill_src.reg_offset = 0;
	 spill_src.abs = false;
	 spill_src.negate = false;
	 spill_src.smear = -1;

	 for (int chan = 0; chan < inst->regs_written; chan++) {
	    fs_inst *spill_inst = new(mem_ctx) fs_inst(FS_OPCODE_SPILL,
						       reg_null_f, spill_src);
	    spill_src.reg_offset++;
	    spill_inst->offset = subset_spill_offset + chan * REG_SIZE;
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
