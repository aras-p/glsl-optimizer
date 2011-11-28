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

/** \file gen6_sol.c
 *
 * Code to initialize the binding table entries used by transform feedback.
 */

#include "brw_context.h"
#include "brw_defines.h"

static void
gen6_update_sol_surfaces(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->intel.ctx;
   /* _NEW_TRANSFORM_FEEDBACK */
   struct gl_transform_feedback_object *xfb_obj =
      ctx->TransformFeedback.CurrentObject;
   /* BRW_NEW_VERTEX_PROGRAM */
   const struct gl_shader_program *shaderprog =
      ctx->Shader.CurrentVertexProgram;
   const struct gl_transform_feedback_info *linked_xfb_info =
      &shaderprog->LinkedTransformFeedback;
   int i;

   for (i = 0; i < BRW_MAX_SOL_BINDINGS; ++i) {
      const int surf_index = SURF_INDEX_SOL_BINDING(i);
      if (xfb_obj->Active && i < linked_xfb_info->NumOutputs) {
         unsigned buffer = linked_xfb_info->Outputs[i].OutputBuffer;
         unsigned buffer_offset =
            xfb_obj->Offset[buffer] / 4 +
            linked_xfb_info->Outputs[i].DstOffset;
         brw_update_sol_surface(
            brw, xfb_obj->Buffers[buffer], &brw->bind.surf_offset[surf_index],
            linked_xfb_info->Outputs[i].NumComponents,
            linked_xfb_info->BufferStride[buffer], buffer_offset);
      } else {
         brw->bind.surf_offset[surf_index] = 0;
      }
   }
}

const struct brw_tracked_state gen6_sol_surface = {
   .dirty = {
      .mesa = _NEW_TRANSFORM_FEEDBACK,
      .brw = (BRW_NEW_BATCH |
              BRW_NEW_VERTEX_PROGRAM),
      .cache = 0
   },
   .emit = gen6_update_sol_surfaces,
};
