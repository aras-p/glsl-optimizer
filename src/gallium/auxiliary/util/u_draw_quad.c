/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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


#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_inlines.h"
#include "pipe/p_winsys.h"
#include "util/u_draw_quad.h"


/**
 * Draw screen-aligned textured quad.
 */
void 
util_draw_texquad(struct pipe_context *pipe,
                  float x0, float y0, float x1, float y1, float z)
{
   struct pipe_buffer *vbuf;
   struct pipe_vertex_buffer vbuffer;
   struct pipe_vertex_element velement;
   uint numAttribs = 2, vertexBytes, i, j;
   float *v;

   vertexBytes = 4 * (4 * numAttribs * sizeof(float));

   /* XXX create one-time */
   vbuf = pipe->winsys->buffer_create(pipe->winsys, 32,
                                      PIPE_BUFFER_USAGE_VERTEX, vertexBytes);
   assert(vbuf);

   v = (float *) pipe->winsys->buffer_map(pipe->winsys, vbuf,
                                          PIPE_BUFFER_USAGE_CPU_WRITE);

   /*
    * Load vertex buffer
    */
   for (i = j = 0; i < 4; i++) {
      v[j + 2] = z;   /* z */
      v[j + 3] = 1.0; /* w */
      v[j + 6] = 0.0; /* r */
      v[j + 7] = 1.0; /* q */
      j += 8;
   }

   v[0] = x0;
   v[1] = y0;
   v[4] = 0.0; /*s*/
   v[5] = 0.0; /*t*/

   v[8] = x1;
   v[9] = y0;
   v[12] = 1.0;
   v[13] = 0.0;

   v[16] = x1;
   v[17] = y1;
   v[20] = 1.0;
   v[21] = 1.0;

   v[24] = x0;
   v[25] = y1;
   v[28] = 0.0;
   v[29] = 1.0;

   pipe->winsys->buffer_unmap(pipe->winsys, vbuf);

   /* tell pipe about the vertex buffer */
   vbuffer.buffer = vbuf;
   vbuffer.pitch = numAttribs * 4 * sizeof(float);  /* vertex size */
   vbuffer.buffer_offset = 0;
   pipe->set_vertex_buffer(pipe, 0, &vbuffer);

   /* tell pipe about the vertex attributes */
   for (i = 0; i < numAttribs; i++) {
      velement.src_offset = i * 4 * sizeof(float);
      velement.vertex_buffer_index = 0;
      velement.src_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
      velement.nr_components = 4;
      pipe->set_vertex_element(pipe, i, &velement);
   }

   /* draw */
   pipe->draw_arrays(pipe, PIPE_PRIM_TRIANGLE_FAN, 0, 4);

   /* XXX: do one-time */
   pipe_buffer_reference(pipe->winsys, &vbuf, NULL);
}
