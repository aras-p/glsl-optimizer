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

#include "intel_chipset.h"
#include "intel_context.h"
#include "intel_extensions.h"
#include "utils.h"


#define need_GL_ARB_copy_buffer
#define need_GL_ARB_draw_elements_base_vertex
#define need_GL_ARB_framebuffer_object
#define need_GL_ARB_map_buffer_range
#define need_GL_ARB_occlusion_query
#define need_GL_ARB_point_parameters
#define need_GL_ARB_shader_objects
#define need_GL_ARB_sync
#define need_GL_ARB_vertex_array_object
#define need_GL_ARB_vertex_program
#define need_GL_ARB_vertex_shader
#define need_GL_ARB_window_pos
#define need_GL_EXT_blend_color
#define need_GL_EXT_blend_equation_separate
#define need_GL_EXT_blend_func_separate
#define need_GL_EXT_blend_minmax
#define need_GL_EXT_cull_vertex
#define need_GL_EXT_draw_buffers2
#define need_GL_EXT_fog_coord
#define need_GL_EXT_framebuffer_object
#define need_GL_EXT_framebuffer_blit
#define need_GL_EXT_gpu_program_parameters
#define need_GL_EXT_point_parameters
#define need_GL_EXT_provoking_vertex
#define need_GL_EXT_secondary_color
#define need_GL_EXT_stencil_two_side
#define need_GL_APPLE_vertex_array_object
#define need_GL_APPLE_object_purgeable
#define need_GL_ATI_separate_stencil
#define need_GL_ATI_envmap_bumpmap
#define need_GL_NV_point_sprite
#define need_GL_NV_vertex_program
#define need_GL_VERSION_2_0
#define need_GL_VERSION_2_1

#include "main/remap_helper.h"


/**
 * Extension strings exported by the intel driver.
 *
 * Extensions supported by all chips supported by i830_dri, i915_dri, or
 * i965_dri.
 */
static const struct dri_extension card_extensions[] = {
   { "GL_ARB_copy_buffer",                GL_ARB_copy_buffer_functions },
   { "GL_ARB_draw_elements_base_vertex",  GL_ARB_draw_elements_base_vertex_functions },
   { "GL_ARB_half_float_pixel",           NULL },
   { "GL_ARB_map_buffer_range",           GL_ARB_map_buffer_range_functions },
   { "GL_ARB_multitexture",               NULL },
   { "GL_ARB_pixel_buffer_object",      NULL },
   { "GL_ARB_point_parameters",           GL_ARB_point_parameters_functions },
   { "GL_ARB_point_sprite",               NULL },
   { "GL_ARB_shader_objects",             GL_ARB_shader_objects_functions },
   { "GL_ARB_shading_language_100",       GL_VERSION_2_0_functions },
   { "GL_ARB_shading_language_120",       GL_VERSION_2_1_functions },
   { "GL_ARB_sync",                       GL_ARB_sync_functions },
   { "GL_ARB_texture_border_clamp",       NULL },
   { "GL_ARB_texture_cube_map",           NULL },
   { "GL_ARB_texture_env_add",            NULL },
   { "GL_ARB_texture_env_combine",        NULL },
   { "GL_ARB_texture_env_crossbar",       NULL },
   { "GL_ARB_texture_env_dot3",           NULL },
   { "GL_ARB_texture_mirrored_repeat",    NULL },
   { "GL_ARB_texture_rectangle",          NULL },
   { "GL_ARB_vertex_array_object",        GL_ARB_vertex_array_object_functions},
   { "GL_ARB_vertex_program",             GL_ARB_vertex_program_functions },
   { "GL_ARB_vertex_shader",              GL_ARB_vertex_shader_functions },
   { "GL_ARB_window_pos",                 GL_ARB_window_pos_functions },
   { "GL_EXT_blend_color",                GL_EXT_blend_color_functions },
   { "GL_EXT_blend_equation_separate",    GL_EXT_blend_equation_separate_functions },
   { "GL_EXT_blend_func_separate",        GL_EXT_blend_func_separate_functions },
   { "GL_EXT_blend_minmax",               GL_EXT_blend_minmax_functions },
   { "GL_EXT_blend_logic_op",             NULL },
   { "GL_EXT_blend_subtract",             NULL },
   { "GL_EXT_cull_vertex",                GL_EXT_cull_vertex_functions },
   { "GL_EXT_framebuffer_blit",         GL_EXT_framebuffer_blit_functions },
   { "GL_EXT_framebuffer_object",       GL_EXT_framebuffer_object_functions },
   { "GL_EXT_fog_coord",                  GL_EXT_fog_coord_functions },
   { "GL_EXT_gpu_program_parameters",     GL_EXT_gpu_program_parameters_functions },
   { "GL_EXT_packed_depth_stencil",       NULL },
   { "GL_EXT_provoking_vertex",           GL_EXT_provoking_vertex_functions },
   { "GL_EXT_secondary_color",            GL_EXT_secondary_color_functions },
   { "GL_EXT_stencil_wrap",               NULL },
   { "GL_EXT_texture_edge_clamp",         NULL },
   { "GL_EXT_texture_env_combine",        NULL },
   { "GL_EXT_texture_env_dot3",           NULL },
   { "GL_EXT_texture_filter_anisotropic", NULL },
   { "GL_EXT_texture_lod_bias",           NULL },
   { "GL_3DFX_texture_compression_FXT1",  NULL },
   { "GL_APPLE_client_storage",           NULL },
   { "GL_APPLE_object_purgeable",         GL_APPLE_object_purgeable_functions },
   { "GL_APPLE_vertex_array_object",      GL_APPLE_vertex_array_object_functions},
   { "GL_MESA_pack_invert",               NULL },
   { "GL_MESA_ycbcr_texture",             NULL },
   { "GL_NV_blend_square",                NULL },
   { "GL_NV_vertex_program",              GL_NV_vertex_program_functions },
   { "GL_NV_vertex_program1_1",           NULL },
   { "GL_SGIS_generate_mipmap",           NULL },
   { NULL, NULL }
};


/** i915 / i945-only extensions */
static const struct dri_extension i915_extensions[] = {
   { "GL_ARB_depth_texture",              NULL },
   { "GL_ARB_fragment_program",           NULL },
   { "GL_ARB_shadow",                     NULL },
   { "GL_ARB_texture_non_power_of_two",   NULL },
   { "GL_ATI_separate_stencil",           GL_ATI_separate_stencil_functions },
   { "GL_ATI_texture_env_combine3",       NULL },
   { "GL_EXT_shadow_funcs",               NULL },
   { "GL_EXT_stencil_two_side",           GL_EXT_stencil_two_side_functions },
   { "GL_NV_texture_env_combine4",        NULL },
   { NULL,                                NULL }
};


/** i965-only extensions */
static const struct dri_extension brw_extensions[] = {
   { "GL_ARB_depth_clamp",                NULL },
   { "GL_ARB_depth_texture",              NULL },
   { "GL_ARB_fragment_coord_conventions", NULL },
   { "GL_ARB_fragment_program",           NULL },
   { "GL_ARB_fragment_program_shadow",    NULL },
   { "GL_ARB_fragment_shader",            NULL },
   { "GL_ARB_framebuffer_object",         GL_ARB_framebuffer_object_functions},
   { "GL_ARB_half_float_vertex",          NULL },
   { "GL_ARB_occlusion_query",            GL_ARB_occlusion_query_functions },
   { "GL_ARB_point_sprite", 		  NULL },
   { "GL_ARB_seamless_cube_map",          NULL },
   { "GL_ARB_shadow",                     NULL },
   { "GL_MESA_texture_signed_rgba",       NULL },
   { "GL_ARB_texture_non_power_of_two",   NULL },
   { "GL_EXT_draw_buffers2",              GL_EXT_draw_buffers2_functions },
   { "GL_EXT_shadow_funcs",               NULL },
   { "GL_EXT_stencil_two_side",           GL_EXT_stencil_two_side_functions },
   { "GL_EXT_texture_sRGB",		  NULL },
   { "GL_EXT_texture_swizzle",		  NULL },
   { "GL_EXT_vertex_array_bgra",	  NULL },
   { "GL_ATI_envmap_bumpmap",             GL_ATI_envmap_bumpmap_functions },
   { "GL_ATI_separate_stencil",           GL_ATI_separate_stencil_functions },
   { "GL_ATI_texture_env_combine3",       NULL },
   { "GL_NV_texture_env_combine4",        NULL },
   { NULL,                                NULL }
};


static const struct dri_extension arb_oq_extensions[] = {
   { "GL_ARB_occlusion_query",            GL_ARB_occlusion_query_functions },
   { NULL, NULL }
};


static const struct dri_extension fragment_shader_extensions[] = {
   { "GL_ARB_fragment_shader",            NULL },
   { NULL, NULL }
};

/**
 * Initializes potential list of extensions if ctx == NULL, or actually enables
 * extensions for a context.
 */
void
intelInitExtensions(GLcontext *ctx)
{
   struct intel_context *intel = intel_context(ctx);

   /* Disable imaging extension until convolution is working in teximage paths.
    */
   driInitExtensions(ctx, card_extensions, GL_FALSE);

   if (intel->gen >= 4)
      driInitExtensions(ctx, brw_extensions, GL_FALSE);

   if (intel->gen == 3) {
      driInitExtensions(ctx, i915_extensions, GL_FALSE);

      if (driQueryOptionb(&intel->optionCache, "fragment_shader"))
	 driInitExtensions(ctx, fragment_shader_extensions, GL_FALSE);

      if (driQueryOptionb(&intel->optionCache, "stub_occlusion_query"))
	 driInitExtensions(ctx, arb_oq_extensions, GL_FALSE);
   }
}
