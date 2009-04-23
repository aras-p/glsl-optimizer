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
#include "main/texformat.h"
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


static GLuint translate_tex_format( GLuint mesa_format, GLenum internal_format,
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

   case MESA_FORMAT_RGB888:
      assert(0);		/* not supported for sampling */
      return BRW_SURFACEFORMAT_R8G8B8_UNORM;      

   case MESA_FORMAT_ARGB8888:
      if (internal_format == GL_RGB)
	 return BRW_SURFACEFORMAT_B8G8R8X8_UNORM;
      else
	 return BRW_SURFACEFORMAT_B8G8R8A8_UNORM;

   case MESA_FORMAT_RGBA8888_REV:
      if (internal_format == GL_RGB)
	 return BRW_SURFACEFORMAT_R8G8B8X8_UNORM;
      else
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


/**
 * Use same key for WM and VS surfaces.
 */
struct brw_surface_key {
   GLenum target, depthmode;
   dri_bo *bo;
   GLint format, internal_format;
   GLint first_level, last_level;
   GLint width, height, depth;
   GLint pitch, cpp;
   uint32_t tiling;
   GLuint offset;
};


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
   if (key->bo) {
      surf.ss0.surface_format = translate_tex_format(key->format,
						     key->internal_format,
						     key->depthmode);
   }
   else {
      switch (key->depth) {
      case 32:
         surf.ss0.surface_format = BRW_SURFACEFORMAT_B8G8R8A8_UNORM;
         break;
      default:
      case 24:
         surf.ss0.surface_format = BRW_SURFACEFORMAT_B8G8R8X8_UNORM;
         break;
      case 16:
         surf.ss0.surface_format = BRW_SURFACEFORMAT_B5G6R5_UNORM;
         break;
      }
   }

   /* This is ok for all textures with channel width 8bit or less:
    */
/*    surf.ss0.data_return_format = BRW_SURFACERETURNFORMAT_S1; */
   if (key->bo)
      surf.ss1.base_addr = key->bo->offset; /* reloc */
   else
      surf.ss1.base_addr = key->offset;

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

   bo = brw_upload_cache(&brw->cache, BRW_SS_SURFACE,
			 key, sizeof(*key),
			 &key->bo, key->bo ? 1 : 0,
			 &surf, sizeof(surf),
			 NULL, NULL);

   if (key->bo) {
      /* Emit relocation to surface contents */
      dri_bo_emit_reloc(bo,
			I915_GEM_DOMAIN_SAMPLER, 0,
			0,
			offsetof(struct brw_surface_state, ss1),
			key->bo);
   }
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

   if (intelObj->imageOverride) {
      key.pitch = intelObj->pitchOverride / intelObj->mt->cpp;
      key.depth = intelObj->depthOverride;
      key.bo = NULL;
      key.offset = intelObj->textureOffset;
   } else {
      key.format = firstImage->TexFormat->MesaFormat;
      key.internal_format = firstImage->InternalFormat;
      key.pitch = intelObj->mt->pitch;
      key.depth = firstImage->Depth;
      key.bo = intelObj->mt->region->buffer;
      key.offset = 0;
   }

   key.target = tObj->Target;
   key.depthmode = tObj->DepthMode;
   key.first_level = intelObj->firstLevel;
   key.last_level = intelObj->lastLevel;
   key.width = firstImage->Width;
   key.height = firstImage->Height;
   key.cpp = intelObj->mt->cpp;
   key.tiling = intelObj->mt->region->tiling;

   dri_bo_unreference(brw->wm.surf_bo[surf]);
   brw->wm.surf_bo[surf] = brw_search_cache(&brw->cache, BRW_SS_SURFACE,
                                         &key, sizeof(key),
                                         &key.bo, key.bo ? 1 : 0,
                                         NULL);
   if (brw->wm.surf_bo[surf] == NULL) {
      brw->wm.surf_bo[surf] = brw_create_texture_surface(brw, &key);
   }
}



/**
 * Create the constant buffer surface.  Vertex/fragment shader constants will be
 * read from this buffer with Data Port Read instructions/messages.
 */
static dri_bo *
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
   if (key->bo)
      surf.ss1.base_addr = key->bo->offset; /* reloc */
   else
      surf.ss1.base_addr = key->offset;

   surf.ss2.width = w & 0x7f;            /* bits 6:0 of size or width */
   surf.ss2.height = (w >> 7) & 0x1fff;  /* bits 19:7 of size or width */
   surf.ss3.depth = (w >> 20) & 0x7f;    /* bits 26:20 of size or width */
   surf.ss3.pitch = (key->pitch * key->cpp) - 1; /* ignored?? */
   brw_set_surface_tiling(&surf, key->tiling); /* tiling now allowed */
 
   bo = brw_upload_cache(&brw->cache, BRW_SS_SURFACE,
			 key, sizeof(*key),
			 &key->bo, key->bo ? 1 : 0,
			 &surf, sizeof(surf),
			 NULL, NULL);

   if (key->bo) {
      /* Emit relocation to surface contents */
      dri_bo_emit_reloc(bo,
			I915_GEM_DOMAIN_SAMPLER, 0,
			0,
			offsetof(struct brw_surface_state, ss1),
			key->bo);
   }

   return bo;
}


/**
 * Update the surface state for a WM constant buffer.
 * The constant buffer will be (re)allocated here if needed.
 */
static dri_bo *
brw_update_wm_constant_surface( GLcontext *ctx,
                                GLuint surf,
                                dri_bo *const_buffer,
                                const struct gl_program_parameter_list *params)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_surface_key key;
   struct intel_context *intel = &brw->intel;
   const int size = params->NumParameters * 4 * sizeof(GLfloat);

   /* free old const buffer if too small */
   if (const_buffer && const_buffer->size < size) {
      dri_bo_unreference(const_buffer);
      const_buffer = NULL;
   }

   /* alloc new buffer if needed */
   if (!const_buffer) {
      const_buffer =
         drm_intel_bo_alloc(intel->bufmgr, "fp_const_buffer", size, 64);
   }

   memset(&key, 0, sizeof(key));

   key.format = MESA_FORMAT_RGBA_FLOAT32;
   key.internal_format = GL_RGBA;
   key.bo = const_buffer;
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
   brw->wm.surf_bo[surf] = brw_search_cache(&brw->cache, BRW_SS_SURFACE,
                                            &key, sizeof(key),
                                            &key.bo, key.bo ? 1 : 0,
                                            NULL);
   if (brw->wm.surf_bo[surf] == NULL) {
      brw->wm.surf_bo[surf] = brw_create_constant_surface(brw, &key);
   }

   return const_buffer;
}


/**
 * Update the surface state for a VS constant buffer.
 * The constant buffer will be (re)allocated here if needed.
 */
static dri_bo *
brw_update_vs_constant_surface( GLcontext *ctx,
                                GLuint surf,
                                dri_bo *const_buffer,
                                const struct gl_program_parameter_list *params)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_surface_key key;
   struct intel_context *intel = &brw->intel;
   const int size = params->NumParameters * 4 * sizeof(GLfloat);

   assert(surf == 0);

   /* free old const buffer if too small */
   if (const_buffer && const_buffer->size < size) {
      dri_bo_unreference(const_buffer);
      const_buffer = NULL;
   }

   /* alloc new buffer if needed */
   if (!const_buffer) {
      const_buffer =
         drm_intel_bo_alloc(intel->bufmgr, "vp_const_buffer", size, 64);
   }

   memset(&key, 0, sizeof(key));

   key.format = MESA_FORMAT_RGBA_FLOAT32;
   key.internal_format = GL_RGBA;
   key.bo = const_buffer;
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

   dri_bo_unreference(brw->vs.surf_bo[surf]);
   brw->vs.surf_bo[surf] = brw_search_cache(&brw->cache, BRW_SS_SURFACE,
                                            &key, sizeof(key),
                                            &key.bo, key.bo ? 1 : 0,
                                            NULL);
   if (brw->vs.surf_bo[surf] == NULL) {
      brw->vs.surf_bo[surf] = brw_create_constant_surface(brw, &key);
   }

   return const_buffer;
}


/**
 * Sets up a surface state structure to point at the given region.
 * While it is only used for the front/back buffer currently, it should be
 * usable for further buffers when doing ARB_draw_buffer support.
 */
static void
brw_update_renderbuffer_surface(struct brw_context *brw,
				struct gl_renderbuffer *rb,
				unsigned int unit, GLboolean cached)
{
   GLcontext *ctx = &brw->intel.ctx;
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
      uint32_t draw_offset;
   } key;

   memset(&key, 0, sizeof(key));

   if (region != NULL) {
      region_bo = region->buffer;

      key.surface_type = BRW_SURFACE_2D;
      switch (irb->texformat->MesaFormat) {
      case MESA_FORMAT_ARGB8888:
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
	 _mesa_problem(ctx, "Bad renderbuffer format: %d\n",
		       irb->texformat->MesaFormat);
      }
      key.tiling = region->tiling;
      key.width = region->width;
      key.height = region->height;
      key.pitch = region->pitch;
      key.cpp = region->cpp;
      key.draw_offset = region->draw_offset; /* cur 3d or cube face offset */
   } else {
      key.surface_type = BRW_SURFACE_NULL;
      key.surface_format = BRW_SURFACEFORMAT_B8G8R8A8_UNORM;
      key.tiling = 0;
      key.width = 1;
      key.height = 1;
      key.cpp = 4;
      key.draw_offset = 0;
   }
   memcpy(key.color_mask, ctx->Color.ColorMask,
	  sizeof(key.color_mask));
   key.color_blend = (!ctx->Color._LogicOpEnabled &&
		      ctx->Color.BlendEnabled);

   dri_bo_unreference(brw->wm.surf_bo[unit]);
   brw->wm.surf_bo[unit] = NULL;
   if (cached) 
       brw->wm.surf_bo[unit] = brw_search_cache(&brw->cache, BRW_SS_SURFACE,
	       &key, sizeof(key),
	       &region_bo, 1,
	       NULL);

   if (brw->wm.surf_bo[unit] == NULL) {
      struct brw_surface_state surf;

      memset(&surf, 0, sizeof(surf));

      surf.ss0.surface_format = key.surface_format;
      surf.ss0.surface_type = key.surface_type;
      surf.ss1.base_addr =  key.draw_offset;
      if (region_bo != NULL)
	 surf.ss1.base_addr += region_bo->offset; /* reloc */

      surf.ss2.width = key.width - 1;
      surf.ss2.height = key.height - 1;
      brw_set_surface_tiling(&surf, key.tiling);
      surf.ss3.pitch = (key.pitch * key.cpp) - 1;

      /* _NEW_COLOR */
      surf.ss0.color_blend = key.color_blend;
      surf.ss0.writedisable_red =   !key.color_mask[0];
      surf.ss0.writedisable_green = !key.color_mask[1];
      surf.ss0.writedisable_blue =  !key.color_mask[2];
      surf.ss0.writedisable_alpha = !key.color_mask[3];

      /* Key size will never match key size for textures, so we're safe. */
      brw->wm.surf_bo[unit] = brw_upload_cache(&brw->cache, BRW_SS_SURFACE,
                                               &key, sizeof(key),
					       &region_bo, 1,
					       &surf, sizeof(surf),
					       NULL, NULL);
      if (region_bo != NULL) {
	 /* We might sample from it, and we might render to it, so flag
	  * them both.  We might be able to figure out from other state
	  * a more restrictive relocation to emit.
	  */
	 drm_intel_bo_emit_reloc(brw->wm.surf_bo[unit],
				 offsetof(struct brw_surface_state, ss1),
				 region_bo,
				 key.draw_offset,
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

   bind_bo = brw_search_cache(&brw->cache, BRW_SS_SURF_BIND,
			      NULL, 0,
			      brw->wm.surf_bo, brw->wm.nr_surfaces,
			      NULL);

   if (bind_bo == NULL) {
      GLuint data_size = brw->wm.nr_surfaces * sizeof(GLuint);
      uint32_t *data = malloc(data_size);
      int i;

      for (i = 0; i < brw->wm.nr_surfaces; i++)
         if (brw->wm.surf_bo[i])
            data[i] = brw->wm.surf_bo[i]->offset;
         else
            data[i] = 0;

      bind_bo = brw_upload_cache( &brw->cache, BRW_SS_SURF_BIND,
				  NULL, 0,
				  brw->wm.surf_bo, brw->wm.nr_surfaces,
				  data, data_size,
				  NULL, NULL);

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

      free(data);
   }

   return bind_bo;
}

static void prepare_wm_surfaces(struct brw_context *brw )
{
   GLcontext *ctx = &brw->intel.ctx;
   struct intel_context *intel = &brw->intel;
   GLuint i;
   int old_nr_surfaces;

   /* _NEW_BUFFERS */
   /* Update surfaces for drawing buffers */
   if (ctx->DrawBuffer->_NumColorDrawBuffers >= 1) {
      for (i = 0; i < ctx->DrawBuffer->_NumColorDrawBuffers; i++) {
         brw_update_renderbuffer_surface(brw,
					 ctx->DrawBuffer->_ColorDrawBuffers[i],
					 i,
					 GL_FALSE);
      }
   } else {
      brw_update_renderbuffer_surface(brw, NULL, 0, GL_TRUE);
   }

   old_nr_surfaces = brw->wm.nr_surfaces;
   brw->wm.nr_surfaces = MAX_DRAW_BUFFERS;

   /* Update surface / buffer for fragment shader constant buffer */
   {
      const GLuint surf = SURF_INDEX_FRAG_CONST_BUFFER;
      struct brw_fragment_program *fp =
         (struct brw_fragment_program *) brw->fragment_program;
      fp->const_buffer =
         brw_update_wm_constant_surface(ctx, surf, fp->const_buffer,
                                     fp->program.Base.Parameters);

      brw->wm.nr_surfaces = surf + 1;
   }

   /* Update surfaces for textures */
   for (i = 0; i < BRW_MAX_TEX_UNIT; i++) {
      const struct gl_texture_unit *texUnit = &ctx->Texture.Unit[i];
      const GLuint surf = SURF_INDEX_TEXTURE(i);

      /* _NEW_TEXTURE, BRW_NEW_TEXDATA */
      if (texUnit->_ReallyEnabled) {
         if (texUnit->_Current == intel->frame_buffer_texobj) {
            /* render to texture */
            dri_bo_unreference(brw->wm.surf_bo[surf]);
            brw->wm.surf_bo[surf] = brw->wm.surf_bo[0];
            dri_bo_reference(brw->wm.surf_bo[surf]);
            brw->wm.nr_surfaces = surf + 1;
         } else {
            /* regular texture */
            brw_update_texture_surface(ctx, i);
            brw->wm.nr_surfaces = surf + 1;
         }
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


/**
 * Constructs the binding table for the VS surface state.
 */
static dri_bo *
brw_vs_get_binding_table(struct brw_context *brw)
{
   dri_bo *bind_bo;

   assert(brw->vs.nr_surfaces <= BRW_VS_MAX_SURF);

   bind_bo = brw_search_cache(&brw->cache, BRW_SS_SURF_BIND,
			      NULL, 0,
			      brw->vs.surf_bo, brw->vs.nr_surfaces,
			      NULL);

   if (bind_bo == NULL) {
      GLuint data_size = brw->vs.nr_surfaces * sizeof(GLuint);
      uint32_t *data = malloc(data_size);
      int i;

      for (i = 0; i < brw->vs.nr_surfaces; i++)
         if (brw->vs.surf_bo[i])
            data[i] = brw->vs.surf_bo[i]->offset;
         else
            data[i] = 0;

      bind_bo = brw_upload_cache( &brw->cache, BRW_SS_SURF_BIND,
				  NULL, 0,
				  brw->vs.surf_bo, brw->vs.nr_surfaces,
				  data, data_size,
				  NULL, NULL);

      /* Emit binding table relocations to surface state */
      for (i = 0; i < BRW_VS_MAX_SURF; i++) {
	 if (brw->vs.surf_bo[i] != NULL) {
	    dri_bo_emit_reloc(bind_bo,
			      I915_GEM_DOMAIN_INSTRUCTION, 0,
			      0,
			      i * sizeof(GLuint),
			      brw->vs.surf_bo[i]);
	 }
      }

      free(data);
   }

   return bind_bo;
}


/**
 * Vertex shader surfaces.  Just constant buffer for now.  Could add vertex 
 * shader textures in the future.
 */
static void prepare_vs_surfaces(struct brw_context *brw )
{
   GLcontext *ctx = &brw->intel.ctx;

   /* Update surface / buffer for vertex shader constant buffer */
   {
      const GLuint surf = SURF_INDEX_VERT_CONST_BUFFER;
      struct brw_vertex_program *vp =
         (struct brw_vertex_program *) brw->vertex_program;
      vp->const_buffer =
         brw_update_vs_constant_surface(ctx, surf, vp->const_buffer,
                                        vp->program.Base.Parameters);

      brw->vs.nr_surfaces = 1;
   }

   dri_bo_unreference(brw->vs.bind_bo);
   brw->vs.bind_bo = brw_vs_get_binding_table(brw);

   if (1)
      brw->state.dirty.brw |= BRW_NEW_NR_VS_SURFACES;
}


static void
prepare_surfaces(struct brw_context *brw)
{
   prepare_wm_surfaces(brw);
   prepare_vs_surfaces(brw);
}


const struct brw_tracked_state brw_wm_surfaces = {
   .dirty = {
      .mesa = _NEW_COLOR | _NEW_TEXTURE | _NEW_BUFFERS | _NEW_PROGRAM,
      .brw = BRW_NEW_CONTEXT,
      .cache = 0
   },
   .prepare = prepare_surfaces,
};



