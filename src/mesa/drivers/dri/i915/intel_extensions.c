/**************************************************************************
 * 
 * Copyright 2003 VMware, Inc.
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

#include "main/version.h"

#include "intel_chipset.h"
#include "intel_context.h"
#include "intel_extensions.h"
#include "intel_reg.h"
#include "utils.h"

/**
 * Initializes potential list of extensions if ctx == NULL, or actually enables
 * extensions for a context.
 */
void
intelInitExtensions(struct gl_context *ctx)
{
   struct intel_context *intel = intel_context(ctx);

   assert(intel->gen == 2 || intel->gen == 3);

   ctx->Extensions.ARB_draw_elements_base_vertex = true;
   ctx->Extensions.ARB_explicit_attrib_location = true;
   ctx->Extensions.ARB_explicit_uniform_location = true;
   ctx->Extensions.ARB_framebuffer_object = true;
   ctx->Extensions.ARB_internalformat_query = true;
   ctx->Extensions.ARB_map_buffer_range = true;
   ctx->Extensions.ARB_point_sprite = true;
   ctx->Extensions.ARB_sync = true;
   ctx->Extensions.ARB_texture_border_clamp = true;
   ctx->Extensions.ARB_texture_cube_map = true;
   ctx->Extensions.ARB_texture_env_combine = true;
   ctx->Extensions.ARB_texture_env_crossbar = true;
   ctx->Extensions.ARB_texture_env_dot3 = true;
   ctx->Extensions.ARB_vertex_program = true;
   ctx->Extensions.ARB_vertex_shader = true;
   ctx->Extensions.EXT_blend_color = true;
   ctx->Extensions.EXT_blend_equation_separate = true;
   ctx->Extensions.EXT_blend_func_separate = true;
   ctx->Extensions.EXT_blend_minmax = true;
   ctx->Extensions.EXT_gpu_program_parameters = true;
   ctx->Extensions.EXT_pixel_buffer_object = true;
   ctx->Extensions.EXT_point_parameters = true;
   ctx->Extensions.EXT_provoking_vertex = true;
   ctx->Extensions.EXT_texture_env_dot3 = true;
   ctx->Extensions.EXT_texture_filter_anisotropic = true;
   ctx->Extensions.APPLE_object_purgeable = true;
   ctx->Extensions.MESA_pack_invert = true;
   ctx->Extensions.MESA_ycbcr_texture = true;
   ctx->Extensions.NV_texture_rectangle = true;
   ctx->Extensions.TDFX_texture_compression_FXT1 = true;
   ctx->Extensions.OES_EGL_image = true;
   ctx->Extensions.OES_draw_texture = true;

   ctx->Const.GLSLVersion = 120;
   _mesa_override_glsl_version(&ctx->Const);

   if (intel->gen >= 3) {
      ctx->Extensions.ARB_ES2_compatibility = true;
      ctx->Extensions.ARB_depth_texture = true;
      ctx->Extensions.ARB_fragment_program = true;
      ctx->Extensions.ARB_shadow = true;
      ctx->Extensions.ARB_texture_non_power_of_two = true;
      ctx->Extensions.EXT_texture_sRGB = true;
      ctx->Extensions.EXT_texture_sRGB_decode = true;
      ctx->Extensions.EXT_stencil_two_side = true;
      ctx->Extensions.ATI_separate_stencil = true;
      ctx->Extensions.ATI_texture_env_combine3 = true;
      ctx->Extensions.NV_texture_env_combine4 = true;
      ctx->Extensions.ARB_fragment_shader = true;
      ctx->Extensions.ARB_occlusion_query = true;
   }

   if (intel->ctx.Mesa_DXTn
       || driQueryOptionb(&intel->optionCache, "force_s3tc_enable"))
      ctx->Extensions.EXT_texture_compression_s3tc = true;

   ctx->Extensions.ANGLE_texture_compression_dxt = true;
}
