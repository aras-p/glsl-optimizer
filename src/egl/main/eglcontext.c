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

   if (!conf) {
      _eglError(EGL_BAD_CONFIG, "_eglInitContext");
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

   return EGL_TRUE;
}


/**
 * Just a placeholder/demo function.  Real driver will never use this!
 */
EGLContext
_eglCreateContext(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config,
                  EGLContext share_list, const EGLint *attrib_list)
{
#if 0 /* example code */
   _EGLContext *context;
   _EGLConfig *conf;

   conf = _eglLookupConfig(drv, dpy, config);
   if (!conf) {
      _eglError(EGL_BAD_CONFIG, "eglCreateContext");
      return EGL_NO_CONTEXT;
   }

   context = (_EGLContext *) calloc(1, sizeof(_EGLContext));
   if (!context)
      return EGL_NO_CONTEXT;

   if (!_eglInitContext(drv, context, conf, attrib_list)) {
      free(context);
      return EGL_NO_CONTEXT;
   }

   return _eglLinkContext(context, _eglLookupDisplay(dpy));
#endif
   return EGL_NO_CONTEXT;
}


/**
 * Default fallback routine - drivers should usually override this.
 */
EGLBoolean
_eglDestroyContext(_EGLDriver *drv, EGLDisplay dpy, EGLContext ctx)
{
   _EGLContext *context = _eglLookupContext(ctx);
   if (context) {
      _eglUnlinkContext(context);
      if (!context->IsBound)
         free(context);
      return EGL_TRUE;
   }
   else {
      _eglError(EGL_BAD_CONTEXT, "eglDestroyContext");
      return EGL_TRUE;
   }
}


EGLBoolean
_eglQueryContext(_EGLDriver *drv, EGLDisplay dpy, EGLContext ctx,
                 EGLint attribute, EGLint *value)
{
   _EGLContext *c = _eglLookupContext(ctx);

   (void) drv;
   (void) dpy;

   if (!c) {
      _eglError(EGL_BAD_CONTEXT, "eglQueryContext");
      return EGL_FALSE;
   }

   switch (attribute) {
   case EGL_CONFIG_ID:
      *value = GET_CONFIG_ATTRIB(c->Config, EGL_CONFIG_ID);
      return EGL_TRUE;
#ifdef EGL_VERSION_1_2
   case EGL_CONTEXT_CLIENT_TYPE:
      *value = c->ClientAPI;
      return EGL_TRUE;
#endif /* EGL_VERSION_1_2 */
   case EGL_CONTEXT_CLIENT_VERSION:
      *value = c->ClientVersion;
      return EGL_TRUE;
   default:
      _eglError(EGL_BAD_ATTRIBUTE, "eglQueryContext");
      return EGL_FALSE;
   }
}


/**
 * Drivers will typically call this to do the error checking and
 * update the various flags.
 * Then, the driver will do its device-dependent Make-Current stuff.
 */
EGLBoolean
_eglMakeCurrent(_EGLDriver *drv, EGLDisplay dpy, EGLSurface d,
                EGLSurface r, EGLContext context)
{
   _EGLThreadInfo *t = _eglGetCurrentThread();
   _EGLContext *ctx = _eglLookupContext(context);
   _EGLSurface *draw = _eglLookupSurface(d);
   _EGLSurface *read = _eglLookupSurface(r);
   _EGLContext *oldContext = NULL;
   _EGLSurface *oldDrawSurface = NULL;
   _EGLSurface *oldReadSurface = NULL;
   EGLint apiIndex;

   if (_eglIsCurrentThreadDummy())
      return _eglError(EGL_BAD_ALLOC, "eglMakeCurrent");

   /* error checking */
   if (ctx) {
      if (draw == NULL || read == NULL) {
         _eglError(EGL_BAD_MATCH, "eglMakeCurrent");
         return EGL_FALSE;
      }
      if (draw->Config != ctx->Config) {
         _eglError(EGL_BAD_MATCH, "eglMakeCurrent");
         return EGL_FALSE;
      }
      if (read->Config != ctx->Config) {
         _eglError(EGL_BAD_MATCH, "eglMakeCurrent");
         return EGL_FALSE;
      }

#ifdef EGL_VERSION_1_4
      /* OpenGL and OpenGL ES are conflicting */
      switch (ctx->ClientAPI) {
      case EGL_OPENGL_ES_API:
         if (t->CurrentContexts[_eglConvertApiToIndex(EGL_OPENGL_API)])
            return _eglError(EGL_BAD_ACCESS, "eglMakeCurrent");
         break;
      case EGL_OPENGL_API:
         if (t->CurrentContexts[_eglConvertApiToIndex(EGL_OPENGL_ES_API)])
            return _eglError(EGL_BAD_ACCESS, "eglMakeCurrent");
         break;
      default:
         break;
      }
#endif
      apiIndex = _eglConvertApiToIndex(ctx->ClientAPI);
   }
   else {
      apiIndex = t->CurrentAPIIndex;
   }

   oldContext = t->CurrentContexts[apiIndex];
   if (oldContext) {
      oldDrawSurface = oldContext->DrawSurface;
      oldReadSurface = oldContext->ReadSurface;
   }

   /*
    * check if the old context or surfaces need to be deleted
    */
   if (oldDrawSurface != NULL) {
      oldDrawSurface->IsBound = EGL_FALSE;
      if (!_eglIsSurfaceLinked(oldDrawSurface)) {
         /* make sure we don't try to rebind a deleted surface */
         if (draw == oldDrawSurface || draw == oldReadSurface) {
            draw = NULL;
         }
         /* really delete surface now */
         drv->API.DestroySurface(drv, dpy, oldDrawSurface->Handle);
      }
   }
   if (oldReadSurface != NULL && oldReadSurface != oldDrawSurface) {
      oldReadSurface->IsBound = EGL_FALSE;
      if (!_eglIsSurfaceLinked(oldReadSurface)) {
         /* make sure we don't try to rebind a deleted surface */
         if (read == oldDrawSurface || read == oldReadSurface) {
            read = NULL;
         }
         /* really delete surface now */
         drv->API.DestroySurface(drv, dpy, oldReadSurface->Handle);
      }
   }
   if (oldContext != NULL) {
      oldContext->IsBound = EGL_FALSE;
      if (!_eglIsContextLinked(oldContext)) {
         /* make sure we don't try to rebind a deleted context */
         if (ctx == oldContext) {
            ctx = NULL;
         }
         /* really delete context now */
         drv->API.DestroyContext(drv, dpy, _eglGetContextHandle(oldContext));
      }
   }

   if (ctx) {
      /* check read/draw again, in case we deleted them above */
      if (draw == NULL || read == NULL) {
         _eglError(EGL_BAD_MATCH, "eglMakeCurrent");
         return EGL_FALSE;
      }
      ctx->DrawSurface = draw;
      ctx->ReadSurface = read;
      ctx->IsBound = EGL_TRUE;
      draw->IsBound = EGL_TRUE;
      read->IsBound = EGL_TRUE;
      t->CurrentContexts[apiIndex] = ctx;
   }
   else {
      t->CurrentContexts[apiIndex] = NULL;
   }

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
