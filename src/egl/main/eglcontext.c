#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "eglconfig.h"
#include "eglcontext.h"
#include "egldriver.h"
#include "eglglobals.h"
#include "eglhash.h"
#include "eglsurface.h"


/**
 * Initialize the given _EGLContext object to defaults.
 */
void
_eglInitContext(_EGLContext *ctx)
{
   /* just init to zer for now */
   memset(ctx, 0, sizeof(_EGLContext));
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
_eglCreateContext(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, EGLContext share_list, const EGLint *attrib_list)
{
   _EGLConfig *conf = _eglLookupConfig(drv, dpy, config);
   if (!conf) {
      _eglError(EGL_BAD_CONFIG, "eglCreateContext");
      return EGL_NO_CONTEXT;
   }

   if (share_list != EGL_NO_CONTEXT) {
      _EGLContext *shareCtx = _eglLookupContext(share_list);
      if (!shareCtx) {
         _eglError(EGL_BAD_CONTEXT, "eglCreateContext(share_list)");
         return EGL_NO_CONTEXT;
      }
   }

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
_eglQueryContext(_EGLDriver *drv, EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value)
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
_eglMakeCurrent(_EGLDriver *drv, EGLDisplay dpy, EGLSurface d, EGLSurface r, EGLContext context)
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
         drv->DestroySurface(drv, dpy, oldDrawSurface->Handle);
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
         drv->DestroySurface(drv, dpy, oldReadSurface->Handle);
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
         drv->DestroyContext(drv, dpy, oldContext->Handle);
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
