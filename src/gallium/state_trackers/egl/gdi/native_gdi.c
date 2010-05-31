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

#include <windows.h>

#include "pipe/p_compiler.h"
#include "util/u_memory.h"
#include "util/u_format.h"
#include "util/u_inlines.h"
#include "target-helpers/wrap_screen.h"
#include "llvmpipe/lp_public.h"
#include "softpipe/sp_public.h"
#include "gdi/gdi_sw_winsys.h"

#include "common/native.h"

struct gdi_display {
   struct native_display base;

   HDC hDC;
   struct native_event_handler *event_handler;

   struct native_config *configs;
   int num_configs;
};

struct gdi_surface {
   struct native_surface base;

   HWND hWnd;
   enum pipe_format color_format;

   struct gdi_display *gdpy;

   unsigned int server_stamp;
   unsigned int client_stamp;
   int width, height;
   uint valid_mask;

   struct pipe_resource *resources[NUM_NATIVE_ATTACHMENTS];
   struct pipe_surface *present_surface;
};

static INLINE struct gdi_display *
gdi_display(const struct native_display *ndpy)
{
   return (struct gdi_display *) ndpy;
}

static INLINE struct gdi_surface *
gdi_surface(const struct native_surface *nsurf)
{
   return (struct gdi_surface *) nsurf;
}

static boolean
gdi_surface_alloc_buffer(struct native_surface *nsurf,
                         enum native_attachment which)
{
   struct gdi_surface *gsurf = gdi_surface(nsurf);
   struct pipe_screen *screen = gsurf->gdpy->base.screen;
   struct pipe_resource templ;

   pipe_resource_reference(&gsurf->resources[which], NULL);

   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D;
   templ.format = gsurf->color_format;
   templ.width0 = gsurf->width;
   templ.height0 = gsurf->height;
   templ.depth0 = 1;
   templ.bind = PIPE_BIND_RENDER_TARGET |
                PIPE_BIND_SCANOUT |
                PIPE_BIND_DISPLAY_TARGET;

   gsurf->resources[which] = screen->resource_create(screen, &templ);

   return (gsurf->resources[which] != NULL);
}

/**
 * Update the geometry of the surface.  Return TRUE if the geometry has changed
 * since last call.
 */
static boolean
gdi_surface_update_geometry(struct native_surface *nsurf)
{
   struct gdi_surface *gsurf = gdi_surface(nsurf);
   RECT rect;
   unsigned int w, h;
   boolean updated = FALSE;

   GetClientRect(gsurf->hWnd, &rect);
   w = rect.right - rect.left;
   h = rect.bottom - rect.top;

   if (gsurf->width != w || gsurf->height != h) {
      gsurf->width = w;
      gsurf->height = h;

      gsurf->server_stamp++;
      updated = TRUE;
   }

   return updated;
}

/**
 * Update the buffers of the surface.  It is a slow function due to the
 * round-trip to the server.
 */
static boolean
gdi_surface_update_buffers(struct native_surface *nsurf, uint buffer_mask)
{
   struct gdi_surface *gsurf = gdi_surface(nsurf);
   boolean updated;
   uint new_valid;
   int att;

   updated = gdi_surface_update_geometry(&gsurf->base);
   if (updated) {
      /* all buffers become invalid */
      gsurf->valid_mask = 0x0;
   }
   else {
      buffer_mask &= ~gsurf->valid_mask;
      /* all requested buffers are valid */
      if (!buffer_mask) {
         gsurf->client_stamp = gsurf->server_stamp;
         return TRUE;
      }
   }

   new_valid = 0x0;
   for (att = 0; att < NUM_NATIVE_ATTACHMENTS; att++) {
      if (native_attachment_mask_test(buffer_mask, att)) {
         /* reallocate the texture */
         if (!gdi_surface_alloc_buffer(&gsurf->base, att))
            break;

         new_valid |= (1 << att);
         if (buffer_mask == new_valid)
            break;
      }
   }

   gsurf->valid_mask |= new_valid;
   gsurf->client_stamp = gsurf->server_stamp;

   return (new_valid == buffer_mask);
}

static boolean
gdi_surface_present(struct native_surface *nsurf,
                    enum native_attachment which)
{
   struct gdi_surface *gsurf = gdi_surface(nsurf);
   struct pipe_screen *screen = gsurf->gdpy->base.screen;
   struct pipe_resource *pres = gsurf->resources[which];
   struct pipe_surface *psurf;
   HDC hDC;

   if (!pres)
      return TRUE;

   psurf = gsurf->present_surface;
   if (!psurf || psurf->texture != pres) {
      pipe_surface_reference(&gsurf->present_surface, NULL);

      psurf = screen->get_tex_surface(screen, pres,
            0, 0, 0, PIPE_BIND_DISPLAY_TARGET);
      if (!psurf)
         return FALSE;

      gsurf->present_surface = psurf;
   }

   hDC = GetDC(gsurf->hWnd);
   screen->flush_frontbuffer(screen, psurf, (void *) hDC);
   ReleaseDC(gsurf->hWnd, hDC);

   return TRUE;
}

static void
gdi_surface_notify_invalid(struct native_surface *nsurf)
{
   struct gdi_surface *gsurf = gdi_surface(nsurf);
   struct gdi_display *gdpy = gsurf->gdpy;

   gdpy->event_handler->invalid_surface(&gdpy->base,
         &gsurf->base, gsurf->server_stamp);
}

static boolean
gdi_surface_flush_frontbuffer(struct native_surface *nsurf)
{
   struct gdi_surface *gsurf = gdi_surface(nsurf);
   boolean ret;

   ret = gdi_surface_present(&gsurf->base, NATIVE_ATTACHMENT_FRONT_LEFT);
   /* force buffers to be updated in next validation call */
   gsurf->server_stamp++;
   gdi_surface_notify_invalid(&gsurf->base);

   return ret;
}

static boolean
gdi_surface_swap_buffers(struct native_surface *nsurf)
{
   struct gdi_surface *gsurf = gdi_surface(nsurf);
   struct pipe_resource **front, **back, *tmp;
   boolean ret;

   /* display the back buffer first */
   ret = gdi_surface_present(&gsurf->base, NATIVE_ATTACHMENT_BACK_LEFT);
   /* force buffers to be updated in next validation call */
   gsurf->server_stamp++;
   gdi_surface_notify_invalid(&gsurf->base);

   front = &gsurf->resources[NATIVE_ATTACHMENT_FRONT_LEFT];
   back = &gsurf->resources[NATIVE_ATTACHMENT_BACK_LEFT];

   /* skip swapping unless there is a front buffer */
   if (*front) {
      tmp = *front;
      *front = *back;
      *back = tmp;
   }

   return ret;
}

static boolean
gdi_surface_validate(struct native_surface *nsurf, uint attachment_mask,
                        unsigned int *seq_num, struct pipe_resource **textures,
                        int *width, int *height)
{
   struct gdi_surface *gsurf = gdi_surface(nsurf);

   if (gsurf->client_stamp != gsurf->server_stamp ||
       (gsurf->valid_mask & attachment_mask) != attachment_mask) {
      if (!gdi_surface_update_buffers(&gsurf->base, attachment_mask))
         return FALSE;
   }

   if (seq_num)
      *seq_num = gsurf->client_stamp;

   if (textures) {
      int att;
      for (att = 0; att < NUM_NATIVE_ATTACHMENTS; att++) {
         if (native_attachment_mask_test(attachment_mask, att)) {
            textures[att] = NULL;
            pipe_resource_reference(&textures[att], gsurf->resources[att]);
         }
      }
   }

   if (width)
      *width = gsurf->width;
   if (height)
      *height = gsurf->height;

   return TRUE;
}

static void
gdi_surface_wait(struct native_surface *nsurf)
{
   /* no-op */
}

static void
gdi_surface_destroy(struct native_surface *nsurf)
{
   struct gdi_surface *gsurf = gdi_surface(nsurf);
   int i;

   pipe_surface_reference(&gsurf->present_surface, NULL);

   for (i = 0; i < NUM_NATIVE_ATTACHMENTS; i++)
      pipe_resource_reference(&gsurf->resources[i], NULL);

   FREE(gsurf);
}

static struct native_surface *
gdi_display_create_window_surface(struct native_display *ndpy,
                                  EGLNativeWindowType win,
                                  const struct native_config *nconf)
{
   struct gdi_display *gdpy = gdi_display(ndpy);
   struct gdi_surface *gsurf;

   gsurf = CALLOC_STRUCT(gdi_surface);
   if (!gsurf)
      return NULL;

   gsurf->gdpy = gdpy;
   gsurf->color_format = nconf->color_format;
   gsurf->hWnd = (HWND) win;

   /* initialize the geometry */
   gdi_surface_update_buffers(&gsurf->base, 0x0);

   gsurf->base.destroy = gdi_surface_destroy;
   gsurf->base.swap_buffers = gdi_surface_swap_buffers;
   gsurf->base.flush_frontbuffer = gdi_surface_flush_frontbuffer;
   gsurf->base.validate = gdi_surface_validate;
   gsurf->base.wait = gdi_surface_wait;

   return &gsurf->base;
}

static int
fill_color_formats(struct native_display *ndpy, enum pipe_format formats[8])
{
   struct pipe_screen *screen = ndpy->screen;
   int i, count = 0;

   enum pipe_format candidates[] = {
      /* 32-bit */
      PIPE_FORMAT_B8G8R8A8_UNORM,
      PIPE_FORMAT_A8R8G8B8_UNORM,
      /* 24-bit */
      PIPE_FORMAT_B8G8R8X8_UNORM,
      PIPE_FORMAT_X8R8G8B8_UNORM,
      /* 16-bit */
      PIPE_FORMAT_B5G6R5_UNORM
   };

   assert(Elements(candidates) <= 8);

   for (i = 0; i < Elements(candidates); i++) {
      if (screen->is_format_supported(screen, candidates[i],
               PIPE_TEXTURE_2D, 0, PIPE_BIND_RENDER_TARGET, 0))
         formats[count++] = candidates[i];
   }

   return count;
}

static const struct native_config **
gdi_display_get_configs(struct native_display *ndpy, int *num_configs)
{
   struct gdi_display *gdpy = gdi_display(ndpy);
   const struct native_config **configs;
   int i;

   /* first time */
   if (!gdpy->configs) {
      enum pipe_format formats[8];
      int i, count;

      count = fill_color_formats(&gdpy->base, formats);

      gdpy->configs = CALLOC(count, sizeof(*gdpy->configs));
      if (!gdpy->configs)
         return NULL;

      for (i = 0; i < count; i++) {
         struct native_config *nconf = &gdpy->configs[i];

         nconf->buffer_mask =
            (1 << NATIVE_ATTACHMENT_FRONT_LEFT) |
            (1 << NATIVE_ATTACHMENT_BACK_LEFT);
         nconf->color_format = formats[i];

         nconf->window_bit = TRUE;
         nconf->slow_config = TRUE;
      }

      gdpy->num_configs = count;
   }

   configs = MALLOC(gdpy->num_configs * sizeof(*configs));
   if (configs) {
      for (i = 0; i < gdpy->num_configs; i++)
         configs[i] = (const struct native_config *) &gdpy->configs[i];
      if (num_configs)
         *num_configs = gdpy->num_configs;
   }
   return configs;
}

static int
gdi_display_get_param(struct native_display *ndpy,
                         enum native_param_type param)
{
   int val;

   switch (param) {
   case NATIVE_PARAM_USE_NATIVE_BUFFER:
      /* private buffers are allocated */
      val = FALSE;
      break;
   default:
      val = 0;
      break;
   }

   return val;
}

static void
gdi_display_destroy(struct native_display *ndpy)
{
   struct gdi_display *gdpy = gdi_display(ndpy);

   if (gdpy->configs)
      FREE(gdpy->configs);

   gdpy->base.screen->destroy(gdpy->base.screen);

   FREE(gdpy);
}

static struct native_display *
gdi_create_display(HDC hDC, struct pipe_screen *screen,
                   struct native_event_handler *event_handler)
{
   struct gdi_display *gdpy;

   gdpy = CALLOC_STRUCT(gdi_display);
   if (!gdpy)
      return NULL;

   gdpy->hDC = hDC;
   gdpy->event_handler = event_handler;

   gdpy->base.screen = screen;

   gdpy->base.destroy = gdi_display_destroy;
   gdpy->base.get_param = gdi_display_get_param;

   gdpy->base.get_configs = gdi_display_get_configs;
   gdpy->base.create_window_surface = gdi_display_create_window_surface;

   return &gdpy->base;
}

static struct pipe_screen *
gdi_create_screen(void)
{
   struct sw_winsys *winsys;
   struct pipe_screen *screen = NULL;

   winsys = gdi_create_sw_winsys();
   if (!winsys)
      return NULL;

#if defined(GALLIUM_LLVMPIPE)
   if (!screen && !debug_get_bool_option("GALLIUM_NO_LLVM", FALSE))
      screen = llvmpipe_create_screen(winsys);
#endif
   if (!screen)
      screen = softpipe_create_screen(winsys);

   if (!screen) {
      if (winsys->destroy)
         winsys->destroy(winsys);
      return NULL;
   }

   return gallium_wrap_screen(screen);
}

struct native_probe *
native_create_probe(EGLNativeDisplayType dpy)
{
   return NULL;
}

enum native_probe_result
native_get_probe_result(struct native_probe *nprobe)
{
   return NATIVE_PROBE_UNKNOWN;
}

const char *
native_get_name(void)
{
   return "GDI";
}

struct native_display *
native_create_display(EGLNativeDisplayType dpy,
                      struct native_event_handler *event_handler)
{
   struct pipe_screen *screen;

   screen = gdi_create_screen();
   if (!screen)
      return NULL;

   return gdi_create_display((HDC) dpy, screen, event_handler);
}
