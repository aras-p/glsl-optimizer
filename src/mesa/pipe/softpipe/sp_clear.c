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
#include "sp_state.h"
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
   struct softpipe_context *softpipe = softpipe_context(pipe);
   GLint x, y, w, h;

   softpipe_update_derived(softpipe); /* not needed?? */

   x = 0;
   y = 0;
   w = softpipe->framebuffer.cbufs[0]->width;
   h = softpipe->framebuffer.cbufs[0]->height;

   if (color) {
      GLuint i;
      for (i = 0; i < softpipe->framebuffer.num_cbufs; i++) {
         struct pipe_surface *ps = softpipe->framebuffer.cbufs[i];
         GLuint clearVal = color_value(ps->format,
                                       softpipe->clear_color.color);
         pipe->region_fill(pipe, ps->region, 0, x, y, w, h, clearVal, ~0);
      }
   }

   if (depth && stencil &&
       softpipe->framebuffer.zbuf == softpipe->framebuffer.sbuf) {
      /* clear Z and stencil together */
      struct pipe_surface *ps = softpipe->framebuffer.zbuf;
      if (ps->format == PIPE_FORMAT_S8_Z24) {
         GLuint mask = (softpipe->stencil.write_mask[0] << 8) | 0xffffff;	 
         GLuint clearVal = (GLuint) (softpipe->depth_test.clear * 0xffffff);

	 assert (mask == ~0);

         clearVal |= (softpipe->stencil.clear_value << 24);
         pipe->region_fill(pipe, ps->region, 0, x, y, w, h, clearVal, ~0);
      }
      else {
         /* XXX Z24_S8 format? */
         assert(0);
      }
   }
   else {
      /* separate Z and stencil */
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
         case PIPE_FORMAT_S8_Z24:
            clearVal = (GLuint) (softpipe->depth_test.clear * 0xffffff);
            break;
         default:
            assert(0);
         }

         pipe->region_fill(pipe, ps->region, 0, x, y, w, h, clearVal, ~0);
      }

      if (stencil) {
         struct pipe_surface *ps = softpipe->framebuffer.sbuf;
         GLuint clearVal = softpipe->stencil.clear_value;

	 /* If this is not ~0, we shouldn't get here - clear should be
	  * done with geometry instead.
	  */
         GLuint mask = softpipe->stencil.write_mask[0];

	 assert((mask & 0xff) == 0xff);

         switch (ps->format) {
         case PIPE_FORMAT_S8_Z24:
            clearVal = clearVal << 24;
            mask = mask << 24;
            break;
         case PIPE_FORMAT_U_S8:
            /* nothing */
            break;
         default:
            assert(0);
         }

         pipe->region_fill(pipe, ps->region, 0, x, y, w, h, clearVal, mask);
      }
   }

   if (accum) {
      /* XXX there might be no notion of accum buffers in 'pipe'.
       * Just implement them with a deep RGBA surface format...
       */
      struct pipe_surface *ps = softpipe->framebuffer.abuf;
      GLuint clearVal = 0x0; /* XXX FIX */
      GLuint mask = ~0;
      assert(ps);
      pipe->region_fill(pipe, ps->region, 0, x, y, w, h, clearVal, mask);
   }
}
