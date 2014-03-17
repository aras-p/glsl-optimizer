/*
 * Copyright © 2011 Intel Corporation
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

extern "C" {
#include "main/macros.h"
#include "program/register_allocate.h"
} /* extern "C" */

#include "brw_vec4.h"
#include "brw_vs.h"

using namespace brw;

namespace brw {

static void
assign(unsigned int *reg_hw_locations, reg *reg)
{
   if (reg->file == GRF) {
      reg->reg = reg_hw_locations[reg->reg];
   }
}

bool
vec4_visitor::reg_allocate_trivial()
{
   unsigned int hw_reg_mapping[this->virtual_grf_count];
   bool virtual_grf_used[this->virtual_grf_count];
   int i;
   int next;

   /* Calculate which virtual GRFs are actually in use after whatever
    * optimization passes have occurred.
    */
   for (int i = 0; i < this->virtual_grf_count; i++) {
      virtual_grf_used[i] = false;
   }

   foreach_list(node, &this->instructions) {
      vec4_instruction *inst = (vec4_instruction *) node;

      if (inst->dst.file == GRF)
	 virtual_grf_used[inst->dst.reg] = true;

      for (int i = 0; i < 3; i++) {
	 if (inst->src[i].file == GRF)
	    virtual_grf_used[inst->src[i].reg] = true;
      }
   }

   hw_reg_mapping[0] = this->first_non_payload_grf;
   next = hw_reg_mapping[0] + this->virtual_grf_sizes[0];
   for (i = 1; i < this->virtual_grf_count; i++) {
      if (virtual_grf_used[i]) {
	 hw_reg_mapping[i] = next;
	 next += this->virtual_grf_sizes[i];
      }
   }
   prog_data->total_grf = next;

   foreach_list(node, &this->instructions) {
      vec4_instruction *inst = (vec4_instruction *) node;

      assign(hw_reg_mapping, &inst->dst);
      assign(hw_reg_mapping, &inst->src[0]);
      assign(hw_reg_mapping, &inst->src[1]);
      assign(hw_reg_mapping, &inst->src[2]);
   }

   if (prog_data->total_grf > max_grf) {
      fail("Ran out of regs on trivial allocator (%d/%d)\n",
	   prog_data->total_grf, max_grf);
      return false;
   }

   return true;
}

extern "C" void
brw_vec4_alloc_reg_set(struct intel_screen *screen)
{
   int base_reg_count =
      screen->devinfo->gen >= 7 ? GEN7_MRF_HACK_START : BRW_MAX_GRF;

   /* After running split_virtual_grfs(), almost all VGRFs will be of size 1.
    * SEND-from-GRF sources cannot be split, so we also need classes for each
    * potential message length.
    */
   const int class_count = 2;
   const int class_sizes[class_count] = {1, 2};

   /* Compute the total number of registers across all classes. */
   int ra_reg_count = 0;
   for (int i = 0; i < class_count; i++) {
      ra_reg_count += base_reg_count - (class_sizes[i] - 1);
   }

   ralloc_free(screen->vec4_reg_set.ra_reg_to_grf);
   screen->vec4_reg_set.ra_reg_to_grf = ralloc_array(screen, uint8_t, ra_reg_count);
   ralloc_free(screen->vec4_reg_set.regs);
   screen->vec4_reg_set.regs = ra_alloc_reg_set(screen, ra_reg_count);
   if (screen->devinfo->gen >= 6)
      ra_set_allocate_round_robin(screen->vec4_reg_set.regs);
   ralloc_free(screen->vec4_reg_set.classes);
   screen->vec4_reg_set.classes = ralloc_array(screen, int, class_count);

   /* Now, add the registers to their classes, and add the conflicts
    * between them and the base GRF registers (and also each other).
    */
   int reg = 0;
   for (int i = 0; i < class_count; i++) {
      int class_reg_count = base_reg_count - (class_sizes[i] - 1);
      screen->vec4_reg_set.classes[i] = ra_alloc_reg_class(screen->vec4_reg_set.regs);

      for (int j = 0; j < class_reg_count; j++) {
	 ra_class_add_reg(screen->vec4_reg_set.regs, screen->vec4_reg_set.classes[i], reg);

	 screen->vec4_reg_set.ra_reg_to_grf[reg] = j;

	 for (int base_reg = j;
	      base_reg < j + class_sizes[i];
	      base_reg++) {
	    ra_add_transitive_reg_conflict(screen->vec4_reg_set.regs, base_reg, reg);
	 }

	 reg++;
      }
   }
   assert(reg == ra_reg_count);

   ra_set_finalize(screen->vec4_reg_set.regs, NULL);
}

void
vec4_visitor::setup_payload_interference(struct ra_graph *g,
                                         int first_payload_node,
                                         int reg_node_count)
{
   int payload_node_count = this->first_non_payload_grf;

   for (int i = 0; i < payload_node_count; i++) {
      /* Mark each payload reg node as being allocated to its physical register.
       *
       * The alternative would be to have per-physical register classes, which
       * would just be silly.
       */
      ra_set_node_reg(g, first_payload_node + i, i);

      /* For now, just mark each payload node as interfering with every other
       * node to be allocated.
       */
      for (int j = 0; j < reg_node_count; j++) {
         ra_add_node_interference(g, first_payload_node + i, j);
      }
   }
}

bool
vec4_visitor::reg_allocate()
{
   struct intel_screen *screen = brw->intelScreen;
   unsigned int hw_reg_mapping[virtual_grf_count];
   int payload_reg_count = this->first_non_payload_grf;

   /* Using the trivial allocator can be useful in debugging undefined
    * register access as a result of broken optimization passes.
    */
   if (0)
      return reg_allocate_trivial();

   calculate_live_intervals();

   int node_count = virtual_grf_count;
   int first_payload_node = node_count;
   node_count += payload_reg_count;
   struct ra_graph *g =
      ra_alloc_interference_graph(screen->vec4_reg_set.regs, node_count);

   for (int i = 0; i < virtual_grf_count; i++) {
      int size = this->virtual_grf_sizes[i];
      assert(size >= 1 && size <= 2 &&
             "Register allocation relies on split_virtual_grfs().");
      ra_set_node_class(g, i, screen->vec4_reg_set.classes[size - 1]);

      for (int j = 0; j < i; j++) {
	 if (virtual_grf_interferes(i, j)) {
	    ra_add_node_interference(g, i, j);
	 }
      }
   }

   setup_payload_interference(g, first_payload_node, node_count);

   if (!ra_allocate_no_spills(g)) {
      /* Failed to allocate registers.  Spill a reg, and the caller will
       * loop back into here to try again.
       */
      int reg = choose_spill_reg(g);
      if (this->no_spills) {
         fail("Failure to register allocate.  Reduce number of live "
              "values to avoid this.");
      } else if (reg == -1) {
         fail("no register to spill\n");
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
   prog_data->total_grf = payload_reg_count;
   for (int i = 0; i < virtual_grf_count; i++) {
      int reg = ra_get_node_reg(g, i);

      hw_reg_mapping[i] = screen->vec4_reg_set.ra_reg_to_grf[reg];
      prog_data->total_grf = MAX2(prog_data->total_grf,
				  hw_reg_mapping[i] + virtual_grf_sizes[i]);
   }

   foreach_list(node, &this->instructions) {
      vec4_instruction *inst = (vec4_instruction *)node;

      assign(hw_reg_mapping, &inst->dst);
      assign(hw_reg_mapping, &inst->src[0]);
      assign(hw_reg_mapping, &inst->src[1]);
      assign(hw_reg_mapping, &inst->src[2]);
   }

   ralloc_free(g);

   return true;
}

void
vec4_visitor::evaluate_spill_costs(float *spill_costs, bool *no_spill)
{
   float loop_scale = 1.0;

   for (int i = 0; i < this->virtual_grf_count; i++) {
      spill_costs[i] = 0.0;
      no_spill[i] = virtual_grf_sizes[i] != 1;
   }

   /* Calculate costs for spilling nodes.  Call it a cost of 1 per
    * spill/unspill we'll have to do, and guess that the insides of
    * loops run 10 times.
    */
   foreach_list(node, &this->instructions) {
      vec4_instruction *inst = (vec4_instruction *) node;

      for (unsigned int i = 0; i < 3; i++) {
	 if (inst->src[i].file == GRF) {
	    spill_costs[inst->src[i].reg] += loop_scale;
            if (inst->src[i].reladdr)
               no_spill[inst->src[i].reg] = true;
	 }
      }

      if (inst->dst.file == GRF) {
	 spill_costs[inst->dst.reg] += loop_scale;
         if (inst->dst.reladdr)
            no_spill[inst->dst.reg] = true;
      }

      switch (inst->opcode) {

      case BRW_OPCODE_DO:
	 loop_scale *= 10;
	 break;

      case BRW_OPCODE_WHILE:
	 loop_scale /= 10;
	 break;

      case SHADER_OPCODE_GEN4_SCRATCH_READ:
      case SHADER_OPCODE_GEN4_SCRATCH_WRITE:
         for (int i = 0; i < 3; i++) {
            if (inst->src[i].file == GRF)
               no_spill[inst->src[i].reg] = true;
         }
	 if (inst->dst.file == GRF)
	    no_spill[inst->dst.reg] = true;
	 break;

      default:
	 break;
      }
   }
}

int
vec4_visitor::choose_spill_reg(struct ra_graph *g)
{
   float spill_costs[this->virtual_grf_count];
   bool no_spill[this->virtual_grf_count];

   evaluate_spill_costs(spill_costs, no_spill);

   for (int i = 0; i < this->virtual_grf_count; i++) {
      if (!no_spill[i])
         ra_set_node_spill_cost(g, i, spill_costs[i]);
   }

   return ra_get_best_spill_node(g);
}

void
vec4_visitor::spill_reg(int spill_reg_nr)
{
   assert(virtual_grf_sizes[spill_reg_nr] == 1);
   unsigned int spill_offset = c->last_scratch++;

   /* Generate spill/unspill instructions for the objects being spilled. */
   foreach_list(node, &this->instructions) {
      vec4_instruction *inst = (vec4_instruction *) node;

      for (unsigned int i = 0; i < 3; i++) {
         if (inst->src[i].file == GRF && inst->src[i].reg == spill_reg_nr) {
            src_reg spill_reg = inst->src[i];
            inst->src[i].reg = virtual_grf_alloc(1);
            dst_reg temp = dst_reg(inst->src[i]);

            /* Only read the necessary channels, to avoid overwriting the rest
             * with data that may not have been written to scratch.
             */
            temp.writemask = 0;
            for (int c = 0; c < 4; c++)
               temp.writemask |= (1 << BRW_GET_SWZ(inst->src[i].swizzle, c));
            assert(temp.writemask != 0);

            emit_scratch_read(inst, temp, spill_reg, spill_offset);
         }
      }

      if (inst->dst.file == GRF && inst->dst.reg == spill_reg_nr) {
         emit_scratch_write(inst, spill_offset);
      }
   }

   invalidate_live_intervals();
}

} /* namespace brw */
