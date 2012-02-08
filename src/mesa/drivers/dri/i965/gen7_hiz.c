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
#include "gen7_hiz.h"

/**
 * \copydoc gen6_hiz_exec()
 */
static void
gen7_hiz_exec(struct intel_context *intel,
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

   uint32_t depth_format;
   switch (mt->format) {
   case MESA_FORMAT_Z16:       depth_format = BRW_DEPTHFORMAT_D16_UNORM; break;
   case MESA_FORMAT_Z32_FLOAT: depth_format = BRW_DEPTHFORMAT_D32_FLOAT; break;
   case MESA_FORMAT_X8_Z24:    depth_format = BRW_DEPTHFORMAT_D24_UNORM_X8_UINT; break;
   default:                    assert(0); break;
   }

   gen6_hiz_emit_batch_head(brw);
   gen6_hiz_emit_vertices(brw, mt, level, layer);

   /* 3DSTATE_URB_VS
    * 3DSTATE_URB_HS
    * 3DSTATE_URB_DS
    * 3DSTATE_URB_GS
    *
    * If the 3DSTATE_URB_VS is emitted, than the others must be also. From the
    * BSpec, Volume 2a "3D Pipeline Overview", Section 1.7.1 3DSTATE_URB_VS:
    *     3DSTATE_URB_HS, 3DSTATE_URB_DS, and 3DSTATE_URB_GS must also be
    *     programmed in order for the programming of this state to be
    *     valid.
    */
   {
      /* The minimum valid value is 32. See 3DSTATE_URB_VS,
       * Dword 1.15:0 "VS Number of URB Entries".
       */
      int num_vs_entries = 32;

      BEGIN_BATCH(2);
      OUT_BATCH(_3DSTATE_URB_VS << 16 | (2 - 2));
      OUT_BATCH(1 << GEN7_URB_ENTRY_SIZE_SHIFT |
                0 << GEN7_URB_STARTING_ADDRESS_SHIFT |
                num_vs_entries);
      ADVANCE_BATCH();

      BEGIN_BATCH(2);
      OUT_BATCH(_3DSTATE_URB_GS << 16 | (2 - 2));
      OUT_BATCH(0);
      ADVANCE_BATCH();

      BEGIN_BATCH(2);
      OUT_BATCH(_3DSTATE_URB_HS << 16 | (2 - 2));
      OUT_BATCH(0);
      ADVANCE_BATCH();

      BEGIN_BATCH(2);
      OUT_BATCH(_3DSTATE_URB_DS << 16 | (2 - 2));
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   /* 3DSTATE_DEPTH_STENCIL_STATE_POINTERS
    *
    * The offset is relative to CMD_STATE_BASE_ADDRESS.DynamicStateBaseAddress.
    */
   {
      uint32_t depthstencil_offset;
      gen6_hiz_emit_depth_stencil_state(brw, op, &depthstencil_offset);

      BEGIN_BATCH(2);
      OUT_BATCH(_3DSTATE_DEPTH_STENCIL_STATE_POINTERS << 16 | (2 - 2));
      OUT_BATCH(depthstencil_offset | 1);
      ADVANCE_BATCH();
   }

   /* 3DSTATE_VS
    *
    * Disable vertex shader.
    */
   {
      BEGIN_BATCH(6);
      OUT_BATCH(_3DSTATE_VS << 16 | (6 - 2));
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   /* 3DSTATE_HS
    *
    * Disable the hull shader.
    */
   {
      BEGIN_BATCH(7);
      OUT_BATCH(_3DSTATE_HS << 16 | (7 - 2));
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   /* 3DSTATE_TE
    *
    * Disable the tesselation engine.
    */
   {
      BEGIN_BATCH(4);
      OUT_BATCH(_3DSTATE_TE << 16 | (4 - 2));
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   /* 3DSTATE_DS
    *
    * Disable the domain shader.
    */
   {
      BEGIN_BATCH(6);
      OUT_BATCH(_3DSTATE_DS << 16 | (6 - 2));
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

   /* 3DSTATE_STREAMOUT
    *
    * Disable streamout.
    */
   {
      BEGIN_BATCH(3);
      OUT_BATCH(_3DSTATE_STREAMOUT << 16 | (3 - 2));
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
    * Disable ViewportTransformEnable (dw1.1)
    *
    * From the SandyBridge PRM, Volume 2, Part 1, Section 1.3, "3D
    * Primitives Overview":
    *     RECTLIST: Viewport Mapping must be DISABLED (as is typical with the
    *     use of screen- space coordinates).
    *
    * A solid rectangle must be rendered, so set FrontFaceFillMode (dw1.6:5)
    * and BackFaceFillMode (dw1.4:3) to SOLID(0).
    *
    * From the Sandy Bridge PRM, Volume 2, Part 1, Section
    * 6.4.1.1 3DSTATE_SF, Field FrontFaceFillMode:
    *     SOLID: Any triangle or rectangle object found to be front-facing
    *     is rendered as a solid object. This setting is required when
    *     (rendering rectangle (RECTLIST) objects.
    */
   {
      BEGIN_BATCH(7);
      OUT_BATCH(_3DSTATE_SF << 16 | (7 - 2));
      OUT_BATCH(depth_format << GEN7_SF_DEPTH_BUFFER_SURFACE_FORMAT_SHIFT);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   /* 3DSTATE_SBE */
   {
      BEGIN_BATCH(14);
      OUT_BATCH(_3DSTATE_SBE << 16 | (14 - 2));
      OUT_BATCH((1 - 1) << GEN7_SBE_NUM_OUTPUTS_SHIFT | /* only position */
                1 << GEN7_SBE_URB_ENTRY_READ_LENGTH_SHIFT |
                0 << GEN7_SBE_URB_ENTRY_READ_OFFSET_SHIFT);
      for (int i = 0; i < 12; ++i)
         OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   /* 3DSTATE_WM
    *
    * Disable PS thread dispatch (dw1.29) and enable the HiZ op.
    */
   {
      uint32_t dw1 = 0;

      switch (op) {
      case GEN6_HIZ_OP_DEPTH_CLEAR:
         assert(!"not implemented");
         dw1 |= GEN7_WM_DEPTH_CLEAR;
         break;
      case GEN6_HIZ_OP_DEPTH_RESOLVE:
         dw1 |= GEN7_WM_DEPTH_RESOLVE;
         break;
      case GEN6_HIZ_OP_HIZ_RESOLVE:
         dw1 |= GEN7_WM_HIERARCHICAL_DEPTH_RESOLVE;
         break;
      default:
         assert(0);
         break;
      }

      BEGIN_BATCH(3);
      OUT_BATCH(_3DSTATE_WM << 16 | (3 - 2));
      OUT_BATCH(dw1);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   /* 3DSTATE_PS
    *
    * Pixel shader dispatch is disabled above in 3DSTATE_WM, dw1.29. Despite
    * that, thread dispatch info must still be specified.
    *     - Maximum Number of Threads (dw4.24:31) must be nonzero, as the BSpec
    *       states that the valid range for this field is [0x3, 0x2f].
    *     - A dispatch mode must be given; that is, at least one of the
    *       "N Pixel Dispatch Enable" (N=8,16,32) fields must be set. This was
    *       discovered through simulator error messages.
    */
   {
      BEGIN_BATCH(8);
      OUT_BATCH(_3DSTATE_PS << 16 | (8 - 2));
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(((brw->max_wm_threads - 1) << GEN7_PS_MAX_THREADS_SHIFT) |
		GEN7_PS_32_DISPATCH_ENABLE);
      OUT_BATCH(0);
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

      intel_emit_depth_stall_flushes(intel);

      BEGIN_BATCH(7);
      OUT_BATCH(GEN7_3DSTATE_DEPTH_BUFFER << 16 | (7 - 2));
      OUT_BATCH(((mt->region->pitch * mt->region->cpp) - 1) |
                depth_format << 18 |
                1 << 22 | /* hiz enable */
                1 << 28 | /* depth write */
                BRW_SURFACE_2D << 29);
      OUT_RELOC(mt->region->bo,
                I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
                offset);
      OUT_BATCH((width + tile_x - 1) << 4 |
                (height + tile_y - 1) << 18);
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
      OUT_BATCH((GEN7_3DSTATE_HIER_DEPTH_BUFFER << 16) | (3 - 2));
      OUT_BATCH(hiz_region->pitch * hiz_region->cpp - 1);
      OUT_RELOC(hiz_region->bo,
                I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
                0);
      ADVANCE_BATCH();
   }

   /* 3DSTATE_STENCIL_BUFFER */
   {
      BEGIN_BATCH(3);
      OUT_BATCH((GEN7_3DSTATE_STENCIL_BUFFER << 16) | (3 - 2));
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   /* 3DSTATE_CLEAR_PARAMS
    *
    * From the BSpec, Volume 2a.11 Windower, Section 1.5.6.3.2
    * 3DSTATE_CLEAR_PARAMS:
    *    [DevIVB] 3DSTATE_CLEAR_PARAMS must always be programmed in the along
    *    with the other Depth/Stencil state commands(i.e.  3DSTATE_DEPTH_BUFFER,
    *    3DSTATE_STENCIL_BUFFER, or 3DSTATE_HIER_DEPTH_BUFFER).
    */
   {
      BEGIN_BATCH(3);
      OUT_BATCH(GEN7_3DSTATE_CLEAR_PARAMS << 16 | (3 - 2));
      OUT_BATCH(0);
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
     BEGIN_BATCH(7);
     OUT_BATCH(CMD_3D_PRIM << 16 | (7 - 2));
     OUT_BATCH(GEN7_3DPRIM_VERTEXBUFFER_ACCESS_SEQUENTIAL |
               _3DPRIM_RECTLIST);
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

/** \copydoc gen6_resolve_hiz_slice() */
void
gen7_resolve_hiz_slice(struct intel_context *intel,
                       struct intel_mipmap_tree *mt,
                       uint32_t level,
                       uint32_t layer)
{
   gen7_hiz_exec(intel, mt, level, layer, GEN6_HIZ_OP_HIZ_RESOLVE);
}

/** \copydoc gen6_resolve_depth_slice() */
void
gen7_resolve_depth_slice(struct intel_context *intel,
                         struct intel_mipmap_tree *mt,
                         uint32_t level,
                         uint32_t layer)
{
   gen7_hiz_exec(intel, mt, level, layer, GEN6_HIZ_OP_DEPTH_RESOLVE);
}
