#ifndef EGLDRIVER_INCLUDED
#define EGLDRIVER_INCLUDED


#include "egltypedefs.h"
#include "eglapi.h"
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


/**
 * Base class for device drivers.
 */
struct _egl_driver
{
   EGLBoolean Initialized; /**< set by driver after initialized */

   void *LibHandle; /**< dlopen handle */

   _EGLDisplay *Display;

   int APImajor, APIminor; /**< as returned by eglInitialize() */
   char Version[10];       /**< initialized from APImajor/minor */

   const char *ClientAPIs;

   _EGLAPI API;  /**< EGL API dispatch table */

   _EGLExtensions Extensions;
};


extern _EGLDriver *_eglMain(_EGLDisplay *dpy, const char *args);


extern const char *
_eglChooseDriver(_EGLDisplay *dpy);


extern _EGLDriver *
_eglOpenDriver(_EGLDisplay *dpy, const char *driverName, const char *args);


extern EGLBoolean
_eglCloseDriver(_EGLDriver *drv, EGLDisplay dpy);


extern _EGLDriver *
_eglLookupDriver(EGLDisplay d);


extern void
_eglInitDriverFallbacks(_EGLDriver *drv);


extern const char *
_eglQueryString(_EGLDriver *drv, EGLDisplay dpy, EGLint name);


extern EGLBoolean
_eglWaitGL(_EGLDriver *drv, EGLDisplay dpy);


extern EGLBoolean
_eglWaitNative(_EGLDriver *drv, EGLDisplay dpy, EGLint engine);



#endif /* EGLDRIVER_INCLUDED */
