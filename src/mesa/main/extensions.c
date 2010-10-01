/*
 * Mesa 3-D graphics library
 * Version:  7.6
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include "glheader.h"
#include "imports.h"
#include "context.h"
#include "extensions.h"
#include "mtypes.h"


#define F(x) offsetof(struct gl_extensions, x)
#define ON GL_TRUE
#define OFF GL_FALSE


/*
 * Note: The GL_MESAX_* extensions are placeholders for future ARB extensions.
 */
static const struct {
   GLboolean enabled;
   const char *name;
   int flag_offset;
} default_extensions[] = {
   { OFF, "GL_ARB_blend_func_extended",        F(ARB_blend_func_extended) },
   { ON,  "GL_ARB_copy_buffer",                F(ARB_copy_buffer) },
   { OFF, "GL_ARB_depth_buffer_float",         F(ARB_depth_buffer_float) },
   { OFF, "GL_ARB_depth_clamp",                F(ARB_depth_clamp) },
   { OFF, "GL_ARB_depth_texture",              F(ARB_depth_texture) },
   { ON,  "GL_ARB_draw_buffers",               F(ARB_draw_buffers) },
   { OFF, "GL_ARB_draw_elements_base_vertex",  F(ARB_draw_elements_base_vertex) },
   { OFF, "GL_ARB_draw_instanced",             F(ARB_draw_instanced) },
   { OFF, "GL_ARB_fragment_coord_conventions", F(ARB_fragment_coord_conventions) },
   { OFF, "GL_ARB_fragment_program",           F(ARB_fragment_program) },
   { OFF, "GL_ARB_fragment_program_shadow",    F(ARB_fragment_program_shadow) },
   { OFF, "GL_ARB_fragment_shader",            F(ARB_fragment_shader) },
   { OFF, "GL_ARB_framebuffer_object",         F(ARB_framebuffer_object) },
   { OFF, "GL_ARB_explicit_attrib_location",   F(ARB_explicit_attrib_location) },
   /* TODO: reenable this when the new GLSL compiler actually supports them */
   /* { OFF, "GL_ARB_geometry_shader4",           F(ARB_geometry_shader4) }, */
   { OFF, "GL_ARB_half_float_pixel",           F(ARB_half_float_pixel) },
   { OFF, "GL_ARB_half_float_vertex",          F(ARB_half_float_vertex) },
   { OFF, "GL_ARB_instanced_arrays",           F(ARB_instanced_arrays) },
   { OFF, "GL_ARB_map_buffer_range",           F(ARB_map_buffer_range) },
   { ON,  "GL_ARB_multisample",                F(ARB_multisample) },
   { OFF, "GL_ARB_multitexture",               F(ARB_multitexture) },
   { OFF, "GL_ARB_occlusion_query",            F(ARB_occlusion_query) },
   { OFF, "GL_ARB_occlusion_query2",           F(ARB_occlusion_query2) },
   { OFF, "GL_ARB_pixel_buffer_object",        F(EXT_pixel_buffer_object) },
   { OFF, "GL_ARB_point_parameters",           F(EXT_point_parameters) },
   { OFF, "GL_ARB_point_sprite",               F(ARB_point_sprite) },
   { OFF, "GL_ARB_provoking_vertex",           F(EXT_provoking_vertex) },
   { OFF, "GL_ARB_sampler_objects",            F(ARB_sampler_objects) },
   { OFF, "GL_ARB_seamless_cube_map",          F(ARB_seamless_cube_map) },
   { OFF, "GL_ARB_shader_objects",             F(ARB_shader_objects) },
   { OFF, "GL_ARB_shading_language_100",       F(ARB_shading_language_100) },
   { OFF, "GL_ARB_shadow",                     F(ARB_shadow) },
   { OFF, "GL_ARB_shadow_ambient",             F(ARB_shadow_ambient) },
   { OFF, "GL_ARB_sync",                       F(ARB_sync) },
   { OFF, "GL_ARB_texture_border_clamp",       F(ARB_texture_border_clamp) },
   { OFF, "GL_ARB_texture_buffer_object",      F(ARB_texture_buffer_object) },
   { ON,  "GL_ARB_texture_compression",        F(ARB_texture_compression) },
   { OFF, "GL_ARB_texture_compression_rgtc",   F(ARB_texture_compression_rgtc) },
   { OFF, "GL_ARB_texture_cube_map",           F(ARB_texture_cube_map) },
   { OFF, "GL_ARB_texture_env_add",            F(EXT_texture_env_add) },
   { OFF, "GL_ARB_texture_env_combine",        F(ARB_texture_env_combine) },
   { OFF, "GL_ARB_texture_env_crossbar",       F(ARB_texture_env_crossbar) },
   { OFF, "GL_ARB_texture_env_dot3",           F(ARB_texture_env_dot3) },
   { OFF, "GL_MESAX_texture_float",            F(ARB_texture_float) },
   { OFF, "GL_ARB_texture_mirrored_repeat",    F(ARB_texture_mirrored_repeat)},
   { OFF, "GL_ARB_texture_multisample",        F(ARB_texture_multisample) },
   { OFF, "GL_ARB_texture_non_power_of_two",   F(ARB_texture_non_power_of_two)},
   { OFF, "GL_ARB_texture_rectangle",          F(NV_texture_rectangle) },
   { OFF, "GL_ARB_texture_rg",                 F(ARB_texture_rg) },
   { OFF, "GL_ARB_texture_rgb10_a2ui",         F(ARB_texture_rgb10_a2ui) },
   { OFF, "GL_ARB_texture_swizzle",            F(EXT_texture_swizzle) },
   { ON,  "GL_ARB_transpose_matrix",           F(ARB_transpose_matrix) },
   { OFF, "GL_ARB_transform_feedback2",        F(ARB_transform_feedback2) },
   { OFF, "GL_ARB_uniform_buffer_object",      F(ARB_uniform_buffer_object) },
   { OFF, "GL_ARB_vertex_array_bgra",          F(EXT_vertex_array_bgra) },
   { OFF, "GL_ARB_vertex_array_object",        F(ARB_vertex_array_object) },
   { ON,  "GL_ARB_vertex_buffer_object",       F(ARB_vertex_buffer_object) },
   { OFF, "GL_ARB_vertex_program",             F(ARB_vertex_program) },
   { OFF, "GL_ARB_vertex_shader",              F(ARB_vertex_shader) },
   { OFF, "GL_ARB_vertex_type_2_10_10_10_rev", F(ARB_vertex_type_2_10_10_10_rev) },
   { ON,  "GL_ARB_window_pos",                 F(ARB_window_pos) },
   { ON,  "GL_EXT_abgr",                       F(EXT_abgr) },
   { ON,  "GL_EXT_bgra",                       F(EXT_bgra) },
   { OFF, "GL_EXT_blend_color",                F(EXT_blend_color) },
   { OFF, "GL_EXT_blend_equation_separate",    F(EXT_blend_equation_separate) },
   { OFF, "GL_EXT_blend_func_separate",        F(EXT_blend_func_separate) },
   { OFF, "GL_EXT_blend_logic_op",             F(EXT_blend_logic_op) },
   { OFF, "GL_EXT_blend_minmax",               F(EXT_blend_minmax) },
   { OFF, "GL_EXT_blend_subtract",             F(EXT_blend_subtract) },
   { OFF, "GL_EXT_clip_volume_hint",           F(EXT_clip_volume_hint) },
   { ON,  "GL_EXT_compiled_vertex_array",      F(EXT_compiled_vertex_array) },
   { ON,  "GL_EXT_copy_texture",               F(EXT_copy_texture) },
   { OFF, "GL_EXT_depth_bounds_test",          F(EXT_depth_bounds_test) },
   { OFF, "GL_EXT_draw_buffers2",              F(EXT_draw_buffers2) },
   { OFF, "GL_EXT_draw_instanced",             F(ARB_draw_instanced) },
   { ON,  "GL_EXT_draw_range_elements",        F(EXT_draw_range_elements) },
   { OFF, "GL_EXT_framebuffer_blit",           F(EXT_framebuffer_blit) },
   { OFF, "GL_EXT_framebuffer_multisample",    F(EXT_framebuffer_multisample) },
   { OFF, "GL_EXT_framebuffer_object",         F(EXT_framebuffer_object) },
   { OFF, "GL_EXT_framebuffer_sRGB",           F(EXT_framebuffer_sRGB) },
   { OFF, "GL_EXT_fog_coord",                  F(EXT_fog_coord) },
   { OFF, "GL_EXT_gpu_program_parameters",     F(EXT_gpu_program_parameters) },
   { ON,  "GL_EXT_multi_draw_arrays",          F(EXT_multi_draw_arrays) },
   { OFF, "GL_EXT_packed_depth_stencil",       F(EXT_packed_depth_stencil) },
   { OFF, "GL_EXT_packed_float",               F(EXT_packed_float) },
   { ON,  "GL_EXT_packed_pixels",              F(EXT_packed_pixels) },
   { OFF, "GL_EXT_paletted_texture",           F(EXT_paletted_texture) },
   { OFF, "GL_EXT_pixel_buffer_object",        F(EXT_pixel_buffer_object) },
   { OFF, "GL_EXT_point_parameters",           F(EXT_point_parameters) },
   { ON,  "GL_EXT_polygon_offset",             F(EXT_polygon_offset) },
   { OFF, "GL_EXT_provoking_vertex",           F(EXT_provoking_vertex) },
   { ON,  "GL_EXT_rescale_normal",             F(EXT_rescale_normal) },
   { OFF, "GL_EXT_secondary_color",            F(EXT_secondary_color) },
   { ON,  "GL_EXT_separate_specular_color",    F(EXT_separate_specular_color) },
   { OFF, "GL_EXT_shadow_funcs",               F(EXT_shadow_funcs) },
   { OFF, "GL_EXT_shared_texture_palette",     F(EXT_shared_texture_palette) },
   { OFF, "GL_EXT_stencil_two_side",           F(EXT_stencil_two_side) },
   { OFF, "GL_EXT_stencil_wrap",               F(EXT_stencil_wrap) },
   { ON,  "GL_EXT_subtexture",                 F(EXT_subtexture) },
   { ON,  "GL_EXT_texture",                    F(EXT_texture) },
   { ON,  "GL_EXT_texture3D",                  F(EXT_texture3D) },
   { OFF, "GL_EXT_texture_array",              F(EXT_texture_array) },
   { OFF, "GL_EXT_texture_compression_s3tc",   F(EXT_texture_compression_s3tc) },
   { OFF, "GL_EXT_texture_compression_rgtc",   F(ARB_texture_compression_rgtc) },
   { OFF, "GL_EXT_texture_cube_map",           F(ARB_texture_cube_map) },
   { ON,  "GL_EXT_texture_edge_clamp",         F(SGIS_texture_edge_clamp) },
   { OFF, "GL_EXT_texture_env_add",            F(EXT_texture_env_add) },
   { OFF, "GL_EXT_texture_env_combine",        F(EXT_texture_env_combine) },
   { OFF, "GL_EXT_texture_env_dot3",           F(EXT_texture_env_dot3) },
   { OFF, "GL_EXT_texture_filter_anisotropic", F(EXT_texture_filter_anisotropic) },
   { OFF, "GL_EXT_texture_integer",            F(EXT_texture_integer) },
   { OFF, "GL_EXT_texture_lod_bias",           F(EXT_texture_lod_bias) },
   { OFF, "GL_EXT_texture_mirror_clamp",       F(EXT_texture_mirror_clamp) },
   { ON,  "GL_EXT_texture_object",             F(EXT_texture_object) },
   { OFF, "GL_EXT_texture_rectangle",          F(NV_texture_rectangle) },
   { OFF, "GL_EXT_texture_shared_exponent",    F(EXT_texture_shared_exponent) },
   { OFF, "GL_EXT_texture_sRGB",               F(EXT_texture_sRGB) },
   { OFF, "GL_EXT_texture_swizzle",            F(EXT_texture_swizzle) },
   { OFF, "GL_EXT_timer_query",                F(EXT_timer_query) },
   { OFF, "GL_EXT_transform_feedback",         F(EXT_transform_feedback) },
   { ON,  "GL_EXT_vertex_array",               F(EXT_vertex_array) },
   { OFF, "GL_EXT_vertex_array_bgra",          F(EXT_vertex_array_bgra) },
   { OFF, "GL_EXT_vertex_array_set",           F(EXT_vertex_array_set) },
   { OFF, "GL_3DFX_texture_compression_FXT1",  F(TDFX_texture_compression_FXT1) },
   { OFF, "GL_APPLE_client_storage",           F(APPLE_client_storage) },
   { ON,  "GL_APPLE_packed_pixels",            F(APPLE_packed_pixels) },
   { OFF, "GL_APPLE_vertex_array_object",      F(APPLE_vertex_array_object) },
   { OFF, "GL_APPLE_object_purgeable",         F(APPLE_object_purgeable) },
   { OFF, "GL_ATI_blend_equation_separate",    F(EXT_blend_equation_separate) },
   { OFF, "GL_ATI_envmap_bumpmap",             F(ATI_envmap_bumpmap) },
   { OFF, "GL_ATI_texture_env_combine3",       F(ATI_texture_env_combine3)},
   { OFF, "GL_ATI_texture_mirror_once",        F(ATI_texture_mirror_once)},
   { OFF, "GL_ATI_fragment_shader",            F(ATI_fragment_shader)},
   { OFF, "GL_ATI_separate_stencil",           F(ATI_separate_stencil)},
   { ON,  "GL_IBM_multimode_draw_arrays",      F(IBM_multimode_draw_arrays) },
   { ON,  "GL_IBM_rasterpos_clip",             F(IBM_rasterpos_clip) },
   { OFF, "GL_IBM_texture_mirrored_repeat",    F(ARB_texture_mirrored_repeat)},
   { OFF, "GL_INGR_blend_func_separate",       F(EXT_blend_func_separate) },
   { OFF, "GL_MESA_pack_invert",               F(MESA_pack_invert) },
   { OFF, "GL_MESA_resize_buffers",            F(MESA_resize_buffers) },
   { OFF, "GL_MESA_texture_array",             F(MESA_texture_array) },
   { OFF, "GL_MESA_texture_signed_rgba",       F(MESA_texture_signed_rgba) },
   { OFF, "GL_MESA_ycbcr_texture",             F(MESA_ycbcr_texture) },
   { ON,  "GL_MESA_window_pos",                F(ARB_window_pos) },
   { OFF, "GL_NV_blend_square",                F(NV_blend_square) },
   { OFF, "GL_NV_conditional_render",          F(NV_conditional_render) },
   { OFF, "GL_NV_depth_clamp",                 F(ARB_depth_clamp) },
   { OFF, "GL_NV_fragment_program",            F(NV_fragment_program) },
   { OFF, "GL_NV_fragment_program_option",     F(NV_fragment_program_option) },
   { ON,  "GL_NV_light_max_exponent",          F(NV_light_max_exponent) },
   { OFF, "GL_NV_packed_depth_stencil",        F(EXT_packed_depth_stencil) },
   { OFF, "GL_NV_point_sprite",                F(NV_point_sprite) },
   { OFF, "GL_NV_primitive_restart",           F(NV_primitive_restart) },
   { ON,  "GL_NV_texgen_reflection",           F(NV_texgen_reflection) },
   { OFF, "GL_NV_texture_env_combine4",        F(NV_texture_env_combine4) },
   { OFF, "GL_NV_texture_rectangle",           F(NV_texture_rectangle) },
   { OFF, "GL_NV_vertex_program",              F(NV_vertex_program) },
   { OFF, "GL_NV_vertex_program1_1",           F(NV_vertex_program1_1) },
   { ON,  "GL_OES_read_format",                F(OES_read_format) },
   { OFF, "GL_SGI_texture_color_table",        F(SGI_texture_color_table) },
   { ON,  "GL_SGIS_generate_mipmap",           F(SGIS_generate_mipmap) },
   { OFF, "GL_SGIS_texture_border_clamp",      F(ARB_texture_border_clamp) },
   { ON,  "GL_SGIS_texture_edge_clamp",        F(SGIS_texture_edge_clamp) },
   { ON,  "GL_SGIS_texture_lod",               F(SGIS_texture_lod) },
   { ON,  "GL_SUN_multi_draw_arrays",          F(EXT_multi_draw_arrays) },
   { OFF, "GL_S3_s3tc",                        F(S3_s3tc) },
#if FEATURE_OES_EGL_image
   { OFF, "GL_OES_EGL_image",                  F(OES_EGL_image) },
#endif
#if FEATURE_OES_draw_texture
   { OFF, "GL_OES_draw_texture",               F(OES_draw_texture) },
#endif /* FEATURE_OES_draw_texture */
};



/**
 * Enable all extensions suitable for a software-only renderer.
 * This is a convenience function used by the XMesa, OSMesa, GGI drivers, etc.
 */
void
_mesa_enable_sw_extensions(GLcontext *ctx)
{
   /*ctx->Extensions.ARB_copy_buffer = GL_TRUE;*/
   ctx->Extensions.ARB_depth_clamp = GL_TRUE;
   ctx->Extensions.ARB_depth_texture = GL_TRUE;
   /*ctx->Extensions.ARB_draw_buffers = GL_TRUE;*/
   ctx->Extensions.ARB_draw_elements_base_vertex = GL_TRUE;
   ctx->Extensions.ARB_fragment_coord_conventions = GL_TRUE;
#if FEATURE_ARB_fragment_program
   ctx->Extensions.ARB_fragment_program = GL_TRUE;
   ctx->Extensions.ARB_fragment_program_shadow = GL_TRUE;
#endif
#if FEATURE_ARB_fragment_shader
   ctx->Extensions.ARB_fragment_shader = GL_TRUE;
#endif
#if FEATURE_ARB_framebuffer_object
   ctx->Extensions.ARB_framebuffer_object = GL_TRUE;
#endif
#if FEATURE_ARB_geometry_shader4
   ctx->Extensions.ARB_geometry_shader4 = GL_TRUE;
#endif
   ctx->Extensions.ARB_half_float_pixel = GL_TRUE;
   ctx->Extensions.ARB_half_float_vertex = GL_TRUE;
   ctx->Extensions.ARB_map_buffer_range = GL_TRUE;
   ctx->Extensions.ARB_multitexture = GL_TRUE;
#if FEATURE_queryobj
   ctx->Extensions.ARB_occlusion_query = GL_TRUE;
#endif
   ctx->Extensions.ARB_point_sprite = GL_TRUE;
#if FEATURE_ARB_shader_objects
   ctx->Extensions.ARB_shader_objects = GL_TRUE;
#endif
#if FEATURE_ARB_shading_language_100
   ctx->Extensions.ARB_shading_language_100 = GL_TRUE;
#endif
   ctx->Extensions.ARB_shadow = GL_TRUE;
   ctx->Extensions.ARB_shadow_ambient = GL_TRUE;
   ctx->Extensions.ARB_texture_border_clamp = GL_TRUE;
   ctx->Extensions.ARB_texture_cube_map = GL_TRUE;
   ctx->Extensions.ARB_texture_env_combine = GL_TRUE;
   ctx->Extensions.ARB_texture_env_crossbar = GL_TRUE;
   ctx->Extensions.ARB_texture_env_dot3 = GL_TRUE;
   /*ctx->Extensions.ARB_texture_float = GL_TRUE;*/
   ctx->Extensions.ARB_texture_mirrored_repeat = GL_TRUE;
   ctx->Extensions.ARB_texture_non_power_of_two = GL_TRUE;
   ctx->Extensions.ARB_texture_rg = GL_TRUE;
   ctx->Extensions.ARB_vertex_array_object = GL_TRUE;
#if FEATURE_ARB_vertex_program
   ctx->Extensions.ARB_vertex_program = GL_TRUE;
#endif
#if FEATURE_ARB_vertex_shader
   ctx->Extensions.ARB_vertex_shader = GL_TRUE;
#endif
#if FEATURE_ARB_vertex_buffer_object
   /*ctx->Extensions.ARB_vertex_buffer_object = GL_TRUE;*/
#endif
#if FEATURE_ARB_sync
   ctx->Extensions.ARB_sync = GL_TRUE;
#endif
   ctx->Extensions.APPLE_vertex_array_object = GL_TRUE;
#if FEATURE_APPLE_object_purgeable
   ctx->Extensions.APPLE_object_purgeable = GL_TRUE;
#endif
   ctx->Extensions.ATI_envmap_bumpmap = GL_TRUE;
#if FEATURE_ATI_fragment_shader
   ctx->Extensions.ATI_fragment_shader = GL_TRUE;
#endif
   ctx->Extensions.ATI_texture_env_combine3 = GL_TRUE;
   ctx->Extensions.ATI_texture_mirror_once = GL_TRUE;
   ctx->Extensions.ATI_separate_stencil = GL_TRUE;
   ctx->Extensions.EXT_blend_color = GL_TRUE;
   ctx->Extensions.EXT_blend_equation_separate = GL_TRUE;
   ctx->Extensions.EXT_blend_func_separate = GL_TRUE;
   ctx->Extensions.EXT_blend_logic_op = GL_TRUE;
   ctx->Extensions.EXT_blend_minmax = GL_TRUE;
   ctx->Extensions.EXT_blend_subtract = GL_TRUE;
   ctx->Extensions.EXT_depth_bounds_test = GL_TRUE;
   ctx->Extensions.EXT_draw_buffers2 = GL_TRUE;
   ctx->Extensions.EXT_fog_coord = GL_TRUE;
#if FEATURE_EXT_framebuffer_object
   ctx->Extensions.EXT_framebuffer_object = GL_TRUE;
#endif
#if FEATURE_EXT_framebuffer_blit
   ctx->Extensions.EXT_framebuffer_blit = GL_TRUE;
#endif
#if FEATURE_ARB_framebuffer_object
   ctx->Extensions.EXT_framebuffer_multisample = GL_TRUE;
#endif
   /*ctx->Extensions.EXT_multi_draw_arrays = GL_TRUE;*/
   ctx->Extensions.EXT_packed_depth_stencil = GL_TRUE;
   ctx->Extensions.EXT_paletted_texture = GL_TRUE;
#if FEATURE_EXT_pixel_buffer_object
   ctx->Extensions.EXT_pixel_buffer_object = GL_TRUE;
#endif
   ctx->Extensions.EXT_point_parameters = GL_TRUE;
   ctx->Extensions.EXT_provoking_vertex = GL_TRUE;
   ctx->Extensions.EXT_shadow_funcs = GL_TRUE;
   ctx->Extensions.EXT_secondary_color = GL_TRUE;
   ctx->Extensions.EXT_shared_texture_palette = GL_TRUE;
   ctx->Extensions.EXT_stencil_wrap = GL_TRUE;
   ctx->Extensions.EXT_stencil_two_side = GL_TRUE;
   ctx->Extensions.EXT_texture_array = GL_TRUE;
   ctx->Extensions.EXT_texture_env_add = GL_TRUE;
   ctx->Extensions.EXT_texture_env_combine = GL_TRUE;
   ctx->Extensions.EXT_texture_env_dot3 = GL_TRUE;
   ctx->Extensions.EXT_texture_mirror_clamp = GL_TRUE;
   ctx->Extensions.EXT_texture_lod_bias = GL_TRUE;
#if FEATURE_EXT_texture_sRGB
   ctx->Extensions.EXT_texture_sRGB = GL_TRUE;
#endif
   ctx->Extensions.EXT_texture_swizzle = GL_TRUE;
#if FEATURE_EXT_transform_feedback
   /*ctx->Extensions.EXT_transform_feedback = GL_TRUE;*/
#endif
   ctx->Extensions.EXT_vertex_array_bgra = GL_TRUE;
   /*ctx->Extensions.IBM_multimode_draw_arrays = GL_TRUE;*/
   ctx->Extensions.MESA_pack_invert = GL_TRUE;
   ctx->Extensions.MESA_resize_buffers = GL_TRUE;
   ctx->Extensions.MESA_texture_array = GL_TRUE;
   ctx->Extensions.MESA_ycbcr_texture = GL_TRUE;
   ctx->Extensions.NV_blend_square = GL_TRUE;
   ctx->Extensions.NV_conditional_render = GL_TRUE;
   /*ctx->Extensions.NV_light_max_exponent = GL_TRUE;*/
   ctx->Extensions.NV_point_sprite = GL_TRUE;
   ctx->Extensions.NV_texture_env_combine4 = GL_TRUE;
   ctx->Extensions.NV_texture_rectangle = GL_TRUE;
   /*ctx->Extensions.NV_texgen_reflection = GL_TRUE;*/
#if FEATURE_NV_vertex_program
   ctx->Extensions.NV_vertex_program = GL_TRUE;
   ctx->Extensions.NV_vertex_program1_1 = GL_TRUE;
#endif
#if FEATURE_NV_fragment_program
   ctx->Extensions.NV_fragment_program = GL_TRUE;
#endif
#if FEATURE_NV_fragment_program && FEATURE_ARB_fragment_program
   ctx->Extensions.NV_fragment_program_option = GL_TRUE;
#endif
   ctx->Extensions.SGI_texture_color_table = GL_TRUE;
   /*ctx->Extensions.SGIS_generate_mipmap = GL_TRUE;*/
   ctx->Extensions.SGIS_texture_edge_clamp = GL_TRUE;
#if FEATURE_ARB_vertex_program || FEATURE_ARB_fragment_program
   ctx->Extensions.EXT_gpu_program_parameters = GL_TRUE;
#endif
#if FEATURE_texture_fxt1
   _mesa_enable_extension(ctx, "GL_3DFX_texture_compression_FXT1");
#endif
#if FEATURE_texture_s3tc
   if (ctx->Mesa_DXTn) {
      _mesa_enable_extension(ctx, "GL_EXT_texture_compression_s3tc");
      _mesa_enable_extension(ctx, "GL_S3_s3tc");
   }
#endif
}


/**
 * Enable common EXT extensions in the ARB_imaging subset.
 */
void
_mesa_enable_imaging_extensions(GLcontext *ctx)
{
   ctx->Extensions.EXT_blend_color = GL_TRUE;
   ctx->Extensions.EXT_blend_logic_op = GL_TRUE;
   ctx->Extensions.EXT_blend_minmax = GL_TRUE;
   ctx->Extensions.EXT_blend_subtract = GL_TRUE;
}



/**
 * Enable all OpenGL 1.3 features and extensions.
 * A convenience function to be called by drivers.
 */
void
_mesa_enable_1_3_extensions(GLcontext *ctx)
{
   /*ctx->Extensions.ARB_multisample = GL_TRUE;*/
   ctx->Extensions.ARB_multitexture = GL_TRUE;
   ctx->Extensions.ARB_texture_border_clamp = GL_TRUE;
   /*ctx->Extensions.ARB_texture_compression = GL_TRUE;*/
   ctx->Extensions.ARB_texture_cube_map = GL_TRUE;
   ctx->Extensions.ARB_texture_env_combine = GL_TRUE;
   ctx->Extensions.ARB_texture_env_dot3 = GL_TRUE;
   ctx->Extensions.EXT_texture_env_add = GL_TRUE;
   /*ctx->Extensions.ARB_transpose_matrix = GL_TRUE;*/
}



/**
 * Enable all OpenGL 1.4 features and extensions.
 * A convenience function to be called by drivers.
 */
void
_mesa_enable_1_4_extensions(GLcontext *ctx)
{
   ctx->Extensions.ARB_depth_texture = GL_TRUE;
   ctx->Extensions.ARB_shadow = GL_TRUE;
   ctx->Extensions.ARB_texture_env_crossbar = GL_TRUE;
   ctx->Extensions.ARB_texture_mirrored_repeat = GL_TRUE;
   ctx->Extensions.ARB_window_pos = GL_TRUE;
   ctx->Extensions.EXT_blend_color = GL_TRUE;
   ctx->Extensions.EXT_blend_func_separate = GL_TRUE;
   ctx->Extensions.EXT_blend_minmax = GL_TRUE;
   ctx->Extensions.EXT_blend_subtract = GL_TRUE;
   ctx->Extensions.EXT_fog_coord = GL_TRUE;
   /*ctx->Extensions.EXT_multi_draw_arrays = GL_TRUE;*/
   ctx->Extensions.EXT_point_parameters = GL_TRUE;
   ctx->Extensions.EXT_secondary_color = GL_TRUE;
   ctx->Extensions.EXT_stencil_wrap = GL_TRUE;
   ctx->Extensions.EXT_texture_lod_bias = GL_TRUE;
   /*ctx->Extensions.SGIS_generate_mipmap = GL_TRUE;*/
}


/**
 * Enable all OpenGL 1.5 features and extensions.
 * A convenience function to be called by drivers.
 */
void
_mesa_enable_1_5_extensions(GLcontext *ctx)
{
   ctx->Extensions.ARB_occlusion_query = GL_TRUE;
   /*ctx->Extensions.ARB_vertex_buffer_object = GL_TRUE;*/
   ctx->Extensions.EXT_shadow_funcs = GL_TRUE;
}


/**
 * Enable all OpenGL 2.0 features and extensions.
 * A convenience function to be called by drivers.
 */
void
_mesa_enable_2_0_extensions(GLcontext *ctx)
{
   /*ctx->Extensions.ARB_draw_buffers = GL_TRUE;*/
#if FEATURE_ARB_fragment_shader
   ctx->Extensions.ARB_fragment_shader = GL_TRUE;
#endif
   ctx->Extensions.ARB_point_sprite = GL_TRUE;
   ctx->Extensions.EXT_blend_equation_separate = GL_TRUE;
   ctx->Extensions.ARB_texture_non_power_of_two = GL_TRUE;
#if FEATURE_ARB_shader_objects
   ctx->Extensions.ARB_shader_objects = GL_TRUE;
#endif
#if FEATURE_ARB_shading_language_100
   ctx->Extensions.ARB_shading_language_100 = GL_TRUE;
#endif
   ctx->Extensions.EXT_stencil_two_side = GL_TRUE;
#if FEATURE_ARB_vertex_shader
   ctx->Extensions.ARB_vertex_shader = GL_TRUE;
#endif
}


/**
 * Enable all OpenGL 2.1 features and extensions.
 * A convenience function to be called by drivers.
 */
void
_mesa_enable_2_1_extensions(GLcontext *ctx)
{
#if FEATURE_EXT_pixel_buffer_object
   ctx->Extensions.EXT_pixel_buffer_object = GL_TRUE;
#endif
#if FEATURE_EXT_texture_sRGB
   ctx->Extensions.EXT_texture_sRGB = GL_TRUE;
#endif
}


/**
 * Either enable or disable the named extension.
 * \return GL_TRUE for success, GL_FALSE if invalid extension name
 */
static GLboolean
set_extension( GLcontext *ctx, const char *name, GLboolean state )
{
   GLboolean *base = (GLboolean *) &ctx->Extensions;
   GLuint i;

   if (ctx->Extensions.String) {
      /* The string was already queried - can't change it now! */
      _mesa_problem(ctx, "Trying to enable/disable extension after glGetString(GL_EXTENSIONS): %s", name);
      return GL_FALSE;
   }

   for (i = 0 ; i < Elements(default_extensions) ; i++) {
      if (strcmp(default_extensions[i].name, name) == 0) {
         if (default_extensions[i].flag_offset) {
            GLboolean *enabled = base + default_extensions[i].flag_offset;
            *enabled = state;
         }
         return GL_TRUE;
      }
   }
   return GL_FALSE;
}


/**
 * Enable the named extension.
 * Typically called by drivers.
 */
void
_mesa_enable_extension( GLcontext *ctx, const char *name )
{
   if (!set_extension(ctx, name, GL_TRUE))
      _mesa_problem(ctx, "Trying to enable unknown extension: %s", name);
}


/**
 * Disable the named extension.
 * XXX is this really needed???
 */
void
_mesa_disable_extension( GLcontext *ctx, const char *name )
{
   if (!set_extension(ctx, name, GL_FALSE))
      _mesa_problem(ctx, "Trying to disable unknown extension: %s", name);
}


/**
 * Check if the i-th extension is enabled.
 */
static GLboolean
extension_enabled(GLcontext *ctx, GLuint index)
{
   const GLboolean *base = (const GLboolean *) &ctx->Extensions;
   if (!default_extensions[index].flag_offset ||
       *(base + default_extensions[index].flag_offset)) {
      return GL_TRUE;
   }
   else {
      return GL_FALSE;
   }
}


/**
 * Test if the named extension is enabled in this context.
 */
GLboolean
_mesa_extension_is_enabled( GLcontext *ctx, const char *name )
{
   GLuint i;

   for (i = 0 ; i < Elements(default_extensions) ; i++) {
      if (strcmp(default_extensions[i].name, name) == 0) {
         return extension_enabled(ctx, i);
      }
   }
   return GL_FALSE;
}


/**
 * Append string 'b' onto string 'a'.  Free 'a' and return new string.
 */
static char *
append(const char *a, const char *b)
{
   const GLuint aLen = a ? strlen(a) : 0;
   const GLuint bLen = b ? strlen(b) : 0;
   char *s = calloc(1, aLen + bLen + 1);
   if (s) {
      if (a)
         memcpy(s, a, aLen);
      if (b)
         memcpy(s + aLen, b, bLen);
      s[aLen + bLen] = '\0';
   }
   if (a)
      free((void *) a);
   return s;
}


/**
 * Check the MESA_EXTENSION_OVERRIDE env var.
 * For extension names that are recognized, turn them on.  For extension
 * names that are recognized and prefixed with '-', turn them off.
 * Return a string of the unknown/leftover names.
 */
static const char *
get_extension_override( GLcontext *ctx )
{
   const char *envExt = _mesa_getenv("MESA_EXTENSION_OVERRIDE");
   char *extraExt = NULL;
   char ext[1000];
   GLuint extLen = 0;
   GLuint i;
   GLboolean disableExt = GL_FALSE;

   if (!envExt)
      return NULL;

   for (i = 0; ; i++) {
      if (envExt[i] == '\0' || envExt[i] == ' ') {
         /* terminate/process 'ext' if extLen > 0 */
         if (extLen > 0) {
            assert(extLen < sizeof(ext));
            /* enable extension named by 'ext' */
            ext[extLen] = 0;
            if (!set_extension(ctx, ext, !disableExt)) {
               /* unknown extension name, append it to extraExt */
               if (extraExt) {
                  extraExt = append(extraExt, " ");
               }
               extraExt = append(extraExt, ext);
            }
            extLen = 0;
            disableExt = GL_FALSE;
         }
         if (envExt[i] == '\0')
            break;
      }
      else if (envExt[i] == '-') {
         disableExt = GL_TRUE;
      }
      else {
         /* accumulate this non-space character */
         ext[extLen++] = envExt[i];
      }
   }

   return extraExt;
}


/**
 * Run through the default_extensions array above and set the
 * ctx->Extensions.ARB/EXT_* flags accordingly.
 * To be called during context initialization.
 */
void
_mesa_init_extensions( GLcontext *ctx )
{
   GLboolean *base = (GLboolean *) &ctx->Extensions;
   GLuint i;

   for (i = 0 ; i < Elements(default_extensions) ; i++) {
      if (default_extensions[i].enabled &&
          default_extensions[i].flag_offset) {
         *(base + default_extensions[i].flag_offset) = GL_TRUE;
      }
   }
}


/**
 * Construct the GL_EXTENSIONS string.  Called the first time that
 * glGetString(GL_EXTENSIONS) is called.
 */
static GLubyte *
compute_extensions( GLcontext *ctx )
{
   const char *extraExt = get_extension_override(ctx);
   GLuint extStrLen = 0;
   char *s;
   GLuint i;

   /* first, compute length of the extension string */
   for (i = 0 ; i < Elements(default_extensions) ; i++) {
      if (extension_enabled(ctx, i)) {
         extStrLen += (GLuint) strlen(default_extensions[i].name) + 1;
      }
   }

   if (extraExt)
      extStrLen += strlen(extraExt) + 1; /* +1 for space */

   /* allocate the extension string */
   s = (char *) malloc(extStrLen);
   if (!s)
      return NULL;

   /* second, build the extension string */
   extStrLen = 0;
   for (i = 0 ; i < Elements(default_extensions) ; i++) {
      if (extension_enabled(ctx, i)) {
         GLuint len = (GLuint) strlen(default_extensions[i].name);
         memcpy(s + extStrLen, default_extensions[i].name, len);
         extStrLen += len;
         s[extStrLen] = ' ';
         extStrLen++;
      }
   }
   ASSERT(extStrLen > 0);

   s[extStrLen - 1] = 0; /* -1 to overwrite trailing the ' ' */

   if (extraExt) {
      s = append(s, " ");
      s = append(s, extraExt);
   }

   return (GLubyte *) s;
}

static size_t
append_extension(GLubyte **str, const char *ext)
{
   GLubyte *s = *str;
   size_t len = strlen(ext);

   if (s) {
      memcpy(s, ext, len);
      s[len++] = ' ';
      s[len] = '\0';

      *str += len;
   }
   else {
      len++;
   }

   return len;
}


static size_t
make_extension_string_es1(const GLcontext *ctx, GLubyte *str)
{
   size_t len = 0;

   /* Core additions */
   len += append_extension(&str, "GL_OES_byte_coordinates");
   len += append_extension(&str, "GL_OES_fixed_point");
   len += append_extension(&str, "GL_OES_single_precision");
   len += append_extension(&str, "GL_OES_matrix_get");

   /* 1.1 required extensions */
   len += append_extension(&str, "GL_OES_read_format");
   len += append_extension(&str, "GL_OES_compressed_paletted_texture");
   len += append_extension(&str, "GL_OES_point_size_array");
   len += append_extension(&str, "GL_OES_point_sprite");

   /* 1.1 deprecated extensions */
   len += append_extension(&str, "GL_OES_query_matrix");

#if FEATURE_OES_draw_texture
   if (ctx->Extensions.OES_draw_texture)
      len += append_extension(&str, "GL_OES_draw_texture");
#endif

   if (ctx->Extensions.EXT_blend_equation_separate)
      len += append_extension(&str, "GL_OES_blend_equation_separate");
   if (ctx->Extensions.EXT_blend_func_separate)
      len += append_extension(&str, "GL_OES_blend_func_separate");
   if (ctx->Extensions.EXT_blend_subtract)
      len += append_extension(&str, "GL_OES_blend_subtract");

   if (ctx->Extensions.EXT_stencil_wrap)
      len += append_extension(&str, "GL_OES_stencil_wrap");

   if (ctx->Extensions.ARB_texture_cube_map)
      len += append_extension(&str, "GL_OES_texture_cube_map");
   if (ctx->Extensions.ARB_texture_env_crossbar)
      len += append_extension(&str, "GL_OES_texture_env_crossbar");
   if (ctx->Extensions.ARB_texture_mirrored_repeat)
      len += append_extension(&str, "GL_OES_texture_mirrored_repeat");

   if (ctx->Extensions.ARB_framebuffer_object) {
      len += append_extension(&str, "GL_OES_framebuffer_object");
      len += append_extension(&str, "GL_OES_depth24");
      len += append_extension(&str, "GL_OES_depth32");
      len += append_extension(&str, "GL_OES_fbo_render_mipmap");
      len += append_extension(&str, "GL_OES_rgb8_rgba8");
      len += append_extension(&str, "GL_OES_stencil1");
      len += append_extension(&str, "GL_OES_stencil4");
      len += append_extension(&str, "GL_OES_stencil8");
   }

   if (ctx->Extensions.EXT_vertex_array)
      len += append_extension(&str, "GL_OES_element_index_uint");
   if (ctx->Extensions.ARB_vertex_buffer_object)
      len += append_extension(&str, "GL_OES_mapbuffer");
   if (ctx->Extensions.EXT_texture_filter_anisotropic)
      len += append_extension(&str, "GL_EXT_texture_filter_anisotropic");

   /* some applications check this for NPOT support */
   if (ctx->Extensions.ARB_texture_non_power_of_two)
      len += append_extension(&str, "GL_ARB_texture_non_power_of_two");

   if (ctx->Extensions.EXT_texture_compression_s3tc)
      len += append_extension(&str, "GL_EXT_texture_compression_dxt1");
   if (ctx->Extensions.EXT_texture_lod_bias)
      len += append_extension(&str, "GL_EXT_texture_lod_bias");
   if (ctx->Extensions.EXT_blend_minmax)
      len += append_extension(&str, "GL_EXT_blend_minmax");
   if (ctx->Extensions.EXT_multi_draw_arrays)
      len += append_extension(&str, "GL_EXT_multi_draw_arrays");

#if FEATURE_OES_EGL_image
   if (ctx->Extensions.OES_EGL_image)
      len += append_extension(&str, "GL_OES_EGL_image");
#endif

   return len;
}


static GLubyte *
compute_extensions_es1(const GLcontext *ctx)
{
   GLubyte *s;
   unsigned int len;
   
   len = make_extension_string_es1(ctx, NULL);
   s = malloc(len + 1);
   if (!s)
      return NULL;
   make_extension_string_es1(ctx, s);
   
   return s;
}

static size_t
make_extension_string_es2(const GLcontext *ctx, GLubyte *str)
{
   size_t len = 0;

   len += append_extension(&str, "GL_OES_compressed_paletted_texture");

   if (ctx->Extensions.ARB_framebuffer_object) {
      len += append_extension(&str, "GL_OES_depth24");
      len += append_extension(&str, "GL_OES_depth32");
      len += append_extension(&str, "GL_OES_fbo_render_mipmap");
      len += append_extension(&str, "GL_OES_rgb8_rgba8");
      len += append_extension(&str, "GL_OES_stencil1");
      len += append_extension(&str, "GL_OES_stencil4");
   }

   if (ctx->Extensions.EXT_vertex_array)
      len += append_extension(&str, "GL_OES_element_index_uint");
   if (ctx->Extensions.ARB_vertex_buffer_object)
      len += append_extension(&str, "GL_OES_mapbuffer");

   if (ctx->Extensions.EXT_texture3D)
      len += append_extension(&str, "GL_OES_texture_3D");
   if (ctx->Extensions.ARB_texture_non_power_of_two)
      len += append_extension(&str, "GL_OES_texture_npot");
   if (ctx->Extensions.EXT_texture_filter_anisotropic)
      len += append_extension(&str, "GL_EXT_texture_filter_anisotropic");

   len += append_extension(&str, "GL_EXT_texture_type_2_10_10_10_REV");
   if (ctx->Extensions.ARB_depth_texture)
      len += append_extension(&str, "GL_OES_depth_texture");
   if (ctx->Extensions.EXT_packed_depth_stencil)
      len += append_extension(&str, "GL_OES_packed_depth_stencil");
   if (ctx->Extensions.ARB_fragment_shader)
      len += append_extension(&str, "GL_OES_standard_derivatives");

   if (ctx->Extensions.EXT_texture_compression_s3tc)
      len += append_extension(&str, "GL_EXT_texture_compression_dxt1");
   if (ctx->Extensions.EXT_blend_minmax)
      len += append_extension(&str, "GL_EXT_blend_minmax");
   if (ctx->Extensions.EXT_multi_draw_arrays)
      len += append_extension(&str, "GL_EXT_multi_draw_arrays");

#if FEATURE_OES_EGL_image
   if (ctx->Extensions.OES_EGL_image)
      len += append_extension(&str, "GL_OES_EGL_image");
#endif

   return len;
}

static GLubyte *
compute_extensions_es2(GLcontext *ctx)
{
   GLubyte *s;
   unsigned int len;

   len = make_extension_string_es2(ctx, NULL);
   s = malloc(len + 1);
   if (!s)
      return NULL;
   make_extension_string_es2(ctx, s);
   
   return s;
}


GLubyte *
_mesa_make_extension_string(GLcontext *ctx)
{
   switch (ctx->API) {
   case API_OPENGL:
      return compute_extensions(ctx);
   case API_OPENGLES2:
      return compute_extensions_es2(ctx);
   case API_OPENGLES:
      return compute_extensions_es1(ctx);
   default:
      assert(0);
      return NULL;
   }
}

/**
 * Return number of enabled extensions.
 */
GLuint
_mesa_get_extension_count(GLcontext *ctx)
{
   GLuint i;

   /* only count once */
   if (!ctx->Extensions.Count) {
      for (i = 0; i < Elements(default_extensions); i++) {
         if (extension_enabled(ctx, i)) {
            ctx->Extensions.Count++;
         }
      }
   }

   if (0)
      _mesa_debug(ctx, "%u of %d extensions enabled\n", ctx->Extensions.Count,
                  (int) Elements(default_extensions));

   return ctx->Extensions.Count;
}


/**
 * Return name of i-th enabled extension
 */
const GLubyte *
_mesa_get_enabled_extension(GLcontext *ctx, GLuint index)
{
   GLuint i;

   for (i = 0; i < Elements(default_extensions); i++) {
      if (extension_enabled(ctx, i)) {
         if (index == 0)
            return (const GLubyte *) default_extensions[i].name;
         index--;
      }
   }

   return NULL;
}
