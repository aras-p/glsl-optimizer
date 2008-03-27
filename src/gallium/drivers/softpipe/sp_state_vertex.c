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


#include "sp_context.h"
#include "sp_state.h"
#include "sp_surface.h"

#include "draw/draw_context.h"


void
softpipe_set_vertex_element(struct pipe_context *pipe,
                            unsigned index,
                            const struct pipe_vertex_element *attrib)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   assert(index < PIPE_MAX_ATTRIBS);
   softpipe->vertex_element[index] = *attrib; /* struct copy */
   softpipe->dirty |= SP_NEW_VERTEX;

   draw_set_vertex_element(softpipe->draw, index, attrib);
}


void
softpipe_set_vertex_buffer(struct pipe_context *pipe,
                           unsigned index,
                           const struct pipe_vertex_buffer *buffer)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);
   assert(index < PIPE_MAX_ATTRIBS);
   softpipe->vertex_buffer[index] = *buffer; /* struct copy */
   softpipe->dirty |= SP_NEW_VERTEX;

   draw_set_vertex_buffer(softpipe->draw, index, buffer);
}
