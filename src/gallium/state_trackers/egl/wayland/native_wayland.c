/*
 * Mesa 3-D graphics library
 * Version:  7.11
 *
 * Copyright (C) 2011 Benjamin Franzke <benjaminfranzke@googlemail.com>
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

#include "util/u_memory.h"
#include "util/u_inlines.h"

#include "pipe/p_compiler.h"
#include "pipe/p_screen.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "state_tracker/drm_driver.h"

#include "egllog.h"
#include <errno.h>

#include "native_wayland.h"

/* see get_drm_screen_name */
#include <radeon_drm.h>
#include "radeon/drm/radeon_drm_public.h"

#include <wayland-client.h>
#include "wayland-drm-client-protocol.h"
#include "wayland-egl-priv.h"

#include <xf86drm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

static struct native_event_handler *wayland_event_handler;

static void
sync_callback(void *data)
{
   int *done = data;

   *done = 1;
}

static void
force_roundtrip(struct wl_display *display)
{
   int done = 0;

   wl_display_sync_callback(display, sync_callback, &done);
   wl_display_iterate(display, WL_DISPLAY_WRITABLE);
   while (!done)
      wl_display_iterate(display, WL_DISPLAY_READABLE);
}

static const struct native_config **
wayland_display_get_configs (struct native_display *ndpy, int *num_configs)
{
   struct wayland_display *display = wayland_display(ndpy);
   const struct native_config **configs;

   if (!display->config) {
      struct native_config *nconf;
      enum pipe_format format;
      display->config = CALLOC(1, sizeof(*display->config));
      if (!display->config)
         return NULL;
      nconf = &display->config->base;

      nconf->buffer_mask =
         (1 << NATIVE_ATTACHMENT_FRONT_LEFT) |
         (1 << NATIVE_ATTACHMENT_BACK_LEFT);

      format = PIPE_FORMAT_B8G8R8A8_UNORM;

      nconf->color_format = format;
      nconf->window_bit = TRUE;
      nconf->pixmap_bit = TRUE;
   }

   configs = MALLOC(sizeof(*configs));
   if (configs) {
      configs[0] = &display->config->base;
      if (num_configs)
         *num_configs = 1;
   }

   return configs;
}

static int
wayland_display_get_param(struct native_display *ndpy,
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

static boolean
wayland_display_is_pixmap_supported(struct native_display *ndpy,
                                    EGLNativePixmapType pix,
                                    const struct native_config *nconf)
{
   /* all wl_egl_pixmaps are supported */

   return TRUE;
}

static void 
wayland_display_destroy(struct native_display *ndpy)
{
   struct wayland_display *display = wayland_display(ndpy);

   if (display->fd)
      close(display->fd);
   if (display->wl_drm)
      wl_drm_destroy(display->wl_drm);
   if (display->device_name)
      FREE(display->device_name);
   if (display->config)
      FREE(display->config);

   ndpy_uninit(ndpy);

   FREE(display);
}


static struct wl_buffer *
wayland_create_buffer(struct wayland_surface *surface,
                      enum native_attachment attachment)
{
   struct wayland_display *display = surface->display;
   struct pipe_resource *resource;
   struct winsys_handle wsh;
   uint width, height;

   resource = resource_surface_get_single_resource(surface->rsurf, attachment);
   resource_surface_get_size(surface->rsurf, &width, &height);

   wsh.type = DRM_API_HANDLE_TYPE_SHARED;
   display->base.screen->resource_get_handle(display->base.screen, resource, &wsh);

   pipe_resource_reference(&resource, NULL);

   return wl_drm_create_buffer(display->wl_drm, wsh.handle,
                               width, height,
                               wsh.stride, surface->win->visual);
}

static void
wayland_pixmap_destroy(struct wl_egl_pixmap *egl_pixmap)
{
   struct pipe_resource *resource = egl_pixmap->driver_private;

   assert(resource);

   pipe_resource_reference(&resource, NULL);
   if (egl_pixmap->buffer) {
      wl_buffer_destroy(egl_pixmap->buffer);
      egl_pixmap->buffer = NULL;
   }
   
   egl_pixmap->driver_private = NULL;
   egl_pixmap->destroy = NULL;
}

static void
wayland_pixmap_surface_initialize(struct wayland_surface *surface)
{
   struct native_display *ndpy = &surface->display->base;
   struct pipe_resource *resource;
   struct winsys_handle wsh;
   const enum native_attachment front_natt = NATIVE_ATTACHMENT_FRONT_LEFT;

   if (surface->pix->buffer != NULL)
      return;

   resource = resource_surface_get_single_resource(surface->rsurf, front_natt);

   wsh.type = DRM_API_HANDLE_TYPE_SHARED;
   ndpy->screen->resource_get_handle(ndpy->screen, resource, &wsh);

   surface->pix->buffer =
      wl_drm_create_buffer(surface->display->wl_drm, wsh.handle,
                           surface->pix->width, surface->pix->height,
                           wsh.stride, surface->pix->visual);

   surface->pix->destroy        = wayland_pixmap_destroy;
   surface->pix->driver_private = resource;
}

static void
wayland_release_pending_resource(void *data)
{
   struct wayland_surface *surface = data;

   /* FIXME: print internal error */
   if (!surface->pending_resource)
      return;

   pipe_resource_reference(&surface->pending_resource, NULL);
}

static void
wayland_window_surface_handle_resize(struct wayland_surface *surface)
{
   struct wayland_display *display = surface->display;
   struct pipe_resource *front_resource;
   const enum native_attachment front_natt = NATIVE_ATTACHMENT_FRONT_LEFT;
   int i;
   
   front_resource = resource_surface_get_single_resource(surface->rsurf,
                                                         front_natt);
   if (resource_surface_set_size(surface->rsurf,
       surface->win->width, surface->win->height)) {

      if (surface->pending_resource)
         force_roundtrip(display->dpy);

      if (front_resource) {
         surface->pending_resource = front_resource;
         front_resource = NULL;
         wl_display_sync_callback(display->dpy,
                                  wayland_release_pending_resource, surface);
      }

      for (i = 0; i < WL_BUFFER_COUNT; ++i) {
         if (surface->buffer[i])
            wl_buffer_destroy(surface->buffer[i]);
         surface->buffer[i] = NULL;
      }
   }
   pipe_resource_reference(&front_resource, NULL);

   surface->dx = surface->win->dx;
   surface->dy = surface->win->dy;
   surface->win->dx = 0;
   surface->win->dy = 0;
}

static boolean
wayland_surface_validate(struct native_surface *nsurf, uint attachment_mask,
                         unsigned int *seq_num, struct pipe_resource **textures,
                         int *width, int *height)
{
   struct wayland_surface *surface = wayland_surface(nsurf);

   if (surface->type == WL_WINDOW_SURFACE)
      wayland_window_surface_handle_resize(surface);

   if (!resource_surface_add_resources(surface->rsurf, attachment_mask |
                                       surface->attachment_mask))
      return FALSE;

   if (textures)
      resource_surface_get_resources(surface->rsurf, textures, attachment_mask);

   if (seq_num)
      *seq_num = surface->sequence_number;

   resource_surface_get_size(surface->rsurf, (uint *) width, (uint *) height);

   if (surface->type == WL_PIXMAP_SURFACE)
      wayland_pixmap_surface_initialize(surface);

   return TRUE;
}

static void
wayland_frame_callback(struct wl_surface *surf, void *data, uint32_t time)
{
   struct wayland_surface *surface = data;

   surface->block_swap_buffers = FALSE;
}

static INLINE void
wayland_buffers_swap(struct wl_buffer **buffer,
                     enum wayland_buffer_type buf1,
                     enum wayland_buffer_type buf2)
{
   struct wl_buffer *tmp = buffer[buf1];
   buffer[buf1] = buffer[buf2];
   buffer[buf2] = tmp;
}

static boolean
wayland_surface_swap_buffers(struct native_surface *nsurf)
{
   struct wayland_surface *surface = wayland_surface(nsurf);
   struct wayland_display *display = surface->display;

   while (surface->block_swap_buffers)
      wl_display_iterate(display->dpy, WL_DISPLAY_READABLE);

   surface->block_swap_buffers = TRUE;
   wl_display_frame_callback(display->dpy, surface->win->surface,
                             wayland_frame_callback, surface);

   if (surface->type == WL_WINDOW_SURFACE) {
      resource_surface_swap_buffers(surface->rsurf,
       NATIVE_ATTACHMENT_FRONT_LEFT, NATIVE_ATTACHMENT_BACK_LEFT, FALSE);

      wayland_buffers_swap(surface->buffer, WL_BUFFER_FRONT, WL_BUFFER_BACK);

      if (surface->buffer[WL_BUFFER_FRONT] == NULL)
         surface->buffer[WL_BUFFER_FRONT] =
       wayland_create_buffer(surface, NATIVE_ATTACHMENT_FRONT_LEFT);

      wl_surface_attach(surface->win->surface, surface->buffer[WL_BUFFER_FRONT],
                        surface->dx, surface->dy);
     
      resource_surface_get_size(surface->rsurf,
                                (uint *) &surface->win->attached_width,
                                (uint *) &surface->win->attached_height);
      surface->dx = 0;
      surface->dy = 0;
   }

   surface->sequence_number++;
   wayland_event_handler->invalid_surface(&display->base,
        &surface->base, surface->sequence_number);

   return TRUE;
}

static boolean
wayland_surface_present(struct native_surface *nsurf,
                        enum native_attachment natt,
                        boolean preserve,
                        uint swap_interval)
{
   struct wayland_surface *surface = wayland_surface(nsurf);
   uint width, height;
   boolean ret;

   if (preserve || swap_interval)
      return FALSE;

   switch (natt) {
   case NATIVE_ATTACHMENT_FRONT_LEFT:
      ret = TRUE;
      break;
   case NATIVE_ATTACHMENT_BACK_LEFT:
      ret = wayland_surface_swap_buffers(nsurf);
      break;
   default:
      ret = FALSE;
      break;
   }

   if (surface->type == WL_WINDOW_SURFACE) {
      resource_surface_get_size(surface->rsurf, &width, &height);
      wl_buffer_damage(surface->buffer[WL_BUFFER_FRONT], 0, 0, width, height);
      wl_surface_damage(surface->win->surface, 0, 0, width, height);
   }

   return ret;
}

static void
wayland_surface_wait(struct native_surface *nsurf)
{
   /* no-op */
}

static void
wayland_surface_destroy(struct native_surface *nsurf)
{
   struct wayland_surface *surface = wayland_surface(nsurf);
   enum wayland_buffer_type buffer;

   for (buffer = 0; buffer < WL_BUFFER_COUNT; ++buffer) {
      if (surface->buffer[buffer])
         wl_buffer_destroy(surface->buffer[buffer]);
   }

   resource_surface_destroy(surface->rsurf);
   FREE(surface);
}

static struct native_surface *
wayland_create_pixmap_surface(struct native_display *ndpy,
                              EGLNativePixmapType pix,
                              const struct native_config *nconf)
{
   struct wayland_display *display = wayland_display(ndpy);
   struct wayland_surface *surface;
   struct wl_egl_pixmap *egl_pixmap = (struct wl_egl_pixmap *) pix;
   enum native_attachment natt = NATIVE_ATTACHMENT_FRONT_LEFT;

   surface = CALLOC_STRUCT(wayland_surface);
   if (!surface)
      return NULL;

   surface->display = display;

   surface->pending_resource = NULL;
   surface->type = WL_PIXMAP_SURFACE;
   surface->pix = egl_pixmap;

   if (surface->pix->visual == wl_display_get_rgb_visual(display->dpy))
      surface->color_format = PIPE_FORMAT_B8G8R8X8_UNORM;
   else
      surface->color_format = PIPE_FORMAT_B8G8R8A8_UNORM;

   surface->attachment_mask = (1 << NATIVE_ATTACHMENT_FRONT_LEFT);
   
   surface->rsurf = resource_surface_create(display->base.screen,
           surface->color_format,
           PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW |
           PIPE_BIND_DISPLAY_TARGET | PIPE_BIND_SCANOUT);

   if (!surface->rsurf) {
       FREE(surface);
       return NULL;
   }

   resource_surface_set_size(surface->rsurf,
                             egl_pixmap->width, egl_pixmap->height);

   /* the pixmap is already allocated, so import it */
   if (surface->pix->buffer != NULL)
      resource_surface_import_resource(surface->rsurf, natt,
                                       surface->pix->driver_private);

   surface->base.destroy = wayland_surface_destroy;
   surface->base.present = wayland_surface_present;
   surface->base.validate = wayland_surface_validate;
   surface->base.wait = wayland_surface_wait;

   return &surface->base;
}

static struct native_surface *
wayland_create_window_surface(struct native_display *ndpy,
                              EGLNativeWindowType win,
                              const struct native_config *nconf)
{
   struct wayland_display *display = wayland_display(ndpy);
   struct wayland_config *config = wayland_config(nconf);
   struct wayland_surface *surface;

   surface = CALLOC_STRUCT(wayland_surface);
   if (!surface)
      return NULL;

   surface->display = display;
   surface->color_format = config->base.color_format;

   surface->win = (struct wl_egl_window *) win;

   surface->pending_resource = NULL;
   surface->block_swap_buffers = FALSE;
   surface->type = WL_WINDOW_SURFACE;

   surface->buffer[WL_BUFFER_FRONT] = NULL;
   surface->buffer[WL_BUFFER_BACK] = NULL;
   surface->attachment_mask = (1 << NATIVE_ATTACHMENT_FRONT_LEFT) |
                              (1 << NATIVE_ATTACHMENT_BACK_LEFT);

   surface->rsurf = resource_surface_create(display->base.screen,
           surface->color_format,
           PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW |
           PIPE_BIND_DISPLAY_TARGET | PIPE_BIND_SCANOUT);

   if (!surface->rsurf) {
       FREE(surface);
       return NULL;
   }

   surface->base.destroy = wayland_surface_destroy;
   surface->base.present = wayland_surface_present;
   surface->base.validate = wayland_surface_validate;
   surface->base.wait = wayland_surface_wait;

   return &surface->base;
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

static void
drm_handle_device(void *data, struct wl_drm *drm, const char *device)
{
   struct wayland_display *display = data;
   drm_magic_t magic;

   display->device_name = strdup(device);
   if (!display->device_name)
      return;

   display->fd = open(display->device_name, O_RDWR);
   if (display->fd == -1) {
      _eglLog(_EGL_WARNING, "wayland-egl: could not open %s (%s)",
              display->device_name, strerror(errno));
      return;
   }

   drmGetMagic(display->fd, &magic);
   wl_drm_authenticate(display->wl_drm, magic);
}

static void
drm_handle_authenticated(void *data, struct wl_drm *drm)
{
   struct wayland_display *display = data;

   display->authenticated = true;
}

static const struct wl_drm_listener drm_listener = {
   drm_handle_device,
   drm_handle_authenticated
};

static boolean
wayland_display_init_screen(struct native_display *ndpy)
{
   struct wayland_display *display = wayland_display(ndpy);
   drmVersionPtr version;
   const char *driver_name;
   uint32_t id;

   id = wl_display_get_global(display->dpy, "wl_drm", 1);
   if (id == 0)
      wl_display_iterate(display->dpy, WL_DISPLAY_READABLE);
   id = wl_display_get_global(display->dpy, "wl_drm", 1);
   if (id == 0)
      return FALSE;

   display->wl_drm = wl_drm_create(display->dpy, id, 1);
   if (!display->wl_drm)
	   return FALSE;

   wl_drm_add_listener(display->wl_drm, &drm_listener, display);
   force_roundtrip(display->dpy);
   if (display->fd == -1)
      return FALSE;

   force_roundtrip(display->dpy);
   if (!display->authenticated)
      return FALSE;

   version = drmGetVersion(display->fd);
   if (!version) {
      _eglLog(_EGL_WARNING, "invalid fd %d", display->fd);
      return FALSE;
   }

   /* FIXME: share this with native_drm or egl_dri2 */
   driver_name = get_drm_screen_name(display->fd, version);

   display->base.screen =
      wayland_event_handler->new_drm_screen(&display->base,
            driver_name, display->fd);
   drmFreeVersion(version);

   if (!display->base.screen) {
      _eglLog(_EGL_WARNING, "failed to create DRM screen");
      return FALSE;
   }

   return TRUE;
}


static void
wayland_set_event_handler(struct native_event_handler *event_handler)
{
   wayland_event_handler = event_handler;
}

static struct pipe_resource *
wayland_display_import_buffer(struct native_display *ndpy,
                              const struct pipe_resource *templ,
                              void *buf)
{
   return ndpy->screen->resource_from_handle(ndpy->screen,
         templ, (struct winsys_handle *) buf);
}

static boolean
wayland_display_export_buffer(struct native_display *ndpy,
                              struct pipe_resource *res,
                              void *buf)
{
   return ndpy->screen->resource_get_handle(ndpy->screen,
         res, (struct winsys_handle *) buf);
}

static struct native_display_buffer wayland_display_buffer = {
   wayland_display_import_buffer,
   wayland_display_export_buffer
};

static struct native_display *
wayland_display_create(void *dpy, boolean use_sw, void *user_data)
{
   struct wayland_display *display;

   display = CALLOC_STRUCT(wayland_display);
   if (!display)
      return NULL;

   display->base.user_data = user_data;

   display->dpy = dpy;
   if (!display->dpy) {
      wayland_display_destroy(&display->base);
      return NULL;
   }

   if (!wayland_display_init_screen(&display->base)) {
      wayland_display_destroy(&display->base);
      return NULL;
   }

   display->base.destroy = wayland_display_destroy;
   display->base.get_param = wayland_display_get_param;
   display->base.get_configs = wayland_display_get_configs;
   display->base.is_pixmap_supported = wayland_display_is_pixmap_supported;
   display->base.create_window_surface = wayland_create_window_surface;
   display->base.create_pixmap_surface = wayland_create_pixmap_surface;
   display->base.buffer = &wayland_display_buffer;

   return &display->base;
}

static const struct native_platform wayland_platform = {
   "wayland", /* name */
   wayland_set_event_handler,
   wayland_display_create
};

const struct native_platform *
native_get_wayland_platform(void)
{
   return &wayland_platform;
}
