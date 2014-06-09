/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2010-2011 LunarG Inc.
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
#include "egldriver.h"
#include "egllog.h"
#include "loader.h"

#include "egl_pipe.h"
#include "egl_st.h"
#include "target-helpers/inline_drm_helper.h"

static struct egl_g3d_loader egl_g3d_loader;

static struct st_module {
   boolean initialized;
   struct st_api *stapi;
} st_modules[ST_API_COUNT];

static struct st_api *
get_st_api(enum st_api_type api)
{
   struct st_module *stmod = &st_modules[api];

   if (!stmod->initialized) {
      stmod->stapi = egl_st_create_api(api);
      stmod->initialized = TRUE;
   }

   return stmod->stapi;
}


static struct pipe_screen *
create_drm_screen(const char *constname, int fd)
{
   return dd_create_screen(fd);
}

static struct pipe_screen *
create_sw_screen(struct sw_winsys *ws)
{
   return egl_pipe_create_swrast_screen(ws);
}

static const struct egl_g3d_loader *
loader_init(void)
{
   egl_g3d_loader.get_st_api = get_st_api;
   egl_g3d_loader.create_drm_screen = create_drm_screen;
   egl_g3d_loader.create_sw_screen = create_sw_screen;

   loader_set_logger(_eglLog);

   return &egl_g3d_loader;
}

static void
loader_fini(void)
{
   int i;

   for (i = 0; i < ST_API_COUNT; i++) {
      struct st_module *stmod = &st_modules[i];

      if (stmod->stapi) {
         egl_st_destroy_api(stmod->stapi);
         stmod->stapi = NULL;
      }
      stmod->initialized = FALSE;
   }
}

static void
egl_g3d_unload(_EGLDriver *drv)
{
   egl_g3d_destroy_driver(drv);
   loader_fini();
}

_EGLDriver *
_EGL_MAIN(const char *args)
{
   const struct egl_g3d_loader *loader;
   _EGLDriver *drv;

   loader = loader_init();
   drv = egl_g3d_create_driver(loader);
   if (!drv) {
      loader_fini();
      return NULL;
   }

   drv->Name = "Gallium";
   drv->Unload = egl_g3d_unload;

   return drv;
}
