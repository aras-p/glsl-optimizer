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

#include "main/extensions.h"

#include "intel_extensions.h"

static const char *es2_extensions[] = {
   /* Used by mesa internally (cf all_mesa_extensions in ../common/utils.c) */
   "GL_ARB_draw_buffers",
   "GL_ARB_multisample",
   "GL_ARB_texture_compression",
   "GL_ARB_transpose_matrix",
   "GL_ARB_vertex_buffer_object",
   "GL_ARB_window_pos",
   "GL_EXT_blend_func_separate",
   "GL_EXT_compiled_vertex_array",
   "GL_EXT_framebuffer_blit",
   "GL_EXT_multi_draw_arrays",
   "GL_EXT_polygon_offset",
   "GL_EXT_texture_object",
   "GL_EXT_vertex_array",
   "GL_IBM_multimode_draw_arrays",
   "GL_MESA_window_pos",
   "GL_NV_vertex_program",

   /* Required by GLES2 */
   "GL_ARB_fragment_program",
   "GL_ARB_fragment_shader",
   "GL_ARB_multitexture",
   "GL_ARB_shader_objects",
   "GL_ARB_texture_cube_map",
   "GL_ARB_texture_mirrored_repeat",
   "GL_ARB_texture_non_power_of_two",
   "GL_ARB_vertex_shader",
   "GL_EXT_blend_color",
   "GL_EXT_blend_equation_separate",
   "GL_EXT_blend_minmax",
   "GL_EXT_blend_subtract",
   "GL_EXT_stencil_wrap",

   /* Optional GLES2 */
   "GL_ARB_framebuffer_object",
   "GL_EXT_texture_filter_anisotropic",
   "GL_ARB_depth_texture",
   "GL_EXT_packed_depth_stencil",
   "GL_EXT_framebuffer_object",
   "GL_EXT_texture_format_BGRA8888",

#if FEATURE_OES_EGL_image
   "GL_OES_EGL_image",
#endif

   NULL,
};

/**
 * Initializes potential list of extensions if ctx == NULL, or actually enables
 * extensions for a context.
 */
void
intelInitExtensionsES2(GLcontext *ctx)
{
   int i;

   /* Can't use driInitExtensions() since it uses extensions from
    * main/remap_helper.h when called the first time. */

   for (i = 0; es2_extensions[i]; i++)
      _mesa_enable_extension(ctx, es2_extensions[i]);
}
