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


PUBLIC _EGLDriver *
_eglMain(const char *args);


extern _EGLDriver *
_eglOpenDriver(_EGLDisplay *dpy);


extern EGLBoolean
_eglCloseDriver(_EGLDriver *drv, _EGLDisplay *dpy);


extern EGLBoolean
_eglPreloadDrivers(void);


extern void
_eglUnloadDrivers(void);


PUBLIC void
_eglInitDriverFallbacks(_EGLDriver *drv);


PUBLIC EGLint
_eglFindAPIs(void);


#endif /* EGLDRIVER_INCLUDED */
