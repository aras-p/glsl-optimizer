/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "main/mfeatures.h"
#include "main/version.h"

#include "intel_chipset.h"
#include "intel_context.h"
#include "intel_extensions.h"
#include "utils.h"

/**
 * Initializes potential list of extensions if ctx == NULL, or actually enables
 * extensions for a context.
 */
void
intelInitExtensions(struct gl_context *ctx)
{
   struct intel_context *intel = intel_context(ctx);

   ctx->Extensions.ARB_draw_elements_base_vertex = true;
   ctx->Extensions.ARB_explicit_attrib_location = true;
   ctx->Extensions.ARB_framebuffer_object = true;
   ctx->Extensions.ARB_half_float_pixel = true;
   ctx->Extensions.ARB_map_buffer_range = true;
   ctx->Extensions.ARB_point_sprite = true;
   ctx->Extensions.ARB_sampler_objects = true;
   ctx->Extensions.ARB_shader_objects = true;
   ctx->Extensions.ARB_shading_language_100 = true;
   ctx->Extensions.ARB_sync = true;
   ctx->Extensions.ARB_texture_border_clamp = true;
   ctx->Extensions.ARB_texture_cube_map = true;
   ctx->Extensions.ARB_texture_env_combine = true;
   ctx->Extensions.ARB_texture_env_crossbar = true;
   ctx->Extensions.ARB_texture_env_dot3 = true;
   ctx->Extensions.ARB_vertex_array_object = true;
   ctx->Extensions.ARB_vertex_program = true;
   ctx->Extensions.ARB_vertex_shader = true;
   ctx->Extensions.EXT_blend_color = true;
   ctx->Extensions.EXT_blend_equation_separate = true;
   ctx->Extensions.EXT_blend_func_separate = true;
   ctx->Extensions.EXT_blend_minmax = true;
   ctx->Extensions.EXT_framebuffer_blit = true;
   ctx->Extensions.EXT_framebuffer_object = true;
   ctx->Extensions.EXT_framebuffer_multisample = true;
   ctx->Extensions.EXT_fog_coord = true;
   ctx->Extensions.EXT_gpu_program_parameters = true;
   ctx->Extensions.EXT_packed_depth_stencil = true;
   ctx->Extensions.EXT_pixel_buffer_object = true;
   ctx->Extensions.EXT_point_parameters = true;
   ctx->Extensions.EXT_provoking_vertex = true;
   ctx->Extensions.EXT_secondary_color = true;
   ctx->Extensions.EXT_separate_shader_objects = true;
   ctx->Extensions.EXT_texture_env_dot3 = true;
   ctx->Extensions.EXT_texture_filter_anisotropic = true;
   ctx->Extensions.APPLE_object_purgeable = true;
   ctx->Extensions.APPLE_vertex_array_object = true;
   ctx->Extensions.MESA_pack_invert = true;
   ctx->Extensions.MESA_ycbcr_texture = true;
   ctx->Extensions.NV_blend_square = true;
   ctx->Extensions.NV_texture_rectangle = true;
   ctx->Extensions.NV_vertex_program = true;
   ctx->Extensions.NV_vertex_program1_1 = true;
   ctx->Extensions.TDFX_texture_compression_FXT1 = true;
#if FEATURE_OES_EGL_image
   ctx->Extensions.OES_EGL_image = true;
#endif

   if (intel->gen >= 6)
      ctx->Const.GLSLVersion = 130;
   else
      ctx->Const.GLSLVersion = 120;
   _mesa_override_glsl_version(ctx);

   if (intel->gen == 6 ||
       (intel->gen == 7 && intel->intelScreen->kernel_has_gen7_sol_reset))
      ctx->Extensions.EXT_transform_feedback = true;

   if (intel->gen >= 5)
      ctx->Extensions.EXT_timer_query = true;

   if (intel->gen >= 4) {
      ctx->Extensions.ARB_color_buffer_float = true;
      ctx->Extensions.ARB_depth_buffer_float = true;
      ctx->Extensions.ARB_depth_clamp = true;
      ctx->Extensions.ARB_fragment_coord_conventions = true;
      ctx->Extensions.ARB_fragment_program_shadow = true;
      ctx->Extensions.ARB_fragment_shader = true;
      ctx->Extensions.ARB_half_float_vertex = true;
      ctx->Extensions.ARB_occlusion_query = true;
      ctx->Extensions.ARB_point_sprite = true;
      ctx->Extensions.ARB_seamless_cube_map = true;
      ctx->Extensions.ARB_shader_texture_lod = true;
#ifdef TEXTURE_FLOAT_ENABLED
      ctx->Extensions.ARB_texture_float = true;
      ctx->Extensions.EXT_texture_shared_exponent = true;
      ctx->Extensions.EXT_packed_float = true;
#endif
      ctx->Extensions.ARB_texture_compression_rgtc = true;
      ctx->Extensions.ARB_texture_rg = true;
      ctx->Extensions.EXT_draw_buffers2 = true;
      ctx->Extensions.EXT_framebuffer_sRGB = true;
      ctx->Extensions.EXT_texture_array = true;
      ctx->Extensions.EXT_texture_integer = true;
      ctx->Extensions.EXT_texture_snorm = true;
      ctx->Extensions.EXT_texture_sRGB = true;
      ctx->Extensions.EXT_texture_sRGB_decode = true;
      ctx->Extensions.EXT_texture_swizzle = true;
      ctx->Extensions.EXT_vertex_array_bgra = true;
      ctx->Extensions.ATI_envmap_bumpmap = true;
      ctx->Extensions.MESA_texture_array = true;
      ctx->Extensions.NV_conditional_render = true;
   }

   if (intel->gen >= 3) {
      ctx->Extensions.ARB_ES2_compatibility = true;
      ctx->Extensions.ARB_depth_texture = true;
      ctx->Extensions.ARB_fragment_program = true;
      ctx->Extensions.ARB_shadow = true;
      ctx->Extensions.ARB_texture_non_power_of_two = true;
      ctx->Extensions.EXT_shadow_funcs = true;
      ctx->Extensions.EXT_stencil_two_side = true;
      ctx->Extensions.ATI_separate_stencil = true;
      ctx->Extensions.ATI_texture_env_combine3 = true;
      ctx->Extensions.NV_texture_env_combine4 = true;

      if (driQueryOptionb(&intel->optionCache, "fragment_shader"))
	 ctx->Extensions.ARB_fragment_shader = true;

      if (driQueryOptionb(&intel->optionCache, "stub_occlusion_query"))
	 ctx->Extensions.ARB_occlusion_query = true;
   }

   if (intel->ctx.Mesa_DXTn) {
      ctx->Extensions.EXT_texture_compression_s3tc = true;
      ctx->Extensions.S3_s3tc = true;
   }
   else if (driQueryOptionb(&intel->optionCache, "force_s3tc_enable")) {
      ctx->Extensions.EXT_texture_compression_s3tc = true;
   }
}
