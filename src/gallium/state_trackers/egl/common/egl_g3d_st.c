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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "util/u_dl.h"
#include "eglimage.h"
#include "eglmutex.h"

#include "egl_g3d.h"
#include "egl_g3d_st.h"

struct egl_g3d_st_manager {
   struct st_manager base;
   _EGLDisplay *display;
};

static INLINE struct egl_g3d_st_manager *
egl_g3d_st_manager(struct st_manager *smapi)
{
   return (struct egl_g3d_st_manager *) smapi;
}

struct st_api *
egl_g3d_create_st_api(enum st_api_type api)
{
   const char *stmod_name;
   struct util_dl_library *lib;
   const struct st_module *mod;

   switch (api) {
   case ST_API_OPENGL:
      stmod_name = ST_MODULE_OPENGL_SYMBOL;
      break;
   case ST_API_OPENGL_ES1:
      stmod_name = ST_MODULE_OPENGL_ES1_SYMBOL;
      break;
   case ST_API_OPENGL_ES2:
      stmod_name = ST_MODULE_OPENGL_ES2_SYMBOL;
      break;
   case ST_API_OPENVG:
      stmod_name = ST_MODULE_OPENVG_SYMBOL;
      break;
   default:
      stmod_name = NULL;
      break;
   }
   if (!stmod_name)
      return NULL;

   mod = NULL;
   lib = util_dl_open(NULL);
   if (lib) {
      mod = (const struct st_module *)
         util_dl_get_proc_address(lib, stmod_name);
      util_dl_close(lib);
   }
   if (!mod || mod->api != api)
      return NULL;

   return mod->create_api();
}

struct st_manager *
egl_g3d_create_st_manager(_EGLDisplay *dpy)
{
   struct egl_g3d_display *gdpy = egl_g3d_display(dpy);
   struct egl_g3d_st_manager *gsmapi;

   gsmapi = CALLOC_STRUCT(egl_g3d_st_manager);
   if (gsmapi) {
      gsmapi->display = dpy;

      gsmapi->base.screen = gdpy->native->screen;
   }

   return &gsmapi->base;;
}

void
egl_g3d_destroy_st_manager(struct st_manager *smapi)
{
   struct egl_g3d_st_manager *gsmapi = egl_g3d_st_manager(smapi);
   free(gsmapi);
}

static boolean
egl_g3d_st_framebuffer_flush_front(struct st_framebuffer_iface *stfbi,
                                   enum st_attachment_type statt)
{
   _EGLSurface *surf = (_EGLSurface *) stfbi->st_manager_private;
   struct egl_g3d_surface *gsurf = egl_g3d_surface(surf);

   return gsurf->native->flush_frontbuffer(gsurf->native);
}

static boolean 
egl_g3d_st_framebuffer_validate(struct st_framebuffer_iface *stfbi,
                                const enum st_attachment_type *statts,
                                unsigned count,
                                struct pipe_texture **out)
{
   _EGLSurface *surf = (_EGLSurface *) stfbi->st_manager_private;
   struct egl_g3d_surface *gsurf = egl_g3d_surface(surf);
   struct pipe_texture *textures[NUM_NATIVE_ATTACHMENTS];
   uint attachment_mask = 0;
   unsigned i;

   for (i = 0; i < count; i++) {
      int natt;

      switch (statts[i]) {
      case ST_ATTACHMENT_FRONT_LEFT:
         natt = NATIVE_ATTACHMENT_FRONT_LEFT;
         break;
      case ST_ATTACHMENT_BACK_LEFT:
         natt = NATIVE_ATTACHMENT_BACK_LEFT;
         break;
      case ST_ATTACHMENT_FRONT_RIGHT:
         natt = NATIVE_ATTACHMENT_FRONT_RIGHT;
         break;
      case ST_ATTACHMENT_BACK_RIGHT:
         natt = NATIVE_ATTACHMENT_BACK_RIGHT;
      default:
         natt = -1;
         break;
      }

      if (natt >= 0)
         attachment_mask |= 1 << natt;
   }

   if (!gsurf->native->validate(gsurf->native, attachment_mask,
         &gsurf->sequence_number, textures, &gsurf->base.Width,
         &gsurf->base.Height))
      return FALSE;

   for (i = 0; i < count; i++) {
      struct pipe_texture *tex;
      int natt;

      switch (statts[i]) {
      case ST_ATTACHMENT_FRONT_LEFT:
         natt = NATIVE_ATTACHMENT_FRONT_LEFT;
         break;
      case ST_ATTACHMENT_BACK_LEFT:
         natt = NATIVE_ATTACHMENT_BACK_LEFT;
         break;
      case ST_ATTACHMENT_FRONT_RIGHT:
         natt = NATIVE_ATTACHMENT_FRONT_RIGHT;
         break;
      case ST_ATTACHMENT_BACK_RIGHT:
         natt = NATIVE_ATTACHMENT_BACK_RIGHT;
         break;
      default:
         natt = -1;
         break;
      }

      if (natt >= 0) {
         tex = textures[natt];

         if (statts[i] == stfbi->visual->render_buffer)
            pipe_texture_reference(&gsurf->render_texture, tex);

         if (attachment_mask & (1 << natt)) {
            /* transfer the ownership to the caller */
            out[i] = tex;
            attachment_mask &= ~(1 << natt);
         }
         else {
            /* the attachment is listed more than once */
            pipe_texture_reference(&out[i], tex);
         }
      }
   }

   return TRUE;
}

struct st_framebuffer_iface *
egl_g3d_create_st_framebuffer(_EGLSurface *surf)
{
   struct egl_g3d_surface *gsurf = egl_g3d_surface(surf);
   struct st_framebuffer_iface *stfbi;

   stfbi = CALLOC_STRUCT(st_framebuffer_iface);
   if (!stfbi)
      return NULL;

   stfbi->visual = &gsurf->stvis;
   stfbi->flush_front = egl_g3d_st_framebuffer_flush_front;
   stfbi->validate = egl_g3d_st_framebuffer_validate;
   stfbi->st_manager_private = (void *) &gsurf->base;

   return stfbi;
}

void
egl_g3d_destroy_st_framebuffer(struct st_framebuffer_iface *stfbi)
{
   free(stfbi);
}
