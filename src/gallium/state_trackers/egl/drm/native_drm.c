/*
 * Mesa 3-D graphics library
 * Version:  7.8
 *
 * Copyright (C) 2010 Chia-I Wu <olv@0xlab.org>
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "util/u_memory.h"
#include "egllog.h"

#include "native_drm.h"

/* see get_drm_screen_name */
#include <radeon_drm.h>
#include "radeon/drm/radeon_drm_public.h"

static boolean
drm_display_is_format_supported(struct native_display *ndpy,
                                enum pipe_format fmt, boolean is_color)
{
   return ndpy->screen->is_format_supported(ndpy->screen,
         fmt, PIPE_TEXTURE_2D, 0,
         (is_color) ? PIPE_BIND_RENDER_TARGET :
         PIPE_BIND_DEPTH_STENCIL);
}

static const struct native_config **
drm_display_get_configs(struct native_display *ndpy, int *num_configs)
{
   struct drm_display *drmdpy = drm_display(ndpy);
   const struct native_config **configs;

   /* first time */
   if (!drmdpy->config) {
      struct native_config *nconf;
      enum pipe_format format;

      drmdpy->config = CALLOC(1, sizeof(*drmdpy->config));
      if (!drmdpy->config)
         return NULL;

      nconf = &drmdpy->config->base;

      nconf->buffer_mask =
         (1 << NATIVE_ATTACHMENT_FRONT_LEFT) |
         (1 << NATIVE_ATTACHMENT_BACK_LEFT);

      format = PIPE_FORMAT_B8G8R8A8_UNORM;
      if (!drm_display_is_format_supported(&drmdpy->base, format, TRUE)) {
         format = PIPE_FORMAT_A8R8G8B8_UNORM;
         if (!drm_display_is_format_supported(&drmdpy->base, format, TRUE))
            format = PIPE_FORMAT_NONE;
      }
      if (format == PIPE_FORMAT_NONE) {
         FREE(drmdpy->config);
         drmdpy->config = NULL;
         return NULL;
      }

      nconf->color_format = format;

      /* support KMS */
      if (drmdpy->resources)
         nconf->scanout_bit = TRUE;
   }

   configs = MALLOC(sizeof(*configs));
   if (configs) {
      configs[0] = &drmdpy->config->base;
      if (num_configs)
         *num_configs = 1;
   }

   return configs;
}

static int
drm_display_get_param(struct native_display *ndpy,
                      enum native_param_type param)
{
   int val;

   switch (param) {
   case NATIVE_PARAM_USE_NATIVE_BUFFER:
   case NATIVE_PARAM_PRESERVE_BUFFER:
   case NATIVE_PARAM_MAX_SWAP_INTERVAL:
   default:
      val = 0;
      break;
   }

   return val;
}

static void
drm_display_destroy(struct native_display *ndpy)
{
   struct drm_display *drmdpy = drm_display(ndpy);

   if (drmdpy->config)
      FREE(drmdpy->config);

   drm_display_fini_modeset(&drmdpy->base);

   ndpy_uninit(ndpy);

   if (drmdpy->fd >= 0)
      close(drmdpy->fd);

   FREE(drmdpy);
}

static const char *
get_drm_screen_name(int fd, drmVersionPtr version)
{
   const char *name = version->name;

   if (name && !strcmp(name, "radeon")) {
      int chip_id;
      struct drm_radeon_info info;

      memset(&info, 0, sizeof(info));
      info.request = RADEON_INFO_DEVICE_ID;
      info.value = pointer_to_intptr(&chip_id);
      if (drmCommandWriteRead(fd, DRM_RADEON_INFO, &info, sizeof(info)) != 0)
         return NULL;

      name = is_r3xx(chip_id) ? "r300" : "r600";
   }

   return name;
}

/**
 * Initialize KMS and pipe screen.
 */
static boolean
drm_display_init_screen(struct native_display *ndpy)
{
   struct drm_display *drmdpy = drm_display(ndpy);
   drmVersionPtr version;
   const char *name;

   version = drmGetVersion(drmdpy->fd);
   if (!version) {
      _eglLog(_EGL_WARNING, "invalid fd %d", drmdpy->fd);
      return FALSE;
   }

   name = get_drm_screen_name(drmdpy->fd, version);
   if (name) {
      drmdpy->base.screen =
         drmdpy->event_handler->new_drm_screen(&drmdpy->base, name, drmdpy->fd);
   }
   drmFreeVersion(version);

   if (!drmdpy->base.screen) {
      _eglLog(_EGL_DEBUG, "failed to create DRM screen");
      return FALSE;
   }

   return TRUE;
}

static struct pipe_resource *
drm_display_import_buffer(struct native_display *ndpy,
                          const struct pipe_resource *templ,
                          void *buf)
{
   return ndpy->screen->resource_from_handle(ndpy->screen,
         templ, (struct winsys_handle *) buf);
}

static boolean
drm_display_export_buffer(struct native_display *ndpy,
                          struct pipe_resource *res,
                          void *buf)
{
   return ndpy->screen->resource_get_handle(ndpy->screen,
         res, (struct winsys_handle *) buf);
}

static struct native_display_buffer drm_display_buffer = {
   drm_display_import_buffer,
   drm_display_export_buffer
};

static struct native_display *
drm_create_display(int fd, struct native_event_handler *event_handler,
                   void *user_data)
{
   struct drm_display *drmdpy;

   drmdpy = CALLOC_STRUCT(drm_display);
   if (!drmdpy)
      return NULL;

   drmdpy->fd = fd;
   drmdpy->event_handler = event_handler;
   drmdpy->base.user_data = user_data;

   if (!drm_display_init_screen(&drmdpy->base)) {
      drm_display_destroy(&drmdpy->base);
      return NULL;
   }

   drmdpy->base.destroy = drm_display_destroy;
   drmdpy->base.get_param = drm_display_get_param;
   drmdpy->base.get_configs = drm_display_get_configs;

   drmdpy->base.buffer = &drm_display_buffer;
   drm_display_init_modeset(&drmdpy->base);

   return &drmdpy->base;
}

static struct native_event_handler *drm_event_handler;

static void
native_set_event_handler(struct native_event_handler *event_handler)
{
   drm_event_handler = event_handler;
}

static struct native_display *
native_create_display(void *dpy, boolean use_sw, void *user_data)
{
   int fd;

   if (dpy) {
      fd = dup((int) pointer_to_intptr(dpy));
   }
   else {
      fd = open("/dev/dri/card0", O_RDWR);
   }
   if (fd < 0)
      return NULL;

   return drm_create_display(fd, drm_event_handler, user_data);
}

static const struct native_platform drm_platform = {
   "DRM", /* name */
   native_set_event_handler,
   native_create_display
};

const struct native_platform *
native_get_drm_platform(void)
{
   return &drm_platform;
}
