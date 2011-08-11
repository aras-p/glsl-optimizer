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

#include "brw_vec4.h"
#include "../glsl/ir_print_visitor.h"

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

   /* Note that compressed instructions require alignment to 2 registers. */
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

   if (prog_data->total_grf > BRW_MAX_GRF) {
      fail("Ran out of regs on trivial allocator (%d/%d)\n",
	   prog_data->total_grf, BRW_MAX_GRF);
   }
}

void
vec4_visitor::reg_allocate()
{
   reg_allocate_trivial();
}

} /* namespace brw */
