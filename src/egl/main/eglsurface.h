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

   /* The context that is currently bound to the surface */
   _EGLContext *CurrentContext;

   _EGLConfig *Config;

   EGLint Type; /* one of EGL_WINDOW_BIT, EGL_PIXMAP_BIT or EGL_PBUFFER_BIT */

   /* attributes set by attribute list */
   EGLint Width, Height;
   EGLenum TextureFormat;
   EGLenum TextureTarget;
   EGLBoolean MipmapTexture;
   EGLBoolean LargestPbuffer;
   EGLenum RenderBuffer;
   EGLenum VGAlphaFormat;
   EGLenum VGColorspace;

   /* attributes set by eglSurfaceAttrib */
   EGLint MipmapLevel;
   EGLenum MultisampleResolve;
   EGLenum SwapBehavior;

   EGLint HorizontalResolution, VerticalResolution;
   EGLint AspectRatio;

   EGLint SwapInterval;

   /* True if the surface is bound to an OpenGL ES texture */
   EGLBoolean BoundToTexture;
};


PUBLIC EGLBoolean
_eglInitSurface(_EGLSurface *surf, _EGLDisplay *dpy, EGLint type,
                _EGLConfig *config, const EGLint *attrib_list);


extern EGLBoolean
_eglSwapBuffers(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf);


extern EGLBoolean
_eglCopyBuffers(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf, EGLNativePixmapType target);


extern EGLBoolean
_eglQuerySurface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf, EGLint attribute, EGLint *value);


extern _EGLSurface *
_eglCreateWindowSurface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf, EGLNativeWindowType window, const EGLint *attrib_list);


extern _EGLSurface *
_eglCreatePixmapSurface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLConfig *conf, EGLNativePixmapType pixmap, const EGLint *attrib_list);


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
 * Return true if there is a context bound to the surface.
 *
 * The binding is considered a reference to the surface.  Drivers should not
 * destroy a surface when it is bound.
 */
static INLINE EGLBoolean
_eglIsSurfaceBound(_EGLSurface *surf)
{
   return (surf->CurrentContext != NULL);
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
   _EGLSurface *surf = (_EGLSurface *) surface;
   if (!dpy || !_eglCheckResource((void *) surf, _EGL_RESOURCE_SURFACE, dpy))
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
 *
 * The link is considered a reference to the surface (the display is owning the
 * surface).  Drivers should not destroy a surface when it is linked.
 */
static INLINE EGLBoolean
_eglIsSurfaceLinked(_EGLSurface *surf)
{
   _EGLResource *res = (_EGLResource *) surf;
   return (res && _eglIsResourceLinked(res));
}


#endif /* EGLSURFACE_INCLUDED */
