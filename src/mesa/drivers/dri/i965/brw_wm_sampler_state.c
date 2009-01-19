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


static GLuint U_FIXED(GLfloat value, GLuint frac_bits)
{
   value *= (1<<frac_bits);
   return value < 0 ? 0 : value;
}

static GLint S_FIXED(GLfloat value, GLuint frac_bits)
{
   return value * (1<<frac_bits);
}


static dri_bo *upload_default_color( struct brw_context *brw,
				     const GLfloat *color )
{
   struct brw_sampler_default_color sdc;

   COPY_4V(sdc.color, color); 
   
   return brw_cache_data( &brw->cache, BRW_SAMPLER_DEFAULT_COLOR, &sdc,
			  NULL, 0 );
}


struct wm_sampler_key {
   int sampler_count;

   struct wm_sampler_entry {
      GLenum wrap_r, wrap_s, wrap_t;
      float maxlod, minlod;
      float lod_bias;
      float max_aniso;
      GLenum minfilter, magfilter;
      GLenum comparemode, comparefunc;
      dri_bo *sdc_bo;
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
   _mesa_memset(sampler, 0, sizeof(*sampler));

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
	 sampler->ss3.max_aniso = MAX2((key->max_aniso - 2) / 2,
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

   /* Fulsim complains if I don't do this.  Hardware doesn't mind:
    */
#if 0
   if (texObj->Target == GL_TEXTURE_CUBE_MAP_ARB) {
      sampler->ss1.r_wrap_mode = BRW_TEXCOORDMODE_CUBE;
      sampler->ss1.s_wrap_mode = BRW_TEXCOORDMODE_CUBE;
      sampler->ss1.t_wrap_mode = BRW_TEXCOORDMODE_CUBE;
   }
#endif

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

   sampler->ss1.max_lod = U_FIXED(MIN2(MAX2(key->maxlod, 0), 13), 6);
   sampler->ss1.min_lod = U_FIXED(MIN2(MAX2(key->minlod, 0), 13), 6);
   
   sampler->ss2.default_color_pointer = sdc_bo->offset >> 5; /* reloc */
}

/** Sets up the cache key for sampler state for all texture units */
static void
brw_wm_sampler_populate_key(struct brw_context *brw,
			    struct wm_sampler_key *key)
{
   int unit;

   memset(key, 0, sizeof(*key));

   for (unit = 0; unit < BRW_MAX_TEX_UNIT; unit++) {
      if (brw->attribs.Texture->Unit[unit]._ReallyEnabled) {
	 struct wm_sampler_entry *entry = &key->sampler[unit];
	 struct gl_texture_unit *texUnit = &brw->attribs.Texture->Unit[unit];
	 struct gl_texture_object *texObj = texUnit->_Current;
	 struct intel_texture_object *intelObj = intel_texture_object(texObj);
	 struct gl_texture_image *firstImage =
	    texObj->Image[0][intelObj->firstLevel];

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
	       texObj->BorderColor[0],
	       texObj->BorderColor[0],
	       texObj->BorderColor[0],
	       texObj->BorderColor[0]
	    };
	    /* GL specs that border color for depth textures is taken from the
	     * R channel, while the hardware uses A.  Spam R into all the
	     * channels for safety.
	     */
	    brw->wm.sdc_bo[unit] = upload_default_color(brw, bordercolor);
	 } else {
	    brw->wm.sdc_bo[unit] = upload_default_color(brw,
							texObj->BorderColor);
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
   struct wm_sampler_key key;
   int i;

   brw_wm_sampler_populate_key(brw, &key);

   if (brw->wm.sampler_count != key.sampler_count) {
      brw->wm.sampler_count = key.sampler_count;
      brw->state.dirty.cache |= CACHE_NEW_SAMPLER;
   }

   dri_bo_unreference(brw->wm.sampler_bo);
   brw->wm.sampler_bo = NULL;
   if (brw->wm.sampler_count == 0)
      return;

   brw->wm.sampler_bo = brw_search_cache(&brw->cache, BRW_SAMPLER,
					 &key, sizeof(key),
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
					    &key, sizeof(key),
					    brw->wm.sdc_bo, key.sampler_count,
					    &sampler, sizeof(sampler),
					    NULL, NULL);

      /* Emit SDC relocations */
      for (i = 0; i < BRW_MAX_TEX_UNIT; i++) {
	 if (!brw->attribs.Texture->Unit[i]._ReallyEnabled)
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


