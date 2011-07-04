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

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "util/u_memory.h"
#include "egllog.h"

#include "native_drm.h"

#include "gbm_gallium_drmint.h"

#ifdef HAVE_LIBUDEV
#include <libudev.h>
#endif

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

   /* gbm owns screen */
   ndpy->screen = NULL;
   ndpy_uninit(ndpy);

   if (drmdpy->device_name)
      FREE(drmdpy->device_name);

   if (drmdpy->fd >= 0)
      close(drmdpy->fd);

   FREE(drmdpy);
}

static struct native_display_buffer drm_display_buffer = {
   /* use the helpers */
   drm_display_import_native_buffer,
   drm_display_export_native_buffer
};

static int
drm_display_authenticate(void *user_data, uint32_t magic)
{
   struct native_display *ndpy = user_data;
   struct drm_display *drmdpy = drm_display(ndpy);

   return drmAuthMagic(drmdpy->fd, magic);
}

static char *
drm_get_device_name(int fd)
{
   char *device_name = NULL;
#ifdef HAVE_LIBUDEV
   struct udev *udev;
   struct udev_device *device;
   struct stat buf;
   const char *tmp;

   udev = udev_new();
   if (fstat(fd, &buf) < 0) {
      _eglLog(_EGL_WARNING, "failed to stat fd %d", fd);
      goto out;
   }

   device = udev_device_new_from_devnum(udev, 'c', buf.st_rdev);
   if (device == NULL) {
      _eglLog(_EGL_WARNING,
              "could not create udev device for fd %d", fd);
      goto out;
   }

   tmp = udev_device_get_devnode(device);
   if (!tmp)
      goto out;
   device_name = strdup(tmp);

out:
   udev_device_unref(device);
   udev_unref(udev);

#endif
   return device_name;
}

#ifdef HAVE_WAYLAND_BACKEND

static struct wayland_drm_callbacks wl_drm_callbacks = {
   drm_display_authenticate,
   egl_g3d_wl_drm_helper_reference_buffer,
   egl_g3d_wl_drm_helper_unreference_buffer
};

static boolean
drm_display_bind_wayland_display(struct native_display *ndpy,
                                  struct wl_display *wl_dpy)
{
   struct drm_display *drmdpy = drm_display(ndpy);

   if (drmdpy->wl_server_drm)
      return FALSE;

   drmdpy->wl_server_drm = wayland_drm_init(wl_dpy,
         drmdpy->device_name,
         &wl_drm_callbacks, ndpy);

   if (!drmdpy->wl_server_drm)
      return FALSE;
   
   return TRUE;
}

static boolean
drm_display_unbind_wayland_display(struct native_display *ndpy,
                                    struct wl_display *wl_dpy)
{
   struct drm_display *drmdpy = drm_display(ndpy);

   if (!drmdpy->wl_server_drm)
      return FALSE;

   wayland_drm_uninit(drmdpy->wl_server_drm);
   drmdpy->wl_server_drm = NULL;

   return TRUE;
}

static struct native_display_wayland_bufmgr drm_display_wayland_bufmgr = {
   drm_display_bind_wayland_display,
   drm_display_unbind_wayland_display,
   egl_g3d_wl_drm_common_wl_buffer_get_resource
};

#endif /* HAVE_WAYLAND_BACKEND */

static struct native_surface *
drm_create_pixmap_surface(struct native_display *ndpy,
                              EGLNativePixmapType pix,
                              const struct native_config *nconf)
{
   struct gbm_gallium_drm_bo *bo = (void *) pix;

   return drm_display_create_surface_from_resource(ndpy, bo->resource);
}

static boolean
drm_display_init_screen(struct native_display *ndpy)
{
   return TRUE;
}

static struct native_display *
drm_create_display(struct gbm_gallium_drm_device *gbmdrm,
                   const struct native_event_handler *event_handler)
{
   struct drm_display *drmdpy;

   drmdpy = CALLOC_STRUCT(drm_display);
   if (!drmdpy)
      return NULL;

   drmdpy->fd = gbmdrm->base.base.fd;
   drmdpy->device_name = drm_get_device_name(drmdpy->fd);

   gbmdrm->lookup_egl_image = (struct pipe_resource *(*)(void *, void *))
      event_handler->lookup_egl_image;
   gbmdrm->lookup_egl_image_data = &drmdpy->base;

   drmdpy->event_handler = event_handler;

   drmdpy->base.screen = gbmdrm->screen;

   drmdpy->base.init_screen = drm_display_init_screen;
   drmdpy->base.destroy = drm_display_destroy;
   drmdpy->base.get_param = drm_display_get_param;
   drmdpy->base.get_configs = drm_display_get_configs;

   drmdpy->base.create_pixmap_surface = drm_create_pixmap_surface;

   drmdpy->base.buffer = &drm_display_buffer;
#ifdef HAVE_WAYLAND_BACKEND
   if (drmdpy->device_name)
      drmdpy->base.wayland_bufmgr = &drm_display_wayland_bufmgr;
#endif
   drm_display_init_modeset(&drmdpy->base);

   return &drmdpy->base;
}

static const struct native_event_handler *drm_event_handler;

static struct native_display *
native_create_display(void *dpy, boolean use_sw)
{
   struct gbm_gallium_drm_device *gbm;
   int fd;

   gbm = dpy;

   if (gbm == NULL) {
      fd = open("/dev/dri/card0", O_RDWR);
      gbm = gbm_gallium_drm_device(gbm_create_device(fd));
   }

   if (gbm == NULL)
      return NULL;
   
   if (strcmp(gbm_device_get_backend_name(&gbm->base.base), "drm") != 0 ||
       gbm->base.type != GBM_DRM_DRIVER_TYPE_GALLIUM)
      return NULL;

   return drm_create_display(gbm, drm_event_handler);
}

static const struct native_platform drm_platform = {
   "DRM", /* name */
   native_create_display
};

const struct native_platform *
native_get_drm_platform(const struct native_event_handler *event_handler)
{
   drm_event_handler = event_handler;
   return &drm_platform;
}
