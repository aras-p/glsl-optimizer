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

#include <windows.h>

#include "main/context.h"
#include "pipe/p_format.h"
#include "pipe/p_screen.h"
#include "state_tracker/st_context.h"
#include "state_tracker/st_public.h"

#ifdef DEBUG
#include "trace/tr_screen.h"
#include "trace/tr_texture.h"
#endif

#include "stw_framebuffer.h"
#include "stw_device.h"
#include "stw_public.h"
#include "stw_winsys.h"


void
framebuffer_resize(
   struct stw_framebuffer *fb,
   GLuint width,
   GLuint height )
{
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

static INLINE boolean
stw_is_supported_color(enum pipe_format format)
{
   struct pipe_screen *screen = stw_dev->screen;
   return screen->is_format_supported(screen, format, PIPE_TEXTURE_2D, 
                                      PIPE_TEXTURE_USAGE_RENDER_TARGET, 0);
}

static INLINE boolean
stw_is_supported_depth_stencil(enum pipe_format format)
{
   struct pipe_screen *screen = stw_dev->screen;
   return screen->is_format_supported(screen, format, PIPE_TEXTURE_2D, 
                                      PIPE_TEXTURE_USAGE_DEPTH_STENCIL, 0);
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

   /* Determine PIPE_FORMATs for buffers.
    */

   if(visual->alphaBits <= 0 && visual->redBits <= 5 && visual->blueBits <= 6 && visual->greenBits <= 5 && 
      stw_is_supported_color(PIPE_FORMAT_R5G6B5_UNORM)) {
      colorFormat = PIPE_FORMAT_R5G6B5_UNORM;
   }
   else if(visual->alphaBits <= 0 && visual->redBits <= 8 && visual->blueBits <= 8 && visual->greenBits <= 8 && 
      stw_is_supported_color(PIPE_FORMAT_X8R8G8B8_UNORM)) {
      colorFormat = PIPE_FORMAT_X8R8G8B8_UNORM;
   }
   else if(visual->alphaBits <= 1 && visual->redBits <= 5 && visual->blueBits <= 5 && visual->greenBits <= 5 &&
      stw_is_supported_color(PIPE_FORMAT_A1R5G5B5_UNORM)) {
      colorFormat = PIPE_FORMAT_A1R5G5B5_UNORM;
   }
   else if(visual->alphaBits <= 4 && visual->redBits <= 4 && visual->blueBits <= 4 && visual->greenBits <= 4 && 
      stw_is_supported_color(PIPE_FORMAT_A4R4G4B4_UNORM)) {
      colorFormat = PIPE_FORMAT_A4R4G4B4_UNORM;
   }
   else if(visual->alphaBits <= 8 && visual->redBits <= 8 && visual->blueBits <= 8 && visual->greenBits <= 8 && 
      stw_is_supported_color(PIPE_FORMAT_A8R8G8B8_UNORM)) {
      colorFormat = PIPE_FORMAT_A8R8G8B8_UNORM;
   }
   else {
      assert(0);
      return NULL;
   }

   if (visual->depthBits == 0)
      depthFormat = PIPE_FORMAT_NONE;
   else if (visual->depthBits <= 16 &&
            stw_is_supported_depth_stencil(PIPE_FORMAT_Z16_UNORM))
      depthFormat = PIPE_FORMAT_Z16_UNORM;
   else if (visual->depthBits <= 24 && visual->stencilBits != 8 &&
            stw_is_supported_depth_stencil(PIPE_FORMAT_X8Z24_UNORM)) {
      depthFormat = PIPE_FORMAT_X8Z24_UNORM;
   }
   else if (visual->depthBits <= 24 && visual->stencilBits != 8 && 
            stw_is_supported_depth_stencil(PIPE_FORMAT_Z24X8_UNORM)) {
      depthFormat = PIPE_FORMAT_Z24X8_UNORM;
   }
   else if (visual->depthBits <= 24 && visual->stencilBits == 8 && 
            stw_is_supported_depth_stencil(PIPE_FORMAT_S8Z24_UNORM)) {
      depthFormat = PIPE_FORMAT_S8Z24_UNORM;
   }
   else if (visual->depthBits <= 24 && visual->stencilBits == 8 && 
            stw_is_supported_depth_stencil(PIPE_FORMAT_Z24S8_UNORM)) {
      depthFormat = PIPE_FORMAT_Z24S8_UNORM;
   }
   else if(stw_is_supported_depth_stencil(PIPE_FORMAT_Z32_UNORM)) {
      depthFormat = PIPE_FORMAT_Z32_UNORM;
   }
   else if(stw_is_supported_depth_stencil(PIPE_FORMAT_Z32_FLOAT)) {
      depthFormat = PIPE_FORMAT_Z32_FLOAT;
   }
   else {
      assert(0);
      depthFormat = PIPE_FORMAT_NONE;
   }

   if (depthFormat == PIPE_FORMAT_S8Z24_UNORM || 
       depthFormat == PIPE_FORMAT_Z24S8_UNORM) {
      stencilFormat = depthFormat;
   }
   else if (visual->stencilBits == 8 && 
            stw_is_supported_depth_stencil(PIPE_FORMAT_S8_UNORM)) {
      stencilFormat = PIPE_FORMAT_S8_UNORM;
   }
   else {
      stencilFormat = PIPE_FORMAT_NONE;
   }

   fb = CALLOC_STRUCT( stw_framebuffer );
   if (fb == NULL)
      return NULL;

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
      fb->WndProc = (WNDPROC) SetWindowLongPtr(
         fb->hWnd,
         GWLP_WNDPROC,
         (LONG_PTR) window_proc );
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
            SetWindowLongPtr(
               fb->hWnd,
               GWLP_WNDPROC,
               (LONG_PTR) fb->WndProc );
         }

         *link = fb->next;
         FREE( fb );
         return;
      }

      link = &pfb->next;
      pfb = pfb->next;
   }
}

/* Given an hdc, return the corresponding stw_framebuffer.
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


BOOL
stw_swap_buffers(
   HDC hdc )
{
   struct stw_framebuffer *fb;
   struct pipe_screen *screen;
   struct pipe_surface *surface;

   fb = framebuffer_from_hdc( hdc );
   if (fb == NULL)
      return FALSE;

   /* If we're swapping the buffer associated with the current context
    * we have to flush any pending rendering commands first.
    */
   st_notify_swapbuffers( fb->stfb );

   screen = stw_dev->screen;
   
   if(!st_get_framebuffer_surface( fb->stfb, ST_SURFACE_BACK_LEFT, &surface ))
      /* FIXME: this shouldn't happen, but does on glean */
      return FALSE;

#ifdef DEBUG
   if(stw_dev->trace_running) {
      screen = trace_screen(screen)->screen;
      surface = trace_surface(surface)->surface;
   }
#endif

   stw_dev->stw_winsys->flush_frontbuffer( screen, surface, hdc );
   
   return TRUE;
}
