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

#include <stdio.h>
#include <string.h>
#include "util/u_debug.h"
#include "util/u_memory.h"
#include "state_tracker/drm_api.h"
#include "egllog.h"

#include "native_x11.h"
#include "x11_screen.h"

#define X11_PROBE_MAGIC 0x11980BE /* "X11PROBE" */

static struct drm_api *api;

static void
x11_probe_destroy(struct native_probe *nprobe)
{
   if (nprobe->data)
      free(nprobe->data);
   free(nprobe);
}

struct native_probe *
native_create_probe(EGLNativeDisplayType dpy)
{
   struct native_probe *nprobe;
   struct x11_screen *xscr;
   int scr;
   const char *driver_name = NULL;
   Display *xdpy;

   nprobe = CALLOC_STRUCT(native_probe);
   if (!nprobe)
      return NULL;

   xdpy = dpy;
   if (!xdpy) {
      xdpy = XOpenDisplay(NULL);
      if (!xdpy) {
         free(nprobe);
         return NULL;
      }
   }

   scr = DefaultScreen(xdpy);
   xscr = x11_screen_create(xdpy, scr);
   if (xscr) {
      if (x11_screen_support(xscr, X11_SCREEN_EXTENSION_DRI2)) {
         driver_name = x11_screen_probe_dri2(xscr, NULL, NULL);
         if (driver_name)
            nprobe->data = strdup(driver_name);
      }

      x11_screen_destroy(xscr);
   }

   if (xdpy != dpy)
      XCloseDisplay(xdpy);

   nprobe->magic = X11_PROBE_MAGIC;
   nprobe->display = dpy;

   nprobe->destroy = x11_probe_destroy;

   return nprobe;
}

enum native_probe_result
native_get_probe_result(struct native_probe *nprobe)
{
   if (!nprobe || nprobe->magic != X11_PROBE_MAGIC)
      return NATIVE_PROBE_UNKNOWN;

   if (!api)
      api = drm_api_create();

   /* this is a software driver */
   if (!api)
      return NATIVE_PROBE_SUPPORTED;

   /* the display does not support DRI2 or the driver mismatches */
   if (!nprobe->data || strcmp(api->name, (const char *) nprobe->data) != 0)
      return NATIVE_PROBE_FALLBACK;

   return NATIVE_PROBE_EXACT;
}

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
                      struct native_event_handler *event_handler)
{
   struct native_display *ndpy = NULL;
   boolean force_sw;

   if (!api)
      api = drm_api_create();

   force_sw = debug_get_bool_option("EGL_SOFTWARE", FALSE);
   if (api && !force_sw) {
      ndpy = x11_create_dri2_display(dpy, event_handler, api);
   }

   if (!ndpy) {
      EGLint level = (force_sw) ? _EGL_INFO : _EGL_WARNING;
      boolean use_shm;

      /*
       * XXX st/mesa calls pipe_screen::update_buffer in st_validate_state.
       * When SHM is used, there is a good chance that the shared memory
       * segment is detached before the softpipe tile cache is flushed.
       */
      use_shm = FALSE;
      _eglLog(level, "use software%s fallback", (use_shm) ? " (SHM)" : "");
      ndpy = x11_create_ximage_display(dpy, event_handler, use_shm);
   }

   return ndpy;
}
