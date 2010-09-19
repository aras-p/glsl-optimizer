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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef _NATIVE_KMS_H_
#define _NATIVE_KMS_H_

#include <xf86drm.h>
#include <xf86drmMode.h>

#include "pipe/p_compiler.h"
#include "util/u_format.h"
#include "pipe/p_state.h"
#include "state_tracker/drm_driver.h"

#include "common/native.h"
#include "common/native_helper.h"

struct kms_config;
struct kms_crtc;
struct kms_connector;
struct kms_mode;
struct kms_surface;

struct kms_display {
   struct native_display base;

   struct native_event_handler *event_handler;

   int fd;
   struct kms_config *config;

   /* for modesetting */
   drmModeResPtr resources;
   struct kms_connector *connectors;
   int num_connectors;

   struct kms_surface **shown_surfaces;
   /* save the original settings of the CRTCs */
   struct kms_crtc *saved_crtcs;
};

struct kms_config {
   struct native_config base;
};

struct kms_crtc {
   drmModeCrtcPtr crtc;
   uint32_t connectors[32];
   int num_connectors;
};

struct kms_framebuffer {
   struct pipe_resource *texture;
   boolean is_passive;

   uint32_t buffer_id;
};

struct kms_surface {
   struct native_surface base;
   struct kms_display *kdpy;

   struct resource_surface *rsurf;
   enum pipe_format color_format;
   int width, height;

   unsigned int sequence_number;
   struct kms_framebuffer front_fb, back_fb;

   boolean is_shown;
   struct kms_crtc current_crtc;
};

struct kms_connector {
   struct native_connector base;

   uint32_t connector_id;
   drmModeConnectorPtr connector;
   struct kms_mode *kms_modes;
   int num_modes;
};

struct kms_mode {
   struct native_mode base;
   drmModeModeInfo mode;
};

static INLINE struct kms_display *
kms_display(const struct native_display *ndpy)
{
   return (struct kms_display *) ndpy;
}

static INLINE struct kms_config *
kms_config(const struct native_config *nconf)
{
   return (struct kms_config *) nconf;
}

static INLINE struct kms_surface *
kms_surface(const struct native_surface *nsurf)
{
   return (struct kms_surface *) nsurf;
}

static INLINE struct kms_connector *
kms_connector(const struct native_connector *nconn)
{
   return (struct kms_connector *) nconn;
}

static INLINE struct kms_mode *
kms_mode(const struct native_mode *nmode)
{
   return (struct kms_mode *) nmode;
}

boolean
kms_display_init_modeset(struct native_display *ndpy);

void
kms_display_fini_modeset(struct native_display *ndpy);

#endif /* _NATIVE_KMS_H_ */
