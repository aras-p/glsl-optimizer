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

GLuint
translate_tex_target(GLenum target)
{
   switch (target) {
   case GL_TEXTURE_1D: 
      return BRW_SURFACE_1D;

   case GL_TEXTURE_RECTANGLE_NV: 
      return BRW_SURFACE_2D;

   case GL_TEXTURE_2D: 
      return BRW_SURFACE_2D;

   case GL_TEXTURE_3D: 
      return BRW_SURFACE_3D;

   case GL_TEXTURE_CUBE_MAP: 
      return BRW_SURFACE_CUBE;

   default: 
      assert(0); 
      return 0;
   }
}

uint32_t
brw_format_for_mesa_format(gl_format mesa_format)
{
   static const uint32_t table[MESA_FORMAT_COUNT] =
   {
      [MESA_FORMAT_L8] = BRW_SURFACEFORMAT_L8_UNORM,
      [MESA_FORMAT_I8] = BRW_SURFACEFORMAT_I8_UNORM,
      [MESA_FORMAT_A8] = BRW_SURFACEFORMAT_A8_UNORM,
      [MESA_FORMAT_AL88] = BRW_SURFACEFORMAT_L8A8_UNORM,
      [MESA_FORMAT_AL1616] = BRW_SURFACEFORMAT_L16A16_UNORM,
      [MESA_FORMAT_R8] = BRW_SURFACEFORMAT_R8_UNORM,
      [MESA_FORMAT_R16] = BRW_SURFACEFORMAT_R16_UNORM,
      [MESA_FORMAT_RG88] = BRW_SURFACEFORMAT_R8G8_UNORM,
      [MESA_FORMAT_RG1616] = BRW_SURFACEFORMAT_R16G16_UNORM,
      [MESA_FORMAT_ARGB8888] = BRW_SURFACEFORMAT_B8G8R8A8_UNORM,
      [MESA_FORMAT_XRGB8888] = BRW_SURFACEFORMAT_B8G8R8X8_UNORM,
      [MESA_FORMAT_RGB565] = BRW_SURFACEFORMAT_B5G6R5_UNORM,
      [MESA_FORMAT_ARGB1555] = BRW_SURFACEFORMAT_B5G5R5A1_UNORM,
      [MESA_FORMAT_ARGB4444] = BRW_SURFACEFORMAT_B4G4R4A4_UNORM,
      [MESA_FORMAT_YCBCR_REV] = BRW_SURFACEFORMAT_YCRCB_NORMAL,
      [MESA_FORMAT_YCBCR] = BRW_SURFACEFORMAT_YCRCB_SWAPUVY,
      [MESA_FORMAT_RGB_FXT1] = BRW_SURFACEFORMAT_FXT1,
      [MESA_FORMAT_RGBA_FXT1] = BRW_SURFACEFORMAT_FXT1,
      [MESA_FORMAT_RGB_DXT1] = BRW_SURFACEFORMAT_DXT1_RGB,
      [MESA_FORMAT_RGBA_DXT1] = BRW_SURFACEFORMAT_BC1_UNORM,
      [MESA_FORMAT_RGBA_DXT3] = BRW_SURFACEFORMAT_BC2_UNORM,
      [MESA_FORMAT_RGBA_DXT5] = BRW_SURFACEFORMAT_BC3_UNORM,
      [MESA_FORMAT_SRGB_DXT1] = BRW_SURFACEFORMAT_DXT1_RGB_SRGB,
      [MESA_FORMAT_SRGBA_DXT1] = BRW_SURFACEFORMAT_BC1_UNORM_SRGB,
      [MESA_FORMAT_SRGBA_DXT3] = BRW_SURFACEFORMAT_BC2_UNORM_SRGB,
      [MESA_FORMAT_SRGBA_DXT5] = BRW_SURFACEFORMAT_BC3_UNORM_SRGB,
      [MESA_FORMAT_SARGB8] = BRW_SURFACEFORMAT_B8G8R8A8_UNORM_SRGB,
      [MESA_FORMAT_SLA8] = BRW_SURFACEFORMAT_L8A8_UNORM_SRGB,
      [MESA_FORMAT_SL8] = BRW_SURFACEFORMAT_L8_UNORM_SRGB,
      [MESA_FORMAT_DUDV8] = BRW_SURFACEFORMAT_R8G8_SNORM,
      [MESA_FORMAT_SIGNED_R8] = BRW_SURFACEFORMAT_R8_SNORM,
      [MESA_FORMAT_SIGNED_RG88_REV] = BRW_SURFACEFORMAT_R8G8_SNORM,
      [MESA_FORMAT_SIGNED_RGBA8888_REV] = BRW_SURFACEFORMAT_R8G8B8A8_SNORM,
      [MESA_FORMAT_SIGNED_R16] = BRW_SURFACEFORMAT_R16_SNORM,
      [MESA_FORMAT_SIGNED_GR1616] = BRW_SURFACEFORMAT_R16G16_SNORM,
      [MESA_FORMAT_RGBA_FLOAT32] = BRW_SURFACEFORMAT_R32G32B32A32_FLOAT,
      [MESA_FORMAT_RG_FLOAT32] = BRW_SURFACEFORMAT_R32G32_FLOAT,
      [MESA_FORMAT_R_FLOAT32] = BRW_SURFACEFORMAT_R32_FLOAT,
      [MESA_FORMAT_INTENSITY_FLOAT32] = BRW_SURFACEFORMAT_I32_FLOAT,
      [MESA_FORMAT_LUMINANCE_FLOAT32] = BRW_SURFACEFORMAT_L32_FLOAT,
      [MESA_FORMAT_ALPHA_FLOAT32] = BRW_SURFACEFORMAT_A32_FLOAT,
      [MESA_FORMAT_LUMINANCE_ALPHA_FLOAT32] = BRW_SURFACEFORMAT_L32A32_FLOAT,
      [MESA_FORMAT_RED_RGTC1] = BRW_SURFACEFORMAT_BC4_UNORM,
      [MESA_FORMAT_SIGNED_RED_RGTC1] = BRW_SURFACEFORMAT_BC4_SNORM,
      [MESA_FORMAT_RG_RGTC2] = BRW_SURFACEFORMAT_BC5_UNORM,
      [MESA_FORMAT_SIGNED_RG_RGTC2] = BRW_SURFACEFORMAT_BC5_SNORM,
   };
   assert(mesa_format < MESA_FORMAT_COUNT);
   return table[mesa_format];
}

bool
brw_render_target_supported(gl_format format)
{
   /* These are not color render targets like the table holds, but we
    * ask the question for FBO completeness.
    */
   if (format == MESA_FORMAT_S8_Z24 ||
       format == MESA_FORMAT_X8_Z24 ||
       format == MESA_FORMAT_S8 ||
       format == MESA_FORMAT_Z16) {
      return true;
   }

   /* The value of this BRW_SURFACEFORMAT is 0, so hardcode it.
    */
   if (format == MESA_FORMAT_RGBA_FLOAT32)
      return true;

   /* Not exactly true, as some of those formats are not renderable.
    * But at least we know how to translate them.
    */
   return brw_format_for_mesa_format(format) != 0;
}

GLuint
translate_tex_format(gl_format mesa_format,
		     GLenum internal_format,
		     GLenum depth_mode,
		     GLenum srgb_decode)
{
   switch( mesa_format ) {

   case MESA_FORMAT_Z16:
      if (depth_mode == GL_INTENSITY) 
	  return BRW_SURFACEFORMAT_I16_UNORM;
      else if (depth_mode == GL_ALPHA)
	  return BRW_SURFACEFORMAT_A16_UNORM;
      else if (depth_mode == GL_RED)
	  return BRW_SURFACEFORMAT_R16_UNORM;
      else
	  return BRW_SURFACEFORMAT_L16_UNORM;

   case MESA_FORMAT_S8_Z24:
   case MESA_FORMAT_X8_Z24:
      /* XXX: these different surface formats don't seem to
       * make any difference for shadow sampler/compares.
       */
      if (depth_mode == GL_INTENSITY) 
         return BRW_SURFACEFORMAT_I24X8_UNORM;
      else if (depth_mode == GL_ALPHA)
         return BRW_SURFACEFORMAT_A24X8_UNORM;
      else if (depth_mode == GL_RED)
         return BRW_SURFACEFORMAT_R24_UNORM_X8_TYPELESS;
      else
         return BRW_SURFACEFORMAT_L24X8_UNORM;
      
   case MESA_FORMAT_SARGB8:
   case MESA_FORMAT_SLA8:
   case MESA_FORMAT_SL8:
      if (srgb_decode == GL_DECODE_EXT)
	 return brw_format_for_mesa_format(mesa_format);
      else if (srgb_decode == GL_SKIP_DECODE_EXT)
	 return brw_format_for_mesa_format(_mesa_get_srgb_format_linear(mesa_format));

   case MESA_FORMAT_RGBA_FLOAT32:
      /* The value of this BRW_SURFACEFORMAT is 0, which tricks the
       * assertion below.
       */
      return BRW_SURFACEFORMAT_R32G32B32A32_FLOAT;

   default:
      assert(brw_format_for_mesa_format(mesa_format) != 0);
      return brw_format_for_mesa_format(mesa_format);
   }
}

static uint32_t
brw_get_surface_tiling_bits(uint32_t tiling)
{
   switch (tiling) {
   case I915_TILING_X:
      return BRW_SURFACE_TILED;
   case I915_TILING_Y:
      return BRW_SURFACE_TILED | BRW_SURFACE_TILED_Y;
   default:
      return 0;
   }
}

static void
brw_update_texture_surface( struct gl_context *ctx, GLuint unit )
{
   struct brw_context *brw = brw_context(ctx);
   struct gl_texture_object *tObj = ctx->Texture.Unit[unit]._Current;
   struct intel_texture_object *intelObj = intel_texture_object(tObj);
   struct gl_texture_image *firstImage = tObj->Image[0][tObj->BaseLevel];
   struct gl_sampler_object *sampler = _mesa_get_samplerobj(ctx, unit);
   const GLuint surf_index = SURF_INDEX_TEXTURE(unit);
   uint32_t *surf;

   surf = brw_state_batch(brw, 6 * 4, 32, &brw->wm.surf_offset[surf_index]);

   surf[0] = (translate_tex_target(tObj->Target) << BRW_SURFACE_TYPE_SHIFT |
	      BRW_SURFACE_MIPMAPLAYOUT_BELOW << BRW_SURFACE_MIPLAYOUT_SHIFT |
	      BRW_SURFACE_CUBEFACE_ENABLES |
	      (translate_tex_format(firstImage->TexFormat,
				    firstImage->InternalFormat,
				    sampler->DepthMode,
				    sampler->sRGBDecode) <<
	       BRW_SURFACE_FORMAT_SHIFT));

   surf[1] = intelObj->mt->region->buffer->offset; /* reloc */

   surf[2] = ((intelObj->_MaxLevel - tObj->BaseLevel) << BRW_SURFACE_LOD_SHIFT |
	      (firstImage->Width - 1) << BRW_SURFACE_WIDTH_SHIFT |
	      (firstImage->Height - 1) << BRW_SURFACE_HEIGHT_SHIFT);

   surf[3] = (brw_get_surface_tiling_bits(intelObj->mt->region->tiling) |
	      (firstImage->Depth - 1) << BRW_SURFACE_DEPTH_SHIFT |
	      ((intelObj->mt->region->pitch * intelObj->mt->cpp) - 1) <<
	      BRW_SURFACE_PITCH_SHIFT);

   surf[4] = 0;
   surf[5] = 0;

   /* Emit relocation to surface contents */
   drm_intel_bo_emit_reloc(brw->intel.batch.bo,
			   brw->wm.surf_offset[surf_index] + 4,
			   intelObj->mt->region->buffer, 0,
			   I915_GEM_DOMAIN_SAMPLER, 0);
}

/**
 * Create the constant buffer surface.  Vertex/fragment shader constants will be
 * read from this buffer with Data Port Read instructions/messages.
 */
void
brw_create_constant_surface(struct brw_context *brw,
			    drm_intel_bo *bo,
			    int width,
			    uint32_t *out_offset)
{
   struct intel_context *intel = &brw->intel;
   const GLint w = width - 1;
   uint32_t *surf;

   surf = brw_state_batch(brw, 6 * 4, 32, out_offset);

   surf[0] = (BRW_SURFACE_BUFFER << BRW_SURFACE_TYPE_SHIFT |
	      BRW_SURFACE_MIPMAPLAYOUT_BELOW << BRW_SURFACE_MIPLAYOUT_SHIFT |
	      BRW_SURFACEFORMAT_R32G32B32A32_FLOAT << BRW_SURFACE_FORMAT_SHIFT);

   if (intel->gen >= 6)
      surf[0] |= BRW_SURFACE_RC_READ_WRITE;

   surf[1] = bo->offset; /* reloc */

   surf[2] = (((w & 0x7f) - 1) << BRW_SURFACE_WIDTH_SHIFT |
	      (((w >> 7) & 0x1fff) - 1) << BRW_SURFACE_HEIGHT_SHIFT);

   surf[3] = ((((w >> 20) & 0x7f) - 1) << BRW_SURFACE_DEPTH_SHIFT |
	      (width * 16 - 1) << BRW_SURFACE_PITCH_SHIFT);

   surf[4] = 0;
   surf[5] = 0;

   /* Emit relocation to surface contents.  Section 5.1.1 of the gen4
    * bspec ("Data Cache") says that the data cache does not exist as
    * a separate cache and is just the sampler cache.
    */
   drm_intel_bo_emit_reloc(brw->intel.batch.bo,
			   *out_offset + 4,
			   bo, 0,
			   I915_GEM_DOMAIN_SAMPLER, 0);
}

/* Creates a new WM constant buffer reflecting the current fragment program's
 * constants, if needed by the fragment program.
 *
 * Otherwise, constants go through the CURBEs using the brw_constant_buffer
 * state atom.
 */
static void
prepare_wm_pull_constants(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->intel.ctx;
   struct intel_context *intel = &brw->intel;
   struct brw_fragment_program *fp =
      (struct brw_fragment_program *) brw->fragment_program;
   const int size = brw->wm.prog_data->nr_pull_params * sizeof(float);
   float *constants;
   unsigned int i;

   _mesa_load_state_parameters(ctx, fp->program.Base.Parameters);

   /* BRW_NEW_FRAGMENT_PROGRAM */
   if (brw->wm.prog_data->nr_pull_params == 0) {
      if (brw->wm.const_bo) {
	 drm_intel_bo_unreference(brw->wm.const_bo);
	 brw->wm.const_bo = NULL;
	 brw->state.dirty.brw |= BRW_NEW_WM_CONSTBUF;
      }
      return;
   }

   drm_intel_bo_unreference(brw->wm.const_bo);
   brw->wm.const_bo = drm_intel_bo_alloc(intel->bufmgr, "WM const bo",
					 size, 64);

   /* _NEW_PROGRAM_CONSTANTS */
   drm_intel_gem_bo_map_gtt(brw->wm.const_bo);
   constants = brw->wm.const_bo->virtual;
   for (i = 0; i < brw->wm.prog_data->nr_pull_params; i++) {
      constants[i] = convert_param(brw->wm.prog_data->pull_param_convert[i],
				   *brw->wm.prog_data->pull_param[i]);
   }
   drm_intel_gem_bo_unmap_gtt(brw->wm.const_bo);

   brw->state.dirty.brw |= BRW_NEW_WM_CONSTBUF;
}

const struct brw_tracked_state brw_wm_constants = {
   .dirty = {
      .mesa = (_NEW_PROGRAM_CONSTANTS),
      .brw = (BRW_NEW_FRAGMENT_PROGRAM),
      .cache = 0
   },
   .prepare = prepare_wm_pull_constants,
};

/**
 * Updates surface / buffer for fragment shader constant buffer, if
 * one is required.
 *
 * This consumes the state updates for the constant buffer, and produces
 * BRW_NEW_WM_SURFACES to get picked up by brw_prepare_wm_surfaces for
 * inclusion in the binding table.
 */
static void upload_wm_constant_surface(struct brw_context *brw )
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

   brw_create_constant_surface(brw, brw->wm.const_bo, params->NumParameters,
			       &brw->wm.surf_offset[surf]);
   brw->state.dirty.brw |= BRW_NEW_WM_SURFACES;
}

const struct brw_tracked_state brw_wm_constant_surface = {
   .dirty = {
      .mesa = 0,
      .brw = (BRW_NEW_WM_CONSTBUF |
	      BRW_NEW_BATCH),
      .cache = 0
   },
   .emit = upload_wm_constant_surface,
};

static void
brw_update_null_renderbuffer_surface(struct brw_context *brw, unsigned int unit)
{
   struct intel_context *intel = &brw->intel;
   uint32_t *surf;

   surf = brw_state_batch(brw, 6 * 4, 32, &brw->wm.surf_offset[unit]);

   surf[0] = (BRW_SURFACE_NULL << BRW_SURFACE_TYPE_SHIFT |
	      BRW_SURFACEFORMAT_B8G8R8A8_UNORM << BRW_SURFACE_FORMAT_SHIFT);
   if (intel->gen < 6) {
      surf[0] |= (1 << BRW_SURFACE_WRITEDISABLE_R_SHIFT |
		  1 << BRW_SURFACE_WRITEDISABLE_G_SHIFT |
		  1 << BRW_SURFACE_WRITEDISABLE_B_SHIFT |
		  1 << BRW_SURFACE_WRITEDISABLE_A_SHIFT);
   }
   surf[1] = 0;
   surf[2] = 0;
   surf[3] = 0;
   surf[4] = 0;
   surf[5] = 0;
}

/**
 * Sets up a surface state structure to point at the given region.
 * While it is only used for the front/back buffer currently, it should be
 * usable for further buffers when doing ARB_draw_buffer support.
 */
static void
brw_update_renderbuffer_surface(struct brw_context *brw,
				struct gl_renderbuffer *rb,
				unsigned int unit)
{
   struct intel_context *intel = &brw->intel;
   struct gl_context *ctx = &intel->ctx;
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);
   struct intel_region *region = irb->region;
   uint32_t *surf;
   uint32_t tile_x, tile_y;
   uint32_t format = 0;

   surf = brw_state_batch(brw, 6 * 4, 32, &brw->wm.surf_offset[unit]);

   switch (irb->Base.Format) {
   case MESA_FORMAT_XRGB8888:
      /* XRGB is handled as ARGB because the chips in this family
       * cannot render to XRGB targets.  This means that we have to
       * mask writes to alpha (ala glColorMask) and reconfigure the
       * alpha blending hardware to use GL_ONE (or GL_ZERO) for
       * cases where GL_DST_ALPHA (or GL_ONE_MINUS_DST_ALPHA) is
       * used.
       */
      format = BRW_SURFACEFORMAT_B8G8R8A8_UNORM;
      break;
   case MESA_FORMAT_INTENSITY_FLOAT32:
   case MESA_FORMAT_LUMINANCE_FLOAT32:
      /* For these formats, we just need to read/write the first
       * channel into R, which is to say that we just treat them as
       * GL_RED.
       */
      format = BRW_SURFACEFORMAT_R32_FLOAT;
      break;
   case MESA_FORMAT_SARGB8:
      /* without GL_EXT_framebuffer_sRGB we shouldn't bind sRGB
	 surfaces to the blend/update as sRGB */
      if (ctx->Color.sRGBEnabled)
	 format = brw_format_for_mesa_format(irb->Base.Format);
      else
	 format = BRW_SURFACEFORMAT_B8G8R8A8_UNORM;
      break;
   default:
      assert(brw_render_target_supported(irb->Base.Format));
      format = brw_format_for_mesa_format(irb->Base.Format);
   }

   surf[0] = (BRW_SURFACE_2D << BRW_SURFACE_TYPE_SHIFT |
	      format << BRW_SURFACE_FORMAT_SHIFT);

   /* reloc */
   surf[1] = (intel_renderbuffer_tile_offsets(irb, &tile_x, &tile_y) +
	      region->buffer->offset);

   surf[2] = ((rb->Width - 1) << BRW_SURFACE_WIDTH_SHIFT |
	      (rb->Height - 1) << BRW_SURFACE_HEIGHT_SHIFT);

   surf[3] = (brw_get_surface_tiling_bits(region->tiling) |
	      ((region->pitch * region->cpp) - 1) << BRW_SURFACE_PITCH_SHIFT);

   surf[4] = 0;

   assert(brw->has_surface_tile_offset || (tile_x == 0 && tile_y == 0));
   /* Note that the low bits of these fields are missing, so
    * there's the possibility of getting in trouble.
    */
   assert(tile_x % 4 == 0);
   assert(tile_y % 2 == 0);
   surf[5] = ((tile_x / 4) << BRW_SURFACE_X_OFFSET_SHIFT |
	      (tile_y / 2) << BRW_SURFACE_Y_OFFSET_SHIFT);

   if (intel->gen < 6) {
      /* _NEW_COLOR */
      if (!ctx->Color._LogicOpEnabled &&
	  (ctx->Color.BlendEnabled & (1 << unit)))
	 surf[0] |= BRW_SURFACE_BLEND_ENABLED;

      if (!ctx->Color.ColorMask[unit][0])
	 surf[0] |= 1 << BRW_SURFACE_WRITEDISABLE_R_SHIFT;
      if (!ctx->Color.ColorMask[unit][1])
	 surf[0] |= 1 << BRW_SURFACE_WRITEDISABLE_G_SHIFT;
      if (!ctx->Color.ColorMask[unit][2])
	 surf[0] |= 1 << BRW_SURFACE_WRITEDISABLE_B_SHIFT;

      /* As mentioned above, disable writes to the alpha component when the
       * renderbuffer is XRGB.
       */
      if (ctx->DrawBuffer->Visual.alphaBits == 0 ||
	  !ctx->Color.ColorMask[unit][3]) {
	 surf[0] |= 1 << BRW_SURFACE_WRITEDISABLE_A_SHIFT;
      }
   }

   drm_intel_bo_emit_reloc(brw->intel.batch.bo,
			   brw->wm.surf_offset[unit] + 4,
			   region->buffer,
			   surf[1] - region->buffer->offset,
			   I915_GEM_DOMAIN_RENDER,
			   I915_GEM_DOMAIN_RENDER);
}

static void
prepare_wm_surfaces(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->intel.ctx;
   int i;
   int nr_surfaces = 0;

   for (i = 0; i < ctx->DrawBuffer->_NumColorDrawBuffers; i++) {
      struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffers[i];
      struct intel_renderbuffer *irb = intel_renderbuffer(rb);
      struct intel_region *region = irb ? irb->region : NULL;

      if (region)
	 brw_add_validated_bo(brw, region->buffer);
      nr_surfaces = SURF_INDEX_DRAW(i) + 1;
   }

   if (brw->wm.const_bo) {
      brw_add_validated_bo(brw, brw->wm.const_bo);
      nr_surfaces = SURF_INDEX_FRAG_CONST_BUFFER + 1;
   }

   for (i = 0; i < BRW_MAX_TEX_UNIT; i++) {
      const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[i];

      if (texUnit->_ReallyEnabled) {
	 struct gl_texture_object *tObj = texUnit->_Current;
	 struct intel_texture_object *intelObj = intel_texture_object(tObj);

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
	    brw_update_renderbuffer_surface(brw,
					    ctx->DrawBuffer->_ColorDrawBuffers[i],
					    i);
	 } else {
	    brw_update_null_renderbuffer_surface(brw, i);
	 }
      }
   } else {
      brw_update_null_renderbuffer_surface(brw, 0);
   }

   /* Update surfaces for textures */
   for (i = 0; i < BRW_MAX_TEX_UNIT; i++) {
      const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[i];
      const GLuint surf = SURF_INDEX_TEXTURE(i);

      /* _NEW_TEXTURE */
      if (texUnit->_ReallyEnabled) {
	 brw_update_texture_surface(ctx, i);
      } else {
         brw->wm.surf_offset[surf] = 0;
      }
   }

   brw->state.dirty.brw |= BRW_NEW_WM_SURFACES;
}

const struct brw_tracked_state brw_wm_surfaces = {
   .dirty = {
      .mesa = (_NEW_COLOR |
               _NEW_TEXTURE |
               _NEW_BUFFERS),
      .brw = (BRW_NEW_BATCH),
      .cache = 0
   },
   .prepare = prepare_wm_surfaces,
   .emit = upload_wm_surfaces,
};

/**
 * Constructs the binding table for the WM surface state, which maps unit
 * numbers to surface state objects.
 */
static void
brw_wm_upload_binding_table(struct brw_context *brw)
{
   uint32_t *bind;
   int i;

   /* Might want to calculate nr_surfaces first, to avoid taking up so much
    * space for the binding table.
    */
   bind = brw_state_batch(brw, sizeof(uint32_t) * BRW_WM_MAX_SURF,
			  32, &brw->wm.bind_bo_offset);

   for (i = 0; i < BRW_WM_MAX_SURF; i++) {
      /* BRW_NEW_WM_SURFACES */
      bind[i] = brw->wm.surf_offset[i];
   }

   brw->state.dirty.brw |= BRW_NEW_PS_BINDING_TABLE;
}

const struct brw_tracked_state brw_wm_binding_table = {
   .dirty = {
      .mesa = 0,
      .brw = (BRW_NEW_BATCH |
	      BRW_NEW_WM_SURFACES),
      .cache = 0
   },
   .emit = brw_wm_upload_binding_table,
};
