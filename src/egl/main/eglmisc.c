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


/**
 * Examine the individual extension enable/disable flags and recompute
 * the driver's Extensions string.
 */
static void
_eglUpdateExtensionsString(_EGLDriver *drv)
{
   drv->Extensions.String[0] = 0;

   if (drv->Extensions.MESA_screen_surface)
      strcat(drv->Extensions.String, "EGL_MESA_screen_surface ");
   if (drv->Extensions.MESA_copy_context)
      strcat(drv->Extensions.String, "EGL_MESA_copy_context ");
   assert(strlen(drv->Extensions.String) < _EGL_MAX_EXTENSIONS_LEN);
}


static void
_eglUpdateAPIsString(_EGLDriver *drv)
{
   _eglGlobal.ClientAPIs[0] = 0;

   if (_eglGlobal.ClientAPIsMask & EGL_OPENGL_BIT)
      strcat(_eglGlobal.ClientAPIs, "OpenGL ");

   if (_eglGlobal.ClientAPIsMask & EGL_OPENGL_ES_BIT)
      strcat(_eglGlobal.ClientAPIs, "OpenGL_ES ");

   if (_eglGlobal.ClientAPIsMask & EGL_OPENGL_ES2_BIT)
      strcat(_eglGlobal.ClientAPIs, "OpenGL_ES2 ");

   if (_eglGlobal.ClientAPIsMask & EGL_OPENVG_BIT)
      strcat(_eglGlobal.ClientAPIs, "OpenVG ");

   assert(strlen(_eglGlobal.ClientAPIs) < sizeof(_eglGlobal.ClientAPIs));
}



const char *
_eglQueryString(_EGLDriver *drv, EGLDisplay dpy, EGLint name)
{
   (void) drv;
   (void) dpy;
   switch (name) {
   case EGL_VENDOR:
      return _EGL_VENDOR_STRING;
   case EGL_VERSION:
      return drv->Version;
   case EGL_EXTENSIONS:
      _eglUpdateExtensionsString(drv);
      return drv->Extensions.String;
#ifdef EGL_VERSION_1_2
   case EGL_CLIENT_APIS:
      _eglUpdateAPIsString(drv);
      return _eglGlobal.ClientAPIs;
#endif
   default:
      _eglError(EGL_BAD_PARAMETER, "eglQueryString");
      return NULL;
   }
}


EGLBoolean
_eglWaitGL(_EGLDriver *drv, EGLDisplay dpy)
{
   /* just a placeholder */
   (void) drv;
   (void) dpy;
   return EGL_TRUE;
}


EGLBoolean
_eglWaitNative(_EGLDriver *drv, EGLDisplay dpy, EGLint engine)
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
