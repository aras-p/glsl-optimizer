#ifndef EGLGLOBALS_INCLUDED
#define EGLGLOBALS_INCLUDED

#include "egltypedefs.h"
#include "eglhash.h"


struct _egl_global
{
   EGLBoolean Initialized;

   _EGLHashtable *Displays;
   _EGLHashtable *Contexts;
   _EGLHashtable *Surfaces;

   EGLint LastError;

   /* XXX this should be per-thread someday */
   _EGLContext *CurrentContext;
};


extern struct _egl_global _eglGlobal;


extern void
_eglInitGlobals(void);


extern void
_eglDestroyGlobals(void);


extern void
_eglError(EGLint errCode, const char *msg);


#endif /* EGLGLOBALS_INCLUDED */
