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
#include "glsl/ir_print_visitor.h"

using namespace brw;

namespace brw {

static void
assign(int *reg_hw_locations, reg *reg)
{
   if (reg->file == GRF) {
      reg->reg = reg_hw_locations[reg->reg];
   }
}

void
vec4_visitor::reg_allocate_trivial()
{
   int hw_reg_mapping[this->virtual_grf_count];
   bool virtual_grf_used[this->virtual_grf_count];
   int i;
   int next;

   /* Calculate which virtual GRFs are actually in use after whatever
    * optimization passes have occurred.
    */
   for (int i = 0; i < this->virtual_grf_count; i++) {
      virtual_grf_used[i] = false;
   }

   foreach_iter(exec_list_iterator, iter, this->instructions) {
      vec4_instruction *inst = (vec4_instruction *)iter.get();

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

   foreach_iter(exec_list_iterator, iter, this->instructions) {
      vec4_instruction *inst = (vec4_instruction *)iter.get();

      assign(hw_reg_mapping, &inst->dst);
      assign(hw_reg_mapping, &inst->src[0]);
      assign(hw_reg_mapping, &inst->src[1]);
      assign(hw_reg_mapping, &inst->src[2]);
   }

   if (prog_data->total_grf > max_grf) {
      fail("Ran out of regs on trivial allocator (%d/%d)\n",
	   prog_data->total_grf, max_grf);
   }
}

static void
brw_alloc_reg_set_for_classes(struct brw_context *brw,
			      int *class_sizes,
			      int class_count,
			      int base_reg_count)
{
   /* Compute the total number of registers across all classes. */
   int ra_reg_count = 0;
   for (int i = 0; i < class_count; i++) {
      ra_reg_count += base_reg_count - (class_sizes[i] - 1);
   }

   ralloc_free(brw->vs.ra_reg_to_grf);
   brw->vs.ra_reg_to_grf = ralloc_array(brw, uint8_t, ra_reg_count);
   ralloc_free(brw->vs.regs);
   brw->vs.regs = ra_alloc_reg_set(brw, ra_reg_count);
   ralloc_free(brw->vs.classes);
   brw->vs.classes = ralloc_array(brw, int, class_count + 1);

   /* Now, add the registers to their classes, and add the conflicts
    * between them and the base GRF registers (and also each other).
    */
   int reg = 0;
   for (int i = 0; i < class_count; i++) {
      int class_reg_count = base_reg_count - (class_sizes[i] - 1);
      brw->vs.classes[i] = ra_alloc_reg_class(brw->vs.regs);

      for (int j = 0; j < class_reg_count; j++) {
	 ra_class_add_reg(brw->vs.regs, brw->vs.classes[i], reg);

	 brw->vs.ra_reg_to_grf[reg] = j;

	 for (int base_reg = j;
	      base_reg < j + class_sizes[i];
	      base_reg++) {
	    ra_add_transitive_reg_conflict(brw->vs.regs, base_reg, reg);
	 }

	 reg++;
      }
   }
   assert(reg == ra_reg_count);

   ra_set_finalize(brw->vs.regs);
}

void
vec4_visitor::reg_allocate()
{
   int hw_reg_mapping[virtual_grf_count];
   int first_assigned_grf = this->first_non_payload_grf;
   int base_reg_count = max_grf - first_assigned_grf;
   int class_sizes[base_reg_count];
   int class_count = 0;

   /* Using the trivial allocator can be useful in debugging undefined
    * register access as a result of broken optimization passes.
    */
   if (0) {
      reg_allocate_trivial();
      return;
   }

   calculate_live_intervals();

   /* Set up the register classes.
    *
    * The base registers store a vec4.  However, we'll need larger
    * storage for arrays, structures, and matrices, which will be sets
    * of contiguous registers.
    */
   class_sizes[class_count++] = 1;

   for (int r = 0; r < virtual_grf_count; r++) {
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

   brw_alloc_reg_set_for_classes(brw, class_sizes, class_count, base_reg_count);

   struct ra_graph *g = ra_alloc_interference_graph(brw->vs.regs,
						    virtual_grf_count);

   for (int i = 0; i < virtual_grf_count; i++) {
      for (int c = 0; c < class_count; c++) {
	 if (class_sizes[c] == this->virtual_grf_sizes[i]) {
	    ra_set_node_class(g, i, brw->vs.classes[c]);
	    break;
	 }
      }

      for (int j = 0; j < i; j++) {
	 if (virtual_grf_interferes(i, j)) {
	    ra_add_node_interference(g, i, j);
	 }
      }
   }

   if (!ra_allocate_no_spills(g)) {
      ralloc_free(g);
      fail("No register spilling support yet\n");
      return;
   }

   /* Get the chosen virtual registers for each node, and map virtual
    * regs in the register classes back down to real hardware reg
    * numbers.
    */
   prog_data->total_grf = first_assigned_grf;
   for (int i = 0; i < virtual_grf_count; i++) {
      int reg = ra_get_node_reg(g, i);

      hw_reg_mapping[i] = first_assigned_grf + brw->vs.ra_reg_to_grf[reg];
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
}

} /* namespace brw */
