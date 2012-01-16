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

static void emit_depthbuffer(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   struct gl_framebuffer *fb = ctx->DrawBuffer;

   /* _NEW_BUFFERS */
   struct intel_renderbuffer *drb = intel_get_renderbuffer(fb, BUFFER_DEPTH);
   struct intel_renderbuffer *srb = intel_get_renderbuffer(fb, BUFFER_STENCIL);
   struct intel_mipmap_tree *depth_mt = NULL,
			    *stencil_mt = NULL,
			    *hiz_mt = NULL;

   if (drb)
      depth_mt = drb->mt;

   if (depth_mt)
      hiz_mt = depth_mt->hiz_mt;

   if (srb) {
      stencil_mt = srb->mt;
      if (stencil_mt->stencil_mt)
	 stencil_mt = stencil_mt->stencil_mt;

      assert(stencil_mt->format == MESA_FORMAT_S8);
   }

   /* Gen7 doesn't support packed depth/stencil */
   assert(stencil_mt == NULL || depth_mt != stencil_mt);
   assert(!depth_mt || !_mesa_is_format_packed_depth_stencil(depth_mt->format));

   intel_emit_depth_stall_flushes(intel);

   if (depth_mt == NULL) {
      uint32_t dw1 = BRW_DEPTHFORMAT_D32_FLOAT << 18;
      uint32_t dw3 = 0;

      if (stencil_mt == NULL) {
	 dw1 |= (BRW_SURFACE_NULL << 29);
      } else {
	 /* _NEW_STENCIL: enable stencil buffer writes */
	 dw1 |= ((ctx->Stencil.WriteMask != 0) << 27);

	 /* 3DSTATE_STENCIL_BUFFER inherits surface type and dimensions. */
	 dw1 |= (BRW_SURFACE_2D << 29);
	 dw3 = ((srb->Base.Base.Width - 1) << 4) |
	       ((srb->Base.Base.Height - 1) << 18);
      }

      BEGIN_BATCH(7);
      OUT_BATCH(GEN7_3DSTATE_DEPTH_BUFFER << 16 | (7 - 2));
      OUT_BATCH(dw1);
      OUT_BATCH(0);
      OUT_BATCH(dw3);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   } else {
      struct intel_region *region = depth_mt->region;
      uint32_t tile_x, tile_y, offset;

      offset = intel_renderbuffer_tile_offsets(drb, &tile_x, &tile_y);

      assert(region->tiling == I915_TILING_Y);

      /* _NEW_DEPTH, _NEW_STENCIL */
      BEGIN_BATCH(7);
      OUT_BATCH(GEN7_3DSTATE_DEPTH_BUFFER << 16 | (7 - 2));
      OUT_BATCH(((region->pitch * region->cpp) - 1) |
		(brw_depthbuffer_format(brw) << 18) |
		((hiz_mt ? 1 : 0) << 22) | /* hiz enable */
		((stencil_mt != NULL && ctx->Stencil.WriteMask != 0) << 27) |
		((ctx->Depth.Mask != 0) << 28) |
		(BRW_SURFACE_2D << 29));
      OUT_RELOC(region->bo,
	        I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
		offset);
      OUT_BATCH((((drb->Base.Base.Width + tile_x) - 1) << 4) |
                (((drb->Base.Base.Height + tile_y) - 1) << 18));
      OUT_BATCH(0);
      OUT_BATCH(tile_x | (tile_y << 16));
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   if (hiz_mt == NULL) {
      BEGIN_BATCH(3);
      OUT_BATCH(GEN7_3DSTATE_HIER_DEPTH_BUFFER << 16 | (3 - 2));
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   } else {
      BEGIN_BATCH(3);
      OUT_BATCH(GEN7_3DSTATE_HIER_DEPTH_BUFFER << 16 | (3 - 2));
      OUT_BATCH(hiz_mt->region->pitch * hiz_mt->region->cpp - 1);
      OUT_RELOC(hiz_mt->region->bo,
                I915_GEM_DOMAIN_RENDER,
                I915_GEM_DOMAIN_RENDER,
                0);
      ADVANCE_BATCH();
   }

   if (stencil_mt == NULL) {
      BEGIN_BATCH(3);
      OUT_BATCH(GEN7_3DSTATE_STENCIL_BUFFER << 16 | (3 - 2));
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   } else {
      BEGIN_BATCH(3);
      OUT_BATCH(GEN7_3DSTATE_STENCIL_BUFFER << 16 | (3 - 2));
      OUT_BATCH(stencil_mt->region->pitch * stencil_mt->region->cpp - 1);
      OUT_RELOC(stencil_mt->region->bo,
	        I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
		0);
      ADVANCE_BATCH();
   }

   BEGIN_BATCH(3);
   OUT_BATCH(GEN7_3DSTATE_CLEAR_PARAMS << 16 | (3 - 2));
   OUT_BATCH(0);
   OUT_BATCH(0);
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
   .emit = emit_depthbuffer,
};
