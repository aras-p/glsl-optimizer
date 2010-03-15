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

#include "pipe/p_screen.h"
#include "pipe/p_format.h"
#include "state_tracker/drm_api.h"
#include "state_tracker/dri1_api.h"

#include "util/u_debug.h"

PUBLIC const char __driConfigOptions[] =
   DRI_CONF_BEGIN DRI_CONF_SECTION_PERFORMANCE
   DRI_CONF_FTHROTTLE_MODE(DRI_CONF_FTHROTTLE_IRQS)
   DRI_CONF_VBLANK_MODE(DRI_CONF_VBLANK_DEF_INTERVAL_0)
   DRI_CONF_SECTION_END DRI_CONF_SECTION_QUALITY
    /*DRI_CONF_FORCE_S3TC_ENABLE(false) */
   DRI_CONF_ALLOW_LARGE_TEXTURES(1)
   DRI_CONF_SECTION_END DRI_CONF_END;

   const uint __driNConfigOptions = 3;

static const __DRItexBufferExtension dri2TexBufferExtension = {
    { __DRI_TEX_BUFFER, __DRI_TEX_BUFFER_VERSION },
   dri2_set_tex_buffer,
   dri2_set_tex_buffer2,
};

static void
dri2_flush_drawable(__DRIdrawable *draw)
{
}

static const __DRI2flushExtension dri2FlushExtension = {
    { __DRI2_FLUSH, __DRI2_FLUSH_VERSION },
    dri2_flush_drawable,
    dri2InvalidateDrawable,
};

   static const __DRIextension *dri_screen_extensions[] = {
      &driReadDrawableExtension,
      &driCopySubBufferExtension.base,
      &driSwapControlExtension.base,
      &driFrameTrackingExtension.base,
      &driMediaStreamCounterExtension.base,
      &dri2TexBufferExtension.base,
      &dri2FlushExtension.base,
      NULL
   };

struct dri1_api *__dri1_api_hooks = NULL;

static const __DRIconfig **
dri_fill_in_modes(struct dri_screen *screen,
		  unsigned pixel_bits)
{
   __DRIconfig **configs = NULL;
   __DRIconfig **configs_r5g6b5 = NULL;
   __DRIconfig **configs_a8r8g8b8 = NULL;
   __DRIconfig **configs_x8r8g8b8 = NULL;
   unsigned num_modes;
   uint8_t depth_bits_array[5];
   uint8_t stencil_bits_array[5];
   uint8_t msaa_samples_array[2];
   unsigned depth_buffer_factor;
   unsigned back_buffer_factor;
   unsigned msaa_samples_factor;
   struct pipe_screen *p_screen = screen->pipe_screen;
   boolean pf_r5g6b5, pf_a8r8g8b8, pf_x8r8g8b8;
   boolean pf_z16, pf_x8z24, pf_z24x8, pf_s8z24, pf_z24s8, pf_z32;

   static const GLenum back_buffer_modes[] = {
      GLX_NONE, GLX_SWAP_UNDEFINED_OML, GLX_SWAP_COPY_OML
   };

   depth_bits_array[0] = 0;
   stencil_bits_array[0] = 0;
   depth_buffer_factor = 1;

   pf_x8z24 = p_screen->is_format_supported(p_screen, PIPE_FORMAT_Z24X8_UNORM,
					    PIPE_TEXTURE_2D,
					    PIPE_TEXTURE_USAGE_DEPTH_STENCIL, 0);
   pf_z24x8 = p_screen->is_format_supported(p_screen, PIPE_FORMAT_X8Z24_UNORM,
					    PIPE_TEXTURE_2D,
					    PIPE_TEXTURE_USAGE_DEPTH_STENCIL, 0);
   pf_s8z24 = p_screen->is_format_supported(p_screen, PIPE_FORMAT_Z24S8_UNORM,
					    PIPE_TEXTURE_2D,
					    PIPE_TEXTURE_USAGE_DEPTH_STENCIL, 0);
   pf_z24s8 = p_screen->is_format_supported(p_screen, PIPE_FORMAT_S8Z24_UNORM,
					    PIPE_TEXTURE_2D,
					    PIPE_TEXTURE_USAGE_DEPTH_STENCIL, 0);
   pf_a8r8g8b8 = p_screen->is_format_supported(p_screen, PIPE_FORMAT_B8G8R8A8_UNORM,
					       PIPE_TEXTURE_2D,
					       PIPE_TEXTURE_USAGE_RENDER_TARGET, 0);
   pf_x8r8g8b8 = p_screen->is_format_supported(p_screen, PIPE_FORMAT_B8G8R8X8_UNORM,
					       PIPE_TEXTURE_2D,
					       PIPE_TEXTURE_USAGE_RENDER_TARGET, 0);
   pf_r5g6b5 = p_screen->is_format_supported(p_screen, PIPE_FORMAT_B5G6R5_UNORM,
					     PIPE_TEXTURE_2D,
					     PIPE_TEXTURE_USAGE_RENDER_TARGET, 0);

   /* We can only get a 16 or 32 bit depth buffer with getBuffersWithFormat */
   if (screen->sPriv->dri2.loader &&
       (screen->sPriv->dri2.loader->base.version > 2) &&
       (screen->sPriv->dri2.loader->getBuffersWithFormat != NULL)) {
      pf_z16 = p_screen->is_format_supported(p_screen, PIPE_FORMAT_Z16_UNORM,
                                             PIPE_TEXTURE_2D,
                                             PIPE_TEXTURE_USAGE_DEPTH_STENCIL, 0);
      pf_z32 = p_screen->is_format_supported(p_screen, PIPE_FORMAT_Z32_UNORM,
                                             PIPE_TEXTURE_2D,
                                             PIPE_TEXTURE_USAGE_DEPTH_STENCIL, 0);
   } else {
      pf_z16 = FALSE;
      pf_z32 = FALSE;
   }

   if (pf_z16) {
      depth_bits_array[depth_buffer_factor] = 16;
      stencil_bits_array[depth_buffer_factor++] = 0;
   }
   if (pf_x8z24 || pf_z24x8) {
      depth_bits_array[depth_buffer_factor] = 24;
      stencil_bits_array[depth_buffer_factor++] = 0;
      screen->d_depth_bits_last = pf_x8z24;
   }
   if (pf_s8z24 || pf_z24s8) {
      depth_bits_array[depth_buffer_factor] = 24;
      stencil_bits_array[depth_buffer_factor++] = 8;
      screen->sd_depth_bits_last = pf_s8z24;
   }
   if (pf_z32) {
      depth_bits_array[depth_buffer_factor] = 32;
      stencil_bits_array[depth_buffer_factor++] = 0;
   }

   msaa_samples_array[0] = 0;
   msaa_samples_array[1] = 4;
   back_buffer_factor = 3;
   msaa_samples_factor = 2;

   num_modes =
      depth_buffer_factor * back_buffer_factor * msaa_samples_factor * 4;

   if (pf_r5g6b5)
      configs_r5g6b5 = driCreateConfigs(GL_RGB, GL_UNSIGNED_SHORT_5_6_5,
                                        depth_bits_array, stencil_bits_array,
                                        depth_buffer_factor, back_buffer_modes,
                                        back_buffer_factor,
                                        msaa_samples_array, msaa_samples_factor,
                                        GL_TRUE);

   if (pf_a8r8g8b8)
      configs_a8r8g8b8 = driCreateConfigs(GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
                                          depth_bits_array,
                                          stencil_bits_array,
                                          depth_buffer_factor,
                                          back_buffer_modes,
                                          back_buffer_factor,
                                          msaa_samples_array,
                                          msaa_samples_factor,
                                          GL_TRUE);

   if (pf_x8r8g8b8)
      configs_x8r8g8b8 = driCreateConfigs(GL_BGR, GL_UNSIGNED_INT_8_8_8_8_REV,
                                          depth_bits_array,
                                          stencil_bits_array,
                                          depth_buffer_factor,
                                          back_buffer_modes,
                                          back_buffer_factor,
                                          msaa_samples_array,
                                          msaa_samples_factor,
                                          GL_TRUE);

   if (pixel_bits == 16) {
      configs = configs_r5g6b5;
      if (configs_a8r8g8b8)
         configs = configs ? driConcatConfigs(configs, configs_a8r8g8b8) : configs_a8r8g8b8;
      if (configs_x8r8g8b8)
	 configs = configs ? driConcatConfigs(configs, configs_x8r8g8b8) : configs_x8r8g8b8;
   } else {
      configs = configs_a8r8g8b8;
      if (configs_x8r8g8b8)
	 configs = configs ? driConcatConfigs(configs, configs_x8r8g8b8) : configs_x8r8g8b8;
      if (configs_r5g6b5)
         configs = configs ? driConcatConfigs(configs, configs_r5g6b5) : configs_r5g6b5;
   }

   if (configs == NULL) {
      debug_printf("%s: driCreateConfigs failed\n", __FUNCTION__);
      return NULL;
   }

   return (const __DRIconfig **)configs;
}

/**
 * Get information about previous buffer swaps.
 */
static int
dri_get_swap_info(__DRIdrawable * dPriv, __DRIswapInfo * sInfo)
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
dri_init_screen(__DRIscreen * sPriv)
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

/**
 * This is the driver specific part of the createNewScreen entry point.
 *
 * Returns the __GLcontextModes supported by this driver.
 */
static const __DRIconfig **
dri_init_screen2(__DRIscreen * sPriv)
{
   struct dri_screen *screen;
   struct drm_create_screen_arg arg;
   const __DRIdri2LoaderExtension *dri2_ext =
     sPriv->dri2.loader;

   screen = CALLOC_STRUCT(dri_screen);
   if (!screen)
      goto fail;

   screen->api = drm_api_create();
   screen->sPriv = sPriv;
   screen->fd = sPriv->fd;
   sPriv->private = (void *)screen;
   sPriv->extensions = dri_screen_extensions;
   arg.mode = DRM_CREATE_NORMAL;

   screen->pipe_screen = screen->api->create_screen(screen->api, screen->fd, &arg);
   if (!screen->pipe_screen) {
      debug_printf("%s: failed to create pipe_screen\n", __FUNCTION__);
      goto fail;
   }

   /* We need to hook in here */
   screen->pipe_screen->update_buffer = dri_update_buffer;
   screen->pipe_screen->flush_frontbuffer = dri_flush_frontbuffer;

   driParseOptionInfo(&screen->optionCache,
		      __driConfigOptions, __driNConfigOptions);

   screen->auto_fake_front = dri2_ext->base.version >= 3 &&
      dri2_ext->getBuffersWithFormat != NULL;

   return dri_fill_in_modes(screen, 32);
 fail:
   return NULL;
}

static void
dri_destroy_screen(__DRIscreen * sPriv)
{
   struct dri_screen *screen = dri_screen(sPriv);
   int i;

   screen->pipe_screen->destroy(screen->pipe_screen);
   
   for (i = 0; i < (1 << screen->optionCache.tableSize); ++i) {
      FREE(screen->optionCache.info[i].name);
      FREE(screen->optionCache.info[i].ranges);
   }

   FREE(screen->optionCache.info);
   FREE(screen->optionCache.values);

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

/* This is the table of extensions that the loader will dlsym() for. */
PUBLIC const __DRIextension *__driDriverExtensions[] = {
    &driCoreExtension.base,
    &driLegacyExtension.base,
    &driDRI2Extension.base,
    NULL
};

/* vim: set sw=3 ts=8 sts=3 expandtab: */
