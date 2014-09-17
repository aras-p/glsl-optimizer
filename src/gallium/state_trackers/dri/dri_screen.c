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
#include "xmlpool.h"

#include "dri_screen.h"

#include "util/u_inlines.h"
#include "pipe/p_screen.h"
#include "pipe/p_format.h"
#include "pipe-loader/pipe_loader.h"
#include "state_tracker/st_gl_api.h" /* for st_gl_api_create */
#include "state_tracker/drm_driver.h"

#include "util/u_debug.h"
#include "util/u_format_s3tc.h"

#define MSAA_VISUAL_MAX_SAMPLES 32

#undef false

const __DRIconfigOptionsExtension gallium_config_options = {
   .base = { __DRI_CONFIG_OPTIONS, 1 },
   .xml =

   DRI_CONF_BEGIN
      DRI_CONF_SECTION_QUALITY
         DRI_CONF_FORCE_S3TC_ENABLE("false")
         DRI_CONF_PP_CELSHADE(0)
         DRI_CONF_PP_NORED(0)
         DRI_CONF_PP_NOGREEN(0)
         DRI_CONF_PP_NOBLUE(0)
         DRI_CONF_PP_JIMENEZMLAA(0, 0, 32)
         DRI_CONF_PP_JIMENEZMLAA_COLOR(0, 0, 32)
      DRI_CONF_SECTION_END

      DRI_CONF_SECTION_DEBUG
         DRI_CONF_FORCE_GLSL_EXTENSIONS_WARN("false")
         DRI_CONF_DISABLE_GLSL_LINE_CONTINUATIONS("false")
         DRI_CONF_DISABLE_BLEND_FUNC_EXTENDED("false")
         DRI_CONF_DISABLE_SHADER_BIT_ENCODING("false")
         DRI_CONF_FORCE_GLSL_VERSION(0)
         DRI_CONF_ALLOW_GLSL_EXTENSION_DIRECTIVE_MIDSHADER("false")
      DRI_CONF_SECTION_END

      DRI_CONF_SECTION_MISCELLANEOUS
         DRI_CONF_ALWAYS_HAVE_DEPTH_BUFFER("false")
      DRI_CONF_SECTION_END
   DRI_CONF_END
};

#define false 0

static void
dri_fill_st_options(struct st_config_options *options,
                    const struct driOptionCache * optionCache)
{
   options->disable_blend_func_extended =
      driQueryOptionb(optionCache, "disable_blend_func_extended");
   options->disable_glsl_line_continuations =
      driQueryOptionb(optionCache, "disable_glsl_line_continuations");
   options->disable_shader_bit_encoding =
      driQueryOptionb(optionCache, "disable_shader_bit_encoding");
   options->force_glsl_extensions_warn =
      driQueryOptionb(optionCache, "force_glsl_extensions_warn");
   options->force_glsl_version =
      driQueryOptioni(optionCache, "force_glsl_version");
   options->force_s3tc_enable =
      driQueryOptionb(optionCache, "force_s3tc_enable");
   options->allow_glsl_extension_directive_midshader =
      driQueryOptionb(optionCache, "allow_glsl_extension_directive_midshader");
}

static const __DRIconfig **
dri_fill_in_modes(struct dri_screen *screen)
{
   static const mesa_format mesa_formats[3] = {
      MESA_FORMAT_B8G8R8A8_UNORM,
      MESA_FORMAT_B8G8R8X8_UNORM,
      MESA_FORMAT_B5G6R5_UNORM,
   };
   static const enum pipe_format pipe_formats[3] = {
      PIPE_FORMAT_BGRA8888_UNORM,
      PIPE_FORMAT_BGRX8888_UNORM,
      PIPE_FORMAT_B5G6R5_UNORM,
   };
   mesa_format format;
   __DRIconfig **configs = NULL;
   uint8_t depth_bits_array[5];
   uint8_t stencil_bits_array[5];
   unsigned depth_buffer_factor;
   unsigned msaa_samples_max;
   unsigned i;
   struct pipe_screen *p_screen = screen->base.screen;
   boolean pf_z16, pf_x8z24, pf_z24x8, pf_s8z24, pf_z24s8, pf_z32;

   static const GLenum back_buffer_modes[] = {
      GLX_NONE, GLX_SWAP_UNDEFINED_OML, GLX_SWAP_COPY_OML
   };

   if (driQueryOptionb(&screen->optionCache, "always_have_depth_buffer")) {
      /* all visuals will have a depth buffer */
      depth_buffer_factor = 0;
   }
   else {
      depth_bits_array[0] = 0;
      stencil_bits_array[0] = 0;
      depth_buffer_factor = 1;
   }
 
   msaa_samples_max = (screen->st_api->feature_mask & ST_API_FEATURE_MS_VISUALS_MASK)
      ? MSAA_VISUAL_MAX_SAMPLES : 1;

   pf_x8z24 = p_screen->is_format_supported(p_screen, PIPE_FORMAT_Z24X8_UNORM,
					    PIPE_TEXTURE_2D, 0,
                                            PIPE_BIND_DEPTH_STENCIL);
   pf_z24x8 = p_screen->is_format_supported(p_screen, PIPE_FORMAT_X8Z24_UNORM,
					    PIPE_TEXTURE_2D, 0,
                                            PIPE_BIND_DEPTH_STENCIL);
   pf_s8z24 = p_screen->is_format_supported(p_screen, PIPE_FORMAT_Z24_UNORM_S8_UINT,
					    PIPE_TEXTURE_2D, 0,
                                            PIPE_BIND_DEPTH_STENCIL);
   pf_z24s8 = p_screen->is_format_supported(p_screen, PIPE_FORMAT_S8_UINT_Z24_UNORM,
					    PIPE_TEXTURE_2D, 0,
                                            PIPE_BIND_DEPTH_STENCIL);
   pf_z16 = p_screen->is_format_supported(p_screen, PIPE_FORMAT_Z16_UNORM,
                                          PIPE_TEXTURE_2D, 0,
                                          PIPE_BIND_DEPTH_STENCIL);
   pf_z32 = p_screen->is_format_supported(p_screen, PIPE_FORMAT_Z32_UNORM,
                                          PIPE_TEXTURE_2D, 0,
                                          PIPE_BIND_DEPTH_STENCIL);

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

   assert(Elements(mesa_formats) == Elements(pipe_formats));

   /* Add configs. */
   for (format = 0; format < Elements(mesa_formats); format++) {
      __DRIconfig **new_configs = NULL;
      unsigned num_msaa_modes = 0; /* includes a single-sample mode */
      uint8_t msaa_modes[MSAA_VISUAL_MAX_SAMPLES];

      for (i = 1; i <= msaa_samples_max; i++) {
         int samples = i > 1 ? i : 0;

         if (p_screen->is_format_supported(p_screen, pipe_formats[format],
                                           PIPE_TEXTURE_2D, samples,
                                           PIPE_BIND_RENDER_TARGET)) {
            msaa_modes[num_msaa_modes++] = samples;
         }
      }

      if (num_msaa_modes) {
         /* Single-sample configs with an accumulation buffer. */
         new_configs = driCreateConfigs(mesa_formats[format],
                                        depth_bits_array, stencil_bits_array,
                                        depth_buffer_factor, back_buffer_modes,
                                        Elements(back_buffer_modes),
                                        msaa_modes, 1,
                                        GL_TRUE);
         configs = driConcatConfigs(configs, new_configs);

         /* Multi-sample configs without an accumulation buffer. */
         if (num_msaa_modes > 1) {
            new_configs = driCreateConfigs(mesa_formats[format],
                                           depth_bits_array, stencil_bits_array,
                                           depth_buffer_factor, back_buffer_modes,
                                           Elements(back_buffer_modes),
                                           msaa_modes+1, num_msaa_modes-1,
                                           GL_FALSE);
            configs = driConcatConfigs(configs, new_configs);
         }
      }
   }

   if (configs == NULL) {
      debug_printf("%s: driCreateConfigs failed\n", __FUNCTION__);
      return NULL;
   }

   return (const __DRIconfig **)configs;
}

/**
 * Roughly the converse of dri_fill_in_modes.
 */
void
dri_fill_st_visual(struct st_visual *stvis, struct dri_screen *screen,
                   const struct gl_config *mode)
{
   memset(stvis, 0, sizeof(*stvis));

   if (!mode)
      return;

   if (mode->redBits == 8) {
      if (mode->alphaBits == 8)
         stvis->color_format = PIPE_FORMAT_BGRA8888_UNORM;
      else
         stvis->color_format = PIPE_FORMAT_BGRX8888_UNORM;
   } else {
      stvis->color_format = PIPE_FORMAT_B5G6R5_UNORM;
   }

   if (mode->sampleBuffers) {
      stvis->samples = mode->samples;
   }

   switch (mode->depthBits) {
   default:
   case 0:
      stvis->depth_stencil_format = PIPE_FORMAT_NONE;
      break;
   case 16:
      stvis->depth_stencil_format = PIPE_FORMAT_Z16_UNORM;
      break;
   case 24:
      if (mode->stencilBits == 0) {
	 stvis->depth_stencil_format = (screen->d_depth_bits_last) ?
                                          PIPE_FORMAT_Z24X8_UNORM:
                                          PIPE_FORMAT_X8Z24_UNORM;
      } else {
	 stvis->depth_stencil_format = (screen->sd_depth_bits_last) ?
                                          PIPE_FORMAT_Z24_UNORM_S8_UINT:
                                          PIPE_FORMAT_S8_UINT_Z24_UNORM;
      }
      break;
   case 32:
      stvis->depth_stencil_format = PIPE_FORMAT_Z32_UNORM;
      break;
   }

   stvis->accum_format = (mode->haveAccumBuffer) ?
      PIPE_FORMAT_R16G16B16A16_SNORM : PIPE_FORMAT_NONE;

   stvis->buffer_mask |= ST_ATTACHMENT_FRONT_LEFT_MASK;
   stvis->render_buffer = ST_ATTACHMENT_FRONT_LEFT;
   if (mode->doubleBufferMode) {
      stvis->buffer_mask |= ST_ATTACHMENT_BACK_LEFT_MASK;
      stvis->render_buffer = ST_ATTACHMENT_BACK_LEFT;
   }
   if (mode->stereoMode) {
      stvis->buffer_mask |= ST_ATTACHMENT_FRONT_RIGHT_MASK;
      if (mode->doubleBufferMode)
         stvis->buffer_mask |= ST_ATTACHMENT_BACK_RIGHT_MASK;
   }

   if (mode->haveDepthBuffer || mode->haveStencilBuffer)
      stvis->buffer_mask |= ST_ATTACHMENT_DEPTH_STENCIL_MASK;
   /* let the state tracker allocate the accum buffer */
}

static boolean
dri_get_egl_image(struct st_manager *smapi,
                  void *egl_image,
                  struct st_egl_image *stimg)
{
   struct dri_screen *screen = (struct dri_screen *)smapi;
   __DRIimage *img = NULL;

   if (screen->lookup_egl_image) {
      img = screen->lookup_egl_image(screen, egl_image);
   }

   if (!img)
      return FALSE;

   stimg->texture = NULL;
   pipe_resource_reference(&stimg->texture, img->texture);
   stimg->level = img->level;
   stimg->layer = img->layer;

   return TRUE;
}

static int
dri_get_param(struct st_manager *smapi,
              enum st_manager_param param)
{
   struct dri_screen *screen = (struct dri_screen *)smapi;

   switch(param) {
   case ST_MANAGER_BROKEN_INVALIDATE:
      return screen->broken_invalidate;
   default:
      return 0;
   }
}

static void
dri_destroy_option_cache(struct dri_screen * screen)
{
   int i;

   if (screen->optionCache.info) {
      for (i = 0; i < (1 << screen->optionCache.tableSize); ++i) {
         free(screen->optionCache.info[i].name);
         free(screen->optionCache.info[i].ranges);
      }
      free(screen->optionCache.info);
   }

   free(screen->optionCache.values);

   /* Default values are copied to screen->optionCache->values in
    * initOptionCache. The info field, however, is a pointer copy, so don't free
    * that twice.
    */
   free(screen->optionCacheDefaults.values);
}

void
dri_destroy_screen_helper(struct dri_screen * screen)
{
   if (screen->st_api && screen->st_api->destroy)
      screen->st_api->destroy(screen->st_api);

   if (screen->base.screen)
      screen->base.screen->destroy(screen->base.screen);

   dri_destroy_option_cache(screen);
}

void
dri_destroy_screen(__DRIscreen * sPriv)
{
   struct dri_screen *screen = dri_screen(sPriv);

   dri_destroy_screen_helper(screen);

#if !GALLIUM_STATIC_TARGETS
   pipe_loader_release(&screen->dev, 1);
#endif // !GALLIUM_STATIC_TARGETS

   free(screen);
   sPriv->driverPrivate = NULL;
   sPriv->extensions = NULL;
}

static void
dri_postprocessing_init(struct dri_screen *screen)
{
   unsigned i;

   for (i = 0; i < PP_FILTERS; i++) {
      screen->pp_enabled[i] = driQueryOptioni(&screen->optionCache,
                                              pp_filters[i].name);
   }
}

const __DRIconfig **
dri_init_screen_helper(struct dri_screen *screen,
                       struct pipe_screen *pscreen,
                       const char* driver_name)
{
   screen->base.screen = pscreen;
   if (!screen->base.screen) {
      debug_printf("%s: failed to create pipe_screen\n", __FUNCTION__);
      return NULL;
   }

   screen->base.get_egl_image = dri_get_egl_image;
   screen->base.get_param = dri_get_param;

   screen->st_api = st_gl_api_create();
   if (!screen->st_api)
      return NULL;

   if(pscreen->get_param(pscreen, PIPE_CAP_NPOT_TEXTURES))
      screen->target = PIPE_TEXTURE_2D;
   else
      screen->target = PIPE_TEXTURE_RECT;

   driParseOptionInfo(&screen->optionCacheDefaults, gallium_config_options.xml);

   driParseConfigFiles(&screen->optionCache,
                       &screen->optionCacheDefaults,
                       screen->sPriv->myNum,
                       driver_name);

   dri_fill_st_options(&screen->options, &screen->optionCache);

   /* Handle force_s3tc_enable. */
   if (!util_format_s3tc_enabled && screen->options.force_s3tc_enable) {
      /* Ensure libtxc_dxtn has been loaded if available.
       * Forcing S3TC on before calling this would prevent loading
       * the library.
       * This is just a precaution, the driver should have called it
       * already.
       */
      util_format_s3tc_init();

      util_format_s3tc_enabled = TRUE;
   }

   dri_postprocessing_init(screen);

   screen->st_api->query_versions(screen->st_api, &screen->base,
                                  &screen->options,
                                  &screen->sPriv->max_gl_core_version,
                                  &screen->sPriv->max_gl_compat_version,
                                  &screen->sPriv->max_gl_es1_version,
                                  &screen->sPriv->max_gl_es2_version);

   return dri_fill_in_modes(screen);
}

/* vim: set sw=3 ts=8 sts=3 expandtab: */
