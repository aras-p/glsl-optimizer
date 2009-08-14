#ifndef EGLDISPLAY_INCLUDED
#define EGLDISPLAY_INCLUDED

#ifdef _EGL_PLATFORM_X
#include <X11/Xlib.h>
#endif

#include "egltypedefs.h"
#include "eglhash.h"
#include "egldefines.h"


/**
 * Optional EGL extensions info.
 */
struct _egl_extensions
{
   EGLBoolean MESA_screen_surface;
   EGLBoolean MESA_copy_context;

   char String[_EGL_MAX_EXTENSIONS_LEN];
};


struct _egl_display 
{
   EGLNativeDisplayType NativeDisplay;
   EGLDisplay Handle;

   const char *DriverName;
   _EGLDriver *Driver;
   void *DriverData; /* private to driver */

   int APImajor, APIminor; /**< as returned by eglInitialize() */
   char Version[1000];     /**< initialized from APImajor/minor, DriverName */

   /** Bitmask of supported APIs (EGL_xx_BIT) set by the driver during init */
   EGLint ClientAPIsMask;
   char ClientAPIs[1000];   /**< updated by eglQueryString */

   _EGLExtensions Extensions;

   int LargestPbuffer;

   EGLint NumScreens;
   _EGLScreen **Screens;  /* array [NumScreens] */

   EGLint NumConfigs;
   _EGLConfig **Configs;  /* array [NumConfigs] of ptr to _EGLConfig */

   /* lists of linked contexts and surface */
   _EGLContext *ContextList;
   _EGLSurface *SurfaceList;

#ifdef _EGL_PLATFORM_X
   Display *Xdpy;
#endif
};


extern _EGLDisplay *
_eglNewDisplay(NativeDisplayType displayName);


extern EGLDisplay
_eglLinkDisplay(_EGLDisplay *dpy);


extern void
_eglUnlinkDisplay(_EGLDisplay *dpy);


extern EGLDisplay
_eglGetDisplayHandle(_EGLDisplay *display);


extern _EGLDisplay *
_eglLookupDisplay(EGLDisplay dpy);


/**
 * Return true if the display is linked.
 */
static INLINE EGLBoolean
_eglIsDisplayLinked(_EGLDisplay *dpy)
{
   return (EGLBoolean) (_eglGetDisplayHandle(dpy) != EGL_NO_DISPLAY);
}


extern _EGLDisplay *
_eglFindDisplay(NativeDisplayType nativeDisplay);


extern void
_eglReleaseDisplayResources(_EGLDriver *drv, _EGLDisplay *dpy);


extern void
_eglCleanupDisplay(_EGLDisplay *disp);


extern EGLContext
_eglLinkContext(_EGLContext *ctx, _EGLDisplay *dpy);


extern void
_eglUnlinkContext(_EGLContext *ctx);


extern EGLContext
_eglGetContextHandle(_EGLContext *ctx);


extern _EGLContext *
_eglLookupContext(EGLContext ctx, _EGLDisplay *dpy);


/**
 * Return true if the context is linked to a display.
 */
static INLINE EGLBoolean
_eglIsContextLinked(_EGLContext *ctx)
{
   return (EGLBoolean) (_eglGetContextHandle(ctx) != EGL_NO_CONTEXT);
}

extern EGLSurface
_eglLinkSurface(_EGLSurface *surf, _EGLDisplay *dpy);


extern void
_eglUnlinkSurface(_EGLSurface *surf);


extern EGLSurface
_eglGetSurfaceHandle(_EGLSurface *);


extern _EGLSurface *
_eglLookupSurface(EGLSurface surf, _EGLDisplay *dpy);


/**
 * Return true if the surface is linked to a display.
 */
static INLINE EGLBoolean
_eglIsSurfaceLinked(_EGLSurface *surf)
{
   return (EGLBoolean) (_eglGetSurfaceHandle(surf) != EGL_NO_SURFACE);
}


/**
 * Cast an unsigned int to a pointer.
 */
static INLINE void *
_eglUIntToPointer(unsigned int v)
{
   return (void *) ((uintptr_t) v);
}


/**
 * Cast a pointer to an unsigned int.  The pointer must be one that is
 * returned by _eglUIntToPointer.
 */
static INLINE unsigned int
_eglPointerToUInt(const void *p)
{
   return (unsigned int) ((uintptr_t) p);
}


#endif /* EGLDISPLAY_INCLUDED */
