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

/**
 * @file gen7_sol_state.c
 *
 * Controls the stream output logic (SOL) stage of the gen7 hardware, which is
 * used to implement GL_EXT_transform_feedback.
 */

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "intel_batchbuffer.h"
#include "intel_buffer_objects.h"
#include "main/transformfeedback.h"

static void
upload_3dstate_so_buffers(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   /* BRW_NEW_TRANSFORM_FEEDBACK */
   struct gl_transform_feedback_object *xfb_obj =
      ctx->TransformFeedback.CurrentObject;
   const struct gl_transform_feedback_info *linked_xfb_info =
      &xfb_obj->shader_program->LinkedTransformFeedback;
   int i;

   /* Set up the up to 4 output buffers.  These are the ranges defined in the
    * gl_transform_feedback_object.
    */
   for (i = 0; i < 4; i++) {
      struct intel_buffer_object *bufferobj =
	 intel_buffer_object(xfb_obj->Buffers[i]);
      drm_intel_bo *bo;
      uint32_t start, end;
      uint32_t stride;

      if (!xfb_obj->Buffers[i]) {
	 /* The pitch of 0 in this command indicates that the buffer is
	  * unbound and won't be written to.
	  */
	 BEGIN_BATCH(4);
	 OUT_BATCH(_3DSTATE_SO_BUFFER << 16 | (4 - 2));
	 OUT_BATCH((i << SO_BUFFER_INDEX_SHIFT));
	 OUT_BATCH(0);
	 OUT_BATCH(0);
	 ADVANCE_BATCH();

	 continue;
      }

      stride = linked_xfb_info->BufferStride[i] * 4;

      start = xfb_obj->Offset[i];
      assert(start % 4 == 0);
      end = ALIGN(start + xfb_obj->Size[i], 4);
      bo = intel_bufferobj_buffer(brw, bufferobj, start, end - start);
      assert(end <= bo->size);

      BEGIN_BATCH(4);
      OUT_BATCH(_3DSTATE_SO_BUFFER << 16 | (4 - 2));
      OUT_BATCH((i << SO_BUFFER_INDEX_SHIFT) | stride);
      OUT_RELOC(bo, I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER, start);
      OUT_RELOC(bo, I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER, end);
      ADVANCE_BATCH();
   }
}

/**
 * Outputs the 3DSTATE_SO_DECL_LIST command.
 *
 * The data output is a series of 64-bit entries containing a SO_DECL per
 * stream.  We only have one stream of rendering coming out of the GS unit, so
 * we only emit stream 0 (low 16 bits) SO_DECLs.
 */
void
gen7_upload_3dstate_so_decl_list(struct brw_context *brw,
                                 const struct brw_vue_map *vue_map)
{
   struct gl_context *ctx = &brw->ctx;
   /* BRW_NEW_TRANSFORM_FEEDBACK */
   struct gl_transform_feedback_object *xfb_obj =
      ctx->TransformFeedback.CurrentObject;
   const struct gl_transform_feedback_info *linked_xfb_info =
      &xfb_obj->shader_program->LinkedTransformFeedback;
   uint16_t so_decl[128];
   int buffer_mask = 0;
   int next_offset[4] = {0, 0, 0, 0};
   int decls = 0;

   STATIC_ASSERT(ARRAY_SIZE(so_decl) >= MAX_PROGRAM_OUTPUTS);

   /* Construct the list of SO_DECLs to be emitted.  The formatting of the
    * command is feels strange -- each dword pair contains a SO_DECL per stream.
    */
   for (int i = 0; i < linked_xfb_info->NumOutputs; i++) {
      int buffer = linked_xfb_info->Outputs[i].OutputBuffer;
      uint16_t decl = 0;
      int varying = linked_xfb_info->Outputs[i].OutputRegister;
      const unsigned components = linked_xfb_info->Outputs[i].NumComponents;
      unsigned component_mask = (1 << components) - 1;

      /* gl_PointSize is stored in VARYING_SLOT_PSIZ.w
       * gl_Layer is stored in VARYING_SLOT_PSIZ.y
       * gl_ViewportIndex is stored in VARYING_SLOT_PSIZ.z
       */
      if (varying == VARYING_SLOT_PSIZ) {
         assert(components == 1);
         component_mask <<= 3;
      } else if (varying == VARYING_SLOT_LAYER) {
         assert(components == 1);
         component_mask <<= 1;
      } else if (varying == VARYING_SLOT_VIEWPORT) {
         assert(components == 1);
         component_mask <<= 2;
      } else {
         component_mask <<= linked_xfb_info->Outputs[i].ComponentOffset;
      }

      buffer_mask |= 1 << buffer;

      decl |= buffer << SO_DECL_OUTPUT_BUFFER_SLOT_SHIFT;
      if (varying == VARYING_SLOT_LAYER || varying == VARYING_SLOT_VIEWPORT) {
         decl |= vue_map->varying_to_slot[VARYING_SLOT_PSIZ] <<
            SO_DECL_REGISTER_INDEX_SHIFT;
      } else {
         assert(vue_map->varying_to_slot[varying] >= 0);
         decl |= vue_map->varying_to_slot[varying] <<
            SO_DECL_REGISTER_INDEX_SHIFT;
      }
      decl |= component_mask << SO_DECL_COMPONENT_MASK_SHIFT;

      /* Mesa doesn't store entries for gl_SkipComponents in the Outputs[]
       * array.  Instead, it simply increments DstOffset for the following
       * input by the number of components that should be skipped.
       *
       * Our hardware is unusual in that it requires us to program SO_DECLs
       * for fake "hole" components, rather than simply taking the offset
       * for each real varying.  Each hole can have size 1, 2, 3, or 4; we
       * program as many size = 4 holes as we can, then a final hole to
       * accomodate the final 1, 2, or 3 remaining.
       */
      int skip_components =
         linked_xfb_info->Outputs[i].DstOffset - next_offset[buffer];

      next_offset[buffer] += skip_components;

      while (skip_components >= 4) {
         so_decl[decls++] = SO_DECL_HOLE_FLAG | 0xf;
         skip_components -= 4;
      }
      if (skip_components > 0)
         so_decl[decls++] = SO_DECL_HOLE_FLAG | ((1 << skip_components) - 1);

      assert(linked_xfb_info->Outputs[i].DstOffset == next_offset[buffer]);

      next_offset[buffer] += components;

      so_decl[decls++] = decl;
   }

   BEGIN_BATCH(decls * 2 + 3);
   OUT_BATCH(_3DSTATE_SO_DECL_LIST << 16 | (decls * 2 + 1));

   OUT_BATCH((buffer_mask << SO_STREAM_TO_BUFFER_SELECTS_0_SHIFT) |
	     (0 << SO_STREAM_TO_BUFFER_SELECTS_1_SHIFT) |
	     (0 << SO_STREAM_TO_BUFFER_SELECTS_2_SHIFT) |
	     (0 << SO_STREAM_TO_BUFFER_SELECTS_3_SHIFT));

   OUT_BATCH((decls << SO_NUM_ENTRIES_0_SHIFT) |
	     (0 << SO_NUM_ENTRIES_1_SHIFT) |
	     (0 << SO_NUM_ENTRIES_2_SHIFT) |
	     (0 << SO_NUM_ENTRIES_3_SHIFT));

   for (int i = 0; i < decls; i++) {
      OUT_BATCH(so_decl[i]);
      OUT_BATCH(0);
   }

   ADVANCE_BATCH();
}

static void
upload_3dstate_streamout(struct brw_context *brw, bool active,
			 const struct brw_vue_map *vue_map)
{
   struct gl_context *ctx = &brw->ctx;
   /* BRW_NEW_TRANSFORM_FEEDBACK */
   struct gl_transform_feedback_object *xfb_obj =
      ctx->TransformFeedback.CurrentObject;
   uint32_t dw1 = 0, dw2 = 0;
   int i;

   if (active) {
      int urb_entry_read_offset = 0;
      int urb_entry_read_length = (vue_map->num_slots + 1) / 2 -
	 urb_entry_read_offset;

      dw1 |= SO_FUNCTION_ENABLE;
      dw1 |= SO_STATISTICS_ENABLE;

      /* _NEW_LIGHT */
      if (ctx->Light.ProvokingVertex != GL_FIRST_VERTEX_CONVENTION)
	 dw1 |= SO_REORDER_TRAILING;

      for (i = 0; i < 4; i++) {
	 if (xfb_obj->Buffers[i]) {
	    dw1 |= SO_BUFFER_ENABLE(i);
	 }
      }

      /* We always read the whole vertex.  This could be reduced at some
       * point by reading less and offsetting the register index in the
       * SO_DECLs.
       */
      dw2 |= urb_entry_read_offset << SO_STREAM_0_VERTEX_READ_OFFSET_SHIFT;
      dw2 |= (urb_entry_read_length - 1) <<
	 SO_STREAM_0_VERTEX_READ_LENGTH_SHIFT;
   }

   BEGIN_BATCH(3);
   OUT_BATCH(_3DSTATE_STREAMOUT << 16 | (3 - 2));
   OUT_BATCH(dw1);
   OUT_BATCH(dw2);
   ADVANCE_BATCH();
}

static void
upload_sol_state(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   /* BRW_NEW_TRANSFORM_FEEDBACK */
   bool active = _mesa_is_xfb_active_and_unpaused(ctx);

   if (active) {
      upload_3dstate_so_buffers(brw);
      /* BRW_NEW_VUE_MAP_GEOM_OUT */
      gen7_upload_3dstate_so_decl_list(brw, &brw->vue_map_geom_out);
   }

   /* Finally, set up the SOL stage.  This command must always follow updates to
    * the nonpipelined SOL state (3DSTATE_SO_BUFFER, 3DSTATE_SO_DECL_LIST) or
    * MMIO register updates (current performed by the kernel at each batch
    * emit).
    */
   upload_3dstate_streamout(brw, active, &brw->vue_map_geom_out);
}

const struct brw_tracked_state gen7_sol_state = {
   .dirty = {
      .mesa  = (_NEW_LIGHT),
      .brw   = (BRW_NEW_BATCH |
                BRW_NEW_VUE_MAP_GEOM_OUT |
                BRW_NEW_TRANSFORM_FEEDBACK)
   },
   .emit = upload_sol_state,
};

/**
 * Tally the number of primitives generated so far.
 *
 * The buffer contains a series of pairs:
 * (<start0, start1, start2, start3>, <end0, end1, end2, end3>) ;
 * (<start0, start1, start2, start3>, <end0, end1, end2, end3>) ;
 *
 * For each stream, we subtract the pair of values (end - start) to get the
 * number of primitives generated during one section.  We accumulate these
 * values, adding them up to get the total number of primitives generated.
 */
static void
gen7_tally_prims_generated(struct brw_context *brw,
                           struct brw_transform_feedback_object *obj)
{
   /* If the current batch is still contributing to the number of primitives
    * generated, flush it now so the results will be present when mapped.
    */
   if (drm_intel_bo_references(brw->batch.bo, obj->prim_count_bo))
      intel_batchbuffer_flush(brw);

   if (unlikely(brw->perf_debug && drm_intel_bo_busy(obj->prim_count_bo)))
      perf_debug("Stalling for # of transform feedback primitives written.\n");

   drm_intel_bo_map(obj->prim_count_bo, false);
   uint64_t *prim_counts = obj->prim_count_bo->virtual;

   assert(obj->prim_count_buffer_index % (2 * BRW_MAX_XFB_STREAMS) == 0);
   int pairs = obj->prim_count_buffer_index / (2 * BRW_MAX_XFB_STREAMS);

   for (int i = 0; i < pairs; i++) {
      for (int s = 0; s < BRW_MAX_XFB_STREAMS; s++) {
         obj->prims_generated[s] +=
            prim_counts[BRW_MAX_XFB_STREAMS + s] - prim_counts[s];
      }
      prim_counts += 2 * BRW_MAX_XFB_STREAMS; /* move to the next pair */
   }

   drm_intel_bo_unmap(obj->prim_count_bo);

   /* We've already gathered up the old data; we can safely overwrite it now. */
   obj->prim_count_buffer_index = 0;
}

/**
 * Store the SO_NUM_PRIMS_WRITTEN counters for each stream (4 uint64_t values)
 * to prim_count_bo.
 *
 * If prim_count_bo is out of space, gather up the results so far into
 * prims_generated[] and allocate a new buffer with enough space.
 *
 * The number of primitives written is used to compute the number of vertices
 * written to a transform feedback stream, which is required to implement
 * DrawTransformFeedback().
 */
static void
gen7_save_primitives_written_counters(struct brw_context *brw,
                                struct brw_transform_feedback_object *obj)
{
   const int streams = BRW_MAX_XFB_STREAMS;

   /* Check if there's enough space for a new pair of four values. */
   if (obj->prim_count_bo != NULL &&
       obj->prim_count_buffer_index + 2 * streams >= 4096 / sizeof(uint64_t)) {
      /* Gather up the results so far and release the BO. */
      gen7_tally_prims_generated(brw, obj);
   }

   /* Flush any drawing so that the counters have the right values. */
   intel_batchbuffer_emit_mi_flush(brw);

   /* Emit MI_STORE_REGISTER_MEM commands to write the values. */
   for (int i = 0; i < streams; i++) {
      brw_store_register_mem64(brw, obj->prim_count_bo,
                               GEN7_SO_NUM_PRIMS_WRITTEN(i),
                               obj->prim_count_buffer_index + i);
   }

   /* Update where to write data to. */
   obj->prim_count_buffer_index += streams;
}

/**
 * Compute the number of vertices written by this transform feedback operation.
 */
static void
brw_compute_xfb_vertices_written(struct brw_context *brw,
                                 struct brw_transform_feedback_object *obj)
{
   if (obj->vertices_written_valid || !obj->base.EndedAnytime)
      return;

   unsigned vertices_per_prim = 0;

   switch (obj->primitive_mode) {
   case GL_POINTS:
      vertices_per_prim = 1;
      break;
   case GL_LINES:
      vertices_per_prim = 2;
      break;
   case GL_TRIANGLES:
      vertices_per_prim = 3;
      break;
   default:
      assert(!"Invalid transform feedback primitive mode.");
   }

   /* Get the number of primitives generated. */
   gen7_tally_prims_generated(brw, obj);

   for (int i = 0; i < BRW_MAX_XFB_STREAMS; i++) {
      obj->vertices_written[i] = vertices_per_prim * obj->prims_generated[i];
   }
   obj->vertices_written_valid = true;
}

/**
 * GetTransformFeedbackVertexCount() driver hook.
 *
 * Returns the number of vertices written to a particular stream by the last
 * Begin/EndTransformFeedback block.  Used to implement DrawTransformFeedback().
 */
GLsizei
brw_get_transform_feedback_vertex_count(struct gl_context *ctx,
                                        struct gl_transform_feedback_object *obj,
                                        GLuint stream)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_transform_feedback_object *brw_obj =
      (struct brw_transform_feedback_object *) obj;

   assert(obj->EndedAnytime);
   assert(stream < BRW_MAX_XFB_STREAMS);

   brw_compute_xfb_vertices_written(brw, brw_obj);
   return brw_obj->vertices_written[stream];
}

void
gen7_begin_transform_feedback(struct gl_context *ctx, GLenum mode,
                              struct gl_transform_feedback_object *obj)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_transform_feedback_object *brw_obj =
      (struct brw_transform_feedback_object *) obj;

   /* Reset the SO buffer offsets to 0. */
   if (brw->gen >= 8) {
      brw_obj->zero_offsets = true;
   } else {
      intel_batchbuffer_flush(brw);
      brw->batch.needs_sol_reset = true;
   }

   /* We're about to lose the information needed to compute the number of
    * vertices written during the last Begin/EndTransformFeedback section,
    * so we can't delay it any further.
    */
   brw_compute_xfb_vertices_written(brw, brw_obj);

   /* No primitives have been generated yet. */
   for (int i = 0; i < BRW_MAX_XFB_STREAMS; i++) {
      brw_obj->prims_generated[i] = 0;
   }

   /* Store the starting value of the SO_NUM_PRIMS_WRITTEN counters. */
   gen7_save_primitives_written_counters(brw, brw_obj);

   brw_obj->primitive_mode = mode;
}

void
gen7_end_transform_feedback(struct gl_context *ctx,
			    struct gl_transform_feedback_object *obj)
{
   /* After EndTransformFeedback, it's likely that the client program will try
    * to draw using the contents of the transform feedback buffer as vertex
    * input.  In order for this to work, we need to flush the data through at
    * least the GS stage of the pipeline, and flush out the render cache.  For
    * simplicity, just do a full flush.
    */
   struct brw_context *brw = brw_context(ctx);
   struct brw_transform_feedback_object *brw_obj =
      (struct brw_transform_feedback_object *) obj;

   /* Store the ending value of the SO_NUM_PRIMS_WRITTEN counters. */
   gen7_save_primitives_written_counters(brw, brw_obj);

   /* EndTransformFeedback() means that we need to update the number of
    * vertices written.  Since it's only necessary if DrawTransformFeedback()
    * is called and it means mapping a buffer object, we delay computing it
    * until it's absolutely necessary to try and avoid stalls.
    */
   brw_obj->vertices_written_valid = false;
}

void
gen7_pause_transform_feedback(struct gl_context *ctx,
                              struct gl_transform_feedback_object *obj)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_transform_feedback_object *brw_obj =
      (struct brw_transform_feedback_object *) obj;

   /* Flush any drawing so that the counters have the right values. */
   intel_batchbuffer_emit_mi_flush(brw);

   /* Save the SOL buffer offset register values. */
   if (brw->gen < 8) {
      for (int i = 0; i < 4; i++) {
         BEGIN_BATCH(3);
         OUT_BATCH(MI_STORE_REGISTER_MEM | (3 - 2));
         OUT_BATCH(GEN7_SO_WRITE_OFFSET(i));
         OUT_RELOC(brw_obj->offset_bo,
                   I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                   i * sizeof(uint32_t));
         ADVANCE_BATCH();
      }
   }

   /* Store the temporary ending value of the SO_NUM_PRIMS_WRITTEN counters.
    * While this operation is paused, other transform feedback actions may
    * occur, which will contribute to the counters.  We need to exclude that
    * from our counts.
    */
   gen7_save_primitives_written_counters(brw, brw_obj);
}

void
gen7_resume_transform_feedback(struct gl_context *ctx,
                               struct gl_transform_feedback_object *obj)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_transform_feedback_object *brw_obj =
      (struct brw_transform_feedback_object *) obj;

   /* Reload the SOL buffer offset registers. */
   if (brw->gen < 8) {
      for (int i = 0; i < 4; i++) {
         BEGIN_BATCH(3);
         OUT_BATCH(GEN7_MI_LOAD_REGISTER_MEM | (3 - 2));
         OUT_BATCH(GEN7_SO_WRITE_OFFSET(i));
         OUT_RELOC(brw_obj->offset_bo,
                   I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                   i * sizeof(uint32_t));
         ADVANCE_BATCH();
      }
   }

   /* Store the new starting value of the SO_NUM_PRIMS_WRITTEN counters. */
   gen7_save_primitives_written_counters(brw, brw_obj);
}
