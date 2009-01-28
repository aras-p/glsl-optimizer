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

#include "main/mtypes.h"
#include "main/context.h"
#include "pipe/p_compiler.h"
#include "pipe/p_context.h"
#include "state_tracker/st_context.h"
#include "state_tracker/st_public.h"
#include "shared/stw_device.h"
#include "shared/stw_winsys.h"
#include "shared/stw_framebuffer.h"
#include "shared/stw_pixelformat.h"
#include "wgl/stw_wgl_arbmultisample.h"
#include "stw_context.h"
//#include "stw_wgl.h"

static struct wgl_context *ctx_head = NULL;

static HDC current_hdc = NULL;
static HGLRC current_hrc = NULL;

BOOL
stw_copy_context(
   HGLRC hglrcSrc,
   HGLRC hglrcDst,
   UINT mask )
{
   (void) hglrcSrc;
   (void) hglrcDst;
   (void) mask;

   return FALSE;
}

HGLRC
stw_create_context(
   HDC hdc,
   int iLayerPlane )
{
   uint pfi;
   const struct pixelformat_info *pf;
   struct wgl_context *ctx;
   GLvisual *visual;
   struct pipe_context *pipe;

   if (iLayerPlane != 0)
      return NULL;

   pfi = wglGetPixelFormat( hdc );
   if (pfi == 0)
      return NULL;

   pf = pixelformat_get_info( pfi - 1 );

   ctx = CALLOC_STRUCT( wgl_context );
   if (ctx == NULL)
      return NULL;

   ctx->hdc = hdc;
   ctx->color_bits = GetDeviceCaps( ctx->hdc, BITSPIXEL );

   /* Create visual based on flags
    */
   visual = _mesa_create_visual(
      GL_TRUE,
      (pf->flags & PF_FLAG_DOUBLEBUFFER) ? GL_TRUE : GL_FALSE,
      GL_FALSE,
      pf->color.redbits,
      pf->color.greenbits,
      pf->color.bluebits,
      pf->alpha.alphabits,
      0,
      pf->depth.depthbits,
      pf->depth.stencilbits,
      0,
      0,
      0,
      0,
      (pf->flags & PF_FLAG_MULTISAMPLED) ? wgl_query_samples() : 0 );
   if (visual == NULL) {
      FREE( ctx );
      return NULL;
   }

   pipe = stw_dev->stw_winsys->create_context( stw_dev->screen );
   if (!pipe) {
      _mesa_destroy_visual( visual );
      FREE( ctx );
      return NULL;
   }
   
   assert(!pipe->priv);
   pipe->priv = hdc;

   ctx->st = st_create_context( pipe, visual, NULL );
   if (ctx->st == NULL) {
      pipe->destroy( pipe );
      _mesa_destroy_visual( visual );
      FREE( ctx );
      return NULL;
   }
   ctx->st->ctx->DriverCtx = ctx;

   ctx->next = ctx_head;
   ctx_head = ctx;

   return (HGLRC) ctx;
}


BOOL
stw_delete_context(
   HGLRC hglrc )
{
   struct wgl_context **link = &ctx_head;
   struct wgl_context *ctx = ctx_head;

   while (ctx != NULL) {
      if (ctx == (struct wgl_context *) hglrc) {
         GLcontext *glctx = ctx->st->ctx;
         GET_CURRENT_CONTEXT( glcurctx );
         struct stw_framebuffer *fb;

         /* Unbind current if deleting current context.
          */
         if (glcurctx == glctx)
            st_make_current( NULL, NULL, NULL );

         fb = framebuffer_from_hdc( ctx->hdc );
         if (fb)
            framebuffer_destroy( fb );

         if (WindowFromDC( ctx->hdc ) != NULL)
            ReleaseDC( WindowFromDC( ctx->hdc ), ctx->hdc );

         st_destroy_context( ctx->st );

         *link = ctx->next;
         FREE( ctx );
         return TRUE;
      }

      link = &ctx->next;
      ctx = ctx->next;
   }

   return FALSE;
}

/* Find the width and height of the window named by hdc.
 */
static void
get_window_size( HDC hdc, GLuint *width, GLuint *height )
{
   if (WindowFromDC( hdc )) {
      RECT rect;

      GetClientRect( WindowFromDC( hdc ), &rect );
      *width = rect.right - rect.left;
      *height = rect.bottom - rect.top;
   }
   else {
      *width = GetDeviceCaps( hdc, HORZRES );
      *height = GetDeviceCaps( hdc, VERTRES );
   }
}

HGLRC
stw_get_current_context( void )
{
   return current_hrc;
}

HDC
stw_get_current_dc( void )
{
    return current_hdc;
}

BOOL
stw_make_current(
   HDC hdc,
   HGLRC hglrc )
{
   struct wgl_context *ctx = ctx_head;
   GET_CURRENT_CONTEXT( glcurctx );
   struct stw_framebuffer *fb;
   GLuint width = 0;
   GLuint height = 0;

   current_hdc = hdc;
   current_hrc = hglrc;

   if (hdc == NULL || hglrc == NULL) {
      st_make_current( NULL, NULL, NULL );
      return TRUE;
   }

   while (ctx != NULL) {
      if (ctx == (struct wgl_context *) hglrc)
         break;
      ctx = ctx->next;
   }
   if (ctx == NULL)
      return FALSE;

   /* Return if already current.
    */
   if (glcurctx != NULL) {
      struct wgl_context *curctx = (struct wgl_context *) glcurctx->DriverCtx;

      if (curctx != NULL && curctx == ctx && ctx->hdc == hdc)
         return TRUE;
   }

   fb = framebuffer_from_hdc( hdc );

   if (hdc != NULL)
      get_window_size( hdc, &width, &height );

   /* Lazy creation of framebuffers.
    */
   if (fb == NULL && ctx != NULL && hdc != NULL) {
      GLvisual *visual = &ctx->st->ctx->Visual;

      fb = framebuffer_create( hdc, visual, width, height );
      if (fb == NULL)
         return FALSE;

      fb->dib_hDC = CreateCompatibleDC( hdc );
      fb->hbmDIB = NULL;
      fb->pbPixels = NULL;
   }

   if (ctx && fb) {
      st_make_current( ctx->st, fb->stfb, fb->stfb );
      framebuffer_resize( fb, width, height );
      ctx->hdc = hdc;
      ctx->st->pipe->priv = hdc;
   }
   else {
      /* Detach */
      st_make_current( NULL, NULL, NULL );
   }

   return TRUE;
}

struct wgl_context *
wgl_context_from_hdc(
   HDC hdc )
{
   struct wgl_context *ctx = ctx_head;

   while (ctx != NULL) {
      if (ctx->hdc == hdc)
         return ctx;
      ctx = ctx->next;
   }
   return NULL;
}



