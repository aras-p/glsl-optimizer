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
   _EGLHashtable *Contexts;
   _EGLHashtable *Surfaces;

   EGLScreenMESA FreeScreenHandle;

   /* XXX these may be temporary */
   EGLBoolean OpenGLESAPISupported;
   EGLBoolean OpenVGAPISupported;

   /* XXX temporary - should be thread-specific data (TSD) */
   _EGLThreadInfo *ThreadInfo;
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
