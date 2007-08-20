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

/* Author:
 *    Brian Paul
 *    Keith Whitwell
 */


#include "pipe/p_defines.h"
#include "pipe/p_context.h"
#include "pipe/p_winsys.h"
#include "pipe/p_util.h"

#include "sp_context.h"
#include "sp_state.h"

#include "pipe/draw/draw_context.h"



void
softpipe_draw_arrays(struct pipe_context *pipe, unsigned mode,
                     unsigned start, unsigned count)
{
   softpipe_draw_elements(pipe, NULL, 0, mode, start, count);
}



/**
 * Draw vertex arrays, with optional indexing.
 * Basically, map the vertex buffers (and drawing surfaces), then hand off
 * the drawing to the 'draw' module.
 *
 * XXX should the element buffer be specified/bound with a separate function?
 */
void
softpipe_draw_elements(struct pipe_context *pipe,
                       struct pipe_buffer_handle *indexBuffer,
                       unsigned indexSize,
                       unsigned mode, unsigned start, unsigned count)
{
   struct softpipe_context *sp = softpipe_context(pipe);
   struct draw_context *draw = sp->draw;
   unsigned length, first, incr, i;
   void *mapped_indexes = NULL;

   /* first, check that the primitive is not malformed */
   draw_prim_info( mode, &first, &incr );
   length = draw_trim( count, first, incr );
   if (!length)
      return;


   if (sp->dirty)
      softpipe_update_derived( sp );

   softpipe_map_surfaces(sp);

   /*
    * Map vertex buffers
    */
   for (i = 0; i < PIPE_ATTRIB_MAX; i++) {
      if (sp->vertex_buffer[i].buffer) {
         void *buf
            = pipe->winsys->buffer_map(pipe->winsys,
                                       sp->vertex_buffer[i].buffer,
                                       PIPE_BUFFER_FLAG_READ);
         draw_set_mapped_vertex_buffer(draw, i, buf);
      }
   }
   /* Map index buffer, if present */
   if (indexBuffer) {
      mapped_indexes = pipe->winsys->buffer_map(pipe->winsys,
                                                indexBuffer,
                                                PIPE_BUFFER_FLAG_READ);
      draw_set_mapped_element_buffer(draw, indexSize, mapped_indexes);
   }
   else {
      draw_set_mapped_element_buffer(draw, 0, NULL);  /* no index/element buffer */
   }


   draw_arrays(draw, mode, start, count);


   /*
    * unmap vertex/index buffers
    */
   for (i = 0; i < PIPE_ATTRIB_MAX; i++) {
      if (sp->vertex_buffer[i].buffer) {
         pipe->winsys->buffer_unmap(pipe->winsys, sp->vertex_buffer[i].buffer);
         draw_set_mapped_vertex_buffer(draw, i, NULL);
      }
   }
   if (indexBuffer) {
      pipe->winsys->buffer_unmap(pipe->winsys, indexBuffer);
      draw_set_mapped_element_buffer(draw, 0, NULL);
   }

   softpipe_unmap_surfaces(sp);
}
