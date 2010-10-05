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

#include "svga_resource_texture.h"
#include "svga_context.h"
#include "svga_debug.h"
#include "svga_cmd.h"
#include "svga_surface.h"

#define FILE_DEBUG_FLAG DEBUG_BLIT


/* XXX I got my doubts about this, should maybe use svga_texture_copy_handle directly? */
static void svga_surface_copy(struct pipe_context *pipe,
                              struct pipe_resource* dst_tex,
                              struct pipe_subresource subdst,
                              unsigned dstx, unsigned dsty, unsigned dstz,
                              struct pipe_resource* src_tex,
                              struct pipe_subresource subsrc,
                              unsigned srcx, unsigned srcy, unsigned srcz,
                              unsigned width, unsigned height)
{
   struct svga_context *svga = svga_context(pipe);
   struct pipe_screen *screen = pipe->screen;
   SVGA3dCopyBox *box;
   enum pipe_error ret;
   struct pipe_surface *srcsurf, *dstsurf;

   svga_hwtnl_flush_retry( svga );

   srcsurf = screen->get_tex_surface(screen, src_tex,
                                     subsrc.face, subsrc.level, srcz,
                                     PIPE_BIND_SAMPLER_VIEW);

   dstsurf = screen->get_tex_surface(screen, dst_tex,
                                     subdst.face, subdst.level, dstz,
                                     PIPE_BIND_RENDER_TARGET);

   SVGA_DBG(DEBUG_DMA, "blit to sid %p (%d,%d), from sid %p (%d,%d) sz %dx%d\n",
            svga_surface(dstsurf)->handle,
            dstx, dsty,
            svga_surface(srcsurf)->handle,
            srcx, srcy,
            width, height);

   ret = SVGA3D_BeginSurfaceCopy(svga->swc,
                                 srcsurf,
                                 dstsurf,
                                 &box,
                                 1);
   if(ret != PIPE_OK) {

      svga_context_flush(svga, NULL);

      ret = SVGA3D_BeginSurfaceCopy(svga->swc,
                                    srcsurf,
                                    dstsurf,
                                    &box,
                                    1);
      assert(ret == PIPE_OK);
   }

   box->x = dstx;
   box->y = dsty;
   box->z = 0;
   box->w = width;
   box->h = height;
   box->d = 1;
   box->srcx = srcx;
   box->srcy = srcy;
   box->srcz = 0;

   SVGA_FIFOCommitAll(svga->swc);

   svga_surface(dstsurf)->dirty = TRUE;
   svga_propagate_surface(pipe, dstsurf);

   pipe_surface_reference(&srcsurf, NULL);
   pipe_surface_reference(&dstsurf, NULL);

}


void
svga_init_blit_functions(struct svga_context *svga)
{
   svga->pipe.resource_copy_region = svga_surface_copy;
}
