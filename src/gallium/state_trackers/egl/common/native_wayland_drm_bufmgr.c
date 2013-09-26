#include <stdint.h>
#include <string.h>

#include "native.h"
#include "util/u_inlines.h"
#include "state_tracker/drm_driver.h"

#ifdef HAVE_WAYLAND_BACKEND

#include <wayland-server.h>
#include <wayland-drm-server-protocol.h>

#include "native_wayland_drm_bufmgr.h"

#include "wayland-drm.h"

struct wayland_drm_bufmgr {
   struct native_display_wayland_bufmgr base;

   struct wl_drm *wl_server_drm;
   char *device_name;

   void *user_data;

   wayland_drm_bufmgr_authenticate_func authenticate;
};

static INLINE struct wayland_drm_bufmgr *
wayland_drm_bufmgr(const struct native_display_wayland_bufmgr *base)
{
   return (struct wayland_drm_bufmgr *) base;
}

static int
wayland_drm_bufmgr_authenticate(void *user_data, uint32_t magic)
{
   struct native_display *ndpy = user_data;
   struct wayland_drm_bufmgr *bufmgr;

   bufmgr = wayland_drm_bufmgr(ndpy->wayland_bufmgr);

   return bufmgr->authenticate(user_data, magic);
}

static void
wayland_drm_bufmgr_reference_buffer(void *user_data, uint32_t name, int fd,
                                    struct wl_drm_buffer *buffer)
{
   struct native_display *ndpy = user_data;
   struct pipe_resource templ;
   struct winsys_handle wsh;
   enum pipe_format pf;

   switch (buffer->format) {
   case WL_DRM_FORMAT_ARGB8888:
      pf = PIPE_FORMAT_B8G8R8A8_UNORM;
      break;
   case WL_DRM_FORMAT_XRGB8888:
      pf = PIPE_FORMAT_B8G8R8X8_UNORM;
      break;
   default:
      pf = PIPE_FORMAT_NONE;
      break;
   }

   if (pf == PIPE_FORMAT_NONE)
      return;

   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D;
   templ.format = pf;
   templ.bind = PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW;
   templ.width0 = buffer->width;
   templ.height0 = buffer->height;
   templ.depth0 = 1;
   templ.array_size = 1;

   memset(&wsh, 0, sizeof(wsh));
   wsh.handle = name;
   wsh.stride = buffer->stride[0];

   buffer->driver_buffer =
      ndpy->screen->resource_from_handle(ndpy->screen, &templ, &wsh);
}

static void
wayland_drm_bufmgr_unreference_buffer(void *user_data,
                                      struct wl_drm_buffer *buffer)
{
   struct pipe_resource *resource = buffer->driver_buffer;

   pipe_resource_reference(&resource, NULL);
}

static struct wayland_drm_callbacks wl_drm_callbacks = {
   wayland_drm_bufmgr_authenticate,
   wayland_drm_bufmgr_reference_buffer,
   wayland_drm_bufmgr_unreference_buffer
};

static boolean
wayland_drm_bufmgr_bind_display(struct native_display *ndpy,
                                struct wl_display *wl_dpy)
{
   struct wayland_drm_bufmgr *bufmgr;

   bufmgr = wayland_drm_bufmgr(ndpy->wayland_bufmgr);

   if (bufmgr->wl_server_drm)
      return FALSE;

   bufmgr->wl_server_drm = wayland_drm_init(wl_dpy, bufmgr->device_name,
         &wl_drm_callbacks, ndpy, 0);

   if (!bufmgr->wl_server_drm)
      return FALSE;

   return TRUE;
}

static boolean
wayland_drm_bufmgr_unbind_display(struct native_display *ndpy,
                                  struct wl_display *wl_dpy)
{
   struct wayland_drm_bufmgr *bufmgr;

   bufmgr = wayland_drm_bufmgr(ndpy->wayland_bufmgr);

   if (!bufmgr->wl_server_drm)
      return FALSE;

   wayland_drm_uninit(bufmgr->wl_server_drm);
   bufmgr->wl_server_drm = NULL;

   return TRUE;
}

static struct pipe_resource *
wayland_drm_bufmgr_wl_buffer_get_resource(struct native_display *ndpy,
                                          struct wl_resource *buffer_resource)
{
   struct wayland_drm_bufmgr *bufmgr;
   struct wl_drm_buffer *buffer;

   bufmgr = wayland_drm_bufmgr(ndpy->wayland_bufmgr);
   buffer = wayland_drm_buffer_get(bufmgr->wl_server_drm, buffer_resource);

   if (!buffer)
      return NULL;

   return wayland_drm_buffer_get_buffer(buffer);
}

static boolean
wayland_drm_bufmgr_query_buffer(struct native_display *ndpy,
                                struct wl_resource *buffer_resource,
                                int attribute, int *value)
{
   struct wayland_drm_bufmgr *bufmgr;
   struct wl_drm_buffer *buffer;
   struct pipe_resource *resource;

   bufmgr = wayland_drm_bufmgr(ndpy->wayland_bufmgr);
   buffer = wayland_drm_buffer_get(bufmgr->wl_server_drm, buffer_resource);
   if (!buffer)
      return FALSE;

   resource = buffer->driver_buffer;

   switch (attribute) {
   case EGL_TEXTURE_FORMAT:
      switch (resource->format) {
      case PIPE_FORMAT_B8G8R8A8_UNORM:
         *value = EGL_TEXTURE_RGBA;
         return TRUE;
      case PIPE_FORMAT_B8G8R8X8_UNORM:
         *value = EGL_TEXTURE_RGB;
         return TRUE;
      default:
         return FALSE;
      }
   case EGL_WIDTH:
      *value = buffer->width;
      return TRUE;
   case EGL_HEIGHT:
      *value = buffer->height;
      return TRUE;
   default:
      return FALSE;
   }
}


struct native_display_wayland_bufmgr *
wayland_drm_bufmgr_create(wayland_drm_bufmgr_authenticate_func authenticate,
                          void *user_data, char *device_name)
{
   struct wayland_drm_bufmgr *bufmgr;

   bufmgr = calloc(1, sizeof *bufmgr);
   if (!bufmgr)
      return NULL;

   bufmgr->user_data = user_data;
   bufmgr->authenticate = authenticate;
   bufmgr->device_name = strdup(device_name);

   bufmgr->base.bind_display = wayland_drm_bufmgr_bind_display;
   bufmgr->base.unbind_display = wayland_drm_bufmgr_unbind_display;
   bufmgr->base.buffer_get_resource = wayland_drm_bufmgr_wl_buffer_get_resource;
   bufmgr->base.query_buffer = wayland_drm_bufmgr_query_buffer;

   return &bufmgr->base;
}

void
wayland_drm_bufmgr_destroy(struct native_display_wayland_bufmgr *_bufmgr)
{
   struct wayland_drm_bufmgr *bufmgr = wayland_drm_bufmgr(_bufmgr);

   if (!bufmgr)
      return;

   free(bufmgr->device_name);
   free(bufmgr);
}

#endif
