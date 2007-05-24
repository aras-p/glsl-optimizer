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


/* scissor state
 */
static void update_scissor( struct st_context *st )
{
   struct softpipe_scissor_rect scissor;

   scissor.minx = st->ctx->Scissor.X;
   scissor.miny = st->ctx->Scissor.Y;
   scissor.maxx = st->ctx->Scissor.X + st->ctx->Scissor.Width;
   scissor.maxy = st->ctx->Scissor.Y + st->ctx->Scissor.Height;
      
   if (memcmp(&scissor, &st->state.scissor, sizeof(scissor)) != 0) {
      /* state has changed */
      st->state.scissor = scissor;  /* struct copy */
      st->softpipe->set_scissor_rect(st->softpipe, &scissor); /* set new state */
   }
}


const struct st_tracked_state st_update_scissor = {
   .dirty = {
      .mesa = (_NEW_SCISSOR),
      .st  = 0,
   },
   .update = update_scissor
};





