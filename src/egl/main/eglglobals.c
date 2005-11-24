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
   const char *s;

   if (_eglGlobal.LastError == EGL_SUCCESS) {
      _eglGlobal.LastError = errCode;

      switch (errCode) {
      case EGL_BAD_ACCESS:
         s = "EGL_BAD_ACCESS";
         break;
      case EGL_BAD_ALLOC:
         s = "EGL_BAD_ALLOC";
         break;
      case EGL_BAD_ATTRIBUTE:
         s = "EGL_BAD_ATTRIBUTE";
         break;
      case EGL_BAD_CONFIG:
         s = "EGL_BAD_CONFIG";
         break;
      case EGL_BAD_CONTEXT:
         s = "EGL_BAD_CONTEXT";
         break;
      case EGL_BAD_CURRENT_SURFACE:
         s = "EGL_BAD_CURRENT_SURFACE";
         break;
      case EGL_BAD_DISPLAY:
         s = "EGL_BAD_DISPLAY";
         break;
      case EGL_BAD_MATCH:
         s = "EGL_BAD_MATCH";
         break;
      case EGL_BAD_NATIVE_PIXMAP:
         s = "EGL_BAD_NATIVE_PIXMAP";
         break;
      case EGL_BAD_NATIVE_WINDOW:
         s = "EGL_BAD_NATIVE_WINDOW";
         break;
      case EGL_BAD_PARAMETER:
         s = "EGL_BAD_PARAMETER";
         break;
      case EGL_BAD_SURFACE:
         s = "EGL_BAD_SURFACE";
         break;
      case EGL_BAD_SCREEN_MESA:
         s = "EGL_BAD_SCREEN_MESA";
         break;
      case EGL_BAD_MODE_MESA:
         s = "EGL_BAD_MODE_MESA";
         break;
      default:
         s = "other";
      }
      /* XXX temporary */
      fprintf(stderr, "EGL user error 0x%x (%s) in %s\n", errCode, s, msg);
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
