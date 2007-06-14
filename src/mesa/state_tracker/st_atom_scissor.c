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
  */
 

#include "st_context.h"
#include "softpipe/sp_context.h"
#include "st_atom.h"


/**
 * Scissor depends on the scissor box, and the framebuffer dimensions.
 */
static void
update_scissor( struct st_context *st )
{
   struct softpipe_scissor_rect scissor;
   const struct gl_framebuffer *fb = st->ctx->DrawBuffer;

   scissor.minx = 0;
   scissor.miny = 0;
   scissor.maxx = fb->Width;
   scissor.maxy = fb->Height;

   if (st->ctx->Scissor.Enabled) {
      if (st->ctx->Scissor.X > scissor.minx)
         scissor.minx = st->ctx->Scissor.X;
      if (st->ctx->Scissor.Y > scissor.miny)
         scissor.miny = st->ctx->Scissor.Y;

      if (st->ctx->Scissor.X + st->ctx->Scissor.Width < scissor.maxx)
         scissor.maxx = st->ctx->Scissor.X + st->ctx->Scissor.Width;
      if (st->ctx->Scissor.Y + st->ctx->Scissor.Height < scissor.maxy)
         scissor.maxy = st->ctx->Scissor.Y + st->ctx->Scissor.Height;

      /* check for null space */
      if (scissor.minx >= scissor.maxx || scissor.miny >= scissor.maxy)
         scissor.minx = scissor.miny = scissor.maxx = scissor.maxy = 0;
   }

   if (memcmp(&scissor, &st->state.scissor, sizeof(scissor)) != 0) {
      /* state has changed */
      st->state.scissor = scissor;  /* struct copy */
      st->softpipe->set_scissor_rect(st->softpipe, &scissor); /* activate */
   }
}


const struct st_tracked_state st_update_scissor = {
   .dirty = {
      .mesa = (_NEW_SCISSOR | _NEW_BUFFERS),
      .st  = 0,
   },
   .update = update_scissor
};





