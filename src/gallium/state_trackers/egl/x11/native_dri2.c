/*
 * Mesa 3-D graphics library
 * Version:  7.8
 *
 * Copyright (C) 2009-2010 Chia-I Wu <olv@0xlab.org>
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
 */

#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_format.h"
#include "util/u_inlines.h"
#include "util/u_hash_table.h"
#include "pipe/p_compiler.h"
#include "pipe/p_screen.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "state_tracker/drm_api.h"
#include "egllog.h"

#include "native_x11.h"
#include "x11_screen.h"

enum dri2_surface_type {
   DRI2_SURFACE_TYPE_WINDOW,
   DRI2_SURFACE_TYPE_PIXMAP,
   DRI2_SURFACE_TYPE_PBUFFER
};

struct dri2_display {
   struct native_display base;
   Display *dpy;
   boolean own_dpy;

   struct native_event_handler *event_handler;

   struct drm_api *api;
   struct x11_screen *xscr;
   int xscr_number;
   const char *dri_driver;
   int dri_major, dri_minor;

   struct dri2_config *configs;
   int num_configs;

   struct util_hash_table *surfaces;
};

struct dri2_surface {
   struct native_surface base;
   Drawable drawable;
   enum dri2_surface_type type;
   enum pipe_format color_format;
   struct dri2_display *dri2dpy;

   unsigned int server_stamp;
   unsigned int client_stamp;
   int width, height;
   struct pipe_texture *textures[NUM_NATIVE_ATTACHMENTS];
   uint valid_mask;

   boolean have_back, have_fake;

   struct x11_drawable_buffer *last_xbufs;
   int last_num_xbufs;
};

struct dri2_config {
   struct native_config base;
};

static INLINE struct dri2_display *
dri2_display(const struct native_display *ndpy)
{
   return (struct dri2_display *) ndpy;
}

static INLINE struct dri2_surface *
dri2_surface(const struct native_surface *nsurf)
{
   return (struct dri2_surface *) nsurf;
}

static INLINE struct dri2_config *
dri2_config(const struct native_config *nconf)
{
   return (struct dri2_config *) nconf;
}

/**
 * Process the buffers returned by the server.
 */
static void
dri2_surface_process_drawable_buffers(struct native_surface *nsurf,
                                      struct x11_drawable_buffer *xbufs,
                                      int num_xbufs)
{
   struct dri2_surface *dri2surf = dri2_surface(nsurf);
   struct dri2_display *dri2dpy = dri2surf->dri2dpy;
   struct pipe_texture templ;
   uint valid_mask;
   int i;

   /* free the old textures */
   for (i = 0; i < NUM_NATIVE_ATTACHMENTS; i++)
      pipe_texture_reference(&dri2surf->textures[i], NULL);
   dri2surf->valid_mask = 0x0;

   dri2surf->have_back = FALSE;
   dri2surf->have_fake = FALSE;

   if (!xbufs)
      return;

   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D;
   templ.last_level = 0;
   templ.width0 = dri2surf->width;
   templ.height0 = dri2surf->height;
   templ.depth0 = 1;
   templ.format = dri2surf->color_format;
   templ.tex_usage = PIPE_TEXTURE_USAGE_RENDER_TARGET;

   valid_mask = 0x0;
   for (i = 0; i < num_xbufs; i++) {
      struct x11_drawable_buffer *xbuf = &xbufs[i];
      const char *desc;
      enum native_attachment natt;

      switch (xbuf->attachment) {
      case DRI2BufferFrontLeft:
         natt = NATIVE_ATTACHMENT_FRONT_LEFT;
         desc = "DRI2 Front Buffer";
         break;
      case DRI2BufferFakeFrontLeft:
         natt = NATIVE_ATTACHMENT_FRONT_LEFT;
         desc = "DRI2 Fake Front Buffer";
         dri2surf->have_fake = TRUE;
         break;
      case DRI2BufferBackLeft:
         natt = NATIVE_ATTACHMENT_BACK_LEFT;
         desc = "DRI2 Back Buffer";
         dri2surf->have_back = TRUE;
         break;
      default:
         desc = NULL;
         break;
      }

      if (!desc || dri2surf->textures[natt]) {
         if (!desc)
            _eglLog(_EGL_WARNING, "unknown buffer %d", xbuf->attachment);
         else
            _eglLog(_EGL_WARNING, "both real and fake front buffers are listed");
         continue;
      }

      dri2surf->textures[natt] =
         dri2dpy->api->texture_from_shared_handle(dri2dpy->api,
               dri2dpy->base.screen, &templ, desc, xbuf->pitch, xbuf->name);
      if (dri2surf->textures[natt])
         valid_mask |= 1 << natt;
   }

   dri2surf->valid_mask = valid_mask;
}

/**
 * Get the buffers from the server.
 */
static void
dri2_surface_get_buffers(struct native_surface *nsurf, uint buffer_mask)
{
   struct dri2_surface *dri2surf = dri2_surface(nsurf);
   struct dri2_display *dri2dpy = dri2surf->dri2dpy;
   unsigned int dri2atts[NUM_NATIVE_ATTACHMENTS];
   int num_ins, num_outs, att;
   struct x11_drawable_buffer *xbufs;

   /* prepare the attachments */
   num_ins = 0;
   for (att = 0; att < NUM_NATIVE_ATTACHMENTS; att++) {
      if (native_attachment_mask_test(buffer_mask, att)) {
         unsigned int dri2att;

         switch (att) {
         case NATIVE_ATTACHMENT_FRONT_LEFT:
            dri2att = DRI2BufferFrontLeft;
            break;
         case NATIVE_ATTACHMENT_BACK_LEFT:
            dri2att = DRI2BufferBackLeft;
            break;
         case NATIVE_ATTACHMENT_FRONT_RIGHT:
            dri2att = DRI2BufferFrontRight;
            break;
         case NATIVE_ATTACHMENT_BACK_RIGHT:
            dri2att = DRI2BufferBackRight;
            break;
         default:
            assert(0);
            dri2att = 0;
            break;
         }

         dri2atts[num_ins] = dri2att;
         num_ins++;
      }
   }

   xbufs = x11_drawable_get_buffers(dri2dpy->xscr, dri2surf->drawable,
                                    &dri2surf->width, &dri2surf->height,
                                    dri2atts, FALSE, num_ins, &num_outs);

   /* we should be able to do better... */
   if (xbufs && dri2surf->last_num_xbufs == num_outs &&
       memcmp(dri2surf->last_xbufs, xbufs, sizeof(*xbufs) * num_outs) == 0) {
      free(xbufs);
      dri2surf->client_stamp = dri2surf->server_stamp;
      return;
   }

   dri2_surface_process_drawable_buffers(&dri2surf->base, xbufs, num_outs);

   dri2surf->server_stamp++;
   dri2surf->client_stamp = dri2surf->server_stamp;

   if (dri2surf->last_xbufs)
      free(dri2surf->last_xbufs);
   dri2surf->last_xbufs = xbufs;
   dri2surf->last_num_xbufs = num_outs;
}

/**
 * Update the buffers of the surface.  This is a slow function due to the
 * round-trip to the server.
 */
static boolean
dri2_surface_update_buffers(struct native_surface *nsurf, uint buffer_mask)
{
   struct dri2_surface *dri2surf = dri2_surface(nsurf);
   struct dri2_display *dri2dpy = dri2surf->dri2dpy;

   /* create textures for pbuffer */
   if (dri2surf->type == DRI2_SURFACE_TYPE_PBUFFER) {
      struct pipe_screen *screen = dri2dpy->base.screen;
      struct pipe_texture templ;
      uint new_valid = 0x0;
      int att;

      buffer_mask &= ~dri2surf->valid_mask;
      if (!buffer_mask)
         return TRUE;

      memset(&templ, 0, sizeof(templ));
      templ.target = PIPE_TEXTURE_2D;
      templ.last_level = 0;
      templ.width0 = dri2surf->width;
      templ.height0 = dri2surf->height;
      templ.depth0 = 1;
      templ.format = dri2surf->color_format;
      templ.tex_usage = PIPE_TEXTURE_USAGE_RENDER_TARGET;

      for (att = 0; att < NUM_NATIVE_ATTACHMENTS; att++) {
         if (native_attachment_mask_test(buffer_mask, att)) {
            assert(!dri2surf->textures[att]);

            dri2surf->textures[att] = screen->texture_create(screen, &templ);
            if (!dri2surf->textures[att])
               break;

            new_valid |= 1 << att;
            if (new_valid == buffer_mask)
               break;
         }
      }
      dri2surf->valid_mask |= new_valid;
      /* no need to update the stamps */
   }
   else {
      dri2_surface_get_buffers(&dri2surf->base, buffer_mask);
   }

   return ((dri2surf->valid_mask & buffer_mask) == buffer_mask);
}

/**
 * Return TRUE if the surface receives DRI2_InvalidateBuffers events.
 */
static INLINE boolean
dri2_surface_receive_events(struct native_surface *nsurf)
{
   struct dri2_surface *dri2surf = dri2_surface(nsurf);
   return (dri2surf->dri2dpy->dri_minor >= 3);
}

static boolean
dri2_surface_flush_frontbuffer(struct native_surface *nsurf)
{
   struct dri2_surface *dri2surf = dri2_surface(nsurf);
   struct dri2_display *dri2dpy = dri2surf->dri2dpy;

   /* pbuffer is private */
   if (dri2surf->type == DRI2_SURFACE_TYPE_PBUFFER)
      return TRUE;

   /* copy to real front buffer */
   if (dri2surf->have_fake)
      x11_drawable_copy_buffers(dri2dpy->xscr, dri2surf->drawable,
            0, 0, dri2surf->width, dri2surf->height,
            DRI2BufferFakeFrontLeft, DRI2BufferFrontLeft);

   /* force buffers to be updated in next validation call */
   if (!dri2_surface_receive_events(&dri2surf->base)) {
      dri2surf->server_stamp++;
      dri2dpy->event_handler->invalid_surface(&dri2dpy->base,
            &dri2surf->base, dri2surf->server_stamp);
   }

   return TRUE;
}

static boolean
dri2_surface_swap_buffers(struct native_surface *nsurf)
{
   struct dri2_surface *dri2surf = dri2_surface(nsurf);
   struct dri2_display *dri2dpy = dri2surf->dri2dpy;

   /* pbuffer is private */
   if (dri2surf->type == DRI2_SURFACE_TYPE_PBUFFER)
      return TRUE;

   /* copy to front buffer */
   if (dri2surf->have_back)
      x11_drawable_copy_buffers(dri2dpy->xscr, dri2surf->drawable,
            0, 0, dri2surf->width, dri2surf->height,
            DRI2BufferBackLeft, DRI2BufferFrontLeft);

   /* and update fake front buffer */
   if (dri2surf->have_fake)
      x11_drawable_copy_buffers(dri2dpy->xscr, dri2surf->drawable,
            0, 0, dri2surf->width, dri2surf->height,
            DRI2BufferFrontLeft, DRI2BufferFakeFrontLeft);

   /* force buffers to be updated in next validation call */
   if (!dri2_surface_receive_events(&dri2surf->base)) {
      dri2surf->server_stamp++;
      dri2dpy->event_handler->invalid_surface(&dri2dpy->base,
            &dri2surf->base, dri2surf->server_stamp);
   }

   return TRUE;
}

static boolean
dri2_surface_validate(struct native_surface *nsurf, uint attachment_mask,
                      unsigned int *seq_num, struct pipe_texture **textures,
                      int *width, int *height)
{
   struct dri2_surface *dri2surf = dri2_surface(nsurf);

   if (dri2surf->server_stamp != dri2surf->client_stamp ||
       (dri2surf->valid_mask & attachment_mask) != attachment_mask) {
      if (!dri2_surface_update_buffers(&dri2surf->base, attachment_mask))
         return FALSE;
   }

   if (seq_num)
      *seq_num = dri2surf->client_stamp;

   if (textures) {
      int att;
      for (att = 0; att < NUM_NATIVE_ATTACHMENTS; att++) {
         if (native_attachment_mask_test(attachment_mask, att)) {
            struct pipe_texture *ptex = dri2surf->textures[att];

            textures[att] = NULL;
            pipe_texture_reference(&textures[att], ptex);
         }
      }
   }

   if (width)
      *width = dri2surf->width;
   if (height)
      *height = dri2surf->height;

   return TRUE;
}

static void
dri2_surface_wait(struct native_surface *nsurf)
{
   struct dri2_surface *dri2surf = dri2_surface(nsurf);
   struct dri2_display *dri2dpy = dri2surf->dri2dpy;

   if (dri2surf->have_fake) {
      x11_drawable_copy_buffers(dri2dpy->xscr, dri2surf->drawable,
            0, 0, dri2surf->width, dri2surf->height,
            DRI2BufferFrontLeft, DRI2BufferFakeFrontLeft);
   }
}

static void
dri2_surface_destroy(struct native_surface *nsurf)
{
   struct dri2_surface *dri2surf = dri2_surface(nsurf);
   int i;

   if (dri2surf->last_xbufs)
      free(dri2surf->last_xbufs);

   for (i = 0; i < NUM_NATIVE_ATTACHMENTS; i++) {
      struct pipe_texture *ptex = dri2surf->textures[i];
      pipe_texture_reference(&ptex, NULL);
   }

   if (dri2surf->drawable) {
      x11_drawable_enable_dri2(dri2surf->dri2dpy->xscr,
            dri2surf->drawable, FALSE);

      util_hash_table_remove(dri2surf->dri2dpy->surfaces,
            (void *) dri2surf->drawable);
   }
   free(dri2surf);
}

static struct dri2_surface *
dri2_display_create_surface(struct native_display *ndpy,
                            enum dri2_surface_type type,
                            Drawable drawable,
                            const struct native_config *nconf)
{
   struct dri2_display *dri2dpy = dri2_display(ndpy);
   struct dri2_config *dri2conf = dri2_config(nconf);
   struct dri2_surface *dri2surf;

   dri2surf = CALLOC_STRUCT(dri2_surface);
   if (!dri2surf)
      return NULL;

   dri2surf->dri2dpy = dri2dpy;
   dri2surf->type = type;
   dri2surf->drawable = drawable;
   dri2surf->color_format = dri2conf->base.color_format;

   dri2surf->base.destroy = dri2_surface_destroy;
   dri2surf->base.swap_buffers = dri2_surface_swap_buffers;
   dri2surf->base.flush_frontbuffer = dri2_surface_flush_frontbuffer;
   dri2surf->base.validate = dri2_surface_validate;
   dri2surf->base.wait = dri2_surface_wait;

   if (drawable) {
      x11_drawable_enable_dri2(dri2dpy->xscr, drawable, TRUE);
      /* initialize the geometry */
      dri2_surface_update_buffers(&dri2surf->base, 0x0);

      util_hash_table_set(dri2surf->dri2dpy->surfaces,
            (void *) dri2surf->drawable, (void *) &dri2surf->base);
   }

   return dri2surf;
}

static struct native_surface *
dri2_display_create_window_surface(struct native_display *ndpy,
                                   EGLNativeWindowType win,
                                   const struct native_config *nconf)
{
   struct dri2_surface *dri2surf;

   dri2surf = dri2_display_create_surface(ndpy, DRI2_SURFACE_TYPE_WINDOW,
         (Drawable) win, nconf);
   return (dri2surf) ? &dri2surf->base : NULL;
}

static struct native_surface *
dri2_display_create_pixmap_surface(struct native_display *ndpy,
                                   EGLNativePixmapType pix,
                                   const struct native_config *nconf)
{
   struct dri2_surface *dri2surf;

   dri2surf = dri2_display_create_surface(ndpy, DRI2_SURFACE_TYPE_PIXMAP,
         (Drawable) pix, nconf);
   return (dri2surf) ? &dri2surf->base : NULL;
}

static struct native_surface *
dri2_display_create_pbuffer_surface(struct native_display *ndpy,
                                    const struct native_config *nconf,
                                    uint width, uint height)
{
   struct dri2_surface *dri2surf;

   dri2surf = dri2_display_create_surface(ndpy, DRI2_SURFACE_TYPE_PBUFFER,
         (Drawable) None, nconf);
   if (dri2surf) {
      dri2surf->width = width;
      dri2surf->height = height;
   }
   return (dri2surf) ? &dri2surf->base : NULL;
}

static int
choose_color_format(const __GLcontextModes *mode, enum pipe_format formats[32])
{
   int count = 0;

   switch (mode->rgbBits) {
   case 32:
      formats[count++] = PIPE_FORMAT_B8G8R8A8_UNORM;
      formats[count++] = PIPE_FORMAT_A8R8G8B8_UNORM;
      break;
   case 24:
      formats[count++] = PIPE_FORMAT_B8G8R8X8_UNORM;
      formats[count++] = PIPE_FORMAT_X8R8G8B8_UNORM;
      formats[count++] = PIPE_FORMAT_B8G8R8A8_UNORM;
      formats[count++] = PIPE_FORMAT_A8R8G8B8_UNORM;
      break;
   case 16:
      formats[count++] = PIPE_FORMAT_B5G6R5_UNORM;
      break;
   default:
      break;
   }

   return count;
}

static int
choose_depth_stencil_format(const __GLcontextModes *mode,
                            enum pipe_format formats[32])
{
   int count = 0;

   switch (mode->depthBits) {
   case 32:
      formats[count++] = PIPE_FORMAT_Z32_UNORM;
      break;
   case 24:
      if (mode->stencilBits) {
         formats[count++] = PIPE_FORMAT_Z24S8_UNORM;
         formats[count++] = PIPE_FORMAT_S8Z24_UNORM;
      }
      else {
         formats[count++] = PIPE_FORMAT_Z24X8_UNORM;
         formats[count++] = PIPE_FORMAT_X8Z24_UNORM;
      }
      break;
   case 16:
      formats[count++] = PIPE_FORMAT_Z16_UNORM;
      break;
   default:
      break;
   }

   return count;
}

static boolean
is_format_supported(struct pipe_screen *screen,
                    enum pipe_format fmt, boolean is_color)
{
   return screen->is_format_supported(screen, fmt, PIPE_TEXTURE_2D,
         (is_color) ? PIPE_TEXTURE_USAGE_RENDER_TARGET :
         PIPE_TEXTURE_USAGE_DEPTH_STENCIL, 0);
}

static boolean
dri2_display_convert_config(struct native_display *ndpy,
                            const __GLcontextModes *mode,
                            struct native_config *nconf)
{
   enum pipe_format formats[32];
   int num_formats, i;

   if (!(mode->renderType & GLX_RGBA_BIT) || !mode->rgbMode)
      return FALSE;

   /* skip single-buffered configs */
   if (!mode->doubleBufferMode)
      return FALSE;

   nconf->mode = *mode;
   nconf->mode.renderType = GLX_RGBA_BIT;
   nconf->mode.rgbMode = TRUE;
   /* pbuffer is allocated locally and is always supported */
   nconf->mode.drawableType |= GLX_PBUFFER_BIT;
   /* the swap method is always copy */
   nconf->mode.swapMethod = GLX_SWAP_COPY_OML;

   /* fix up */
   nconf->mode.rgbBits =
      nconf->mode.redBits + nconf->mode.greenBits +
      nconf->mode.blueBits + nconf->mode.alphaBits;
   if (!(nconf->mode.drawableType & GLX_WINDOW_BIT)) {
      nconf->mode.visualID = 0;
      nconf->mode.visualType = GLX_NONE;
   }
   if (!(nconf->mode.drawableType & GLX_PBUFFER_BIT)) {
      nconf->mode.bindToTextureRgb = FALSE;
      nconf->mode.bindToTextureRgba = FALSE;
   }

   nconf->color_format = PIPE_FORMAT_NONE;
   nconf->depth_format = PIPE_FORMAT_NONE;
   nconf->stencil_format = PIPE_FORMAT_NONE;

   /* choose color format */
   num_formats = choose_color_format(mode, formats);
   for (i = 0; i < num_formats; i++) {
      if (is_format_supported(ndpy->screen, formats[i], TRUE)) {
         nconf->color_format = formats[i];
         break;
      }
   }
   if (nconf->color_format == PIPE_FORMAT_NONE)
      return FALSE;

   /* choose depth/stencil format */
   num_formats = choose_depth_stencil_format(mode, formats);
   for (i = 0; i < num_formats; i++) {
      if (is_format_supported(ndpy->screen, formats[i], FALSE)) {
         nconf->depth_format = formats[i];
         nconf->stencil_format = formats[i];
         break;
      }
   }
   if ((nconf->mode.depthBits && nconf->depth_format == PIPE_FORMAT_NONE) ||
       (nconf->mode.stencilBits && nconf->stencil_format == PIPE_FORMAT_NONE))
      return FALSE;

   return TRUE;
}

static const struct native_config **
dri2_display_get_configs(struct native_display *ndpy, int *num_configs)
{
   struct dri2_display *dri2dpy = dri2_display(ndpy);
   const struct native_config **configs;
   int i;

   /* first time */
   if (!dri2dpy->configs) {
      const __GLcontextModes *modes;
      int num_modes, count;

      modes = x11_screen_get_glx_configs(dri2dpy->xscr);
      if (!modes)
         return NULL;
      num_modes = x11_context_modes_count(modes);

      dri2dpy->configs = calloc(num_modes, sizeof(*dri2dpy->configs));
      if (!dri2dpy->configs)
         return NULL;

      count = 0;
      for (i = 0; i < num_modes; i++) {
         struct native_config *nconf = &dri2dpy->configs[count].base;
         if (dri2_display_convert_config(&dri2dpy->base, modes, nconf))
            count++;
         modes = modes->next;
      }

      dri2dpy->num_configs = count;
   }

   configs = malloc(dri2dpy->num_configs * sizeof(*configs));
   if (configs) {
      for (i = 0; i < dri2dpy->num_configs; i++)
         configs[i] = (const struct native_config *) &dri2dpy->configs[i];
      if (num_configs)
         *num_configs = dri2dpy->num_configs;
   }

   return configs;
}

static boolean
dri2_display_is_pixmap_supported(struct native_display *ndpy,
                                 EGLNativePixmapType pix,
                                 const struct native_config *nconf)
{
   struct dri2_display *dri2dpy = dri2_display(ndpy);
   uint depth, nconf_depth;

   depth = x11_drawable_get_depth(dri2dpy->xscr, (Drawable) pix);
   nconf_depth = util_format_get_blocksizebits(nconf->color_format);

   /* simple depth match for now */
   return (depth == nconf_depth || (depth == 24 && depth + 8 == nconf_depth));
}

static int
dri2_display_get_param(struct native_display *ndpy,
                       enum native_param_type param)
{
   int val;

   switch (param) {
   case NATIVE_PARAM_USE_NATIVE_BUFFER:
      /* DRI2GetBuffers use the native buffers */
      val = TRUE;
      break;
   default:
      val = 0;
      break;
   }

   return val;
}

static void
dri2_display_destroy(struct native_display *ndpy)
{
   struct dri2_display *dri2dpy = dri2_display(ndpy);

   if (dri2dpy->configs)
      free(dri2dpy->configs);

   if (dri2dpy->base.screen)
      dri2dpy->base.screen->destroy(dri2dpy->base.screen);

   if (dri2dpy->surfaces)
      util_hash_table_destroy(dri2dpy->surfaces);

   if (dri2dpy->xscr)
      x11_screen_destroy(dri2dpy->xscr);
   if (dri2dpy->own_dpy)
      XCloseDisplay(dri2dpy->dpy);
   if (dri2dpy->api && dri2dpy->api->destroy)
      dri2dpy->api->destroy(dri2dpy->api);
   free(dri2dpy);
}

static void
dri2_display_invalidate_buffers(struct x11_screen *xscr, Drawable drawable,
                                void *user_data)
{
   struct native_display *ndpy = (struct native_display* ) user_data;
   struct dri2_display *dri2dpy = dri2_display(ndpy);
   struct native_surface *nsurf;
   struct dri2_surface *dri2surf;

   nsurf = (struct native_surface *)
      util_hash_table_get(dri2dpy->surfaces, (void *) drawable);
   if (!nsurf)
      return;

   dri2surf = dri2_surface(nsurf);

   dri2surf->server_stamp++;
   dri2dpy->event_handler->invalid_surface(&dri2dpy->base,
         &dri2surf->base, dri2surf->server_stamp);
}

/**
 * Initialize DRI2 and pipe screen.
 */
static boolean
dri2_display_init_screen(struct native_display *ndpy)
{
   struct dri2_display *dri2dpy = dri2_display(ndpy);
   const char *driver = dri2dpy->api->name;
   struct drm_create_screen_arg arg;
   int fd;

   if (!x11_screen_support(dri2dpy->xscr, X11_SCREEN_EXTENSION_DRI2) ||
       !x11_screen_support(dri2dpy->xscr, X11_SCREEN_EXTENSION_GLX)) {
      _eglLog(_EGL_WARNING, "GLX/DRI2 is not supported");
      return FALSE;
   }

   dri2dpy->dri_driver = x11_screen_probe_dri2(dri2dpy->xscr,
         &dri2dpy->dri_major, &dri2dpy->dri_minor);
   if (!dri2dpy->dri_driver || !driver ||
       strcmp(dri2dpy->dri_driver, driver) != 0) {
      _eglLog(_EGL_WARNING, "Driver mismatch: %s != %s",
            dri2dpy->dri_driver, dri2dpy->api->name);
      return FALSE;
   }

   fd = x11_screen_enable_dri2(dri2dpy->xscr,
         dri2_display_invalidate_buffers, &dri2dpy->base);
   if (fd < 0)
      return FALSE;

   memset(&arg, 0, sizeof(arg));
   arg.mode = DRM_CREATE_NORMAL;
   dri2dpy->base.screen = dri2dpy->api->create_screen(dri2dpy->api, fd, &arg);
   if (!dri2dpy->base.screen) {
      _eglLog(_EGL_WARNING, "failed to create DRM screen");
      return FALSE;
   }

   return TRUE;
}

static unsigned
dri2_display_hash_table_hash(void *key)
{
   XID drawable = pointer_to_uintptr(key);
   return (unsigned) drawable;
}

static int
dri2_display_hash_table_compare(void *key1, void *key2)
{
   return (key1 - key2);
}

struct native_display *
x11_create_dri2_display(EGLNativeDisplayType dpy,
                        struct native_event_handler *event_handler,
                        struct drm_api *api)
{
   struct dri2_display *dri2dpy;

   dri2dpy = CALLOC_STRUCT(dri2_display);
   if (!dri2dpy)
      return NULL;

   dri2dpy->event_handler = event_handler;
   dri2dpy->api = api;

   dri2dpy->dpy = dpy;
   if (!dri2dpy->dpy) {
      dri2dpy->dpy = XOpenDisplay(NULL);
      if (!dri2dpy->dpy) {
         dri2_display_destroy(&dri2dpy->base);
         return NULL;
      }
      dri2dpy->own_dpy = TRUE;
   }

   dri2dpy->xscr_number = DefaultScreen(dri2dpy->dpy);
   dri2dpy->xscr = x11_screen_create(dri2dpy->dpy, dri2dpy->xscr_number);
   if (!dri2dpy->xscr) {
      dri2_display_destroy(&dri2dpy->base);
      return NULL;
   }

   if (!dri2_display_init_screen(&dri2dpy->base)) {
      dri2_display_destroy(&dri2dpy->base);
      return NULL;
   }

   dri2dpy->surfaces = util_hash_table_create(dri2_display_hash_table_hash,
         dri2_display_hash_table_compare);
   if (!dri2dpy->surfaces) {
      dri2_display_destroy(&dri2dpy->base);
      return NULL;
   }

   dri2dpy->base.destroy = dri2_display_destroy;
   dri2dpy->base.get_param = dri2_display_get_param;
   dri2dpy->base.get_configs = dri2_display_get_configs;
   dri2dpy->base.is_pixmap_supported = dri2_display_is_pixmap_supported;
   dri2dpy->base.create_window_surface = dri2_display_create_window_surface;
   dri2dpy->base.create_pixmap_surface = dri2_display_create_pixmap_surface;
   dri2dpy->base.create_pbuffer_surface = dri2_display_create_pbuffer_surface;

   return &dri2dpy->base;
}
