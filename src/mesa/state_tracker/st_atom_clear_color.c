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


static void
update_clear_color_state( struct st_context *st )
{
   struct pipe_clear_color_state clear;

   clear.color[0] = st->ctx->Color.ClearColor[0];
   clear.color[1] = st->ctx->Color.ClearColor[1];
   clear.color[2] = st->ctx->Color.ClearColor[2];
   clear.color[3] = st->ctx->Color.ClearColor[3];

   if (memcmp(&clear, &st->state.clear_color, sizeof(clear)) != 0) {
      st->state.clear_color = clear;
      st->pipe->set_clear_color_state( st->pipe, &clear );
   }
}


const struct st_tracked_state st_update_clear_color = {
   .dirty = {
      .mesa = _NEW_COLOR,
      .st  = 0,
   },
   .update = update_clear_color_state
};
