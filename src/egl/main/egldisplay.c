#include <stdlib.h>
#include <string.h>
#include "eglcontext.h"
#include "egldisplay.h"
#include "eglglobals.h"
#include "eglhash.h"


static char *
my_strdup(const char *s)
{
   int l = strlen(s);
   char *s2 = malloc(l + 1);
   strcpy(s2, s);
   return s2;
}


/**
 * We're assuming that the NativeDisplayType parameter is actually
 * a string.
 * Return a new _EGLDisplay object for the given displayName
 */
_EGLDisplay *
_eglNewDisplay(NativeDisplayType displayName)
{
   _EGLDisplay *dpy = (_EGLDisplay *) calloc(1, sizeof(_EGLDisplay));
   if (dpy) {
      EGLuint key = _eglHashGenKey(_eglGlobal.Displays);
      dpy->Handle = (EGLDisplay) key;
      _eglHashInsert(_eglGlobal.Displays, key, dpy);
      if (displayName)
         dpy->Name = my_strdup((char *) displayName);
      else
         dpy->Name = NULL;
      dpy->Driver = NULL;  /* this gets set later */
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
   _EGLDisplay *d = (_EGLDisplay *) _eglHashLookup(_eglGlobal.Displays, key);
   return d;
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


void
_eglCleanupDisplay(_EGLDisplay *disp)
{
   /* XXX incomplete */
   free(disp->Configs);
   free(disp->Name);
   /* driver deletes _EGLDisplay */
}
