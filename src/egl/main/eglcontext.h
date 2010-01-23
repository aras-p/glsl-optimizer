
#ifndef EGLCONTEXT_INCLUDED
#define EGLCONTEXT_INCLUDED


#include "egltypedefs.h"


/**
 * "Base" class for device driver contexts.
 */
struct _egl_context
{
   /* Managed by EGLDisplay for linking */
   _EGLDisplay *Display;
   _EGLContext *Next;

   /* The bound status of the context */
   _EGLThreadInfo *Binding;
   _EGLSurface *DrawSurface;
   _EGLSurface *ReadSurface;

   _EGLConfig *Config;

   EGLint ClientAPI; /**< EGL_OPENGL_ES_API, EGL_OPENGL_API, EGL_OPENVG_API */
   EGLint ClientVersion; /**< 1 = OpenGLES 1.x, 2 = OpenGLES 2.x */

   /* The real render buffer when a window surface is bound */
   EGLint WindowRenderBuffer;
};


PUBLIC EGLBoolean
_eglInitContext(_EGLDriver *drv, _EGLContext *ctx,
                _EGLConfig *config, const EGLint *attrib_list);


extern _EGLContext *
_eglCreateContext(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf, _EGLContext *share_list, const EGLint *attrib_list);


extern EGLBoolean
_eglDestroyContext(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *ctx);


extern EGLBoolean
_eglQueryContext(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *ctx, EGLint attribute, EGLint *value);


PUBLIC EGLBoolean
_eglMakeCurrent(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *draw, _EGLSurface *read, _EGLContext *ctx);


extern EGLBoolean
_eglCopyContextMESA(_EGLDriver *drv, EGLDisplay dpy, EGLContext source, EGLContext dest, EGLint mask);


/**
 * Return true if the context is bound to a thread.
 */
static INLINE EGLBoolean
_eglIsContextBound(_EGLContext *ctx)
{
   return (ctx->Binding != NULL);
}


extern EGLContext
_eglLinkContext(_EGLContext *ctx, _EGLDisplay *dpy);


extern void
_eglUnlinkContext(_EGLContext *ctx);


#ifndef _EGL_SKIP_HANDLE_CHECK


extern EGLBoolean
_eglCheckContextHandle(EGLContext ctx, _EGLDisplay *dpy);


#else /* !_EGL_SKIP_HANDLE_CHECK */


static INLINE EGLBoolean
_eglCheckContextHandle(EGLContext ctx, _EGLDisplay *dpy)
{
   _EGLContext *c = (_EGLContext *) ctx;
   return (dpy && c && c->Display == dpy);
}


#endif /* _EGL_SKIP_HANDLE_CHECK */


/**
 * Lookup a handle to find the linked context.
 * Return NULL if the handle has no corresponding linked context.
 */
static INLINE _EGLContext *
_eglLookupContext(EGLContext context, _EGLDisplay *dpy)
{
   _EGLContext *ctx = (_EGLContext *) context;
   if (!_eglCheckContextHandle(context, dpy))
      ctx = NULL;
   return ctx;
}


/**
 * Return the handle of a linked context, or EGL_NO_CONTEXT.
 */
static INLINE EGLContext
_eglGetContextHandle(_EGLContext *ctx)
{
   return (EGLContext) ((ctx && ctx->Display) ? ctx : EGL_NO_CONTEXT);
}


/**
 * Return true if the context is linked to a display.
 */
static INLINE EGLBoolean
_eglIsContextLinked(_EGLContext *ctx)
{
   return (EGLBoolean) (_eglGetContextHandle(ctx) != EGL_NO_CONTEXT);
}


#endif /* EGLCONTEXT_INCLUDED */
