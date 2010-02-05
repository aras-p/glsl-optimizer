#ifndef EGLDRIVER_INCLUDED
#define EGLDRIVER_INCLUDED


#include "egltypedefs.h"
#include "eglapi.h"


/**
 * Define an inline driver typecast function.
 *
 * Note that this macro defines a function and should not be ended with a
 * semicolon when used.
 */
#define _EGL_DRIVER_TYPECAST(drvtype, egltype, code)           \
   static INLINE struct drvtype *drvtype(const egltype *obj)   \
   { return (struct drvtype *) code; }


/**
 * Define the driver typecast functions for _EGLDriver, _EGLDisplay,
 * _EGLContext, _EGLSurface, and _EGLConfig.
 *
 * Note that this macro defines several functions and should not be ended with
 * a semicolon when used.
 */
#define _EGL_DRIVER_STANDARD_TYPECASTS(drvname)                            \
   _EGL_DRIVER_TYPECAST(drvname ## _driver, _EGLDriver, obj)               \
   /* note that this is not a direct cast */                               \
   _EGL_DRIVER_TYPECAST(drvname ## _display, _EGLDisplay, obj->DriverData) \
   _EGL_DRIVER_TYPECAST(drvname ## _context, _EGLContext, obj)             \
   _EGL_DRIVER_TYPECAST(drvname ## _surface, _EGLSurface, obj)             \
   _EGL_DRIVER_TYPECAST(drvname ## _config, _EGLConfig, obj)


typedef _EGLDriver *(*_EGLMain_t)(const char *args);


/**
 * Base class for device drivers.
 */
struct _egl_driver
{
   void *LibHandle; /**< dlopen handle */
   const char *Path;  /**< path to this driver */
   const char *Args;  /**< args to load this driver */

   const char *Name;  /**< name of this driver */

   /**
    * Probe a display and return a score.
    *
    * Roughly,
    *  50 means the driver supports the display;
    *  90 means the driver can accelerate the display;
    * 100 means a perfect match.
    */
   EGLint (*Probe)(_EGLDriver *drv, _EGLDisplay *dpy);

   /**
    * Release the driver resource.
    *
    * It is called before dlclose().
    */
   void (*Unload)(_EGLDriver *drv);

   _EGLAPI API;  /**< EGL API dispatch table */
};


PUBLIC _EGLDriver *
_eglMain(const char *args);


extern _EGLDriver *
_eglMatchDriver(_EGLDisplay *dpy);


extern EGLBoolean
_eglPreloadDrivers(void);


extern void
_eglUnloadDrivers(void);


PUBLIC void
_eglInitDriverFallbacks(_EGLDriver *drv);


PUBLIC void
_eglSetProbeCache(EGLint key, const void *val);


PUBLIC const void *
_eglGetProbeCache(EGLint key);


#endif /* EGLDRIVER_INCLUDED */
