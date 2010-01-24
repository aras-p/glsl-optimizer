#ifndef EGLSURFACE_INCLUDED
#define EGLSURFACE_INCLUDED


#include "egltypedefs.h"
#include "egldisplay.h"


/**
 * "Base" class for device driver surfaces.
 */
struct _egl_surface
{
   /* A surface is a display resource */
   _EGLResource Resource;

   /* The bound status of the surface */
   _EGLContext *Binding;
   EGLBoolean BoundToTexture;

   _EGLConfig *Config;

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


PUBLIC EGLBoolean
_eglInitSurface(_EGLDriver *drv, _EGLSurface *surf, EGLint type,
                _EGLConfig *config, const EGLint *attrib_list);


extern EGLBoolean
_eglSwapBuffers(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf);


extern EGLBoolean
_eglCopyBuffers(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf, NativePixmapType target);


extern EGLBoolean
_eglQuerySurface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf, EGLint attribute, EGLint *value);


extern _EGLSurface *
_eglCreateWindowSurface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf, NativeWindowType window, const EGLint *attrib_list);


extern _EGLSurface *
_eglCreatePixmapSurface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf, NativePixmapType pixmap, const EGLint *attrib_list);


extern _EGLSurface *
_eglCreatePbufferSurface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf, const EGLint *attrib_list);


extern EGLBoolean
_eglDestroySurface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf);


extern EGLBoolean
_eglSurfaceAttrib(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf, EGLint attribute, EGLint value);


extern EGLBoolean
_eglBindTexImage(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf, EGLint buffer);


extern EGLBoolean
_eglReleaseTexImage(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf, EGLint buffer);


extern EGLBoolean
_eglSwapInterval(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf, EGLint interval);


#ifdef EGL_VERSION_1_2

extern _EGLSurface *
_eglCreatePbufferFromClientBuffer(_EGLDriver *drv, _EGLDisplay *dpy,
                                  EGLenum buftype, EGLClientBuffer buffer,
                                  _EGLConfig *conf, const EGLint *attrib_list);

#endif /* EGL_VERSION_1_2 */


/**
 * Return true if the surface is bound to a thread.
 * A surface bound to a texutre is not considered bound by
 * this function.
 */
static INLINE EGLBoolean
_eglIsSurfaceBound(_EGLSurface *surf)
{
   return (surf->Binding != NULL);
}


/**
 * Link a surface to a display and return the handle of the link.
 * The handle can be passed to client directly.
 */
static INLINE EGLSurface
_eglLinkSurface(_EGLSurface *surf, _EGLDisplay *dpy)
{
   _eglLinkResource(&surf->Resource, _EGL_RESOURCE_SURFACE, dpy);
   return (EGLSurface) surf;
}


/**
 * Unlink a linked surface from its display.
 * Accessing an unlinked surface should generate EGL_BAD_SURFACE error.
 */
static INLINE void
_eglUnlinkSurface(_EGLSurface *surf)
{
   _eglUnlinkResource(&surf->Resource, _EGL_RESOURCE_SURFACE);
}


/**
 * Lookup a handle to find the linked surface.
 * Return NULL if the handle has no corresponding linked surface.
 */
static INLINE _EGLSurface *
_eglLookupSurface(EGLSurface surface, _EGLDisplay *dpy)
{
   _EGLResource *res = (_EGLResource *) surface;
   _EGLSurface *surf = (_EGLSurface *) surface;
   if (!res || !dpy || !_eglCheckResource(res, _EGL_RESOURCE_SURFACE, dpy))
      surf = NULL;
   return surf;
}


/**
 * Return the handle of a linked surface, or EGL_NO_SURFACE.
 */
static INLINE EGLSurface
_eglGetSurfaceHandle(_EGLSurface *surf)
{
   _EGLResource *res = (_EGLResource *) surf;
   return (res && _eglIsResourceLinked(res)) ?
      (EGLSurface) surf : EGL_NO_SURFACE;
}


/**
 * Return true if the surface is linked to a display.
 */
static INLINE EGLBoolean
_eglIsSurfaceLinked(_EGLSurface *surf)
{
   _EGLResource *res = (_EGLResource *) surf;
   return (res && _eglIsResourceLinked(res));
}


#endif /* EGLSURFACE_INCLUDED */
