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

#include "util/u_memory.h"

#include "dri_screen.h"
#include "dri_context.h"
#include "dri_drawable.h"
#include "dri1.h"

static const __DRIextension *dri1_screen_extensions[] = {
   &driReadDrawableExtension,
   &driCopySubBufferExtension.base,
   &driSwapControlExtension.base,
   &driFrameTrackingExtension.base,
   &driMediaStreamCounterExtension.base,
   NULL
};

struct dri1_api *__dri1_api_hooks = NULL;

static INLINE void
dri1_copy_version(struct dri1_api_version *dst,
		  const struct __DRIversionRec *src)
{
   dst->major = src->major;
   dst->minor = src->minor;
   dst->patch_level = src->patch;
}

const __DRIconfig **
dri1_init_screen(__DRIscreen * sPriv)
{
   struct dri_screen *screen;
   const __DRIconfig **configs;
   struct dri1_create_screen_arg arg;

   screen = CALLOC_STRUCT(dri_screen);
   if (!screen)
      return NULL;

   screen->api = drm_api_create();
   screen->sPriv = sPriv;
   screen->fd = sPriv->fd;
   screen->drmLock = (drmLock *) & sPriv->pSAREA->lock;

   sPriv->private = (void *)screen;
   sPriv->extensions = dri1_screen_extensions;

   arg.base.mode = DRM_CREATE_DRI1;
   arg.lf = &dri1_lf;
   arg.ddx_info = sPriv->pDevPriv;
   arg.ddx_info_size = sPriv->devPrivSize;
   arg.sarea = sPriv->pSAREA;
   dri1_copy_version(&arg.ddx_version, &sPriv->ddx_version);
   dri1_copy_version(&arg.dri_version, &sPriv->dri_version);
   dri1_copy_version(&arg.drm_version, &sPriv->drm_version);
   arg.api = NULL;

   screen->pipe_screen = screen->api->create_screen(screen->api, screen->fd, &arg.base);

   if (!screen->pipe_screen || !arg.api) {
      debug_printf("%s: failed to create dri1 screen\n", __FUNCTION__);
      goto out_no_screen;
   }

   __dri1_api_hooks = arg.api;

   screen->pipe_screen->flush_frontbuffer = dri1_flush_frontbuffer;
   driParseOptionInfo(&screen->optionCache,
		      __driConfigOptions, __driNConfigOptions);

   /**
    * FIXME: If the driver supports format conversion swapbuffer blits, we might
    * want to support other color bit depths than the server is currently
    * using.
    */

   configs = dri_fill_in_modes(screen, sPriv->fbBPP);
   if (!configs)
      goto out_no_configs;

   return configs;
 out_no_configs:
   screen->pipe_screen->destroy(screen->pipe_screen);
 out_no_screen:
   FREE(screen);
   return NULL;
}
