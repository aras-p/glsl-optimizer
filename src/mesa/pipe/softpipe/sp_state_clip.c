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

/* Authors:  Keith Whitwell <keith@tungstengraphics.com>
 */
#include "imports.h"

#include "sp_context.h"
#include "sp_state.h"
#include "sp_draw.h"



void softpipe_set_clip_state( struct pipe_context *pipe,
			     const struct pipe_clip_state *clip )
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   memcpy(&softpipe->plane[6], clip->ucp, clip->nr * sizeof(clip->ucp[0]));

   softpipe->nr_planes = 6 + clip->nr;
   softpipe->dirty |= G_NEW_CLIP;
}



/* Called when driver state tracker notices changes to the viewport
 * matrix:
 */
void softpipe_set_viewport( struct pipe_context *pipe,
			   const struct pipe_viewport *viewport )
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   memcpy(&softpipe->viewport, viewport, sizeof(viewport));

   /* Using tnl/ and vf/ modules is temporary while getting started.
    * Full pipe will have vertex shader, vertex fetch of its own.
    */
   draw_set_viewport( softpipe->draw, viewport->scale, viewport->translate );
   softpipe->dirty |= G_NEW_VIEWPORT;
}



