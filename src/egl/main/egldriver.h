#ifndef EGLDRIVER_INCLUDED
#define EGLDRIVER_INCLUDED


#include "egltypedefs.h"
#include "eglapi.h"

/* should probably use a dynamic-lengh string, but this will do */
#define MAX_EXTENSIONS_LEN 1000


/**
 * Base class for device drivers.
 */
struct _egl_driver
{
   EGLBoolean Initialized; /* set by driver after initialized */

   void *LibHandle; /* dlopen handle */

   _EGLDisplay *Display;

   int ABIversion;
   int APImajor, APIminor; /* returned through eglInitialize */

   _EGLAPI API;

   /* Extension enable flags */
   EGLBoolean MESA_screen_surface;
   EGLBoolean MESA_copy_context;

   /* Extensions string */
   char Extensions[MAX_EXTENSIONS_LEN];
};


extern _EGLDriver *_eglMain(_EGLDisplay *dpy);


extern _EGLDriver *
_eglChooseDriver(EGLDisplay dpy);


extern _EGLDriver *
_eglOpenDriver(_EGLDisplay *dpy, const char *driverName);


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
