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

#include "native_kms.h"

/* see get_drm_screen_name */
#include <radeon_drm.h>
#include "radeon/drm/radeon_drm.h"

static boolean
kms_display_is_format_supported(struct native_display *ndpy,
                                enum pipe_format fmt, boolean is_color)
{
   return ndpy->screen->is_format_supported(ndpy->screen,
         fmt, PIPE_TEXTURE_2D, 0,
         (is_color) ? PIPE_BIND_RENDER_TARGET :
         PIPE_BIND_DEPTH_STENCIL, 0);
}

static const struct native_config **
kms_display_get_configs(struct native_display *ndpy, int *num_configs)
{
   struct kms_display *kdpy = kms_display(ndpy);
   const struct native_config **configs;

   /* first time */
   if (!kdpy->config) {
      struct native_config *nconf;
      enum pipe_format format;

      kdpy->config = CALLOC(1, sizeof(*kdpy->config));
      if (!kdpy->config)
         return NULL;

      nconf = &kdpy->config->base;

      nconf->buffer_mask =
         (1 << NATIVE_ATTACHMENT_FRONT_LEFT) |
         (1 << NATIVE_ATTACHMENT_BACK_LEFT);

      format = PIPE_FORMAT_B8G8R8A8_UNORM;
      if (!kms_display_is_format_supported(&kdpy->base, format, TRUE)) {
         format = PIPE_FORMAT_A8R8G8B8_UNORM;
         if (!kms_display_is_format_supported(&kdpy->base, format, TRUE))
            format = PIPE_FORMAT_NONE;
      }
      if (format == PIPE_FORMAT_NONE) {
         FREE(kdpy->config);
         kdpy->config = NULL;
         return NULL;
      }

      nconf->color_format = format;

      /* support KMS */
      if (kdpy->resources)
         nconf->scanout_bit = TRUE;
   }

   configs = MALLOC(sizeof(*configs));
   if (configs) {
      configs[0] = &kdpy->config->base;
      if (num_configs)
         *num_configs = 1;
   }

   return configs;
}

static int
kms_display_get_param(struct native_display *ndpy,
                      enum native_param_type param)
{
   int val;

   switch (param) {
   default:
      val = 0;
      break;
   }

   return val;
}

static void
kms_display_destroy(struct native_display *ndpy)
{
   struct kms_display *kdpy = kms_display(ndpy);

   if (kdpy->config)
      FREE(kdpy->config);

   kms_display_fini_modeset(&kdpy->base);

   if (kdpy->base.screen)
      kdpy->base.screen->destroy(kdpy->base.screen);

   if (kdpy->fd >= 0)
      close(kdpy->fd);

   FREE(kdpy);
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
kms_display_init_screen(struct native_display *ndpy)
{
   struct kms_display *kdpy = kms_display(ndpy);
   drmVersionPtr version;
   const char *name;

   version = drmGetVersion(kdpy->fd);
   if (!version) {
      _eglLog(_EGL_WARNING, "invalid fd %d", kdpy->fd);
      return FALSE;
   }

   name = get_drm_screen_name(kdpy->fd, version);
   if (name) {
      kdpy->base.screen =
         kdpy->event_handler->new_drm_screen(&kdpy->base, name, kdpy->fd);
   }
   drmFreeVersion(version);

   if (!kdpy->base.screen) {
      _eglLog(_EGL_WARNING, "failed to create DRM screen");
      return FALSE;
   }

   return TRUE;
}

static struct native_display *
kms_create_display(int fd, struct native_event_handler *event_handler,
                   void *user_data)
{
   struct kms_display *kdpy;

   kdpy = CALLOC_STRUCT(kms_display);
   if (!kdpy)
      return NULL;

   kdpy->fd = fd;
   kdpy->event_handler = event_handler;
   kdpy->base.user_data = user_data;

   if (!kms_display_init_screen(&kdpy->base)) {
      kms_display_destroy(&kdpy->base);
      return NULL;
   }

   kdpy->base.destroy = kms_display_destroy;
   kdpy->base.get_param = kms_display_get_param;
   kdpy->base.get_configs = kms_display_get_configs;

   kms_display_init_modeset(&kdpy->base);

   return &kdpy->base;
}

static struct native_display *
native_create_display(void *dpy, struct native_event_handler *event_handler,
                      void *user_data)
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

   return kms_create_display(fd, event_handler, user_data);
}

static const struct native_platform kms_platform = {
   "KMS", /* name */
   native_create_display
};

const struct native_platform *
native_get_kms_platform(void)
{
   return &kms_platform;
}
