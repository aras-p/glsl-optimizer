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


#include "xlib.h"
#include "softpipe/sp_texture.h"
#include "softpipe/sp_screen.h"
#include "state_tracker/sw_winsys.h"
#include "util/u_debug.h"

static struct pipe_screen *
xlib_create_softpipe_screen( Display *display )
{
   struct sw_winsys *winsys;
   struct pipe_screen *screen;

   winsys = xlib_create_sw_winsys( display );
   if (winsys == NULL)
      return NULL;

   screen = softpipe_create_screen(winsys);
   if (screen == NULL)
      goto fail;

   return screen;

fail:
   if (winsys)
      winsys->destroy( winsys );

   return NULL;
}


static void
xlib_softpipe_display_surface(struct xmesa_buffer *xm_buffer,
                              struct pipe_surface *surf)
{
   struct softpipe_texture *texture = softpipe_texture(surf->texture);

   assert(texture->dt);
   if (texture->dt)
      xlib_sw_display(xm_buffer, texture->dt);
}


struct xm_driver xlib_softpipe_driver = 
{
   .create_pipe_screen = xlib_create_softpipe_screen,
   .display_surface = xlib_softpipe_display_surface
};



