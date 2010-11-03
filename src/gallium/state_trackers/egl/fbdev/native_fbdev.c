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

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/fb.h>

#include "pipe/p_screen.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "util/u_pointer.h"

#include "common/native.h"
#include "common/native_helper.h"
#include "fbdev/fbdev_sw_winsys.h"

struct fbdev_display {
   struct native_display base;

   int fd;
   struct native_event_handler *event_handler;

   struct fb_fix_screeninfo finfo;
   struct fb_var_screeninfo vinfo;

   struct native_config config;
   struct native_connector connector;
   struct native_mode mode;

   struct fbdev_surface *current_surface;
};

struct fbdev_surface {
   struct native_surface base;

   struct fbdev_display *fbdpy;
   struct resource_surface *rsurf;
   int width, height;

   unsigned int sequence_number;

   boolean is_current;
};

static INLINE struct fbdev_display *
fbdev_display(const struct native_display *ndpy)
{
   return (struct fbdev_display *) ndpy;
}

static INLINE struct fbdev_surface *
fbdev_surface(const struct native_surface *nsurf)
{
   return (struct fbdev_surface *) nsurf;
}

static boolean
fbdev_surface_validate(struct native_surface *nsurf, uint attachment_mask,
                     unsigned int *seq_num, struct pipe_resource **textures,
                     int *width, int *height)
{
   struct fbdev_surface *fbsurf = fbdev_surface(nsurf);

   if (!resource_surface_add_resources(fbsurf->rsurf, attachment_mask))
      return FALSE;
   if (textures)
      resource_surface_get_resources(fbsurf->rsurf, textures, attachment_mask);

   if (seq_num)
      *seq_num = fbsurf->sequence_number;
   if (width)
      *width = fbsurf->width;
   if (height)
      *height = fbsurf->height;

   return TRUE;
}

static boolean
fbdev_surface_flush_frontbuffer(struct native_surface *nsurf)
{
   struct fbdev_surface *fbsurf = fbdev_surface(nsurf);

   if (!fbsurf->is_current)
      return TRUE;

   return resource_surface_present(fbsurf->rsurf,
         NATIVE_ATTACHMENT_FRONT_LEFT, NULL);
}

static boolean
fbdev_surface_swap_buffers(struct native_surface *nsurf)
{
   struct fbdev_surface *fbsurf = fbdev_surface(nsurf);
   struct fbdev_display *fbdpy = fbsurf->fbdpy;
   boolean ret = TRUE;

   if (fbsurf->is_current) {
      ret = resource_surface_present(fbsurf->rsurf,
            NATIVE_ATTACHMENT_BACK_LEFT, NULL);
   }

   resource_surface_swap_buffers(fbsurf->rsurf,
         NATIVE_ATTACHMENT_FRONT_LEFT, NATIVE_ATTACHMENT_BACK_LEFT, TRUE);
   /* the front/back textures are swapped */
   fbsurf->sequence_number++;
   fbdpy->event_handler->invalid_surface(&fbdpy->base,
         &fbsurf->base, fbsurf->sequence_number);

   return ret;
}

static boolean
fbdev_surface_present(struct native_surface *nsurf,
                      enum native_attachment natt,
                      boolean preserve,
                      uint swap_interval)
{
   boolean ret;

   if (preserve || swap_interval)
      return FALSE;

   switch (natt) {
   case NATIVE_ATTACHMENT_FRONT_LEFT:
      ret = fbdev_surface_flush_frontbuffer(nsurf);
      break;
   case NATIVE_ATTACHMENT_BACK_LEFT:
      ret = fbdev_surface_swap_buffers(nsurf);
      break;
   default:
      ret = FALSE;
      break;
   }

   return ret;
}

static void
fbdev_surface_wait(struct native_surface *nsurf)
{
   /* no-op */
}

static void
fbdev_surface_destroy(struct native_surface *nsurf)
{
   struct fbdev_surface *fbsurf = fbdev_surface(nsurf);

   resource_surface_destroy(fbsurf->rsurf);
   FREE(fbsurf);
}

static struct native_surface *
fbdev_display_create_scanout_surface(struct native_display *ndpy,
                                   const struct native_config *nconf,
                                   uint width, uint height)
{
   struct fbdev_display *fbdpy = fbdev_display(ndpy);
   struct fbdev_surface *fbsurf;

   fbsurf = CALLOC_STRUCT(fbdev_surface);
   if (!fbsurf)
      return NULL;

   fbsurf->fbdpy = fbdpy;
   fbsurf->width = width;
   fbsurf->height = height;

   fbsurf->rsurf = resource_surface_create(fbdpy->base.screen,
         nconf->color_format,
         PIPE_BIND_RENDER_TARGET |
         PIPE_BIND_DISPLAY_TARGET |
         PIPE_BIND_SCANOUT);
   if (!fbsurf->rsurf) {
      FREE(fbsurf);
      return NULL;
   }

   resource_surface_set_size(fbsurf->rsurf, fbsurf->width, fbsurf->height);

   fbsurf->base.destroy = fbdev_surface_destroy;
   fbsurf->base.present = fbdev_surface_present;
   fbsurf->base.validate = fbdev_surface_validate;
   fbsurf->base.wait = fbdev_surface_wait;

   return &fbsurf->base;
}

static boolean
fbdev_display_program(struct native_display *ndpy, int crtc_idx,
                      struct native_surface *nsurf, uint x, uint y,
                      const struct native_connector **nconns, int num_nconns,
                      const struct native_mode *nmode)
{
   struct fbdev_display *fbdpy = fbdev_display(ndpy);
   struct fbdev_surface *fbsurf = fbdev_surface(nsurf);

   if (x || y)
      return FALSE;

   if (fbdpy->current_surface) {
      if (fbdpy->current_surface == fbsurf)
         return TRUE;
      fbdpy->current_surface->is_current = FALSE;
   }

   if (fbsurf)
      fbsurf->is_current = TRUE;
   fbdpy->current_surface = fbsurf;

   return TRUE;
}

static const struct native_mode **
fbdev_display_get_modes(struct native_display *ndpy,
                      const struct native_connector *nconn,
                      int *num_modes)
{
   struct fbdev_display *fbdpy = fbdev_display(ndpy);
   const struct native_mode **modes;

   modes = MALLOC(sizeof(*modes));
   if (modes) {
      modes[0] = &fbdpy->mode;
      if (num_modes)
         *num_modes = 1;
   }

   return modes;
}

static const struct native_connector **
fbdev_display_get_connectors(struct native_display *ndpy, int *num_connectors,
                           int *num_crtc)
{
   struct fbdev_display *fbdpy = fbdev_display(ndpy);
   const struct native_connector **connectors;

   connectors = MALLOC(sizeof(*connectors));
   if (connectors) {
      connectors[0] = &fbdpy->connector;
      if (num_connectors)
         *num_connectors = 1;
   }

   return connectors;
}

static struct native_display_modeset fbdev_display_modeset = {
   .get_connectors = fbdev_display_get_connectors,
   .get_modes = fbdev_display_get_modes,
   .create_scanout_surface = fbdev_display_create_scanout_surface,
   .program = fbdev_display_program
};

static const struct native_config **
fbdev_display_get_configs(struct native_display *ndpy, int *num_configs)
{
   struct fbdev_display *fbdpy = fbdev_display(ndpy);
   const struct native_config **configs;

   configs = MALLOC(sizeof(*configs));
   if (configs) {
      configs[0] = &fbdpy->config;
      if (num_configs)
         *num_configs = 1;
   }

   return configs;
}

static int
fbdev_display_get_param(struct native_display *ndpy,
                      enum native_param_type param)
{
   int val;

   switch (param) {
   case NATIVE_PARAM_USE_NATIVE_BUFFER:
   case NATIVE_PARAM_PRESERVE_BUFFER:
   case NATIVE_PARAM_MAX_SWAP_INTERVAL:
   default:
      val = 0;
      break;
   }

   return val;
}

static void
fbdev_display_destroy(struct native_display *ndpy)
{
   struct fbdev_display *fbdpy = fbdev_display(ndpy);

   fbdpy->base.screen->destroy(fbdpy->base.screen);
   close(fbdpy->fd);
   FREE(fbdpy);
}

static boolean
fbdev_display_init_modes(struct native_display *ndpy)
{
   struct fbdev_display *fbdpy = fbdev_display(ndpy);
   struct native_mode *nmode = &fbdpy->mode;

   nmode->desc = "Current Mode";
   nmode->width = fbdpy->vinfo.xres;
   nmode->height = fbdpy->vinfo.yres;
   nmode->refresh_rate = 60 * 1000; /* dummy */

   return TRUE;
}

static boolean
fbdev_display_init_connectors(struct native_display *ndpy)
{
   return TRUE;
}

static enum pipe_format
vinfo_to_format(const struct fb_var_screeninfo *vinfo)
{
   enum pipe_format format = PIPE_FORMAT_NONE;

   switch (vinfo->bits_per_pixel) {
   case 32:
      if (vinfo->red.length == 8 &&
          vinfo->green.length == 8 &&
          vinfo->blue.length == 8) {
         format = (vinfo->transp.length == 8) ?
            PIPE_FORMAT_B8G8R8A8_UNORM : PIPE_FORMAT_B8G8R8X8_UNORM;
      }
      break;
   case 16:
      if (vinfo->red.length == 5 &&
          vinfo->green.length == 6 &&
          vinfo->blue.length == 5 &&
          vinfo->transp.length == 0)
         format = PIPE_FORMAT_B5G6R5_UNORM;
      break;
   default:
      break;
   }

   return format;
}

static boolean
fbdev_display_init_configs(struct native_display *ndpy)
{
   struct fbdev_display *fbdpy = fbdev_display(ndpy);
   struct native_config *nconf = &fbdpy->config;

   nconf->color_format = vinfo_to_format(&fbdpy->vinfo);
   if (nconf->color_format == PIPE_FORMAT_NONE)
      return FALSE;

   nconf->buffer_mask =
      (1 << NATIVE_ATTACHMENT_FRONT_LEFT) |
      (1 << NATIVE_ATTACHMENT_BACK_LEFT);

   nconf->scanout_bit = TRUE;

   return TRUE;
}

static boolean
fbdev_display_init(struct native_display *ndpy)
{
   struct fbdev_display *fbdpy = fbdev_display(ndpy);
   struct sw_winsys *ws;

   if (ioctl(fbdpy->fd, FBIOGET_FSCREENINFO, &fbdpy->finfo))
      return FALSE;

   if (ioctl(fbdpy->fd, FBIOGET_VSCREENINFO, &fbdpy->vinfo))
      return FALSE;

   if (fbdpy->finfo.visual != FB_VISUAL_TRUECOLOR ||
       fbdpy->finfo.type != FB_TYPE_PACKED_PIXELS)
      return FALSE;

   if (!fbdev_display_init_configs(&fbdpy->base) ||
       !fbdev_display_init_connectors(&fbdpy->base) ||
       !fbdev_display_init_modes(&fbdpy->base))
      return FALSE;

   ws = fbdev_create_sw_winsys(fbdpy->fd, fbdpy->config.color_format);
   if (ws) {
      fbdpy->base.screen =
         fbdpy->event_handler->new_sw_screen(&fbdpy->base, ws);
   }

   if (fbdpy->base.screen) {
      if (!fbdpy->base.screen->is_format_supported(fbdpy->base.screen,
               fbdpy->config.color_format, PIPE_TEXTURE_2D, 0,
               PIPE_BIND_RENDER_TARGET, 0)) {
         fbdpy->base.screen->destroy(fbdpy->base.screen);
         fbdpy->base.screen = NULL;
      }
   }

   return (fbdpy->base.screen != NULL);
}

static struct native_display *
fbdev_display_create(int fd, struct native_event_handler *event_handler,
                     void *user_data)
{
   struct fbdev_display *fbdpy;

   fbdpy = CALLOC_STRUCT(fbdev_display);
   if (!fbdpy)
      return NULL;

   fbdpy->fd = fd;
   fbdpy->event_handler = event_handler;
   fbdpy->base.user_data = user_data;

   if (!fbdev_display_init(&fbdpy->base)) {
      FREE(fbdpy);
      return NULL;
   }

   fbdpy->base.destroy = fbdev_display_destroy;
   fbdpy->base.get_param = fbdev_display_get_param;
   fbdpy->base.get_configs = fbdev_display_get_configs;

   fbdpy->base.modeset = &fbdev_display_modeset;

   return &fbdpy->base;
}

static struct native_display *
native_create_display(void *dpy, struct native_event_handler *event_handler,
                      void *user_data)
{
   struct native_display *ndpy;
   int fd;

   /* well, this makes fd 0 being ignored */
   if (!dpy) {
      fd = open("/dev/fb0", O_RDWR);
   }
   else {
      fd = dup((int) pointer_to_intptr(dpy));
   }
   if (fd < 0)
      return NULL;

   ndpy = fbdev_display_create(fd, event_handler, user_data);
   if (!ndpy)
      close(fd);

   return ndpy;
}

static const struct native_platform fbdev_platform = {
   "FBDEV", /* name */
   native_create_display
};

const struct native_platform *
native_get_fbdev_platform(void)
{
   return &fbdev_platform;
}
