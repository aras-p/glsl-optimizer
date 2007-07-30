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


extern struct pipe_surface *
xmesa_get_color_surface(GLcontext *ctx, GLuint i);

extern struct pipe_surface *
xmesa_get_z_surface(GLcontext *ctx);

extern struct pipe_surface *
xmesa_get_stencil_surface(GLcontext *ctx);


/**
 * Update framebuffer state (color, depth, stencil, etc. buffers)
 * XXX someday: separate draw/read buffers.
 */
static void
update_framebuffer_state( struct st_context *st )
{
   struct pipe_framebuffer_state framebuffer;
   struct gl_renderbuffer *rb;
   GLuint i;

   memset(&framebuffer, 0, sizeof(framebuffer));

   /* Examine Mesa's ctx->DrawBuffer->_ColorDrawBuffers state
    * to determine which surfaces to draw to
    */
   framebuffer.num_cbufs = st->ctx->DrawBuffer->_NumColorDrawBuffers[0];
   for (i = 0; i < framebuffer.num_cbufs; i++) {
      rb = st->ctx->DrawBuffer->_ColorDrawBuffers[0][i];
      assert(rb->surface);
      framebuffer.cbufs[i] = rb->surface;
   }

   rb = st->ctx->DrawBuffer->_DepthBuffer;
   if (rb) {
      assert(rb->surface);
      framebuffer.zbuf = rb->Wrapped->surface;
   }

   rb = st->ctx->DrawBuffer->_StencilBuffer;
   if (rb) {
      assert(rb->surface);
      framebuffer.sbuf = rb->Wrapped->surface;
   }

   if (memcmp(&framebuffer, &st->state.framebuffer, sizeof(framebuffer)) != 0) {
      st->state.framebuffer = framebuffer;
      st->pipe->set_framebuffer_state( st->pipe, &framebuffer );
   }
}


const struct st_tracked_state st_update_framebuffer = {
   .dirty = {
      .mesa = _NEW_BUFFERS,
      .st  = 0,
   },
   .update = update_framebuffer_state
};

