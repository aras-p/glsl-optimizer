#include <stdlib.h>
#include "eglglobals.h"
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
      _eglGlobal.Displays = _eglNewHashTable();
      _eglGlobal.Surfaces = _eglNewHashTable();
      _eglGlobal.FreeScreenHandle = 1;
      _eglGlobal.Initialized = EGL_TRUE;

      _eglGlobal.ClientAPIsMask = 0x0;

      if (!_eglInitCurrent())
         _eglLog(_EGL_FATAL, "failed to initialize \"current\" system");
   }
}


/**
 * Should call this via an atexit handler.
 */
void
_eglDestroyGlobals(void)
{
   _eglFiniCurrent();
   /* XXX TODO walk over table entries, deleting each */
   _eglDeleteHashTable(_eglGlobal.Displays);
   _eglDeleteHashTable(_eglGlobal.Surfaces);
}
