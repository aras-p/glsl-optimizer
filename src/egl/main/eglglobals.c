#include <stdio.h>
#include "eglglobals.h"


struct _egl_global _eglGlobal = { EGL_FALSE };


/**
 * Init the fields in the _eglGlobal struct
 * May be safely called more than once.
 */
void
_eglInitGlobals(void)
{
   if (!_eglGlobal.Initialized) {
      _eglGlobal.Displays = _eglNewHashTable();
      _eglGlobal.Contexts = _eglNewHashTable();
      _eglGlobal.Surfaces = _eglNewHashTable();
      _eglGlobal.FreeScreenHandle = 1;
      _eglGlobal.CurrentContext = EGL_NO_CONTEXT;
      _eglGlobal.LastError = EGL_SUCCESS;
      _eglGlobal.Initialized = EGL_TRUE;
   }
}


/**
 * Should call this via an atexit handler.
 */
void
_eglDestroyGlobals(void)
{
   /* XXX TODO walk over table entries, deleting each */
   _eglDeleteHashTable(_eglGlobal.Displays);
   _eglDeleteHashTable(_eglGlobal.Contexts);
   _eglDeleteHashTable(_eglGlobal.Surfaces);
}



/**
 * Record EGL error code.
 */
void
_eglError(EGLint errCode, const char *msg)
{
   if (_eglGlobal.LastError == EGL_SUCCESS) {
      _eglGlobal.LastError = errCode;
      /* XXX temporary */
      fprintf(stderr, "EGL Error 0x%x in %s\n", errCode, msg);
   }
}


/**
 * Return a new screen handle/ID.
 * NOTE: we never reuse these!
 */
EGLScreenMESA
_eglAllocScreenHandle(void)
{
   EGLScreenMESA s = _eglGlobal.FreeScreenHandle;
   _eglGlobal.FreeScreenHandle++;
   return s;
}
