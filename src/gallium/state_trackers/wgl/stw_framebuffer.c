/**************************************************************************
 *
 * Copyright 2008-2009 Vmware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include <windows.h>

#include "main/context.h"
#include "pipe/p_format.h"
#include "pipe/p_screen.h"
#include "util/u_format.h"
#include "state_tracker/st_context.h"
#include "state_tracker/st_public.h"

#ifdef DEBUG
#include "trace/tr_screen.h"
#include "trace/tr_texture.h"
#endif

#include "stw_icd.h"
#include "stw_framebuffer.h"
#include "stw_device.h"
#include "stw_winsys.h"
#include "stw_tls.h"


/**
 * Search the framebuffer with the matching HWND while holding the
 * stw_dev::fb_mutex global lock.
 */
static INLINE struct stw_framebuffer *
stw_framebuffer_from_hwnd_locked(
   HWND hwnd )
{
   struct stw_framebuffer *fb;

   for (fb = stw_dev->fb_head; fb != NULL; fb = fb->next)
      if (fb->hWnd == hwnd) {
         pipe_mutex_lock(fb->mutex);
         break;
      }

   return fb;
}


/**
 * Destroy this framebuffer. Both stw_dev::fb_mutex and stw_framebuffer::mutex
 * must be held, by this order. Obviously no further access to fb can be done 
 * after this.
 */
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

   if(fb->shared_surface)
      stw_dev->stw_winsys->shared_surface_close(stw_dev->screen, fb->shared_surface);

   st_unreference_framebuffer(fb->stfb);
   
   pipe_mutex_unlock( fb->mutex );

   pipe_mutex_destroy( fb->mutex );
   
   FREE( fb );
}


void
stw_framebuffer_release(
   struct stw_framebuffer *fb)
{
   assert(fb);
   pipe_mutex_unlock( fb->mutex );
}


static INLINE void
stw_framebuffer_get_size( struct stw_framebuffer *fb )
{
   unsigned width, height;
   RECT client_rect;
   RECT window_rect;
   POINT client_pos;

   assert(fb->hWnd);
   
   /* Get the client area size. */
   GetClientRect( fb->hWnd, &client_rect );
   assert(client_rect.left == 0);
   assert(client_rect.top == 0);
   width = client_rect.right - client_rect.left;
   height = client_rect.bottom - client_rect.top;

   if(width < 1)
      width = 1;
   if(height < 1)
      height = 1;

   if(width != fb->width || height != fb->height) {
      fb->must_resize = TRUE;
      fb->width = width; 
      fb->height = height; 
   }

   client_pos.x = 0;
   client_pos.y = 0;
   ClientToScreen(fb->hWnd, &client_pos);

   GetWindowRect(fb->hWnd, &window_rect);

   fb->client_rect.left = client_pos.x - window_rect.left;
   fb->client_rect.top =  client_pos.y - window_rect.top;
   fb->client_rect.right = fb->client_rect.left + fb->width;
   fb->client_rect.bottom = fb->client_rect.top + fb->height;

#if 0
   debug_printf("\n");
   debug_printf("%s: client_position = (%i, %i)\n",
                __FUNCTION__, client_pos.x, client_pos.y);
   debug_printf("%s: window_rect = (%i, %i) - (%i, %i)\n",
                __FUNCTION__,
                window_rect.left, window_rect.top,
                window_rect.right, window_rect.bottom);
   debug_printf("%s: client_rect = (%i, %i) - (%i, %i)\n",
                __FUNCTION__,
                fb->client_rect.left, fb->client_rect.top,
                fb->client_rect.right, fb->client_rect.bottom);
#endif
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
   struct stw_framebuffer *fb;
   
   tls_data = stw_tls_get_data();
   if(!tls_data)
      return 0;
   
   if (nCode < 0 || !stw_dev)
       return CallNextHookEx(tls_data->hCallWndProcHook, nCode, wParam, lParam);

   if (pParams->message == WM_WINDOWPOSCHANGED) {
      /* We handle WM_WINDOWPOSCHANGED instead of WM_SIZE because according to
       * http://blogs.msdn.com/oldnewthing/archive/2008/01/15/7113860.aspx 
       * WM_SIZE is generated from WM_WINDOWPOSCHANGED by DefWindowProc so it 
       * can be masked out by the application. */
      LPWINDOWPOS lpWindowPos = (LPWINDOWPOS)pParams->lParam;
      if((lpWindowPos->flags & SWP_SHOWWINDOW) || 
         !(lpWindowPos->flags & SWP_NOMOVE) ||
         !(lpWindowPos->flags & SWP_NOSIZE)) {
         fb = stw_framebuffer_from_hwnd( pParams->hwnd );
         if(fb) {
            /* Size in WINDOWPOS includes the window frame, so get the size 
             * of the client area via GetClientRect.  */
            stw_framebuffer_get_size(fb);
            stw_framebuffer_release(fb);
         }
      }
   }
   else if (pParams->message == WM_DESTROY) {
      pipe_mutex_lock( stw_dev->fb_mutex );
      fb = stw_framebuffer_from_hwnd_locked( pParams->hwnd );
      if(fb)
         stw_framebuffer_destroy_locked(fb);
      pipe_mutex_unlock( stw_dev->fb_mutex );
   }

   return CallNextHookEx(tls_data->hCallWndProcHook, nCode, wParam, lParam);
}


struct stw_framebuffer *
stw_framebuffer_create(
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

   /* This is the only case where we lock the stw_framebuffer::mutex before
    * stw_dev::fb_mutex, since no other thread can know about this framebuffer
    * and we must prevent any other thread from destroying it before we return.
    */
   pipe_mutex_lock( fb->mutex );

   pipe_mutex_lock( stw_dev->fb_mutex );
   fb->next = stw_dev->fb_head;
   stw_dev->fb_head = fb;
   pipe_mutex_unlock( stw_dev->fb_mutex );

   return fb;
}


BOOL
stw_framebuffer_allocate(
   struct stw_framebuffer *fb)
{
   assert(fb);
   
   if(!fb->stfb) {
      const struct stw_pixelformat_info *pfi = fb->pfi;
      enum pipe_format colorFormat, depthFormat, stencilFormat;

      colorFormat = pfi->color_format;

      if(util_format_get_component_bits(pfi->depth_stencil_format, UTIL_FORMAT_COLORSPACE_ZS, 0))
         depthFormat = pfi->depth_stencil_format;
      else
         depthFormat = PIPE_FORMAT_NONE;
   
      if(util_format_get_component_bits(pfi->depth_stencil_format, UTIL_FORMAT_COLORSPACE_ZS, 1))
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

   if (!stw_dev)
      return;

   pipe_mutex_lock( stw_dev->fb_mutex );

   fb = stw_dev->fb_head;
   while (fb) {
      next = fb->next;
      
      pipe_mutex_lock(fb->mutex);
      stw_framebuffer_destroy_locked(fb);
      
      fb = next;
   }
   stw_dev->fb_head = NULL;
   
   pipe_mutex_unlock( stw_dev->fb_mutex );
}


/**
 * Given an hdc, return the corresponding stw_framebuffer.
 */
static INLINE struct stw_framebuffer *
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
      if (fb->hDC == hdc) {
         pipe_mutex_lock(fb->mutex);
         break;
      }

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

   if (!stw_dev)
      return NULL;

   pipe_mutex_lock( stw_dev->fb_mutex );
   fb = stw_framebuffer_from_hdc_locked(hdc);
   pipe_mutex_unlock( stw_dev->fb_mutex );

   return fb;
}


/**
 * Given an hdc, return the corresponding stw_framebuffer.
 */
struct stw_framebuffer *
stw_framebuffer_from_hwnd(
   HWND hwnd )
{
   struct stw_framebuffer *fb;

   pipe_mutex_lock( stw_dev->fb_mutex );
   fb = stw_framebuffer_from_hwnd_locked(hwnd);
   pipe_mutex_unlock( stw_dev->fb_mutex );

   return fb;
}


BOOL APIENTRY
DrvSetPixelFormat(
   HDC hdc,
   LONG iPixelFormat )
{
   uint count;
   uint index;
   struct stw_framebuffer *fb;

   if (!stw_dev)
      return FALSE;

   index = (uint) iPixelFormat - 1;
   count = stw_pixelformat_get_extended_count();
   if (index >= count)
      return FALSE;

   fb = stw_framebuffer_from_hdc_locked(hdc);
   if(fb) {
      /* SetPixelFormat must be called only once */
      stw_framebuffer_release( fb );
      return FALSE;
   }

   fb = stw_framebuffer_create(hdc, iPixelFormat);
   if(!fb) {
      return FALSE;
   }
      
   stw_framebuffer_release( fb );

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
   int iPixelFormat = 0;
   struct stw_framebuffer *fb;

   fb = stw_framebuffer_from_hdc(hdc);
   if(fb) {
      iPixelFormat = fb->iPixelFormat;
      stw_framebuffer_release(fb);
   }
   
   return iPixelFormat;
}


BOOL APIENTRY
DrvPresentBuffers(HDC hdc, PGLPRESENTBUFFERSDATA data)
{
   struct stw_framebuffer *fb;
   struct pipe_screen *screen;
   struct pipe_surface *surface;

   if (!stw_dev)
      return FALSE;

   fb = stw_framebuffer_from_hdc( hdc );
   if (fb == NULL)
      return FALSE;

   screen = stw_dev->screen;

   surface = (struct pipe_surface *)data->pPrivateData;

#ifdef DEBUG
   if(stw_dev->trace_running) {
      screen = trace_screen(screen)->screen;
      surface = trace_surface(surface)->surface;
   }
#endif

   if(data->hSharedSurface != fb->hSharedSurface) {
      if(fb->shared_surface) {
         stw_dev->stw_winsys->shared_surface_close(screen, fb->shared_surface);
         fb->shared_surface = NULL;
      }

      fb->hSharedSurface = data->hSharedSurface;

      if(data->hSharedSurface &&
         stw_dev->stw_winsys->shared_surface_open) {
         fb->shared_surface = stw_dev->stw_winsys->shared_surface_open(screen, fb->hSharedSurface);
      }
   }

   if(fb->shared_surface) {
      stw_dev->stw_winsys->compose(screen,
                                   surface,
                                   fb->shared_surface,
                                   &fb->client_rect,
                                   data->PresentHistoryToken);
   }
   else {
      stw_dev->stw_winsys->present( screen, surface, hdc );
   }

   stw_framebuffer_update(fb);

   stw_framebuffer_release(fb);

   return TRUE;
}


/**
 * Queue a composition.
 *
 * It will drop the lock on success.
 */
BOOL
stw_framebuffer_present_locked(HDC hdc,
                               struct stw_framebuffer *fb,
                               struct pipe_surface *surface)
{
   if(stw_dev->callbacks.wglCbPresentBuffers &&
      stw_dev->stw_winsys->compose) {
      GLCBPRESENTBUFFERSDATA data;

      memset(&data, 0, sizeof data);
      data.magic1 = 2;
      data.magic2 = 0;
      data.AdapterLuid = stw_dev->AdapterLuid;
      data.rect = fb->client_rect;
      data.pPrivateData = (void *)surface;

      stw_framebuffer_release(fb);

      return stw_dev->callbacks.wglCbPresentBuffers(hdc, &data);
   }
   else {
      struct pipe_screen *screen = stw_dev->screen;

#ifdef DEBUG
      if(stw_dev->trace_running) {
         screen = trace_screen(screen)->screen;
         surface = trace_surface(surface)->surface;
      }
#endif

      stw_dev->stw_winsys->present( screen, surface, hdc );

      stw_framebuffer_update(fb);

      stw_framebuffer_release(fb);

      return TRUE;
   }
}


BOOL APIENTRY
DrvSwapBuffers(
   HDC hdc )
{
   struct stw_framebuffer *fb;
   struct pipe_surface *surface = NULL;

   if (!stw_dev)
      return FALSE;

   fb = stw_framebuffer_from_hdc( hdc );
   if (fb == NULL)
      return FALSE;

   if (!(fb->pfi->pfd.dwFlags & PFD_DOUBLEBUFFER)) {
      stw_framebuffer_release(fb);
      return TRUE;
   }

   st_swapbuffers(fb->stfb, &surface, NULL);

   return stw_framebuffer_present_locked(hdc, fb, surface);
}


BOOL APIENTRY
DrvSwapLayerBuffers(
   HDC hdc,
   UINT fuPlanes )
{
   if(fuPlanes & WGL_SWAP_MAIN_PLANE)
      return DrvSwapBuffers(hdc);

   return FALSE;
}
