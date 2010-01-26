#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "eglconfig.h"
#include "eglcontext.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "eglglobals.h"
#include "eglsurface.h"


/**
 * Initialize the given _EGLContext object to defaults and/or the values
 * in the attrib_list.
 */
EGLBoolean
_eglInitContext(_EGLDriver *drv, _EGLContext *ctx,
                _EGLConfig *conf, const EGLint *attrib_list)
{
   EGLint i;
   const EGLenum api = eglQueryAPI();

   if (api == EGL_NONE) {
      _eglError(EGL_BAD_MATCH, "eglCreateContext(no client API)");
      return EGL_FALSE;
   }

   memset(ctx, 0, sizeof(_EGLContext));

   ctx->ClientVersion = 1; /* the default, per EGL spec */

   for (i = 0; attrib_list && attrib_list[i] != EGL_NONE; i++) {
      switch (attrib_list[i]) {
      case EGL_CONTEXT_CLIENT_VERSION:
         i++;
         ctx->ClientVersion = attrib_list[i];
         break;
      default:
         _eglError(EGL_BAD_ATTRIBUTE, "_eglInitContext");
         return EGL_FALSE;
      }
   }

   ctx->Config = conf;
   ctx->DrawSurface = EGL_NO_SURFACE;
   ctx->ReadSurface = EGL_NO_SURFACE;
   ctx->ClientAPI = api;
   ctx->WindowRenderBuffer = EGL_NONE;

   return EGL_TRUE;
}


/**
 * Just a placeholder/demo function.  Real driver will never use this!
 */
_EGLContext *
_eglCreateContext(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf,
                  _EGLContext *share_list, const EGLint *attrib_list)
{
#if 0 /* example code */
   _EGLContext *context;

   context = (_EGLContext *) calloc(1, sizeof(_EGLContext));
   if (!context)
      return NULL;

   if (!_eglInitContext(drv, context, conf, attrib_list)) {
      free(context);
      return NULL;
   }

   return context;
#endif
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
      *value = GET_CONFIG_ATTRIB(c->Config, EGL_CONFIG_ID);
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
 * Bind the context to the surface as the draw or read surface.  Return the
 * previous surface that the context is bound to.
 *
 * Note that the context may be NULL.
 */
static _EGLSurface *
_eglBindContextToSurface(_EGLContext *ctx, _EGLSurface *surf, EGLint readdraw)
{
   _EGLSurface **attachment, *oldSurf;

   if (!ctx) {
      surf->Binding = NULL;
      return NULL;
   }

   attachment = (readdraw == EGL_DRAW) ?
      &ctx->DrawSurface : &ctx->ReadSurface;
   oldSurf = *attachment;

   if (oldSurf == surf)
      return NULL;

   if (oldSurf)
      oldSurf->Binding = NULL;
   surf->Binding = ctx;
   *attachment = surf;

   return oldSurf;
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
   if (ctx == oldCtx)
      return NULL;

   if (oldCtx)
      oldCtx->Binding = NULL;
   if (ctx)
      ctx->Binding = t;

   t->CurrentContexts[apiIndex] = ctx;

   return oldCtx;
}


/**
 * Return true if the given context and surfaces can be made current.
 */
static EGLBoolean
_eglCheckMakeCurrent(_EGLContext *ctx, _EGLSurface *draw, _EGLSurface *read)
{
   _EGLThreadInfo *t = _eglGetCurrentThread();
   EGLint conflict_api;

   if (_eglIsCurrentThreadDummy())
      return _eglError(EGL_BAD_ALLOC, "eglMakeCurrent");

   /* this is easy */
   if (!ctx) {
      if (draw || read)
         return _eglError(EGL_BAD_MATCH, "eglMakeCurrent");
      return EGL_TRUE;
   }

   /* ctx/draw/read must be all given */
   if (draw == NULL || read == NULL)
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
   if ((draw->Binding && draw->Binding != ctx) ||
       (read->Binding && read->Binding != ctx))
      return _eglError(EGL_BAD_ACCESS, "eglMakeCurrent");

   /* simply require the configs to be equal */
   if (draw->Config != ctx->Config || read->Config != ctx->Config)
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
 * Drivers will typically call this to do the error checking and
 * update the various flags.
 * Then, the driver will do its device-dependent Make-Current stuff.
 */
EGLBoolean
_eglMakeCurrent(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *draw,
                _EGLSurface *read, _EGLContext *ctx)
{
   _EGLThreadInfo *t = _eglGetCurrentThread();
   _EGLSurface *oldDraw = NULL, *oldRead = NULL;
   _EGLContext *oldCtx;

   if (!_eglCheckMakeCurrent(ctx, draw, read))
      return EGL_FALSE;

   oldCtx = _eglBindContextToThread(ctx, t);

   if (ctx) {
      oldDraw = _eglBindContextToSurface(ctx, draw, EGL_DRAW);
      oldRead = _eglBindContextToSurface(ctx, read, EGL_READ);
   }
   else if (oldCtx) {
      /* unbind the old context from its binding surfaces */
      oldDraw = oldCtx->DrawSurface;
      oldRead = oldCtx->ReadSurface;

      if (oldDraw)
         _eglBindContextToSurface(NULL, oldDraw, EGL_DRAW);
      if (oldRead && oldRead != oldDraw)
         _eglBindContextToSurface(NULL, oldRead, EGL_READ);
   }

   /* avoid double destroy */
   if (oldRead && oldRead == oldDraw)
      oldRead = NULL;

   if (oldCtx && !_eglIsContextLinked(oldCtx))
      drv->API.DestroyContext(drv, dpy, oldCtx);
   if (oldDraw && !_eglIsSurfaceLinked(oldDraw))
      drv->API.DestroySurface(drv, dpy, oldDraw);
   if (oldRead && !_eglIsSurfaceLinked(oldRead))
      drv->API.DestroySurface(drv, dpy, oldRead);

   return EGL_TRUE;
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
