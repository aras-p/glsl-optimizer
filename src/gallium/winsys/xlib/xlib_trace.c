/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Bismarck, ND., USA
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE 
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * 
 **************************************************************************/

/*
 * Authors:
 *   Keith Whitwell
 *   Brian Paul
 */


#include "xlib.h"

#include "trace/tr_screen.h"
#include "trace/tr_context.h"
#include "trace/tr_texture.h"

#include "pipe/p_screen.h"



static struct pipe_screen *
xlib_create_trace_screen( void )
{
   struct pipe_screen *screen, *trace_screen;

   screen = xlib_softpipe_driver.create_pipe_screen();
   if (screen == NULL)
      goto fail;

   /* Wrap it:
    */
   trace_screen = trace_screen_create(screen);
   if (trace_screen == NULL)
      goto fail;

   return trace_screen;

fail:
   if (screen)
      screen->destroy( screen );
   return NULL;
}

static struct pipe_context *
xlib_create_trace_context( struct pipe_screen *_screen,
                           void *priv )
{
   struct trace_screen *tr_scr = trace_screen( _screen );
   struct pipe_screen *screen = tr_scr->screen;
   struct pipe_context *pipe, *trace_pipe;
   
   pipe = xlib_softpipe_driver.create_pipe_context( screen, priv );
   if (pipe == NULL)
      goto fail;

   /* Wrap it:
    */
   trace_pipe = trace_context_create(_screen, pipe);
   if (trace_pipe == NULL)
      goto fail;

   trace_pipe->priv = priv;

   return trace_pipe;

fail:
   if (pipe)
      pipe->destroy( pipe );
   return NULL;
}

static void
xlib_trace_display_surface( struct xmesa_buffer *buffer,
                            struct pipe_surface *_surf )
{
   struct trace_surface *tr_surf = trace_surface( _surf );
   struct pipe_surface *surf = tr_surf->surface;

   xlib_softpipe_driver.display_surface( buffer, surf );
}


struct xm_driver xlib_trace_driver = 
{
   .create_pipe_screen = xlib_create_trace_screen,
   .create_pipe_context = xlib_create_trace_context,
   .display_surface = xlib_trace_display_surface,
};
