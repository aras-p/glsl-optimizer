/**************************************************************************
 * 
 * Copyright 2007 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


#include "main/context.h"
#include "st_context.h"
#include "st_atom.h"
#include "pipe/p_context.h"
#include "cso_cache/cso_context.h"

/**
 * Update the viewport transformation matrix.  Depends on:
 *  - viewport pos/size
 *  - depthrange
 *  - window pos/size or FBO size
 */
static void
update_viewport( struct st_context *st )
{
   struct gl_context *ctx = st->ctx;
   GLfloat yScale, yBias;
   int i;
   /* _NEW_BUFFERS
    */
   if (st_fb_orientation(ctx->DrawBuffer) == Y_0_TOP) {
      /* Drawing to a window.  The corresponding gallium surface uses
       * Y=0=TOP but OpenGL is Y=0=BOTTOM.  So we need to invert the viewport.
       */
      yScale = -1;
      yBias = (GLfloat)ctx->DrawBuffer->Height;
   }
   else {
      /* Drawing to an FBO where Y=0=BOTTOM, like OpenGL - don't invert */
      yScale = 1.0;
      yBias = 0.0;
   }

   /* _NEW_VIEWPORT 
    */
   for (i = 0; i < ctx->Const.MaxViewports; i++)
   {
      GLfloat x = ctx->ViewportArray[i].X;
      GLfloat y = ctx->ViewportArray[i].Y;
      GLfloat z = ctx->ViewportArray[i].Near;
      GLfloat half_width = ctx->ViewportArray[i].Width * 0.5f;
      GLfloat half_height = ctx->ViewportArray[i].Height * 0.5f;
      GLfloat half_depth = (GLfloat)(ctx->ViewportArray[i].Far - ctx->ViewportArray[i].Near) * 0.5f;
      
      st->state.viewport[i].scale[0] = half_width;
      st->state.viewport[i].scale[1] = half_height * yScale;
      st->state.viewport[i].scale[2] = half_depth;
      st->state.viewport[i].scale[3] = 1.0;

      st->state.viewport[i].translate[0] = half_width + x;
      st->state.viewport[i].translate[1] = (half_height + y) * yScale + yBias;
      st->state.viewport[i].translate[2] = half_depth + z;
      st->state.viewport[i].translate[3] = 0.0;
   }

   cso_set_viewport(st->cso_context, &st->state.viewport[0]);
   if (ctx->Const.MaxViewports > 1)
      st->pipe->set_viewport_states(st->pipe, 1, ctx->Const.MaxViewports - 1, &st->state.viewport[1]);
}


const struct st_tracked_state st_update_viewport = {
   "st_update_viewport",				/* name */
   {							/* dirty */
      _NEW_BUFFERS | _NEW_VIEWPORT,			/* mesa */
      0,						/* st */
   },
   update_viewport					/* update */
};
