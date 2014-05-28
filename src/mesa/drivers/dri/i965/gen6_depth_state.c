/*
 * Copyright (c) 2014 Intel Corporation
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
#include "intel_fbo.h"
#include "intel_mipmap_tree.h"

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"

#include "main/mtypes.h"
#include "main/fbobject.h"
#include "main/glformats.h"

void
gen6_emit_depth_stencil_hiz(struct brw_context *brw,
                            struct intel_mipmap_tree *depth_mt,
                            uint32_t depth_offset, uint32_t depthbuffer_format,
                            uint32_t depth_surface_type,
                            struct intel_mipmap_tree *stencil_mt,
                            bool hiz, bool separate_stencil,
                            uint32_t width, uint32_t height,
                            uint32_t tile_x, uint32_t tile_y)
{
   struct gl_context *ctx = &brw->ctx;
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   uint32_t surftype;
   unsigned int depth = 1;
   GLenum gl_target = GL_TEXTURE_2D;
   unsigned int lod;
   const struct intel_mipmap_tree *mt = depth_mt ? depth_mt : stencil_mt;
   const struct intel_renderbuffer *irb = NULL;
   const struct gl_renderbuffer *rb = NULL;

   /* Enable the hiz bit if we're doing separate stencil, because it and the
    * separate stencil bit must have the same value. From Section 2.11.5.6.1.1
    * 3DSTATE_DEPTH_BUFFER, Bit 1.21 "Separate Stencil Enable":
    *     [DevIL]: If this field is enabled, Hierarchical Depth Buffer
    *     Enable must also be enabled.
    *
    *     [DevGT]: This field must be set to the same value (enabled or
    *     disabled) as Hierarchical Depth Buffer Enable
    */
   bool enable_hiz_ss = hiz || separate_stencil;


   /* 3DSTATE_DEPTH_BUFFER, 3DSTATE_STENCIL_BUFFER are both
    * non-pipelined state that will need the PIPE_CONTROL workaround.
    */
   intel_emit_post_sync_nonzero_flush(brw);
   intel_emit_depth_stall_flushes(brw);

   irb = intel_get_renderbuffer(fb, BUFFER_DEPTH);
   if (!irb)
      irb = intel_get_renderbuffer(fb, BUFFER_STENCIL);
   rb = (struct gl_renderbuffer*) irb;

   if (rb) {
      depth = MAX2(rb->Depth, 1);
      if (rb->TexImage)
         gl_target = rb->TexImage->TexObject->Target;
   }

   switch (gl_target) {
   case GL_TEXTURE_CUBE_MAP_ARRAY:
   case GL_TEXTURE_CUBE_MAP:
      /* The PRM claims that we should use BRW_SURFACE_CUBE for this
       * situation, but experiments show that gl_Layer doesn't work when we do
       * this.  So we use BRW_SURFACE_2D, since for rendering purposes this is
       * equivalent.
       */
      surftype = BRW_SURFACE_2D;
      depth *= 6;
      break;
   default:
      surftype = translate_tex_target(gl_target);
      break;
   }

   const unsigned min_array_element = irb ? irb->mt_layer : 0;

   lod = irb ? irb->mt_level - irb->mt->first_level : 0;

   if (mt) {
      width = mt->logical_width0;
      height = mt->logical_height0;
   }

   BEGIN_BATCH(7);
   /* 3DSTATE_DEPTH_BUFFER dw0 */
   OUT_BATCH(_3DSTATE_DEPTH_BUFFER << 16 | (7 - 2));

   /* 3DSTATE_DEPTH_BUFFER dw1 */
   OUT_BATCH((depth_mt ? depth_mt->pitch - 1 : 0) |
             (depthbuffer_format << 18) |
             ((enable_hiz_ss ? 1 : 0) << 21) | /* separate stencil enable */
             ((enable_hiz_ss ? 1 : 0) << 22) | /* hiz enable */
             (BRW_TILEWALK_YMAJOR << 26) |
             ((depth_mt ? depth_mt->tiling != I915_TILING_NONE : 1)
              << 27) |
             (surftype << 29));

   /* 3DSTATE_DEPTH_BUFFER dw2 */
   if (depth_mt) {
      OUT_RELOC(depth_mt->bo,
		I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
		0);
   } else {
      OUT_BATCH(0);
   }

   /* 3DSTATE_DEPTH_BUFFER dw3 */
   OUT_BATCH(((width - 1) << 6) |
             ((height - 1) << 19) |
             lod << 2);

   /* 3DSTATE_DEPTH_BUFFER dw4 */
   OUT_BATCH((depth - 1) << 21 |
             min_array_element << 10 |
             (depth - 1) << 1);

   /* 3DSTATE_DEPTH_BUFFER dw5 */
   OUT_BATCH(0);
   assert(tile_x == 0 && tile_y == 0);

   /* 3DSTATE_DEPTH_BUFFER dw6 */
   OUT_BATCH(0);

   ADVANCE_BATCH();

   if (hiz || separate_stencil) {
      /*
       * In the 3DSTATE_DEPTH_BUFFER batch emitted above, the 'separate
       * stencil enable' and 'hiz enable' bits were set. Therefore we must
       * emit 3DSTATE_HIER_DEPTH_BUFFER and 3DSTATE_STENCIL_BUFFER. Even if
       * there is no stencil buffer, 3DSTATE_STENCIL_BUFFER must be emitted;
       * failure to do so causes hangs on gen5 and a stall on gen6.
       */

      /* Emit hiz buffer. */
      if (hiz) {
         struct intel_mipmap_tree *hiz_mt = depth_mt->hiz_mt;
         uint32_t offset = 0;

         if (hiz_mt->array_layout == ALL_SLICES_AT_EACH_LOD) {
            offset = intel_miptree_get_aligned_offset(
                        hiz_mt,
                        hiz_mt->level[lod].level_x,
                        hiz_mt->level[lod].level_y,
                        false);
         }

	 BEGIN_BATCH(3);
	 OUT_BATCH((_3DSTATE_HIER_DEPTH_BUFFER << 16) | (3 - 2));
	 OUT_BATCH(hiz_mt->pitch - 1);
	 OUT_RELOC(hiz_mt->bo,
		   I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
		   offset);
	 ADVANCE_BATCH();
      } else {
	 BEGIN_BATCH(3);
	 OUT_BATCH((_3DSTATE_HIER_DEPTH_BUFFER << 16) | (3 - 2));
	 OUT_BATCH(0);
	 OUT_BATCH(0);
	 ADVANCE_BATCH();
      }

      /* Emit stencil buffer. */
      if (separate_stencil) {
         uint32_t offset = 0;

         if (stencil_mt->array_layout == ALL_SLICES_AT_EACH_LOD) {
            if (stencil_mt->format == MESA_FORMAT_S_UINT8) {
               /* Note: we can't compute the stencil offset using
                * intel_region_get_aligned_offset(), because stencil_region
                * claims that the region is untiled even though it's W tiled.
                */
               offset =
                  stencil_mt->level[lod].level_y * stencil_mt->pitch +
                  stencil_mt->level[lod].level_x * 64;
            } else {
               offset = intel_miptree_get_aligned_offset(
                           stencil_mt,
                           stencil_mt->level[lod].level_x,
                           stencil_mt->level[lod].level_y,
                           false);
            }
         }

	 BEGIN_BATCH(3);
	 OUT_BATCH((_3DSTATE_STENCIL_BUFFER << 16) | (3 - 2));
         /* The stencil buffer has quirky pitch requirements.  From Vol 2a,
          * 11.5.6.2.1 3DSTATE_STENCIL_BUFFER, field "Surface Pitch":
          *    The pitch must be set to 2x the value computed based on width, as
          *    the stencil buffer is stored with two rows interleaved.
          */
	 OUT_BATCH(2 * stencil_mt->pitch - 1);
	 OUT_RELOC(stencil_mt->bo,
		   I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
		   offset);
	 ADVANCE_BATCH();
      } else {
	 BEGIN_BATCH(3);
	 OUT_BATCH((_3DSTATE_STENCIL_BUFFER << 16) | (3 - 2));
	 OUT_BATCH(0);
	 OUT_BATCH(0);
	 ADVANCE_BATCH();
      }
   }

   /*
    * On Gen >= 6, emit clear params for safety. If using hiz, then clear
    * params must be emitted.
    *
    * From Section 2.11.5.6.4.1 3DSTATE_CLEAR_PARAMS:
    *     3DSTATE_CLEAR_PARAMS packet must follow the DEPTH_BUFFER_STATE packet
    *     when HiZ is enabled and the DEPTH_BUFFER_STATE changes.
    */
   intel_emit_post_sync_nonzero_flush(brw);

   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_CLEAR_PARAMS << 16 |
             GEN5_DEPTH_CLEAR_VALID |
             (2 - 2));
   OUT_BATCH(depth_mt ? depth_mt->depth_clear_value : 0);
   ADVANCE_BATCH();
}
