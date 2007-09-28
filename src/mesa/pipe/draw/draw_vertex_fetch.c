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

#include "pipe/p_util.h"
#include "draw_private.h"
#include "draw_context.h"
#include "draw_vertex.h"

#include "pipe/tgsi/exec/tgsi_core.h"

/**
 * Fetch a float[4] vertex attribute from memory, doing format/type
 * conversion as needed.
 * XXX this might be a temporary thing.
 */
static void
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
void draw_vertex_fetch( struct draw_context *draw,
			struct tgsi_exec_machine *machine,
			const unsigned *elts,
			unsigned count )
{
   unsigned j;

   /* loop over vertices */
   for (j = 0; j < count; j++) {
      uint attr;

      /*printf("fetch vertex %u: \n", j);*/

      /* loop over vertex attributes (vertex shader inputs) */
      for (attr = 0; attr < draw->vertex_shader.num_inputs; attr++) {

         unsigned buf = draw->vertex_element[attr].vertex_buffer_index;
         const void *src
            = (const void *) ((const ubyte *) draw->mapped_vbuffer[buf]
                              + draw->vertex_buffer[buf].buffer_offset
                              + draw->vertex_element[attr].src_offset
                              + elts[j] * draw->vertex_buffer[buf].pitch);
         float p[4];

         fetch_attrib4(src, draw->vertex_element[attr].src_format, p);

         /*printf("  %u: %f %f %f %f\n", attr, p[0], p[1], p[2], p[3]);*/

         /* Transform to AoS xxxx/yyyy/zzzz/wwww representation:
          */
         machine->Inputs[attr].xyzw[0].f[j] = p[0]; /*X*/
         machine->Inputs[attr].xyzw[1].f[j] = p[1]; /*Y*/
         machine->Inputs[attr].xyzw[2].f[j] = p[2]; /*Z*/
         machine->Inputs[attr].xyzw[3].f[j] = p[3]; /*W*/
      }
   }
}

