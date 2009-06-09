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


struct stw_framebuffer *
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
static LRESULT CALLBACK
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
         
         /* FIXME: The mesa statetracker makes the assumptions that only
          * one context is using the framebuffer, and that that context is the 
          * current one. However neither holds true, as WGL allows more than
          * one context to be bound to the same drawable, and this function can 
          * be called from any thread.
          */
         pipe_mutex_lock( fb->mutex );
         if (fb->stfb)
            st_resize_framebuffer( fb->stfb, width, height );
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
   
   pipe_mutex_init( fb->mutex );

   fb->next = stw_dev->fb_head;
   stw_dev->fb_head = fb;

   return fb;
}


static void
stw_framebuffer_get_size( struct stw_framebuffer *fb, GLuint *pwidth, GLuint *pheight )
{
   GLuint width, height;

   if (fb->hWnd) {
      RECT rect;
      GetClientRect( fb->hWnd, &rect );
      width = rect.right - rect.left;
      height = rect.bottom - rect.top;
   }
   else {
      width = GetDeviceCaps( fb->hDC, HORZRES );
      height = GetDeviceCaps( fb->hDC, VERTRES );
   }

   if(width < 1)
      width = 1;
   if(height < 1)
      height = 1;

   *pwidth = width; 
   *pheight = height; 
}


BOOL
stw_framebuffer_allocate(
   struct stw_framebuffer *fb)
{
   pipe_mutex_lock( fb->mutex );

   if(!fb->stfb) {
      const struct stw_pixelformat_info *pfi = fb->pfi;
      enum pipe_format colorFormat, depthFormat, stencilFormat;
      GLuint width, height; 

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
   
      stw_framebuffer_get_size(fb, &width, &height);
      
      fb->stfb = st_create_framebuffer(
         &fb->visual,
         colorFormat,
         depthFormat,
         stencilFormat,
         width,
         height,
         (void *) fb );
   }
   
   pipe_mutex_unlock( fb->mutex );

   return fb->stfb ? TRUE : FALSE;
}


void
stw_framebuffer_resize(
   struct stw_framebuffer *fb)
{
   GLuint width, height; 
   assert(fb->stfb);
   stw_framebuffer_get_size(fb, &width, &height);
   st_resize_framebuffer(fb->stfb, width, height);
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
   struct stw_framebuffer *fb;

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


boolean
stw_framebuffer_init_thread(void)
{
   struct stw_tls_data *tls_data;
   
   tls_data = stw_tls_get_data();
   if(!tls_data)
      return FALSE;
   
   tls_data->hCallWndProcHook = SetWindowsHookEx(WH_CALLWNDPROC,
                                                 stw_call_window_proc,
                                                 NULL,
                                                 GetCurrentThreadId());
   if(tls_data->hCallWndProcHook == NULL)
      return FALSE;
   
   return TRUE;
}

void
stw_framebuffer_cleanup_thread(void)
{
   struct stw_tls_data *tls_data;
   
   tls_data = stw_tls_get_data();
   if(!tls_data)
      return;
   
   if(tls_data->hCallWndProcHook) {
      UnhookWindowsHookEx(tls_data->hCallWndProcHook);
      tls_data->hCallWndProcHook = NULL;
   }
}
