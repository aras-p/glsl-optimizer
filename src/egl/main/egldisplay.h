#ifndef EGLDISPLAY_INCLUDED
#define EGLDISPLAY_INCLUDED

#include "egltypedefs.h"
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
   /* used to link displays */
   _EGLDisplay *Next;

   EGLNativeDisplayType NativeDisplay;

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


extern _EGLDisplay *
_eglNewDisplay(NativeDisplayType displayName);


extern EGLDisplay
_eglLinkDisplay(_EGLDisplay *dpy);


extern void
_eglUnlinkDisplay(_EGLDisplay *dpy);


extern _EGLDisplay *
_eglFindDisplay(NativeDisplayType nativeDisplay);


PUBLIC void
_eglReleaseDisplayResources(_EGLDriver *drv, _EGLDisplay *dpy);


PUBLIC void
_eglCleanupDisplay(_EGLDisplay *disp);


#ifndef _EGL_SKIP_HANDLE_CHECK


extern EGLBoolean
_eglCheckDisplayHandle(EGLDisplay dpy);


#else /* !_EGL_SKIP_HANDLE_CHECK */

/* Only do a quick check.  This is NOT standard compliant. */

static INLINE EGLBoolean
_eglCheckDisplayHandle(EGLDisplay dpy)
{
   return ((_EGLDisplay *) dpy != NULL);
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
