/**************************************************************************
 *
 * Copyright 2009, VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/
/*
 * Author: Keith Whitwell <keithw@vmware.com>
 * Author: Jakob Bornecrantz <wallbraker@gmail.com>
 */

#ifndef DRI_SCREEN_H
#define DRI_SCREEN_H

#include "dri_util.h"
#include "xmlconfig.h"

#include "pipe/p_compiler.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "state_tracker/st_api.h"
#include "postprocess/filters.h"

struct dri_context;
struct dri_drawable;
struct pipe_loader_device;

struct dri_screen
{
   /* st_api */
   struct st_manager base;
   struct st_api *st_api;

   /* on old libGL's invalidate doesn't get called as it should */
   boolean broken_invalidate;

   /* dri */
   __DRIscreen *sPriv;
   boolean throttling_enabled;
   int default_throttle_frames;

   /** Configuration cache with default values for all contexts */
   driOptionCache optionCacheDefaults;

   /** The screen's effective configuration options */
   driOptionCache optionCache;

   struct st_config_options options;

   /* Which postprocessing filters are enabled. */
   unsigned pp_enabled[PP_FILTERS];

   /* drm */
   int fd;
   boolean can_share_buffer;

   struct pipe_loader_device *dev;

   /* gallium */
   boolean d_depth_bits_last;
   boolean sd_depth_bits_last;
   boolean auto_fake_front;
   enum pipe_texture_target target;

   /* hooks filled in by dri2 & drisw */
   __DRIimage * (*lookup_egl_image)(struct dri_screen *ctx, void *handle);
};

/** cast wrapper */
static INLINE struct dri_screen *
dri_screen(__DRIscreen * sPriv)
{
   return (struct dri_screen *)sPriv->driverPrivate;
}

struct __DRIimageRec {
   struct pipe_resource *texture;
   unsigned level;
   unsigned layer;
   uint32_t dri_format;
   uint32_t dri_components;

   void *loader_private;

   /**
    * Provided by EGL_EXT_image_dma_buf_import.
    */
   enum __DRIYUVColorSpace yuv_color_space;
   enum __DRISampleRange sample_range;
   enum __DRIChromaSiting horizontal_siting;
   enum __DRIChromaSiting vertical_siting;

};

#ifndef __NOT_HAVE_DRM_H

static INLINE boolean
dri_with_format(__DRIscreen * sPriv)
{
   const __DRIdri2LoaderExtension *loader = sPriv->dri2.loader;

   return loader
       && (loader->base.version >= 3)
       && (loader->getBuffersWithFormat != NULL);
}

#else

static INLINE boolean
dri_with_format(__DRIscreen * sPriv)
{
   return TRUE;
}

#endif

void
dri_fill_st_visual(struct st_visual *stvis, struct dri_screen *screen,
                   const struct gl_config *mode);

const __DRIconfig **
dri_init_screen_helper(struct dri_screen *screen,
                       struct pipe_screen *pscreen,
                       const char* driver_name);

void
dri_destroy_screen_helper(struct dri_screen * screen);

void
dri_destroy_screen(__DRIscreen * sPriv);

extern struct pipe_screen *kms_swrast_create_screen(int fd);
extern const struct __DriverAPIRec dri_kms_driver_api;

extern const struct __DriverAPIRec galliumdrm_driver_api;
extern const __DRIextension *galliumdrm_driver_extensions[];
extern const struct __DriverAPIRec galliumsw_driver_api;
extern const __DRIextension *galliumsw_driver_extensions[];
extern const __DRIconfigOptionsExtension gallium_config_options;

#endif

/* vim: set sw=3 ts=8 sts=3 expandtab: */
