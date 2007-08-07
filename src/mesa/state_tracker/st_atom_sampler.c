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
 

#include "st_context.h"
#include "st_atom.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"


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
      abort();
      return 0;
   }
}


static GLuint
gl_filter_to_sp(GLenum filter)
{
   switch (filter) {
   case GL_NEAREST:
      return PIPE_TEX_FILTER_NEAREST;
   case GL_LINEAR:
      return PIPE_TEX_FILTER_LINEAR;
   case GL_NEAREST_MIPMAP_NEAREST:
      return PIPE_TEX_FILTER_NEAREST_MIPMAP_NEAREST;
   case GL_NEAREST_MIPMAP_LINEAR:
      return PIPE_TEX_FILTER_NEAREST_MIPMAP_LINEAR;
   case GL_LINEAR_MIPMAP_NEAREST:
      return PIPE_TEX_FILTER_LINEAR_MIPMAP_NEAREST;
   case GL_LINEAR_MIPMAP_LINEAR:
      return PIPE_TEX_FILTER_LINEAR_MIPMAP_LINEAR;
   default:
      abort();
      return 0;
   }
}


static void 
update_samplers(struct st_context *st)
{
   GLuint u;

   for (u = 0; u < st->ctx->Const.MaxTextureImageUnits; u++) {
      const struct gl_texture_object *texobj
         = st->ctx->Texture.Unit[u]._Current;
      struct pipe_sampler_state sampler;

      memset(&sampler, 0, sizeof(sampler));

      if (texobj) {
         sampler.wrap_s = gl_wrap_to_sp(texobj->WrapS);
         sampler.wrap_t = gl_wrap_to_sp(texobj->WrapT);
         sampler.wrap_r = gl_wrap_to_sp(texobj->WrapR);

         sampler.min_filter = gl_filter_to_sp(texobj->MinFilter);
         sampler.mag_filter = gl_filter_to_sp(texobj->MagFilter);

         /* XXX more sampler state here */
      }

      if (memcmp(&sampler, &st->state.sampler[u], sizeof(sampler)) != 0) {
         /* state has changed */
         st->state.sampler[u] = sampler;
         st->pipe->set_sampler_state(st->pipe, u, &sampler);
      }
   }
}


const struct st_tracked_state st_update_sampler = {
   .dirty = {
      .mesa = _NEW_TEXTURE,
      .st  = 0,
   },
   .update = update_samplers
};





