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


#include "main/context.h"
#include "main/blend.h"
#include "main/mtypes.h"
#include "main/samplerobj.h"
#include "program/prog_parameter.h"

#include "intel_mipmap_tree.h"
#include "intel_batchbuffer.h"
#include "intel_tex.h"
#include "intel_fbo.h"
#include "intel_buffer_objects.h"

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "brw_wm.h"

/**
 * Sets up a surface state structure to point at the given region.
 * While it is only used for the front/back buffer currently, it should be
 * usable for further buffers when doing ARB_draw_buffer support.
 */
static void
gen6_update_renderbuffer_surface(struct brw_context *brw,
                                 struct gl_renderbuffer *rb,
                                 bool layered,
                                 unsigned int unit)
{
   struct gl_context *ctx = &brw->ctx;
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);
   struct intel_mipmap_tree *mt = irb->mt;
   uint32_t *surf;
   uint32_t tile_x, tile_y;
   uint32_t format = 0;
   /* _NEW_BUFFERS */
   mesa_format rb_format = _mesa_get_render_format(ctx, intel_rb_format(irb));
   uint32_t surf_index =
      brw->wm.prog_data->binding_table.render_target_start + unit;

   assert(!layered);

   if (rb->TexImage && !brw->has_surface_tile_offset) {
      intel_renderbuffer_get_tile_offsets(irb, &tile_x, &tile_y);

      if (tile_x != 0 || tile_y != 0) {
	 /* Original gen4 hardware couldn't draw to a non-tile-aligned
	  * destination in a miptree unless you actually setup your renderbuffer
	  * as a miptree and used the fragile lod/array_index/etc. controls to
	  * select the image.  So, instead, we just make a new single-level
	  * miptree and render into that.
	  */
	 intel_renderbuffer_move_to_temp(brw, irb, false);
	 mt = irb->mt;
      }
   }

   intel_miptree_used_for_rendering(irb->mt);

   surf = brw_state_batch(brw, AUB_TRACE_SURFACE_STATE, 6 * 4, 32,
                          &brw->wm.base.surf_offset[surf_index]);

   format = brw->render_target_format[rb_format];
   if (unlikely(!brw->format_supported_as_render_target[rb_format])) {
      _mesa_problem(ctx, "%s: renderbuffer format %s unsupported\n",
                    __FUNCTION__, _mesa_get_format_name(rb_format));
   }

   surf[0] = (BRW_SURFACE_2D << BRW_SURFACE_TYPE_SHIFT |
	      format << BRW_SURFACE_FORMAT_SHIFT);

   /* reloc */
   surf[1] = (intel_renderbuffer_get_tile_offsets(irb, &tile_x, &tile_y) +
	      mt->bo->offset64);

   surf[2] = ((rb->Width - 1) << BRW_SURFACE_WIDTH_SHIFT |
	      (rb->Height - 1) << BRW_SURFACE_HEIGHT_SHIFT);

   surf[3] = (brw_get_surface_tiling_bits(mt->tiling) |
	      (mt->pitch - 1) << BRW_SURFACE_PITCH_SHIFT);

   surf[4] = brw_get_surface_num_multisamples(mt->num_samples);

   assert(brw->has_surface_tile_offset || (tile_x == 0 && tile_y == 0));
   /* Note that the low bits of these fields are missing, so
    * there's the possibility of getting in trouble.
    */
   assert(tile_x % 4 == 0);
   assert(tile_y % 2 == 0);
   surf[5] = ((tile_x / 4) << BRW_SURFACE_X_OFFSET_SHIFT |
	      (tile_y / 2) << BRW_SURFACE_Y_OFFSET_SHIFT |
	      (mt->align_h == 4 ? BRW_SURFACE_VERTICAL_ALIGN_ENABLE : 0));

   drm_intel_bo_emit_reloc(brw->batch.bo,
			   brw->wm.base.surf_offset[surf_index] + 4,
			   mt->bo,
			   surf[1] - mt->bo->offset64,
			   I915_GEM_DOMAIN_RENDER,
			   I915_GEM_DOMAIN_RENDER);
}

void
gen6_init_vtable_surface_functions(struct brw_context *brw)
{
   gen4_init_vtable_surface_functions(brw);
   brw->vtbl.update_renderbuffer_surface = gen6_update_renderbuffer_surface;
}
