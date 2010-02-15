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

#include "stw_icd.h"
#include "stw_device.h"
#include "stw_winsys.h"
#include "stw_framebuffer.h"
#include "stw_pixelformat.h"
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

BOOL APIENTRY
DrvCopyContext(
   DHGLRC dhrcSource,
   DHGLRC dhrcDest,
   UINT fuMask )
{
   struct stw_context *src;
   struct stw_context *dst;
   BOOL ret = FALSE;

   if (!stw_dev)
      return FALSE;

   pipe_mutex_lock( stw_dev->ctx_mutex );
   
   src = stw_lookup_context_locked( dhrcSource );
   dst = stw_lookup_context_locked( dhrcDest );

   if (src && dst) { 
      /* FIXME */
      assert(0);
      (void) src;
      (void) dst;
      (void) fuMask;
   }

   pipe_mutex_unlock( stw_dev->ctx_mutex );
   
   return ret;
}

BOOL APIENTRY
DrvShareLists(
   DHGLRC dhglrc1,
   DHGLRC dhglrc2 )
{
   struct stw_context *ctx1;
   struct stw_context *ctx2;
   BOOL ret = FALSE;

   if (!stw_dev)
      return FALSE;

   pipe_mutex_lock( stw_dev->ctx_mutex );
   
   ctx1 = stw_lookup_context_locked( dhglrc1 );
   ctx2 = stw_lookup_context_locked( dhglrc2 );

   if (ctx1 && ctx2) {
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

DHGLRC APIENTRY
DrvCreateContext(
   HDC hdc )
{
   return DrvCreateLayerContext( hdc, 0 );
}

DHGLRC APIENTRY
DrvCreateLayerContext(
   HDC hdc,
   INT iLayerPlane )
{
   int iPixelFormat;
   const struct stw_pixelformat_info *pfi;
   GLvisual visual;
   struct stw_context *ctx = NULL;
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

   /* priv == hdc, pass to stw_flush_frontbuffer as context_private
    */
   pipe = stw_dev->screen->context_create( stw_dev->screen, hdc );
   if (pipe == NULL) 
      goto no_pipe;

   ctx->st = st_create_context( pipe, &visual, NULL );
   if (ctx->st == NULL) 
      goto no_st_ctx;

   ctx->st->ctx->DriverCtx = ctx;
   ctx->st->ctx->Driver.Viewport = stw_viewport;

   pipe_mutex_lock( stw_dev->ctx_mutex );
   ctx->dhglrc = handle_table_add(stw_dev->ctx_table, ctx);
   pipe_mutex_unlock( stw_dev->ctx_mutex );
   if (!ctx->dhglrc)
      goto no_hglrc;

   return ctx->dhglrc;

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

BOOL APIENTRY
DrvDeleteContext(
   DHGLRC dhglrc )
{
   struct stw_context *ctx ;
   BOOL ret = FALSE;
   
   if (!stw_dev)
      return FALSE;

   pipe_mutex_lock( stw_dev->ctx_mutex );
   ctx = stw_lookup_context_locked(dhglrc);
   handle_table_remove(stw_dev->ctx_table, dhglrc);
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

BOOL APIENTRY
DrvReleaseContext(
   DHGLRC dhglrc )
{
   struct stw_context *ctx;

   if (!stw_dev)
      return FALSE;

   pipe_mutex_lock( stw_dev->ctx_mutex );
   ctx = stw_lookup_context_locked( dhglrc );
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


DHGLRC
stw_get_current_context( void )
{
   struct stw_context *ctx;

   ctx = stw_current_context();
   if(!ctx)
      return 0;
   
   return ctx->dhglrc;
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
   DHGLRC dhglrc )
{
   struct stw_context *curctx = NULL;
   struct stw_context *ctx = NULL;
   struct stw_framebuffer *fb = NULL;

   if (!stw_dev)
      goto fail;

   curctx = stw_current_context();
   if (curctx != NULL) {
      if (curctx->dhglrc != dhglrc)
	 st_flush(curctx->st, PIPE_FLUSH_RENDER_CACHE, NULL);
      
      /* Return if already current. */
      if (curctx->dhglrc == dhglrc && curctx->hdc == hdc) {
         ctx = curctx;
         fb = stw_framebuffer_from_hdc( hdc );
         goto success;
      }
   }

   if (hdc == NULL || dhglrc == 0) {
      return st_make_current( NULL, NULL, NULL );
   }

   pipe_mutex_lock( stw_dev->ctx_mutex ); 
   ctx = stw_lookup_context_locked( dhglrc );
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

/**
 * Although WGL allows different dispatch entrypoints per context
 */
static const GLCLTPROCTABLE cpt =
{
   OPENGL_VERSION_110_ENTRIES,
   {
      &glNewList,
      &glEndList,
      &glCallList,
      &glCallLists,
      &glDeleteLists,
      &glGenLists,
      &glListBase,
      &glBegin,
      &glBitmap,
      &glColor3b,
      &glColor3bv,
      &glColor3d,
      &glColor3dv,
      &glColor3f,
      &glColor3fv,
      &glColor3i,
      &glColor3iv,
      &glColor3s,
      &glColor3sv,
      &glColor3ub,
      &glColor3ubv,
      &glColor3ui,
      &glColor3uiv,
      &glColor3us,
      &glColor3usv,
      &glColor4b,
      &glColor4bv,
      &glColor4d,
      &glColor4dv,
      &glColor4f,
      &glColor4fv,
      &glColor4i,
      &glColor4iv,
      &glColor4s,
      &glColor4sv,
      &glColor4ub,
      &glColor4ubv,
      &glColor4ui,
      &glColor4uiv,
      &glColor4us,
      &glColor4usv,
      &glEdgeFlag,
      &glEdgeFlagv,
      &glEnd,
      &glIndexd,
      &glIndexdv,
      &glIndexf,
      &glIndexfv,
      &glIndexi,
      &glIndexiv,
      &glIndexs,
      &glIndexsv,
      &glNormal3b,
      &glNormal3bv,
      &glNormal3d,
      &glNormal3dv,
      &glNormal3f,
      &glNormal3fv,
      &glNormal3i,
      &glNormal3iv,
      &glNormal3s,
      &glNormal3sv,
      &glRasterPos2d,
      &glRasterPos2dv,
      &glRasterPos2f,
      &glRasterPos2fv,
      &glRasterPos2i,
      &glRasterPos2iv,
      &glRasterPos2s,
      &glRasterPos2sv,
      &glRasterPos3d,
      &glRasterPos3dv,
      &glRasterPos3f,
      &glRasterPos3fv,
      &glRasterPos3i,
      &glRasterPos3iv,
      &glRasterPos3s,
      &glRasterPos3sv,
      &glRasterPos4d,
      &glRasterPos4dv,
      &glRasterPos4f,
      &glRasterPos4fv,
      &glRasterPos4i,
      &glRasterPos4iv,
      &glRasterPos4s,
      &glRasterPos4sv,
      &glRectd,
      &glRectdv,
      &glRectf,
      &glRectfv,
      &glRecti,
      &glRectiv,
      &glRects,
      &glRectsv,
      &glTexCoord1d,
      &glTexCoord1dv,
      &glTexCoord1f,
      &glTexCoord1fv,
      &glTexCoord1i,
      &glTexCoord1iv,
      &glTexCoord1s,
      &glTexCoord1sv,
      &glTexCoord2d,
      &glTexCoord2dv,
      &glTexCoord2f,
      &glTexCoord2fv,
      &glTexCoord2i,
      &glTexCoord2iv,
      &glTexCoord2s,
      &glTexCoord2sv,
      &glTexCoord3d,
      &glTexCoord3dv,
      &glTexCoord3f,
      &glTexCoord3fv,
      &glTexCoord3i,
      &glTexCoord3iv,
      &glTexCoord3s,
      &glTexCoord3sv,
      &glTexCoord4d,
      &glTexCoord4dv,
      &glTexCoord4f,
      &glTexCoord4fv,
      &glTexCoord4i,
      &glTexCoord4iv,
      &glTexCoord4s,
      &glTexCoord4sv,
      &glVertex2d,
      &glVertex2dv,
      &glVertex2f,
      &glVertex2fv,
      &glVertex2i,
      &glVertex2iv,
      &glVertex2s,
      &glVertex2sv,
      &glVertex3d,
      &glVertex3dv,
      &glVertex3f,
      &glVertex3fv,
      &glVertex3i,
      &glVertex3iv,
      &glVertex3s,
      &glVertex3sv,
      &glVertex4d,
      &glVertex4dv,
      &glVertex4f,
      &glVertex4fv,
      &glVertex4i,
      &glVertex4iv,
      &glVertex4s,
      &glVertex4sv,
      &glClipPlane,
      &glColorMaterial,
      &glCullFace,
      &glFogf,
      &glFogfv,
      &glFogi,
      &glFogiv,
      &glFrontFace,
      &glHint,
      &glLightf,
      &glLightfv,
      &glLighti,
      &glLightiv,
      &glLightModelf,
      &glLightModelfv,
      &glLightModeli,
      &glLightModeliv,
      &glLineStipple,
      &glLineWidth,
      &glMaterialf,
      &glMaterialfv,
      &glMateriali,
      &glMaterialiv,
      &glPointSize,
      &glPolygonMode,
      &glPolygonStipple,
      &glScissor,
      &glShadeModel,
      &glTexParameterf,
      &glTexParameterfv,
      &glTexParameteri,
      &glTexParameteriv,
      &glTexImage1D,
      &glTexImage2D,
      &glTexEnvf,
      &glTexEnvfv,
      &glTexEnvi,
      &glTexEnviv,
      &glTexGend,
      &glTexGendv,
      &glTexGenf,
      &glTexGenfv,
      &glTexGeni,
      &glTexGeniv,
      &glFeedbackBuffer,
      &glSelectBuffer,
      &glRenderMode,
      &glInitNames,
      &glLoadName,
      &glPassThrough,
      &glPopName,
      &glPushName,
      &glDrawBuffer,
      &glClear,
      &glClearAccum,
      &glClearIndex,
      &glClearColor,
      &glClearStencil,
      &glClearDepth,
      &glStencilMask,
      &glColorMask,
      &glDepthMask,
      &glIndexMask,
      &glAccum,
      &glDisable,
      &glEnable,
      &glFinish,
      &glFlush,
      &glPopAttrib,
      &glPushAttrib,
      &glMap1d,
      &glMap1f,
      &glMap2d,
      &glMap2f,
      &glMapGrid1d,
      &glMapGrid1f,
      &glMapGrid2d,
      &glMapGrid2f,
      &glEvalCoord1d,
      &glEvalCoord1dv,
      &glEvalCoord1f,
      &glEvalCoord1fv,
      &glEvalCoord2d,
      &glEvalCoord2dv,
      &glEvalCoord2f,
      &glEvalCoord2fv,
      &glEvalMesh1,
      &glEvalPoint1,
      &glEvalMesh2,
      &glEvalPoint2,
      &glAlphaFunc,
      &glBlendFunc,
      &glLogicOp,
      &glStencilFunc,
      &glStencilOp,
      &glDepthFunc,
      &glPixelZoom,
      &glPixelTransferf,
      &glPixelTransferi,
      &glPixelStoref,
      &glPixelStorei,
      &glPixelMapfv,
      &glPixelMapuiv,
      &glPixelMapusv,
      &glReadBuffer,
      &glCopyPixels,
      &glReadPixels,
      &glDrawPixels,
      &glGetBooleanv,
      &glGetClipPlane,
      &glGetDoublev,
      &glGetError,
      &glGetFloatv,
      &glGetIntegerv,
      &glGetLightfv,
      &glGetLightiv,
      &glGetMapdv,
      &glGetMapfv,
      &glGetMapiv,
      &glGetMaterialfv,
      &glGetMaterialiv,
      &glGetPixelMapfv,
      &glGetPixelMapuiv,
      &glGetPixelMapusv,
      &glGetPolygonStipple,
      &glGetString,
      &glGetTexEnvfv,
      &glGetTexEnviv,
      &glGetTexGendv,
      &glGetTexGenfv,
      &glGetTexGeniv,
      &glGetTexImage,
      &glGetTexParameterfv,
      &glGetTexParameteriv,
      &glGetTexLevelParameterfv,
      &glGetTexLevelParameteriv,
      &glIsEnabled,
      &glIsList,
      &glDepthRange,
      &glFrustum,
      &glLoadIdentity,
      &glLoadMatrixf,
      &glLoadMatrixd,
      &glMatrixMode,
      &glMultMatrixf,
      &glMultMatrixd,
      &glOrtho,
      &glPopMatrix,
      &glPushMatrix,
      &glRotated,
      &glRotatef,
      &glScaled,
      &glScalef,
      &glTranslated,
      &glTranslatef,
      &glViewport,
      &glArrayElement,
      &glBindTexture,
      &glColorPointer,
      &glDisableClientState,
      &glDrawArrays,
      &glDrawElements,
      &glEdgeFlagPointer,
      &glEnableClientState,
      &glIndexPointer,
      &glIndexub,
      &glIndexubv,
      &glInterleavedArrays,
      &glNormalPointer,
      &glPolygonOffset,
      &glTexCoordPointer,
      &glVertexPointer,
      &glAreTexturesResident,
      &glCopyTexImage1D,
      &glCopyTexImage2D,
      &glCopyTexSubImage1D,
      &glCopyTexSubImage2D,
      &glDeleteTextures,
      &glGenTextures,
      &glGetPointerv,
      &glIsTexture,
      &glPrioritizeTextures,
      &glTexSubImage1D,
      &glTexSubImage2D,
      &glPopClientAttrib,
      &glPushClientAttrib
   }
};

PGLCLTPROCTABLE APIENTRY
DrvSetContext(
   HDC hdc,
   DHGLRC dhglrc,
   PFN_SETPROCTABLE pfnSetProcTable )
{
   PGLCLTPROCTABLE r = (PGLCLTPROCTABLE)&cpt;

   if (!stw_make_current( hdc, dhglrc ))
      r = NULL;

   return r;
}
