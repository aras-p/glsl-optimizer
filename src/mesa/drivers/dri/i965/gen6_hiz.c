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

#include <assert.h>

#include "intel_batchbuffer.h"
#include "intel_fbo.h"
#include "intel_mipmap_tree.h"

#include "brw_context.h"
#include "brw_defines.h"
#include "brw_state.h"

#include "gen6_hiz.h"

/**
 * \name Constants for HiZ VBO
 * \{
 *
 * \see brw_context::hiz::vertex_bo
 */
#define GEN6_HIZ_NUM_VERTICES 3
#define GEN6_HIZ_NUM_VUE_ELEMS 8
#define GEN6_HIZ_VBO_SIZE (GEN6_HIZ_NUM_VERTICES \
                           * GEN6_HIZ_NUM_VUE_ELEMS \
                           * sizeof(float))
/** \} */

/**
 * \brief Initialize data needed for the HiZ op.
 *
 * This called when executing the first HiZ op.
 * \see brw_context::hiz
 */
void
gen6_hiz_init(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->intel.ctx;
   struct intel_context *intel = &brw->intel;
   struct brw_hiz_state *hiz = &brw->hiz;

   hiz->vertex_bo = drm_intel_bo_alloc(intel->bufmgr, "bufferobj",
                                       GEN6_HIZ_VBO_SIZE, /* size */
                                       64); /* alignment */

   if (!hiz->vertex_bo)
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "failed to allocate internal VBO");
}

void
gen6_hiz_emit_batch_head(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->intel.ctx;
   struct intel_context *intel = &brw->intel;

   /* To ensure that the batch contains only the resolve, flush the batch
    * before beginning and after finishing emitting the resolve packets.
    *
    * Ideally, we would not need to flush for the resolve op. But, I suspect
    * that it's unsafe for CMD_PIPELINE_SELECT to occur multiple times in
    * a single batch, and there is no safe way to ensure that other than by
    * fencing the resolve with flushes. Ideally, we would just detect if
    * a batch is in progress and do the right thing, but that would require
    * the ability to *safely* access brw_context::state::dirty::brw
    * outside of the brw_upload_state() codepath.
    */
   intel_flush(ctx);

   /* CMD_PIPELINE_SELECT
    *
    * Select the 3D pipeline, as opposed to the media pipeline.
    */
   {
      BEGIN_BATCH(1);
      OUT_BATCH(brw->CMD_PIPELINE_SELECT << 16);
      ADVANCE_BATCH();
   }

   /* 3DSTATE_MULTISAMPLE */
   {
      int length = intel->gen == 7 ? 4 : 3;

      BEGIN_BATCH(length);
      OUT_BATCH(_3DSTATE_MULTISAMPLE << 16 | (length - 2));
      OUT_BATCH(MS_PIXEL_LOCATION_CENTER |
                MS_NUMSAMPLES_1);
      OUT_BATCH(0);
      if (length >= 4)
         OUT_BATCH(0);
      ADVANCE_BATCH();

   }

   /* 3DSTATE_SAMPLE_MASK */
   {
      BEGIN_BATCH(2);
      OUT_BATCH(_3DSTATE_SAMPLE_MASK << 16 | (2 - 2));
      OUT_BATCH(1);
      ADVANCE_BATCH();
   }

   /* CMD_STATE_BASE_ADDRESS
    *
    * From the Sandy Bridge PRM, Volume 1, Part 1, Table STATE_BASE_ADDRESS:
    *     The following commands must be reissued following any change to the
    *     base addresses:
    *         3DSTATE_CC_POINTERS
    *         3DSTATE_BINDING_TABLE_POINTERS
    *         3DSTATE_SAMPLER_STATE_POINTERS
    *         3DSTATE_VIEWPORT_STATE_POINTERS
    *         MEDIA_STATE_POINTERS
    */
   {
      BEGIN_BATCH(10);
      OUT_BATCH(CMD_STATE_BASE_ADDRESS << 16 | (10 - 2));
      OUT_BATCH(1); /* GeneralStateBaseAddressModifyEnable */
      /* SurfaceStateBaseAddress */
      OUT_RELOC(intel->batch.bo, I915_GEM_DOMAIN_SAMPLER, 0, 1);
      /* DynamicStateBaseAddress */
      OUT_RELOC(intel->batch.bo, (I915_GEM_DOMAIN_RENDER |
                                  I915_GEM_DOMAIN_INSTRUCTION), 0, 1);
      OUT_BATCH(1); /* IndirectObjectBaseAddress */
      OUT_BATCH(1); /* InstructionBaseAddress */
      OUT_BATCH(1); /* GeneralStateUpperBound */
      OUT_BATCH(1); /* DynamicStateUpperBound */
      OUT_BATCH(1); /* IndirectObjectUpperBound*/
      OUT_BATCH(1); /* InstructionAccessUpperBound */
      ADVANCE_BATCH();
   }
}

void
gen6_hiz_emit_vertices(struct brw_context *brw,
                       struct intel_mipmap_tree *mt,
                       unsigned int level,
                       unsigned int layer)
{
   struct intel_context *intel = &brw->intel;
   struct brw_hiz_state *hiz = &brw->hiz;

   /* Setup VBO for the rectangle primitive..
    *
    * A rectangle primitive (3DPRIM_RECTLIST) consists of only three
    * vertices. The vertices reside in screen space with DirectX coordinates
    * (that is, (0, 0) is the upper left corner).
    *
    *   v2 ------ implied
    *    |        |
    *    |        |
    *   v0 ----- v1
    *
    * Since the VS is disabled, the clipper loads each VUE directly from
    * the URB. This is controlled by the 3DSTATE_VERTEX_BUFFERS and
    * 3DSTATE_VERTEX_ELEMENTS packets below. The VUE contents are as follows:
    *   dw0: Reserved, MBZ.
    *   dw1: Render Target Array Index. The HiZ op does not use indexed
    *        vertices, so set the dword to 0.
    *   dw2: Viewport Index. The HiZ op disables viewport mapping and
    *        scissoring, so set the dword to 0.
    *   dw3: Point Width: The HiZ op does not emit the POINTLIST primitive, so
    *        set the dword to 0.
    *   dw4: Vertex Position X.
    *   dw5: Vertex Position Y.
    *   dw6: Vertex Position Z.
    *   dw7: Vertex Position W.
    *
    * For details, see the Sandybridge PRM, Volume 2, Part 1, Section 1.5.1
    * "Vertex URB Entry (VUE) Formats".
    */
   {
      const int width = mt->level[level].width;
      const int height = mt->level[level].height;

      const float vertices[GEN6_HIZ_VBO_SIZE] = {
         /* v0 */ 0, 0, 0, 0,         0, height, 0, 1,
         /* v1 */ 0, 0, 0, 0,     width, height, 0, 1,
         /* v2 */ 0, 0, 0, 0,         0,      0, 0, 1,
      };

      drm_intel_bo_subdata(hiz->vertex_bo, 0, GEN6_HIZ_VBO_SIZE, vertices);
   }

   /* 3DSTATE_VERTEX_BUFFERS */
   {
      const int num_buffers = 1;
      const int batch_length = 1 + 4 * num_buffers;

      uint32_t dw0 = GEN6_VB0_ACCESS_VERTEXDATA |
                     (GEN6_HIZ_NUM_VUE_ELEMS * sizeof(float)) << BRW_VB0_PITCH_SHIFT;

      if (intel->gen >= 7)
         dw0 |= GEN7_VB0_ADDRESS_MODIFYENABLE;

      BEGIN_BATCH(batch_length);
      OUT_BATCH((_3DSTATE_VERTEX_BUFFERS << 16) | (batch_length - 2));
      OUT_BATCH(dw0);
      /* start address */
      OUT_RELOC(hiz->vertex_bo, I915_GEM_DOMAIN_VERTEX, 0, 0);
      /* end address */
      OUT_RELOC(hiz->vertex_bo, I915_GEM_DOMAIN_VERTEX,
                0, hiz->vertex_bo->size - 1);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   /* 3DSTATE_VERTEX_ELEMENTS
    *
    * Fetch dwords 0 - 7 from each VUE. See the comments above where
    * hiz->vertex_bo is filled with data.
    */
   {
      const int num_elements = 2;
      const int batch_length = 1 + 2 * num_elements;

      BEGIN_BATCH(batch_length);
      OUT_BATCH((_3DSTATE_VERTEX_ELEMENTS << 16) | (batch_length - 2));
      /* Element 0 */
      OUT_BATCH(GEN6_VE0_VALID |
                BRW_SURFACEFORMAT_R32G32B32A32_FLOAT << BRW_VE0_FORMAT_SHIFT |
                0 << BRW_VE0_SRC_OFFSET_SHIFT);
      OUT_BATCH(BRW_VE1_COMPONENT_STORE_SRC << BRW_VE1_COMPONENT_0_SHIFT |
                BRW_VE1_COMPONENT_STORE_SRC << BRW_VE1_COMPONENT_1_SHIFT |
                BRW_VE1_COMPONENT_STORE_SRC << BRW_VE1_COMPONENT_2_SHIFT |
                BRW_VE1_COMPONENT_STORE_SRC << BRW_VE1_COMPONENT_3_SHIFT);
      /* Element 1 */
      OUT_BATCH(GEN6_VE0_VALID |
                BRW_SURFACEFORMAT_R32G32B32A32_FLOAT << BRW_VE0_FORMAT_SHIFT |
                16 << BRW_VE0_SRC_OFFSET_SHIFT);
      OUT_BATCH(BRW_VE1_COMPONENT_STORE_SRC << BRW_VE1_COMPONENT_0_SHIFT |
                BRW_VE1_COMPONENT_STORE_SRC << BRW_VE1_COMPONENT_1_SHIFT |
                BRW_VE1_COMPONENT_STORE_SRC << BRW_VE1_COMPONENT_2_SHIFT |
                BRW_VE1_COMPONENT_STORE_SRC << BRW_VE1_COMPONENT_3_SHIFT);
      ADVANCE_BATCH();
   }
}

/**
 * \brief Execute a HiZ op on a miptree slice.
 *
 * To execute the HiZ op, this function manually constructs and emits a batch
 * to "draw" the HiZ op's rectangle primitive. The batchbuffer is flushed
 * before constructing and after emitting the batch.
 *
 * This function alters no GL state.
 *
 * For an overview of HiZ ops, see the following sections of the Sandy Bridge
 * PRM, Volume 1, Part 2:
 *   - 7.5.3.1 Depth Buffer Clear
 *   - 7.5.3.2 Depth Buffer Resolve
 *   - 7.5.3.3 Hierarchical Depth Buffer Resolve
 */
static void
gen6_hiz_exec(struct intel_context *intel,
              struct intel_mipmap_tree *mt,
              unsigned int level,
              unsigned int layer,
              enum gen6_hiz_op op)
{
   struct gl_context *ctx = &intel->ctx;
   struct brw_context *brw = brw_context(ctx);
   struct brw_hiz_state *hiz = &brw->hiz;

   assert(op != GEN6_HIZ_OP_DEPTH_CLEAR); /* Not implemented yet. */
   assert(mt->hiz_mt != NULL);
   intel_miptree_check_level_layer(mt, level, layer);

   if (hiz->vertex_bo == NULL)
      gen6_hiz_init(brw);

   if (hiz->vertex_bo == NULL) {
      /* Ouch. Give up. */
      return;
   }

   gen6_hiz_emit_batch_head(brw);
   gen6_hiz_emit_vertices(brw, mt, level, layer);

   /* 3DSTATE_URB
    *
    * Assign the entire URB to the VS. Even though the VS disabled, URB space
    * is still needed because the clipper loads the VUE's from the URB. From
    * the Sandybridge PRM, Volume 2, Part 1, Section 3DSTATE,
    * Dword 1.15:0 "VS Number of URB Entries":
    *     This field is always used (even if VS Function Enable is DISABLED).
    *
    * The warning below appears in the PRM (Section 3DSTATE_URB), but we can
    * safely ignore it because this batch contains only one draw call.
    *     Because of URB corruption caused by allocating a previous GS unit
    *     URB entry to the VS unit, software is required to send a “GS NULL
    *     Fence” (Send URB fence with VS URB size == 1 and GS URB size == 0)
    *     plus a dummy DRAW call before any case where VS will be taking over
    *     GS URB space.
    */
   {
      BEGIN_BATCH(3);
      OUT_BATCH(_3DSTATE_URB << 16 | (3 - 2));
      OUT_BATCH(brw->urb.max_vs_entries << GEN6_URB_VS_ENTRIES_SHIFT);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   /* 3DSTATE_CC_STATE_POINTERS
    *
    * The pointer offsets are relative to
    * CMD_STATE_BASE_ADDRESS.DynamicStateBaseAddress.
    *
    * The HiZ op doesn't use BLEND_STATE or COLOR_CALC_STATE.
    */
   {
      uint32_t depthstencil_offset;
      gen6_hiz_emit_depth_stencil_state(brw, op, &depthstencil_offset);

      BEGIN_BATCH(4);
      OUT_BATCH(_3DSTATE_CC_STATE_POINTERS << 16 | (4 - 2));
      OUT_BATCH(1); /* BLEND_STATE offset */
      OUT_BATCH(depthstencil_offset | 1); /* DEPTH_STENCIL_STATE offset */
      OUT_BATCH(1); /* COLOR_CALC_STATE offset */
      ADVANCE_BATCH();
   }

   /* 3DSTATE_VS
    *
    * Disable vertex shader.
    */
   {
      /* From the BSpec, Volume 2a, Part 3 "Vertex Shader", Section
       * 3DSTATE_VS, Dword 5.0 "VS Function Enable":
       *   [DevSNB] A pipeline flush must be programmed prior to a 3DSTATE_VS
       *   command that causes the VS Function Enable to toggle. Pipeline
       *   flush can be executed by sending a PIPE_CONTROL command with CS
       *   stall bit set and a post sync operation.
       */
      intel_emit_post_sync_nonzero_flush(intel);

      BEGIN_BATCH(6);
      OUT_BATCH(_3DSTATE_VS << 16 | (6 - 2));
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   /* 3DSTATE_GS
    *
    * Disable the geometry shader.
    */
   {
      BEGIN_BATCH(7);
      OUT_BATCH(_3DSTATE_GS << 16 | (7 - 2));
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   /* 3DSTATE_CLIP
    *
    * Disable the clipper.
    *
    * The HiZ op emits a rectangle primitive, which requires clipping to
    * be disabled. From page 10 of the Sandy Bridge PRM Volume 2 Part 1
    * Section 1.3 "3D Primitives Overview":
    *    RECTLIST:
    *    Either the CLIP unit should be DISABLED, or the CLIP unit's Clip
    *    Mode should be set to a value other than CLIPMODE_NORMAL.
    *
    * Also disable perspective divide. This doesn't change the clipper's
    * output, but does spare a few electrons.
    */
   {
      BEGIN_BATCH(4);
      OUT_BATCH(_3DSTATE_CLIP << 16 | (4 - 2));
      OUT_BATCH(0);
      OUT_BATCH(GEN6_CLIP_PERSPECTIVE_DIVIDE_DISABLE);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   /* 3DSTATE_SF
    *
    * Disable ViewportTransformEnable (dw2.1)
    *
    * From the SandyBridge PRM, Volume 2, Part 1, Section 1.3, "3D
    * Primitives Overview":
    *     RECTLIST: Viewport Mapping must be DISABLED (as is typical with the
    *     use of screen- space coordinates).
    *
    * A solid rectangle must be rendered, so set FrontFaceFillMode (dw2.4:3)
    * and BackFaceFillMode (dw2.5:6) to SOLID(0).
    *
    * From the Sandy Bridge PRM, Volume 2, Part 1, Section
    * 6.4.1.1 3DSTATE_SF, Field FrontFaceFillMode:
    *     SOLID: Any triangle or rectangle object found to be front-facing
    *     is rendered as a solid object. This setting is required when
    *     (rendering rectangle (RECTLIST) objects.
    */
   {
      BEGIN_BATCH(20);
      OUT_BATCH(_3DSTATE_SF << 16 | (20 - 2));
      OUT_BATCH((1 - 1) << GEN6_SF_NUM_OUTPUTS_SHIFT | /* only position */
                1 << GEN6_SF_URB_ENTRY_READ_LENGTH_SHIFT |
                0 << GEN6_SF_URB_ENTRY_READ_OFFSET_SHIFT);
      for (int i = 0; i < 18; ++i)
         OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   /* 3DSTATE_WM
    *
    * Disable thread dispatch (dw5.19) and enable the HiZ op.
    *
    * Even though thread dispatch is disabled, max threads (dw5.25:31) must be
    * nonzero to prevent the GPU from hanging. See the valid ranges in the
    * BSpec, Volume 2a.11 Windower, Section 3DSTATE_WM, Dword 5.25:31
    * "Maximum Number Of Threads".
    */
   {
      uint32_t dw4 = 0;

      switch (op) {
      case GEN6_HIZ_OP_DEPTH_CLEAR:
         assert(!"not implemented");
         dw4 |= GEN6_WM_DEPTH_CLEAR;
         break;
      case GEN6_HIZ_OP_DEPTH_RESOLVE:
         dw4 |= GEN6_WM_DEPTH_RESOLVE;
         break;
      case GEN6_HIZ_OP_HIZ_RESOLVE:
         dw4 |= GEN6_WM_HIERARCHICAL_DEPTH_RESOLVE;
         break;
      default:
         assert(0);
         break;
      }

      BEGIN_BATCH(9);
      OUT_BATCH(_3DSTATE_WM << 16 | (9 - 2));
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(dw4);
      OUT_BATCH((brw->max_wm_threads - 1) << GEN6_WM_MAX_THREADS_SHIFT);
      OUT_BATCH((1 - 1) << GEN6_WM_NUM_SF_OUTPUTS_SHIFT); /* only position */
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   /* 3DSTATE_DEPTH_BUFFER */
   {
      uint32_t width = mt->level[level].width;
      uint32_t height = mt->level[level].height;

      uint32_t tile_x;
      uint32_t tile_y;
      uint32_t offset;
      {
         /* Construct a dummy renderbuffer just to extract tile offsets. */
         struct intel_renderbuffer rb;
         rb.mt = mt;
         rb.mt_level = level;
         rb.mt_layer = layer;
         intel_renderbuffer_set_draw_offset(&rb);
         offset = intel_renderbuffer_tile_offsets(&rb, &tile_x, &tile_y);
      }

      uint32_t format;
      switch (mt->format) {
      case MESA_FORMAT_Z16:       format = BRW_DEPTHFORMAT_D16_UNORM; break;
      case MESA_FORMAT_Z32_FLOAT: format = BRW_DEPTHFORMAT_D32_FLOAT; break;
      case MESA_FORMAT_X8_Z24:    format = BRW_DEPTHFORMAT_D24_UNORM_X8_UINT; break;
      default:                    assert(0); break;
      }

      intel_emit_post_sync_nonzero_flush(intel);
      intel_emit_depth_stall_flushes(intel);

      BEGIN_BATCH(7);
      OUT_BATCH(_3DSTATE_DEPTH_BUFFER << 16 | (7 - 2));
      OUT_BATCH(((mt->region->pitch * mt->region->cpp) - 1) |
                format << 18 |
                1 << 21 | /* separate stencil enable */
                1 << 22 | /* hiz enable */
                BRW_TILEWALK_YMAJOR << 26 |
                1 << 27 | /* y-tiled */
                BRW_SURFACE_2D << 29);
      OUT_RELOC(mt->region->bo,
                I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
                offset);
      OUT_BATCH(BRW_SURFACE_MIPMAPLAYOUT_BELOW << 1 |
                (width + tile_x - 1) << 6 |
                (height + tile_y - 1) << 19);
      OUT_BATCH(0);
      OUT_BATCH(tile_x |
                tile_y << 16);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   /* 3DSTATE_HIER_DEPTH_BUFFER */
   {
      struct intel_region *hiz_region = mt->hiz_mt->region;

      BEGIN_BATCH(3);
      OUT_BATCH((_3DSTATE_HIER_DEPTH_BUFFER << 16) | (3 - 2));
      OUT_BATCH(hiz_region->pitch * hiz_region->cpp - 1);
      OUT_RELOC(hiz_region->bo,
                I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
                0);
      ADVANCE_BATCH();
   }

   /* 3DSTATE_STENCIL_BUFFER */
   {
      BEGIN_BATCH(3);
      OUT_BATCH((_3DSTATE_STENCIL_BUFFER << 16) | (3 - 2));
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   /* 3DSTATE_CLEAR_PARAMS
    *
    * From the Sandybridge PRM, Volume 2, Part 1, Section 3DSTATE_CLEAR_PARAMS:
    *   [DevSNB] 3DSTATE_CLEAR_PARAMS packet must follow the DEPTH_BUFFER_STATE
    *   packet when HiZ is enabled and the DEPTH_BUFFER_STATE changes.
    */
   {
      BEGIN_BATCH(2);
      OUT_BATCH(_3DSTATE_CLEAR_PARAMS << 16 | (2 - 2));
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   /* 3DSTATE_DRAWING_RECTANGLE */
   {
      BEGIN_BATCH(4);
      OUT_BATCH(_3DSTATE_DRAWING_RECTANGLE << 16 | (4 - 2));
      OUT_BATCH(0);
      OUT_BATCH(((mt->level[level].width - 1) & 0xffff) |
                ((mt->level[level].height - 1) << 16));
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   /* 3DPRIMITIVE */
   {
     BEGIN_BATCH(6);
     OUT_BATCH(CMD_3D_PRIM << 16 | (6 - 2) |
               _3DPRIM_RECTLIST << GEN4_3DPRIM_TOPOLOGY_TYPE_SHIFT |
               GEN4_3DPRIM_VERTEXBUFFER_ACCESS_SEQUENTIAL);
     OUT_BATCH(3); /* vertex count per instance */
     OUT_BATCH(0);
     OUT_BATCH(1); /* instance count */
     OUT_BATCH(0);
     OUT_BATCH(0);
     ADVANCE_BATCH();
   }

   /* See comments above at first invocation of intel_flush() in
    * gen6_hiz_emit_batch_head().
    */
   intel_flush(ctx);

   /* Be safe. */
   brw->state.dirty.brw = ~0;
   brw->state.dirty.cache = ~0;
}

/**
 * \param out_offset is relative to
 *        CMD_STATE_BASE_ADDRESS.DynamicStateBaseAddress.
 */
void
gen6_hiz_emit_depth_stencil_state(struct brw_context *brw,
                                  enum gen6_hiz_op op,
                                  uint32_t *out_offset)
{
   struct gen6_depth_stencil_state *state;
   state = brw_state_batch(brw, AUB_TRACE_DEPTH_STENCIL_STATE,
                              sizeof(*state), 64,
                              out_offset);
   memset(state, 0, sizeof(*state));

   /* See the following sections of the Sandy Bridge PRM, Volume 1, Part2:
    *   - 7.5.3.1 Depth Buffer Clear
    *   - 7.5.3.2 Depth Buffer Resolve
    *   - 7.5.3.3 Hierarchical Depth Buffer Resolve
    */
   state->ds2.depth_write_enable = 1;
   if (op == GEN6_HIZ_OP_DEPTH_RESOLVE) {
      state->ds2.depth_test_enable = 1;
      state->ds2.depth_test_func = COMPAREFUNC_NEVER;
   }
}

/** \see intel_context::vtbl::resolve_hiz_slice */
void
gen6_resolve_hiz_slice(struct intel_context *intel,
                       struct intel_mipmap_tree *mt,
                       uint32_t level,
                       uint32_t layer)
{
   gen6_hiz_exec(intel, mt, level, layer, GEN6_HIZ_OP_HIZ_RESOLVE);
}

/** \see intel_context::vtbl::resolve_depth_slice */
void
gen6_resolve_depth_slice(struct intel_context *intel,
                         struct intel_mipmap_tree *mt,
                         uint32_t level,
                         uint32_t layer)
{
   gen6_hiz_exec(intel, mt, level, layer, GEN6_HIZ_OP_DEPTH_RESOLVE);
}
