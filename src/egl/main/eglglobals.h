#ifndef EGLGLOBALS_INCLUDED
#define EGLGLOBALS_INCLUDED

#include "egltypedefs.h"
#include "eglhash.h"
#include "eglcurrent.h"
#include "eglmutex.h"


/**
 * Global library data
 */
struct _egl_global
{
   _EGLMutex *Mutex;
   EGLScreenMESA FreeScreenHandle;

   /* bitmaks of supported APIs (supported by _some_ driver) */
   EGLint ClientAPIsMask;

   char ClientAPIs[1000];   /**< updated by eglQueryString */

   EGLint NumDrivers;
   _EGLDriver *Drivers[10];

   EGLint NumAtExitCalls;
   void (*AtExitCalls[10])(void);
};


extern struct _egl_global _eglGlobal;


extern void
_eglAddAtExitCall(void (*func)(void));


#endif /* EGLGLOBALS_INCLUDED */
