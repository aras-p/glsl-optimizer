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
 * Convert GLenum stencil func tokens to pipe tokens.
 */
static GLuint
gl_alpha_func_to_sp(GLenum func)
{
   /* Same values, just biased */
   assert(PIPE_FUNC_NEVER == GL_NEVER - GL_NEVER);
   assert(PIPE_FUNC_LESS == GL_LESS - GL_NEVER);
   assert(PIPE_FUNC_EQUAL == GL_EQUAL - GL_NEVER);
   assert(PIPE_FUNC_LEQUAL == GL_LEQUAL - GL_NEVER);
   assert(PIPE_FUNC_GREATER == GL_GREATER - GL_NEVER);
   assert(PIPE_FUNC_NOTEQUAL == GL_NOTEQUAL - GL_NEVER);
   assert(PIPE_FUNC_GEQUAL == GL_GEQUAL - GL_NEVER);
   assert(PIPE_FUNC_ALWAYS == GL_ALWAYS - GL_NEVER);
   assert(func >= GL_NEVER);
   assert(func <= GL_ALWAYS);
   return func - GL_NEVER;
}


static void 
update_alpha_test( struct st_context *st )
{
   struct pipe_alpha_test_state alpha;

   memset(&alpha, 0, sizeof(alpha));

   if (st->ctx->Color.AlphaEnabled) {
      alpha.enabled = 1;
      alpha.func = gl_alpha_func_to_sp(st->ctx->Color.AlphaFunc);
      alpha.ref = st->ctx->Color.AlphaRef;
   }

   if (memcmp(&alpha, &st->state.alpha_test, sizeof(alpha)) != 0) {
      /* state has changed */
      st->state.alpha_test = alpha;  /* struct copy */
      st->pipe->set_alpha_test_state(st->pipe, &alpha); /* set new state */
   }
}


const struct st_tracked_state st_update_alpha_test = {
   .name = "st_update_alpha_test",
   .dirty = {
      .mesa = (_NEW_COLOR),
      .st  = 0,
   },
   .update = update_alpha_test
};





