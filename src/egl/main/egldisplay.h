#ifndef EGLDISPLAY_INCLUDED
#define EGLDISPLAY_INCLUDED

#ifdef _EGL_PLATFORM_X
#include <X11/Xlib.h>
#endif

#include "egltypedefs.h"


struct _egl_display 
{
   EGLNativeDisplayType NativeDisplay;
   EGLDisplay Handle;

   const char *DriverName;
   const char *DriverArgs;
   _EGLDriver *Driver;

   EGLint NumScreens;
   _EGLScreen **Screens;  /* array [NumScreens] */

   EGLint NumConfigs;
   _EGLConfig **Configs;  /* array [NumConfigs] of ptr to _EGLConfig */

#ifdef _EGL_PLATFORM_X
   Display *Xdpy;
#endif
};


extern _EGLDisplay *
_eglNewDisplay(NativeDisplayType displayName);


EGLDisplay
_eglGetDisplayHandle(_EGLDisplay *display);


extern _EGLDisplay *
_eglLookupDisplay(EGLDisplay dpy);


extern void
_eglSaveDisplay(_EGLDisplay *dpy);


extern _EGLDisplay *
_eglGetCurrentDisplay(void);


extern void
_eglCleanupDisplay(_EGLDisplay *disp);


extern EGLBoolean 
_eglQueryDisplayMESA(_EGLDriver *drv, EGLDisplay dpy, EGLint attrib, EGLint *value);



#endif /* EGLDISPLAY_INCLUDED */
