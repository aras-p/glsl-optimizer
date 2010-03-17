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
#include "main/texstore.h"
#include "shader/prog_parameter.h"

#include "intel_mipmap_tree.h"
#include "intel_batchbuffer.h"
#include "intel_tex.h"
#include "intel_fbo.h"

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"


static GLuint translate_tex_target( GLenum target )
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


static GLuint translate_tex_format( gl_format mesa_format,
                                    GLenum internal_format,
				    GLenum depth_mode )
{
   switch( mesa_format ) {
   case MESA_FORMAT_L8:
      return BRW_SURFACEFORMAT_L8_UNORM;

   case MESA_FORMAT_I8:
      return BRW_SURFACEFORMAT_I8_UNORM;

   case MESA_FORMAT_A8:
      return BRW_SURFACEFORMAT_A8_UNORM; 

   case MESA_FORMAT_AL88:
      return BRW_SURFACEFORMAT_L8A8_UNORM;

   case MESA_FORMAT_AL1616:
      return BRW_SURFACEFORMAT_L16A16_UNORM;

   case MESA_FORMAT_RGB888:
      assert(0);		/* not supported for sampling */
      return BRW_SURFACEFORMAT_R8G8B8_UNORM;      

   case MESA_FORMAT_ARGB8888:
      return BRW_SURFACEFORMAT_B8G8R8A8_UNORM;

   case MESA_FORMAT_XRGB8888:
      return BRW_SURFACEFORMAT_B8G8R8X8_UNORM;

   case MESA_FORMAT_RGBA8888_REV:
      _mesa_problem(NULL, "unexpected format in i965:translate_tex_format()");
      return BRW_SURFACEFORMAT_R8G8B8A8_UNORM;

   case MESA_FORMAT_RGB565:
      return BRW_SURFACEFORMAT_B5G6R5_UNORM;

   case MESA_FORMAT_ARGB1555:
      return BRW_SURFACEFORMAT_B5G5R5A1_UNORM;

   case MESA_FORMAT_ARGB4444:
      return BRW_SURFACEFORMAT_B4G4R4A4_UNORM;

   case MESA_FORMAT_YCBCR_REV:
      return BRW_SURFACEFORMAT_YCRCB_NORMAL;

   case MESA_FORMAT_YCBCR:
      return BRW_SURFACEFORMAT_YCRCB_SWAPUVY;

   case MESA_FORMAT_RGB_FXT1:
   case MESA_FORMAT_RGBA_FXT1:
      return BRW_SURFACEFORMAT_FXT1;

   case MESA_FORMAT_Z16:
      if (depth_mode == GL_INTENSITY) 
	  return BRW_SURFACEFORMAT_I16_UNORM;
      else if (depth_mode == GL_ALPHA)
	  return BRW_SURFACEFORMAT_A16_UNORM;
      else
	  return BRW_SURFACEFORMAT_L16_UNORM;

   case MESA_FORMAT_RGB_DXT1:
       return BRW_SURFACEFORMAT_DXT1_RGB;

   case MESA_FORMAT_RGBA_DXT1:
       return BRW_SURFACEFORMAT_BC1_UNORM;
       
   case MESA_FORMAT_RGBA_DXT3:
       return BRW_SURFACEFORMAT_BC2_UNORM;
       
   case MESA_FORMAT_RGBA_DXT5:
       return BRW_SURFACEFORMAT_BC3_UNORM;

   case MESA_FORMAT_SARGB8:
      return BRW_SURFACEFORMAT_B8G8R8A8_UNORM_SRGB;

   case MESA_FORMAT_SLA8:
      return BRW_SURFACEFORMAT_L8A8_UNORM_SRGB;

   case MESA_FORMAT_SL8:
      return BRW_SURFACEFORMAT_L8_UNORM_SRGB;

   case MESA_FORMAT_SRGB_DXT1:
      return BRW_SURFACEFORMAT_BC1_UNORM_SRGB;

   case MESA_FORMAT_S8_Z24:
      /* XXX: these different surface formats don't seem to
       * make any difference for shadow sampler/compares.
       */
      if (depth_mode == GL_INTENSITY) 
         return BRW_SURFACEFORMAT_I24X8_UNORM;
      else if (depth_mode == GL_ALPHA)
         return BRW_SURFACEFORMAT_A24X8_UNORM;
      else
         return BRW_SURFACEFORMAT_L24X8_UNORM;

   case MESA_FORMAT_DUDV8:
      return BRW_SURFACEFORMAT_R8G8_SNORM;

   case MESA_FORMAT_SIGNED_RGBA8888_REV:
      return BRW_SURFACEFORMAT_R8G8B8A8_SNORM;

   default:
      assert(0);
      return 0;
   }
}

static void
brw_set_surface_tiling(struct brw_surface_state *surf, uint32_t tiling)
{
   switch (tiling) {
   case I915_TILING_NONE:
      surf->ss3.tiled_surface = 0;
      surf->ss3.tile_walk = 0;
      break;
   case I915_TILING_X:
      surf->ss3.tiled_surface = 1;
      surf->ss3.tile_walk = BRW_TILEWALK_XMAJOR;
      break;
   case I915_TILING_Y:
      surf->ss3.tiled_surface = 1;
      surf->ss3.tile_walk = BRW_TILEWALK_YMAJOR;
      break;
   }
}

static dri_bo *
brw_create_texture_surface( struct brw_context *brw,
			    struct brw_surface_key *key )
{
   struct brw_surface_state surf;
   dri_bo *bo;

   memset(&surf, 0, sizeof(surf));

   surf.ss0.mipmap_layout_mode = BRW_SURFACE_MIPMAPLAYOUT_BELOW;
   surf.ss0.surface_type = translate_tex_target(key->target);
   surf.ss0.surface_format = translate_tex_format(key->format,
						  key->internal_format,
						  key->depthmode);

   /* This is ok for all textures with channel width 8bit or less:
    */
/*    surf.ss0.data_return_format = BRW_SURFACERETURNFORMAT_S1; */
   surf.ss1.base_addr = key->bo->offset; /* reloc */

   surf.ss2.mip_count = key->last_level - key->first_level;
   surf.ss2.width = key->width - 1;
   surf.ss2.height = key->height - 1;
   brw_set_surface_tiling(&surf, key->tiling);
   surf.ss3.pitch = (key->pitch * key->cpp) - 1;
   surf.ss3.depth = key->depth - 1;

   surf.ss4.min_lod = 0;
 
   if (key->target == GL_TEXTURE_CUBE_MAP) {
      surf.ss0.cube_pos_x = 1;
      surf.ss0.cube_pos_y = 1;
      surf.ss0.cube_pos_z = 1;
      surf.ss0.cube_neg_x = 1;
      surf.ss0.cube_neg_y = 1;
      surf.ss0.cube_neg_z = 1;
   }

   bo = brw_upload_cache(&brw->surface_cache, BRW_SS_SURFACE,
			 key, sizeof(*key),
			 &key->bo, 1,
			 &surf, sizeof(surf));

   /* Emit relocation to surface contents */
   drm_intel_bo_emit_reloc(bo, offsetof(struct brw_surface_state, ss1),
			   key->bo, 0,
			   I915_GEM_DOMAIN_SAMPLER, 0);

   return bo;
}

static void
brw_update_texture_surface( GLcontext *ctx, GLuint unit )
{
   struct brw_context *brw = brw_context(ctx);
   struct gl_texture_object *tObj = ctx->Texture.Unit[unit]._Current;
   struct intel_texture_object *intelObj = intel_texture_object(tObj);
   struct gl_texture_image *firstImage = tObj->Image[0][intelObj->firstLevel];
   struct brw_surface_key key;
   const GLuint surf = SURF_INDEX_TEXTURE(unit);

   memset(&key, 0, sizeof(key));

   key.format = firstImage->TexFormat;
   key.internal_format = firstImage->InternalFormat;
   key.pitch = intelObj->mt->region->pitch;
   key.depth = firstImage->Depth;
   key.bo = intelObj->mt->region->buffer;
   key.offset = 0;

   key.target = tObj->Target;
   key.depthmode = tObj->DepthMode;
   key.first_level = intelObj->firstLevel;
   key.last_level = intelObj->lastLevel;
   key.width = firstImage->Width;
   key.height = firstImage->Height;
   key.cpp = intelObj->mt->cpp;
   key.tiling = intelObj->mt->region->tiling;

   dri_bo_unreference(brw->wm.surf_bo[surf]);
   brw->wm.surf_bo[surf] = brw_search_cache(&brw->surface_cache,
                                            BRW_SS_SURFACE,
                                            &key, sizeof(key),
                                            &key.bo, 1,
                                            NULL);
   if (brw->wm.surf_bo[surf] == NULL) {
      brw->wm.surf_bo[surf] = brw_create_texture_surface(brw, &key);
   }
}



/**
 * Create the constant buffer surface.  Vertex/fragment shader constants will be
 * read from this buffer with Data Port Read instructions/messages.
 */
dri_bo *
brw_create_constant_surface( struct brw_context *brw,
                             struct brw_surface_key *key )
{
   const GLint w = key->width - 1;
   struct brw_surface_state surf;
   dri_bo *bo;

   memset(&surf, 0, sizeof(surf));

   surf.ss0.mipmap_layout_mode = BRW_SURFACE_MIPMAPLAYOUT_BELOW;
   surf.ss0.surface_type = BRW_SURFACE_BUFFER;
   surf.ss0.surface_format = BRW_SURFACEFORMAT_R32G32B32A32_FLOAT;

   assert(key->bo);
   surf.ss1.base_addr = key->bo->offset; /* reloc */

   surf.ss2.width = w & 0x7f;            /* bits 6:0 of size or width */
   surf.ss2.height = (w >> 7) & 0x1fff;  /* bits 19:7 of size or width */
   surf.ss3.depth = (w >> 20) & 0x7f;    /* bits 26:20 of size or width */
   surf.ss3.pitch = (key->pitch * key->cpp) - 1; /* ignored?? */
   brw_set_surface_tiling(&surf, key->tiling); /* tiling now allowed */
 
   bo = brw_upload_cache(&brw->surface_cache, BRW_SS_SURFACE,
			 key, sizeof(*key),
			 &key->bo, 1,
			 &surf, sizeof(surf));

   /* Emit relocation to surface contents.  Section 5.1.1 of the gen4
    * bspec ("Data Cache") says that the data cache does not exist as
    * a separate cache and is just the sampler cache.
    */
   drm_intel_bo_emit_reloc(bo, offsetof(struct brw_surface_state, ss1),
			   key->bo, 0,
			   I915_GEM_DOMAIN_SAMPLER, 0);

   return bo;
}

/* Creates a new WM constant buffer reflecting the current fragment program's
 * constants, if needed by the fragment program.
 *
 * Otherwise, constants go through the CURBEs using the brw_constant_buffer
 * state atom.
 */
static drm_intel_bo *
brw_wm_update_constant_buffer(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct brw_fragment_program *fp =
      (struct brw_fragment_program *) brw->fragment_program;
   const struct gl_program_parameter_list *params = fp->program.Base.Parameters;
   const int size = params->NumParameters * 4 * sizeof(GLfloat);
   drm_intel_bo *const_buffer;

   /* BRW_NEW_FRAGMENT_PROGRAM */
   if (!fp->use_const_buffer)
      return NULL;

   const_buffer = drm_intel_bo_alloc(intel->bufmgr, "fp_const_buffer",
				     size, 64);

   /* _NEW_PROGRAM_CONSTANTS */
   dri_bo_subdata(const_buffer, 0, size, params->ParameterValues);

   return const_buffer;
}

/**
 * Update the surface state for a WM constant buffer.
 * The constant buffer will be (re)allocated here if needed.
 */
static void
brw_update_wm_constant_surface( GLcontext *ctx,
                                GLuint surf)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_surface_key key;
   struct brw_fragment_program *fp =
      (struct brw_fragment_program *) brw->fragment_program;
   const struct gl_program_parameter_list *params =
      fp->program.Base.Parameters;

   /* If we're in this state update atom, we need to update WM constants, so
    * free the old buffer and create a new one for the new contents.
    */
   dri_bo_unreference(fp->const_buffer);
   fp->const_buffer = brw_wm_update_constant_buffer(brw);

   /* If there's no constant buffer, then no surface BO is needed to point at
    * it.
    */
   if (fp->const_buffer == NULL) {
      drm_intel_bo_unreference(brw->wm.surf_bo[surf]);
      brw->wm.surf_bo[surf] = NULL;
      return;
   }

   memset(&key, 0, sizeof(key));

   key.format = MESA_FORMAT_RGBA_FLOAT32;
   key.internal_format = GL_RGBA;
   key.bo = fp->const_buffer;
   key.depthmode = GL_NONE;
   key.pitch = params->NumParameters;
   key.width = params->NumParameters;
   key.height = 1;
   key.depth = 1;
   key.cpp = 16;

   /*
   printf("%s:\n", __FUNCTION__);
   printf("  width %d  height %d  depth %d  cpp %d  pitch %d\n",
          key.width, key.height, key.depth, key.cpp, key.pitch);
   */

   dri_bo_unreference(brw->wm.surf_bo[surf]);
   brw->wm.surf_bo[surf] = brw_search_cache(&brw->surface_cache,
                                            BRW_SS_SURFACE,
                                            &key, sizeof(key),
                                            &key.bo, 1,
                                            NULL);
   if (brw->wm.surf_bo[surf] == NULL) {
      brw->wm.surf_bo[surf] = brw_create_constant_surface(brw, &key);
   }
   brw->state.dirty.brw |= BRW_NEW_WM_SURFACES;
}

/**
 * Updates surface / buffer for fragment shader constant buffer, if
 * one is required.
 *
 * This consumes the state updates for the constant buffer, and produces
 * BRW_NEW_WM_SURFACES to get picked up by brw_prepare_wm_surfaces for
 * inclusion in the binding table.
 */
static void prepare_wm_constant_surface(struct brw_context *brw )
{
   GLcontext *ctx = &brw->intel.ctx;
   struct brw_fragment_program *fp =
      (struct brw_fragment_program *) brw->fragment_program;
   GLuint surf = SURF_INDEX_FRAG_CONST_BUFFER;

   drm_intel_bo_unreference(fp->const_buffer);
   fp->const_buffer = brw_wm_update_constant_buffer(brw);

   /* If there's no constant buffer, then no surface BO is needed to point at
    * it.
    */
   if (fp->const_buffer == 0) {
      if (brw->wm.surf_bo[surf] != NULL) {
	 drm_intel_bo_unreference(brw->wm.surf_bo[surf]);
	 brw->wm.surf_bo[surf] = NULL;
	 brw->state.dirty.brw |= BRW_NEW_WM_SURFACES;
      }
      return;
   }

   brw_update_wm_constant_surface(ctx, surf);
}

const struct brw_tracked_state brw_wm_constant_surface = {
   .dirty = {
      .mesa = (_NEW_PROGRAM_CONSTANTS),
      .brw = (BRW_NEW_FRAGMENT_PROGRAM),
      .cache = 0
   },
   .prepare = prepare_wm_constant_surface,
};


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
   GLcontext *ctx = &intel->ctx;
   dri_bo *region_bo = NULL;
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);
   struct intel_region *region = irb ? irb->region : NULL;
   struct {
      unsigned int surface_type;
      unsigned int surface_format;
      unsigned int width, height, pitch, cpp;
      GLubyte color_mask[4];
      GLboolean color_blend;
      uint32_t tiling;
      uint32_t draw_x;
      uint32_t draw_y;
   } key;

   memset(&key, 0, sizeof(key));

   if (region != NULL) {
      region_bo = region->buffer;

      key.surface_type = BRW_SURFACE_2D;
      switch (irb->Base.Format) {
      /* XRGB and ARGB are treated the same here because the chips in this
       * family cannot render to XRGB targets.  This means that we have to
       * mask writes to alpha (ala glColorMask) and reconfigure the alpha
       * blending hardware to use GL_ONE (or GL_ZERO) for cases where
       * GL_DST_ALPHA (or GL_ONE_MINUS_DST_ALPHA) is used.
       */
      case MESA_FORMAT_ARGB8888:
      case MESA_FORMAT_XRGB8888:
	 key.surface_format = BRW_SURFACEFORMAT_B8G8R8A8_UNORM;
	 break;
      case MESA_FORMAT_RGB565:
	 key.surface_format = BRW_SURFACEFORMAT_B5G6R5_UNORM;
	 break;
      case MESA_FORMAT_ARGB1555:
	 key.surface_format = BRW_SURFACEFORMAT_B5G5R5A1_UNORM;
	 break;
      case MESA_FORMAT_ARGB4444:
	 key.surface_format = BRW_SURFACEFORMAT_B4G4R4A4_UNORM;
	 break;
      default:
	 _mesa_problem(ctx, "Bad renderbuffer format: %d\n", irb->Base.Format);
      }
      key.tiling = region->tiling;
      if (brw->intel.intelScreen->driScrnPriv->dri2.enabled) {
	 key.width = rb->Width;
	 key.height = rb->Height;
      } else {
	 key.width = region->width;
	 key.height = region->height;
      }
      key.pitch = region->pitch;
      key.cpp = region->cpp;
      key.draw_x = region->draw_x;
      key.draw_y = region->draw_y;
   } else {
      key.surface_type = BRW_SURFACE_NULL;
      key.surface_format = BRW_SURFACEFORMAT_B8G8R8A8_UNORM;
      key.tiling = I915_TILING_X;
      key.width = 1;
      key.height = 1;
      key.cpp = 4;
      key.draw_x = 0;
      key.draw_y = 0;
   }

   if (intel->gen < 6) {
      /* _NEW_COLOR */
      memcpy(key.color_mask, ctx->Color.ColorMask[unit],
	     sizeof(key.color_mask));

      /* As mentioned above, disable writes to the alpha component when the
       * renderbuffer is XRGB.
       */
      if (ctx->DrawBuffer->Visual.alphaBits == 0)
	 key.color_mask[3] = GL_FALSE;

      key.color_blend = (!ctx->Color._LogicOpEnabled &&
			 (ctx->Color.BlendEnabled & (1 << unit)));
   }

   dri_bo_unreference(brw->wm.surf_bo[unit]);
   brw->wm.surf_bo[unit] = brw_search_cache(&brw->surface_cache,
					    BRW_SS_SURFACE,
					    &key, sizeof(key),
					    &region_bo, 1,
					    NULL);

   if (brw->wm.surf_bo[unit] == NULL) {
      struct brw_surface_state surf;

      memset(&surf, 0, sizeof(surf));

      surf.ss0.surface_format = key.surface_format;
      surf.ss0.surface_type = key.surface_type;
      if (key.tiling == I915_TILING_NONE) {
	 surf.ss1.base_addr = (key.draw_x + key.draw_y * key.pitch) * key.cpp;
      } else {
	 uint32_t tile_base, tile_x, tile_y;
	 uint32_t pitch = key.pitch * key.cpp;

	 if (key.tiling == I915_TILING_X) {
	    tile_x = key.draw_x % (512 / key.cpp);
	    tile_y = key.draw_y % 8;
	    tile_base = ((key.draw_y / 8) * (8 * pitch));
	    tile_base += (key.draw_x - tile_x) / (512 / key.cpp) * 4096;
	 } else {
	    /* Y */
	    tile_x = key.draw_x % (128 / key.cpp);
	    tile_y = key.draw_y % 32;
	    tile_base = ((key.draw_y / 32) * (32 * pitch));
	    tile_base += (key.draw_x - tile_x) / (128 / key.cpp) * 4096;
	 }
	 assert(intel->is_g4x || (tile_x == 0 && tile_y == 0));
	 assert(tile_x % 4 == 0);
	 assert(tile_y % 2 == 0);
	 /* Note that the low bits of these fields are missing, so
	  * there's the possibility of getting in trouble.
	  */
	 surf.ss1.base_addr = tile_base;
	 surf.ss5.x_offset = tile_x / 4;
	 surf.ss5.y_offset = tile_y / 2;
      }
      if (region_bo != NULL)
	 surf.ss1.base_addr += region_bo->offset; /* reloc */

      surf.ss2.width = key.width - 1;
      surf.ss2.height = key.height - 1;
      brw_set_surface_tiling(&surf, key.tiling);
      surf.ss3.pitch = (key.pitch * key.cpp) - 1;

      if (intel->gen < 6) {
	 /* _NEW_COLOR */
	 surf.ss0.color_blend = key.color_blend;
	 surf.ss0.writedisable_red =   !key.color_mask[0];
	 surf.ss0.writedisable_green = !key.color_mask[1];
	 surf.ss0.writedisable_blue =  !key.color_mask[2];
	 surf.ss0.writedisable_alpha = !key.color_mask[3];
      }

      /* Key size will never match key size for textures, so we're safe. */
      brw->wm.surf_bo[unit] = brw_upload_cache(&brw->surface_cache,
                                               BRW_SS_SURFACE,
                                               &key, sizeof(key),
					       &region_bo, 1,
					       &surf, sizeof(surf));
      if (region_bo != NULL) {
	 /* We might sample from it, and we might render to it, so flag
	  * them both.  We might be able to figure out from other state
	  * a more restrictive relocation to emit.
	  */
	 drm_intel_bo_emit_reloc(brw->wm.surf_bo[unit],
				 offsetof(struct brw_surface_state, ss1),
				 region_bo,
				 surf.ss1.base_addr - region_bo->offset,
				 I915_GEM_DOMAIN_RENDER,
				 I915_GEM_DOMAIN_RENDER);
      }
   }
}


/**
 * Constructs the binding table for the WM surface state, which maps unit
 * numbers to surface state objects.
 */
static dri_bo *
brw_wm_get_binding_table(struct brw_context *brw)
{
   dri_bo *bind_bo;

   assert(brw->wm.nr_surfaces <= BRW_WM_MAX_SURF);

   bind_bo = brw_search_cache(&brw->surface_cache, BRW_SS_SURF_BIND,
			      NULL, 0,
			      brw->wm.surf_bo, brw->wm.nr_surfaces,
			      NULL);

   if (bind_bo == NULL) {
      GLuint data_size = brw->wm.nr_surfaces * sizeof(GLuint);
      uint32_t data[BRW_WM_MAX_SURF];
      int i;

      for (i = 0; i < brw->wm.nr_surfaces; i++)
         if (brw->wm.surf_bo[i])
            data[i] = brw->wm.surf_bo[i]->offset;
         else
            data[i] = 0;

      bind_bo = brw_upload_cache( &brw->surface_cache, BRW_SS_SURF_BIND,
				  NULL, 0,
				  brw->wm.surf_bo, brw->wm.nr_surfaces,
				  data, data_size);

      /* Emit binding table relocations to surface state */
      for (i = 0; i < BRW_WM_MAX_SURF; i++) {
	 if (brw->wm.surf_bo[i] != NULL) {
	    dri_bo_emit_reloc(bind_bo,
			      I915_GEM_DOMAIN_INSTRUCTION, 0,
			      0,
			      i * sizeof(GLuint),
			      brw->wm.surf_bo[i]);
	 }
      }
   }

   return bind_bo;
}

static void prepare_wm_surfaces(struct brw_context *brw )
{
   GLcontext *ctx = &brw->intel.ctx;
   GLuint i;
   int old_nr_surfaces;

   /* _NEW_BUFFERS | _NEW_COLOR */
   /* Update surfaces for drawing buffers */
   if (ctx->DrawBuffer->_NumColorDrawBuffers >= 1) {
      for (i = 0; i < ctx->DrawBuffer->_NumColorDrawBuffers; i++) {
         brw_update_renderbuffer_surface(brw,
					 ctx->DrawBuffer->_ColorDrawBuffers[i],
					 i);
      }
   } else {
      brw_update_renderbuffer_surface(brw, NULL, 0);
   }

   old_nr_surfaces = brw->wm.nr_surfaces;
   brw->wm.nr_surfaces = BRW_MAX_DRAW_BUFFERS;

   if (brw->wm.surf_bo[SURF_INDEX_FRAG_CONST_BUFFER] != NULL)
       brw->wm.nr_surfaces = SURF_INDEX_FRAG_CONST_BUFFER + 1;

   /* Update surfaces for textures */
   for (i = 0; i < BRW_MAX_TEX_UNIT; i++) {
      const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[i];
      const GLuint surf = SURF_INDEX_TEXTURE(i);

      /* _NEW_TEXTURE, BRW_NEW_TEXDATA */
      if (texUnit->_ReallyEnabled) {
	 brw_update_texture_surface(ctx, i);
	 brw->wm.nr_surfaces = surf + 1;
      } else {
         dri_bo_unreference(brw->wm.surf_bo[surf]);
         brw->wm.surf_bo[surf] = NULL;
      }
   }

   dri_bo_unreference(brw->wm.bind_bo);
   brw->wm.bind_bo = brw_wm_get_binding_table(brw);

   if (brw->wm.nr_surfaces != old_nr_surfaces)
      brw->state.dirty.brw |= BRW_NEW_NR_WM_SURFACES;
}

const struct brw_tracked_state brw_wm_surfaces = {
   .dirty = {
      .mesa = (_NEW_COLOR |
               _NEW_TEXTURE |
               _NEW_BUFFERS),
      .brw = (BRW_NEW_CONTEXT |
	      BRW_NEW_WM_SURFACES),
      .cache = 0
   },
   .prepare = prepare_wm_surfaces,
};



