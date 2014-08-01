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

#include "main/bufferobj.h"
#include "main/macros.h"
#include "brw_context.h"
#include "intel_batchbuffer.h"
#include "brw_defines.h"
#include "brw_state.h"
#include "main/transformfeedback.h"

static void
gen6_update_sol_surfaces(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   /* BRW_NEW_TRANSFORM_FEEDBACK */
   struct gl_transform_feedback_object *xfb_obj =
      ctx->TransformFeedback.CurrentObject;
   const struct gl_shader_program *shaderprog;
   const struct gl_transform_feedback_info *linked_xfb_info;
   int i;

   if (brw->geometry_program) {
      /* BRW_NEW_GEOMETRY_PROGRAM */
      shaderprog =
         ctx->_Shader->CurrentProgram[MESA_SHADER_GEOMETRY];
   } else {
      /* BRW_NEW_VERTEX_PROGRAM */
      shaderprog =
         ctx->_Shader->CurrentProgram[MESA_SHADER_VERTEX];
   }
   linked_xfb_info = &shaderprog->LinkedTransformFeedback;

   for (i = 0; i < BRW_MAX_SOL_BINDINGS; ++i) {
      const int surf_index = SURF_INDEX_GEN6_SOL_BINDING(i);
      if (_mesa_is_xfb_active_and_unpaused(ctx) &&
          i < linked_xfb_info->NumOutputs) {
         unsigned buffer = linked_xfb_info->Outputs[i].OutputBuffer;
         unsigned buffer_offset =
            xfb_obj->Offset[buffer] / 4 +
            linked_xfb_info->Outputs[i].DstOffset;
         if (brw->geometry_program) {
            brw_update_sol_surface(
               brw, xfb_obj->Buffers[buffer],
               &brw->gs.base.surf_offset[surf_index],
               linked_xfb_info->Outputs[i].NumComponents,
               linked_xfb_info->BufferStride[buffer], buffer_offset);
         } else {
            brw_update_sol_surface(
               brw, xfb_obj->Buffers[buffer],
               &brw->ff_gs.surf_offset[surf_index],
               linked_xfb_info->Outputs[i].NumComponents,
               linked_xfb_info->BufferStride[buffer], buffer_offset);
         }
      } else {
         if (!brw->geometry_program)
            brw->ff_gs.surf_offset[surf_index] = 0;
         else
            brw->gs.base.surf_offset[surf_index] = 0;
      }
   }

   brw->state.dirty.brw |= BRW_NEW_SURFACES;
}

const struct brw_tracked_state gen6_sol_surface = {
   .dirty = {
      .mesa = 0,
      .brw = (BRW_NEW_BATCH |
              BRW_NEW_VERTEX_PROGRAM |
              BRW_NEW_GEOMETRY_PROGRAM |
              BRW_NEW_TRANSFORM_FEEDBACK),
      .cache = 0
   },
   .emit = gen6_update_sol_surfaces,
};

/**
 * Constructs the binding table for the WM surface state, which maps unit
 * numbers to surface state objects.
 */
static void
brw_gs_upload_binding_table(struct brw_context *brw)
{
   uint32_t *bind;
   struct gl_context *ctx = &brw->ctx;
   const struct gl_shader_program *shaderprog;
   bool need_binding_table = false;

   /* We have two scenarios here:
    * 1) We are using a geometry shader only to implement transform feedback
    *    for a vertex shader (brw->geometry_program == NULL). In this case, we
    *    only need surfaces for transform feedback in the GS stage.
    * 2) We have a user-provided geometry shader. In this case we may need
    *    surfaces for transform feedback and/or other stuff, like textures,
    *    in the GS stage.
    */

   if (!brw->geometry_program) {
      /* BRW_NEW_VERTEX_PROGRAM */
      shaderprog = ctx->_Shader->CurrentProgram[MESA_SHADER_VERTEX];
      if (shaderprog) {
         /* Skip making a binding table if we don't have anything to put in it */
         const struct gl_transform_feedback_info *linked_xfb_info =
            &shaderprog->LinkedTransformFeedback;
         need_binding_table = linked_xfb_info->NumOutputs > 0;
      }
      if (!need_binding_table) {
         if (brw->ff_gs.bind_bo_offset != 0) {
            brw->state.dirty.brw |= BRW_NEW_GS_BINDING_TABLE;
            brw->ff_gs.bind_bo_offset = 0;
         }
         return;
      }

      /* Might want to calculate nr_surfaces first, to avoid taking up so much
       * space for the binding table. Anyway, in this case we know that we only
       * use BRW_MAX_SOL_BINDINGS surfaces at most.
       */
      bind = brw_state_batch(brw, AUB_TRACE_BINDING_TABLE,
                             sizeof(uint32_t) * BRW_MAX_SOL_BINDINGS,
                             32, &brw->ff_gs.bind_bo_offset);

      /* BRW_NEW_SURFACES */
      memcpy(bind, brw->ff_gs.surf_offset,
             BRW_MAX_SOL_BINDINGS * sizeof(uint32_t));
   } else {
      /* BRW_NEW_GEOMETRY_PROGRAM */
      shaderprog = ctx->_Shader->CurrentProgram[MESA_SHADER_GEOMETRY];
      if (shaderprog) {
         /* Skip making a binding table if we don't have anything to put in it */
         struct brw_stage_prog_data *prog_data = brw->gs.base.prog_data;
         const struct gl_transform_feedback_info *linked_xfb_info =
            &shaderprog->LinkedTransformFeedback;
         need_binding_table = linked_xfb_info->NumOutputs > 0 ||
                              prog_data->binding_table.size_bytes > 0;
      }
      if (!need_binding_table) {
         if (brw->gs.base.bind_bo_offset != 0) {
            brw->gs.base.bind_bo_offset = 0;
            brw->state.dirty.brw |= BRW_NEW_GS_BINDING_TABLE;
         }
         return;
      }

      /* Might want to calculate nr_surfaces first, to avoid taking up so much
       * space for the binding table.
       */
      bind = brw_state_batch(brw, AUB_TRACE_BINDING_TABLE,
                             sizeof(uint32_t) * BRW_MAX_SURFACES,
                             32, &brw->gs.base.bind_bo_offset);

      /* BRW_NEW_SURFACES */
      memcpy(bind, brw->gs.base.surf_offset,
             BRW_MAX_SURFACES * sizeof(uint32_t));
   }

   brw->state.dirty.brw |= BRW_NEW_GS_BINDING_TABLE;
}

const struct brw_tracked_state gen6_gs_binding_table = {
   .dirty = {
      .mesa = 0,
      .brw = (BRW_NEW_BATCH |
              BRW_NEW_VERTEX_PROGRAM |
              BRW_NEW_GEOMETRY_PROGRAM |
              BRW_NEW_SURFACES),
      .cache = 0
   },
   .emit = brw_gs_upload_binding_table,
};

struct gl_transform_feedback_object *
brw_new_transform_feedback(struct gl_context *ctx, GLuint name)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_transform_feedback_object *brw_obj =
      CALLOC_STRUCT(brw_transform_feedback_object);
   if (!brw_obj)
      return NULL;

   _mesa_init_transform_feedback_object(&brw_obj->base, name);

   brw_obj->offset_bo =
      drm_intel_bo_alloc(brw->bufmgr, "transform feedback offsets", 16, 64);
   brw_obj->prim_count_bo =
      drm_intel_bo_alloc(brw->bufmgr, "xfb primitive counts", 4096, 64);

   return &brw_obj->base;
}

void
brw_delete_transform_feedback(struct gl_context *ctx,
                              struct gl_transform_feedback_object *obj)
{
   struct brw_transform_feedback_object *brw_obj =
      (struct brw_transform_feedback_object *) obj;

   for (unsigned i = 0; i < Elements(obj->Buffers); i++) {
      _mesa_reference_buffer_object(ctx, &obj->Buffers[i], NULL);
   }

   drm_intel_bo_unreference(brw_obj->offset_bo);
   drm_intel_bo_unreference(brw_obj->prim_count_bo);

   free(brw_obj);
}

void
brw_begin_transform_feedback(struct gl_context *ctx, GLenum mode,
			     struct gl_transform_feedback_object *obj)
{
   struct brw_context *brw = brw_context(ctx);
   const struct gl_shader_program *shaderprog;
   const struct gl_transform_feedback_info *linked_xfb_info;
   struct gl_transform_feedback_object *xfb_obj =
      ctx->TransformFeedback.CurrentObject;

   assert(brw->gen == 6);

   if (brw->geometry_program) {
      /* BRW_NEW_GEOMETRY_PROGRAM */
      shaderprog =
         ctx->_Shader->CurrentProgram[MESA_SHADER_GEOMETRY];
   } else {
      /* BRW_NEW_VERTEX_PROGRAM */
      shaderprog =
         ctx->_Shader->CurrentProgram[MESA_SHADER_VERTEX];
   }
   linked_xfb_info = &shaderprog->LinkedTransformFeedback;

   /* Compute the maximum number of vertices that we can write without
    * overflowing any of the buffers currently being used for feedback.
    */
   unsigned max_index
      = _mesa_compute_max_transform_feedback_vertices(xfb_obj,
                                                      linked_xfb_info);

   /* 3DSTATE_GS_SVB_INDEX is non-pipelined. */
   intel_emit_post_sync_nonzero_flush(brw);

   /* Initialize the SVBI 0 register to zero and set the maximum index. */
   BEGIN_BATCH(4);
   OUT_BATCH(_3DSTATE_GS_SVB_INDEX << 16 | (4 - 2));
   OUT_BATCH(0); /* SVBI 0 */
   OUT_BATCH(0); /* starting index */
   OUT_BATCH(max_index);
   ADVANCE_BATCH();

   /* Initialize the rest of the unused streams to sane values.  Otherwise,
    * they may indicate that there is no room to write data and prevent
    * anything from happening at all.
    */
   for (int i = 1; i < 4; i++) {
      BEGIN_BATCH(4);
      OUT_BATCH(_3DSTATE_GS_SVB_INDEX << 16 | (4 - 2));
      OUT_BATCH(i << SVB_INDEX_SHIFT);
      OUT_BATCH(0); /* starting index */
      OUT_BATCH(0xffffffff);
      ADVANCE_BATCH();
   }
}

void
brw_end_transform_feedback(struct gl_context *ctx,
                           struct gl_transform_feedback_object *obj)
{
   /* After EndTransformFeedback, it's likely that the client program will try
    * to draw using the contents of the transform feedback buffer as vertex
    * input.  In order for this to work, we need to flush the data through at
    * least the GS stage of the pipeline, and flush out the render cache.  For
    * simplicity, just do a full flush.
    */
   struct brw_context *brw = brw_context(ctx);
   intel_batchbuffer_emit_mi_flush(brw);
}
