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
   _EGLDisplay *dpy = (_EGLDisplay *) malloc(sizeof(_EGLDisplay));
   if (dpy) {
      dpy->Handle = _eglHashGenKey(_eglGlobal.Displays);
      _eglHashInsert(_eglGlobal.Displays, dpy->Handle, dpy);
      if (displayName)
         dpy->Name = my_strdup(displayName);
      else
         dpy->Name = NULL;
      dpy->Driver = NULL;  /* this gets set later */
   }
   return dpy;
}


/**
 * Return the _EGLDisplay object that corresponds to the given public/
 * opaque display handle.
 */
_EGLDisplay *
_eglLookupDisplay(EGLDisplay dpy)
{
   _EGLDisplay *d = (_EGLDisplay *) _eglHashLookup(_eglGlobal.Displays, dpy);
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


EGLBoolean 
_eglQueryDisplayMESA(_EGLDriver *drv, EGLDisplay dpy, EGLint attrib, EGLint *value)
{
   return EGL_FALSE;
}
