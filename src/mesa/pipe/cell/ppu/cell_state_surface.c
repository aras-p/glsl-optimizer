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


#include "pipe/p_inlines.h"
#include "cell_context.h"
#include "cell_state.h"


void
cell_set_framebuffer_state(struct pipe_context *pipe,
                           const struct pipe_framebuffer_state *fb)
{
   struct cell_context *cell = cell_context(pipe);

   if (1 /*memcmp(&cell->framebuffer, fb, sizeof(*fb))*/) {
      struct pipe_surface *csurf = fb->cbufs[0];
      struct pipe_surface *zsurf = fb->zsbuf;
      uint i;

      /* unmap old surfaces */
      for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++) {
         if (cell->framebuffer.cbufs[i] && cell->cbuf_map[i]) {
            pipe_surface_unmap(cell->framebuffer.cbufs[i]);
            cell->cbuf_map[i] = NULL;
         }
      }

      if (cell->framebuffer.zsbuf && cell->zsbuf_map) {
         pipe_surface_unmap(cell->framebuffer.zsbuf);
         cell->zsbuf_map = NULL;
      }

      /* update my state */
      cell->framebuffer = *fb;

      /* map new surfaces */
      if (csurf)
         cell->cbuf_map[0] = pipe_surface_map(csurf);

      if (zsurf)
         cell->zsbuf_map = pipe_surface_map(zsurf);

      cell->dirty |= CELL_NEW_FRAMEBUFFER;
   }
}

