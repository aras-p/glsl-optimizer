
/**
 * X-specific EGL code.
 *
 * Any glue code needed to make EGL work with X is placed in this file.
 */


#include <assert.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include "eglx.h"


static const char *DefaultXDriver = "softpipe_egl";


/**
 * Given an X Display ptr (at dpy->Xdpy) try to determine the appropriate
 * device driver.  Return its name.
 */
const char *
_xeglChooseDriver(_EGLDisplay *dpy)
{
#ifdef _EGL_PLATFORM_X
   _XPrivDisplay xdpy;

   assert(dpy);

   if (!dpy->Xdpy) {
      dpy->Xdpy = XOpenDisplay(NULL);
      if (!dpy->Xdpy) {
         /* can't open X display -> can't use X-based driver */
         return NULL;
      }
   }
   xdpy = (_XPrivDisplay) dpy->Xdpy;

   assert(dpy->Xdpy);

   printf("%s\n", xdpy->display_name);

   return DefaultXDriver; /* XXX temporary */
#else
   return NULL;
#endif
}


