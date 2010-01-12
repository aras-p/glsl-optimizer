/**************************************************************************
 * 
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
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

/* Authors:
 *    Michel Dänzer
 */


#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_pack_color.h"
#include "util/u_rect.h"


/**
 * Clear the given buffers to the specified values.
 * No masking, no scissor (clear entire buffer).
 */
static INLINE void
util_clear(struct pipe_context *pipe,
           struct pipe_framebuffer_state *framebuffer, unsigned buffers,
           const float *rgba, double depth, unsigned stencil)
{
   if (buffers & PIPE_CLEAR_COLOR) {
      struct pipe_surface *ps = framebuffer->cbufs[0];
      union util_color uc;

      util_pack_color(rgba, ps->format, &uc);
      if (pipe->surface_fill) {
         pipe->surface_fill(pipe, ps, 0, 0, ps->width, ps->height, uc.ui);
      } else {
         util_surface_fill(pipe, ps, 0, 0, ps->width, ps->height, uc.ui);
      }
   }

   if (buffers & PIPE_CLEAR_DEPTHSTENCIL) {
      struct pipe_surface *ps = framebuffer->zsbuf;

      if (pipe->surface_fill) {
         pipe->surface_fill(pipe, ps, 0, 0, ps->width, ps->height,
                            util_pack_z_stencil(ps->format, depth, stencil));
      } else {
         util_surface_fill(pipe, ps, 0, 0, ps->width, ps->height,
                           util_pack_z_stencil(ps->format, depth, stencil));
      }
   }
}
