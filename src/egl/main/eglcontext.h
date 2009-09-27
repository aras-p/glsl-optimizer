
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


extern EGLBoolean
_eglInitContext(_EGLDriver *drv, _EGLContext *ctx,
                _EGLConfig *config, const EGLint *attrib_list);


extern _EGLContext *
_eglCreateContext(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf, _EGLContext *share_list, const EGLint *attrib_list);


extern EGLBoolean
_eglDestroyContext(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *ctx);


extern EGLBoolean
_eglQueryContext(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *ctx, EGLint attribute, EGLint *value);


extern EGLBoolean
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


#endif /* EGLCONTEXT_INCLUDED */
