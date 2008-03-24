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


/**
 * XXX This needs some work yet....
 * Need to "upload" texture images at appropriate times.
 */
static void 
update_textures(struct st_context *st)
{
   /* ST_NEW_FRAGMENT_PROGRAM
    */
   struct gl_fragment_program *fprog = st->ctx->FragmentProgram._Current;
   GLuint unit;

   st->state.num_textures = 0;

   for (unit = 0; unit < st->ctx->Const.MaxTextureCoordUnits; unit++) {
      const GLuint su = fprog->Base.SamplerUnits[unit];
      struct pipe_texture *pt = NULL;

      if (fprog->Base.SamplersUsed & (1 << su)) {
         struct gl_texture_object *texObj = st->ctx->Texture.Unit[su]._Current;
         struct st_texture_object *stObj = st_texture_object(texObj);

         if (texObj) {
            GLboolean flush, retval;

            retval = st_finalize_texture(st->ctx, st->pipe, texObj, &flush);
            if (!retval) {
               /* out of mem */
               continue;
            }

            st->state.num_textures = unit + 1;
         }

         pt = st_get_stobj_texture(stObj);
      }

      pipe_texture_reference(&st->state.sampler_texture[unit], pt);
   }

   st->pipe->set_sampler_textures(st->pipe, st->state.num_textures,
                                  st->state.sampler_texture);
}


const struct st_tracked_state st_update_texture = {
   .name = "st_update_texture",
   .dirty = {
      .mesa = _NEW_TEXTURE,
      .st  = ST_NEW_FRAGMENT_PROGRAM,
   },
   .update = update_textures
};





