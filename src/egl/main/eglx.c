
/**
 * X-specific EGL code.
 *
 * Any glue code needed to make EGL work with X is placed in this file.
 */


#include <assert.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include "eglx.h"



/**
 * Given an X Display ptr (at dpy->Xdpy) try to determine the appropriate
 * device driver.  Return its name.
 */
const char *
_xeglChooseDriver(_EGLDisplay *dpy)
{
#ifdef _EGL_PLATFORM_X
   _XPrivDisplay xdpy = (_XPrivDisplay) dpy->Xdpy;

   assert(dpy);
   assert(dpy->Xdpy);

   printf("%s\n", xdpy->display_name);

   return "foo"; /* XXX todo */
#else
   return NULL;
#endif
}


