/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  *   Brian Paul
  */
 

#include "main/macros.h"

#include "st_context.h"
#include "st_cb_texture.h"
#include "st_atom.h"
#include "st_program.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"

#include "cso_cache/cso_context.h"


/**
 * Convert GLenum texcoord wrap tokens to pipe tokens.
 */
static GLuint
gl_wrap_to_sp(GLenum wrap)
{
   switch (wrap) {
   case GL_REPEAT:
      return PIPE_TEX_WRAP_REPEAT;
   case GL_CLAMP:
      return PIPE_TEX_WRAP_CLAMP;
   case GL_CLAMP_TO_EDGE:
      return PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   case GL_CLAMP_TO_BORDER:
      return PIPE_TEX_WRAP_CLAMP_TO_BORDER;
   case GL_MIRRORED_REPEAT:
      return PIPE_TEX_WRAP_MIRROR_REPEAT;
   case GL_MIRROR_CLAMP_EXT:
      return PIPE_TEX_WRAP_MIRROR_CLAMP;
   case GL_MIRROR_CLAMP_TO_EDGE_EXT:
      return PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE;
   case GL_MIRROR_CLAMP_TO_BORDER_EXT:
      return PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER;
   default:
      assert(0);
      return 0;
   }
}


static GLuint
gl_filter_to_mip_filter(GLenum filter)
{
   switch (filter) {
   case GL_NEAREST:
   case GL_LINEAR:
      return PIPE_TEX_MIPFILTER_NONE;

   case GL_NEAREST_MIPMAP_NEAREST:
   case GL_LINEAR_MIPMAP_NEAREST:
      return PIPE_TEX_MIPFILTER_NEAREST;

   case GL_NEAREST_MIPMAP_LINEAR:
   case GL_LINEAR_MIPMAP_LINEAR:
      return PIPE_TEX_MIPFILTER_LINEAR;

   default:
      assert(0);
      return PIPE_TEX_MIPFILTER_NONE;
   }
}


static GLuint
gl_filter_to_img_filter(GLenum filter)
{
   switch (filter) {
   case GL_NEAREST:
   case GL_NEAREST_MIPMAP_NEAREST:
   case GL_NEAREST_MIPMAP_LINEAR:
      return PIPE_TEX_FILTER_NEAREST;

   case GL_LINEAR:
   case GL_LINEAR_MIPMAP_NEAREST:
   case GL_LINEAR_MIPMAP_LINEAR:
      return PIPE_TEX_FILTER_LINEAR;

   default:
      assert(0);
      return PIPE_TEX_FILTER_NEAREST;
   }
}


static void 
update_samplers(struct st_context *st)
{
   const struct st_fragment_program *fs = st->fp;
   GLuint su;

   st->state.num_samplers = 0;

   /*printf("%s samplers used = 0x%x\n", __FUNCTION__, fs->Base.Base.SamplersUsed);*/

   /* loop over sampler units (aka tex image units) */
   for (su = 0; su < st->ctx->Const.MaxTextureImageUnits; su++) {
      struct pipe_sampler_state *sampler = st->state.samplers + su;

      memset(sampler, 0, sizeof(*sampler));

      if (fs->Base.Base.SamplersUsed & (1 << su)) {
         GLuint texUnit = fs->Base.Base.SamplerUnits[su];
         const struct gl_texture_object *texobj
            = st->ctx->Texture.Unit[texUnit]._Current;

         if (!texobj) {
            texobj = st_get_default_texture(st);
         }

         sampler->wrap_s = gl_wrap_to_sp(texobj->WrapS);
         sampler->wrap_t = gl_wrap_to_sp(texobj->WrapT);
         sampler->wrap_r = gl_wrap_to_sp(texobj->WrapR);

         sampler->min_img_filter = gl_filter_to_img_filter(texobj->MinFilter);
         sampler->min_mip_filter = gl_filter_to_mip_filter(texobj->MinFilter);
         sampler->mag_img_filter = gl_filter_to_img_filter(texobj->MagFilter);

         if (texobj->Target != GL_TEXTURE_RECTANGLE_ARB)
            sampler->normalized_coords = 1;

         sampler->lod_bias = st->ctx->Texture.Unit[su].LodBias;
         sampler->min_lod = MAX2(0.0f, texobj->MinLod);
         sampler->max_lod = MIN2(texobj->MaxLevel - texobj->BaseLevel,
                                 texobj->MaxLod);
         if (sampler->max_lod < sampler->min_lod) {
            /* The GL spec doesn't seem to specify what to do in this case.
             * Swap the values.
             */
            float tmp = sampler->max_lod;
            sampler->max_lod = sampler->min_lod;
            sampler->min_lod = tmp;
            assert(sampler->min_lod <= sampler->max_lod);
         }

         sampler->border_color[0] = texobj->BorderColor[RCOMP];
         sampler->border_color[1] = texobj->BorderColor[GCOMP];
         sampler->border_color[2] = texobj->BorderColor[BCOMP];
         sampler->border_color[3] = texobj->BorderColor[ACOMP];

	 sampler->max_anisotropy = texobj->MaxAnisotropy;
         if (sampler->max_anisotropy > 1.0) {
            sampler->min_img_filter = PIPE_TEX_FILTER_ANISO;
            sampler->mag_img_filter = PIPE_TEX_FILTER_ANISO;
         }

         /* only care about ARB_shadow, not SGI shadow */
         if (texobj->CompareMode == GL_COMPARE_R_TO_TEXTURE) {
            sampler->compare_mode = PIPE_TEX_COMPARE_R_TO_TEXTURE;
            sampler->compare_func
               = st_compare_func_to_pipe(texobj->CompareFunc);
         }

         st->state.num_samplers = su + 1;

         /*printf("%s su=%u non-null\n", __FUNCTION__, su);*/
         cso_single_sampler(st->cso_context, su, sampler);
      }
      else {
         /*printf("%s su=%u null\n", __FUNCTION__, su);*/
         cso_single_sampler(st->cso_context, su, NULL);
      }
   }

   cso_single_sampler_done(st->cso_context);
}


const struct st_tracked_state st_update_sampler = {
   "st_update_sampler",					/* name */
   {							/* dirty */
      _NEW_TEXTURE,					/* mesa */
      0,						/* st */
   },
   update_samplers					/* update */
};
