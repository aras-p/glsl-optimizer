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
   if (ir->sampler->type->sampler_dimensionality != GLSL_SAMPLER_DIM_RECT ||
       !ir->offset || ir->op != ir_tg4)
      return visit_continue;

   ir->coordinate = add(ir->coordinate, i2f(ir->offset));
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
