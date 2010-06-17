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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <string.h>
#include "util/u_debug.h"
#include "util/u_memory.h"
#include "util/u_string.h"
#include "egllog.h"

#include "native_x11.h"
#include "x11_screen.h"

#include "state_tracker/drm_driver.h"

#define X11_PROBE_MAGIC 0x11980BE /* "X11PROBE" */

static void
x11_probe_destroy(struct native_probe *nprobe)
{
   if (nprobe->data)
      FREE(nprobe->data);
   FREE(nprobe);
}

static struct native_probe *
x11_create_probe(void *dpy)
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
         FREE(nprobe);
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

static enum native_probe_result
x11_get_probe_result(struct native_probe *nprobe)
{
   if (!nprobe || nprobe->magic != X11_PROBE_MAGIC)
      return NATIVE_PROBE_UNKNOWN;

   /* this is a software driver */
   if (!driver_descriptor.create_screen)
      return NATIVE_PROBE_SUPPORTED;

   /* the display does not support DRI2 or the driver mismatches */
   if (!nprobe->data || strcmp(driver_descriptor.name, (const char *) nprobe->data) != 0)
      return NATIVE_PROBE_FALLBACK;

   return NATIVE_PROBE_EXACT;
}

static struct native_display *
native_create_display(void *dpy, struct native_event_handler *event_handler)
{
   struct native_display *ndpy = NULL;
   boolean force_sw;

   force_sw = debug_get_bool_option("EGL_SOFTWARE", FALSE);
   if (!driver_descriptor.create_screen)
      force_sw = TRUE;

   if (!force_sw) {
      ndpy = x11_create_dri2_display((Display *) dpy, event_handler);
   }

   if (!ndpy) {
      EGLint level = (force_sw) ? _EGL_INFO : _EGL_WARNING;

      _eglLog(level, "use software fallback");
      ndpy = x11_create_ximage_display((Display *) dpy, event_handler);
   }

   return ndpy;
}

static void
x11_init_platform(struct native_platform *nplat)
{
   static char x11_name[32];

   if (nplat->name)
      return;

   util_snprintf(x11_name, sizeof(x11_name), "X11/%s", driver_descriptor.name);

   nplat->name = x11_name;
   nplat->create_probe = x11_create_probe;
   nplat->get_probe_result = x11_get_probe_result;
   nplat->create_display = native_create_display;
}

static struct native_platform x11_platform;

const struct native_platform *
native_get_x11_platform(void)
{
   x11_init_platform(&x11_platform);
   return &x11_platform;
}
