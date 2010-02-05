#ifndef EGLCONTEXT_INCLUDED
#define EGLCONTEXT_INCLUDED


#include "egltypedefs.h"
#include "egldisplay.h"


/**
 * "Base" class for device driver contexts.
 */
struct _egl_context
{
   /* A context is a display resource */
   _EGLResource Resource;

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
_eglInitContext(_EGLContext *ctx, _EGLDisplay *dpy,
                _EGLConfig *config, const EGLint *attrib_list);


extern _EGLContext *
_eglCreateContext(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf, _EGLContext *share_list, const EGLint *attrib_list);


extern EGLBoolean
_eglDestroyContext(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *ctx);


extern EGLBoolean
_eglQueryContext(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *ctx, EGLint attribute, EGLint *value);


PUBLIC EGLBoolean
_eglBindContext(_EGLContext **ctx, _EGLSurface **draw, _EGLSurface **read);


extern EGLBoolean
_eglMakeCurrent(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *draw, _EGLSurface *read, _EGLContext *ctx);


extern EGLBoolean
_eglCopyContextMESA(_EGLDriver *drv, EGLDisplay dpy, EGLContext source, EGLContext dest, EGLint mask);


/**
 * Return true if the context is bound to a thread.
 *
 * The binding is considered a reference to the context.  Drivers should not
 * destroy a context when it is bound.
 */
static INLINE EGLBoolean
_eglIsContextBound(_EGLContext *ctx)
{
   return (ctx->Binding != NULL);
}


/**
 * Link a context to a display and return the handle of the link.
 * The handle can be passed to client directly.
 */
static INLINE EGLContext
_eglLinkContext(_EGLContext *ctx, _EGLDisplay *dpy)
{
   _eglLinkResource(&ctx->Resource, _EGL_RESOURCE_CONTEXT, dpy);
   return (EGLContext) ctx;
}


/**
 * Unlink a linked context from its display.
 * Accessing an unlinked context should generate EGL_BAD_CONTEXT error.
 */
static INLINE void
_eglUnlinkContext(_EGLContext *ctx)
{
   _eglUnlinkResource(&ctx->Resource, _EGL_RESOURCE_CONTEXT);
}


/**
 * Lookup a handle to find the linked context.
 * Return NULL if the handle has no corresponding linked context.
 */
static INLINE _EGLContext *
_eglLookupContext(EGLContext context, _EGLDisplay *dpy)
{
   _EGLContext *ctx = (_EGLContext *) context;
   if (!dpy || !_eglCheckResource((void *) ctx, _EGL_RESOURCE_CONTEXT, dpy))
      ctx = NULL;
   return ctx;
}


/**
 * Return the handle of a linked context, or EGL_NO_CONTEXT.
 */
static INLINE EGLContext
_eglGetContextHandle(_EGLContext *ctx)
{
   _EGLResource *res = (_EGLResource *) ctx;
   return (res && _eglIsResourceLinked(res)) ?
      (EGLContext) ctx : EGL_NO_CONTEXT;
}


/**
 * Return true if the context is linked to a display.
 *
 * The link is considered a reference to the context (the display is owning the
 * context).  Drivers should not destroy a context when it is linked.
 */
static INLINE EGLBoolean
_eglIsContextLinked(_EGLContext *ctx)
{
   _EGLResource *res = (_EGLResource *) ctx;
   return (res && _eglIsResourceLinked(res));
}


#endif /* EGLCONTEXT_INCLUDED */
