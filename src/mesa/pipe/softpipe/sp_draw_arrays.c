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



static void
softpipe_map_constant_buffers(struct softpipe_context *sp)
{
   struct pipe_winsys *ws = sp->pipe.winsys;
   uint i;
   for (i = 0; i < 2; i++) {
      if (sp->constants[i].size)
         sp->mapped_constants[i] = ws->buffer_map(ws, sp->constants[i].buffer,
                                                  PIPE_BUFFER_FLAG_READ);
   }

   draw_set_mapped_constant_buffer(sp->draw,
                                   sp->mapped_constants[PIPE_SHADER_VERTEX]);
}

static void
softpipe_unmap_constant_buffers(struct softpipe_context *sp)
{
   struct pipe_winsys *ws = sp->pipe.winsys;
   uint i;
   for (i = 0; i < 2; i++) {
      if (sp->constants[i].size)
         ws->buffer_unmap(ws, sp->constants[i].buffer);
      sp->mapped_constants[i] = NULL;
   }
}


boolean
softpipe_draw_arrays(struct pipe_context *pipe, unsigned mode,
                     unsigned start, unsigned count)
{
   return softpipe_draw_elements(pipe, NULL, 0, mode, start, count);
}



/**
 * Draw vertex arrays, with optional indexing.
 * Basically, map the vertex buffers (and drawing surfaces), then hand off
 * the drawing to the 'draw' module.
 *
 * XXX should the element buffer be specified/bound with a separate function?
 */
boolean
softpipe_draw_elements(struct pipe_context *pipe,
                       struct pipe_buffer_handle *indexBuffer,
                       unsigned indexSize,
                       unsigned mode, unsigned start, unsigned count)
{
   struct softpipe_context *sp = softpipe_context(pipe);
   struct draw_context *draw = sp->draw;
   unsigned i;

   /* first, check that the primitive is not malformed.  It is the
    * state tracker's responsibility to do send only correctly formed
    * primitives down.  It currently isn't doing that though...
    */
#if 1
   count = draw_trim_prim( mode, count );
#else
   if (!draw_validate_prim( mode, count ))
      assert(0);
#endif


   if (sp->dirty)
      softpipe_update_derived( sp );

   softpipe_map_surfaces(sp);

   softpipe_map_constant_buffers(sp);

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
      void *mapped_indexes
         = pipe->winsys->buffer_map(pipe->winsys, indexBuffer,
                                    PIPE_BUFFER_FLAG_READ);
      draw_set_mapped_element_buffer(draw, indexSize, mapped_indexes);
   }
   else {
      /* no index/element buffer */
      draw_set_mapped_element_buffer(draw, 0, NULL);
   }

   /* Map feedback buffers if enabled */
   if (sp->feedback.enabled) {
      const uint n = sp->feedback.interleaved ? 1 : sp->feedback.num_attribs;
      for (i = 0; i < n; i++) {
         void *ptr = pipe->winsys->buffer_map(pipe->winsys,
                                              sp->feedback_buffer[i].buffer,
                                              PIPE_BUFFER_FLAG_WRITE);
         draw_set_mapped_feedback_buffer(draw, i, ptr,
                                         sp->feedback_buffer[i].size);
      }
   }


   /* draw! */
   draw_arrays(draw, mode, start, count);

   /* always flush for now */
   draw_flush(draw);

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

   /* Unmap feedback buffers if enabled */
   if (sp->feedback.enabled) {
      const uint n = sp->feedback.interleaved ? 1 : sp->feedback.num_attribs;
      for (i = 0; i < n; i++) {
         pipe->winsys->buffer_unmap(pipe->winsys,
                                    sp->feedback_buffer[i].buffer);
         draw_set_mapped_feedback_buffer(draw, i, NULL, 0);
      }
   }


   softpipe_unmap_surfaces(sp);
   softpipe_unmap_constant_buffers(sp);

   return TRUE;
}
