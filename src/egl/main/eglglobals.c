#include <stdio.h>
#include <stdlib.h>
#include "eglglobals.h"

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

      /* XXX temporary */
      _eglGlobal.ThreadInfo = _eglNewThreadInfo();
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
   _eglDeleteHashTable(_eglGlobal.Surfaces);
}


/**
 * Allocate and init a new _EGLThreadInfo object.
 */
_EGLThreadInfo *
_eglNewThreadInfo(void)
{
   _EGLThreadInfo *t = (_EGLThreadInfo *) calloc(1, sizeof(_EGLThreadInfo));
   if (t) {
      t->CurrentContext = EGL_NO_CONTEXT;
      t->LastError = EGL_SUCCESS;
      t->CurrentAPI = EGL_OPENGL_ES_API;  /* default, per EGL spec */
   }
   return t;
}


/**
 * Delete/free a _EGLThreadInfo object.
 */
void
_eglDeleteThreadData(_EGLThreadInfo *t)
{
   free(t);
}



/**
 * Return pointer to calling thread's _EGLThreadInfo object.
 * Create a new one if needed.
 * Should never return NULL.
 */
_EGLThreadInfo *
_eglGetCurrentThread(void)
{
   _eglInitGlobals();

   /* XXX temporary */
   return _eglGlobal.ThreadInfo;
}


/**
 * Record EGL error code.
 */
void
_eglError(EGLint errCode, const char *msg)
{
   _EGLThreadInfo *t = _eglGetCurrentThread();
   const char *s;

   if (t->LastError == EGL_SUCCESS) {
      t->LastError = errCode;

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
