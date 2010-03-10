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
                   

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"

#include "main/macros.h"



/* Samplers aren't strictly wm state from the hardware's perspective,
 * but that is the only situation in which we use them in this driver.
 */



/* The brw (and related graphics cores) do not support GL_CLAMP.  The
 * Intel drivers for "other operating systems" implement GL_CLAMP as
 * GL_CLAMP_TO_EDGE, so the same is done here.
 */
static GLuint translate_wrap_mode( GLenum wrap )
{
   switch( wrap ) {
   case GL_REPEAT: 
      return BRW_TEXCOORDMODE_WRAP;
   case GL_CLAMP:  
      return BRW_TEXCOORDMODE_CLAMP;
   case GL_CLAMP_TO_EDGE: 
      return BRW_TEXCOORDMODE_CLAMP; /* conform likes it this way */
   case GL_CLAMP_TO_BORDER: 
      return BRW_TEXCOORDMODE_CLAMP_BORDER;
   case GL_MIRRORED_REPEAT: 
      return BRW_TEXCOORDMODE_MIRROR;
   default: 
      return BRW_TEXCOORDMODE_WRAP;
   }
}

static dri_bo *upload_default_color( struct brw_context *brw,
				     const GLfloat *color )
{
   struct brw_sampler_default_color sdc;

   COPY_4V(sdc.color, color); 
   
   return brw_cache_data(&brw->cache, BRW_SAMPLER_DEFAULT_COLOR,
			 &sdc, sizeof(sdc), NULL, 0);
}


struct wm_sampler_key {
   int sampler_count;

   struct wm_sampler_entry {
      GLenum tex_target;
      GLenum wrap_r, wrap_s, wrap_t;
      float maxlod, minlod;
      float lod_bias;
      float max_aniso;
      GLenum minfilter, magfilter;
      GLenum comparemode, comparefunc;

      /** If target is cubemap, take context setting.
       */
      GLboolean seamless_cube_map;
   } sampler[BRW_MAX_TEX_UNIT];
};

/**
 * Sets the sampler state for a single unit based off of the sampler key
 * entry.
 */
static void brw_update_sampler_state(struct wm_sampler_entry *key,
				     dri_bo *sdc_bo,
				     struct brw_sampler_state *sampler)
{
   memset(sampler, 0, sizeof(*sampler));

   switch (key->minfilter) {
   case GL_NEAREST:
      sampler->ss0.min_filter = BRW_MAPFILTER_NEAREST;
      sampler->ss0.mip_filter = BRW_MIPFILTER_NONE;
      break;
   case GL_LINEAR:
      sampler->ss0.min_filter = BRW_MAPFILTER_LINEAR;
      sampler->ss0.mip_filter = BRW_MIPFILTER_NONE;
      break;
   case GL_NEAREST_MIPMAP_NEAREST:
      sampler->ss0.min_filter = BRW_MAPFILTER_NEAREST;
      sampler->ss0.mip_filter = BRW_MIPFILTER_NEAREST;
      break;
   case GL_LINEAR_MIPMAP_NEAREST:
      sampler->ss0.min_filter = BRW_MAPFILTER_LINEAR;
      sampler->ss0.mip_filter = BRW_MIPFILTER_NEAREST;
      break;
   case GL_NEAREST_MIPMAP_LINEAR:
      sampler->ss0.min_filter = BRW_MAPFILTER_NEAREST;
      sampler->ss0.mip_filter = BRW_MIPFILTER_LINEAR;
      break;
   case GL_LINEAR_MIPMAP_LINEAR:
      sampler->ss0.min_filter = BRW_MAPFILTER_LINEAR;
      sampler->ss0.mip_filter = BRW_MIPFILTER_LINEAR;
      break;
   default:
      break;
   }

   /* Set Anisotropy: 
    */
   if (key->max_aniso > 1.0) {
      sampler->ss0.min_filter = BRW_MAPFILTER_ANISOTROPIC; 
      sampler->ss0.mag_filter = BRW_MAPFILTER_ANISOTROPIC;

      if (key->max_aniso > 2.0) {
	 sampler->ss3.max_aniso = MIN2((key->max_aniso - 2) / 2,
				       BRW_ANISORATIO_16);
      }
   }
   else {
      switch (key->magfilter) {
      case GL_NEAREST:
	 sampler->ss0.mag_filter = BRW_MAPFILTER_NEAREST;
	 break;
      case GL_LINEAR:
	 sampler->ss0.mag_filter = BRW_MAPFILTER_LINEAR;
	 break;
      default:
	 break;
      }  
   }

   sampler->ss1.r_wrap_mode = translate_wrap_mode(key->wrap_r);
   sampler->ss1.s_wrap_mode = translate_wrap_mode(key->wrap_s);
   sampler->ss1.t_wrap_mode = translate_wrap_mode(key->wrap_t);

   /* Cube-maps on 965 and later must use the same wrap mode for all 3
    * coordinate dimensions.  Futher, only CUBE and CLAMP are valid.
    */
   if (key->tex_target == GL_TEXTURE_CUBE_MAP) {
      if (key->seamless_cube_map &&
	  (key->minfilter != GL_NEAREST || key->magfilter != GL_NEAREST)) {
	 sampler->ss1.r_wrap_mode = BRW_TEXCOORDMODE_CUBE;
	 sampler->ss1.s_wrap_mode = BRW_TEXCOORDMODE_CUBE;
	 sampler->ss1.t_wrap_mode = BRW_TEXCOORDMODE_CUBE;
      } else {
	 sampler->ss1.r_wrap_mode = BRW_TEXCOORDMODE_CLAMP;
	 sampler->ss1.s_wrap_mode = BRW_TEXCOORDMODE_CLAMP;
	 sampler->ss1.t_wrap_mode = BRW_TEXCOORDMODE_CLAMP;
      }
   } else if (key->tex_target == GL_TEXTURE_1D) {
      /* There's a bug in 1D texture sampling - it actually pays
       * attention to the wrap_t value, though it should not.
       * Override the wrap_t value here to GL_REPEAT to keep
       * any nonexistent border pixels from floating in.
       */
      sampler->ss1.t_wrap_mode = BRW_TEXCOORDMODE_WRAP;
   }


   /* Set shadow function: 
    */
   if (key->comparemode == GL_COMPARE_R_TO_TEXTURE_ARB) {
      /* Shadowing is "enabled" by emitting a particular sampler
       * message (sample_c).  So need to recompile WM program when
       * shadow comparison is enabled on each/any texture unit.
       */
      sampler->ss0.shadow_function =
	 intel_translate_shadow_compare_func(key->comparefunc);
   }

   /* Set LOD bias: 
    */
   sampler->ss0.lod_bias = S_FIXED(CLAMP(key->lod_bias, -16, 15), 6);

   sampler->ss0.lod_preclamp = 1; /* OpenGL mode */
   sampler->ss0.default_color_mode = 0; /* OpenGL/DX10 mode */

   /* Set BaseMipLevel, MaxLOD, MinLOD: 
    *
    * XXX: I don't think that using firstLevel, lastLevel works,
    * because we always setup the surface state as if firstLevel ==
    * level zero.  Probably have to subtract firstLevel from each of
    * these:
    */
   sampler->ss0.base_level = U_FIXED(0, 1);

   sampler->ss1.max_lod = U_FIXED(CLAMP(key->maxlod, 0, 13), 6);
   sampler->ss1.min_lod = U_FIXED(CLAMP(key->minlod, 0, 13), 6);
   
   sampler->ss2.default_color_pointer = sdc_bo->offset >> 5; /* reloc */
}


/** Sets up the cache key for sampler state for all texture units */
static void
brw_wm_sampler_populate_key(struct brw_context *brw,
			    struct wm_sampler_key *key)
{
   GLcontext *ctx = &brw->intel.ctx;
   int unit;

   key->sampler_count = 0;

   for (unit = 0; unit < BRW_MAX_TEX_UNIT; unit++) {
      if (ctx->Texture.Unit[unit]._ReallyEnabled) {
	 struct wm_sampler_entry *entry = &key->sampler[unit];
	 struct gl_texture_unit *texUnit = &ctx->Texture.Unit[unit];
	 struct gl_texture_object *texObj = texUnit->_Current;
	 struct intel_texture_object *intelObj = intel_texture_object(texObj);
	 struct gl_texture_image *firstImage =
	    texObj->Image[0][intelObj->firstLevel];

	 memset(entry, 0, sizeof(*entry));

         entry->tex_target = texObj->Target;

	 entry->seamless_cube_map = (texObj->Target == GL_TEXTURE_CUBE_MAP)
	    ? ctx->Texture.CubeMapSeamless : GL_FALSE;

	 entry->wrap_r = texObj->WrapR;
	 entry->wrap_s = texObj->WrapS;
	 entry->wrap_t = texObj->WrapT;

	 entry->maxlod = texObj->MaxLod;
	 entry->minlod = texObj->MinLod;
	 entry->lod_bias = texUnit->LodBias + texObj->LodBias;
	 entry->max_aniso = texObj->MaxAnisotropy;
	 entry->minfilter = texObj->MinFilter;
	 entry->magfilter = texObj->MagFilter;
	 entry->comparemode = texObj->CompareMode;
         entry->comparefunc = texObj->CompareFunc;

	 dri_bo_unreference(brw->wm.sdc_bo[unit]);
	 if (firstImage->_BaseFormat == GL_DEPTH_COMPONENT) {
	    float bordercolor[4] = {
	       texObj->BorderColor.f[0],
	       texObj->BorderColor.f[0],
	       texObj->BorderColor.f[0],
	       texObj->BorderColor.f[0]
	    };
	    /* GL specs that border color for depth textures is taken from the
	     * R channel, while the hardware uses A.  Spam R into all the
	     * channels for safety.
	     */
	    brw->wm.sdc_bo[unit] = upload_default_color(brw, bordercolor);
	 } else {
	    brw->wm.sdc_bo[unit] = upload_default_color(brw,
							texObj->BorderColor.f);
	 }
	 key->sampler_count = unit + 1;
      }
   }
}

/* All samplers must be uploaded in a single contiguous array, which
 * complicates various things.  However, this is still too confusing -
 * FIXME: simplify all the different new texture state flags.
 */
static void upload_wm_samplers( struct brw_context *brw )
{
   GLcontext *ctx = &brw->intel.ctx;
   struct wm_sampler_key key;
   int i, sampler_key_size;

   brw_wm_sampler_populate_key(brw, &key);

   if (brw->wm.sampler_count != key.sampler_count) {
      brw->wm.sampler_count = key.sampler_count;
      brw->state.dirty.cache |= CACHE_NEW_SAMPLER;
   }

   dri_bo_unreference(brw->wm.sampler_bo);
   brw->wm.sampler_bo = NULL;
   if (brw->wm.sampler_count == 0)
      return;

   /* Only include the populated portion of the key in the search. */
   sampler_key_size = offsetof(struct wm_sampler_key,
			       sampler[key.sampler_count]);
   brw->wm.sampler_bo = brw_search_cache(&brw->cache, BRW_SAMPLER,
					 &key, sampler_key_size,
					 brw->wm.sdc_bo, key.sampler_count,
					 NULL);

   /* If we didnt find it in the cache, compute the state and put it in the
    * cache.
    */
   if (brw->wm.sampler_bo == NULL) {
      struct brw_sampler_state sampler[BRW_MAX_TEX_UNIT];

      memset(sampler, 0, sizeof(sampler));
      for (i = 0; i < key.sampler_count; i++) {
	 if (brw->wm.sdc_bo[i] == NULL)
	    continue;

	 brw_update_sampler_state(&key.sampler[i], brw->wm.sdc_bo[i],
				  &sampler[i]);
      }

      brw->wm.sampler_bo = brw_upload_cache(&brw->cache, BRW_SAMPLER,
					    &key, sampler_key_size,
					    brw->wm.sdc_bo, key.sampler_count,
					    &sampler, sizeof(sampler));

      /* Emit SDC relocations */
      for (i = 0; i < BRW_MAX_TEX_UNIT; i++) {
	 if (!ctx->Texture.Unit[i]._ReallyEnabled)
	    continue;

	 dri_bo_emit_reloc(brw->wm.sampler_bo,
			   I915_GEM_DOMAIN_SAMPLER, 0,
			   0,
			   i * sizeof(struct brw_sampler_state) +
			   offsetof(struct brw_sampler_state, ss2),
			   brw->wm.sdc_bo[i]);
      }
   }
}

const struct brw_tracked_state brw_wm_samplers = {
   .dirty = {
      .mesa = _NEW_TEXTURE,
      .brw = 0,
      .cache = 0
   },
   .prepare = upload_wm_samplers,
};


