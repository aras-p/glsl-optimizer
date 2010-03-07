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

#ifndef SVGA_SCREEN_H
#define SVGA_SCREEN_H


#include "pipe/p_screen.h"
#include "os/os_thread.h"

#include "util/u_double_list.h"

#include "svga_screen_cache.h"


struct svga_winsys_screen;
struct svga_winsys_context;
struct SVGACmdMemory;

#define SVGA_COMBINE_USERBUFFERS 1

/**
 * Subclass of pipe_screen
 */
struct svga_screen
{
   struct pipe_screen screen;
   struct svga_winsys_screen *sws;

   unsigned use_ps30;
   unsigned use_vs30;
   
   struct {
      boolean force_level_surface_view;
      boolean force_surface_view;
      boolean no_surface_view;
      boolean force_sampler_view;
      boolean no_sampler_view;
   } debug;

   /* The screen needs its own context */
   struct svga_winsys_context *swc;
   struct SVGACmdMemory *fifo;

   unsigned texture_timestamp;
   pipe_mutex tex_mutex; 
   pipe_mutex swc_mutex; /* Protects the use of swc and dirty_buffers */
   
   struct svga_host_surface_cache cache;
};

#ifndef DEBUG
/** cast wrapper */
static INLINE struct svga_screen *
svga_screen(struct pipe_screen *pscreen)
{
   return (struct svga_screen *) pscreen;
}
#else
struct svga_screen *
svga_screen(struct pipe_screen *screen);
#endif

void svga_screen_flush( struct svga_screen *svga_screen, 
                        struct pipe_fence_handle **pfence );

#endif /* SVGA_SCREEN_H */
