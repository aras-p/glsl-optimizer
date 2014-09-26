/*
 * Copyright © 2012 Intel Corporation
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

#include <errno.h>
#include "intel_batchbuffer.h"
#include "intel_fbo.h"

#include "brw_blorp.h"
#include "brw_defines.h"
#include "brw_state.h"
#include "gen6_blorp.h"
#include "gen7_blorp.h"

#define FILE_DEBUG_FLAG DEBUG_BLORP

brw_blorp_mip_info::brw_blorp_mip_info()
   : mt(NULL),
     level(0),
     layer(0),
     width(0),
     height(0),
     x_offset(0),
     y_offset(0)
{
}

brw_blorp_surface_info::brw_blorp_surface_info()
   : map_stencil_as_y_tiled(false),
     num_samples(0)
{
}

void
brw_blorp_mip_info::set(struct intel_mipmap_tree *mt,
                        unsigned int level, unsigned int layer)
{
   /* Layer is a physical layer, so if this is a 2D multisample array texture
    * using INTEL_MSAA_LAYOUT_UMS or INTEL_MSAA_LAYOUT_CMS, then it had better
    * be a multiple of num_samples.
    */
   if (mt->msaa_layout == INTEL_MSAA_LAYOUT_UMS ||
       mt->msaa_layout == INTEL_MSAA_LAYOUT_CMS) {
      assert(layer % mt->num_samples == 0);
   }

   intel_miptree_check_level_layer(mt, level, layer);

   this->mt = mt;
   this->level = level;
   this->layer = layer;
   this->width = minify(mt->physical_width0, level - mt->first_level);
   this->height = minify(mt->physical_height0, level - mt->first_level);

   intel_miptree_get_image_offset(mt, level, layer, &x_offset, &y_offset);
}

void
brw_blorp_surface_info::set(struct brw_context *brw,
                            struct intel_mipmap_tree *mt,
                            unsigned int level, unsigned int layer,
                            mesa_format format, bool is_render_target)
{
   brw_blorp_mip_info::set(mt, level, layer);
   this->num_samples = mt->num_samples;
   this->array_layout = mt->array_layout;
   this->map_stencil_as_y_tiled = false;
   this->msaa_layout = mt->msaa_layout;

   if (format == MESA_FORMAT_NONE)
      format = mt->format;

   switch (format) {
   case MESA_FORMAT_S_UINT8:
      /* The miptree is a W-tiled stencil buffer.  Surface states can't be set
       * up for W tiling, so we'll need to use Y tiling and have the WM
       * program swizzle the coordinates.
       */
      this->map_stencil_as_y_tiled = true;
      this->brw_surfaceformat = BRW_SURFACEFORMAT_R8_UNORM;
      break;
   case MESA_FORMAT_Z24_UNORM_X8_UINT:
      /* It would make sense to use BRW_SURFACEFORMAT_R24_UNORM_X8_TYPELESS
       * here, but unfortunately it isn't supported as a render target, which
       * would prevent us from blitting to 24-bit depth.
       *
       * The miptree consists of 32 bits per pixel, arranged as 24-bit depth
       * values interleaved with 8 "don't care" bits.  Since depth values don't
       * require any blending, it doesn't matter how we interpret the bit
       * pattern as long as we copy the right amount of data, so just map it
       * as 8-bit BGRA.
       */
      this->brw_surfaceformat = BRW_SURFACEFORMAT_B8G8R8A8_UNORM;
      break;
   case MESA_FORMAT_Z_FLOAT32:
      this->brw_surfaceformat = BRW_SURFACEFORMAT_R32_FLOAT;
      break;
   case MESA_FORMAT_Z_UNORM16:
      this->brw_surfaceformat = BRW_SURFACEFORMAT_R16_UNORM;
      break;
   default: {
      mesa_format linear_format = _mesa_get_srgb_format_linear(format);
      if (is_render_target) {
         assert(brw->format_supported_as_render_target[linear_format]);
         this->brw_surfaceformat = brw->render_target_format[linear_format];
      } else {
         this->brw_surfaceformat = brw_format_for_mesa_format(linear_format);
      }
      break;
   }
   }
}


/**
 * Split x_offset and y_offset into a base offset (in bytes) and a remaining
 * x/y offset (in pixels).  Note: we can't do this by calling
 * intel_renderbuffer_tile_offsets(), because the offsets may have been
 * adjusted to account for Y vs. W tiling differences.  So we compute it
 * directly from the adjusted offsets.
 */
uint32_t
brw_blorp_surface_info::compute_tile_offsets(uint32_t *tile_x,
                                             uint32_t *tile_y) const
{
   uint32_t mask_x, mask_y;

   intel_miptree_get_tile_masks(mt, &mask_x, &mask_y, map_stencil_as_y_tiled);

   *tile_x = x_offset & mask_x;
   *tile_y = y_offset & mask_y;

   return intel_miptree_get_aligned_offset(mt, x_offset & ~mask_x,
                                           y_offset & ~mask_y,
                                           map_stencil_as_y_tiled);
}


brw_blorp_params::brw_blorp_params()
   : x0(0),
     y0(0),
     x1(0),
     y1(0),
     depth_format(0),
     hiz_op(GEN6_HIZ_OP_NONE),
     fast_clear_op(GEN7_FAST_CLEAR_OP_NONE),
     use_wm_prog(false)
{
   color_write_disable[0] = false;
   color_write_disable[1] = false;
   color_write_disable[2] = false;
   color_write_disable[3] = false;
}

extern "C" {
void
intel_hiz_exec(struct brw_context *brw, struct intel_mipmap_tree *mt,
	       unsigned int level, unsigned int layer, gen6_hiz_op op)
{
   const char *opname = NULL;

   switch (op) {
   case GEN6_HIZ_OP_DEPTH_RESOLVE:
      opname = "depth resolve";
      break;
   case GEN6_HIZ_OP_HIZ_RESOLVE:
      opname = "hiz ambiguate";
      break;
   case GEN6_HIZ_OP_DEPTH_CLEAR:
      opname = "depth clear";
      break;
   case GEN6_HIZ_OP_NONE:
      opname = "noop?";
      break;
   }

   DBG("%s %s to mt %p level %d layer %d\n",
       __FUNCTION__, opname, mt, level, layer);

   if (brw->gen >= 8) {
      gen8_hiz_exec(brw, mt, level, layer, op);
   } else {
      brw_hiz_op_params params(mt, level, layer, op);
      brw_blorp_exec(brw, &params);
   }
}

} /* extern "C" */

void
brw_blorp_exec(struct brw_context *brw, const brw_blorp_params *params)
{
   struct gl_context *ctx = &brw->ctx;
   uint32_t estimated_max_batch_usage = 1500;
   bool check_aperture_failed_once = false;

   /* Flush the sampler and render caches.  We definitely need to flush the
    * sampler cache so that we get updated contents from the render cache for
    * the glBlitFramebuffer() source.  Also, we are sometimes warned in the
    * docs to flush the cache between reinterpretations of the same surface
    * data with different formats, which blorp does for stencil and depth
    * data.
    */
   intel_batchbuffer_emit_mi_flush(brw);

retry:
   intel_batchbuffer_require_space(brw, estimated_max_batch_usage, RENDER_RING);
   intel_batchbuffer_save_state(brw);
   drm_intel_bo *saved_bo = brw->batch.bo;
   uint32_t saved_used = brw->batch.used;
   uint32_t saved_state_batch_offset = brw->batch.state_batch_offset;

   switch (brw->gen) {
   case 6:
      gen6_blorp_exec(brw, params);
      break;
   case 7:
      gen7_blorp_exec(brw, params);
      break;
   default:
      /* BLORP is not supported before Gen6. */
      unreachable("not reached");
   }

   /* Make sure we didn't wrap the batch unintentionally, and make sure we
    * reserved enough space that a wrap will never happen.
    */
   assert(brw->batch.bo == saved_bo);
   assert((brw->batch.used - saved_used) * 4 +
          (saved_state_batch_offset - brw->batch.state_batch_offset) <
          estimated_max_batch_usage);
   /* Shut up compiler warnings on release build */
   (void)saved_bo;
   (void)saved_used;
   (void)saved_state_batch_offset;

   /* Check if the blorp op we just did would make our batch likely to fail to
    * map all the BOs into the GPU at batch exec time later.  If so, flush the
    * batch and try again with nothing else in the batch.
    */
   if (dri_bufmgr_check_aperture_space(&brw->batch.bo, 1)) {
      if (!check_aperture_failed_once) {
         check_aperture_failed_once = true;
         intel_batchbuffer_reset_to_saved(brw);
         intel_batchbuffer_flush(brw);
         goto retry;
      } else {
         int ret = intel_batchbuffer_flush(brw);
         WARN_ONCE(ret == -ENOSPC,
                   "i965: blorp emit exceeded available aperture space\n");
      }
   }

   if (unlikely(brw->always_flush_batch))
      intel_batchbuffer_flush(brw);

   /* We've smashed all state compared to what the normal 3D pipeline
    * rendering tracks for GL.
    */
   brw->state.dirty.brw = ~0ull;
   brw->state.dirty.cache = ~0;
   brw->no_depth_or_stencil = false;
   brw->ib.type = -1;

   /* Flush the sampler cache so any texturing from the destination is
    * coherent.
    */
   intel_batchbuffer_emit_mi_flush(brw);
}

brw_hiz_op_params::brw_hiz_op_params(struct intel_mipmap_tree *mt,
                                     unsigned int level,
                                     unsigned int layer,
                                     gen6_hiz_op op)
{
   this->hiz_op = op;

   depth.set(mt, level, layer);

   /* Align the rectangle primitive to 8x4 pixels.
    *
    * During fast depth clears, the emitted rectangle primitive  must be
    * aligned to 8x4 pixels.  From the Ivybridge PRM, Vol 2 Part 1 Section
    * 11.5.3.1 Depth Buffer Clear (and the matching section in the Sandybridge
    * PRM):
    *     If Number of Multisamples is NUMSAMPLES_1, the rectangle must be
    *     aligned to an 8x4 pixel block relative to the upper left corner
    *     of the depth buffer [...]
    *
    * For hiz resolves, the rectangle must also be 8x4 aligned. Item
    * WaHizAmbiguate8x4Aligned from the Haswell workarounds page and the
    * Ivybridge simulator require the alignment.
    *
    * To be safe, let's just align the rect for all hiz operations and all
    * hardware generations.
    *
    * However, for some miptree slices of a Z24 texture, emitting an 8x4
    * aligned rectangle that covers the slice may clobber adjacent slices if
    * we strictly adhered to the texture alignments specified in the PRM.  The
    * Ivybridge PRM, Section "Alignment Unit Size", states that
    * SURFACE_STATE.Surface_Horizontal_Alignment should be 4 for Z24 surfaces,
    * not 8. But commit 1f112cc increased the alignment from 4 to 8, which
    * prevents the clobbering.
    */
   depth.width = ALIGN(depth.width, 8);
   depth.height = ALIGN(depth.height, 4);

   x1 = depth.width;
   y1 = depth.height;

   assert(intel_miptree_level_has_hiz(mt, level));

   switch (mt->format) {
   case MESA_FORMAT_Z_UNORM16:       depth_format = BRW_DEPTHFORMAT_D16_UNORM; break;
   case MESA_FORMAT_Z_FLOAT32: depth_format = BRW_DEPTHFORMAT_D32_FLOAT; break;
   case MESA_FORMAT_Z24_UNORM_X8_UINT:    depth_format = BRW_DEPTHFORMAT_D24_UNORM_X8_UINT; break;
   default:                    unreachable("not reached");
   }
}

uint32_t
brw_hiz_op_params::get_wm_prog(struct brw_context *brw,
                               brw_blorp_prog_data **prog_data) const
{
   return 0;
}
