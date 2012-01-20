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
#include "main/mtypes.h"
#include "main/samplerobj.h"
#include "program/prog_parameter.h"

#include "intel_mipmap_tree.h"
#include "intel_batchbuffer.h"
#include "intel_tex.h"
#include "intel_fbo.h"

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "brw_wm.h"

static void
gen7_set_surface_tiling(struct gen7_surface_state *surf, uint32_t tiling)
{
   switch (tiling) {
   case I915_TILING_NONE:
      surf->ss0.tiled_surface = 0;
      surf->ss0.tile_walk = 0;
      break;
   case I915_TILING_X:
      surf->ss0.tiled_surface = 1;
      surf->ss0.tile_walk = BRW_TILEWALK_XMAJOR;
      break;
   case I915_TILING_Y:
      surf->ss0.tiled_surface = 1;
      surf->ss0.tile_walk = BRW_TILEWALK_YMAJOR;
      break;
   }
}

static void
gen7_update_texture_surface(struct gl_context *ctx, GLuint unit)
{
   struct brw_context *brw = brw_context(ctx);
   struct gl_texture_object *tObj = ctx->Texture.Unit[unit]._Current;
   struct intel_texture_object *intelObj = intel_texture_object(tObj);
   struct intel_mipmap_tree *mt = intelObj->mt;
   struct gl_texture_image *firstImage = tObj->Image[0][tObj->BaseLevel];
   struct gl_sampler_object *sampler = _mesa_get_samplerobj(ctx, unit);
   const GLuint surf_index = SURF_INDEX_TEXTURE(unit);
   struct gen7_surface_state *surf;
   int width, height, depth;

   intel_miptree_get_dimensions_for_image(firstImage, &width, &height, &depth);

   surf = brw_state_batch(brw, AUB_TRACE_SURFACE_STATE,
			  sizeof(*surf), 32, &brw->bind.surf_offset[surf_index]);
   memset(surf, 0, sizeof(*surf));

   if (mt->align_h == 4)
      surf->ss0.vertical_alignment = 1;
   if (mt->align_w == 8)
      surf->ss0.horizontal_alignment = 1;

   surf->ss0.surface_type = translate_tex_target(tObj->Target);
   surf->ss0.surface_format = translate_tex_format(mt->format,
                                                   firstImage->InternalFormat,
                                                   sampler->DepthMode,
                                                   sampler->sRGBDecode);
   if (tObj->Target == GL_TEXTURE_CUBE_MAP) {
      surf->ss0.cube_pos_x = 1;
      surf->ss0.cube_pos_y = 1;
      surf->ss0.cube_pos_z = 1;
      surf->ss0.cube_neg_x = 1;
      surf->ss0.cube_neg_y = 1;
      surf->ss0.cube_neg_z = 1;
   }

   surf->ss0.is_array = depth > 1 && tObj->Target != GL_TEXTURE_3D;

   gen7_set_surface_tiling(surf, intelObj->mt->region->tiling);

   /* ss0 remaining fields:
    * - vert_line_stride (exists on gen6 but we ignore it)
    * - vert_line_stride_ofs (exists on gen6 but we ignore it)
    * - surface_array_spacing
    * - render_cache_read_write (exists on gen6 but ignored here)
    */

   surf->ss1.base_addr = intelObj->mt->region->bo->offset; /* reloc */

   surf->ss2.width = width - 1;
   surf->ss2.height = height - 1;

   surf->ss3.pitch = (intelObj->mt->region->pitch * intelObj->mt->cpp) - 1;
   surf->ss3.depth = depth - 1;

   /* ss4: ignored? */

   surf->ss5.mip_count = intelObj->_MaxLevel - tObj->BaseLevel;
   surf->ss5.min_lod = 0;

   /* ss5 remaining fields:
    * - x_offset (N/A for textures?)
    * - y_offset (ditto)
    * - cache_control
    */

   /* Emit relocation to surface contents */
   drm_intel_bo_emit_reloc(brw->intel.batch.bo,
			   brw->bind.surf_offset[surf_index] +
			   offsetof(struct gen7_surface_state, ss1),
			   intelObj->mt->region->bo, 0,
			   I915_GEM_DOMAIN_SAMPLER, 0);
}

/**
 * Create the constant buffer surface.  Vertex/fragment shader constants will
 * be read from this buffer with Data Port Read instructions/messages.
 */
void
gen7_create_constant_surface(struct brw_context *brw,
			     drm_intel_bo *bo,
			     int width,
			     uint32_t *out_offset)
{
   const GLint w = width - 1;
   struct gen7_surface_state *surf;

   surf = brw_state_batch(brw, AUB_TRACE_SURFACE_STATE,
			  sizeof(*surf), 32, out_offset);
   memset(surf, 0, sizeof(*surf));

   surf->ss0.surface_type = BRW_SURFACE_BUFFER;
   surf->ss0.surface_format = BRW_SURFACEFORMAT_R32G32B32A32_FLOAT;

   surf->ss0.render_cache_read_write = 1;

   assert(bo);
   surf->ss1.base_addr = bo->offset; /* reloc */

   surf->ss2.width = w & 0x7f;            /* bits 6:0 of size or width */
   surf->ss2.height = (w >> 7) & 0x1fff;  /* bits 19:7 of size or width */
   surf->ss3.depth = (w >> 20) & 0x7f;    /* bits 26:20 of size or width */
   surf->ss3.pitch = (16 - 1); /* ignored */
   gen7_set_surface_tiling(surf, I915_TILING_NONE); /* tiling now allowed */

   /* Emit relocation to surface contents.  Section 5.1.1 of the gen4
    * bspec ("Data Cache") says that the data cache does not exist as
    * a separate cache and is just the sampler cache.
    */
   drm_intel_bo_emit_reloc(brw->intel.batch.bo,
			   (*out_offset +
			    offsetof(struct gen7_surface_state, ss1)),
			   bo, 0,
			   I915_GEM_DOMAIN_SAMPLER, 0);
}

static void
gen7_update_null_renderbuffer_surface(struct brw_context *brw, unsigned unit)
{
   struct gen7_surface_state *surf;

   surf = brw_state_batch(brw, AUB_TRACE_SURFACE_STATE,
			  sizeof(*surf), 32, &brw->bind.surf_offset[unit]);
   memset(surf, 0, sizeof(*surf));

   surf->ss0.surface_type = BRW_SURFACE_NULL;
   surf->ss0.surface_format = BRW_SURFACEFORMAT_B8G8R8A8_UNORM;
}

/**
 * Sets up a surface state structure to point at the given region.
 * While it is only used for the front/back buffer currently, it should be
 * usable for further buffers when doing ARB_draw_buffer support.
 */
static void
gen7_update_renderbuffer_surface(struct brw_context *brw,
				 struct gl_renderbuffer *rb,
				 unsigned int unit)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);
   struct intel_region *region = irb->mt->region;
   struct gen7_surface_state *surf;
   uint32_t tile_x, tile_y;
   gl_format rb_format = intel_rb_format(irb);

   surf = brw_state_batch(brw, AUB_TRACE_SURFACE_STATE,
			  sizeof(*surf), 32, &brw->bind.surf_offset[unit]);
   memset(surf, 0, sizeof(*surf));

   if (irb->mt->align_h == 4)
      surf->ss0.vertical_alignment = 1;
   if (irb->mt->align_w == 8)
      surf->ss0.horizontal_alignment = 1;

   switch (rb_format) {
   case MESA_FORMAT_SARGB8:
      /* without GL_EXT_framebuffer_sRGB we shouldn't bind sRGB
	 surfaces to the blend/update as sRGB */
      if (ctx->Color.sRGBEnabled)
	 surf->ss0.surface_format = brw_format_for_mesa_format(rb_format);
      else
	 surf->ss0.surface_format = BRW_SURFACEFORMAT_B8G8R8A8_UNORM;
      break;
   default:
      assert(brw_render_target_supported(intel, rb));
      surf->ss0.surface_format = brw->render_target_format[rb_format];
      if (unlikely(!brw->format_supported_as_render_target[rb_format])) {
	 _mesa_problem(ctx, "%s: renderbuffer format %s unsupported\n",
		       __FUNCTION__, _mesa_get_format_name(rb_format));
      }
       break;
   }

   surf->ss0.surface_type = BRW_SURFACE_2D;
   /* reloc */
   surf->ss1.base_addr = intel_renderbuffer_tile_offsets(irb, &tile_x, &tile_y);
   surf->ss1.base_addr += region->bo->offset; /* reloc */

   assert(brw->has_surface_tile_offset);
   /* Note that the low bits of these fields are missing, so
    * there's the possibility of getting in trouble.
    */
   assert(tile_x % 4 == 0);
   assert(tile_y % 2 == 0);
   surf->ss5.x_offset = tile_x / 4;
   surf->ss5.y_offset = tile_y / 2;

   surf->ss2.width = rb->Width - 1;
   surf->ss2.height = rb->Height - 1;
   gen7_set_surface_tiling(surf, region->tiling);
   surf->ss3.pitch = (region->pitch * region->cpp) - 1;

   drm_intel_bo_emit_reloc(brw->intel.batch.bo,
			   brw->bind.surf_offset[unit] +
			   offsetof(struct gen7_surface_state, ss1),
			   region->bo,
			   surf->ss1.base_addr - region->bo->offset,
			   I915_GEM_DOMAIN_RENDER,
			   I915_GEM_DOMAIN_RENDER);
}

void
gen7_init_vtable_surface_functions(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;

   intel->vtbl.update_texture_surface = gen7_update_texture_surface;
   intel->vtbl.update_renderbuffer_surface = gen7_update_renderbuffer_surface;
   intel->vtbl.update_null_renderbuffer_surface =
      gen7_update_null_renderbuffer_surface;
   intel->vtbl.create_constant_surface = gen7_create_constant_surface;
}
