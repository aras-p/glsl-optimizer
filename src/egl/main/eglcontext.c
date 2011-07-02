/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright 2009-2010 Chia-I Wu <olvaffe@gmail.com>
 * Copyright 2010-2011 LunarG, Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "eglconfig.h"
#include "eglcontext.h"
#include "egldisplay.h"
#include "eglcurrent.h"
#include "eglsurface.h"
#include "egllog.h"


/**
 * Return the API bit (one of EGL_xxx_BIT) of the context.
 */
static EGLint
_eglGetContextAPIBit(_EGLContext *ctx)
{
   EGLint bit = 0;

   switch (ctx->ClientAPI) {
   case EGL_OPENGL_ES_API:
      switch (ctx->ClientVersion) {
      case 1:
         bit = EGL_OPENGL_ES_BIT;
         break;
      case 2:
         bit = EGL_OPENGL_ES2_BIT;
         break;
      default:
         break;
      }
      break;
   case EGL_OPENVG_API:
      bit = EGL_OPENVG_BIT;
      break;
   case EGL_OPENGL_API:
      bit = EGL_OPENGL_BIT;
      break;
   default:
      break;
   }

   return bit;
}


/**
 * Parse the list of context attributes and return the proper error code.
 */
static EGLint
_eglParseContextAttribList(_EGLContext *ctx, const EGLint *attrib_list)
{
   EGLenum api = ctx->ClientAPI;
   EGLint i, err = EGL_SUCCESS;

   if (!attrib_list)
      return EGL_SUCCESS;

   for (i = 0; attrib_list[i] != EGL_NONE; i++) {
      EGLint attr = attrib_list[i++];
      EGLint val = attrib_list[i];

      switch (attr) {
      case EGL_CONTEXT_CLIENT_VERSION:
         if (api != EGL_OPENGL_ES_API) {
            err = EGL_BAD_ATTRIBUTE;
            break;
         }
         if (val != 1 && val != 2) {
            err = EGL_BAD_ATTRIBUTE;
            break;
         }
         ctx->ClientVersion = val;
         break;
      default:
         err = EGL_BAD_ATTRIBUTE;
         break;
      }

      if (err != EGL_SUCCESS) {
         _eglLog(_EGL_DEBUG, "bad context attribute 0x%04x", attr);
         break;
      }
   }

   return err;
}


/**
 * Initialize the given _EGLContext object to defaults and/or the values
 * in the attrib_list.
 */
EGLBoolean
_eglInitContext(_EGLContext *ctx, _EGLDisplay *dpy, _EGLConfig *conf,
                const EGLint *attrib_list)
{
   const EGLenum api = eglQueryAPI();
   EGLint err;

   if (api == EGL_NONE) {
      _eglError(EGL_BAD_MATCH, "eglCreateContext(no client API)");
      return EGL_FALSE;
   }

   _eglInitResource(&ctx->Resource, sizeof(*ctx), dpy);
   ctx->ClientAPI = api;
   ctx->Config = conf;
   ctx->WindowRenderBuffer = EGL_NONE;

   ctx->ClientVersion = 1; /* the default, per EGL spec */

   err = _eglParseContextAttribList(ctx, attrib_list);
   if (err == EGL_SUCCESS && ctx->Config) {
      EGLint api_bit;

      api_bit = _eglGetContextAPIBit(ctx);
      if (!(ctx->Config->RenderableType & api_bit)) {
         _eglLog(_EGL_DEBUG, "context api is 0x%x while config supports 0x%x",
               api_bit, ctx->Config->RenderableType);
         err = EGL_BAD_CONFIG;
      }
   }
   if (err != EGL_SUCCESS)
      return _eglError(err, "eglCreateContext");

   return EGL_TRUE;
}


#ifdef EGL_VERSION_1_2
static EGLint
_eglQueryContextRenderBuffer(_EGLContext *ctx)
{
   _EGLSurface *surf = ctx->DrawSurface;
   EGLint rb;

   if (!surf)
      return EGL_NONE;
   if (surf->Type == EGL_WINDOW_BIT && ctx->WindowRenderBuffer != EGL_NONE)
      rb = ctx->WindowRenderBuffer;
   else
      rb = surf->RenderBuffer;
   return rb;
}
#endif /* EGL_VERSION_1_2 */


EGLBoolean
_eglQueryContext(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *c,
                 EGLint attribute, EGLint *value)
{
   (void) drv;
   (void) dpy;

   if (!value)
      return _eglError(EGL_BAD_PARAMETER, "eglQueryContext");

   switch (attribute) {
   case EGL_CONFIG_ID:
      if (!c->Config)
         return _eglError(EGL_BAD_ATTRIBUTE, "eglQueryContext");
      *value = c->Config->ConfigID;
      break;
   case EGL_CONTEXT_CLIENT_VERSION:
      *value = c->ClientVersion;
      break;
#ifdef EGL_VERSION_1_2
   case EGL_CONTEXT_CLIENT_TYPE:
      *value = c->ClientAPI;
      break;
   case EGL_RENDER_BUFFER:
      *value = _eglQueryContextRenderBuffer(c);
      break;
#endif /* EGL_VERSION_1_2 */
   default:
      return _eglError(EGL_BAD_ATTRIBUTE, "eglQueryContext");
   }

   return EGL_TRUE;
}


/**
 * Bind the context to the thread and return the previous context.
 *
 * Note that the context may be NULL.
 */
static _EGLContext *
_eglBindContextToThread(_EGLContext *ctx, _EGLThreadInfo *t)
{
   EGLint apiIndex;
   _EGLContext *oldCtx;

   apiIndex = (ctx) ?
      _eglConvertApiToIndex(ctx->ClientAPI) : t->CurrentAPIIndex;

   oldCtx = t->CurrentContexts[apiIndex];
   if (ctx != oldCtx) {
      if (oldCtx)
         oldCtx->Binding = NULL;
      if (ctx)
         ctx->Binding = t;

      t->CurrentContexts[apiIndex] = ctx;
   }

   return oldCtx;
}


/**
 * Return true if the given context and surfaces can be made current.
 */
static EGLBoolean
_eglCheckMakeCurrent(_EGLContext *ctx, _EGLSurface *draw, _EGLSurface *read)
{
   _EGLThreadInfo *t = _eglGetCurrentThread();
   _EGLDisplay *dpy;
   EGLint conflict_api;
   EGLBoolean surfaceless;

   if (_eglIsCurrentThreadDummy())
      return _eglError(EGL_BAD_ALLOC, "eglMakeCurrent");

   /* this is easy */
   if (!ctx) {
      if (draw || read)
         return _eglError(EGL_BAD_MATCH, "eglMakeCurrent");
      return EGL_TRUE;
   }

   dpy = ctx->Resource.Display;
   switch (_eglGetContextAPIBit(ctx)) {
   case EGL_OPENGL_ES_BIT:
      surfaceless = dpy->Extensions.KHR_surfaceless_gles1;
      break;
   case EGL_OPENGL_ES2_BIT:
      surfaceless = dpy->Extensions.KHR_surfaceless_gles2;
      break;
   case EGL_OPENGL_BIT:
      surfaceless = dpy->Extensions.KHR_surfaceless_opengl;
      break;
   default:
      surfaceless = EGL_FALSE;
      break;
   }

   if (!surfaceless && (draw == NULL || read == NULL))
      return _eglError(EGL_BAD_MATCH, "eglMakeCurrent");

   /*
    * The spec says
    *
    * "If ctx is current to some other thread, or if either draw or read are
    * bound to contexts in another thread, an EGL_BAD_ACCESS error is
    * generated."
    *
    * and
    *
    * "at most one context may be bound to a particular surface at a given
    * time"
    */
   if (ctx->Binding && ctx->Binding != t)
      return _eglError(EGL_BAD_ACCESS, "eglMakeCurrent");
   if (draw && draw->CurrentContext && draw->CurrentContext != ctx) {
      if (draw->CurrentContext->Binding != t ||
          draw->CurrentContext->ClientAPI != ctx->ClientAPI)
         return _eglError(EGL_BAD_ACCESS, "eglMakeCurrent");
   }
   if (read && read->CurrentContext && read->CurrentContext != ctx) {
      if (read->CurrentContext->Binding != t ||
          read->CurrentContext->ClientAPI != ctx->ClientAPI)
         return _eglError(EGL_BAD_ACCESS, "eglMakeCurrent");
   }

   /* simply require the configs to be equal */
   if ((draw && draw->Config != ctx->Config) ||
       (read && read->Config != ctx->Config))
      return _eglError(EGL_BAD_MATCH, "eglMakeCurrent");

   switch (ctx->ClientAPI) {
#ifdef EGL_VERSION_1_4
   /* OpenGL and OpenGL ES are conflicting */
   case EGL_OPENGL_ES_API:
      conflict_api = EGL_OPENGL_API;
      break;
   case EGL_OPENGL_API:
      conflict_api = EGL_OPENGL_ES_API;
      break;
#endif
   default:
      conflict_api = -1;
      break;
   }

   if (conflict_api >= 0 && _eglGetAPIContext(conflict_api))
      return _eglError(EGL_BAD_ACCESS, "eglMakeCurrent");

   return EGL_TRUE;
}


/**
 * Bind the context to the current thread and given surfaces.  Return the
 * previous bound context and surfaces.  The caller should unreference the
 * returned context and surfaces.
 *
 * Making a second call with the resources returned by the first call
 * unsurprisingly undoes the first call, except for the resouce reference
 * counts.
 */
EGLBoolean
_eglBindContext(_EGLContext *ctx, _EGLSurface *draw, _EGLSurface *read,
                _EGLContext **old_ctx,
                _EGLSurface **old_draw, _EGLSurface **old_read)
{
   _EGLThreadInfo *t = _eglGetCurrentThread();
   _EGLContext *prev_ctx;
   _EGLSurface *prev_draw, *prev_read;

   if (!_eglCheckMakeCurrent(ctx, draw, read))
      return EGL_FALSE;

   /* increment refcounts before binding */
   _eglGetContext(ctx);
   _eglGetSurface(draw);
   _eglGetSurface(read);

   /* bind the new context */
   prev_ctx = _eglBindContextToThread(ctx, t);

   /* break previous bindings */
   if (prev_ctx) {
      prev_draw = prev_ctx->DrawSurface;
      prev_read = prev_ctx->ReadSurface;

      if (prev_draw)
         prev_draw->CurrentContext = NULL;
      if (prev_read)
         prev_read->CurrentContext = NULL;

      prev_ctx->DrawSurface = NULL;
      prev_ctx->ReadSurface = NULL;
   }
   else {
      prev_draw = prev_read = NULL;
   }

   /* establish new bindings */
   if (ctx) {
      if (draw)
         draw->CurrentContext = ctx;
      if (read)
         read->CurrentContext = ctx;

      ctx->DrawSurface = draw;
      ctx->ReadSurface = read;
   }

   assert(old_ctx && old_draw && old_read);
   *old_ctx = prev_ctx;
   *old_draw = prev_draw;
   *old_read = prev_read;

   return EGL_TRUE;
}
