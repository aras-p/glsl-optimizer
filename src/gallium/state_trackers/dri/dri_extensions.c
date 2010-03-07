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

#include "dri_screen.h"
#include "dri_context.h"
#include "state_tracker/st_context.h"

#define need_GL_ARB_map_buffer_range
#define need_GL_ARB_multisample
#define need_GL_ARB_occlusion_query
#define need_GL_ARB_point_parameters
#define need_GL_ARB_provoking_vertex
#define need_GL_ARB_shader_objects
#define need_GL_ARB_texture_compression
#define need_GL_ARB_vertex_array_object
#define need_GL_ARB_vertex_buffer_object
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
#define need_GL_EXT_multi_draw_arrays
#define need_GL_EXT_provoking_vertex
#define need_GL_EXT_secondary_color
#define need_GL_EXT_stencil_two_side
#define need_GL_APPLE_vertex_array_object
#define need_GL_NV_vertex_program
#define need_GL_VERSION_2_0
#define need_GL_VERSION_2_1
#include "main/remap_helper.h"
#include "utils.h"

/**
 * Extension strings exported by the driver.
 */
static const struct dri_extension card_extensions[] = {
   {"GL_ARB_fragment_shader", NULL},
   {"GL_ARB_map_buffer_range", GL_ARB_map_buffer_range_functions},
   {"GL_ARB_multisample", GL_ARB_multisample_functions},
   {"GL_ARB_multitexture", NULL},
   {"GL_ARB_occlusion_query", GL_ARB_occlusion_query_functions},
   {"GL_ARB_pixel_buffer_object", NULL},
   {"GL_ARB_provoking_vertex", GL_ARB_provoking_vertex_functions},
   {"GL_ARB_point_parameters", GL_ARB_point_parameters_functions},
   {"GL_ARB_shading_language_100", GL_VERSION_2_0_functions },
   {"GL_ARB_shading_language_120", GL_VERSION_2_1_functions },
   {"GL_ARB_shader_objects", GL_ARB_shader_objects_functions},
   {"GL_ARB_texture_border_clamp", NULL},
   {"GL_ARB_texture_compression", GL_ARB_texture_compression_functions},
   {"GL_ARB_texture_cube_map", NULL},
   {"GL_ARB_texture_env_add", NULL},
   {"GL_ARB_texture_env_combine", NULL},
   {"GL_ARB_texture_env_dot3", NULL},
   {"GL_ARB_texture_mirrored_repeat", NULL},
   {"GL_ARB_texture_non_power_of_two", NULL},
   {"GL_ARB_texture_rectangle", NULL},
   {"GL_ARB_vertex_array_object", GL_ARB_vertex_array_object_functions},
   {"GL_ARB_vertex_buffer_object", GL_ARB_vertex_buffer_object_functions},
   {"GL_ARB_vertex_shader", GL_ARB_vertex_shader_functions},
   {"GL_ARB_vertex_program", GL_ARB_vertex_program_functions},
   {"GL_ARB_window_pos", GL_ARB_window_pos_functions},
   {"GL_EXT_blend_color", GL_EXT_blend_color_functions},
   {"GL_EXT_blend_equation_separate", GL_EXT_blend_equation_separate_functions},
   {"GL_EXT_blend_func_separate", GL_EXT_blend_func_separate_functions},
   {"GL_EXT_blend_minmax", GL_EXT_blend_minmax_functions},
   {"GL_EXT_blend_subtract", NULL},
   {"GL_EXT_cull_vertex", GL_EXT_cull_vertex_functions},
   {"GL_EXT_draw_buffers2", GL_EXT_draw_buffers2_functions},
   {"GL_EXT_fog_coord", GL_EXT_fog_coord_functions},
   {"GL_EXT_framebuffer_object", GL_EXT_framebuffer_object_functions},
   {"GL_EXT_multi_draw_arrays", GL_EXT_multi_draw_arrays_functions},
   {"GL_EXT_packed_depth_stencil", NULL},
   {"GL_EXT_pixel_buffer_object", NULL},
   {"GL_EXT_provoking_vertex", GL_EXT_provoking_vertex_functions},
   {"GL_EXT_secondary_color", GL_EXT_secondary_color_functions},
   {"GL_EXT_stencil_two_side", GL_EXT_stencil_two_side_functions},
   {"GL_EXT_stencil_wrap", NULL},
   {"GL_EXT_texture_edge_clamp", NULL},
   {"GL_EXT_texture_env_combine", NULL},
   {"GL_EXT_texture_env_dot3", NULL},
   {"GL_EXT_texture_filter_anisotropic", NULL},
   {"GL_EXT_texture_lod_bias", NULL},
   {"GL_3DFX_texture_compression_FXT1", NULL},
   {"GL_APPLE_client_storage", NULL},
   {"GL_APPLE_vertex_array_object", GL_APPLE_vertex_array_object_functions},
   {"GL_MESA_pack_invert", NULL},
   {"GL_MESA_ycbcr_texture", NULL},
   {"GL_NV_blend_square", NULL},
   {"GL_NV_vertex_program", GL_NV_vertex_program_functions},
   {"GL_NV_vertex_program1_1", NULL},
   {"GL_SGIS_generate_mipmap", NULL},
   {NULL, NULL}
};

void
dri_init_extensions(struct dri_context *ctx)
{
   /* The card_extensions list should be pruned according to the
    * capabilities of the pipe_screen. This is actually something
    * that can/should be done inside st_create_context().
    * XXX Not pruning is very bogus. Always all these extensions above
    * will be advertized, regardless what st_init_extensions
    * (which depends on the pipe cap bits) does.
    */
   driInitExtensions(ctx->st->ctx, card_extensions, GL_TRUE);
}

/* vim: set sw=3 ts=8 sts=3 expandtab: */
