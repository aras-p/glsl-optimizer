/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


/**
 * X-specific EGL code.
 *
 * Any glue code needed to make EGL work with X is placed in this file.
 */


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>

#include "egldriver.h"
#include "egllog.h"
#include "eglstring.h"
#include "eglx.h"


static const char *DefaultDRIDriver = "egl_xdri";
static const char *DefaultSoftDriver = "egl_softpipe";


/**
 * Given an X Display ptr (at dpy->Xdpy) try to determine the appropriate
 * device driver.  Return its name.
 *
 * This boils down to whether to use the egl_xdri.so driver which will
 * load a DRI driver or the egl_softpipe.so driver that'll do software
 * rendering on Xlib.
 */
const char *
_xeglChooseDriver(_EGLDisplay *dpy)
{
#ifdef _EGL_PLATFORM_X
   _XPrivDisplay xdpy;
   int screen;
   const char *driverName;

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

   screen = DefaultScreen(dpy->Xdpy);

   /* See if we can choose a DRI/DRM driver */
   driverName = _eglChooseDRMDriver(screen);
   if (driverName) {
      /* DRI is available */
      free((void *) driverName);
      driverName = _eglstrdup(DefaultDRIDriver);
   }
   else {
      driverName = _eglstrdup(DefaultSoftDriver);
   }

   _eglLog(_EGL_DEBUG, "_xeglChooseDriver: %s", driverName);

   return driverName;
#else
   return NULL;
#endif
}


