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


static INLINE struct stw_context *
stw_context(GLcontext *glctx)
{
   if(!glctx)
      return NULL;
   assert(glctx->DriverCtx);
   return (struct stw_context *)glctx->DriverCtx;
}

static INLINE struct stw_context *
stw_current_context(void)
{
   /* We must check if multiple threads are being used or GET_CURRENT_CONTEXT 
    * might return the current context of the thread first seen. */
   _glapi_check_multithread();

   {
      GET_CURRENT_CONTEXT( glctx );
      return stw_context(glctx);
   }
}

BOOL
stw_copy_context(
   UINT_PTR hglrcSrc,
   UINT_PTR hglrcDst,
   UINT mask )
{
   struct stw_context *src;
   struct stw_context *dst;
   BOOL ret = FALSE;

   pipe_mutex_lock( stw_dev->ctx_mutex );
   
   src = stw_lookup_context_locked( hglrcSrc );
   dst = stw_lookup_context_locked( hglrcDst );

   if (src && dst) { 
      /* FIXME */
      assert(0);
      (void) src;
      (void) dst;
      (void) mask;
   }

   pipe_mutex_unlock( stw_dev->ctx_mutex );
   
   return ret;
}

BOOL
stw_share_lists(
   UINT_PTR hglrc1, 
   UINT_PTR hglrc2 )
{
   struct stw_context *ctx1;
   struct stw_context *ctx2;
   BOOL ret = FALSE;

   pipe_mutex_lock( stw_dev->ctx_mutex );
   
   ctx1 = stw_lookup_context_locked( hglrc1 );
   ctx2 = stw_lookup_context_locked( hglrc2 );

   if (ctx1 && ctx2 &&
       ctx1->iPixelFormat == ctx2->iPixelFormat) { 
      ret = _mesa_share_state(ctx2->st->ctx, ctx1->st->ctx);
   }

   pipe_mutex_unlock( stw_dev->ctx_mutex );
   
   return ret;
}

static void
stw_viewport(GLcontext * glctx, GLint x, GLint y,
             GLsizei width, GLsizei height)
{
   struct stw_context *ctx = (struct stw_context *)glctx->DriverCtx;
   struct stw_framebuffer *fb;
   
   fb = stw_framebuffer_from_hdc( ctx->hdc );
   if(fb) {
      stw_framebuffer_update(fb);
      stw_framebuffer_release(fb);
   }
}

UINT_PTR
stw_create_layer_context(
   HDC hdc,
   int iLayerPlane )
{
   int iPixelFormat;
   const struct stw_pixelformat_info *pfi;
   GLvisual visual;
   struct stw_context *ctx = NULL;
   struct pipe_screen *screen = NULL;
   struct pipe_context *pipe = NULL;
   
   if(!stw_dev)
      return 0;
   
   if (iLayerPlane != 0)
      return 0;

   iPixelFormat = GetPixelFormat(hdc);
   if(!iPixelFormat)
      return 0;
   
   pfi = stw_pixelformat_get_info( iPixelFormat - 1 );
   stw_pixelformat_visual(&visual, pfi);
   
   ctx = CALLOC_STRUCT( stw_context );
   if (ctx == NULL)
      goto no_ctx;

   ctx->hdc = hdc;
   ctx->iPixelFormat = iPixelFormat;

   screen = stw_dev->screen;

#ifdef DEBUG
   /* Unwrap screen */
   if(stw_dev->trace_running)
      screen = trace_screen(screen)->screen;
#endif

   pipe = stw_dev->stw_winsys->create_context( screen );
   if (pipe == NULL) 
      goto no_pipe;

#ifdef DEBUG
   /* Wrap context */
   if(stw_dev->trace_running)
      pipe = trace_context_create(stw_dev->screen, pipe);
#endif

   /* pass to stw_flush_frontbuffer as context_private */
   assert(!pipe->priv);
   pipe->priv = hdc;

   ctx->st = st_create_context( pipe, &visual, NULL );
   if (ctx->st == NULL) 
      goto no_st_ctx;

   ctx->st->ctx->DriverCtx = ctx;
   ctx->st->ctx->Driver.Viewport = stw_viewport;

   pipe_mutex_lock( stw_dev->ctx_mutex );
   ctx->hglrc = handle_table_add(stw_dev->ctx_table, ctx);
   pipe_mutex_unlock( stw_dev->ctx_mutex );
   if (!ctx->hglrc)
      goto no_hglrc;

   return ctx->hglrc;

no_hglrc:
   st_destroy_context(ctx->st);
   goto no_pipe; /* st_context_destroy already destroys pipe */
no_st_ctx:
   pipe->destroy( pipe );
no_pipe:
   FREE(ctx);
no_ctx:
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

   pipe_mutex_lock( stw_dev->ctx_mutex );
   ctx = stw_lookup_context_locked(hglrc);
   handle_table_remove(stw_dev->ctx_table, hglrc);
   pipe_mutex_unlock( stw_dev->ctx_mutex );

   if (ctx) {
      struct stw_context *curctx = stw_current_context();
      
      /* Unbind current if deleting current context. */
      if (curctx == ctx)
         st_make_current( NULL, NULL, NULL );

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

   pipe_mutex_lock( stw_dev->ctx_mutex );
   ctx = stw_lookup_context_locked( hglrc );
   pipe_mutex_unlock( stw_dev->ctx_mutex );

   if (!ctx)
      return FALSE;
   
   /* The expectation is that ctx is the same context which is
    * current for this thread.  We should check that and return False
    * if not the case.
    */
   if (ctx != stw_current_context())
      return FALSE;

   if (stw_make_current( NULL, 0 ) == FALSE)
      return FALSE;

   return TRUE;
}


UINT_PTR
stw_get_current_context( void )
{
   struct stw_context *ctx;

   ctx = stw_current_context();
   if(!ctx)
      return 0;
   
   return ctx->hglrc;
}

HDC
stw_get_current_dc( void )
{
   struct stw_context *ctx;

   ctx = stw_current_context();
   if(!ctx)
      return NULL;
   
   return ctx->hdc;
}

BOOL
stw_make_current(
   HDC hdc,
   UINT_PTR hglrc )
{
   struct stw_context *curctx = NULL;
   struct stw_context *ctx = NULL;
   struct stw_framebuffer *fb = NULL;

   if (!stw_dev)
      goto fail;

   curctx = stw_current_context();
   if (curctx != NULL) {
      if (curctx->hglrc != hglrc)
	 st_flush(curctx->st, PIPE_FLUSH_RENDER_CACHE, NULL);
      
      /* Return if already current. */
      if (curctx->hglrc == hglrc && curctx->hdc == hdc) {
         ctx = curctx;
         fb = stw_framebuffer_from_hdc( hdc );
         goto success;
      }
   }

   if (hdc == NULL || hglrc == 0) {
      return st_make_current( NULL, NULL, NULL );
   }

   pipe_mutex_lock( stw_dev->ctx_mutex ); 
   ctx = stw_lookup_context_locked( hglrc );
   pipe_mutex_unlock( stw_dev->ctx_mutex ); 
   if(!ctx)
      goto fail;

   fb = stw_framebuffer_from_hdc( hdc );
   if(!fb) { 
      /* Applications should call SetPixelFormat before creating a context,
       * but not all do, and the opengl32 runtime seems to use a default pixel
       * format in some cases, so we must create a framebuffer for those here
       */
      int iPixelFormat = GetPixelFormat(hdc);
      if(iPixelFormat)
         fb = stw_framebuffer_create( hdc, iPixelFormat );
      if(!fb) 
         goto fail;
   }
   
   if(fb->iPixelFormat != ctx->iPixelFormat)
      goto fail;

   /* Lazy allocation of the frame buffer */
   if(!stw_framebuffer_allocate(fb))
      goto fail;

   /* Bind the new framebuffer */
   ctx->hdc = hdc;
   
   /* pass to stw_flush_frontbuffer as context_private */
   ctx->st->pipe->priv = hdc;
   
   if(!st_make_current( ctx->st, fb->stfb, fb->stfb ))
      goto fail;

success:
   assert(fb);
   if(fb) {
      stw_framebuffer_update(fb);
      stw_framebuffer_release(fb);
   }
   
   return TRUE;

fail:
   if(fb)
      stw_framebuffer_release(fb);
   st_make_current( NULL, NULL, NULL );
   return FALSE;
}
