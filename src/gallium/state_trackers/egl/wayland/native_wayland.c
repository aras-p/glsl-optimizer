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

#include "native_wayland.h"

static const struct native_event_handler *wayland_event_handler;

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
   int i;

   if (!display->config) {
      struct native_config *nconf;
      display->config = CALLOC(2, sizeof(*display->config));
      if (!display->config)
         return NULL;

      for (i = 0; i < 2; ++i) {
         nconf = &display->config[i].base;
         
         nconf->buffer_mask =
            (1 << NATIVE_ATTACHMENT_FRONT_LEFT) |
            (1 << NATIVE_ATTACHMENT_BACK_LEFT);
         
         nconf->window_bit = TRUE;
         nconf->pixmap_bit = TRUE;
      }

      display->config[0].base.color_format = PIPE_FORMAT_B8G8R8A8_UNORM;
      display->config[1].base.color_format = PIPE_FORMAT_B8G8R8X8_UNORM;
   }

   configs = MALLOC(2 * sizeof(*configs));
   if (configs) {
      configs[0] = &display->config[0].base;
      configs[1] = &display->config[1].base;
      if (num_configs)
         *num_configs = 2;
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
   struct wayland_display *display = wayland_display(&surface->display->base);
   const enum native_attachment front_natt = NATIVE_ATTACHMENT_FRONT_LEFT;

   if (surface->pix->buffer != NULL)
      return;

   surface->pix->buffer  = display->create_buffer(display, surface, front_natt);
   surface->pix->destroy = wayland_pixmap_destroy;
   surface->pix->driver_private =
      resource_surface_get_single_resource(surface->rsurf, front_natt);
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

      surface->dx = surface->win->dx;
      surface->dy = surface->win->dy;
   }
   pipe_resource_reference(&front_resource, NULL);
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
                                    NATIVE_ATTACHMENT_FRONT_LEFT,
                                    NATIVE_ATTACHMENT_BACK_LEFT, FALSE);

      wayland_buffers_swap(surface->buffer, WL_BUFFER_FRONT, WL_BUFFER_BACK);

      if (surface->buffer[WL_BUFFER_FRONT] == NULL)
         surface->buffer[WL_BUFFER_FRONT] =
            display->create_buffer(display, surface,
                                   NATIVE_ATTACHMENT_FRONT_LEFT);

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
                                          &surface->base,
                                          surface->sequence_number);

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
   uint bind = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW |
      PIPE_BIND_DISPLAY_TARGET | PIPE_BIND_SCANOUT;

   surface = CALLOC_STRUCT(wayland_surface);
   if (!surface)
      return NULL;

   surface->display = display;

   surface->pending_resource = NULL;
   surface->type = WL_PIXMAP_SURFACE;
   surface->pix = egl_pixmap;

   if (nconf)
      surface->color_format = nconf->color_format;
   else /* FIXME: derive format from wl_visual */
      surface->color_format = PIPE_FORMAT_B8G8R8A8_UNORM;

   surface->attachment_mask = (1 << NATIVE_ATTACHMENT_FRONT_LEFT);

   surface->rsurf = resource_surface_create(display->base.screen,
                                            surface->color_format, bind);

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
   uint bind = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW |
      PIPE_BIND_DISPLAY_TARGET | PIPE_BIND_SCANOUT;

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
                                            surface->color_format, bind);

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

static struct native_display *
native_create_display(void *dpy, boolean use_sw)
{
   struct wayland_display *display = NULL;
   boolean own_dpy = FALSE;

   use_sw = use_sw || debug_get_bool_option("EGL_SOFTWARE", FALSE);

   if (dpy == NULL) {
      dpy = wl_display_connect(NULL);
      if (dpy == NULL)
         return NULL;
      own_dpy = TRUE;
   }

   if (use_sw) {
      _eglLog(_EGL_INFO, "use software fallback");
      display = wayland_create_shm_display((struct wl_display *) dpy,
                                           wayland_event_handler);
   } else {
      display = wayland_create_drm_display((struct wl_display *) dpy,
                                           wayland_event_handler);
   }

   if (!display)
      return NULL;

   display->base.get_param = wayland_display_get_param;
   display->base.get_configs = wayland_display_get_configs;
   display->base.is_pixmap_supported = wayland_display_is_pixmap_supported;
   display->base.create_window_surface = wayland_create_window_surface;
   display->base.create_pixmap_surface = wayland_create_pixmap_surface;

   display->own_dpy = own_dpy;

   return &display->base;
}

static const struct native_platform wayland_platform = {
   "wayland", /* name */
   native_create_display
};

const struct native_platform *
native_get_wayland_platform(const struct native_event_handler *event_handler)
{
   wayland_event_handler = event_handler;
   return &wayland_platform;
}

/* vim: set sw=3 ts=8 sts=3 expandtab: */
