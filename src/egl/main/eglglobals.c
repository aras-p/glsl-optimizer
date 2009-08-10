#include <stdlib.h>
#include "eglglobals.h"
#include "egldisplay.h"
#include "egllog.h"

struct _egl_global _eglGlobal = 
{
   EGL_FALSE
};

/**
 * Init the fields in the _eglGlobal struct
 * May be safely called more than once.
 */
void
_eglInitGlobals(void)
{
   if (!_eglGlobal.Initialized) {
      _eglGlobal.FreeScreenHandle = 1;
      _eglGlobal.Initialized = EGL_TRUE;

      _eglGlobal.ClientAPIsMask = 0x0;
   }
}


/**
 * Should call this via an atexit handler.
 */
void
_eglDestroyGlobals(void)
{
}
