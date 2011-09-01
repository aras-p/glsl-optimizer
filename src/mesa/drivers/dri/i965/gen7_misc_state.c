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
#include "intel_regions.h"
#include "intel_fbo.h"
#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"

unsigned int
gen7_depth_format(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   struct intel_renderbuffer *drb = intel_get_renderbuffer(fb, BUFFER_DEPTH);
   struct intel_region *region = NULL;

   if (drb)
      region = drb->region;
   else
      return BRW_DEPTHFORMAT_D32_FLOAT;

   switch (region->cpp) {
   case 2:
      return BRW_DEPTHFORMAT_D16_UNORM;
   case 4:
      if (intel->depth_buffer_is_float)
	 return BRW_DEPTHFORMAT_D32_FLOAT;
      else
	 return BRW_DEPTHFORMAT_D24_UNORM_X8_UINT;
   default:
      assert(!"Should not get here.");
   }
   return 0;
}

static void prepare_depthbuffer(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   struct intel_renderbuffer *drb = intel_get_renderbuffer(fb, BUFFER_DEPTH);
   struct intel_renderbuffer *srb = intel_get_renderbuffer(fb, BUFFER_STENCIL);

   if (drb)
      brw_add_validated_bo(brw, drb->region->buffer);
   if (srb)
      brw_add_validated_bo(brw, srb->region->buffer);
}

static void emit_depthbuffer(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   struct gl_framebuffer *fb = ctx->DrawBuffer;

   /* _NEW_BUFFERS */
   struct intel_renderbuffer *drb = intel_get_renderbuffer(fb, BUFFER_DEPTH);
   struct intel_renderbuffer *srb = intel_get_renderbuffer(fb, BUFFER_STENCIL);

   intel_emit_depth_stall_flushes(intel);

   /* Gen7 doesn't support packed depth/stencil */
   assert(srb == NULL || srb != drb);

   if (drb == NULL) {
      uint32_t dw1 = BRW_DEPTHFORMAT_D32_FLOAT << 18;
      uint32_t dw3 = 0;

      if (srb == NULL) {
	 dw1 |= (BRW_SURFACE_NULL << 29);
      } else {
	 struct intel_region *region = srb->region;

	 /* _NEW_STENCIL: enable stencil buffer writes */
	 dw1 |= ((ctx->Stencil.WriteMask != 0) << 27);

	 /* 3DSTATE_STENCIL_BUFFER inherits surface type and dimensions. */
	 dw1 |= (BRW_SURFACE_2D << 29);
	 dw3 = ((region->width - 1) << 4) | ((2 * region->height - 1) << 18);
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
      struct intel_region *region = drb->region;
      uint32_t tile_x, tile_y, offset;

      offset = intel_renderbuffer_tile_offsets(drb, &tile_x, &tile_y);

      assert(region->tiling == I915_TILING_Y);

      /* _NEW_DEPTH, _NEW_STENCIL */
      BEGIN_BATCH(7);
      OUT_BATCH(GEN7_3DSTATE_DEPTH_BUFFER << 16 | (7 - 2));
      OUT_BATCH(((region->pitch * region->cpp) - 1) |
		(gen7_depth_format(brw) << 18) |
		(0 << 22) /* no HiZ buffer */ |
		((srb != NULL && ctx->Stencil.WriteMask != 0) << 27) |
		((ctx->Depth.Mask != 0) << 28) |
		(BRW_SURFACE_2D << 29));
      OUT_RELOC(region->buffer,
	        I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
		offset);
      OUT_BATCH(((region->width - 1) << 4) | ((region->height - 1) << 18));
      OUT_BATCH(0);
      OUT_BATCH(tile_x | (tile_y << 16));
      OUT_BATCH(0);
      ADVANCE_BATCH();
   }

   BEGIN_BATCH(4);
   OUT_BATCH(GEN7_3DSTATE_HIER_DEPTH_BUFFER << 16 | (4 - 2));
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();

   if (srb == NULL) {
      BEGIN_BATCH(3);
      OUT_BATCH(GEN7_3DSTATE_STENCIL_BUFFER << 16 | (3 - 2));
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   } else {
      BEGIN_BATCH(3);
      OUT_BATCH(GEN7_3DSTATE_STENCIL_BUFFER << 16 | (3 - 2));
      OUT_BATCH(srb->region->pitch * srb->region->cpp - 1);
      OUT_RELOC(srb->region->buffer,
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
   .prepare = prepare_depthbuffer,
   .emit = emit_depthbuffer,
};
