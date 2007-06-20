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


static void 
update_point( struct st_context *st )
{
   struct pipe_point_state point;

   memset(&point, 0, sizeof(point));

   point.smooth = st->ctx->Point.SmoothFlag;
   point.size = st->ctx->Point.Size;
   point.min_size = st->ctx->Point.MinSize;
   point.max_size = st->ctx->Point.MaxSize;
   point.attenuation[0] = st->ctx->Point.Params[0];
   point.attenuation[1] = st->ctx->Point.Params[1];
   point.attenuation[2] = st->ctx->Point.Params[2];

   if (memcmp(&point, &st->state.point, sizeof(point)) != 0) {
      /* state has changed */
      st->state.point = point;  /* struct copy */
      st->pipe->set_point_state(st->pipe, &point); /* set new state */
   }
}


const struct st_tracked_state st_update_point = {
   .dirty = {
      .mesa = (_NEW_POINT),
      .st  = 0,
   },
   .update = update_point
};
