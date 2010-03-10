/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2010  VMware, Inc.  All Rights Reserved.
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
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include "context.h"
#include "version.h"



/**
 * Examine enabled GL extensions to determine GL version.
 * Return major and minor version numbers.
 */
static void
compute_version(const GLcontext *ctx, GLuint *major, GLuint *minor)
{
   const GLboolean ver_1_3 = (ctx->Extensions.ARB_multisample &&
                              ctx->Extensions.ARB_multitexture &&
                              ctx->Extensions.ARB_texture_border_clamp &&
                              ctx->Extensions.ARB_texture_compression &&
                              ctx->Extensions.ARB_texture_cube_map &&
                              ctx->Extensions.EXT_texture_env_add &&
                              ctx->Extensions.ARB_texture_env_combine &&
                              ctx->Extensions.ARB_texture_env_dot3);
   const GLboolean ver_1_4 = (ver_1_3 &&
                              ctx->Extensions.ARB_depth_texture &&
                              ctx->Extensions.ARB_shadow &&
                              ctx->Extensions.ARB_texture_env_crossbar &&
                              ctx->Extensions.ARB_texture_mirrored_repeat &&
                              ctx->Extensions.ARB_window_pos &&
                              ctx->Extensions.EXT_blend_color &&
                              ctx->Extensions.EXT_blend_func_separate &&
                              ctx->Extensions.EXT_blend_minmax &&
                              ctx->Extensions.EXT_blend_subtract &&
                              ctx->Extensions.EXT_fog_coord &&
                              ctx->Extensions.EXT_multi_draw_arrays &&
                              ctx->Extensions.EXT_point_parameters &&
                              ctx->Extensions.EXT_secondary_color &&
                              ctx->Extensions.EXT_stencil_wrap &&
                              ctx->Extensions.EXT_texture_lod_bias &&
                              ctx->Extensions.SGIS_generate_mipmap);
   const GLboolean ver_1_5 = (ver_1_4 &&
                              ctx->Extensions.ARB_occlusion_query &&
                              ctx->Extensions.ARB_vertex_buffer_object &&
                              ctx->Extensions.EXT_shadow_funcs);
   const GLboolean ver_2_0 = (ver_1_5 &&
                              ctx->Extensions.ARB_draw_buffers &&
                              ctx->Extensions.ARB_point_sprite &&
                              ctx->Extensions.ARB_shader_objects &&
                              ctx->Extensions.ARB_vertex_shader &&
                              ctx->Extensions.ARB_fragment_shader &&
                              ctx->Extensions.ARB_texture_non_power_of_two &&
                              ctx->Extensions.EXT_blend_equation_separate &&

			      /* Technically, 2.0 requires the functionality
			       * of the EXT version.  Enable 2.0 if either
			       * extension is available, and assume that a
			       * driver that only exposes the ATI extension
			       * will fallback to software when necessary.
			       */
			      (ctx->Extensions.EXT_stencil_two_side
			       || ctx->Extensions.ATI_separate_stencil));
   const GLboolean ver_2_1 = (ver_2_0 &&
                              ctx->Extensions.ARB_shading_language_120 &&
                              ctx->Extensions.EXT_pixel_buffer_object &&
                              ctx->Extensions.EXT_texture_sRGB);
   if (ver_2_1) {
      *major = 2;
      *minor = 1;
   }
   else if (ver_2_0) {
      *major = 2;
      *minor = 0;
   }
   else if (ver_1_5) {
      *major = 1;
      *minor = 5;
   }
   else if (ver_1_4) {
      *major = 1;
      *minor = 4;
   }
   else if (ver_1_3) {
      *major = 1;
      *minor = 3;
   }
   else {
      *major = 1;
      *minor = 2;
   }
}


/**
 * Set the context's VersionMajor, VersionMinor, VersionString fields.
 * This should only be called once as part of context initialization.
 */
void
_mesa_compute_version(GLcontext *ctx)
{
   static const int max = 100;

   compute_version(ctx, &ctx->VersionMajor, &ctx->VersionMinor);
   
   ctx->VersionString = (char *) malloc(max);
   if (ctx->VersionString) {
      _mesa_snprintf(ctx->VersionString, max, "%u.%u Mesa " MESA_VERSION_STRING,
	       ctx->VersionMajor, ctx->VersionMinor);
   }
}
