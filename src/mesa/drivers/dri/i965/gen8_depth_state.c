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
gen8_emit_depth_stencil_hiz(struct brw_context *brw,
                            struct intel_mipmap_tree *depth_mt,
                            uint32_t depth_offset,
                            uint32_t depthbuffer_format,
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
   unsigned int min_array_element;
   GLenum gl_target = GL_TEXTURE_2D;
   unsigned int lod;
   const struct intel_mipmap_tree *mt = depth_mt ? depth_mt : stencil_mt;
   const struct intel_renderbuffer *irb = NULL;
   const struct gl_renderbuffer *rb = NULL;

   intel_emit_depth_stall_flushes(brw);

   irb = intel_get_renderbuffer(fb, BUFFER_DEPTH);
   if (!irb)
      irb = intel_get_renderbuffer(fb, BUFFER_STENCIL);
   rb = (struct gl_renderbuffer *) irb;

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

   if (fb->MaxNumLayers > 0 || !irb) {
      min_array_element = 0;
   } else if (irb->mt->num_samples > 1) {
      /* Convert physical to logical layer. */
      min_array_element = irb->mt_layer / irb->mt->num_samples;
   } else {
      min_array_element = irb->mt_layer;
   }

   lod = irb ? irb->mt_level - irb->mt->first_level : 0;

   if (mt) {
      width = mt->logical_width0;
      height = mt->logical_height0;
   }

   /* _NEW_BUFFERS, _NEW_DEPTH, _NEW_STENCIL */
   BEGIN_BATCH(8);
   OUT_BATCH(GEN7_3DSTATE_DEPTH_BUFFER << 16 | (8 - 2));
   OUT_BATCH((surftype << 29) |
             ((ctx->Depth.Mask != 0) << 28) |
             ((stencil_mt != NULL && ctx->Stencil._WriteEnabled) << 27) |
             ((hiz ? 1 : 0) << 22) |
             (depthbuffer_format << 18) |
             (depth_mt ? depth_mt->region->pitch - 1 : 0));
   if (depth_mt) {
      OUT_RELOC64(depth_mt->region->bo,
                  I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
                  0);
   } else {
      OUT_BATCH(0);
      OUT_BATCH(0);
   }
   OUT_BATCH(((width - 1) << 4) | ((height - 1) << 18) | lod);
   OUT_BATCH(((depth - 1) << 21) | (min_array_element << 10));
   OUT_BATCH(0);
   OUT_BATCH(depth_mt ? depth_mt->qpitch >> 2 : 0);
   ADVANCE_BATCH();

   assert(!hiz); /* TODO: Implement HiZ. */
   BEGIN_BATCH(5);
   OUT_BATCH(GEN7_3DSTATE_HIER_DEPTH_BUFFER << 16 | (5 - 2));
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();

   if (stencil_mt == NULL) {
      BEGIN_BATCH(5);
      OUT_BATCH(GEN7_3DSTATE_STENCIL_BUFFER << 16 | (5 - 2));
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      OUT_BATCH(0);
      ADVANCE_BATCH();
   } else {
      BEGIN_BATCH(5);
      OUT_BATCH(GEN7_3DSTATE_STENCIL_BUFFER << 16 | (5 - 2));
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
      OUT_BATCH(HSW_STENCIL_ENABLED | (2 * stencil_mt->region->pitch - 1));
      OUT_RELOC64(stencil_mt->region->bo,
                  I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
                  brw->depthstencil.stencil_offset);
      OUT_BATCH(stencil_mt ? stencil_mt->qpitch >> 2 : 0);
      ADVANCE_BATCH();
   }

   BEGIN_BATCH(3);
   OUT_BATCH(GEN7_3DSTATE_CLEAR_PARAMS << 16 | (3 - 2));
   OUT_BATCH(depth_mt ? depth_mt->depth_clear_value : 0);
   OUT_BATCH(1);
   ADVANCE_BATCH();
}
