/*
 * Copyright © 2013 Intel Corporation
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file brw_lower_unnormalized_offset.cpp
 *
 * IR lower pass to convert a texture offset into an adjusted coordinate,
 * for use with unnormalized coordinates. At least the gather4* messages
 * on Ivybridge and Haswell make a mess with nonzero offsets.
 *
 * \author Chris Forbes <chrisf@ijw.co.nz>
 */

#include "glsl/glsl_types.h"
#include "glsl/ir.h"
#include "glsl/ir_builder.h"

using namespace ir_builder;

class brw_lower_unnormalized_offset_visitor : public ir_hierarchical_visitor {
public:
   brw_lower_unnormalized_offset_visitor()
   {
      progress = false;
   }

   ir_visitor_status visit_leave(ir_texture *ir);

   bool progress;
};

ir_visitor_status
brw_lower_unnormalized_offset_visitor::visit_leave(ir_texture *ir)
{
   if (!ir->offset)
      return visit_continue;

   if (ir->op == ir_tg4 || ir->op == ir_tex) {
      if (ir->sampler->type->sampler_dimensionality != GLSL_SAMPLER_DIM_RECT)
         return visit_continue;
   }
   else if (ir->op != ir_txf) {
      return visit_continue;
   }

   void *mem_ctx = ralloc_parent(ir);

   if (ir->op == ir_txf) {
      ir_variable *var = new(mem_ctx) ir_variable(ir->coordinate->type,
                                                  "coordinate",
                                                  ir_var_temporary);
      base_ir->insert_before(var);
      base_ir->insert_before(assign(var, ir->coordinate));
      base_ir->insert_before(assign(var,
               add(swizzle_for_size(var, ir->offset->type->vector_elements), ir->offset),
               (1 << ir->offset->type->vector_elements) - 1));

      ir->coordinate = new(mem_ctx) ir_dereference_variable(var);
   } else {
      ir->coordinate = add(ir->coordinate, i2f(ir->offset));
   }

   ir->offset = NULL;

   progress = true;
   return visit_continue;
}

extern "C" {

bool
brw_do_lower_unnormalized_offset(exec_list *instructions)
{
   brw_lower_unnormalized_offset_visitor v;

   visit_list_elements(&v, instructions);

   return v.progress;
}

}
