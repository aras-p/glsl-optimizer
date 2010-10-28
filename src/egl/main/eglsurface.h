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
_eglQuerySurface(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf, EGLint attribute, EGLint *value);


extern EGLBoolean
_eglSurfaceAttrib(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf, EGLint attribute, EGLint value);


PUBLIC extern EGLBoolean
_eglBindTexImage(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf, EGLint buffer);


extern EGLBoolean
_eglSwapInterval(_EGLDriver *drv, _EGLDisplay *dpy, _EGLSurface *surf, EGLint interval);


/**
 * Increment reference count for the surface.
 */
static INLINE _EGLSurface *
_eglGetSurface(_EGLSurface *surf)
{
   if (surf)
      _eglGetResource(&surf->Resource);
   return surf;
}


/**
 * Decrement reference count for the surface.
 */
static INLINE EGLBoolean
_eglPutSurface(_EGLSurface *surf)
{
   return (surf) ? _eglPutResource(&surf->Resource) : EGL_FALSE;
}


/**
 * Link a surface to its display and return the handle of the link.
 * The handle can be passed to client directly.
 */
static INLINE EGLSurface
_eglLinkSurface(_EGLSurface *surf)
{
   _eglLinkResource(&surf->Resource, _EGL_RESOURCE_SURFACE);
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


#endif /* EGLSURFACE_INCLUDED */
