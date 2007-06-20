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
 */


#include "sp_clear.h"
#include "sp_context.h"
#include "sp_surface.h"
#include "colormac.h"


void
softpipe_clear(struct pipe_context *pipe, GLboolean color, GLboolean depth,
               GLboolean stencil, GLboolean accum)
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   if (color) {
      GLuint i;
      const GLint x = softpipe->scissor.minx;
      const GLint y = softpipe->scissor.miny;
      const GLint w = softpipe->scissor.maxx - x;
      const GLint h = softpipe->scissor.maxy - y;
      GLubyte clr[4];

      UNCLAMPED_FLOAT_TO_UBYTE(clr[0], softpipe->clear_color.color[0]);
      UNCLAMPED_FLOAT_TO_UBYTE(clr[1], softpipe->clear_color.color[1]);
      UNCLAMPED_FLOAT_TO_UBYTE(clr[2], softpipe->clear_color.color[2]);
      UNCLAMPED_FLOAT_TO_UBYTE(clr[3], softpipe->clear_color.color[3]);

      for (i = 0; i < softpipe->framebuffer.num_cbufs; i++) {
         struct pipe_surface *ps = softpipe->framebuffer.cbufs[i];
         struct softpipe_surface *sps = softpipe_surface(ps);
         GLint j;
         for (j = 0; j < h; j++) {
            sps->write_mono_row_ub(sps, w, x, y + j, clr);
         }
      }
   }

   if (depth) {
   }

}
