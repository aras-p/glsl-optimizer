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
_eglInitContext(_EGLDriver *drv, EGLDisplay dpy, _EGLContext *ctx,
                EGLConfig config, const EGLint *attrib_list)
{
   _EGLConfig *conf;
   _EGLDisplay *display = _eglLookupDisplay(dpy);
   EGLint i;
   const EGLenum api = eglQueryAPI();

   if (api == EGL_NONE) {
      _eglError(EGL_BAD_MATCH, "eglCreateContext(no client API)");
      return EGL_FALSE;
   }

   conf = _eglLookupConfig(drv, dpy, config);
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

   ctx->Display = display;
   ctx->Config = conf;
   ctx->DrawSurface = EGL_NO_SURFACE;
   ctx->ReadSurface = EGL_NO_SURFACE;
   ctx->ClientAPI = api;

   return EGL_TRUE;
}


/**
 * Save a new _EGLContext into the hash table.
 */
void
_eglSaveContext(_EGLContext *ctx)
{
   /* no-op.
    * Public EGLContext handle and private _EGLContext are the same.
    */
}


/**
 * Remove the given _EGLContext object from the hash table.
 */
void
_eglRemoveContext(_EGLContext *ctx)
{
   /* no-op.
    * Public EGLContext handle and private _EGLContext are the same.
    */
}


/**
 * Return the public handle for the given private context ptr.
 * This is the inverse of _eglLookupContext().
 */
EGLContext
_eglGetContextHandle(_EGLContext *ctx)
{
   /* just a cast! */
   return (EGLContext) ctx;
}


/**
 * Return the _EGLContext object that corresponds to the given
 * EGLContext handle.
 * This is the inverse of _eglGetContextHandle().
 */
_EGLContext *
_eglLookupContext(EGLContext ctx)
{
   /* just a cast since EGLContext is just a void ptr */
   return (_EGLContext *) ctx;
}


/**
 * Return the currently bound _EGLContext object, or NULL.
 */
_EGLContext *
_eglGetCurrentContext(void)
{
   _EGLThreadInfo *t = _eglGetCurrentThread();
   return t->CurrentContext;
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

   context = (_EGLContext *) calloc(1, sizeof(_EGLContext));
   if (!context)
      return EGL_NO_CONTEXT;

   if (!_eglInitContext(drv, dpy, context, config, attrib_list)) {
      free(context);
      return EGL_NO_CONTEXT;
   }

   _eglSaveContext(context);
   return (EGLContext) context;
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
      if (context->IsBound) {
         context->DeletePending = EGL_TRUE;
      }
      else {
         free(context);
      }
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
 * update the various IsBound and DeletePending flags.
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

   _EGLContext *oldContext = _eglGetCurrentContext();
   _EGLSurface *oldDrawSurface = _eglGetCurrentSurface(EGL_DRAW);
   _EGLSurface *oldReadSurface = _eglGetCurrentSurface(EGL_READ);

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
   }

   /*
    * check if the old context or surfaces need to be deleted
    */
   if (oldDrawSurface != NULL) {
      oldDrawSurface->IsBound = EGL_FALSE;
      if (oldDrawSurface->DeletePending) {
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
      if (oldReadSurface->DeletePending) {
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
      if (oldContext->DeletePending) {
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
   }

   t->CurrentContext = ctx;

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
