/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#define _GDI32_

#include <windows.h>
#include "pipe/p_winsys.h"
#include "pipe/p_screen.h"
#include "pipe/p_context.h"
#include "state_tracker/st_context.h"
#include "state_tracker/st_public.h"
#include "stw_winsys.h"
#include "stw_device.h"
#include "stw_framebuffer.h"
#include "stw_wgl_context.h"

WINGDIAPI BOOL APIENTRY
wglSwapBuffers(
   HDC hdc )
{
   struct stw_framebuffer *fb;
   struct pipe_surface *surf;

   fb = framebuffer_from_hdc( hdc );
   if (fb == NULL)
      return FALSE;

   /* If we're swapping the buffer associated with the current context
    * we have to flush any pending rendering commands first.
    */
   st_notify_swapbuffers( fb->stfb );

   surf = st_get_framebuffer_surface( fb->stfb, ST_SURFACE_BACK_LEFT );

   stw_winsys.flush_frontbuffer(stw_dev->screen->winsys,
                               surf,
                               hdc );

   return TRUE;
}

WINGDIAPI BOOL APIENTRY
wglSwapLayerBuffers(
   HDC hdc,
   UINT fuPlanes )
{
   (void) hdc;
   (void) fuPlanes;

   return FALSE;
}
