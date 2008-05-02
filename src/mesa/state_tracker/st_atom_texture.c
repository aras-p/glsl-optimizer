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
#include "st_texture.h"
#include "st_cb_texture.h"
#include "pipe/p_context.h"
#include "pipe/p_inlines.h"
#include "cso_cache/cso_context.h"
#include "util/u_simple_shaders.h"


static void *
get_passthrough_fs(struct st_context *st)
{
   struct pipe_shader_state shader;

   if (!st->passthrough_fs) {
      st->passthrough_fs =
         util_make_fragment_passthrough_shader(st->pipe, &shader);
      free((void *) shader.tokens);
   }

   return st->passthrough_fs;
}


/**
 * XXX This needs some work yet....
 * Need to "upload" texture images at appropriate times.
 */
static void 
update_textures(struct st_context *st)
{
   struct gl_fragment_program *fprog = st->ctx->FragmentProgram._Current;
   GLuint su;
   GLboolean missing_textures = GL_FALSE;

   st->state.num_textures = 0;

   for (su = 0; su < st->ctx->Const.MaxTextureCoordUnits; su++) {
      struct pipe_texture *pt = NULL;

      if (fprog->Base.SamplersUsed & (1 << su)) {
         const GLuint texUnit = fprog->Base.SamplerUnits[su];
         struct gl_texture_object *texObj
            = st->ctx->Texture.Unit[texUnit]._Current;
         struct st_texture_object *stObj = st_texture_object(texObj);

         if (texObj) {
            GLboolean flush, retval;

            retval = st_finalize_texture(st->ctx, st->pipe, texObj, &flush);
            if (!retval) {
               /* out of mem */
               missing_textures = GL_TRUE;
               continue;
            }

            st->state.num_textures = su + 1;

            stObj->teximage_realloc = TRUE;
         }

         pt = st_get_stobj_texture(stObj);
      }

      pipe_texture_reference(&st->state.sampler_texture[su], pt);
   }

   cso_set_sampler_textures(st->cso_context,
                            st->state.num_textures,
                            st->state.sampler_texture);

   if (missing_textures) {
      /* use a pass-through frag shader that uses no textures */
      void *fs = get_passthrough_fs(st);
      cso_set_fragment_shader_handle(st->cso_context, fs);
   }
}


const struct st_tracked_state st_update_texture = {
   "st_update_texture",					/* name */
   {							/* dirty */
      _NEW_TEXTURE,					/* mesa */
      ST_NEW_FRAGMENT_PROGRAM,				/* st */
   },
   update_textures					/* update */
};
