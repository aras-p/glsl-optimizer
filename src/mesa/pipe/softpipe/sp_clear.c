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


#include "pipe/p_defines.h"
#include "sp_clear.h"
#include "sp_context.h"
#include "sp_surface.h"
#include "colormac.h"


static GLuint
color_value(GLuint format, const GLfloat color[4])
{
   GLubyte r, g, b, a;

   UNCLAMPED_FLOAT_TO_UBYTE(r, color[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(g, color[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(b, color[2]);
   UNCLAMPED_FLOAT_TO_UBYTE(a, color[3]);

   switch (format) {
   case PIPE_FORMAT_U_R8_G8_B8_A8:
      return (r << 24) | (g << 16) | (b << 8) | a;
   case PIPE_FORMAT_U_A8_R8_G8_B8:
      return (a << 24) | (r << 16) | (g << 8) | b;
   case PIPE_FORMAT_U_R5_G6_B5:
      return ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3);
   default:
      return 0;
   }
}
 

void
softpipe_clear(struct pipe_context *pipe, GLboolean color, GLboolean depth,
               GLboolean stencil, GLboolean accum)
{
   const struct softpipe_context *softpipe = softpipe_context(pipe);
   const GLint x = softpipe->cliprect.minx;
   const GLint y = softpipe->cliprect.miny;
   const GLint w = softpipe->cliprect.maxx - x;
   const GLint h = softpipe->cliprect.maxy - y;

   if (color) {
      GLuint i;
      for (i = 0; i < softpipe->framebuffer.num_cbufs; i++) {
         struct pipe_surface *ps = softpipe->framebuffer.cbufs[i];

         if (softpipe->blend.colormask == (PIPE_MASK_R | PIPE_MASK_G |
                                           PIPE_MASK_B | PIPE_MASK_A)) {
            /* no masking */
            GLuint clearVal = color_value(ps->format,
                                          softpipe->clear_color.color);
            pipe->region_fill(pipe, ps->region, 0, x, y, w, h, clearVal);
         }
         else {
            /* masking */

            /*
            for (j = 0; j < h; j++) {
               sps->write_mono_row_ub(sps, w, x, y + j, clr);
            }
            */
         }
      }
   }

   if (depth) {
      struct pipe_surface *ps = softpipe->framebuffer.zbuf;
      GLuint clearVal;

      switch (ps->format) {
      case PIPE_FORMAT_U_Z16:
         clearVal = (GLuint) (softpipe->depth_test.clear * 65535.0);
         break;
      case PIPE_FORMAT_U_Z32:
         clearVal = (GLuint) (softpipe->depth_test.clear * 0xffffffff);
         break;
      case PIPE_FORMAT_Z24_S8:
         clearVal = (GLuint) (softpipe->depth_test.clear * 0xffffff);
         break;
      default:
         assert(0);
      }

      pipe->region_fill(pipe, ps->region, 0, x, y, w, h, clearVal);
   }

   if (stencil) {
      struct pipe_surface *ps = softpipe->framebuffer.sbuf;
      GLuint clearVal = softpipe->stencil.clear_value;
      if (softpipe->stencil.write_mask[0] /*== 0xff*/) {
         /* no masking */
         pipe->region_fill(pipe, ps->region, 0, x, y, w, h, clearVal);
      }
      else if (softpipe->stencil.write_mask[0] != 0x0) {
         /* masking */
         /* fill with quad funcs */
         assert(0);
      }
   }
}
