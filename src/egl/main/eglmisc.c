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
 * Small/misc EGL functions
 */


#include <assert.h>
#include <string.h>
#include "eglglobals.h"
#include "eglmisc.h"
#include "egldisplay.h"


/**
 * Examine the individual extension enable/disable flags and recompute
 * the driver's Extensions string.
 */
static void
_eglUpdateExtensionsString(_EGLDisplay *dpy)
{
   char *exts = dpy->Extensions.String;

   if (exts[0])
      return;

   if (dpy->Extensions.MESA_screen_surface)
      strcat(exts, "EGL_MESA_screen_surface ");
   if (dpy->Extensions.MESA_copy_context)
      strcat(exts, "EGL_MESA_copy_context ");
   assert(strlen(exts) < _EGL_MAX_EXTENSIONS_LEN);
}


static void
_eglUpdateAPIsString(_EGLDisplay *dpy)
{
   char *apis = dpy->ClientAPIs;

   if (apis[0] || !dpy->ClientAPIsMask)
      return;

   if (dpy->ClientAPIsMask & EGL_OPENGL_BIT)
      strcat(apis, "OpenGL ");

   if (dpy->ClientAPIsMask & EGL_OPENGL_ES_BIT)
      strcat(apis, "OpenGL_ES ");

   if (dpy->ClientAPIsMask & EGL_OPENGL_ES2_BIT)
      strcat(apis, "OpenGL_ES2 ");

   if (dpy->ClientAPIsMask & EGL_OPENVG_BIT)
      strcat(apis, "OpenVG ");

   assert(strlen(apis) < sizeof(dpy->ClientAPIs));
}


const char *
_eglQueryString(_EGLDriver *drv, _EGLDisplay *dpy, EGLint name)
{
   (void) drv;
   (void) dpy;
   switch (name) {
   case EGL_VENDOR:
      return _EGL_VENDOR_STRING;
   case EGL_VERSION:
      return dpy->Version;
   case EGL_EXTENSIONS:
      _eglUpdateExtensionsString(dpy);
      return dpy->Extensions.String;
#ifdef EGL_VERSION_1_2
   case EGL_CLIENT_APIS:
      _eglUpdateAPIsString(dpy);
      return dpy->ClientAPIs;
#endif
   default:
      _eglError(EGL_BAD_PARAMETER, "eglQueryString");
      return NULL;
   }
}


EGLBoolean
_eglWaitClient(_EGLDriver *drv, _EGLDisplay *dpy, _EGLContext *ctx)
{
   /* just a placeholder */
   (void) drv;
   (void) dpy;
   (void) ctx;
   return EGL_TRUE;
}


EGLBoolean
_eglWaitNative(_EGLDriver *drv, _EGLDisplay *dpy, EGLint engine)
{
   /* just a placeholder */
   (void) drv;
   (void) dpy;
   switch (engine) {
   case EGL_CORE_NATIVE_ENGINE:
      break;
   default:
      _eglError(EGL_BAD_PARAMETER, "eglWaitNative(engine)");
      return EGL_FALSE;
   }

   return EGL_TRUE;
}
