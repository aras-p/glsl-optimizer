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
#include "stw_tls.h"


static INLINE struct stw_framebuffer *
stw_framebuffer_from_hwnd_locked(
   HWND hwnd )
{
   struct stw_framebuffer *fb;

   for (fb = stw_dev->fb_head; fb != NULL; fb = fb->next)
      if (fb->hWnd == hwnd)
         break;

   return fb;
}


static INLINE void
stw_framebuffer_destroy_locked(
   struct stw_framebuffer *fb )
{
   struct stw_framebuffer **link;

   link = &stw_dev->fb_head;
   while (*link != fb)
      link = &(*link)->next;
   assert(*link);
   *link = fb->next;
   fb->next = NULL;

   st_unreference_framebuffer(fb->stfb);
   
   pipe_mutex_destroy( fb->mutex );
   
   FREE( fb );
}


/**
 * @sa http://msdn.microsoft.com/en-us/library/ms644975(VS.85).aspx
 * @sa http://msdn.microsoft.com/en-us/library/ms644960(VS.85).aspx
 */
LRESULT CALLBACK
stw_call_window_proc(
   int nCode,
   WPARAM wParam,
   LPARAM lParam )
{
   struct stw_tls_data *tls_data;
   PCWPSTRUCT pParams = (PCWPSTRUCT)lParam;
   
   tls_data = stw_tls_get_data();
   if(!tls_data)
      return 0;
   
   if (nCode < 0)
       return CallNextHookEx(tls_data->hCallWndProcHook, nCode, wParam, lParam);

   if (pParams->message == WM_SIZE && pParams->wParam != SIZE_MINIMIZED) {
      struct stw_framebuffer *fb;

      pipe_mutex_lock( stw_dev->mutex );
      fb = stw_framebuffer_from_hwnd_locked( pParams->hwnd );
      pipe_mutex_unlock( stw_dev->mutex );
      
      if(fb) {
         unsigned width = LOWORD( pParams->lParam );
         unsigned height = HIWORD( pParams->lParam );
         
         pipe_mutex_lock( fb->mutex );
         fb->must_resize = TRUE;
         fb->width = width;
         fb->height = height;
         pipe_mutex_unlock( fb->mutex );
      }
   }

   if (pParams->message == WM_DESTROY) {
      struct stw_framebuffer *fb;

      pipe_mutex_lock( stw_dev->mutex );
      
      fb = stw_framebuffer_from_hwnd_locked( pParams->hwnd );
      if(fb)
         stw_framebuffer_destroy_locked(fb);
      
      pipe_mutex_unlock( stw_dev->mutex );
   }

   return CallNextHookEx(tls_data->hCallWndProcHook, nCode, wParam, lParam);
}


static void
stw_framebuffer_get_size( struct stw_framebuffer *fb )
{
   unsigned width, height;
   RECT rect;

   assert(fb->hWnd);
   
   GetClientRect( fb->hWnd, &rect );
   width = rect.right - rect.left;
   height = rect.bottom - rect.top;

   if(width < 1)
      width = 1;
   if(height < 1)
      height = 1;

   if(width != fb->width || height != fb->height) {
      fb->must_resize = TRUE;
      fb->width = width; 
      fb->height = height; 
   }
}


/**
 * Create a new framebuffer object which will correspond to the given HDC.
 */
struct stw_framebuffer *
stw_framebuffer_create_locked(
   HDC hdc,
   int iPixelFormat )
{
   HWND hWnd;
   struct stw_framebuffer *fb;
   const struct stw_pixelformat_info *pfi;

   /* We only support drawing to a window. */
   hWnd = WindowFromDC( hdc );
   if(!hWnd)
      return NULL;
   
   fb = CALLOC_STRUCT( stw_framebuffer );
   if (fb == NULL)
      return NULL;

   fb->hDC = hdc;
   fb->hWnd = hWnd;
   fb->iPixelFormat = iPixelFormat;

   fb->pfi = pfi = stw_pixelformat_get_info( iPixelFormat - 1 );

   stw_pixelformat_visual(&fb->visual, pfi);
   
   stw_framebuffer_get_size(fb);

   pipe_mutex_init( fb->mutex );

   fb->next = stw_dev->fb_head;
   stw_dev->fb_head = fb;

   return fb;
}


BOOL
stw_framebuffer_allocate(
   struct stw_framebuffer *fb)
{
   pipe_mutex_lock( fb->mutex );

   if(!fb->stfb) {
      const struct stw_pixelformat_info *pfi = fb->pfi;
      enum pipe_format colorFormat, depthFormat, stencilFormat;

      colorFormat = pfi->color_format;
      
      assert(pf_layout( pfi->depth_stencil_format ) == PIPE_FORMAT_LAYOUT_RGBAZS );
   
      if(pf_get_component_bits( pfi->depth_stencil_format, PIPE_FORMAT_COMP_Z ))
         depthFormat = pfi->depth_stencil_format;
      else
         depthFormat = PIPE_FORMAT_NONE;
   
      if(pf_get_component_bits( pfi->depth_stencil_format, PIPE_FORMAT_COMP_S ))
         stencilFormat = pfi->depth_stencil_format;
      else
         stencilFormat = PIPE_FORMAT_NONE;
   
      assert(fb->must_resize);
      assert(fb->width);
      assert(fb->height);

      fb->stfb = st_create_framebuffer(
         &fb->visual,
         colorFormat,
         depthFormat,
         stencilFormat,
         fb->width,
         fb->height,
         (void *) fb );
      
      // to notify the context
      fb->must_resize = TRUE;
   }
   
   pipe_mutex_unlock( fb->mutex );

   return fb->stfb ? TRUE : FALSE;
}


/**
 * Update the framebuffer's size if necessary.
 */
void
stw_framebuffer_update(
   struct stw_framebuffer *fb)
{
   assert(fb->stfb);
   assert(fb->height);
   assert(fb->width);
   
   /* XXX: It would be nice to avoid checking the size again -- in theory  
    * stw_call_window_proc would have cought the resize and stored the right 
    * size already, but unfortunately threads created before the DllMain is 
    * called don't get a DLL_THREAD_ATTACH notification, and there is no way
    * to know of their existing without using the not very portable PSAPI.
    */
   stw_framebuffer_get_size(fb);
   
   if(fb->must_resize) {
      st_resize_framebuffer(fb->stfb, fb->width, fb->height);
      fb->must_resize = FALSE;
   }
}                      


void
stw_framebuffer_cleanup( void )
{
   struct stw_framebuffer *fb;
   struct stw_framebuffer *next;

   pipe_mutex_lock( stw_dev->mutex );

   fb = stw_dev->fb_head;
   while (fb) {
      next = fb->next;
      stw_framebuffer_destroy_locked(fb);
      fb = next;
   }
   stw_dev->fb_head = NULL;
   
   pipe_mutex_unlock( stw_dev->mutex );
}


/**
 * Given an hdc, return the corresponding stw_framebuffer.
 */
struct stw_framebuffer *
stw_framebuffer_from_hdc_locked(
   HDC hdc )
{
   HWND hwnd;
   struct stw_framebuffer *fb;

   /* 
    * Some applications create and use several HDCs for the same window, so 
    * looking up the framebuffer by the HDC is not reliable. Use HWND whenever
    * possible.
    */ 
   hwnd = WindowFromDC(hdc);
   if(hwnd)
      return stw_framebuffer_from_hwnd_locked(hwnd);
   
   for (fb = stw_dev->fb_head; fb != NULL; fb = fb->next)
      if (fb->hDC == hdc)
         break;

   return fb;
}


/**
 * Given an hdc, return the corresponding stw_framebuffer.
 */
struct stw_framebuffer *
stw_framebuffer_from_hdc(
   HDC hdc )
{
   struct stw_framebuffer *fb;

   pipe_mutex_lock( stw_dev->mutex );
   fb = stw_framebuffer_from_hdc_locked(hdc);
   pipe_mutex_unlock( stw_dev->mutex );

   return fb;
}


BOOL
stw_pixelformat_set(
   HDC hdc,
   int iPixelFormat )
{
   uint count;
   uint index;
   struct stw_framebuffer *fb;

   index = (uint) iPixelFormat - 1;
   count = stw_pixelformat_get_extended_count();
   if (index >= count)
      return FALSE;

   pipe_mutex_lock( stw_dev->mutex );
   
   fb = stw_framebuffer_from_hdc_locked(hdc);
   if(fb) {
      /* SetPixelFormat must be called only once */
      pipe_mutex_unlock( stw_dev->mutex );
      return FALSE;
   }

   fb = stw_framebuffer_create_locked(hdc, iPixelFormat);
   if(!fb) {
      pipe_mutex_unlock( stw_dev->mutex );
      return FALSE;
   }
      
   pipe_mutex_unlock( stw_dev->mutex );

   /* Some applications mistakenly use the undocumented wglSetPixelFormat 
    * function instead of SetPixelFormat, so we call SetPixelFormat here to 
    * avoid opengl32.dll's wglCreateContext to fail */
   if (GetPixelFormat(hdc) == 0) {
        SetPixelFormat(hdc, iPixelFormat, NULL);
   }
   
   return TRUE;
}


int
stw_pixelformat_get(
   HDC hdc )
{
   struct stw_framebuffer *fb;

   fb = stw_framebuffer_from_hdc(hdc);
   if(!fb)
      return 0;
   
   return fb->iPixelFormat;
}


BOOL
stw_swap_buffers(
   HDC hdc )
{
   struct stw_framebuffer *fb;
   struct pipe_screen *screen;
   struct pipe_surface *surface;

   fb = stw_framebuffer_from_hdc( hdc );
   if (fb == NULL)
      return FALSE;

   if (!(fb->pfi->pfd.dwFlags & PFD_DOUBLEBUFFER))
      return TRUE;

   pipe_mutex_lock( fb->mutex );

   /* If we're swapping the buffer associated with the current context
    * we have to flush any pending rendering commands first.
    */
   st_notify_swapbuffers( fb->stfb );

   screen = stw_dev->screen;
   
   if(!st_get_framebuffer_surface( fb->stfb, ST_SURFACE_BACK_LEFT, &surface )) {
      /* FIXME: this shouldn't happen, but does on glean */
      pipe_mutex_unlock( fb->mutex );
      return FALSE;
   }

#ifdef DEBUG
   if(stw_dev->trace_running) {
      screen = trace_screen(screen)->screen;
      surface = trace_surface(surface)->surface;
   }
#endif

   stw_dev->stw_winsys->flush_frontbuffer( screen, surface, hdc );
   
   stw_framebuffer_update(fb);
   
   pipe_mutex_unlock( fb->mutex );
   
   return TRUE;
}


BOOL
stw_swap_layer_buffers(
   HDC hdc,
   UINT fuPlanes )
{
   if(fuPlanes & WGL_SWAP_MAIN_PLANE)
      return stw_swap_buffers(hdc);

   return FALSE;
}
