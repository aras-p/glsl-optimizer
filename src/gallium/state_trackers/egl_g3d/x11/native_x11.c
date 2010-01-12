/*
 * Mesa 3-D graphics library
 * Version:  7.8
 *
 * Copyright (C) 2009-2010 Chia-I Wu <olv@0xlab.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <string.h>
#include "util/u_debug.h"
#include "state_tracker/drm_api.h"
#include "egllog.h"

#include "native_x11.h"

static struct drm_api *api;

const char *
native_get_name(void)
{
   static char x11_name[32];

   if (!api)
      api = drm_api_create();

   if (api)
      snprintf(x11_name, sizeof(x11_name), "X11/%s", api->name);
   else
      snprintf(x11_name, sizeof(x11_name), "X11");

   return x11_name;
}

struct native_display *
native_create_display(EGLNativeDisplayType dpy,
                      native_flush_frontbuffer flush_frontbuffer)
{
   struct native_display *ndpy = NULL;
   boolean force_sw;

   if (!api)
      api = drm_api_create();

   force_sw = debug_get_bool_option("EGL_SOFTWARE", FALSE);
   if (api && !force_sw) {
      ndpy = x11_create_dri2_display(dpy, api, flush_frontbuffer);
   }

   if (!ndpy) {
      EGLint level = (force_sw) ? _EGL_INFO : _EGL_WARNING;

      _eglLog(level, "use software fallback");
      ndpy = x11_create_ximage_display(dpy, TRUE, flush_frontbuffer);
   }

   return ndpy;
}
