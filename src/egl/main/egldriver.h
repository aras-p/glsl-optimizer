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

   const char *Name;  /**< name of this driver */

   int APImajor, APIminor; /**< as returned by eglInitialize() */
   char Version[1000];       /**< initialized from APImajor/minor, Name */

   /** Bitmask of supported APIs (EGL_xx_BIT) set by the driver during init */
   EGLint ClientAPIsMask;

   _EGLAPI API;  /**< EGL API dispatch table */

   _EGLExtensions Extensions;

   int LargestPbuffer;
};


extern _EGLDriver *_eglMain(_EGLDisplay *dpy, const char *args);


extern const char *
_eglChooseDRMDriver(int card);

extern const char *
_eglChooseDriver(_EGLDisplay *dpy);


extern _EGLDriver *
_eglOpenDriver(_EGLDisplay *dpy, const char *driverName, const char *args);


extern EGLBoolean
_eglCloseDriver(_EGLDriver *drv, EGLDisplay dpy);


extern void
_eglSaveDriver(_EGLDriver *drv);


extern _EGLDriver *
_eglLookupDriver(EGLDisplay d);


extern void
_eglInitDriverFallbacks(_EGLDriver *drv);


extern EGLint
_eglFindAPIs(void);


#endif /* EGLDRIVER_INCLUDED */
