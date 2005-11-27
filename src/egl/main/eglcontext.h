
#ifndef EGLCONTEXT_INCLUDED
#define EGLCONTEXT_INCLUDED


#include "egltypedefs.h"


/**
 * "Base" class for device driver contexts.
 */
struct _egl_context
{
   EGLContext Handle;  /* The public/opaque handle which names this object */

   _EGLDisplay *Display; /* who do I belong to? */

   _EGLConfig *Config;

   _EGLSurface *DrawSurface;
   _EGLSurface *ReadSurface;

   EGLBoolean IsBound;
   EGLBoolean DeletePending;
};


extern EGLBoolean
_eglInitContext(_EGLDriver *drv, EGLDisplay dpy, _EGLContext *ctx,
                EGLConfig config, const EGLint *attrib_list);


extern void
_eglSaveContext(_EGLContext *ctx);


extern void
_eglRemoveContext(_EGLContext *ctx);


extern _EGLContext *
_eglLookupContext(EGLContext ctx);
 

extern _EGLContext *
_eglGetCurrentContext(void);


extern EGLContext
_eglCreateContext(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, EGLContext share_list, const EGLint *attrib_list);


extern EGLBoolean
_eglDestroyContext(_EGLDriver *drv, EGLDisplay dpy, EGLContext ctx);


extern EGLBoolean
_eglQueryContext(_EGLDriver *drv, EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value);


extern EGLBoolean
_eglMakeCurrent(_EGLDriver *drv, EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx);


extern EGLBoolean
_eglCopyContextMESA(_EGLDriver *drv, EGLDisplay dpy, EGLContext source, EGLContext dest, EGLint mask);

#endif /* EGLCONTEXT_INCLUDED */
