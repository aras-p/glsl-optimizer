#ifndef EGLDISPLAY_INCLUDED
#define EGLDISPLAY_INCLUDED

#include "egltypedefs.h"
#include "egldefines.h"
#include "eglcontext.h"
#include "eglsurface.h"


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
   /* used to link displays */
   _EGLDisplay *Next;

   EGLNativeDisplayType NativeDisplay;

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

   EGLint MaxConfigs;
   EGLint NumConfigs;
   _EGLConfig **Configs;  /* array [NumConfigs] of ptr to _EGLConfig */

   /* lists of linked contexts and surface */
   _EGLContext *ContextList;
   _EGLSurface *SurfaceList;
};


extern void
_eglFiniDisplay(void);


extern char *
_eglSplitDisplayString(const char *dpyString, const char **args);


extern _EGLDisplay *
_eglNewDisplay(NativeDisplayType displayName);


extern EGLDisplay
_eglLinkDisplay(_EGLDisplay *dpy);


extern void
_eglUnlinkDisplay(_EGLDisplay *dpy);


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


extern EGLSurface
_eglLinkSurface(_EGLSurface *surf, _EGLDisplay *dpy);


extern void
_eglUnlinkSurface(_EGLSurface *surf);


#ifndef _EGL_SKIP_HANDLE_CHECK


extern EGLBoolean
_eglCheckDisplayHandle(EGLDisplay dpy);


extern EGLBoolean
_eglCheckContextHandle(EGLContext ctx, _EGLDisplay *dpy);


extern EGLBoolean
_eglCheckSurfaceHandle(EGLSurface surf, _EGLDisplay *dpy);


#else /* !_EGL_SKIP_HANDLE_CHECK */

/* Only do a quick check.  This is NOT standard compliant. */

static INLINE EGLBoolean
_eglCheckDisplayHandle(EGLDisplay dpy)
{
   return ((_EGLDisplay *) dpy != NULL);
}


static INLINE EGLBoolean
_eglCheckContextHandle(EGLContext ctx, _EGLDisplay *dpy)
{
   _EGLContext *c = (_EGLContext *) ctx;
   return (dpy && c && c->Display == dpy);
}


static INLINE EGLBoolean
_eglCheckSurfaceHandle(EGLSurface surf, _EGLDisplay *dpy)
{
   _EGLSurface *s = (_EGLSurface *) surf;
   return (dpy && s && s->Display == dpy);
}


#endif /* _EGL_SKIP_HANDLE_CHECK */


/**
 * Lookup a handle to find the linked display.
 * Return NULL if the handle has no corresponding linked display.
 */
static INLINE _EGLDisplay *
_eglLookupDisplay(EGLDisplay display)
{
   _EGLDisplay *dpy = (_EGLDisplay *) display;
   if (!_eglCheckDisplayHandle(display))
      dpy = NULL;
   return dpy;
}


/**
 * Return the handle of a linked display, or EGL_NO_DISPLAY.
 */
static INLINE EGLDisplay
_eglGetDisplayHandle(_EGLDisplay *dpy)
{
   return (EGLDisplay) ((dpy) ? dpy : EGL_NO_DISPLAY);
}


/**
 * Return true if the display is linked.
 */
static INLINE EGLBoolean
_eglIsDisplayLinked(_EGLDisplay *dpy)
{
   return (EGLBoolean) (_eglGetDisplayHandle(dpy) != EGL_NO_DISPLAY);
}


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


/**
 * Lookup a handle to find the linked surface.
 * Return NULL if the handle has no corresponding linked surface.
 */
static INLINE _EGLSurface *
_eglLookupSurface(EGLSurface surface, _EGLDisplay *dpy)
{
   _EGLSurface *surf = (_EGLSurface *) surface;
   if (!_eglCheckSurfaceHandle(surf, dpy))
      surf = NULL;
   return surf;
}


/**
 * Return the handle of a linked surface, or EGL_NO_SURFACE.
 */
static INLINE EGLSurface
_eglGetSurfaceHandle(_EGLSurface *surf)
{
   return (EGLSurface) ((surf && surf->Display) ? surf : EGL_NO_SURFACE);
}


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
