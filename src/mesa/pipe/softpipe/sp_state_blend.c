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



void softpipe_set_blend_state( struct pipe_context *pipe,
			     const struct pipe_blend_state *blend )
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   softpipe->blend = *blend;

   softpipe->dirty |= G_NEW_BLEND;
}


void softpipe_set_blend_color( struct pipe_context *pipe,
			     const struct pipe_blend_color *blend_color )
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   softpipe->blend_color = *blend_color;

   softpipe->dirty |= G_NEW_BLEND;
}


/** XXX move someday?  Or consolidate all these simple state setters
 * into one file.
 */
void
softpipe_set_depth_test_state(struct pipe_context *pipe,
                              const struct pipe_depth_state *depth)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   softpipe->depth_test = *depth;

   softpipe->dirty |= G_NEW_DEPTH_TEST;
}

void
softpipe_set_alpha_test_state(struct pipe_context *pipe,
                              const struct pipe_alpha_test_state *alpha)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   softpipe->alpha_test = *alpha;

   softpipe->dirty |= G_NEW_ALPHA_TEST;
}

