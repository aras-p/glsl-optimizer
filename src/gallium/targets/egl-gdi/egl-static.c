/*
 * Mesa 3-D graphics library
 * Version:  7.9
 *
 * Copyright (C) 2010 LunarG Inc.
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
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include "common/egl_g3d_loader.h"
#include "state_tracker/st_gl_api.h"
#include "vg_api.h"
#include "target-helpers/inline_sw_helper.h"
#include "target-helpers/inline_debug_helper.h"
#include "egldriver.h"

static struct st_api *stapis[ST_API_COUNT];

static uint
get_api_mask(void)
{
   uint api_mask = 0x0;

#if FEATURE_GL
   api_mask |= 1 << ST_API_OPENGL;
#endif
#if FEATURE_ES1
   api_mask |= 1 << ST_API_OPENGL_ES1;
#endif
#if FEATURE_ES2
   api_mask |= 1 << ST_API_OPENGL_ES2;
#endif
#if FEATURE_VG
   api_mask |= 1 << ST_API_OPENVG;
#endif

   return api_mask;
}

static struct st_api *
get_st_api(enum st_api_type api)
{
   struct st_api *stapi;

   stapi = stapis[api];
   if (stapi)
      return stapi;

   switch (api) {
#if FEATURE_GL
      case ST_API_OPENGL:
         stapi = st_gl_api_create();
         break;
#endif
#if FEATURE_ES1
      case ST_API_OPENGL_ES1:
         stapi = st_gl_api_create_es1();
         break;
#endif
#if FEATURE_ES2
      case ST_API_OPENGL_ES2:
         stapi = st_gl_api_create_es2();
         break;
#endif
#if FEATURE_VG
      case ST_API_OPENVG:
         stapi = (struct st_api *) vg_api_get();
         break;
#endif
      default:
         break;
   }

   stapis[api] = stapi;

   return stapi;
}

static struct st_api *
guess_gl_api(void)
{
   struct st_api *stapi = NULL;

#if FEATURE_GL
   stapi = get_st_api(ST_API_OPENGL);
   if (stapi)
      return stapi;
#endif
#if FEATURE_ES1
   stapi = get_st_api(ST_API_OPENGL_ES1);
   if (stapi)
      return stapi;
#endif
#if FEATURE_ES2
   stapi = get_st_api(ST_API_OPENGL_ES2);
   if (stapi)
      return stapi;
#endif

   return stapi;
}

static struct pipe_screen *
create_drm_screen(const char *name, int fd)
{
   return NULL;
}

static struct pipe_screen *
create_sw_screen(struct sw_winsys *ws)
{
   struct pipe_screen *screen;

   screen = sw_screen_create(ws);
   if (screen)
      screen = debug_screen_wrap(screen);

   return screen;
}

static void
init_loader(struct egl_g3d_loader *loader)
{
   if (loader->api_mask)
      return;

   loader->api_mask = get_api_mask();
   loader->get_st_api = get_st_api;
   loader->guess_gl_api = guess_gl_api;
   loader->create_drm_screen = create_drm_screen;
   loader->create_sw_screen = create_sw_screen;
}

static void
egl_g3d_unload(_EGLDriver *drv)
{
   int i;

   egl_g3d_destroy_driver(drv);

   for (i = 0; i < ST_API_COUNT; i++) {
      if (stapis[i]) {
         stapis[i]->destroy(stapis[i]);
         stapis[i] = NULL;
      }
   }
}

static struct egl_g3d_loader loader;

_EGLDriver *
_eglMain(const char *args)
{
   _EGLDriver *drv;

   init_loader(&loader);
   drv = egl_g3d_create_driver(&loader);
   if (drv) {
      drv->Name = "Gallium";
      drv->Unload = egl_g3d_unload;
   }

   return drv;
}
