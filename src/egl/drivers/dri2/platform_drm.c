/*
 * Copyright © 2011 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Kristian Høgsberg <krh@bitplanet.net>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <xf86drm.h>

#include "egl_dri2.h"

static int
dri2_drm_authenticate(_EGLDisplay *disp, uint32_t id)
{
   struct dri2_egl_display *dri2_dpy = dri2_egl_display(disp);

   return drmAuthMagic(dri2_dpy->fd, id);
}

EGLBoolean
dri2_initialize_drm(_EGLDriver *drv, _EGLDisplay *disp)
{
   struct dri2_egl_display *dri2_dpy;
   int i;

   dri2_dpy = malloc(sizeof *dri2_dpy);
   if (!dri2_dpy)
      return _eglError(EGL_BAD_ALLOC, "eglInitialize");
   
   memset(dri2_dpy, 0, sizeof *dri2_dpy);

   disp->DriverData = (void *) dri2_dpy;
   dri2_dpy->fd = (int) (intptr_t) disp->PlatformDisplay;

   dri2_dpy->driver_name = dri2_get_driver_for_fd(dri2_dpy->fd);
   if (dri2_dpy->driver_name == NULL)
      return _eglError(EGL_BAD_ALLOC, "DRI2: failed to get driver name");

   dri2_dpy->device_name = dri2_get_device_name_for_fd(dri2_dpy->fd);
   if (dri2_dpy->device_name == NULL) {
      _eglError(EGL_BAD_ALLOC, "DRI2: failed to get device name");
      goto cleanup_driver_name;
   }

   if (!dri2_load_driver(disp))
      goto cleanup_device_name;

   dri2_dpy->extensions[0] = &image_lookup_extension.base;
   dri2_dpy->extensions[1] = &use_invalidate.base;
   dri2_dpy->extensions[2] = NULL;

   if (!dri2_create_screen(disp))
      goto cleanup_driver;

   for (i = 0; dri2_dpy->driver_configs[i]; i++)
      dri2_add_config(disp, dri2_dpy->driver_configs[i], i + 1, 0, 0, NULL);

#ifdef HAVE_WAYLAND_PLATFORM
   disp->Extensions.WL_bind_wayland_display = EGL_TRUE;
#endif
   dri2_dpy->authenticate = dri2_drm_authenticate;
   
   /* we're supporting EGL 1.4 */
   disp->VersionMajor = 1;
   disp->VersionMinor = 4;

   return EGL_TRUE;

 cleanup_driver:
   dlclose(dri2_dpy->driver);
 cleanup_device_name:
   free(dri2_dpy->device_name);
 cleanup_driver_name:
   free(dri2_dpy->driver_name);

   return EGL_FALSE;
}
