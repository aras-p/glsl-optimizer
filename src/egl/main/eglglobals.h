#ifndef EGLGLOBALS_INCLUDED
#define EGLGLOBALS_INCLUDED

#include "egltypedefs.h"
#include "eglhash.h"
#include "eglcurrent.h"


/**
 * Global library data
 */
struct _egl_global
{
   EGLScreenMESA FreeScreenHandle;

   /* bitmaks of supported APIs (supported by _some_ driver) */
   EGLint ClientAPIsMask;

   char ClientAPIs[1000];   /**< updated by eglQueryString */

   EGLint NumDrivers;
   _EGLDriver *Drivers[10];
};


extern struct _egl_global _eglGlobal;


#endif /* EGLGLOBALS_INCLUDED */
