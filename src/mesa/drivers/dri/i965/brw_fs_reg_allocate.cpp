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
brw_alloc_reg_set(struct intel_screen *screen, int reg_width)
{
   const struct brw_device_info *devinfo = screen->devinfo;
   int base_reg_count = BRW_MAX_GRF / reg_width;
   int index = reg_width - 1;

   /* The registers used to make up almost all values handled in the compiler
    * are a scalar value occupying a single register (or 2 registers in the
    * case of SIMD16, which is handled by dividing base_reg_count by 2 and
    * multiplying allocated register numbers by 2).  Things that were
    * aggregates of scalar values at the GLSL level were split to scalar
    * values by split_virtual_grfs().
    *
    * However, texture SEND messages return a series of contiguous registers
    * to write into.  We currently always ask for 4 registers, but we may
    * convert that to use less some day.
    *
    * Additionally, on gen5 we need aligned pairs of registers for the PLN
    * instruction, and on gen4 we need 8 contiguous regs for workaround simd16
    * texturing.
    *
    * So we have a need for classes for 1, 2, 4, and 8 registers currently,
    * and we add in '3' to make indexing the array easier for the common case
    * (since we'll probably want it for texturing later).
    *
    * And, on gen7 and newer, we do texturing SEND messages from GRFs, which
    * means that we may need any size up to the sampler message size limit (11
    * regs).
    */
   int class_count;
   int class_sizes[BRW_MAX_MRF];

   if (devinfo->gen >= 7) {
      for (class_count = 0; class_count < MAX_SAMPLER_MESSAGE_SIZE;
           class_count++)
         class_sizes[class_count] = class_count + 1;
   } else {
      for (class_count = 0; class_count < 4; class_count++)
         class_sizes[class_count] = class_count + 1;
      class_sizes[class_count++] = 8;
   }

   /* Compute the total number of registers across all classes. */
   int ra_reg_count = 0;
   for (int i = 0; i < class_count; i++) {
      ra_reg_count += base_reg_count - (class_sizes[i] - 1);
   }

   uint8_t *ra_reg_to_grf = ralloc_array(screen, uint8_t, ra_reg_count);
   struct ra_regs *regs = ra_alloc_reg_set(screen, ra_reg_count);
   if (devinfo->gen >= 6)
      ra_set_allocate_round_robin(regs);
   int *classes = ralloc_array(screen, int, class_count);
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
   if (devinfo->has_pln && reg_width == 1 && devinfo->gen < 6) {
      aligned_pairs_class = ra_alloc_reg_class(regs);

      for (int i = 0; i < pairs_reg_count; i++) {
	 if ((ra_reg_to_grf[pairs_base_reg + i] & 1) == 0) {
	    ra_class_add_reg(regs, aligned_pairs_class, pairs_base_reg + i);
	 }
      }
   }

   ra_set_finalize(regs, NULL);

   screen->wm_reg_sets[index].regs = regs;
   for (unsigned i = 0; i < ARRAY_SIZE(screen->wm_reg_sets[index].classes); i++)
      screen->wm_reg_sets[index].classes[i] = -1;
   for (int i = 0; i < class_count; i++)
      screen->wm_reg_sets[index].classes[class_sizes[i] - 1] = classes[i];
   screen->wm_reg_sets[index].ra_reg_to_grf = ra_reg_to_grf;
   screen->wm_reg_sets[index].aligned_pairs_class = aligned_pairs_class;
}

void
brw_fs_alloc_reg_sets(struct intel_screen *screen)
{
   brw_alloc_reg_set(screen, 1);
   brw_alloc_reg_set(screen, 2);
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
 * (note that in SIMD16, a node is two registers).
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
         if (inst->src[i].file == HW_REG &&
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
         /* On gen6+ in SIMD16, there are 4 adjacent registers (so 2 nodes)
          * used by PLN's sourcing of the deltas, while we list only the first
          * two in the arguments (1 node).  Pre-gen6, the deltas are computed
          * in normal VGRFs.
          */
         if (brw->gen >= 6) {
            int delta_x_arg = 0;
            if (inst->src[delta_x_arg].file == HW_REG &&
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
         if (this->virtual_grf_start[j] <= payload_last_use_ip[i]) {
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
 * Sets the mrf_used array to indicate which MRFs are used by the shader IR
 *
 * This is used in assign_regs() to decide which of the GRFs that we use as
 * MRFs on gen7 get normally register allocated, and in register spilling to
 * see if we can actually use MRFs to do spills without overwriting normal MRF
 * contents.
 */
void
fs_visitor::get_used_mrfs(bool *mrf_used)
{
   int reg_width = dispatch_width / 8;

   memset(mrf_used, 0, BRW_MAX_MRF * sizeof(bool));

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
}

/**
 * Sets interference between virtual GRFs and usage of the high GRFs for SEND
 * messages (treated as MRFs in code generation).
 */
void
fs_visitor::setup_mrf_hack_interference(struct ra_graph *g, int first_mrf_node)
{
   int reg_width = dispatch_width / 8;

   bool mrf_used[BRW_MAX_MRF];
   get_used_mrfs(mrf_used);

   for (int i = 0; i < BRW_MAX_MRF; i++) {
      /* Mark each MRF reg node as being allocated to its physical register.
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
fs_visitor::assign_regs(bool allow_spilling)
{
   struct intel_screen *screen = brw->intelScreen;
   /* Most of this allocation was written for a reg_width of 1
    * (dispatch_width == 8).  In extending to SIMD16, the code was
    * left in place and it was converted to have the hardware
    * registers it's allocating be contiguous physical pairs of regs
    * for reg_width == 2.
    */
   int reg_width = dispatch_width / 8;
   int hw_reg_mapping[this->virtual_grf_count];
   int payload_node_count = (ALIGN(this->first_non_payload_grf, reg_width) /
                            reg_width);
   int rsi = reg_width - 1; /* Which screen->wm_reg_sets[] to use */
   calculate_live_intervals();

   int node_count = this->virtual_grf_count;
   int first_payload_node = node_count;
   node_count += payload_node_count;
   int first_mrf_hack_node = node_count;
   if (brw->gen >= 7)
      node_count += BRW_MAX_GRF - GEN7_MRF_HACK_START;
   struct ra_graph *g = ra_alloc_interference_graph(screen->wm_reg_sets[rsi].regs,
                                                    node_count);

   for (int i = 0; i < this->virtual_grf_count; i++) {
      unsigned size = this->virtual_grf_sizes[i];
      int c;

      assert(size <= ARRAY_SIZE(screen->wm_reg_sets[rsi].classes) &&
             "Register allocation relies on split_virtual_grfs()");
      c = screen->wm_reg_sets[rsi].classes[size - 1];

      /* Special case: on pre-GEN6 hardware that supports PLN, the
       * second operand of a PLN instruction needs to be an
       * even-numbered register, so we have a special register class
       * wm_aligned_pairs_class to handle this case.  pre-GEN6 always
       * uses this->delta_x[BRW_WM_PERSPECTIVE_PIXEL_BARYCENTRIC] as the
       * second operand of a PLN instruction (since it doesn't support
       * any other interpolation modes).  So all we need to do is find
       * that register and set it to the appropriate class.
       */
      if (screen->wm_reg_sets[rsi].aligned_pairs_class >= 0 &&
          this->delta_x[BRW_WM_PERSPECTIVE_PIXEL_BARYCENTRIC].reg == i) {
         c = screen->wm_reg_sets[rsi].aligned_pairs_class;
      }

      ra_set_node_class(g, i, c);

      for (int j = 0; j < i; j++) {
	 if (virtual_grf_interferes(i, j)) {
	    ra_add_node_interference(g, i, j);
	 }
      }
   }

   setup_payload_interference(g, payload_node_count, first_payload_node);
   if (brw->gen >= 7)
      setup_mrf_hack_interference(g, first_mrf_hack_node);

   /* Debug of register spilling: Go spill everything. */
   if (0) {
      int reg = choose_spill_reg(g);

      if (reg != -1) {
         spill_reg(reg);
         ralloc_free(g);
         return false;
      }
   }

   if (!ra_allocate_no_spills(g)) {
      /* Failed to allocate registers.  Spill a reg, and the caller will
       * loop back into here to try again.
       */
      int reg = choose_spill_reg(g);

      if (reg == -1) {
         fail("no register to spill:\n");
         dump_instructions();
      } else if (allow_spilling) {
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

      hw_reg_mapping[i] = screen->wm_reg_sets[rsi].ra_reg_to_grf[reg] * reg_width;
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
fs_visitor::emit_unspill(fs_inst *inst, fs_reg dst, uint32_t spill_offset,
                         int count)
{
   for (int i = 0; i < count; i++) {
      /* The gen7 descriptor-based offset is 12 bits of HWORD units. */
      bool gen7_read = brw->gen >= 7 && spill_offset < (1 << 12) * REG_SIZE;

      fs_inst *unspill_inst =
         new(mem_ctx) fs_inst(gen7_read ?
                              SHADER_OPCODE_GEN7_SCRATCH_READ :
                              SHADER_OPCODE_GEN4_SCRATCH_READ,
                              dst);
      unspill_inst->offset = spill_offset;
      unspill_inst->ir = inst->ir;
      unspill_inst->annotation = inst->annotation;

      if (!gen7_read) {
         unspill_inst->base_mrf = 14;
         unspill_inst->mlen = 1; /* header contains offset */
      }
      inst->insert_before(unspill_inst);

      dst.reg_offset++;
      spill_offset += dispatch_width * sizeof(float);
   }
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
            if (!inst->src[i].is_contiguous()) {
               no_spill[inst->src[i].reg] = true;
            }
	 }
      }

      if (inst->dst.file == GRF) {
	 spill_costs[inst->dst.reg] += inst->regs_written * loop_scale;

         if (!inst->dst.is_contiguous()) {
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

      case SHADER_OPCODE_GEN4_SCRATCH_WRITE:
	 if (inst->src[0].file == GRF)
	    no_spill[inst->src[0].reg] = true;
	 break;

      case SHADER_OPCODE_GEN4_SCRATCH_READ:
      case SHADER_OPCODE_GEN7_SCRATCH_READ:
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
   int reg_size = dispatch_width * sizeof(float);
   int size = virtual_grf_sizes[spill_reg];
   unsigned int spill_offset = c->last_scratch;
   assert(ALIGN(spill_offset, 16) == spill_offset); /* oword read/write req. */
   int spill_base_mrf = dispatch_width > 8 ? 13 : 14;

   /* Spills may use MRFs 13-15 in the SIMD16 case.  Our texturing is done
    * using up to 11 MRFs starting from either m1 or m2, and fb writes can use
    * up to m13 (gen6+ simd16: 2 header + 8 color + 2 src0alpha + 2 omask) or
    * m15 (gen4-5 simd16: 2 header + 8 color + 1 aads + 2 src depth + 2 dst
    * depth), starting from m1.  In summary: We may not be able to spill in
    * SIMD16 mode, because we'd stomp the FB writes.
    */
   if (!spilled_any_registers) {
      bool mrf_used[BRW_MAX_MRF];
      get_used_mrfs(mrf_used);

      for (int i = spill_base_mrf; i < BRW_MAX_MRF; i++) {
         if (mrf_used[i]) {
            fail("Register spilling not supported with m%d used", i);
          return;
         }
      }

      spilled_any_registers = true;
   }

   c->last_scratch += size * reg_size;

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
            int regs_read = inst->regs_read(this, i);
            int subset_spill_offset = (spill_offset +
                                       reg_size * inst->src[i].reg_offset);
            fs_reg unspill_dst(GRF, virtual_grf_alloc(regs_read));

            inst->src[i].reg = unspill_dst.reg;
            inst->src[i].reg_offset = 0;

            emit_unspill(inst, unspill_dst, subset_spill_offset, regs_read);
	 }
      }

      if (inst->dst.file == GRF &&
	  inst->dst.reg == spill_reg) {
         int subset_spill_offset = (spill_offset +
                                    reg_size * inst->dst.reg_offset);
         fs_reg spill_src(GRF, virtual_grf_alloc(inst->regs_written));

         inst->dst.reg = spill_src.reg;
         inst->dst.reg_offset = 0;

	 /* If our write is going to affect just part of the
          * inst->regs_written(), then we need to unspill the destination
          * since we write back out all of the regs_written().
	  */
	 if (inst->predicate || inst->force_uncompressed ||
             inst->force_sechalf || inst->dst.subreg_offset) {
            emit_unspill(inst, spill_src, subset_spill_offset,
                         inst->regs_written);
	 }

	 for (int chan = 0; chan < inst->regs_written; chan++) {
	    fs_inst *spill_inst =
               new(mem_ctx) fs_inst(SHADER_OPCODE_GEN4_SCRATCH_WRITE,
                                    reg_null_f, spill_src);
	    spill_src.reg_offset++;
	    spill_inst->offset = subset_spill_offset + chan * reg_size;
	    spill_inst->ir = inst->ir;
	    spill_inst->annotation = inst->annotation;
	    spill_inst->mlen = 1 + dispatch_width / 8; /* header, value */
	    spill_inst->base_mrf = spill_base_mrf;
	    inst->insert_after(spill_inst);
	 }
      }
   }

   invalidate_live_intervals();
}
