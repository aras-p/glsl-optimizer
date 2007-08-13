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
#include "st_cb_texture.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"


/**
 * XXX This needs some work yet....
 * Need to "upload" texture images at appropriate times.
 */
static void 
update_textures(struct st_context *st)
{
   GLuint u;

   for (u = 0; u < st->ctx->Const.MaxTextureImageUnits; u++) {
      struct gl_texture_object *texObj
         = st->ctx->Texture.Unit[u]._Current;
      struct pipe_mipmap_tree *mt;
      if (texObj) {
         GLboolean flush, retval;

         retval = st_finalize_mipmap_tree(st->ctx, st->pipe, u, &flush);
         printf("finalize_mipmap_tree returned %d, flush = %d\n",
                retval, flush);

         mt = st_get_texobj_mipmap_tree(texObj);
      }
      else {
         mt = NULL;
      }

      st->state.texture[u] = mt;
      st->pipe->set_texture_state(st->pipe, u, mt);
   }
}


const struct st_tracked_state st_update_texture = {
   .dirty = {
      .mesa = _NEW_TEXTURE,
      .st  = 0,
   },
   .update = update_textures
};





