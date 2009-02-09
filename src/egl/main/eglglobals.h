#ifndef EGLGLOBALS_INCLUDED
#define EGLGLOBALS_INCLUDED

#include "egltypedefs.h"
#include "eglhash.h"


/**
 * Per-thread info
 */
struct _egl_thread_info
{
   EGLint LastError;
   _EGLContext *CurrentContext;
   EGLenum CurrentAPI;
};


/**
 * Global library data
 */
struct _egl_global
{
   EGLBoolean Initialized;

   _EGLHashtable *Displays;
   _EGLHashtable *Surfaces;

   EGLScreenMESA FreeScreenHandle;

   /* bitmaks of supported APIs (supported by _some_ driver) */
   EGLint ClientAPIsMask;

   char ClientAPIs[1000];   /**< updated by eglQueryString */

   /* XXX temporary - should be thread-specific data (TSD) */
   _EGLThreadInfo *ThreadInfo;

   EGLint NumDrivers;
   _EGLDriver *Drivers[10];
};


extern struct _egl_global _eglGlobal;


extern void
_eglInitGlobals(void);


extern void
_eglDestroyGlobals(void);


extern _EGLThreadInfo *
_eglNewThreadInfo(void);


extern void
_eglDeleteThreadData(_EGLThreadInfo *t);


extern _EGLThreadInfo *
_eglGetCurrentThread(void);


extern void
_eglError(EGLint errCode, const char *msg);


#endif /* EGLGLOBALS_INCLUDED */
