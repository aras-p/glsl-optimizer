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

#include "pipe/p_defines.h"
#include "svga_screen.h"
#include "svga_screen_texture.h"
#include "svga_context.h"
#include "svga_debug.h"


static void svga_flush( struct pipe_context *pipe,
                        unsigned flags,
                        struct pipe_fence_handle **fence )
{
   struct svga_context *svga = svga_context(pipe);
   int i;

   /* Emit buffered drawing commands.
    */
   svga_hwtnl_flush_retry( svga );

   /* Emit back-copy from render target view to texture.
    */
   for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++) {
      if (svga->curr.framebuffer.cbufs[i])
         svga_propagate_surface(pipe, svga->curr.framebuffer.cbufs[i]);
   }
   if (svga->curr.framebuffer.zsbuf)
      svga_propagate_surface(pipe, svga->curr.framebuffer.zsbuf);

   /* Flush command queue.
    */
   svga_context_flush(svga, fence);

   SVGA_DBG(DEBUG_DMA|DEBUG_PERF, "%s flags %x fence_ptr %p\n",
            __FUNCTION__, flags, fence ? *fence : 0x0);
}


void svga_init_flush_functions( struct svga_context *svga )
{
   svga->pipe.flush = svga_flush;
}
