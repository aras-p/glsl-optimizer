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

 /*
  * Authors:
  *   Keith Whitwell <keithw@vmware.com>
  */
 

#include "main/macros.h"
#include "st_context.h"
#include "pipe/p_context.h"
#include "st_atom.h"


/**
 * Scissor depends on the scissor box, and the framebuffer dimensions.
 */
static void
update_scissor( struct st_context *st )
{
   struct pipe_scissor_state scissor[PIPE_MAX_VIEWPORTS];
   const struct gl_context *ctx = st->ctx;
   const struct gl_framebuffer *fb = ctx->DrawBuffer;
   GLint miny, maxy;
   int i;
   bool changed = false;
   for (i = 0 ; i < ctx->Const.MaxViewports; i++) {
      scissor[i].minx = 0;
      scissor[i].miny = 0;
      scissor[i].maxx = fb->Width;
      scissor[i].maxy = fb->Height;

      if (ctx->Scissor.EnableFlags & (1 << i)) {
         /* need to be careful here with xmax or ymax < 0 */
         GLint xmax = MAX2(0, ctx->Scissor.ScissorArray[i].X + ctx->Scissor.ScissorArray[i].Width);
         GLint ymax = MAX2(0, ctx->Scissor.ScissorArray[i].Y + ctx->Scissor.ScissorArray[i].Height);

         if (ctx->Scissor.ScissorArray[i].X > (GLint)scissor[i].minx)
            scissor[i].minx = ctx->Scissor.ScissorArray[i].X;
         if (ctx->Scissor.ScissorArray[i].Y > (GLint)scissor[i].miny)
            scissor[i].miny = ctx->Scissor.ScissorArray[i].Y;

         if (xmax < (GLint) scissor[i].maxx)
            scissor[i].maxx = xmax;
         if (ymax < (GLint) scissor[i].maxy)
            scissor[i].maxy = ymax;
         
         /* check for null space */
         if (scissor[i].minx >= scissor[i].maxx || scissor[i].miny >= scissor[i].maxy)
            scissor[i].minx = scissor[i].miny = scissor[i].maxx = scissor[i].maxy = 0;
      }

      /* Now invert Y if needed.
       * Gallium drivers use the convention Y=0=top for surfaces.
       */
      if (st_fb_orientation(fb) == Y_0_TOP) {
         miny = fb->Height - scissor[i].maxy;
         maxy = fb->Height - scissor[i].miny;
         scissor[i].miny = miny;
         scissor[i].maxy = maxy;
      }

      if (memcmp(&scissor[i], &st->state.scissor[i], sizeof(scissor[0])) != 0) {
         /* state has changed */
         st->state.scissor[i] = scissor[i];  /* struct copy */
         changed = true;
      }
   }
   if (changed)
      st->pipe->set_scissor_states(st->pipe, 0, ctx->Const.MaxViewports, scissor); /* activate */
}


const struct st_tracked_state st_update_scissor = {
   "st_update_scissor",					/* name */
   {							/* dirty */
      (_NEW_SCISSOR | _NEW_BUFFERS),			/* mesa */
      0,						/* st */
   },
   update_scissor					/* update */
};
