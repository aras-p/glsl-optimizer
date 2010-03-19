/**********************************************************
 * Copyright 2010 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/


#include "util/u_memory.h"
#include "softpipe/sp_public.h"
#include "state_tracker/drm_api.h"
#include "wrapper_sw_winsys.h"
#include "sw_drm_api.h"


/*
 * Defines
 */


struct sw_drm_api
{
   struct drm_api base;
   struct drm_api *api;
};

static INLINE struct sw_drm_api *
sw_drm_api(struct drm_api *api)
{
   return (struct sw_drm_api *)api;
}


/*
 * Exported functions
 */


static struct pipe_screen *
sw_drm_create_screen(struct drm_api *_api, int drmFD,
                     struct drm_create_screen_arg *arg)
{
   struct sw_drm_api *swapi = sw_drm_api(_api);
   struct drm_api *api = swapi->api;
   struct sw_winsys *sww;
   struct pipe_screen *screen;

   screen = api->create_screen(api, drmFD, arg);

   sww = wrapper_sw_winsys_warp_pipe_screen(screen);

   return softpipe_create_screen(sww);
}

static void
sw_drm_destroy(struct drm_api *api)
{
   struct sw_drm_api *swapi = sw_drm_api(api);
   if (swapi->api->destroy)
      swapi->api->destroy(swapi->api);

   FREE(swapi);
}

struct drm_api *
sw_drm_api_create(struct drm_api *api)
{
   struct sw_drm_api *swapi = CALLOC_STRUCT(sw_drm_api);

   swapi->base.name = "sw";
   swapi->base.driver_name = api->driver_name;
   swapi->base.create_screen = sw_drm_create_screen;
   swapi->base.destroy = sw_drm_destroy;

   swapi->api = api;

   return &swapi->base;
}
