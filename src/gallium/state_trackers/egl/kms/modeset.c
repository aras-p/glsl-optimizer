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

#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "egllog.h"

#include "native_kms.h"

static boolean
kms_surface_validate(struct native_surface *nsurf, uint attachment_mask,
                     unsigned int *seq_num, struct pipe_resource **textures,
                     int *width, int *height)
{
   struct kms_surface *ksurf = kms_surface(nsurf);

   if (!resource_surface_add_resources(ksurf->rsurf, attachment_mask))
      return FALSE;
   if (textures)
      resource_surface_get_resources(ksurf->rsurf, textures, attachment_mask);

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
      struct winsys_handle whandle;
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
         resource_surface_add_resources(ksurf->rsurf, 1 << natt);
         fb->texture =
            resource_surface_get_single_resource(ksurf->rsurf, natt);
         if (!fb->texture)
            return FALSE;
      }

      /* already initialized */
      if (fb->buffer_id)
         continue;

      /* TODO detect the real value */
      fb->is_passive = TRUE;

      memset(&whandle, 0, sizeof(whandle));
      whandle.type = DRM_API_HANDLE_TYPE_KMS;

      if (!kdpy->base.screen->resource_get_handle(kdpy->base.screen,
               fb->texture, &whandle))
         return FALSE;

      block_bits = util_format_get_blocksizebits(ksurf->color_format);
      err = drmModeAddFB(kdpy->fd, ksurf->width, ksurf->height,
            block_bits, block_bits, whandle.stride, whandle.handle,
            &fb->buffer_id);
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
   int err;

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

   resource_surface_swap_buffers(ksurf->rsurf,
         NATIVE_ATTACHMENT_FRONT_LEFT, NATIVE_ATTACHMENT_BACK_LEFT, FALSE);
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

   if (ksurf->current_crtc.crtc)
         drmModeFreeCrtc(ksurf->current_crtc.crtc);

   if (ksurf->front_fb.buffer_id)
      drmModeRmFB(ksurf->kdpy->fd, ksurf->front_fb.buffer_id);
   pipe_resource_reference(&ksurf->front_fb.texture, NULL);

   if (ksurf->back_fb.buffer_id)
      drmModeRmFB(ksurf->kdpy->fd, ksurf->back_fb.buffer_id);
   pipe_resource_reference(&ksurf->back_fb.texture, NULL);

   resource_surface_destroy(ksurf->rsurf);
   FREE(ksurf);
}

static struct kms_surface *
kms_display_create_surface(struct native_display *ndpy,
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
   ksurf->color_format = kconf->base.color_format;
   ksurf->width = width;
   ksurf->height = height;

   ksurf->rsurf = resource_surface_create(kdpy->base.screen,
         ksurf->color_format,
         PIPE_BIND_RENDER_TARGET |
         PIPE_BIND_SAMPLER_VIEW |
         PIPE_BIND_DISPLAY_TARGET |
         PIPE_BIND_SCANOUT);
   if (!ksurf->rsurf) {
      FREE(ksurf);
      return NULL;
   }

   resource_surface_set_size(ksurf->rsurf, ksurf->width, ksurf->height);

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
      FREE(kconn->kms_modes);

      kconn->connector = NULL;
      kconn->kms_modes = NULL;
      kconn->num_modes = 0;
   }

   /* detect again */
   kconn->connector = drmModeGetConnector(kdpy->fd, kconn->connector_id);
   if (!kconn->connector)
      return NULL;

   count = kconn->connector->count_modes;
   kconn->kms_modes = CALLOC(count, sizeof(*kconn->kms_modes));
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

   nmodes_return = MALLOC(count * sizeof(*nmodes_return));
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
         CALLOC(kdpy->resources->count_connectors, sizeof(*kdpy->connectors));
      if (!kdpy->connectors)
         return NULL;

      for (i = 0; i < kdpy->resources->count_connectors; i++) {
         struct kms_connector *kconn = &kdpy->connectors[i];

         kconn->connector_id = kdpy->resources->connectors[i];
         /* kconn->connector is allocated when the modes are asked */
      }

      kdpy->num_connectors = kdpy->resources->count_connectors;
   }

   connectors = MALLOC(kdpy->num_connectors * sizeof(*connectors));
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

   ksurf = kms_display_create_surface(ndpy, nconf, width, height);
   return &ksurf->base;
}

static struct native_display_modeset kms_display_modeset = {
   .get_connectors = kms_display_get_connectors,
   .get_modes = kms_display_get_modes,
   .create_scanout_surface = kms_display_create_scanout_surface,
   .program = kms_display_program
};

void
kms_display_fini_modeset(struct native_display *ndpy)
{
   struct kms_display *kdpy = kms_display(ndpy);
   int i;

   if (kdpy->connectors) {
      for (i = 0; i < kdpy->num_connectors; i++) {
         struct kms_connector *kconn = &kdpy->connectors[i];
         if (kconn->connector) {
            drmModeFreeConnector(kconn->connector);
            FREE(kconn->kms_modes);
         }
      }
      FREE(kdpy->connectors);
   }

   if (kdpy->shown_surfaces) {
      FREE(kdpy->shown_surfaces);
      kdpy->shown_surfaces = NULL;
   }

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
      FREE(kdpy->saved_crtcs);
   }

   if (kdpy->resources) {
      drmModeFreeResources(kdpy->resources);
      kdpy->resources = NULL;
   }

   kdpy->base.modeset = NULL;
}

boolean
kms_display_init_modeset(struct native_display *ndpy)
{
   struct kms_display *kdpy = kms_display(ndpy);

   /* resources are fixed, unlike crtc, connector, or encoder */
   kdpy->resources = drmModeGetResources(kdpy->fd);
   if (!kdpy->resources) {
      _eglLog(_EGL_DEBUG, "Failed to get KMS resources.  Disable modeset.");
      return FALSE;
   }

   kdpy->saved_crtcs =
      CALLOC(kdpy->resources->count_crtcs, sizeof(*kdpy->saved_crtcs));
   if (!kdpy->saved_crtcs) {
      kms_display_fini_modeset(&kdpy->base);
      return FALSE;
   }

   kdpy->shown_surfaces =
      CALLOC(kdpy->resources->count_crtcs, sizeof(*kdpy->shown_surfaces));
   if (!kdpy->shown_surfaces) {
      kms_display_fini_modeset(&kdpy->base);
      return FALSE;
   }

   kdpy->base.modeset = &kms_display_modeset;

   return TRUE;
}
