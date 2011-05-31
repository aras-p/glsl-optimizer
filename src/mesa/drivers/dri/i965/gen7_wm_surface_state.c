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
#include "main/texstore.h"
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
   struct gl_texture_image *firstImage = tObj->Image[0][tObj->BaseLevel];
   struct gl_sampler_object *sampler = _mesa_get_samplerobj(ctx, unit);
   const GLuint surf_index = SURF_INDEX_TEXTURE(unit);
   struct gen7_surface_state *surf;

   surf = brw_state_batch(brw, sizeof(*surf), 32,
			 &brw->wm.surf_offset[surf_index]);
   memset(surf, 0, sizeof(*surf));

   surf->ss0.surface_type = translate_tex_target(tObj->Target);
   surf->ss0.surface_format = translate_tex_format(firstImage->TexFormat,
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

   gen7_set_surface_tiling(surf, intelObj->mt->region->tiling);

   /* ss0 remaining fields:
    * - is_array
    * - vertical_alignment
    * - horizontal_alignment
    * - vert_line_stride (exists on gen6 but we ignore it)
    * - vert_line_stride_ofs (exists on gen6 but we ignore it)
    * - surface_array_spacing
    * - render_cache_read_write (exists on gen6 but ignored here)
    */

   surf->ss1.base_addr = intelObj->mt->region->buffer->offset; /* reloc */

   surf->ss2.width = firstImage->Width - 1;
   surf->ss2.height = firstImage->Height - 1;

   surf->ss3.pitch = (intelObj->mt->region->pitch * intelObj->mt->cpp) - 1;
   surf->ss3.depth = firstImage->Depth - 1;

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
			   brw->wm.surf_offset[surf_index] +
			   offsetof(struct gen7_surface_state, ss1),
			   intelObj->mt->region->buffer, 0,
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

   surf = brw_state_batch(brw, sizeof(*surf), 32, out_offset);
   memset(surf, 0, sizeof(*surf));

   surf->ss0.surface_type = BRW_SURFACE_BUFFER;
   surf->ss0.surface_format = BRW_SURFACEFORMAT_R32G32B32A32_FLOAT;

   surf->ss0.render_cache_read_write = 1;

   assert(bo);
   surf->ss1.base_addr = bo->offset; /* reloc */

   surf->ss2.width = w & 0x7f;            /* bits 6:0 of size or width */
   surf->ss2.height = (w >> 7) & 0x1fff;  /* bits 19:7 of size or width */
   surf->ss3.depth = (w >> 20) & 0x7f;    /* bits 26:20 of size or width */
   surf->ss3.pitch = (width * 16) - 1; /* ignored?? */
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

/**
 * Updates surface / buffer for fragment shader constant buffer, if
 * one is required.
 *
 * This consumes the state updates for the constant buffer, and produces
 * BRW_NEW_WM_SURFACES to get picked up by brw_prepare_wm_surfaces for
 * inclusion in the binding table.
 */
static void upload_wm_constant_surface(struct brw_context *brw)
{
   GLuint surf = SURF_INDEX_FRAG_CONST_BUFFER;
   struct brw_fragment_program *fp =
      (struct brw_fragment_program *) brw->fragment_program;
   const struct gl_program_parameter_list *params =
      fp->program.Base.Parameters;

   /* If there's no constant buffer, then no surface BO is needed to point at
    * it.
    */
   if (brw->wm.const_bo == 0) {
      if (brw->wm.surf_offset[surf]) {
	 brw->state.dirty.brw |= BRW_NEW_WM_SURFACES;
	 brw->wm.surf_offset[surf] = 0;
      }
      return;
   }

   gen7_create_constant_surface(brw, brw->wm.const_bo, params->NumParameters,
			        &brw->wm.surf_offset[surf]);
   brw->state.dirty.brw |= BRW_NEW_WM_SURFACES;
}

const struct brw_tracked_state gen7_wm_constant_surface = {
   .dirty = {
      .mesa = 0,
      .brw = (BRW_NEW_WM_CONSTBUF |
	      BRW_NEW_BATCH),
      .cache = 0
   },
   .emit = upload_wm_constant_surface,
};

static void
gen7_update_null_renderbuffer_surface(struct brw_context *brw, unsigned unit)
{
   struct gen7_surface_state *surf;

   surf = brw_state_batch(brw, sizeof(*surf), 32,
			 &brw->wm.surf_offset[unit]);
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
   struct intel_region *region = irb->region;
   struct gen7_surface_state *surf;
   uint32_t tile_x, tile_y;

   surf = brw_state_batch(brw, sizeof(*surf), 32,
			  &brw->wm.surf_offset[unit]);
   memset(surf, 0, sizeof(*surf));

   switch (irb->Base.Format) {
   case MESA_FORMAT_XRGB8888:
      /* XRGB is handled as ARGB because the chips in this family
       * cannot render to XRGB targets.  This means that we have to
       * mask writes to alpha (ala glColorMask) and reconfigure the
       * alpha blending hardware to use GL_ONE (or GL_ZERO) for
       * cases where GL_DST_ALPHA (or GL_ONE_MINUS_DST_ALPHA) is
       * used.
       */
      surf->ss0.surface_format = BRW_SURFACEFORMAT_B8G8R8A8_UNORM;
      break;
   case MESA_FORMAT_INTENSITY_FLOAT32:
   case MESA_FORMAT_LUMINANCE_FLOAT32:
      /* For these formats, we just need to read/write the first
       * channel into R, which is to say that we just treat them as
       * GL_RED.
       */
      surf->ss0.surface_format = BRW_SURFACEFORMAT_R32_FLOAT;
      break;
   case MESA_FORMAT_SARGB8:
      /* without GL_EXT_framebuffer_sRGB we shouldn't bind sRGB
	 surfaces to the blend/update as sRGB */
      if (ctx->Color.sRGBEnabled)
	 surf->ss0.surface_format = brw_format_for_mesa_format(irb->Base.Format);
      else
	 surf->ss0.surface_format = BRW_SURFACEFORMAT_B8G8R8A8_UNORM;
      break;
   default:
      assert(brw_render_target_supported(irb->Base.Format));
      surf->ss0.surface_format = brw_format_for_mesa_format(irb->Base.Format);
   }

   surf->ss0.surface_type = BRW_SURFACE_2D;
   /* reloc */
   surf->ss1.base_addr = intel_renderbuffer_tile_offsets(irb, &tile_x, &tile_y);
   surf->ss1.base_addr += region->buffer->offset; /* reloc */

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
			   brw->wm.surf_offset[unit] +
			   offsetof(struct gen7_surface_state, ss1),
			   region->buffer,
			   surf->ss1.base_addr - region->buffer->offset,
			   I915_GEM_DOMAIN_RENDER,
			   I915_GEM_DOMAIN_RENDER);
}

static void
prepare_wm_surfaces(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->intel.ctx;
   int i;
   int nr_surfaces = 0;

   if (ctx->DrawBuffer->_NumColorDrawBuffers >= 1) {
      for (i = 0; i < ctx->DrawBuffer->_NumColorDrawBuffers; i++) {
	 struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffers[i];
	 struct intel_renderbuffer *irb = intel_renderbuffer(rb);
	 struct intel_region *region = irb ? irb->region : NULL;

	 if (region)
	    brw_add_validated_bo(brw, region->buffer);
	 nr_surfaces = SURF_INDEX_DRAW(i) + 1;
      }
   }

   if (brw->wm.const_bo) {
      brw_add_validated_bo(brw, brw->wm.const_bo);
      nr_surfaces = SURF_INDEX_FRAG_CONST_BUFFER + 1;
   }

   for (i = 0; i < BRW_MAX_TEX_UNIT; i++) {
      const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[i];
      struct gl_texture_object *tObj = texUnit->_Current;
      struct intel_texture_object *intelObj = intel_texture_object(tObj);

      if (texUnit->_ReallyEnabled) {
	 brw_add_validated_bo(brw, intelObj->mt->region->buffer);
	 nr_surfaces = SURF_INDEX_TEXTURE(i) + 1;
      }
   }

   /* Have to update this in our prepare, since the unit's prepare
    * relies on it.
    */
   if (brw->wm.nr_surfaces != nr_surfaces) {
      brw->wm.nr_surfaces = nr_surfaces;
      brw->state.dirty.brw |= BRW_NEW_NR_WM_SURFACES;
   }
}

/**
 * Constructs the set of surface state objects pointed to by the
 * binding table.
 */
static void
upload_wm_surfaces(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->intel.ctx;
   GLuint i;

   /* _NEW_BUFFERS | _NEW_COLOR */
   /* Update surfaces for drawing buffers */
   if (ctx->DrawBuffer->_NumColorDrawBuffers >= 1) {
      for (i = 0; i < ctx->DrawBuffer->_NumColorDrawBuffers; i++) {
	 if (intel_renderbuffer(ctx->DrawBuffer->_ColorDrawBuffers[i])) {
	    gen7_update_renderbuffer_surface(brw,
	       ctx->DrawBuffer->_ColorDrawBuffers[i], i);
	 } else {
	    gen7_update_null_renderbuffer_surface(brw, i);
	 }
      }
   } else {
      gen7_update_null_renderbuffer_surface(brw, 0);
   }

   /* Update surfaces for textures */
   for (i = 0; i < BRW_MAX_TEX_UNIT; i++) {
      const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[i];
      const GLuint surf = SURF_INDEX_TEXTURE(i);

      /* _NEW_TEXTURE */
      if (texUnit->_ReallyEnabled) {
	 gen7_update_texture_surface(ctx, i);
      } else {
         brw->wm.surf_offset[surf] = 0;
      }
   }

   brw->state.dirty.brw |= BRW_NEW_WM_SURFACES;
}

const struct brw_tracked_state gen7_wm_surfaces = {
   .dirty = {
      .mesa = (_NEW_COLOR |
               _NEW_TEXTURE |
               _NEW_BUFFERS),
      .brw = BRW_NEW_BATCH,
      .cache = 0
   },
   .prepare = prepare_wm_surfaces,
   .emit = upload_wm_surfaces,
};
