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
#include "glhd_drm.h"
#include "glhd_screen.h"
#include "glhd_public.h"

struct galahad_drm_api
{
   struct drm_api base;

   struct drm_api *api;
};

static INLINE struct galahad_drm_api *
galahad_drm_api(struct drm_api *_api)
{
   return (struct galahad_drm_api *)_api;
}

static struct pipe_screen *
galahad_drm_create_screen(struct drm_api *_api, int fd)
{
   struct galahad_drm_api *glhd_api = galahad_drm_api(_api);
   struct drm_api *api = glhd_api->api;
   struct pipe_screen *screen;

   screen = api->create_screen(api, fd);

   return galahad_screen_create(screen);
}

static void
galahad_drm_destroy(struct drm_api *_api)
{
   struct galahad_drm_api *glhd_api = galahad_drm_api(_api);
   struct drm_api *api = glhd_api->api;
   api->destroy(api);

   FREE(glhd_api);
}

struct drm_api *
galahad_drm_create(struct drm_api *api)
{
   struct galahad_drm_api *glhd_api;

   if (!api)
      goto error;

   if (!debug_get_option("GALAHAD", FALSE))
      goto error;

   glhd_api = CALLOC_STRUCT(galahad_drm_api);

   if (!glhd_api)
      goto error;

   glhd_api->base.name = api->name;
   glhd_api->base.driver_name = api->driver_name;
   glhd_api->base.create_screen = galahad_drm_create_screen;
   glhd_api->base.destroy = galahad_drm_destroy;
   glhd_api->api = api;

   return &glhd_api->base;

error:
   return api;
}
