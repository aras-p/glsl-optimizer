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
#include "main/mfeatures.h"

#include "intel_context.h"
#include "intel_extensions.h"

static const char *common_extensions[] = {
   /* Used by mesa internally (cf all_mesa_extensions in ../common/utils.c) */
   "GL_ARB_transpose_matrix",
   "GL_ARB_window_pos",
   "GL_EXT_blend_func_separate",
   "GL_EXT_compiled_vertex_array",
   "GL_EXT_framebuffer_blit",
   "GL_IBM_multimode_draw_arrays",
   "GL_MESA_window_pos",
   "GL_NV_vertex_program",

   /* Optional GLES1 or GLES2 */
#if FEATURE_OES_EGL_image
   "GL_OES_EGL_image",
#endif
   "GL_EXT_texture_filter_anisotropic",
   "GL_EXT_packed_depth_stencil",
   "GL_EXT_blend_minmax",

   NULL
};

static const char *es1_extensions[] = {
   /* Required by GLES1 */
   "GL_ARB_multitexture",
   "GL_ARB_texture_env_add",
   "GL_ARB_texture_env_combine",
   "GL_ARB_texture_env_dot3",
   "GL_ARB_point_parameters",

   /* Optional GLES1 */
   "GL_EXT_blend_equation_separate",
   "GL_EXT_blend_func_separate",
   "GL_EXT_blend_subtract",
   "GL_OES_draw_texture",
   "GL_EXT_framebuffer_object",
   "GL_ARB_point_sprite",
   "GL_EXT_stencil_wrap",
   "GL_ARB_texture_cube_map",
   "GL_ARB_texture_env_crossbar",
   "GL_ARB_texture_mirrored_repeat",
   "GL_EXT_texture_lod_bias",

   NULL
};

static const char *es2_extensions[] = {
   /* Required by GLES2 */
   "GL_ARB_fragment_program",
   "GL_ARB_fragment_shader",
   "GL_ARB_shader_objects",
   "GL_ARB_texture_cube_map",
   "GL_ARB_texture_non_power_of_two",
   "GL_ARB_vertex_shader",
   "GL_EXT_blend_color",
   "GL_EXT_blend_equation_separate",
   "GL_EXT_blend_minmax",
   "GL_NV_blend_square",

   /* Optional GLES2 */
   "GL_ARB_depth_texture",
   "GL_EXT_framebuffer_object",

   NULL,
};

void
intelInitExtensionsES1(struct gl_context *ctx)
{
   int i;

   for (i = 0; common_extensions[i]; i++)
      _mesa_enable_extension(ctx, common_extensions[i]);
   for (i = 0; es1_extensions[i]; i++)
      _mesa_enable_extension(ctx, es1_extensions[i]);
}

/**
 * Initializes potential list of extensions if ctx == NULL, or actually enables
 * extensions for a context.
 */
void
intelInitExtensionsES2(struct gl_context *ctx)
{
   int i;
   struct intel_context *intel = intel_context(ctx);

   for (i = 0; common_extensions[i]; i++)
      _mesa_enable_extension(ctx, common_extensions[i]);
   for (i = 0; es2_extensions[i]; i++)
      _mesa_enable_extension(ctx, es2_extensions[i]);

   /* This extension must be manually disabled on GEN3 because it may have
    * been enabled by default.
    */
   if (intel->gen < 4) {
      ctx->Extensions.OES_standard_derivatives = false;
   }
}
