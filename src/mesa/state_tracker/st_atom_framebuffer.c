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
#include "st_cb_fbo.h"
#include "pipe/p_context.h"
#include "cso_cache/cso_context.h"


/**
 * Update framebuffer state (color, depth, stencil, etc. buffers)
 * XXX someday: separate draw/read buffers.
 */
static void
update_framebuffer_state( struct st_context *st )
{
   struct pipe_framebuffer_state framebuffer;
   struct gl_framebuffer *fb = st->ctx->DrawBuffer;
   struct st_renderbuffer *strb;
   GLuint i;

   memset(&framebuffer, 0, sizeof(framebuffer));

   /* Examine Mesa's ctx->DrawBuffer->_ColorDrawBuffers state
    * to determine which surfaces to draw to
    */
   framebuffer.num_cbufs = fb->_NumColorDrawBuffers[0];
   for (i = 0; i < framebuffer.num_cbufs; i++) {
      strb = st_renderbuffer(fb->_ColorDrawBuffers[0][i]);
      assert(strb->surface);
      framebuffer.cbufs[i] = strb->surface;
   }

   strb = st_renderbuffer(fb->Attachment[BUFFER_DEPTH].Renderbuffer);
   if (strb) {
      strb = st_renderbuffer(strb->Base.Wrapped);
      assert(strb->surface);
      framebuffer.zsbuf = strb->surface;
   }
   else {
      strb = st_renderbuffer(fb->Attachment[BUFFER_STENCIL].Renderbuffer);
      if (strb) {
         strb = st_renderbuffer(strb->Base.Wrapped);
         assert(strb->surface);
         framebuffer.zsbuf = strb->surface;
      }
   }

   cso_set_framebuffer(st->cso_context, &framebuffer);
}


const struct st_tracked_state st_update_framebuffer = {
   .name = "st_update_framebuffer",
   .dirty = {
      .mesa = _NEW_BUFFERS,
      .st  = 0,
   },
   .update = update_framebuffer_state
};

