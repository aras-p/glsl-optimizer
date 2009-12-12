/**********************************************************
 * Copyright 2008-2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/

#include "svga_screen_texture.h"
#include "svga_context.h"
#include "svga_debug.h"
#include "svga_cmd.h"

#define FILE_DEBUG_FLAG DEBUG_BLIT


static void svga_surface_copy(struct pipe_context *pipe,
                              struct pipe_surface *dest,
                              unsigned destx, unsigned desty,
                              struct pipe_surface *src,
                              unsigned srcx, unsigned srcy,
                              unsigned width, unsigned height)
{
   struct svga_context *svga = svga_context(pipe);
   SVGA3dCopyBox *box;
   enum pipe_error ret;

   svga_hwtnl_flush_retry( svga );

   SVGA_DBG(DEBUG_DMA, "blit to sid %p (%d,%d), from sid %p (%d,%d) sz %dx%d\n",
            svga_surface(dest)->handle,
            destx, desty,
            svga_surface(src)->handle,
            srcx, srcy,
            width, height);

   ret = SVGA3D_BeginSurfaceCopy(svga->swc,
                                 src,
                                 dest,
                                 &box,
                                 1);
   if(ret != PIPE_OK) {

      svga_context_flush(svga, NULL);

      ret = SVGA3D_BeginSurfaceCopy(svga->swc,
                                    src,
                                    dest,
                                    &box,
                                    1);
      assert(ret == PIPE_OK);
   }

   box->x = destx;
   box->y = desty;
   box->z = 0;
   box->w = width;
   box->h = height;
   box->d = 1;
   box->srcx = srcx;
   box->srcy = srcy;
   box->srcz = 0;

   SVGA_FIFOCommitAll(svga->swc);

   svga_surface(dest)->dirty = TRUE;
   svga_propagate_surface(pipe, dest);
}


void
svga_init_blit_functions(struct svga_context *svga)
{
   svga->pipe.surface_copy = svga_surface_copy;
}
