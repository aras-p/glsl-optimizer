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
};


/**
 * Global library data
 */
struct _egl_global
{
   EGLBoolean Initialized;

   _EGLHashtable *Displays;
   _EGLHashtable *Contexts;
   _EGLHashtable *Surfaces;

   EGLScreenMESA FreeScreenHandle;

   /* XXX temporary */
   _EGLThreadInfo ThreadInfo;
};


extern struct _egl_global _eglGlobal;


extern void
_eglInitGlobals(void);


extern void
_eglDestroyGlobals(void);


extern _EGLThreadInfo *
_eglGetCurrentThread(void);


extern void
_eglError(EGLint errCode, const char *msg);


extern EGLScreenMESA
_eglAllocScreenHandle(void);


#endif /* EGLGLOBALS_INCLUDED */
