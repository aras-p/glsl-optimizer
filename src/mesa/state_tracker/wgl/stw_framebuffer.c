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
#include "main/context.h"
#include "pipe/p_format.h"
#include "state_tracker/st_context.h"
#include "state_tracker/st_public.h"
#include "stw_framebuffer.h"

void
framebuffer_resize(
   struct stw_framebuffer *fb,
   GLuint width,
   GLuint height )
{
   if (fb->hbmDIB == NULL || fb->stfb->Base.Width != width || fb->stfb->Base.Height != height) {
      if (fb->hbmDIB)
         DeleteObject( fb->hbmDIB );

      fb->hbmDIB = CreateCompatibleBitmap(
         fb->hDC,
         width,
         height );
   }

   st_resize_framebuffer( fb->stfb, width, height );
}

static struct stw_framebuffer *fb_head = NULL;

static LRESULT CALLBACK
window_proc(
   HWND hWnd,
   UINT uMsg,
   WPARAM wParam,
   LPARAM lParam )
{
   struct stw_framebuffer *fb;

   for (fb = fb_head; fb != NULL; fb = fb->next)
      if (fb->hWnd == hWnd)
         break;
   assert( fb != NULL );

   if (uMsg == WM_SIZE && wParam != SIZE_MINIMIZED)
      framebuffer_resize( fb, LOWORD( lParam ), HIWORD( lParam ) );

   return CallWindowProc( fb->WndProc, hWnd, uMsg, wParam, lParam );
}

/* Create a new framebuffer object which will correspond to the given HDC.
 */
struct stw_framebuffer *
framebuffer_create(
   HDC hdc,
   GLvisual *visual,
   GLuint width,
   GLuint height )
{
   struct stw_framebuffer *fb;
   enum pipe_format colorFormat, depthFormat, stencilFormat;

   fb = CALLOC_STRUCT( stw_framebuffer );
   if (fb == NULL)
      return NULL;

   /* Determine PIPE_FORMATs for buffers.
    */
   colorFormat = PIPE_FORMAT_A8R8G8B8_UNORM;

   if (visual->depthBits == 0)
      depthFormat = PIPE_FORMAT_NONE;
   else if (visual->depthBits <= 16)
      depthFormat = PIPE_FORMAT_Z16_UNORM;
   else if (visual->depthBits <= 24)
      depthFormat = PIPE_FORMAT_S8Z24_UNORM;
   else
      depthFormat = PIPE_FORMAT_Z32_UNORM;

   if (visual->stencilBits == 8) {
      if (depthFormat == PIPE_FORMAT_S8Z24_UNORM)
         stencilFormat = depthFormat;
      else
         stencilFormat = PIPE_FORMAT_S8_UNORM;
   }
   else {
      stencilFormat = PIPE_FORMAT_NONE;
   }

   fb->stfb = st_create_framebuffer(
      visual,
      colorFormat,
      depthFormat,
      stencilFormat,
      width,
      height,
      (void *) fb );

   fb->cColorBits = GetDeviceCaps( hdc, BITSPIXEL );
   fb->hDC = hdc;

   /* Subclass a window associated with the device context.
    */
   fb->hWnd = WindowFromDC( hdc );
   if (fb->hWnd != NULL) {
      fb->WndProc = (WNDPROC) SetWindowLong(
         fb->hWnd,
         GWL_WNDPROC,
         (LONG) window_proc );
   }

   fb->next = fb_head;
   fb_head = fb;
   return fb;
}

void
framebuffer_destroy(
   struct stw_framebuffer *fb )
{
   struct stw_framebuffer **link = &fb_head;
   struct stw_framebuffer *pfb = fb_head;

   while (pfb != NULL) {
      if (pfb == fb) {
         if (fb->hWnd != NULL) {
            SetWindowLong(
               fb->hWnd,
               GWL_WNDPROC,
               (LONG) fb->WndProc );
         }

         *link = fb->next;
         FREE( fb );
         return;
      }

      link = &pfb->next;
      pfb = pfb->next;
   }
}

/* Given an hdc, return the corresponding wgl_context.
 */
struct stw_framebuffer *
framebuffer_from_hdc(
   HDC hdc )
{
   struct stw_framebuffer *fb;

   for (fb = fb_head; fb != NULL; fb = fb->next)
      if (fb->hDC == hdc)
         return fb;
   return NULL;
}
