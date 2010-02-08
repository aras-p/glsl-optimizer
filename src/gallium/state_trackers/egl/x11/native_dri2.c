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

   struct drm_api *api;
   struct x11_screen *xscr;
   int xscr_number;

   struct dri2_config *configs;
   int num_configs;
};

struct dri2_surface {
   struct native_surface base;
   Drawable drawable;
   enum dri2_surface_type type;
   enum pipe_format color_format;
   struct dri2_display *dri2dpy;

   struct pipe_texture *pbuffer_textures[NUM_NATIVE_ATTACHMENTS];
   boolean have_back, have_fake;
   int width, height;
   unsigned int sequence_number;
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

   return TRUE;
}

static boolean
dri2_surface_validate(struct native_surface *nsurf, uint attachment_mask,
                      unsigned int *seq_num, struct pipe_texture **textures,
                      int *width, int *height)
{
   struct dri2_surface *dri2surf = dri2_surface(nsurf);
   struct dri2_display *dri2dpy = dri2surf->dri2dpy;
   unsigned int dri2atts[NUM_NATIVE_ATTACHMENTS];
   struct pipe_texture templ;
   struct x11_drawable_buffer *xbufs;
   int num_ins, num_outs, att, i;

   if (attachment_mask) {
      memset(&templ, 0, sizeof(templ));
      templ.target = PIPE_TEXTURE_2D;
      templ.last_level = 0;
      templ.width0 = dri2surf->width;
      templ.height0 = dri2surf->height;
      templ.depth0 = 1;
      templ.format = dri2surf->color_format;
      templ.tex_usage = PIPE_TEXTURE_USAGE_RENDER_TARGET;

      if (textures)
         memset(textures, 0, sizeof(*textures) * NUM_NATIVE_ATTACHMENTS);
   }

   /* create textures for pbuffer */
   if (dri2surf->type == DRI2_SURFACE_TYPE_PBUFFER) {
      struct pipe_screen *screen = dri2dpy->base.screen;

      for (att = 0; att < NUM_NATIVE_ATTACHMENTS; att++) {
         struct pipe_texture *ptex = dri2surf->pbuffer_textures[att];

         /* delay the allocation */
         if (!native_attachment_mask_test(attachment_mask, att))
            continue;

         if (!ptex) {
            ptex = screen->texture_create(screen, &templ);
            dri2surf->pbuffer_textures[att] = ptex;
         }

         if (textures)
            pipe_texture_reference(&textures[att], ptex);
      }

      if (seq_num)
         *seq_num = dri2surf->sequence_number;
      if (width)
         *width = dri2surf->width;
      if (height)
         *height = dri2surf->height;

      return TRUE;
   }

   /* prepare the attachments */
   num_ins = 0;
   for (att = 0; att < NUM_NATIVE_ATTACHMENTS; att++) {
      if (native_attachment_mask_test(attachment_mask, att)) {
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

   dri2surf->have_back = FALSE;
   dri2surf->have_fake = FALSE;

   /* remember old geometry */
   templ.width0 = dri2surf->width;
   templ.height0 = dri2surf->height;

   xbufs = x11_drawable_get_buffers(dri2dpy->xscr, dri2surf->drawable,
                                    &dri2surf->width, &dri2surf->height,
                                    dri2atts, FALSE, num_ins, &num_outs);
   if (!xbufs)
      return FALSE;

   if (templ.width0 != dri2surf->width || templ.height0 != dri2surf->height) {
      /* are there cases where the buffers change and the geometry doesn't? */
      dri2surf->sequence_number++;

      templ.width0 = dri2surf->width;
      templ.height0 = dri2surf->height;
   }

   for (i = 0; i < num_outs; i++) {
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

      if (!desc || !native_attachment_mask_test(attachment_mask, natt) ||
          (textures && textures[natt])) {
         if (!desc)
            _eglLog(_EGL_WARNING, "unknown buffer %d", xbuf->attachment);
         else if (!native_attachment_mask_test(attachment_mask, natt))
            _eglLog(_EGL_WARNING, "unexpected buffer %d", xbuf->attachment);
         else
            _eglLog(_EGL_WARNING, "both real and fake front buffers are listed");
         continue;
      }

      if (textures) {
         struct pipe_texture *ptex =
            dri2dpy->api->texture_from_shared_handle(dri2dpy->api,
                  dri2dpy->base.screen, &templ,
                  desc, xbuf->pitch, xbuf->name);
         if (ptex) {
            /* the caller owns the textures */
            textures[natt] = ptex;
         }
      }
   }

   free(xbufs);

   if (seq_num)
      *seq_num = dri2surf->sequence_number;
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

   for (i = 0; i < NUM_NATIVE_ATTACHMENTS; i++) {
      struct pipe_texture *ptex = dri2surf->pbuffer_textures[i];
      pipe_texture_reference(&ptex, NULL);
   }

   if (dri2surf->drawable)
      x11_drawable_enable_dri2(dri2surf->dri2dpy->xscr,
            dri2surf->drawable, FALSE);
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

   if (drawable)
      x11_drawable_enable_dri2(dri2dpy->xscr, drawable, TRUE);

   dri2surf->dri2dpy = dri2dpy;
   dri2surf->type = type;
   dri2surf->drawable = drawable;
   dri2surf->color_format = dri2conf->base.color_format;

   dri2surf->base.destroy = dri2_surface_destroy;
   dri2surf->base.swap_buffers = dri2_surface_swap_buffers;
   dri2surf->base.flush_frontbuffer = dri2_surface_flush_frontbuffer;
   dri2surf->base.validate = dri2_surface_validate;
   dri2surf->base.wait = dri2_surface_wait;

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
      formats[count++] = PIPE_FORMAT_A8R8G8B8_UNORM;
      formats[count++] = PIPE_FORMAT_B8G8R8A8_UNORM;
      break;
   case 24:
      formats[count++] = PIPE_FORMAT_X8R8G8B8_UNORM;
      formats[count++] = PIPE_FORMAT_B8G8R8X8_UNORM;
      formats[count++] = PIPE_FORMAT_A8R8G8B8_UNORM;
      formats[count++] = PIPE_FORMAT_B8G8R8A8_UNORM;
      break;
   case 16:
      formats[count++] = PIPE_FORMAT_R5G6B5_UNORM;
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
         formats[count++] = PIPE_FORMAT_S8Z24_UNORM;
         formats[count++] = PIPE_FORMAT_Z24S8_UNORM;
      }
      else {
         formats[count++] = PIPE_FORMAT_X8Z24_UNORM;
         formats[count++] = PIPE_FORMAT_Z24X8_UNORM;
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

static void
dri2_display_destroy(struct native_display *ndpy)
{
   struct dri2_display *dri2dpy = dri2_display(ndpy);

   if (dri2dpy->configs)
      free(dri2dpy->configs);

   if (dri2dpy->base.screen)
      dri2dpy->base.screen->destroy(dri2dpy->base.screen);

   if (dri2dpy->xscr)
      x11_screen_destroy(dri2dpy->xscr);
   if (dri2dpy->own_dpy)
      XCloseDisplay(dri2dpy->dpy);
   if (dri2dpy->api && dri2dpy->api->destroy)
      dri2dpy->api->destroy(dri2dpy->api);
   free(dri2dpy);
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

   fd = x11_screen_enable_dri2(dri2dpy->xscr, driver);
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

struct native_display *
x11_create_dri2_display(EGLNativeDisplayType dpy, struct drm_api *api)
{
   struct dri2_display *dri2dpy;

   dri2dpy = CALLOC_STRUCT(dri2_display);
   if (!dri2dpy)
      return NULL;

   dri2dpy->api = api;
   if (!dri2dpy->api) {
      _eglLog(_EGL_WARNING, "failed to create DRM API");
      free(dri2dpy);
      return NULL;
   }

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

   dri2dpy->base.destroy = dri2_display_destroy;
   dri2dpy->base.get_configs = dri2_display_get_configs;
   dri2dpy->base.is_pixmap_supported = dri2_display_is_pixmap_supported;
   dri2dpy->base.create_window_surface = dri2_display_create_window_surface;
   dri2dpy->base.create_pixmap_surface = dri2_display_create_pixmap_surface;
   dri2dpy->base.create_pbuffer_surface = dri2_display_create_pbuffer_surface;

   return &dri2dpy->base;
}
