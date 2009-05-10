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

#include "utils.h"
#include "vblank.h"
#include "xmlpool.h"

#include "dri_screen.h"
#include "dri_context.h"
#include "dri_drawable.h"

#include "pipe/p_context.h"
#include "pipe/p_screen.h"
#include "pipe/p_inlines.h"
#include "pipe/p_format.h"
#include "state_tracker/drm_api.h"
#include "state_tracker/dri1_api.h"
#include "state_tracker/st_public.h"
#include "state_tracker/st_cb_fbo.h"

PUBLIC const char __driConfigOptions[] =
   DRI_CONF_BEGIN DRI_CONF_SECTION_PERFORMANCE
   DRI_CONF_FTHROTTLE_MODE(DRI_CONF_FTHROTTLE_IRQS)
   DRI_CONF_VBLANK_MODE(DRI_CONF_VBLANK_DEF_INTERVAL_0)
   DRI_CONF_SECTION_END DRI_CONF_SECTION_QUALITY
    /*DRI_CONF_FORCE_S3TC_ENABLE(false) */
   DRI_CONF_ALLOW_LARGE_TEXTURES(1)
   DRI_CONF_SECTION_END DRI_CONF_END;

   const uint __driNConfigOptions = 3;

   static const __DRIextension *dri_screen_extensions[] = {
      &driReadDrawableExtension,
      &driCopySubBufferExtension.base,
      &driSwapControlExtension.base,
      &driFrameTrackingExtension.base,
      &driMediaStreamCounterExtension.base,
      NULL
   };

struct dri1_api *__dri1_api_hooks = NULL;

static const __DRIconfig **
dri_fill_in_modes(__DRIscreenPrivate * psp,
		  unsigned pixel_bits, unsigned depth_bits,
		  unsigned stencil_bits, GLboolean have_back_buffer)
{
   __DRIconfig **configs;
   __GLcontextModes *m;
   unsigned num_modes;
   uint8_t depth_bits_array[3];
   uint8_t stencil_bits_array[3];
   uint8_t msaa_samples_array[1];
   unsigned depth_buffer_factor;
   unsigned back_buffer_factor;
   unsigned msaa_samples_factor;
   GLenum fb_format;
   GLenum fb_type;
   int i;

   static const GLenum back_buffer_modes[] = {
      GLX_NONE, GLX_SWAP_UNDEFINED_OML, GLX_SWAP_COPY_OML
   };

   /* TODO probe the hardware of what is supports */
   depth_bits_array[0] = 0;
   depth_bits_array[1] = 24;
   depth_bits_array[2] = 24;

   stencil_bits_array[0] = 0;	       /* no depth or stencil */
   stencil_bits_array[1] = 0;	       /* z24x8 */
   stencil_bits_array[2] = 8;	       /* z24s8 */

   msaa_samples_array[0] = 0;

   depth_buffer_factor = 3;
   back_buffer_factor = 3;
   msaa_samples_factor = 1;

   num_modes =
      depth_buffer_factor * back_buffer_factor * msaa_samples_factor * 4;

   if (pixel_bits == 16) {
      fb_format = GL_RGB;
      fb_type = GL_UNSIGNED_SHORT_5_6_5;
   } else {
      fb_format = GL_BGRA;
      fb_type = GL_UNSIGNED_INT_8_8_8_8_REV;
   }

   configs = driCreateConfigs(fb_format, fb_type,
			      depth_bits_array,
			      stencil_bits_array, depth_buffer_factor,
			      back_buffer_modes, back_buffer_factor,
			      msaa_samples_array, msaa_samples_factor);
   if (configs == NULL) {
      debug_printf("%s: driCreateConfigs failed\n", __FUNCTION__);
      return NULL;
   }

   for (i = 0; configs[i]; i++) {
      m = &configs[i]->modes;
      if ((m->stencilBits != 0) && (m->stencilBits != stencil_bits)) {
	 m->visualRating = GLX_SLOW_CONFIG;
      }
   }

   return (const const __DRIconfig **)configs;
}

/**
 * Get information about previous buffer swaps.
 */
static int
dri_get_swap_info(__DRIdrawablePrivate * dPriv, __DRIswapInfo * sInfo)
{
   if (dPriv == NULL || dPriv->driverPrivate == NULL || sInfo == NULL)
      return -1;
   else
      return 0;
}

static INLINE void
dri_copy_version(struct dri1_api_version *dst,
		 const struct __DRIversionRec *src)
{
   dst->major = src->major;
   dst->minor = src->minor;
   dst->patch_level = src->patch;
}

static const __DRIconfig **
dri_init_screen(__DRIscreenPrivate * sPriv)
{
   struct dri_screen *screen;
   const __DRIconfig **configs;
   struct dri1_create_screen_arg arg;

   dri_init_extensions(NULL);

   screen = CALLOC_STRUCT(dri_screen);
   if (!screen)
      return NULL;

   screen->sPriv = sPriv;
   screen->fd = sPriv->fd;
   screen->drmLock = (drmLock *) & sPriv->pSAREA->lock;

   sPriv->private = (void *)screen;
   sPriv->extensions = dri_screen_extensions;

   arg.base.mode = DRM_CREATE_DRI1;
   arg.lf = &dri1_lf;
   arg.ddx_info = sPriv->pDevPriv;
   arg.ddx_info_size = sPriv->devPrivSize;
   arg.sarea = sPriv->pSAREA;
   dri_copy_version(&arg.ddx_version, &sPriv->ddx_version);
   dri_copy_version(&arg.dri_version, &sPriv->dri_version);
   dri_copy_version(&arg.drm_version, &sPriv->drm_version);
   arg.api = NULL;

   screen->pipe_screen = drm_api_hooks.create_screen(screen->fd, &arg.base);

   if (!screen->pipe_screen || !arg.api) {
      debug_printf("%s: failed to create dri1 screen\n", __FUNCTION__);
      goto out_no_screen;
   }

   __dri1_api_hooks = arg.api;

   screen->pipe_screen->flush_frontbuffer = dri1_flush_frontbuffer;
   driParseOptionInfo(&screen->optionCache,
		      __driConfigOptions, __driNConfigOptions);

   configs = dri_fill_in_modes(sPriv, sPriv->fbBPP, 24, 8, 1);
   if (!configs)
      goto out_no_configs;

   return configs;
 out_no_configs:
   screen->pipe_screen->destroy(screen->pipe_screen);
 out_no_screen:
   FREE(screen);
   return NULL;
}

/**
 * This is the driver specific part of the createNewScreen entry point.
 *
 * Returns the __GLcontextModes supported by this driver.
 */
static const __DRIconfig **
dri_init_screen2(__DRIscreenPrivate * sPriv)
{
   struct dri_screen *screen;
   struct drm_create_screen_arg arg;

   /* Set up dispatch table to cope with all known extensions */
   dri_init_extensions(NULL);

   screen = CALLOC_STRUCT(dri_screen);
   if (!screen)
      goto fail;

   screen->sPriv = sPriv;
   screen->fd = sPriv->fd;
   sPriv->private = (void *)screen;
   sPriv->extensions = dri_screen_extensions;
   arg.mode = DRM_CREATE_NORMAL;

   screen->pipe_screen = drm_api_hooks.create_screen(screen->fd, &arg);
   if (!screen->pipe_screen) {
      debug_printf("%s: failed to create pipe_screen\n", __FUNCTION__);
      goto fail;
   }

   /* We need to hook in here */
   screen->pipe_screen->flush_frontbuffer = dri_flush_frontbuffer;

   driParseOptionInfo(&screen->optionCache,
		      __driConfigOptions, __driNConfigOptions);

   return dri_fill_in_modes(sPriv, 4 * 8, 24, 8, 1);
 fail:
   return NULL;
}

static void
dri_destroy_screen(__DRIscreenPrivate * sPriv)
{
   struct dri_screen *screen = dri_screen(sPriv);

   screen->pipe_screen->destroy(screen->pipe_screen);
   FREE(screen);
   sPriv->private = NULL;
}

PUBLIC const struct __DriverAPIRec driDriverAPI = {
   .InitScreen = dri_init_screen,
   .DestroyScreen = dri_destroy_screen,
   .CreateContext = dri_create_context,
   .DestroyContext = dri_destroy_context,
   .CreateBuffer = dri_create_buffer,
   .DestroyBuffer = dri_destroy_buffer,
   .SwapBuffers = dri_swap_buffers,
   .MakeCurrent = dri_make_current,
   .UnbindContext = dri_unbind_context,
   .GetSwapInfo = dri_get_swap_info,
   .GetDrawableMSC = driDrawableGetMSC32,
   .WaitForMSC = driWaitForMSC32,
   .CopySubBuffer = dri_copy_sub_buffer,
   .InitScreen = dri_init_screen,
   .InitScreen2 = dri_init_screen2,
};

/* vim: set sw=3 ts=8 sts=3 expandtab: */
