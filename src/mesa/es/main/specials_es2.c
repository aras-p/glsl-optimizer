/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
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
 * TUNGSTEN GRAPHICS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 **************************************************************************/


#include "main/mtypes.h"
#include "main/context.h"
#include "main/imports.h"
#include "main/get.h"


const GLubyte * GLAPIENTRY _es_GetString(GLenum name);


static const GLubyte *
compute_es_version(void)
{
   GET_CURRENT_CONTEXT(ctx);
   static const char es_2_0[] = "OpenGL ES 2.0";
   /* OpenGL ES 2.0 is derived from OpenGL 2.0 */
   const GLboolean ver_2_0 = (ctx->Extensions.ARB_multisample &&
                              ctx->Extensions.ARB_multitexture &&
                              ctx->Extensions.ARB_texture_compression &&
                              ctx->Extensions.ARB_texture_cube_map &&
                              ctx->Extensions.ARB_texture_mirrored_repeat &&
                              ctx->Extensions.EXT_blend_color &&
                              ctx->Extensions.EXT_blend_func_separate &&
                              ctx->Extensions.EXT_blend_minmax &&
                              ctx->Extensions.EXT_blend_subtract &&
                              ctx->Extensions.EXT_stencil_wrap &&
                              ctx->Extensions.ARB_vertex_buffer_object &&
                              ctx->Extensions.ARB_shader_objects &&
                              ctx->Extensions.ARB_vertex_shader &&
                              ctx->Extensions.ARB_fragment_shader &&
                              ctx->Extensions.ARB_texture_non_power_of_two &&
                              ctx->Extensions.EXT_blend_equation_separate);
   if (!ver_2_0)
      _mesa_problem(ctx, "Incomplete OpenGL ES 2.0 support.");
   return (const GLubyte *) es_2_0;
}


static size_t
append_extension(char **str, const char *ext)
{
   char *s = *str;
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
make_extension_string(const GLcontext *ctx, char *str)
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

   return len;
}


static const GLubyte *
compute_es_extensions(void)
{
   GET_CURRENT_CONTEXT(ctx);

   if (!ctx->Extensions.String) {
      char *s;
      unsigned int len;

      len = make_extension_string(ctx, NULL);
      s = (char *) malloc(len + 1);
      if (!s)
         return NULL;
      make_extension_string(ctx, s);
      ctx->Extensions.String = (const GLubyte *) s;
   }

   return ctx->Extensions.String;
}

const GLubyte * GLAPIENTRY
_es_GetString(GLenum name)
{
   switch (name) {
   case GL_VERSION:
      return compute_es_version();
   case GL_SHADING_LANGUAGE_VERSION:
      return (const GLubyte *) "OpenGL ES GLSL ES 1.0.16";
   case GL_EXTENSIONS:
      return compute_es_extensions();
   default:
      return _mesa_GetString(name);
   }
}


void
_mesa_initialize_context_extra(GLcontext *ctx)
{
   ctx->FragmentProgram._MaintainTexEnvProgram = GL_TRUE;
   ctx->VertexProgram._MaintainTnlProgram = GL_TRUE;

   ctx->Point.PointSprite = GL_TRUE;  /* always on for ES 2.x */
}
