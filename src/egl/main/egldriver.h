#ifndef EGLDRIVER_INCLUDED
#define EGLDRIVER_INCLUDED


#include "egltypedefs.h"
#include "eglapi.h"


/**
 * Base class for device drivers.
 */
struct _egl_driver
{
   void *LibHandle; /**< dlopen handle */
   const char *Path;  /**< path to this driver */
   const char *Args;  /**< args to load this driver */

   const char *Name;  /**< name of this driver */
   /**< probe a display to see if it is supported */
   EGLBoolean (*Probe)(_EGLDriver *drv, _EGLDisplay *dpy);
   /**< called before dlclose to release this driver */
   void (*Unload)(_EGLDriver *drv);

   _EGLAPI API;  /**< EGL API dispatch table */
};


extern _EGLDriver *_eglMain(const char *args);


extern const char *
_eglPreloadDriver(_EGLDisplay *dpy);


extern _EGLDriver *
_eglOpenDriver(_EGLDisplay *dpy);


extern EGLBoolean
_eglCloseDriver(_EGLDriver *drv, _EGLDisplay *dpy);


void
_eglUnloadDrivers(void);


extern _EGLDriver *
_eglLookupDriver(EGLDisplay d);


extern void
_eglInitDriverFallbacks(_EGLDriver *drv);


extern EGLint
_eglFindAPIs(void);


#endif /* EGLDRIVER_INCLUDED */
