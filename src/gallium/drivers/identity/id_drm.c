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
#include "id_drm.h"
#include "id_screen.h"
#include "id_public.h"
#include "id_screen.h"
#include "id_objects.h"

struct identity_drm_api
{
   struct drm_api base;

   struct drm_api *api;
};

static INLINE struct identity_drm_api *
identity_drm_api(struct drm_api *_api)
{
   return (struct identity_drm_api *)_api;
}

static struct pipe_screen *
identity_drm_create_screen(struct drm_api *_api, int fd,
                           struct drm_create_screen_arg *arg)
{
   struct identity_drm_api *id_api = identity_drm_api(_api);
   struct drm_api *api = id_api->api;
   struct pipe_screen *screen;

   if (arg && arg->mode != DRM_CREATE_NORMAL)
      return NULL;

   screen = api->create_screen(api, fd, arg);

   return identity_screen_create(screen);
}

static void
identity_drm_destroy(struct drm_api *_api)
{
   struct identity_drm_api *id_api = identity_drm_api(_api);
   struct drm_api *api = id_api->api;
   api->destroy(api);

   free(id_api);
}

struct drm_api *
identity_drm_create(struct drm_api *api)
{
   struct identity_drm_api *id_api;

   if (!api)
      goto error;

   id_api = CALLOC_STRUCT(identity_drm_api);

   if (!id_api)
      goto error;

   id_api->base.name = api->name;
   id_api->base.driver_name = api->driver_name;
   id_api->base.create_screen = identity_drm_create_screen;
   id_api->base.destroy = identity_drm_destroy;
   id_api->api = api;

   return &id_api->base;

error:
   return api;
}
