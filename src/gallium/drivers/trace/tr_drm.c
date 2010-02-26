/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "state_tracker/drm_api.h"

#include "util/u_memory.h"
#include "tr_drm.h"
#include "tr_screen.h"
#include "tr_context.h"
#include "tr_buffer.h"
#include "tr_texture.h"

struct trace_drm_api
{
   struct drm_api base;

   struct drm_api *api;
};

static INLINE struct trace_drm_api *
trace_drm_api(struct drm_api *_api)
{
   return (struct trace_drm_api *)_api;
}

static struct pipe_screen *
trace_drm_create_screen(struct drm_api *_api, int fd,
                        struct drm_create_screen_arg *arg)
{
   struct trace_drm_api *tr_api = trace_drm_api(_api);
   struct drm_api *api = tr_api->api;
   struct pipe_screen *screen;

   /* TODO trace call */

   if (arg && arg->mode != DRM_CREATE_NORMAL)
      return NULL;

   screen = api->create_screen(api, fd, arg);

   return trace_screen_create(screen);
}


static struct pipe_texture *
trace_drm_texture_from_shared_handle(struct drm_api *_api,
                                     struct pipe_screen *_screen,
                                     struct pipe_texture *templ,
                                     const char *name,
                                     unsigned stride,
                                     unsigned handle)
{
   struct trace_screen *tr_screen = trace_screen(_screen);
   struct trace_drm_api *tr_api = trace_drm_api(_api);
   struct pipe_screen *screen = tr_screen->screen;
   struct drm_api *api = tr_api->api;
   struct pipe_texture *result;

   /* TODO trace call */

   result = api->texture_from_shared_handle(api, screen, templ, name, stride, handle);

   result = trace_texture_create(trace_screen(_screen), result);

   return result;
}

static boolean
trace_drm_shared_handle_from_texture(struct drm_api *_api,
                                     struct pipe_screen *_screen,
                                     struct pipe_texture *_texture,
                                     unsigned *stride,
                                     unsigned *handle)
{
   struct trace_screen *tr_screen = trace_screen(_screen);
   struct trace_texture *tr_texture = trace_texture(_texture);
   struct trace_drm_api *tr_api = trace_drm_api(_api);
   struct pipe_screen *screen = tr_screen->screen;
   struct pipe_texture *texture = tr_texture->texture;
   struct drm_api *api = tr_api->api;

   /* TODO trace call */

   return api->shared_handle_from_texture(api, screen, texture, stride, handle);
}

static boolean
trace_drm_local_handle_from_texture(struct drm_api *_api,
                                    struct pipe_screen *_screen,
                                    struct pipe_texture *_texture,
                                    unsigned *stride,
                                    unsigned *handle)
{
   struct trace_screen *tr_screen = trace_screen(_screen);
   struct trace_texture *tr_texture = trace_texture(_texture);
   struct trace_drm_api *tr_api = trace_drm_api(_api);
   struct pipe_screen *screen = tr_screen->screen;
   struct pipe_texture *texture = tr_texture->texture;
   struct drm_api *api = tr_api->api;

   /* TODO trace call */

   return api->local_handle_from_texture(api, screen, texture, stride, handle);
}

static void
trace_drm_destroy(struct drm_api *_api)
{
   struct trace_drm_api *tr_api = trace_drm_api(_api);
   struct drm_api *api = tr_api->api;

   if (api->destroy)
      api->destroy(api);

   free(tr_api);
}

struct drm_api *
trace_drm_create(struct drm_api *api)
{
   struct trace_drm_api *tr_api;

   if (!api)
      goto error;

   if (!trace_enabled())
      goto error;

   tr_api = CALLOC_STRUCT(trace_drm_api);

   if (!tr_api)
      goto error;

   tr_api->base.name = api->name;
   tr_api->base.driver_name = api->driver_name;
   tr_api->base.create_screen = trace_drm_create_screen;
   tr_api->base.texture_from_shared_handle = trace_drm_texture_from_shared_handle;
   tr_api->base.shared_handle_from_texture = trace_drm_shared_handle_from_texture;
   tr_api->base.local_handle_from_texture = trace_drm_local_handle_from_texture;
   tr_api->base.destroy = trace_drm_destroy;
   tr_api->api = api;

   return &tr_api->base;

error:
   return api;
}
