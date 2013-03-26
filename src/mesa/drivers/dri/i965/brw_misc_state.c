/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */
 


#include "intel_batchbuffer.h"
#include "intel_fbo.h"
#include "intel_mipmap_tree.h"
#include "intel_regions.h"

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"

#include "main/fbobject.h"
#include "main/glformats.h"

/* Constant single cliprect for framebuffer object or DRI2 drawing */
static void upload_drawing_rect(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;

   BEGIN_BATCH(4);
   OUT_BATCH(_3DSTATE_DRAWING_RECTANGLE << 16 | (4 - 2));
   OUT_BATCH(0); /* xmin, ymin */
   OUT_BATCH(((ctx->DrawBuffer->Width - 1) & 0xffff) |
	    ((ctx->DrawBuffer->Height - 1) << 16));
   OUT_BATCH(0);
   ADVANCE_BATCH();
}

const struct brw_tracked_state brw_drawing_rect = {
   .dirty = {
      .mesa = _NEW_BUFFERS,
      .brw = BRW_NEW_CONTEXT,
      .cache = 0
   },
   .emit = upload_drawing_rect
};

/**
 * Upload the binding table pointers, which point each stage's array of surface
 * state pointers.
 *
 * The binding table pointers are relative to the surface state base address,
 * which points at the batchbuffer containing the streamed batch state.
 */
static void upload_binding_table_pointers(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(6);
   OUT_BATCH(_3DSTATE_BINDING_TABLE_POINTERS << 16 | (6 - 2));
   OUT_BATCH(brw->vs.bind_bo_offset);
   OUT_BATCH(0); /* gs */
   OUT_BATCH(0); /* clip */
   OUT_BATCH(0); /* sf */
   OUT_BATCH(brw->wm.bind_bo_offset);
   ADVANCE_BATCH();
}

const struct brw_tracked_state brw_binding_table_pointers = {
   .dirty = {
      .mesa = 0,
      .brw = (BRW_NEW_BATCH |
	      BRW_NEW_STATE_BASE_ADDRESS |
	      BRW_NEW_VS_BINDING_TABLE |
	      BRW_NEW_GS_BINDING_TABLE |
	      BRW_NEW_PS_BINDING_TABLE),
      .cache = 0,
   },
   .emit = upload_binding_table_pointers,
};

/**
 * Upload the binding table pointers, which point each stage's array of surface
 * state pointers.
 *
 * The binding table pointers are relative to the surface state base address,
 * which points at the batchbuffer containing the streamed batch state.
 */
static void upload_gen6_binding_table_pointers(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;

   BEGIN_BATCH(4);
   OUT_BATCH(_3DSTATE_BINDING_TABLE_POINTERS << 16 |
	     GEN6_BINDING_TABLE_MODIFY_VS |
	     GEN6_BINDING_TABLE_MODIFY_GS |
	     GEN6_BINDING_TABLE_MODIFY_PS |
	     (4 - 2));
   OUT_BATCH(brw->vs.bind_bo_offset); /* vs */
   OUT_BATCH(brw->gs.bind_bo_offset); /* gs */
   OUT_BATCH(brw->wm.bind_bo_offset); /* wm/ps */
   ADVANCE_BATCH();
}

const struct brw_tracked_state gen6_binding_table_pointers = {
   .dirty = {
      .mesa = 0,
      .brw = (BRW_NEW_BATCH |
	      BRW_NEW_STATE_BASE_ADDRESS |
	      BRW_NEW_VS_BINDING_TABLE |
	      BRW_NEW_GS_BINDING_TABLE |
	      BRW_NEW_PS_BINDING_TABLE),
      .cache = 0,
   },
   .emit = upload_gen6_binding_table_pointers,
};

/**
 * Upload pointers to the per-stage state.
 *
 * The state pointers in this packet are all relative to the general state
 * base address set by CMD_STATE_BASE_ADDRESS, which is 0.
 */
static void upload_pipelined_state_pointers(struct brw_context *brw )
{
   struct intel_context *intel = &brw->intel;

   if (intel->gen == 5) {
      /* Need to flush before changing clip max threads for errata. */
      BEGIN_BATCH(1);
      OUT_BATCH(MI_FLUSH);
      ADVANCE_BATCH();
   }

   BEGIN_BATCH(7);
   OUT_BATCH(_3DSTATE_PIPELINED_POINTERS << 16 | (7 - 2));
   OUT_RELOC(intel->batch.bo, I915_GEM_DOMAIN_INSTRUCTION, 0,
	     brw->vs.state_offset);
   if (brw->gs.prog_active)
      OUT_RELOC(brw->intel.batch.bo, I915_GEM_DOMAIN_INSTRUCTION, 0,
		brw->gs.state_offset | 1);
   else
      OUT_BATCH(0);
   OUT_RELOC(brw->intel.batch.bo, I915_GEM_DOMAIN_INSTRUCTION, 0,
	     brw->clip.state_offset | 1);
   OUT_RELOC(brw->intel.batch.bo, I915_GEM_DOMAIN_INSTRUCTION, 0,
	     brw->sf.state_offset);
   OUT_RELOC(brw->intel.batch.bo, I915_GEM_DOMAIN_INSTRUCTION, 0,
	     brw->wm.state_offset);
   OUT_RELOC(brw->intel.batch.bo, I915_GEM_DOMAIN_INSTRUCTION, 0,
	     brw->cc.state_offset);
   ADVANCE_BATCH();

   brw->state.dirty.brw |= BRW_NEW_PSP;
}

static void upload_psp_urb_cbs(struct brw_context *brw )
{
   upload_pipelined_state_pointers(brw);
   brw_upload_urb_fence(brw);
   brw_upload_cs_urb_state(brw);
}

const struct brw_tracked_state brw_psp_urb_cbs = {
   .dirty = {
      .mesa = 0,
      .brw = (BRW_NEW_URB_FENCE |
	      BRW_NEW_BATCH |
	      BRW_NEW_STATE_BASE_ADDRESS),
      .cache = (CACHE_NEW_VS_UNIT | 
		CACHE_NEW_GS_UNIT | 
		CACHE_NEW_GS_PROG | 
		CACHE_NEW_CLIP_UNIT | 
		CACHE_NEW_SF_UNIT | 
		CACHE_NEW_WM_UNIT | 
		CACHE_NEW_CC_UNIT)
   },
   .emit = upload_psp_urb_cbs,
};

uint32_t
brw_depthbuffer_format(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   struct intel_renderbuffer *drb = intel_get_renderbuffer(fb, BUFFER_DEPTH);
   struct intel_renderbuffer *srb;

   if (!drb &&
       (srb = intel_get_renderbuffer(fb, BUFFER_STENCIL)) &&
       !srb->mt->stencil_mt &&
       (intel_rb_format(srb) == MESA_FORMAT_S8_Z24 ||
	intel_rb_format(srb) == MESA_FORMAT_Z32_FLOAT_X24S8)) {
      drb = srb;
   }

   if (!drb)
      return BRW_DEPTHFORMAT_D32_FLOAT;

   switch (drb->mt->format) {
   case MESA_FORMAT_Z16:
      return BRW_DEPTHFORMAT_D16_UNORM;
   case MESA_FORMAT_Z32_FLOAT:
      return BRW_DEPTHFORMAT_D32_FLOAT;
   case MESA_FORMAT_X8_Z24:
      if (intel->gen >= 6) {
	 return BRW_DEPTHFORMAT_D24_UNORM_X8_UINT;
      } else {
	 /* Use D24_UNORM_S8, not D24_UNORM_X8.
	  *
	  * D24_UNORM_X8 was not introduced until Gen5. (See the Ironlake PRM,
	  * Volume 2, Part 1, Section 8.4.6 "Depth/Stencil Buffer State", Bits
	  * 3DSTATE_DEPTH_BUFFER.Surface_Format).
	  *
	  * However, on Gen5, D24_UNORM_X8 may be used only if separate
	  * stencil is enabled, and we never enable it. From the Ironlake PRM,
	  * same section as above, Bit 3DSTATE_DEPTH_BUFFER.Separate_Stencil_Buffer_Enable:
	  *     If this field is disabled, the Surface Format of the depth
	  *     buffer cannot be D24_UNORM_X8_UINT.
	  */
	 return BRW_DEPTHFORMAT_D24_UNORM_S8_UINT;
      }
   case MESA_FORMAT_S8_Z24:
      return BRW_DEPTHFORMAT_D24_UNORM_S8_UINT;
   case MESA_FORMAT_Z32_FLOAT_X24S8:
      return BRW_DEPTHFORMAT_D32_FLOAT_S8X24_UINT;
   default:
      _mesa_problem(ctx, "Unexpected depth format %s\n",
		    _mesa_get_format_name(intel_rb_format(drb)));
      return BRW_DEPTHFORMAT_D16_UNORM;
   }
}

/**
 * Returns the mask of how many bits of x and y must be handled through the
 * depthbuffer's draw offset x and y fields.
 *
 * The draw offset x/y field of the depthbuffer packet is unfortunately shared
 * between the depth, hiz, and stencil buffers.  Because it can be hard to get
 * all 3 to agree on this value, we want to do as much drawing offset
 * adjustment as possible by moving the base offset of the 3 buffers, which is
 * restricted to tile boundaries.
 *
 * For each buffer, the remainder must be applied through the x/y draw offset.
 * This returns the worst-case mask of the low bits that have to go into the
 * packet.  If the 3 buffers don't agree on the drawing offset ANDed with this
 * mask, then we're in trouble.
 */
void
brw_get_depthstencil_tile_masks(struct intel_mipmap_tree *depth_mt,
                                struct intel_mipmap_tree *stencil_mt,
                                uint32_t *out_tile_mask_x,
                                uint32_t *out_tile_mask_y)
{
   uint32_t tile_mask_x = 0, tile_mask_y = 0;

   if (depth_mt) {
      intel_region_get_tile_masks(depth_mt->region,
                                  &tile_mask_x, &tile_mask_y, false);

      struct intel_mipmap_tree *hiz_mt = depth_mt->hiz_mt;
      if (hiz_mt) {
         uint32_t hiz_tile_mask_x, hiz_tile_mask_y;
         intel_region_get_tile_masks(hiz_mt->region,
                                     &hiz_tile_mask_x, &hiz_tile_mask_y, false);

         /* Each HiZ row represents 2 rows of pixels */
         hiz_tile_mask_y = hiz_tile_mask_y << 1 | 1;

         tile_mask_x |= hiz_tile_mask_x;
         tile_mask_y |= hiz_tile_mask_y;
      }
   }

   if (stencil_mt) {
      if (stencil_mt->stencil_mt)
	 stencil_mt = stencil_mt->stencil_mt;

      if (stencil_mt->format == MESA_FORMAT_S8) {
         /* Separate stencil buffer uses 64x64 tiles. */
         tile_mask_x |= 63;
         tile_mask_y |= 63;
      } else {
         uint32_t stencil_tile_mask_x, stencil_tile_mask_y;
         intel_region_get_tile_masks(stencil_mt->region,
                                     &stencil_tile_mask_x,
                                     &stencil_tile_mask_y, false);

         tile_mask_x |= stencil_tile_mask_x;
         tile_mask_y |= stencil_tile_mask_y;
      }
   }

   *out_tile_mask_x = tile_mask_x;
   *out_tile_mask_y = tile_mask_y;
}

static struct intel_mipmap_tree *
get_stencil_miptree(struct intel_renderbuffer *irb)
{
   if (!irb)
      return NULL;
   if (irb->mt->stencil_mt)
      return irb->mt->stencil_mt;
   return irb->mt;
}

void
brw_workaround_depthstencil_alignment(struct brw_context *brw,
                                      GLbitfield clear_mask)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   bool rebase_depth = false;
   bool rebase_stencil = false;
   struct intel_renderbuffer *depth_irb = intel_get_renderbuffer(fb, BUFFER_DEPTH);
   struct intel_renderbuffer *stencil_irb = intel_get_renderbuffer(fb, BUFFER_STENCIL);
   struct intel_mipmap_tree *depth_mt = NULL;
   struct intel_mipmap_tree *stencil_mt = get_stencil_miptree(stencil_irb);
   uint32_t tile_x = 0, tile_y = 0, stencil_tile_x = 0, stencil_tile_y = 0;
   uint32_t stencil_draw_x = 0, stencil_draw_y = 0;
   bool invalidate_depth = clear_mask & BUFFER_BIT_DEPTH;
   bool invalidate_stencil = clear_mask & BUFFER_BIT_STENCIL;

   if (depth_irb)
      depth_mt = depth_irb->mt;

   /* Check if depth buffer is in depth/stencil format.  If so, then it's only
    * safe to invalidate it if we're also clearing stencil, and both depth_irb
    * and stencil_irb point to the same miptree.
    *
    * Note: it's not sufficient to check for the case where
    * _mesa_get_format_base_format(depth_mt->format) == GL_DEPTH_STENCIL,
    * because this fails to catch depth/stencil buffers on hardware that uses
    * separate stencil.  To catch that case, we check whether
    * depth_mt->stencil_mt is non-NULL.
    */
   if (depth_irb && invalidate_depth &&
       (_mesa_get_format_base_format(depth_mt->format) == GL_DEPTH_STENCIL ||
        depth_mt->stencil_mt)) {
      invalidate_depth = invalidate_stencil && depth_irb && stencil_irb
         && depth_irb->mt == stencil_irb->mt;
   }

   uint32_t tile_mask_x, tile_mask_y;
   brw_get_depthstencil_tile_masks(depth_mt, stencil_mt,
                                   &tile_mask_x, &tile_mask_y);

   if (depth_irb) {
      tile_x = depth_irb->draw_x & tile_mask_x;
      tile_y = depth_irb->draw_y & tile_mask_y;

      /* According to the Sandy Bridge PRM, volume 2 part 1, pp326-327
       * (3DSTATE_DEPTH_BUFFER dw5), in the documentation for "Depth
       * Coordinate Offset X/Y":
       *
       *   "The 3 LSBs of both offsets must be zero to ensure correct
       *   alignment"
       */
      if (tile_x & 7 || tile_y & 7)
         rebase_depth = true;

      /* We didn't even have intra-tile offsets before g45. */
      if (intel->gen == 4 && !intel->is_g4x) {
         if (tile_x || tile_y)
            rebase_depth = true;
      }

      if (rebase_depth) {
         perf_debug("HW workaround: blitting depth level %d to a temporary "
                    "to fix alignment (depth tile offset %d,%d)\n",
                    depth_irb->mt_level, tile_x, tile_y);
         intel_renderbuffer_move_to_temp(intel, depth_irb, invalidate_depth);
         /* In the case of stencil_irb being the same packed depth/stencil
          * texture but not the same rb, make it point at our rebased mt, too.
          */
         if (stencil_irb &&
             stencil_irb != depth_irb &&
             stencil_irb->mt == depth_mt) {
            intel_miptree_reference(&stencil_irb->mt, depth_irb->mt);
            intel_renderbuffer_set_draw_offset(stencil_irb);
         }

         stencil_mt = get_stencil_miptree(stencil_irb);

         tile_x = depth_irb->draw_x & tile_mask_x;
         tile_y = depth_irb->draw_y & tile_mask_y;
      }

      if (stencil_irb) {
         stencil_mt = get_stencil_miptree(stencil_irb);
         intel_miptree_get_image_offset(stencil_mt,
                                        stencil_irb->mt_level,
                                        stencil_irb->mt_layer,
                                        &stencil_draw_x, &stencil_draw_y);
         int stencil_tile_x = stencil_draw_x & tile_mask_x;
         int stencil_tile_y = stencil_draw_y & tile_mask_y;

         /* If stencil doesn't match depth, then we'll need to rebase stencil
          * as well.  (if we hadn't decided to rebase stencil before, the
          * post-stencil depth test will also rebase depth to try to match it
          * up).
          */
         if (tile_x != stencil_tile_x ||
             tile_y != stencil_tile_y) {
            rebase_stencil = true;
         }
      }
   }

   /* If we have (just) stencil, check it for ignored low bits as well */
   if (stencil_irb) {
      intel_miptree_get_image_offset(stencil_mt,
                                     stencil_irb->mt_level,
                                     stencil_irb->mt_layer,
                                     &stencil_draw_x, &stencil_draw_y);
      stencil_tile_x = stencil_draw_x & tile_mask_x;
      stencil_tile_y = stencil_draw_y & tile_mask_y;

      if (stencil_tile_x & 7 || stencil_tile_y & 7)
         rebase_stencil = true;

      if (intel->gen == 4 && !intel->is_g4x) {
         if (stencil_tile_x || stencil_tile_y)
            rebase_stencil = true;
      }
   }

   if (rebase_stencil) {
      perf_debug("HW workaround: blitting stencil level %d to a temporary "
                 "to fix alignment (stencil tile offset %d,%d)\n",
                 stencil_irb->mt_level, stencil_tile_x, stencil_tile_y);

      intel_renderbuffer_move_to_temp(intel, stencil_irb, invalidate_stencil);
      stencil_mt = get_stencil_miptree(stencil_irb);

      intel_miptree_get_image_offset(stencil_mt,
                                     stencil_irb->mt_level,
                                     stencil_irb->mt_layer,
                                     &stencil_draw_x, &stencil_draw_y);
      stencil_tile_x = stencil_draw_x & tile_mask_x;
      stencil_tile_y = stencil_draw_y & tile_mask_y;

      if (depth_irb && depth_irb->mt == stencil_irb->mt) {
         intel_miptree_reference(&depth_irb->mt, stencil_irb->mt);
         intel_renderbuffer_set_draw_offset(depth_irb);
      } else if (depth_irb && !rebase_depth) {
         if (tile_x != stencil_tile_x ||
             tile_y != stencil_tile_y) {
            perf_debug("HW workaround: blitting depth level %d to a temporary "
                       "to match stencil level %d alignment (depth tile offset "
                       "%d,%d, stencil offset %d,%d)\n",
                       depth_irb->mt_level,
                       stencil_irb->mt_level,
                       tile_x, tile_y,
                       stencil_tile_x, stencil_tile_y);

            intel_renderbuffer_move_to_temp(intel, depth_irb,
                                            invalidate_depth);

            tile_x = depth_irb->draw_x & tile_mask_x;
            tile_y = depth_irb->draw_y & tile_mask_y;

            if (stencil_irb && stencil_irb->mt == depth_mt) {
               intel_miptree_reference(&stencil_irb->mt, depth_irb->mt);
               intel_renderbuffer_set_draw_offset(stencil_irb);
            }

            WARN_ONCE(stencil_tile_x != tile_x ||
                      stencil_tile_y != tile_y,
                      "Rebased stencil tile offset (%d,%d) doesn't match depth "
                      "tile offset (%d,%d).\n",
                      stencil_tile_x, stencil_tile_y,
                      tile_x, tile_y);
         }
      }
   }

   if (!depth_irb) {
      tile_x = stencil_tile_x;
      tile_y = stencil_tile_y;
   }

   /* While we just tried to get everything aligned, we may have failed to do
    * so in the case of rendering to array or 3D textures, where nonzero faces
    * will still have an offset post-rebase.  At least give an informative
    * warning.
    */
   WARN_ONCE((tile_x & 7) || (tile_y & 7),
             "Depth/stencil buffer needs alignment to 8-pixel boundaries.\n"
             "Truncating offset, bad rendering may occur.\n");
   tile_x &= ~7;
   tile_y &= ~7;

   /* Now, after rebasing, save off the new dephtstencil state so the hardware
    * packets can just dereference that without re-calculating tile offsets.
    */
   brw->depthstencil.tile_x = tile_x;
   brw->depthstencil.tile_y = tile_y;
   brw->depthstencil.depth_offset = 0;
   brw->depthstencil.stencil_offset = 0;
   brw->depthstencil.hiz_offset = 0;
   brw->depthstencil.depth_mt = NULL;
   brw->depthstencil.stencil_mt = NULL;
   brw->depthstencil.hiz_mt = NULL;
   if (depth_irb) {
      depth_mt = depth_irb->mt;
      brw->depthstencil.depth_mt = depth_mt;
      brw->depthstencil.depth_offset =
         intel_region_get_aligned_offset(depth_mt->region,
                                         depth_irb->draw_x & ~tile_mask_x,
                                         depth_irb->draw_y & ~tile_mask_y,
                                         false);
      if (depth_mt->hiz_mt) {
         brw->depthstencil.hiz_mt = depth_mt->hiz_mt;
         brw->depthstencil.hiz_offset =
            intel_region_get_aligned_offset(depth_mt->region,
                                            depth_irb->draw_x & ~tile_mask_x,
                                            (depth_irb->draw_y & ~tile_mask_y) /
                                            2,
                                            false);
      }
   }
   if (stencil_irb) {
      stencil_mt = get_stencil_miptree(stencil_irb);

      brw->depthstencil.stencil_mt = stencil_mt;
      if (stencil_mt->format == MESA_FORMAT_S8) {
         /* Note: we can't compute the stencil offset using
          * intel_region_get_aligned_offset(), because stencil_region claims
          * that the region is untiled even though it's W tiled.
          */
         brw->depthstencil.stencil_offset =
            (stencil_draw_y & ~tile_mask_y) * stencil_mt->region->pitch +
            (stencil_draw_x & ~tile_mask_x) * 64;
      }
   }
}

void
brw_emit_depthbuffer(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   /* _NEW_BUFFERS */
   struct intel_renderbuffer *depth_irb = intel_get_renderbuffer(fb, BUFFER_DEPTH);
   struct intel_renderbuffer *stencil_irb = intel_get_renderbuffer(fb, BUFFER_STENCIL);
   struct intel_mipmap_tree *depth_mt = brw->depthstencil.depth_mt;
   struct intel_mipmap_tree *stencil_mt = brw->depthstencil.stencil_mt;
   struct intel_mipmap_tree *hiz_mt = brw->depthstencil.hiz_mt;
   uint32_t tile_x = brw->depthstencil.tile_x;
   uint32_t tile_y = brw->depthstencil.tile_y;
   bool separate_stencil = false;
   uint32_t depth_surface_type = BRW_SURFACE_NULL;
   uint32_t depthbuffer_format = BRW_DEPTHFORMAT_D32_FLOAT;
   uint32_t depth_offset = 0;
   uint32_t width = 1, height = 1;

   if (stencil_mt) {
      separate_stencil = stencil_mt->format == MESA_FORMAT_S8;

      /* Gen7 supports only separate stencil */
      assert(separate_stencil || intel->gen < 7);
   }

   /* If there's a packed depth/stencil bound to stencil only, we need to
    * emit the packed depth/stencil buffer packet.
    */
   if (!depth_irb && stencil_irb && !separate_stencil) {
      depth_irb = stencil_irb;
      depth_mt = stencil_mt;
   }

   if (depth_irb) {
      struct intel_region *region = depth_mt->region;

      /* When 3DSTATE_DEPTH_BUFFER.Separate_Stencil_Enable is set, then
       * 3DSTATE_DEPTH_BUFFER.Surface_Format is not permitted to be a packed
       * depthstencil format.
       *
       * Gens prior to 7 require that HiZ_Enable and Separate_Stencil_Enable be
       * set to the same value. Gens after 7 implicitly always set
       * Separate_Stencil_Enable; software cannot disable it.
       */
      if ((intel->gen < 7 && depth_mt->hiz_mt) || intel->gen >= 7) {
         assert(!_mesa_is_format_packed_depth_stencil(depth_mt->format));
      }

      /* Prior to Gen7, if using separate stencil, hiz must be enabled. */
      assert(intel->gen >= 7 || !separate_stencil || hiz_mt);

      assert(intel->gen < 6 || region->tiling == I915_TILING_Y);
      assert(!hiz_mt || region->tiling == I915_TILING_Y);

      depthbuffer_format = brw_depthbuffer_format(brw);
      depth_surface_type = BRW_SURFACE_2D;
      depth_offset = brw->depthstencil.depth_offset;
      width = depth_irb->Base.Base.Width;
      height = depth_irb->Base.Base.Height;
   } else if (separate_stencil) {
      /*
       * There exists a separate stencil buffer but no depth buffer.
       *
       * The stencil buffer inherits most of its fields from
       * 3DSTATE_DEPTH_BUFFER: namely the tile walk, surface type, width, and
       * height.
       *
       * The tiled bit must be set. From the Sandybridge PRM, Volume 2, Part 1,
       * Section 7.5.5.1.1 3DSTATE_DEPTH_BUFFER, Bit 1.27 Tiled Surface:
       *     [DevGT+]: This field must be set to TRUE.
       */
      assert(intel->has_separate_stencil);

      depth_surface_type = BRW_SURFACE_2D;
      width = stencil_irb->Base.Base.Width;
      height = stencil_irb->Base.Base.Height;
   }

   intel->vtbl.emit_depth_stencil_hiz(brw, depth_mt, depth_offset,
                                      depthbuffer_format, depth_surface_type,
                                      stencil_mt, hiz_mt, separate_stencil,
                                      width, height, tile_x, tile_y);
}

void
brw_emit_depth_stencil_hiz(struct brw_context *brw,
                           struct intel_mipmap_tree *depth_mt,
                           uint32_t depth_offset, uint32_t depthbuffer_format,
                           uint32_t depth_surface_type,
                           struct intel_mipmap_tree *stencil_mt,
                           struct intel_mipmap_tree *hiz_mt,
                           bool separate_stencil, uint32_t width,
                           uint32_t height, uint32_t tile_x, uint32_t tile_y)
{
   struct intel_context *intel = &brw->intel;

   /* Enable the hiz bit if we're doing separate stencil, because it and the
    * separate stencil bit must have the same value. From Section 2.11.5.6.1.1
    * 3DSTATE_DEPTH_BUFFER, Bit 1.21 "Separate Stencil Enable":
    *     [DevIL]: If this field is enabled, Hierarchical Depth Buffer
    *     Enable must also be enabled.
    *
    *     [DevGT]: This field must be set to the same value (enabled or
    *     disabled) as Hierarchical Depth Buffer Enable
    */
   bool enable_hiz_ss = hiz_mt || separate_stencil;


   /* 3DSTATE_DEPTH_BUFFER, 3DSTATE_STENCIL_BUFFER are both
    * non-pipelined state that will need the PIPE_CONTROL workaround.
    */
   if (intel->gen == 6) {
      intel_emit_post_sync_nonzero_flush(intel);
      intel_emit_depth_stall_flushes(intel);
   }

   unsigned int len;
   if (intel->gen >= 6)
      len = 7;
   else if (intel->is_g4x || intel->gen == 5)
      len = 6;
   else
      len = 5;

   BEGIN_BATCH(len);
   OUT_BATCH(_3DSTATE_DEPTH_BUFFER << 16 | (len - 2));
   OUT_BATCH((depth_mt ? depth_mt->region->pitch - 1 : 0) |
             (depthbuffer_format << 18) |
             ((enable_hiz_ss ? 1 : 0) << 21) | /* separate stencil enable */
             ((enable_hiz_ss ? 1 : 0) << 22) | /* hiz enable */
             (BRW_TILEWALK_YMAJOR << 26) |
             ((depth_mt ? depth_mt->region->tiling != I915_TILING_NONE : 1)
              << 27) |
             (depth_surface_type << 29));

   if (depth_mt) {
      OUT_RELOC(depth_mt->region->bo,
		I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
		depth_offset);
   } else {
      OUT_BATCH(0);
   }

   OUT_BATCH(((width + tile_x - 1) << 6) |
             ((height + tile_y - 1) << 19));
   OUT_BATCH(0);

   if (intel->is_g4x || intel->gen >= 5)
      OUT_BATCH(tile_x | (tile_y << 16));
   else
      assert(tile_x == 0 && tile_y == 0);

   if (intel->gen >= 6)
      OUT_BATCH(0);

   ADVANCE_BATCH();

   if (hiz_mt || separate_stencil) {
      /*
       * In the 3DSTATE_DEPTH_BUFFER batch emitted above, the 'separate
       * stencil enable' and 'hiz enable' bits were set. Therefore we must
       * emit 3DSTATE_HIER_DEPTH_BUFFER and 3DSTATE_STENCIL_BUFFER. Even if
       * there is no stencil buffer, 3DSTATE_STENCIL_BUFFER must be emitted;
       * failure to do so causes hangs on gen5 and a stall on gen6.
       */

      /* Emit hiz buffer. */
      if (hiz_mt) {
	 BEGIN_BATCH(3);
	 OUT_BATCH((_3DSTATE_HIER_DEPTH_BUFFER << 16) | (3 - 2));
	 OUT_BATCH(hiz_mt->region->pitch - 1);
	 OUT_RELOC(hiz_mt->region->bo,
		   I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
		   brw->depthstencil.hiz_offset);
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
	 struct intel_region *region = stencil_mt->region;

	 BEGIN_BATCH(3);
	 OUT_BATCH((_3DSTATE_STENCIL_BUFFER << 16) | (3 - 2));
         /* The stencil buffer has quirky pitch requirements.  From Vol 2a,
          * 11.5.6.2.1 3DSTATE_STENCIL_BUFFER, field "Surface Pitch":
          *    The pitch must be set to 2x the value computed based on width, as
          *    the stencil buffer is stored with two rows interleaved.
          */
	 OUT_BATCH(2 * region->pitch - 1);
	 OUT_RELOC(region->bo,
		   I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
		   brw->depthstencil.stencil_offset);
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
   if (intel->gen >= 6 || hiz_mt) {
      if (intel->gen == 6)
	 intel_emit_post_sync_nonzero_flush(intel);

      BEGIN_BATCH(2);
      OUT_BATCH(_3DSTATE_CLEAR_PARAMS << 16 |
		GEN5_DEPTH_CLEAR_VALID |
		(2 - 2));
      OUT_BATCH(depth_mt ? depth_mt->depth_clear_value : 0);
      ADVANCE_BATCH();
   }
}

const struct brw_tracked_state brw_depthbuffer = {
   .dirty = {
      .mesa = _NEW_BUFFERS,
      .brw = BRW_NEW_BATCH,
      .cache = 0,
   },
   .emit = brw_emit_depthbuffer,
};



/***********************************************************************
 * Polygon stipple packet
 */

static void upload_polygon_stipple(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &brw->intel.ctx;
   GLuint i;

   /* _NEW_POLYGON */
   if (!ctx->Polygon.StippleFlag)
      return;

   if (intel->gen == 6)
      intel_emit_post_sync_nonzero_flush(intel);

   BEGIN_BATCH(33);
   OUT_BATCH(_3DSTATE_POLY_STIPPLE_PATTERN << 16 | (33 - 2));

   /* Polygon stipple is provided in OpenGL order, i.e. bottom
    * row first.  If we're rendering to a window (i.e. the
    * default frame buffer object, 0), then we need to invert
    * it to match our pixel layout.  But if we're rendering
    * to a FBO (i.e. any named frame buffer object), we *don't*
    * need to invert - we already match the layout.
    */
   if (_mesa_is_winsys_fbo(ctx->DrawBuffer)) {
      for (i = 0; i < 32; i++)
	  OUT_BATCH(ctx->PolygonStipple[31 - i]); /* invert */
   }
   else {
      for (i = 0; i < 32; i++)
	 OUT_BATCH(ctx->PolygonStipple[i]);
   }
   CACHED_BATCH();
}

const struct brw_tracked_state brw_polygon_stipple = {
   .dirty = {
      .mesa = (_NEW_POLYGONSTIPPLE |
	       _NEW_POLYGON),
      .brw = BRW_NEW_CONTEXT,
      .cache = 0
   },
   .emit = upload_polygon_stipple
};


/***********************************************************************
 * Polygon stipple offset packet
 */

static void upload_polygon_stipple_offset(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &brw->intel.ctx;

   /* _NEW_POLYGON */
   if (!ctx->Polygon.StippleFlag)
      return;

   if (intel->gen == 6)
      intel_emit_post_sync_nonzero_flush(intel);

   BEGIN_BATCH(2);
   OUT_BATCH(_3DSTATE_POLY_STIPPLE_OFFSET << 16 | (2-2));

   /* _NEW_BUFFERS
    *
    * If we're drawing to a system window we have to invert the Y axis
    * in order to match the OpenGL pixel coordinate system, and our
    * offset must be matched to the window position.  If we're drawing
    * to a user-created FBO then our native pixel coordinate system
    * works just fine, and there's no window system to worry about.
    */
   if (_mesa_is_winsys_fbo(brw->intel.ctx.DrawBuffer))
      OUT_BATCH((32 - (ctx->DrawBuffer->Height & 31)) & 31);
   else
      OUT_BATCH(0);
   CACHED_BATCH();
}

const struct brw_tracked_state brw_polygon_stipple_offset = {
   .dirty = {
      .mesa = (_NEW_BUFFERS |
	       _NEW_POLYGON),
      .brw = BRW_NEW_CONTEXT,
      .cache = 0
   },
   .emit = upload_polygon_stipple_offset
};

/**********************************************************************
 * AA Line parameters
 */
static void upload_aa_line_parameters(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &brw->intel.ctx;

   if (!ctx->Line.SmoothFlag || !brw->has_aa_line_parameters)
      return;

   if (intel->gen == 6)
      intel_emit_post_sync_nonzero_flush(intel);

   OUT_BATCH(_3DSTATE_AA_LINE_PARAMETERS << 16 | (3 - 2));
   /* use legacy aa line coverage computation */
   OUT_BATCH(0);
   OUT_BATCH(0);
   CACHED_BATCH();
}

const struct brw_tracked_state brw_aa_line_parameters = {
   .dirty = {
      .mesa = _NEW_LINE,
      .brw = BRW_NEW_CONTEXT,
      .cache = 0
   },
   .emit = upload_aa_line_parameters
};

/***********************************************************************
 * Line stipple packet
 */

static void upload_line_stipple(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &brw->intel.ctx;
   GLfloat tmp;
   GLint tmpi;

   if (!ctx->Line.StippleFlag)
      return;

   if (intel->gen == 6)
      intel_emit_post_sync_nonzero_flush(intel);

   BEGIN_BATCH(3);
   OUT_BATCH(_3DSTATE_LINE_STIPPLE_PATTERN << 16 | (3 - 2));
   OUT_BATCH(ctx->Line.StipplePattern);
   tmp = 1.0 / (GLfloat) ctx->Line.StippleFactor;
   tmpi = tmp * (1<<13);
   OUT_BATCH(tmpi << 16 | ctx->Line.StippleFactor);
   CACHED_BATCH();
}

const struct brw_tracked_state brw_line_stipple = {
   .dirty = {
      .mesa = _NEW_LINE,
      .brw = BRW_NEW_CONTEXT,
      .cache = 0
   },
   .emit = upload_line_stipple
};


/***********************************************************************
 * Misc invariant state packets
 */

static void upload_invariant_state( struct brw_context *brw )
{
   struct intel_context *intel = &brw->intel;

   /* 3DSTATE_SIP, 3DSTATE_MULTISAMPLE, etc. are nonpipelined. */
   if (intel->gen == 6)
      intel_emit_post_sync_nonzero_flush(intel);

   /* Select the 3D pipeline (as opposed to media) */
   BEGIN_BATCH(1);
   OUT_BATCH(brw->CMD_PIPELINE_SELECT << 16 | 0);
   ADVANCE_BATCH();

   if (intel->gen < 6) {
      /* Disable depth offset clamping. */
      BEGIN_BATCH(2);
      OUT_BATCH(_3DSTATE_GLOBAL_DEPTH_OFFSET_CLAMP << 16 | (2 - 2));
      OUT_BATCH_F(0.0);
      ADVANCE_BATCH();
   }

   if (intel->gen == 6) {
      int i;

      for (i = 0; i < 4; i++) {
         BEGIN_BATCH(4);
         OUT_BATCH(_3DSTATE_GS_SVB_INDEX << 16 | (4 - 2));
         OUT_BATCH(i << SVB_INDEX_SHIFT);
         OUT_BATCH(0);
         OUT_BATCH(0xffffffff);
         ADVANCE_BATCH();
      }
   }

   BEGIN_BATCH(2);
   OUT_BATCH(CMD_STATE_SIP << 16 | (2 - 2));
   OUT_BATCH(0);
   ADVANCE_BATCH();

   BEGIN_BATCH(1);
   OUT_BATCH(brw->CMD_VF_STATISTICS << 16 |
	     (unlikely(INTEL_DEBUG & DEBUG_STATS) ? 1 : 0));
   ADVANCE_BATCH();
}

const struct brw_tracked_state brw_invariant_state = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_CONTEXT,
      .cache = 0
   },
   .emit = upload_invariant_state
};

/**
 * Define the base addresses which some state is referenced from.
 *
 * This allows us to avoid having to emit relocations for the objects,
 * and is actually required for binding table pointers on gen6.
 *
 * Surface state base address covers binding table pointers and
 * surface state objects, but not the surfaces that the surface state
 * objects point to.
 */
static void upload_state_base_address( struct brw_context *brw )
{
   struct intel_context *intel = &brw->intel;

   /* FINISHME: According to section 3.6.1 "STATE_BASE_ADDRESS" of
    * vol1a of the G45 PRM, MI_FLUSH with the ISC invalidate should be
    * programmed prior to STATE_BASE_ADDRESS.
    *
    * However, given that the instruction SBA (general state base
    * address) on this chipset is always set to 0 across X and GL,
    * maybe this isn't required for us in particular.
    */

   if (intel->gen >= 6) {
      if (intel->gen == 6)
	 intel_emit_post_sync_nonzero_flush(intel);

       BEGIN_BATCH(10);
       OUT_BATCH(CMD_STATE_BASE_ADDRESS << 16 | (10 - 2));
       /* General state base address: stateless DP read/write requests */
       OUT_BATCH(1);
       /* Surface state base address:
	* BINDING_TABLE_STATE
	* SURFACE_STATE
	*/
       OUT_RELOC(intel->batch.bo, I915_GEM_DOMAIN_SAMPLER, 0, 1);
        /* Dynamic state base address:
	 * SAMPLER_STATE
	 * SAMPLER_BORDER_COLOR_STATE
	 * CLIP, SF, WM/CC viewport state
	 * COLOR_CALC_STATE
	 * DEPTH_STENCIL_STATE
	 * BLEND_STATE
	 * Push constants (when INSTPM: CONSTANT_BUFFER Address Offset
	 * Disable is clear, which we rely on)
	 */
       OUT_RELOC(intel->batch.bo, (I915_GEM_DOMAIN_RENDER |
				   I915_GEM_DOMAIN_INSTRUCTION), 0, 1);

       OUT_BATCH(1); /* Indirect object base address: MEDIA_OBJECT data */
       OUT_RELOC(brw->cache.bo, I915_GEM_DOMAIN_INSTRUCTION, 0,
		 1); /* Instruction base address: shader kernels (incl. SIP) */

       OUT_BATCH(1); /* General state upper bound */
       /* Dynamic state upper bound.  Although the documentation says that
	* programming it to zero will cause it to be ignored, that is a lie.
	* If this isn't programmed to a real bound, the sampler border color
	* pointer is rejected, causing border color to mysteriously fail.
	*/
       OUT_BATCH(0xfffff001);
       OUT_BATCH(1); /* Indirect object upper bound */
       OUT_BATCH(1); /* Instruction access upper bound */
       ADVANCE_BATCH();
   } else if (intel->gen == 5) {
       BEGIN_BATCH(8);
       OUT_BATCH(CMD_STATE_BASE_ADDRESS << 16 | (8 - 2));
       OUT_BATCH(1); /* General state base address */
       OUT_RELOC(intel->batch.bo, I915_GEM_DOMAIN_SAMPLER, 0,
		 1); /* Surface state base address */
       OUT_BATCH(1); /* Indirect object base address */
       OUT_RELOC(brw->cache.bo, I915_GEM_DOMAIN_INSTRUCTION, 0,
		 1); /* Instruction base address */
       OUT_BATCH(0xfffff001); /* General state upper bound */
       OUT_BATCH(1); /* Indirect object upper bound */
       OUT_BATCH(1); /* Instruction access upper bound */
       ADVANCE_BATCH();
   } else {
       BEGIN_BATCH(6);
       OUT_BATCH(CMD_STATE_BASE_ADDRESS << 16 | (6 - 2));
       OUT_BATCH(1); /* General state base address */
       OUT_RELOC(intel->batch.bo, I915_GEM_DOMAIN_SAMPLER, 0,
		 1); /* Surface state base address */
       OUT_BATCH(1); /* Indirect object base address */
       OUT_BATCH(1); /* General state upper bound */
       OUT_BATCH(1); /* Indirect object upper bound */
       ADVANCE_BATCH();
   }

   /* According to section 3.6.1 of VOL1 of the 965 PRM,
    * STATE_BASE_ADDRESS updates require a reissue of:
    *
    * 3DSTATE_PIPELINE_POINTERS
    * 3DSTATE_BINDING_TABLE_POINTERS
    * MEDIA_STATE_POINTERS
    *
    * and this continues through Ironlake.  The Sandy Bridge PRM, vol
    * 1 part 1 says that the folowing packets must be reissued:
    *
    * 3DSTATE_CC_POINTERS
    * 3DSTATE_BINDING_TABLE_POINTERS
    * 3DSTATE_SAMPLER_STATE_POINTERS
    * 3DSTATE_VIEWPORT_STATE_POINTERS
    * MEDIA_STATE_POINTERS
    *
    * Those are always reissued following SBA updates anyway (new
    * batch time), except in the case of the program cache BO
    * changing.  Having a separate state flag makes the sequence more
    * obvious.
    */

   brw->state.dirty.brw |= BRW_NEW_STATE_BASE_ADDRESS;
}

const struct brw_tracked_state brw_state_base_address = {
   .dirty = {
      .mesa = 0,
      .brw = (BRW_NEW_BATCH |
	      BRW_NEW_PROGRAM_CACHE),
      .cache = 0,
   },
   .emit = upload_state_base_address
};
