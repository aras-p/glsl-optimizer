/*
 * Mesa 3-D graphics library
 * Version:  7.8
 *
 * Copyright (C) 2010 Chia-I Wu <olv@0xlab.org>
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

#include <stdio.h>
#include <string.h>

#include "pipe/p_screen.h"
#include "pipe/p_context.h"
#include "util/u_debug.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "egllog.h"

#include "native_kms.h"

static boolean
kms_surface_validate(struct native_surface *nsurf, uint attachment_mask,
                     unsigned int *seq_num, struct pipe_texture **textures,
                     int *width, int *height)
{
   struct kms_surface *ksurf = kms_surface(nsurf);
   struct kms_display *kdpy = ksurf->kdpy;
   struct pipe_screen *screen = kdpy->base.screen;
   struct pipe_texture templ, *ptex;
   int att;

   if (attachment_mask) {
      memset(&templ, 0, sizeof(templ));
      templ.target = PIPE_TEXTURE_2D;
      templ.last_level = 0;
      templ.width0 = ksurf->width;
      templ.height0 = ksurf->height;
      templ.depth0 = 1;
      templ.format = ksurf->color_format;
      templ.tex_usage = PIPE_TEXTURE_USAGE_RENDER_TARGET;
      if (ksurf->type == KMS_SURFACE_TYPE_SCANOUT)
         templ.tex_usage |= PIPE_TEXTURE_USAGE_PRIMARY;
   }

   /* create textures */
   for (att = 0; att < NUM_NATIVE_ATTACHMENTS; att++) {
      /* delay the allocation */
      if (!native_attachment_mask_test(attachment_mask, att))
         continue;

      ptex = ksurf->textures[att];
      if (!ptex) {
         ptex = screen->texture_create(screen, &templ);
         ksurf->textures[att] = ptex;
      }

      if (textures) {
         textures[att] = NULL;
         pipe_texture_reference(&textures[att], ptex);
      }
   }

   if (seq_num)
      *seq_num = ksurf->sequence_number;
   if (width)
      *width = ksurf->width;
   if (height)
      *height = ksurf->height;

   return TRUE;
}

/**
 * Add textures as DRM framebuffers.
 */
static boolean
kms_surface_init_framebuffers(struct native_surface *nsurf, boolean need_back)
{
   struct kms_surface *ksurf = kms_surface(nsurf);
   struct kms_display *kdpy = ksurf->kdpy;
   int num_framebuffers = (need_back) ? 2 : 1;
   int i, err;

   for (i = 0; i < num_framebuffers; i++) {
      struct kms_framebuffer *fb;
      enum native_attachment natt;
      unsigned int handle, stride;
      uint block_bits;

      if (i == 0) {
         fb = &ksurf->front_fb;
         natt = NATIVE_ATTACHMENT_FRONT_LEFT;
      }
      else {
         fb = &ksurf->back_fb;
         natt = NATIVE_ATTACHMENT_BACK_LEFT;
      }

      if (!fb->texture) {
         /* make sure the texture has been allocated */
         kms_surface_validate(&ksurf->base, 1 << natt, NULL, NULL, NULL, NULL);
         if (!ksurf->textures[natt])
            return FALSE;

         pipe_texture_reference(&fb->texture, ksurf->textures[natt]);
      }

      /* already initialized */
      if (fb->buffer_id)
         continue;

      /* TODO detect the real value */
      fb->is_passive = TRUE;

      if (!kdpy->api->local_handle_from_texture(kdpy->api,
               kdpy->base.screen, fb->texture, &stride, &handle))
         return FALSE;

      block_bits = util_format_get_blocksizebits(ksurf->color_format);
      err = drmModeAddFB(kdpy->fd, ksurf->width, ksurf->height,
            block_bits, block_bits, stride, handle, &fb->buffer_id);
      if (err) {
         fb->buffer_id = 0;
         return FALSE;
      }
   }

   return TRUE;
}

static boolean
kms_surface_flush_frontbuffer(struct native_surface *nsurf)
{
#ifdef DRM_MODE_FEATURE_DIRTYFB
   struct kms_surface *ksurf = kms_surface(nsurf);
   struct kms_display *kdpy = ksurf->kdpy;

   /* pbuffer is private */
   if (ksurf->type == KMS_SURFACE_TYPE_PBUFFER)
      return TRUE;

   if (ksurf->front_fb.is_passive)
      drmModeDirtyFB(kdpy->fd, ksurf->front_fb.buffer_id, NULL, 0);
#endif

   return TRUE;
}

static boolean
kms_surface_swap_buffers(struct native_surface *nsurf)
{
   struct kms_surface *ksurf = kms_surface(nsurf);
   struct kms_crtc *kcrtc = &ksurf->current_crtc;
   struct kms_display *kdpy = ksurf->kdpy;
   struct kms_framebuffer tmp_fb;
   struct pipe_texture *tmp_texture;
   int err;

   /* pbuffer is private */
   if (ksurf->type == KMS_SURFACE_TYPE_PBUFFER)
      return TRUE;

   if (!ksurf->back_fb.buffer_id) {
      if (!kms_surface_init_framebuffers(&ksurf->base, TRUE))
         return FALSE;
   }

   if (ksurf->is_shown && kcrtc->crtc) {
      err = drmModeSetCrtc(kdpy->fd, kcrtc->crtc->crtc_id,
            ksurf->back_fb.buffer_id, kcrtc->crtc->x, kcrtc->crtc->y,
            kcrtc->connectors, kcrtc->num_connectors, &kcrtc->crtc->mode);
      if (err)
         return FALSE;
   }

   /* swap the buffers */
   tmp_fb = ksurf->front_fb;
   ksurf->front_fb = ksurf->back_fb;
   ksurf->back_fb = tmp_fb;

   tmp_texture = ksurf->textures[NATIVE_ATTACHMENT_FRONT_LEFT];
   ksurf->textures[NATIVE_ATTACHMENT_FRONT_LEFT] =
      ksurf->textures[NATIVE_ATTACHMENT_BACK_LEFT];
   ksurf->textures[NATIVE_ATTACHMENT_BACK_LEFT] = tmp_texture;

   /* the front/back textures are swapped */
   ksurf->sequence_number++;
   kdpy->event_handler->invalid_surface(&kdpy->base,
         &ksurf->base, ksurf->sequence_number);

   return TRUE;
}

static void
kms_surface_wait(struct native_surface *nsurf)
{
   /* no-op */
}

static void
kms_surface_destroy(struct native_surface *nsurf)
{
   struct kms_surface *ksurf = kms_surface(nsurf);
   int i;

   if (ksurf->current_crtc.crtc)
         drmModeFreeCrtc(ksurf->current_crtc.crtc);

   if (ksurf->front_fb.buffer_id)
      drmModeRmFB(ksurf->kdpy->fd, ksurf->front_fb.buffer_id);
   pipe_texture_reference(&ksurf->front_fb.texture, NULL);

   if (ksurf->back_fb.buffer_id)
      drmModeRmFB(ksurf->kdpy->fd, ksurf->back_fb.buffer_id);
   pipe_texture_reference(&ksurf->back_fb.texture, NULL);

   for (i = 0; i < NUM_NATIVE_ATTACHMENTS; i++) {
      struct pipe_texture *ptex = ksurf->textures[i];
      pipe_texture_reference(&ptex, NULL);
   }

   free(ksurf);
}

static struct kms_surface *
kms_display_create_surface(struct native_display *ndpy,
                           enum kms_surface_type type,
                           const struct native_config *nconf,
                           uint width, uint height)
{
   struct kms_display *kdpy = kms_display(ndpy);
   struct kms_config *kconf = kms_config(nconf);
   struct kms_surface *ksurf;

   ksurf = CALLOC_STRUCT(kms_surface);
   if (!ksurf)
      return NULL;

   ksurf->kdpy = kdpy;
   ksurf->type = type;
   ksurf->color_format = kconf->base.color_format;
   ksurf->width = width;
   ksurf->height = height;

   ksurf->base.destroy = kms_surface_destroy;
   ksurf->base.swap_buffers = kms_surface_swap_buffers;
   ksurf->base.flush_frontbuffer = kms_surface_flush_frontbuffer;
   ksurf->base.validate = kms_surface_validate;
   ksurf->base.wait = kms_surface_wait;

   return ksurf;
}

/**
 * Choose a CRTC that supports all given connectors.
 */
static uint32_t
kms_display_choose_crtc(struct native_display *ndpy,
                        uint32_t *connectors, int num_connectors)
{
   struct kms_display *kdpy = kms_display(ndpy);
   int idx;

   for (idx = 0; idx < kdpy->resources->count_crtcs; idx++) {
      boolean found_crtc = TRUE;
      int i, j;

      for (i = 0; i < num_connectors; i++) {
         drmModeConnectorPtr connector;
         int encoder_idx = -1;

         connector = drmModeGetConnector(kdpy->fd, connectors[i]);
         if (!connector) {
            found_crtc = FALSE;
            break;
         }

         /* find an encoder the CRTC supports */
         for (j = 0; j < connector->count_encoders; j++) {
            drmModeEncoderPtr encoder =
               drmModeGetEncoder(kdpy->fd, connector->encoders[j]);
            if (encoder->possible_crtcs & (1 << idx)) {
               encoder_idx = j;
               break;
            }
            drmModeFreeEncoder(encoder);
         }

         drmModeFreeConnector(connector);
         if (encoder_idx < 0) {
            found_crtc = FALSE;
            break;
         }
      }

      if (found_crtc)
         break;
   }

   if (idx >= kdpy->resources->count_crtcs) {
      _eglLog(_EGL_WARNING,
            "failed to find a CRTC that supports the given %d connectors",
            num_connectors);
      return 0;
   }

   return kdpy->resources->crtcs[idx];
}

/**
 * Remember the original CRTC status and set the CRTC
 */
static boolean
kms_display_set_crtc(struct native_display *ndpy, int crtc_idx,
                     uint32_t buffer_id, uint32_t x, uint32_t y,
                     uint32_t *connectors, int num_connectors,
                     drmModeModeInfoPtr mode)
{
   struct kms_display *kdpy = kms_display(ndpy);
   struct kms_crtc *kcrtc = &kdpy->saved_crtcs[crtc_idx];
   uint32_t crtc_id;
   int err;

   if (kcrtc->crtc) {
      crtc_id = kcrtc->crtc->crtc_id;
   }
   else {
      int count = 0, i;

      /*
       * Choose the CRTC once.  It could be more dynamic, but let's keep it
       * simple for now.
       */
      crtc_id = kms_display_choose_crtc(&kdpy->base,
            connectors, num_connectors);

      /* save the original CRTC status */
      kcrtc->crtc = drmModeGetCrtc(kdpy->fd, crtc_id);
      if (!kcrtc->crtc)
         return FALSE;

      for (i = 0; i < kdpy->num_connectors; i++) {
         struct kms_connector *kconn = &kdpy->connectors[i];
         drmModeConnectorPtr connector = kconn->connector;
         drmModeEncoderPtr encoder;

         encoder = drmModeGetEncoder(kdpy->fd, connector->encoder_id);
         if (encoder) {
            if (encoder->crtc_id == crtc_id) {
               kcrtc->connectors[count++] = connector->connector_id;
               if (count >= Elements(kcrtc->connectors))
                  break;
            }
            drmModeFreeEncoder(encoder);
         }
      }

      kcrtc->num_connectors = count;
   }

   err = drmModeSetCrtc(kdpy->fd, crtc_id, buffer_id, x, y,
         connectors, num_connectors, mode);
   if (err) {
      drmModeFreeCrtc(kcrtc->crtc);
      kcrtc->crtc = NULL;
      kcrtc->num_connectors = 0;

      return FALSE;
   }

   return TRUE;
}

static boolean
kms_display_program(struct native_display *ndpy, int crtc_idx,
                    struct native_surface *nsurf, uint x, uint y,
                    const struct native_connector **nconns, int num_nconns,
                    const struct native_mode *nmode)
{
   struct kms_display *kdpy = kms_display(ndpy);
   struct kms_surface *ksurf = kms_surface(nsurf);
   const struct kms_mode *kmode = kms_mode(nmode);
   uint32_t connector_ids[32];
   uint32_t buffer_id;
   drmModeModeInfo mode_tmp, *mode;
   int i;

   if (num_nconns > Elements(connector_ids)) {
      _eglLog(_EGL_WARNING, "too many connectors (%d)", num_nconns);
      num_nconns = Elements(connector_ids);
   }

   if (ksurf) {
      if (!kms_surface_init_framebuffers(&ksurf->base, FALSE))
         return FALSE;

      buffer_id = ksurf->front_fb.buffer_id;
      /* the mode argument of drmModeSetCrtc is not constified */
      mode_tmp = kmode->mode;
      mode = &mode_tmp;
   }
   else {
      /* disable the CRTC */
      buffer_id = 0;
      mode = NULL;
      num_nconns = 0;
   }

   for (i = 0; i < num_nconns; i++) {
      struct kms_connector *kconn = kms_connector(nconns[i]);
      connector_ids[i] = kconn->connector->connector_id;
   }

   if (!kms_display_set_crtc(&kdpy->base, crtc_idx, buffer_id, x, y,
            connector_ids, num_nconns, mode)) {
      _eglLog(_EGL_WARNING, "failed to set CRTC %d", crtc_idx);

      return FALSE;
   }

   if (kdpy->shown_surfaces[crtc_idx])
      kdpy->shown_surfaces[crtc_idx]->is_shown = FALSE;
   kdpy->shown_surfaces[crtc_idx] = ksurf;

   /* remember the settings for buffer swapping */
   if (ksurf) {
      uint32_t crtc_id = kdpy->saved_crtcs[crtc_idx].crtc->crtc_id;
      struct kms_crtc *kcrtc = &ksurf->current_crtc;

      if (kcrtc->crtc)
         drmModeFreeCrtc(kcrtc->crtc);
      kcrtc->crtc = drmModeGetCrtc(kdpy->fd, crtc_id);

      assert(num_nconns < Elements(kcrtc->connectors));
      memcpy(kcrtc->connectors, connector_ids,
            sizeof(*connector_ids) * num_nconns);
      kcrtc->num_connectors = num_nconns;

      ksurf->is_shown = TRUE;
   }

   return TRUE;
}

static const struct native_mode **
kms_display_get_modes(struct native_display *ndpy,
                      const struct native_connector *nconn,
                      int *num_modes)
{
   struct kms_display *kdpy = kms_display(ndpy);
   struct kms_connector *kconn = kms_connector(nconn);
   const struct native_mode **nmodes_return;
   int count, i;

   /* delete old data */
   if (kconn->connector) {
      drmModeFreeConnector(kconn->connector);
      free(kconn->kms_modes);

      kconn->connector = NULL;
      kconn->kms_modes = NULL;
      kconn->num_modes = 0;
   }

   /* detect again */
   kconn->connector = drmModeGetConnector(kdpy->fd, kconn->connector_id);
   if (!kconn->connector)
      return NULL;

   count = kconn->connector->count_modes;
   kconn->kms_modes = calloc(count, sizeof(*kconn->kms_modes));
   if (!kconn->kms_modes) {
      drmModeFreeConnector(kconn->connector);
      kconn->connector = NULL;

      return NULL;
   }

   for (i = 0; i < count; i++) {
      struct kms_mode *kmode = &kconn->kms_modes[i];
      drmModeModeInfoPtr mode = &kconn->connector->modes[i];

      kmode->mode = *mode;

      kmode->base.desc = kmode->mode.name;
      kmode->base.width = kmode->mode.hdisplay;
      kmode->base.height = kmode->mode.vdisplay;
      kmode->base.refresh_rate = kmode->mode.vrefresh;
      /* not all kernels have vrefresh = refresh_rate * 1000 */
      if (kmode->base.refresh_rate > 1000)
         kmode->base.refresh_rate = (kmode->base.refresh_rate + 500) / 1000;
   }

   nmodes_return = malloc(count * sizeof(*nmodes_return));
   if (nmodes_return) {
      for (i = 0; i < count; i++)
         nmodes_return[i] = &kconn->kms_modes[i].base;
      if (num_modes)
         *num_modes = count;
   }

   return nmodes_return;
}

static const struct native_connector **
kms_display_get_connectors(struct native_display *ndpy, int *num_connectors,
                           int *num_crtc)
{
   struct kms_display *kdpy = kms_display(ndpy);
   const struct native_connector **connectors;
   int i;

   if (!kdpy->connectors) {
      kdpy->connectors =
         calloc(kdpy->resources->count_connectors, sizeof(*kdpy->connectors));
      if (!kdpy->connectors)
         return NULL;

      for (i = 0; i < kdpy->resources->count_connectors; i++) {
         struct kms_connector *kconn = &kdpy->connectors[i];

         kconn->connector_id = kdpy->resources->connectors[i];
         /* kconn->connector is allocated when the modes are asked */
      }

      kdpy->num_connectors = kdpy->resources->count_connectors;
   }

   connectors = malloc(kdpy->num_connectors * sizeof(*connectors));
   if (connectors) {
      for (i = 0; i < kdpy->num_connectors; i++)
         connectors[i] = &kdpy->connectors[i].base;
      if (num_connectors)
         *num_connectors = kdpy->num_connectors;
   }

   if (num_crtc)
      *num_crtc = kdpy->resources->count_crtcs;

   return connectors;
}

static struct native_surface *
kms_display_create_scanout_surface(struct native_display *ndpy,
                                   const struct native_config *nconf,
                                   uint width, uint height)
{
   struct kms_surface *ksurf;

   ksurf = kms_display_create_surface(ndpy,
         KMS_SURFACE_TYPE_SCANOUT, nconf, width, height);
   return &ksurf->base;
}

static struct native_surface *
kms_display_create_pbuffer_surface(struct native_display *ndpy,
                                   const struct native_config *nconf,
                                   uint width, uint height)
{
   struct kms_surface *ksurf;

   ksurf = kms_display_create_surface(ndpy,
         KMS_SURFACE_TYPE_PBUFFER, nconf, width, height);
   return &ksurf->base;
}


static boolean
kms_display_is_format_supported(struct native_display *ndpy,
                                enum pipe_format fmt, boolean is_color)
{
   return ndpy->screen->is_format_supported(ndpy->screen,
         fmt, PIPE_TEXTURE_2D,
         (is_color) ? PIPE_TEXTURE_USAGE_RENDER_TARGET :
         PIPE_TEXTURE_USAGE_DEPTH_STENCIL, 0);
}

static const struct native_config **
kms_display_get_configs(struct native_display *ndpy, int *num_configs)
{
   struct kms_display *kdpy = kms_display(ndpy);
   const struct native_config **configs;

   /* first time */
   if (!kdpy->config) {
      struct native_config *nconf;
      enum pipe_format format;

      kdpy->config = calloc(1, sizeof(*kdpy->config));
      if (!kdpy->config)
         return NULL;

      nconf = &kdpy->config->base;

      /* always double-buffered */
      nconf->mode.doubleBufferMode = TRUE;

      format = PIPE_FORMAT_B8G8R8A8_UNORM;
      if (!kms_display_is_format_supported(&kdpy->base, format, TRUE)) {
         format = PIPE_FORMAT_A8R8G8B8_UNORM;
         if (!kms_display_is_format_supported(&kdpy->base, format, TRUE))
            format = PIPE_FORMAT_NONE;
      }
      if (format == PIPE_FORMAT_NONE)
         return NULL;

      nconf->color_format = format;
      nconf->mode.redBits = 8;
      nconf->mode.greenBits = 8;
      nconf->mode.blueBits = 8;
      nconf->mode.alphaBits = 8;
      nconf->mode.rgbBits = 32;

      format = PIPE_FORMAT_Z24S8_UNORM;
      if (!kms_display_is_format_supported(&kdpy->base, format, FALSE)) {
         format = PIPE_FORMAT_S8Z24_UNORM;
         if (!kms_display_is_format_supported(&kdpy->base, format, FALSE))
            format = PIPE_FORMAT_NONE;
      }
      if (format != PIPE_FORMAT_NONE) {
         nconf->depth_format = format;
         nconf->stencil_format = format;

         nconf->mode.depthBits = 24;
         nconf->mode.stencilBits = 8;
         nconf->mode.haveDepthBuffer = TRUE;
         nconf->mode.haveStencilBuffer = TRUE;
      }

      nconf->scanout_bit = TRUE;
      nconf->mode.drawableType = GLX_PBUFFER_BIT;
      nconf->mode.swapMethod = GLX_SWAP_EXCHANGE_OML;

      nconf->mode.visualID = 0;
      nconf->mode.visualType = EGL_NONE;

      nconf->mode.renderType = GLX_RGBA_BIT;
      nconf->mode.rgbMode = TRUE;
      nconf->mode.xRenderable = FALSE;
   }

   configs = malloc(sizeof(*configs));
   if (configs) {
      configs[0] = &kdpy->config->base;
      if (num_configs)
         *num_configs = 1;
   }

   return configs;
}

static int
kms_display_get_param(struct native_display *ndpy,
                      enum native_param_type param)
{
   int val;

   switch (param) {
   default:
      val = 0;
      break;
   }

   return val;
}

static void
kms_display_destroy(struct native_display *ndpy)
{
   struct kms_display *kdpy = kms_display(ndpy);
   int i;

   if (kdpy->config)
      free(kdpy->config);

   if (kdpy->connectors) {
      for (i = 0; i < kdpy->num_connectors; i++) {
         struct kms_connector *kconn = &kdpy->connectors[i];
         if (kconn->connector) {
            drmModeFreeConnector(kconn->connector);
            free(kconn->kms_modes);
         }
      }
      free(kdpy->connectors);
   }

   if (kdpy->shown_surfaces)
      free(kdpy->shown_surfaces);

   if (kdpy->saved_crtcs) {
      for (i = 0; i < kdpy->resources->count_crtcs; i++) {
         struct kms_crtc *kcrtc = &kdpy->saved_crtcs[i];

         if (kcrtc->crtc) {
            /* restore crtc */
            drmModeSetCrtc(kdpy->fd, kcrtc->crtc->crtc_id,
                  kcrtc->crtc->buffer_id, kcrtc->crtc->x, kcrtc->crtc->y,
                  kcrtc->connectors, kcrtc->num_connectors,
                  &kcrtc->crtc->mode);

            drmModeFreeCrtc(kcrtc->crtc);
         }
      }
      free(kdpy->saved_crtcs);
   }

   if (kdpy->resources)
      drmModeFreeResources(kdpy->resources);

   if (kdpy->base.screen)
      kdpy->base.screen->destroy(kdpy->base.screen);

   if (kdpy->fd >= 0)
      drmClose(kdpy->fd);

   if (kdpy->api)
      kdpy->api->destroy(kdpy->api);
   free(kdpy);
}

/**
 * Initialize KMS and pipe screen.
 */
static boolean
kms_display_init_screen(struct native_display *ndpy)
{
   struct kms_display *kdpy = kms_display(ndpy);
   struct drm_create_screen_arg arg;
   int fd;

   fd = drmOpen(kdpy->api->name, NULL);
   if (fd < 0) {
      _eglLog(_EGL_WARNING, "failed to open DRM device");
      return FALSE;
   }

#if 0
   if (drmSetMaster(fd)) {
      _eglLog(_EGL_WARNING, "failed to become DRM master");
      return FALSE;
   }
#endif

   memset(&arg, 0, sizeof(arg));
   arg.mode = DRM_CREATE_NORMAL;
   kdpy->base.screen = kdpy->api->create_screen(kdpy->api, fd, &arg);
   if (!kdpy->base.screen) {
      _eglLog(_EGL_WARNING, "failed to create DRM screen");
      drmClose(fd);
      return FALSE;
   }

   kdpy->fd = fd;

   return TRUE;
}

static struct native_display_modeset kms_display_modeset = {
   .get_connectors = kms_display_get_connectors,
   .get_modes = kms_display_get_modes,
   .create_scanout_surface = kms_display_create_scanout_surface,
   .program = kms_display_program
};

static struct native_display *
kms_create_display(EGLNativeDisplayType dpy,
                   struct native_event_handler *event_handler,
                   struct drm_api *api)
{
   struct kms_display *kdpy;

   kdpy = CALLOC_STRUCT(kms_display);
   if (!kdpy)
      return NULL;

   kdpy->event_handler = event_handler;

   kdpy->api = api;
   if (!kdpy->api) {
      _eglLog(_EGL_WARNING, "failed to create DRM API");
      free(kdpy);
      return NULL;
   }

   kdpy->fd = -1;
   if (!kms_display_init_screen(&kdpy->base)) {
      kms_display_destroy(&kdpy->base);
      return NULL;
   }

   /* resources are fixed, unlike crtc, connector, or encoder */
   kdpy->resources = drmModeGetResources(kdpy->fd);
   if (!kdpy->resources) {
      kms_display_destroy(&kdpy->base);
      return NULL;
   }

   kdpy->saved_crtcs =
      calloc(kdpy->resources->count_crtcs, sizeof(*kdpy->saved_crtcs));
   if (!kdpy->saved_crtcs) {
      kms_display_destroy(&kdpy->base);
      return NULL;
   }

   kdpy->shown_surfaces =
      calloc(kdpy->resources->count_crtcs, sizeof(*kdpy->shown_surfaces));
   if (!kdpy->shown_surfaces) {
      kms_display_destroy(&kdpy->base);
      return NULL;
   }

   kdpy->base.destroy = kms_display_destroy;
   kdpy->base.get_param = kms_display_get_param;
   kdpy->base.get_configs = kms_display_get_configs;
   kdpy->base.create_pbuffer_surface = kms_display_create_pbuffer_surface;

   kdpy->base.modeset = &kms_display_modeset;

   return &kdpy->base;
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

/* the api is destroyed with the native display */
static struct drm_api *drm_api;

const char *
native_get_name(void)
{
   static char kms_name[32];

   if (!drm_api)
      drm_api = drm_api_create();

   if (drm_api)
      snprintf(kms_name, sizeof(kms_name), "KMS/%s", drm_api->name);
   else
      snprintf(kms_name, sizeof(kms_name), "KMS");

   return kms_name;
}

struct native_display *
native_create_display(EGLNativeDisplayType dpy,
                      struct native_event_handler *event_handler)
{
   struct native_display *ndpy = NULL;

   if (!drm_api)
      drm_api = drm_api_create();

   if (drm_api)
      ndpy = kms_create_display(dpy, event_handler, drm_api);

   return ndpy;
}
