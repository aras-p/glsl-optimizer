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

   memset(ctx, 0, sizeof(_EGLContext));
   ctx->Resource.Display = dpy;
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


/**
 * Just a placeholder/demo function.  Real driver will never use this!
 */
_EGLContext *
_eglCreateContext(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf,
                  _EGLContext *share_list, const EGLint *attrib_list)
{
   return NULL;
}


/**
 * Default fallback routine - drivers should usually override this.
 */
EGLBoolean
_eglDestroyContext(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *ctx)
{
   if (!_eglIsContextBound(ctx))
      free(ctx);
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

   /* context stealing from another thread is not allowed */
   if (ctx->Binding && ctx->Binding != t)
      return _eglError(EGL_BAD_ACCESS, "eglMakeCurrent");

   /*
    * The spec says
    *
    * "If ctx is current to some other thread, or if either draw or read are
    * bound to contexts in another thread, an EGL_BAD_ACCESS error is
    * generated."
    *
    * But it also says
    *
    * "at most one context may be bound to a particular surface at a given
    * time"
    *
    * The latter is more restrictive so we can check only the latter case.
    */
   if ((draw && draw->CurrentContext && draw->CurrentContext != ctx) ||
       (read && read->CurrentContext && read->CurrentContext != ctx))
      return _eglError(EGL_BAD_ACCESS, "eglMakeCurrent");

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
 * "orphaned" context and surfaces.  Each argument is both input and output.
 */
EGLBoolean
_eglBindContext(_EGLContext **ctx, _EGLSurface **draw, _EGLSurface **read)
{
   _EGLThreadInfo *t = _eglGetCurrentThread();
   _EGLContext *newCtx = *ctx, *oldCtx;
   _EGLSurface *newDraw = *draw, *newRead = *read;

   if (!_eglCheckMakeCurrent(newCtx, newDraw, newRead))
      return EGL_FALSE;

   /* bind the new context */
   oldCtx = _eglBindContextToThread(newCtx, t);

   /* break old bindings */
   if (oldCtx) {
      *ctx = oldCtx;
      *draw = oldCtx->DrawSurface;
      *read = oldCtx->ReadSurface;

      if (*draw)
         (*draw)->CurrentContext = NULL;
      if (*read)
         (*read)->CurrentContext = NULL;

      oldCtx->DrawSurface = NULL;
      oldCtx->ReadSurface = NULL;
   }

   /* establish new bindings */
   if (newCtx) {
      if (newDraw)
         newDraw->CurrentContext = newCtx;
      if (newRead)
         newRead->CurrentContext = newCtx;

      newCtx->DrawSurface = newDraw;
      newCtx->ReadSurface = newRead;
   }

   /* an old context or surface is not orphaned if it is still bound */
   if (*ctx == newCtx)
      *ctx = NULL;
   if (*draw == newDraw || *draw == newRead)
      *draw = NULL;
   if (*read == newDraw || *read == newRead)
      *read = NULL;

   return EGL_TRUE;
}


/**
 * Just a placeholder/demo function.  Drivers should override this.
 */
EGLBoolean
_eglMakeCurrent(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *draw,
                _EGLSurface *read, _EGLContext *ctx)
{
   return EGL_FALSE;
}


/**
 * This is defined by the EGL_MESA_copy_context extension.
 */
EGLBoolean
_eglCopyContextMESA(_EGLDriver *drv, EGLDisplay dpy, EGLContext source,
                    EGLContext dest, EGLint mask)
{
   /* This function will always have to be overridden/implemented in the
    * device driver.  If the driver is based on Mesa, use _mesa_copy_context().
    */
   return EGL_FALSE;
}
