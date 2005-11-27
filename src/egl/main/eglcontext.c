#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "eglconfig.h"
#include "eglcontext.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "eglglobals.h"
#include "eglhash.h"
#include "eglsurface.h"


/**
 * Initialize the given _EGLContext object to defaults.
 */
EGLBoolean
_eglInitContext(_EGLDriver *drv, EGLDisplay dpy, _EGLContext *ctx,
                EGLConfig config, const EGLint *attrib_list)
{
   _EGLConfig *conf;
   _EGLDisplay *display = _eglLookupDisplay(dpy);
   EGLint i;

   conf = _eglLookupConfig(drv, dpy, config);
   if (!conf) {
      _eglError(EGL_BAD_CONFIG, "eglCreateContext");
      return EGL_FALSE;
   }

   for (i = 0; attrib_list && attrib_list[i] != EGL_NONE; i++) {
      switch (attrib_list[i]) {
         /* no attribs defined for now */
      default:
         _eglError(EGL_BAD_ATTRIBUTE, "eglCreateContext");
         return EGL_NO_CONTEXT;
      }
   }

   memset(ctx, 0, sizeof(_EGLContext));
   ctx->Display = display;
   ctx->Config = conf;
   ctx->DrawSurface = EGL_NO_SURFACE;
   ctx->ReadSurface = EGL_NO_SURFACE;

   return EGL_TRUE;
}


/*
 * Assign an EGLContext handle to the _EGLContext object then put it into
 * the hash table.
 */
void
_eglSaveContext(_EGLContext *ctx)
{
   assert(ctx);
   ctx->Handle = _eglHashGenKey(_eglGlobal.Contexts);
   _eglHashInsert(_eglGlobal.Contexts, ctx->Handle, ctx);
}


/**
 * Remove the given _EGLContext object from the hash table.
 */
void
_eglRemoveContext(_EGLContext *ctx)
{
   _eglHashRemove(_eglGlobal.Contexts, ctx->Handle);
}


/**
 * Return the _EGLContext object that corresponds to the given
 * EGLContext handle.
 */
_EGLContext *
_eglLookupContext(EGLContext ctx)
{
   _EGLContext *c = (_EGLContext *) _eglHashLookup(_eglGlobal.Contexts, ctx);
   return c;
}


/**
 * Return the currently bound _EGLContext object, or NULL.
 */
_EGLContext *
_eglGetCurrentContext(void)
{
   /* XXX this should be per-thread someday */
   return _eglGlobal.CurrentContext;
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
   return context->Handle;
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
      _eglHashRemove(_eglGlobal.Contexts, ctx);
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
         drv->API.DestroyContext(drv, dpy, oldContext->Handle);
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

   _eglGlobal.CurrentContext = ctx;

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
