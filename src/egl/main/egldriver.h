#ifndef EGLDRIVER_INCLUDED
#define EGLDRIVER_INCLUDED


#include "egltypedefs.h"
#include "eglapi.h"
#include <stddef.h>

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


extern _EGLDriver *
_eglBuiltInDriverGALLIUM(const char *args);


extern _EGLDriver *
_eglBuiltInDriverDRI2(const char *args);


extern _EGLDriver *
_eglBuiltInDriverGLX(const char *args);


PUBLIC _EGLDriver *
_eglMain(const char *args);


extern _EGLDriver *
_eglMatchDriver(_EGLDisplay *dpy, EGLBoolean probe_only);


extern __eglMustCastToProperFunctionPointerType
_eglGetDriverProc(const char *procname);


extern void
_eglUnloadDrivers(void);


/* defined in eglfallbacks.c */
PUBLIC void
_eglInitDriverFallbacks(_EGLDriver *drv);


PUBLIC void
_eglSearchPathForEach(EGLBoolean (*callback)(const char *, size_t, void *),
                      void *callback_data);


#endif /* EGLDRIVER_INCLUDED */
