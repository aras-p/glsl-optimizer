
/**
 * Functions related to EGLDisplay.
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "eglcontext.h"
#include "egldisplay.h"
#include "egldriver.h"
#include "eglglobals.h"
#include "eglhash.h"
#include "eglstring.h"


/**
 * Allocate a new _EGLDisplay object for the given nativeDisplay handle.
 * We'll also try to determine the device driver name at this time.
 *
 * Note that nativeDisplay may be an X Display ptr, or a string.
 */
_EGLDisplay *
_eglNewDisplay(NativeDisplayType nativeDisplay)
{
   _EGLDisplay *dpy = (_EGLDisplay *) calloc(1, sizeof(_EGLDisplay));
   if (dpy) {
      EGLuint key = _eglHashGenKey(_eglGlobal.Displays);

      dpy->Handle = (EGLDisplay) key;
      _eglHashInsert(_eglGlobal.Displays, key, dpy);

      dpy->NativeDisplay = nativeDisplay;
#if defined(_EGL_PLATFORM_X)
      dpy->Xdpy = (Display *) nativeDisplay;
#endif

      dpy->DriverName = _eglChooseDriver(dpy);
      if (!dpy->DriverName) {
         free(dpy);
         return NULL;
      }
   }
   return dpy;
}


/**
 * Return the public handle for an internal _EGLDisplay.
 * This is the inverse of _eglLookupDisplay().
 */
EGLDisplay
_eglGetDisplayHandle(_EGLDisplay *display)
{
   if (display)
      return display->Handle;
   else
      return EGL_NO_DISPLAY;
}

 
/**
 * Return the _EGLDisplay object that corresponds to the given public/
 * opaque display handle.
 * This is the inverse of _eglGetDisplayHandle().
 */
_EGLDisplay *
_eglLookupDisplay(EGLDisplay dpy)
{
   EGLuint key = (EGLuint) dpy;
   if (!_eglGlobal.Displays)
      return NULL;
   return (_EGLDisplay *) _eglHashLookup(_eglGlobal.Displays, key);
}


void
_eglSaveDisplay(_EGLDisplay *dpy)
{
   EGLuint key = _eglHashGenKey(_eglGlobal.Displays);
   assert(dpy);
   assert(!dpy->Handle);
   dpy->Handle = (EGLDisplay) key;
   assert(dpy->Handle);
   _eglHashInsert(_eglGlobal.Displays, key, dpy);
}


_EGLDisplay *
_eglGetCurrentDisplay(void)
{
   _EGLContext *ctx = _eglGetCurrentContext();
   if (ctx)
      return ctx->Display;
   else
      return NULL;
}


/**
 * Free all the data hanging of an _EGLDisplay object, but not
 * the object itself.
 */
void
_eglCleanupDisplay(_EGLDisplay *disp)
{
   EGLint i;

   for (i = 0; i < disp->NumConfigs; i++) {
      free(disp->Configs[i]);
   }
   free(disp->Configs);
   disp->Configs = NULL;

   /* XXX incomplete */

   free((void *) disp->DriverName);
   disp->DriverName = NULL;

   /* driver deletes the _EGLDisplay object */
}
