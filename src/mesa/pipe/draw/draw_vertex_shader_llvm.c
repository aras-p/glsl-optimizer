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
  *   Zack Rusin zack@tungstengraphics.com
  */

#include "pipe/p_util.h"
#include "draw_private.h"
#include "draw_context.h"
#include "draw_vertex.h"

#include "pipe/llvm/llvmtgsi.h"
#include "pipe/tgsi/exec/tgsi_core.h"

static INLINE void
fetch_attrib4(const void *ptr, unsigned format, float attrib[4])
{
   /* defaults */
   attrib[1] = 0.0;
   attrib[2] = 0.0;
   attrib[3] = 1.0;
   switch (format) {
   case PIPE_FORMAT_R32G32B32A32_FLOAT:
      attrib[3] = ((float *) ptr)[3];
      /* fall-through */
   case PIPE_FORMAT_R32G32B32_FLOAT:
      attrib[2] = ((float *) ptr)[2];
      /* fall-through */
   case PIPE_FORMAT_R32G32_FLOAT:
      attrib[1] = ((float *) ptr)[1];
      /* fall-through */
   case PIPE_FORMAT_R32_FLOAT:
      attrib[0] = ((float *) ptr)[0];
      break;
   default:
      assert(0);
   }
}


/**
 * Fetch vertex attributes for 'count' vertices.
 */
static INLINE
void vertex_fetch(struct draw_context *draw,
                  const unsigned elt,
                  float (*inputs)[4])
{
   uint attr;

   printf("fetch vertex %u: \n", elt);

   /* loop over vertex attributes (vertex shader inputs) */
   for (attr = 0; attr < draw->vertex_shader->state->num_inputs; attr++) {

      unsigned buf = draw->vertex_element[attr].vertex_buffer_index;
      const void *src
         = (const void *) ((const ubyte *) draw->mapped_vbuffer[buf]
                           + draw->vertex_buffer[buf].buffer_offset
                           + draw->vertex_element[attr].src_offset
                           + elt * draw->vertex_buffer[buf].pitch);
      float p[4];

      fetch_attrib4(src, draw->vertex_element[attr].src_format, p);

      printf(">  %u: %f %f %f %f\n", attr, p[0], p[1], p[2], p[3]);

      inputs[attr][0] = p[0]; /*X*/
      inputs[attr][1] = p[1]; /*Y*/
      inputs[attr][2] = p[2]; /*Z*/
      inputs[attr][3] = p[3]; /*W*/
   }
}


/**
 * Called by the draw module when the vertx cache needs to be flushed.
 * This involves running the vertex shader.
 */
void draw_vertex_shader_queue_flush_llvm(struct draw_context *draw)
{
   unsigned i;

   struct vertex_header *dests[VS_QUEUE_LENGTH];
   float                 inputs[VS_QUEUE_LENGTH][PIPE_MAX_SHADER_INPUTS][4];
   float (*consts)[4] = (float (*)[4]) draw->mapped_constants;
   struct ga_llvm_prog *prog = draw->vertex_shader->state->llvm_prog;

   fprintf(stderr, "XX q(%d) ", draw->vs.queue_nr);

   /* fetch the inputs */
   for (i = 0; i < draw->vs.queue_nr; ++i) {
      unsigned elt = draw->vs.queue[i].elt;
      dests[i] = draw->vs.queue[i].dest;
      vertex_fetch(draw, elt, inputs[i]);
   }

   /* batch execute the shaders on all the vertices */
   ga_llvm_prog_exec(prog, inputs, dests, consts,
                     draw->vs.queue_nr);

   draw->vs.queue_nr = 0;
}
