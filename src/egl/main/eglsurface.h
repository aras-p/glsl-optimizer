#ifndef EGLSURFACE_INCLUDED
#define EGLSURFACE_INCLUDED


#include "egltypedefs.h"


/**
 * "Base" class for device driver surfaces.
 */
struct _egl_surface
{
   EGLSurface Handle;  /* The public/opaque handle which names this object */
   _EGLConfig *Config;

   /* May need reference counting here */
   EGLBoolean IsBound;
   EGLBoolean DeletePending;
   EGLBoolean BoundToTexture;

   EGLint Type; /* one of EGL_WINDOW_BIT, EGL_PIXMAP_BIT or EGL_PBUFFER_BIT */
   EGLint Width, Height;
   EGLint TextureFormat, TextureTarget;
   EGLint MipmapTexture, MipmapLevel;
   EGLint SwapInterval;

   /* If type == EGL_SCREEN_BIT: */
   EGLint VisibleRefCount; /* number of screens I'm displayed on */

#ifdef EGL_VERSION_1_2
   EGLint SwapBehavior; /* one of EGL_BUFFER_PRESERVED/DESTROYED */
   EGLint HorizontalResolution, VerticalResolution;
   EGLint AspectRatio;
   EGLint RenderBuffer; /* EGL_BACK_BUFFER or EGL_SINGLE_BUFFER */
   EGLint AlphaFormat; /* EGL_ALPHA_FORMAT_NONPRE or EGL_ALPHA_FORMAT_PRE */
   EGLint Colorspace; /* EGL_COLORSPACE_sRGB or EGL_COLORSPACE_LINEAR */
#endif /* EGL_VERSION_1_2 */
};


extern EGLBoolean
_eglInitSurface(_EGLDriver *drv, EGLDisplay dpy,
                _EGLSurface *surf, EGLint type, EGLConfig config,
                const EGLint *attrib_list);


extern void
_eglSaveSurface(_EGLSurface *surf);


extern void
_eglRemoveSurface(_EGLSurface *surf);


extern EGLSurface
_eglGetSurfaceHandle(_EGLSurface *surface);


extern _EGLSurface *
_eglLookupSurface(EGLSurface surf);
 

extern _EGLSurface *
_eglGetCurrentSurface(EGLint readdraw);


extern EGLBoolean
_eglSwapBuffers(_EGLDriver *drv, EGLDisplay dpy, EGLSurface draw);


extern EGLBoolean
_eglCopyBuffers(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surface, NativePixmapType target);


extern EGLBoolean
_eglQuerySurface(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value);


extern EGLSurface
_eglCreateWindowSurface(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, NativeWindowType window, const EGLint *attrib_list);


extern EGLSurface
_eglCreatePixmapSurface(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, NativePixmapType pixmap, const EGLint *attrib_list);


extern EGLSurface
_eglCreatePbufferSurface(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list);


extern EGLBoolean
_eglDestroySurface(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surface);


extern EGLBoolean
_eglSurfaceAttrib(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value);


extern EGLBoolean
_eglBindTexImage(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surface, EGLint buffer);


extern EGLBoolean
_eglReleaseTexImage(_EGLDriver *drv, EGLDisplay dpy, EGLSurface surface, EGLint buffer);


extern EGLBoolean
_eglSwapInterval(_EGLDriver *drv, EGLDisplay dpy, EGLint interval);


#ifdef EGL_VERSION_1_2

extern EGLSurface
_eglCreatePbufferFromClientBuffer(_EGLDriver *drv, EGLDisplay dpy,
                                  EGLenum buftype, EGLClientBuffer buffer,
                                  EGLConfig config, const EGLint *attrib_list);

#endif /* EGL_VERSION_1_2 */



#endif /* EGLSURFACE_INCLUDED */
