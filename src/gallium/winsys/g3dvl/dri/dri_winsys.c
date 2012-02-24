/**************************************************************************
 *
 * Copyright 2009 Younes Manton.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <X11/Xlib-xcb.h>
#include <xcb/dri2.h>
#include <xf86drm.h>

#include "pipe/p_screen.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "state_tracker/drm_driver.h"

#include "util/u_memory.h"
#include "util/u_hash.h"
#include "util/u_hash_table.h"
#include "util/u_inlines.h"

#include "vl_winsys.h"

struct vl_dri_screen
{
   struct vl_screen base;
   xcb_connection_t *conn;
   xcb_drawable_t drawable;

   bool flushed;
   xcb_dri2_swap_buffers_cookie_t swap_cookie;
   xcb_dri2_wait_sbc_cookie_t wait_cookie;
   xcb_dri2_get_buffers_cookie_t buffers_cookie;
};

static const unsigned int attachments[1] = { XCB_DRI2_ATTACHMENT_BUFFER_BACK_LEFT };

static void
vl_dri2_flush_frontbuffer(struct pipe_screen *screen,
                          struct pipe_resource *resource,
                          unsigned level, unsigned layer,
                          void *context_private)
{
   struct vl_dri_screen *scrn = (struct vl_dri_screen*)context_private;

   assert(screen);
   assert(resource);
   assert(context_private);

   if (scrn->flushed)
      free(xcb_dri2_swap_buffers_reply(scrn->conn, scrn->swap_cookie, NULL));
   else
      scrn->flushed = true;

   scrn->swap_cookie = xcb_dri2_swap_buffers_unchecked(scrn->conn, scrn->drawable, 0, 0, 0, 0, 0, 0);
   scrn->wait_cookie = xcb_dri2_wait_sbc_unchecked(scrn->conn, scrn->drawable, 0, 0);
   scrn->buffers_cookie = xcb_dri2_get_buffers_unchecked(scrn->conn, scrn->drawable, 1, 1, attachments);
}

static void
vl_dri2_destroy_drawable(struct vl_dri_screen *scrn)
{
   xcb_void_cookie_t destroy_cookie;
   if (scrn->drawable) {
      destroy_cookie = xcb_dri2_destroy_drawable_checked(scrn->conn, scrn->drawable);
      /* ignore any error here, since the drawable can be destroyed long ago */
      free(xcb_request_check(scrn->conn, destroy_cookie));
   }
}

struct pipe_resource*
vl_screen_texture_from_drawable(struct vl_screen *vscreen, Drawable drawable)
{
   struct vl_dri_screen *scrn = (struct vl_dri_screen*)vscreen;

   struct winsys_handle dri2_front_handle;
   struct pipe_resource template, *tex;

   xcb_dri2_get_buffers_reply_t *reply;
   xcb_dri2_dri2_buffer_t *buffers;

   assert(scrn);

   if (scrn->flushed) {
      free(xcb_dri2_swap_buffers_reply(scrn->conn, scrn->swap_cookie, NULL));
      free(xcb_dri2_wait_sbc_reply(scrn->conn, scrn->wait_cookie, NULL));
   }

   if (scrn->drawable != drawable) {
      vl_dri2_destroy_drawable(scrn);
      xcb_dri2_create_drawable(scrn->conn, drawable);
      scrn->drawable = drawable;

      if (scrn->flushed) {
         free(xcb_dri2_get_buffers_reply(scrn->conn, scrn->buffers_cookie, NULL));
         scrn->flushed = false;
      }
   }

   if (!scrn->flushed)
      scrn->buffers_cookie = xcb_dri2_get_buffers_unchecked(scrn->conn, drawable, 1, 1, attachments);
   else
      scrn->flushed = false;

   reply = xcb_dri2_get_buffers_reply(scrn->conn, scrn->buffers_cookie, NULL);
   if (!reply)
      return NULL;

   buffers = xcb_dri2_get_buffers_buffers(reply);
   if (!buffers)  {
      free(reply);
      return NULL;
   }

   assert(reply->count == 1);

   memset(&dri2_front_handle, 0, sizeof(dri2_front_handle));
   dri2_front_handle.type = DRM_API_HANDLE_TYPE_SHARED;
   dri2_front_handle.handle = buffers[0].name;
   dri2_front_handle.stride = buffers[0].pitch;

   memset(&template, 0, sizeof(template));
   template.target = PIPE_TEXTURE_2D;
   template.format = PIPE_FORMAT_B8G8R8X8_UNORM;
   template.last_level = 0;
   template.width0 = reply->width;
   template.height0 = reply->height;
   template.depth0 = 1;
   template.array_size = 1;
   template.usage = PIPE_USAGE_STATIC;
   template.bind = PIPE_BIND_RENDER_TARGET;
   template.flags = 0;

   tex = scrn->base.pscreen->resource_from_handle(scrn->base.pscreen, &template, &dri2_front_handle);
   free(reply);

   return tex;
}

void*
vl_screen_get_private(struct vl_screen *vscreen)
{
   return vscreen;
}

struct vl_screen*
vl_screen_create(Display *display, int screen)
{
   struct vl_dri_screen *scrn;
   const xcb_query_extension_reply_t *extension;
   xcb_dri2_query_version_cookie_t dri2_query_cookie;
   xcb_dri2_query_version_reply_t *dri2_query = NULL;
   xcb_dri2_connect_cookie_t connect_cookie;
   xcb_dri2_connect_reply_t *connect = NULL;
   xcb_dri2_authenticate_cookie_t authenticate_cookie;
   xcb_dri2_authenticate_reply_t *authenticate = NULL;
   xcb_screen_iterator_t s;
   xcb_generic_error_t *error = NULL;
   char *device_name;
   int fd;

   drm_magic_t magic;

   assert(display);

   scrn = CALLOC_STRUCT(vl_dri_screen);
   if (!scrn)
      return NULL;

   scrn->conn = XGetXCBConnection(display);
   if (!scrn->conn)
      goto free_screen;

   xcb_prefetch_extension_data(scrn->conn, &xcb_dri2_id);

   extension = xcb_get_extension_data(scrn->conn, &xcb_dri2_id);
   if (!(extension && extension->present))
      goto free_screen;

   dri2_query_cookie = xcb_dri2_query_version (scrn->conn, XCB_DRI2_MAJOR_VERSION, XCB_DRI2_MINOR_VERSION);
   dri2_query = xcb_dri2_query_version_reply (scrn->conn, dri2_query_cookie, &error);
   if (dri2_query == NULL || error != NULL || dri2_query->minor_version < 2)
      goto free_screen;

   s = xcb_setup_roots_iterator(xcb_get_setup(scrn->conn));
   connect_cookie = xcb_dri2_connect_unchecked(scrn->conn, s.data->root, XCB_DRI2_DRIVER_TYPE_DRI);
   connect = xcb_dri2_connect_reply(scrn->conn, connect_cookie, NULL);
   if (connect == NULL || connect->driver_name_length + connect->device_name_length == 0)
      goto free_screen;

   device_name = xcb_dri2_connect_device_name(connect);
   device_name = strndup(device_name, xcb_dri2_connect_device_name_length(connect));
   fd = open(device_name, O_RDWR);
   free(device_name);

   if (fd < 0)
      goto free_screen;

   if (drmGetMagic(fd, &magic))
      goto free_screen;

   authenticate_cookie = xcb_dri2_authenticate_unchecked(scrn->conn, s.data->root, magic);
   authenticate = xcb_dri2_authenticate_reply(scrn->conn, authenticate_cookie, NULL);

   if (authenticate == NULL || !authenticate->authenticated)
      goto free_screen;

   scrn->base.pscreen = driver_descriptor.create_screen(fd);
   if (!scrn->base.pscreen)
      goto free_screen;

   scrn->base.pscreen->flush_frontbuffer = vl_dri2_flush_frontbuffer;

   free(dri2_query);
   free(connect);
   free(authenticate);

   return &scrn->base;

free_screen:
   FREE(scrn);

   free(dri2_query);
   free(connect);
   free(authenticate);
   free(error);

   return NULL;
}

void vl_screen_destroy(struct vl_screen *vscreen)
{
   struct vl_dri_screen *scrn = (struct vl_dri_screen*)vscreen;

   assert(vscreen);

   if (scrn->flushed) {
      free(xcb_dri2_swap_buffers_reply(scrn->conn, scrn->swap_cookie, NULL));
      free(xcb_dri2_wait_sbc_reply(scrn->conn, scrn->wait_cookie, NULL));
      free(xcb_dri2_get_buffers_reply(scrn->conn, scrn->buffers_cookie, NULL));
   }

   vl_dri2_destroy_drawable(scrn);
   scrn->base.pscreen->destroy(scrn->base.pscreen);
   FREE(scrn);
}
