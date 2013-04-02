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

#include "intel_batchbuffer.h"
#include "intel_mipmap_tree.h"
#include "intel_regions.h"
#include "intel_fbo.h"
#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"

void
gen7_emit_depth_stencil_hiz(struct brw_context *brw,
                            struct intel_mipmap_tree *depth_mt,
                            uint32_t depth_offset, uint32_t depthbuffer_format,
                            uint32_t depth_surface_type,
                            struct intel_mipmap_tree *stencil_mt,
                            struct intel_mipmap_tree *hiz_mt,
                            bool separate_stencil, uint32_t width,
                            uint32_t height, uint32_t tile_x, uint32_t tile_y)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;

   intel_emit_depth_stall_flushes(intel);

   /* _NEW_DEPTH, _NEW_STENCIL */
   BEGIN_BATCH(7);
   OUT_BATCH(GEN7_3DSTATE_DEPTH_BUFFER << 16 | (7 - 2));
   OUT_BATCH((depth_mt ? depth_mt->region->pitch - 1 : 0) |
             (depthbuffer_format << 18) |
             ((hiz_mt ? 1 : 0) << 22) |
             ((stencil_mt != NULL && ctx->Stencil._WriteEnabled) << 27) |
             ((ctx->Depth.Mask != 0) << 28) |
             (depth_surface_type << 29));

   if (depth_mt) {
      OUT_RELOC(depth_mt->region->bo,
	        I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
		depth_offset);
   } else {
      OUT_BATCH(0);
   }

   OUT_BATCH(((width + tile_x - 1) << 4) |
             ((height + tile_y - 1) << 18));
   OUT_BATCH(0);
   OUT_BATCH(tile_x | (tile_y << 16));
   OUT_BATCH(0);
   ADVANCE_BATCH();

   if (hiz_mt == NULL) {
      BEGIN_BATCH(3);
      OUT_BATCH(GEN7_3DSTATE_HIER_DEPTH_BUFFER << 16 | (3 - 2));
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   } else {
      BEGIN_BATCH(3);
      OUT_BATCH(GEN7_3DSTATE_HIER_DEPTH_BUFFER << 16 | (3 - 2));
      OUT_BATCH(hiz_mt->region->pitch - 1);
      OUT_RELOC(hiz_mt->region->bo,
                I915_GEM_DOMAIN_RENDER,
                I915_GEM_DOMAIN_RENDER,
                brw->depthstencil.hiz_offset);
      ADVANCE_BATCH();
   }

   if (stencil_mt == NULL) {
      BEGIN_BATCH(3);
      OUT_BATCH(GEN7_3DSTATE_STENCIL_BUFFER << 16 | (3 - 2));
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   } else {
      const int enabled = intel->is_haswell ? HSW_STENCIL_ENABLED : 0;

      BEGIN_BATCH(3);
      OUT_BATCH(GEN7_3DSTATE_STENCIL_BUFFER << 16 | (3 - 2));
      /* The stencil buffer has quirky pitch requirements.  From the Graphics
       * BSpec: vol2a.11 3D Pipeline Windower > Early Depth/Stencil Processing
       * > Depth/Stencil Buffer State > 3DSTATE_STENCIL_BUFFER [DevIVB+],
       * field "Surface Pitch":
       *
       *    The pitch must be set to 2x the value computed based on width, as
       *    the stencil buffer is stored with two rows interleaved.
       *
       * (Note that it is not 100% clear whether this intended to apply to
       * Gen7; the BSpec flags this comment as "DevILK,DevSNB" (which would
       * imply that it doesn't), however the comment appears on a "DevIVB+"
       * page (which would imply that it does).  Experiments with the hardware
       * indicate that it does.
       */
      OUT_BATCH(enabled |
	        (2 * stencil_mt->region->pitch - 1));
      OUT_RELOC(stencil_mt->region->bo,
	        I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
		brw->depthstencil.stencil_offset);
      ADVANCE_BATCH();
   }

   BEGIN_BATCH(3);
   OUT_BATCH(GEN7_3DSTATE_CLEAR_PARAMS << 16 | (3 - 2));
   OUT_BATCH(depth_mt ? depth_mt->depth_clear_value : 0);
   OUT_BATCH(1);
   ADVANCE_BATCH();
}

/**
 * \see brw_context.state.depth_region
 */
const struct brw_tracked_state gen7_depthbuffer = {
   .dirty = {
      .mesa = (_NEW_BUFFERS | _NEW_DEPTH | _NEW_STENCIL),
      .brw = BRW_NEW_BATCH,
      .cache = 0,
   },
   .emit = brw_emit_depthbuffer,
};
