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

#ifdef DEBUG
#include "trace/tr_screen.h"
#include "trace/tr_context.h"
#endif

#include "shared/stw_device.h"
#include "shared/stw_winsys.h"
#include "shared/stw_framebuffer.h"
#include "shared/stw_pixelformat.h"
#include "stw_public.h"
#include "stw_context.h"
#include "stw_tls.h"

BOOL
stw_copy_context(
   UINT_PTR hglrcSrc,
   UINT_PTR hglrcDst,
   UINT mask )
{
   struct stw_context *src;
   struct stw_context *dst;
   BOOL ret = FALSE;

   pipe_mutex_lock( stw_dev->mutex );
   
   src = stw_lookup_context_locked( hglrcSrc );
   dst = stw_lookup_context_locked( hglrcDst );

   if (src && dst) { 
      /* FIXME */
      (void) src;
      (void) dst;
      (void) mask;
   }

   pipe_mutex_unlock( stw_dev->mutex );
   
   return ret;
}

UINT_PTR
stw_create_layer_context(
   HDC hdc,
   int iLayerPlane )
{
   uint pfi;
   const struct stw_pixelformat_info *pf = NULL;
   struct stw_context *ctx = NULL;
   GLvisual *visual = NULL;
   struct pipe_screen *screen = NULL;
   struct pipe_context *pipe = NULL;
   UINT_PTR hglrc = 0;

   if(!stw_dev)
      return 0;
   
   if (iLayerPlane != 0)
      return 0;

   pfi = stw_pixelformat_get( hdc );
   if (pfi == 0)
      return 0;

   pf = stw_pixelformat_get_info( pfi - 1 );

   ctx = CALLOC_STRUCT( stw_context );
   if (ctx == NULL)
      return 0;

   ctx->hdc = hdc;
   ctx->color_bits = GetDeviceCaps( ctx->hdc, BITSPIXEL );

   /* Create visual based on flags
    */
   visual = _mesa_create_visual(
      GL_TRUE,
      (pf->flags & STW_PF_FLAG_DOUBLEBUFFER) ? GL_TRUE : GL_FALSE,
      GL_FALSE,
      pf->color.redbits,
      pf->color.greenbits,
      pf->color.bluebits,
      pf->color.alphabits,
      0,
      pf->depth.depthbits,
      pf->depth.stencilbits,
      0,
      0,
      0,
      0,
      (pf->flags & STW_PF_FLAG_MULTISAMPLED) ? stw_query_samples() : 0 );
   if (visual == NULL) 
      goto fail;

   screen = stw_dev->screen;

#ifdef DEBUG
   /* Unwrap screen */
   if(stw_dev->trace_running)
      screen = trace_screen(screen)->screen;
#endif

   pipe = stw_dev->stw_winsys->create_context( screen );
   if (pipe == NULL) 
      goto fail;

#ifdef DEBUG
   /* Wrap context */
   if(stw_dev->trace_running)
      pipe = trace_context_create(stw_dev->screen, pipe);
#endif

   assert(!pipe->priv);
   pipe->priv = hdc;

   ctx->st = st_create_context( pipe, visual, NULL );
   if (ctx->st == NULL) 
      goto fail;

   ctx->st->ctx->DriverCtx = ctx;

   pipe_mutex_lock( stw_dev->mutex );
   hglrc = handle_table_add(stw_dev->ctx_table, ctx);
   pipe_mutex_unlock( stw_dev->mutex );

   /* Success?
    */
   if (hglrc != 0)
      return hglrc;

fail:
   if (visual)
      _mesa_destroy_visual( visual );
   
   if (pipe)
      pipe->destroy( pipe );
      
   FREE( ctx );
   return 0;
}

BOOL
stw_delete_context(
   UINT_PTR hglrc )
{
   struct stw_context *ctx ;
   BOOL ret = FALSE;
   
   if (!stw_dev)
      return FALSE;

   pipe_mutex_lock( stw_dev->mutex );
   ctx = stw_lookup_context_locked(hglrc);
   handle_table_remove(stw_dev->ctx_table, hglrc);
   pipe_mutex_unlock( stw_dev->mutex );

   if (ctx) {
      GLcontext *glctx = ctx->st->ctx;
      GET_CURRENT_CONTEXT( glcurctx );
      struct stw_framebuffer *fb;

      /* Unbind current if deleting current context.
       */
      if (glcurctx == glctx)
         st_make_current( NULL, NULL, NULL );

      fb = stw_framebuffer_from_hdc( ctx->hdc );
      if (fb)
         stw_framebuffer_destroy( fb );

      if (WindowFromDC( ctx->hdc ) != NULL)
         ReleaseDC( WindowFromDC( ctx->hdc ), ctx->hdc );

      st_destroy_context(ctx->st);
      FREE(ctx);

      ret = TRUE;
   }

   return ret;
}

BOOL
stw_release_context(
   UINT_PTR hglrc )
{
   struct stw_context *ctx;

   if (!stw_dev)
      return FALSE;

   pipe_mutex_lock( stw_dev->mutex );
   ctx = stw_lookup_context_locked( hglrc );
   pipe_mutex_unlock( stw_dev->mutex );

   if (!ctx)
      return FALSE;
   
   /* The expectation is that ctx is the same context which is
    * current for this thread.  We should check that and return False
    * if not the case.
    */
   {
      GLcontext *glctx = ctx->st->ctx;
      GET_CURRENT_CONTEXT( glcurctx );

      if (glcurctx != glctx)
         return FALSE;
   }

   if (stw_make_current( NULL, 0 ) == FALSE)
      return FALSE;

   return TRUE;
}

/* Find the width and height of the window named by hdc.
 */
static void
stw_get_window_size( HDC hdc, GLuint *width, GLuint *height )
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

UINT_PTR
stw_get_current_context( void )
{
   return stw_tls_get_data()->currentGLRC;
}

HDC
stw_get_current_dc( void )
{
    return stw_tls_get_data()->currentDC;
}

BOOL
stw_make_current(
   HDC hdc,
   UINT_PTR hglrc )
{
   struct stw_context *ctx;
   GET_CURRENT_CONTEXT( glcurctx );
   struct stw_framebuffer *fb;
   GLuint width = 0;
   GLuint height = 0;
   struct stw_context *curctx;

   if (!stw_dev)
      return FALSE;

   pipe_mutex_lock( stw_dev->mutex ); 
   ctx = stw_lookup_context_locked( hglrc );
   pipe_mutex_unlock( stw_dev->mutex );

   stw_tls_get_data()->currentDC = hdc;
   stw_tls_get_data()->currentGLRC = hglrc;

   if (glcurctx != NULL) {
      curctx = (struct stw_context *) glcurctx->DriverCtx;

      if (curctx != ctx)
	 st_flush(glcurctx->st, PIPE_FLUSH_RENDER_CACHE, NULL);
   }

   if (hdc == NULL || hglrc == 0) {
      st_make_current( NULL, NULL, NULL );
      return TRUE;
   }

   /* Return if already current.
    */
   if (glcurctx != NULL) {
      if (curctx != NULL && curctx == ctx && ctx->hdc == hdc)
         return TRUE;
   }

   fb = stw_framebuffer_from_hdc( hdc );

   if (hdc != NULL)
      stw_get_window_size( hdc, &width, &height );

   /* Lazy creation of stw_framebuffers.
    */
   if (fb == NULL && ctx != NULL && hdc != NULL) {
      GLvisual *visual = &ctx->st->ctx->Visual;

      fb = stw_framebuffer_create( hdc, visual, width, height );
      if (fb == NULL)
         return FALSE;
   }

   if (ctx && fb) {
      st_make_current( ctx->st, fb->stfb, fb->stfb );
      stw_framebuffer_resize( fb, width, height );
      ctx->hdc = hdc;
      ctx->st->pipe->priv = hdc;
   }
   else {
      /* Detach */
      st_make_current( NULL, NULL, NULL );
   }

   return TRUE;
}
