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

#include "util/u_debug.h"
#include "util/u_string.h"
#include "util/u_memory.h"
#include "util/u_dl.h"
#include "egldriver.h"
#include "egllog.h"

#include "state_tracker/st_api.h"
#include "state_tracker/drm_driver.h"
#include "common/egl_g3d_loader.h"

#include "egl.h"

struct egl_g3d_loader egl_g3d_loader;

static struct st_module {
   boolean initialized;
   char *name;
   struct util_dl_library *lib;
   struct st_api *stapi;
} st_modules[ST_API_COUNT];

static struct pipe_module {
   boolean initialized;
   char *name;
   struct util_dl_library *lib;
   const struct drm_driver_descriptor *drmdd;
   struct pipe_screen *(*swrast_create_screen)(struct sw_winsys *);
} pipe_modules[16];

static char *
loader_strdup(const char *s)
{
   size_t len = (s) ? strlen(s) : 0;
   char *t = MALLOC(len + 1);
   if (t) {
      memcpy(t, s, len);
      t[len] = '\0';
   }
   return t;
}

static EGLBoolean
dlopen_st_module_cb(const char *dir, size_t len, void *callback_data)
{
   struct st_module *stmod =
      (struct st_module *) callback_data;
   char path[1024];
   int ret;

   if (len) {
      ret = util_snprintf(path, sizeof(path),
            "%.*s/" ST_PREFIX "%s" UTIL_DL_EXT, len, dir, stmod->name);
   }
   else {
      ret = util_snprintf(path, sizeof(path),
            ST_PREFIX "%s" UTIL_DL_EXT, stmod->name);
   }

   if (ret > 0 && ret < sizeof(path)) {
      stmod->lib = util_dl_open(path);
      if (stmod->lib)
         _eglLog(_EGL_DEBUG, "loaded %s", path);
   }

   return !(stmod->lib);
}

static boolean
load_st_module(struct st_module *stmod,
                       const char *name, const char *procname)
{
   struct st_api *(*create_api)(void);

   if (name) {
      _eglLog(_EGL_DEBUG, "searching for st module %s", name);
      stmod->name = loader_strdup(name);
   }
   else {
      stmod->name = NULL;
   }

   if (stmod->name)
      _eglSearchPathForEach(dlopen_st_module_cb, (void *) stmod);
   else
      stmod->lib = util_dl_open(NULL);

   if (stmod->lib) {
      create_api = (struct st_api *(*)(void))
         util_dl_get_proc_address(stmod->lib, procname);
      if (create_api)
         stmod->stapi = create_api();

      if (!stmod->stapi) {
         util_dl_close(stmod->lib);
         stmod->lib = NULL;
      }
   }

   if (!stmod->stapi) {
      FREE(stmod->name);
      stmod->name = NULL;
   }

   return (stmod->stapi != NULL);
}

static EGLBoolean
dlopen_pipe_module_cb(const char *dir, size_t len, void *callback_data)
{
   struct pipe_module *pmod = (struct pipe_module *) callback_data;
   char path[1024];
   int ret;

   if (len) {
      ret = util_snprintf(path, sizeof(path),
            "%.*s/" PIPE_PREFIX "%s" UTIL_DL_EXT, len, dir, pmod->name);
   }
   else {
      ret = util_snprintf(path, sizeof(path),
            PIPE_PREFIX "%s" UTIL_DL_EXT, pmod->name);
   }
   if (ret > 0 && ret < sizeof(path)) {
      pmod->lib = util_dl_open(path);
      if (pmod->lib)
         _eglLog(_EGL_DEBUG, "loaded %s", path);
   }

   return !(pmod->lib);
}

static boolean
load_pipe_module(struct pipe_module *pmod, const char *name)
{
   pmod->name = loader_strdup(name);
   if (!pmod->name)
      return FALSE;

   _eglLog(_EGL_DEBUG, "searching for pipe module %s", pmod->name);
   _eglSearchPathForEach(dlopen_pipe_module_cb, (void *) pmod);
   if (pmod->lib) {
      pmod->drmdd = (const struct drm_driver_descriptor *)
         util_dl_get_proc_address(pmod->lib, "driver_descriptor");

      /* sanity check on the name */
      if (pmod->drmdd && strcmp(pmod->drmdd->name, pmod->name) != 0)
         pmod->drmdd = NULL;

      /* swrast */
      if (pmod->drmdd && !pmod->drmdd->driver_name) {
         pmod->swrast_create_screen =
            (struct pipe_screen *(*)(struct sw_winsys *))
            util_dl_get_proc_address(pmod->lib, "swrast_create_screen");
         if (!pmod->swrast_create_screen)
            pmod->drmdd = NULL;
      }

      if (!pmod->drmdd) {
         util_dl_close(pmod->lib);
         pmod->lib = NULL;
      }
   }

   return (pmod->drmdd != NULL);
}

static struct st_api *
get_st_api_full(enum st_api_type api, enum st_profile_type profile)
{
   struct st_module *stmod = &st_modules[api];
   const char *names[8], *symbol;
   int i, count = 0;

   if (stmod->initialized)
      return stmod->stapi;

   switch (api) {
   case ST_API_OPENGL:
      symbol = ST_CREATE_OPENGL_SYMBOL;
      switch (profile) {
      case ST_PROFILE_OPENGL_ES1:
         names[count++] = "GLESv1_CM";
         names[count++] = "GL";
         break;
      case ST_PROFILE_OPENGL_ES2:
         names[count++] = "GLESv2";
         names[count++] = "GL";
         break;
      default:
         names[count++] = "GL";
         break;
      }
      break;
   case ST_API_OPENVG:
      symbol = ST_CREATE_OPENVG_SYMBOL;
      names[count++] = "OpenVG";
      break;
   default:
      symbol = NULL;
      assert(!"Unknown API Type\n");
      break;
   }

   /* NULL means the process itself */
   names[count++] = NULL;

   for (i = 0; i < count; i++) {
      if (load_st_module(stmod, names[i], symbol))
         break;
   }

   /* try again with libGL.so loaded */
   if (!stmod->stapi && api == ST_API_OPENGL) {
      struct util_dl_library *glapi = util_dl_open("libGL" UTIL_DL_EXT);

      if (glapi) {
         _eglLog(_EGL_DEBUG, "retry with libGL" UTIL_DL_EXT " loaded");
         /* skip the last name (which is NULL) */
         for (i = 0; i < count - 1; i++) {
            if (load_st_module(stmod, names[i], symbol))
               break;
         }
         util_dl_close(glapi);
      }
   }

   if (!stmod->stapi) {
      EGLint level = (egl_g3d_loader.profile_masks[api]) ?
         _EGL_WARNING : _EGL_DEBUG;
      _eglLog(level, "unable to load " ST_PREFIX "%s" UTIL_DL_EXT, names[0]);
   }

   stmod->initialized = TRUE;

   return stmod->stapi;
}

static struct st_api *
get_st_api(enum st_api_type api)
{
   enum st_profile_type profile = ST_PROFILE_DEFAULT;

   /* determine the profile from the linked libraries */
   if (api == ST_API_OPENGL) {
      struct util_dl_library *self;

      self = util_dl_open(NULL);
      if (self) {
         if (util_dl_get_proc_address(self, "glColor4x"))
            profile = ST_PROFILE_OPENGL_ES1;
         else if (util_dl_get_proc_address(self, "glShaderBinary"))
            profile = ST_PROFILE_OPENGL_ES2;
         util_dl_close(self);
      }
   }

   return get_st_api_full(api, profile);
}

static struct st_api *
guess_gl_api(enum st_profile_type profile)
{
   return get_st_api_full(ST_API_OPENGL, profile);
}

static struct pipe_module *
get_pipe_module(const char *name)
{
   struct pipe_module *pmod = NULL;
   int i;

   if (!name)
      return NULL;

   for (i = 0; i < Elements(pipe_modules); i++) {
      if (!pipe_modules[i].initialized ||
          strcmp(pipe_modules[i].name, name) == 0) {
         pmod = &pipe_modules[i];
         break;
      }
   }
   if (!pmod)
      return NULL;

   if (!pmod->initialized) {
      load_pipe_module(pmod, name);
      pmod->initialized = TRUE;
   }

   return pmod;
}

static struct pipe_screen *
create_drm_screen(const char *name, int fd)
{
   struct pipe_module *pmod = get_pipe_module(name);
   return (pmod && pmod->drmdd && pmod->drmdd->create_screen) ?
      pmod->drmdd->create_screen(fd) : NULL;
}

static struct pipe_screen *
create_sw_screen(struct sw_winsys *ws)
{
   struct pipe_module *pmod = get_pipe_module("swrast");
   return (pmod && pmod->swrast_create_screen) ?
      pmod->swrast_create_screen(ws) : NULL;
}

static const struct egl_g3d_loader *
loader_init(void)
{
   /* TODO detect at runtime? */
#if FEATURE_GL
   egl_g3d_loader.profile_masks[ST_API_OPENGL] |= ST_PROFILE_DEFAULT_MASK;
#endif
#if FEATURE_ES1
   egl_g3d_loader.profile_masks[ST_API_OPENGL] |= ST_PROFILE_OPENGL_ES1_MASK;
#endif
#if FEATURE_ES2
   egl_g3d_loader.profile_masks[ST_API_OPENGL] |= ST_PROFILE_OPENGL_ES2_MASK;
#endif
#if FEATURE_VG
   egl_g3d_loader.profile_masks[ST_API_OPENVG] |= ST_PROFILE_DEFAULT_MASK;
#endif

   egl_g3d_loader.get_st_api = get_st_api;
   egl_g3d_loader.guess_gl_api = guess_gl_api;
   egl_g3d_loader.create_drm_screen = create_drm_screen;
   egl_g3d_loader.create_sw_screen = create_sw_screen;

   return &egl_g3d_loader;
}

static void
loader_fini(void)
{
   int i;

   for (i = 0; i < ST_API_COUNT; i++) {
      struct st_module *stmod = &st_modules[i];

      if (stmod->stapi) {
         stmod->stapi->destroy(stmod->stapi);
         stmod->stapi = NULL;
      }
      if (stmod->lib) {
         util_dl_close(stmod->lib);
         stmod->lib = NULL;
      }
      if (stmod->name) {
         FREE(stmod->name);
         stmod->name = NULL;
      }
      stmod->initialized = FALSE;
   }
   for (i = 0; i < Elements(pipe_modules); i++) {
      struct pipe_module *pmod = &pipe_modules[i];

      if (!pmod->initialized)
         break;

      pmod->drmdd = NULL;
      pmod->swrast_create_screen = NULL;
      if (pmod->lib) {
         util_dl_close(pmod->lib);
         pmod->lib = NULL;
      }
      if (pmod->name) {
         FREE(pmod->name);
         pmod->name = NULL;
      }
      pmod->initialized = FALSE;
   }
}

static void
egl_g3d_unload(_EGLDriver *drv)
{
   egl_g3d_destroy_driver(drv);
   loader_fini();
}

_EGLDriver *
_eglMain(const char *args)
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
