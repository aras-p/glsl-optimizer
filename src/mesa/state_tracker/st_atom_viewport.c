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


#include "context.h"
#include "colormac.h"
#include "st_context.h"
#include "pipe/p_context.h"
#include "st_atom.h"





/**
 * Update the viewport transformation matrix.  Depends on:
 *  - viewport pos/size
 *  - depthrange
 *  - window pos/size or FBO size
 */
static void update_viewport( struct st_context *st )
{
   GLcontext *ctx = st->ctx;
   const GLframebuffer *DrawBuffer = ctx->DrawBuffer;
   GLfloat yScale = 1.0;
   GLfloat yBias = 0.0;

   /* _NEW_BUFFERS
    */
   if (DrawBuffer) {

#if 0
      if (DrawBuffer->Name) {
	 /* User created FBO */
	 struct st_renderbuffer *irb
	    = st_renderbuffer(DrawBuffer->_ColorDrawBuffers[0][0]);
	 if (irb && !irb->RenderToTexture) {
	    /* y=0=top */
	    yScale = -1.0;
	    yBias = irb->Base.Height;
	 }
	 else {
	    /* y=0=bottom */
	    yScale = 1.0;
	    yBias = 0.0;
	 }
      }
      else 
      {
	 /* window buffer, y=0=top */
	 yScale = -1.0;
	 yBias = DrawBuffer->Height;
      }
#endif
   }

   {
      /* _NEW_VIEWPORT 
       */
      GLfloat x = ctx->Viewport.X;
      GLfloat y = ctx->Viewport.Y;
      GLfloat z = ctx->Viewport.Near;
      GLfloat half_width = ctx->Viewport.Width / 2.0;
      GLfloat half_height = ctx->Viewport.Height / 2.0;
      GLfloat half_depth = (ctx->Viewport.Far - ctx->Viewport.Near) / 2.0;

      struct pipe_viewport vp;
      
      vp.scale[0] = half_width;
      vp.scale[1] = half_height * yScale;
      vp.scale[2] = half_depth;
      vp.scale[3] = 1.0;

      vp.translate[0] = (half_width + x);
      vp.translate[1] = (half_height + y) * yScale + yBias;
      vp.translate[2] = (half_depth + z);
      vp.translate[3] = 0.0;

      if (memcmp(&vp, &st->state.viewport, sizeof(vp)) != 0) {
	 st->state.viewport = vp;
	 st->pipe->set_viewport(st->pipe, &vp);
      }
   }
}


const struct st_tracked_state st_update_viewport = {
   .dirty = {
      .mesa = _NEW_BUFFERS | _NEW_VIEWPORT,
      .st = 0,
   },
   .update = update_viewport
};
