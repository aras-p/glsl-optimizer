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


extern const GLubyte * GLAPIENTRY _es_GetString(GLenum name);


static const GLubyte *
compute_es_version(void)
{
   GET_CURRENT_CONTEXT(ctx);
   static const char es_1_0[] = "OpenGL ES-CM 1.0";
   static const char es_1_1[] = "OpenGL ES-CM 1.1";
   /* OpenGL ES 1.0 is derived from OpenGL 1.3 */
   const GLboolean ver_1_0 = (ctx->Extensions.ARB_multisample &&
                              ctx->Extensions.ARB_multitexture &&
                              ctx->Extensions.ARB_texture_compression &&
                              ctx->Extensions.EXT_texture_env_add &&
                              ctx->Extensions.ARB_texture_env_combine &&
                              ctx->Extensions.ARB_texture_env_dot3);
   /* OpenGL ES 1.1 is derived from OpenGL 1.5 */
   const GLboolean ver_1_1 = (ver_1_0 &&
                              ctx->Extensions.EXT_point_parameters &&
                              ctx->Extensions.SGIS_generate_mipmap &&
                              ctx->Extensions.ARB_vertex_buffer_object);
   if (ver_1_1)
      return (const GLubyte *) es_1_1;

   if (!ver_1_0)
      _mesa_problem(ctx, "Incomplete OpenGL ES 1.0 support.");
   return (const GLubyte *) es_1_0;
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
   case GL_EXTENSIONS:
      return compute_es_extensions();
   default:
      return _mesa_GetString(name);
   }
}


void
_mesa_initialize_context_extra(GLcontext *ctx)
{
   GLuint i;

   /**
    * GL_OES_texture_cube_map says
    * "Initially all texture generation modes are set to REFLECTION_MAP_OES"
    */
   for (i = 0; i < MAX_TEXTURE_UNITS; i++) {
      struct gl_texture_unit *texUnit = &ctx->Texture.Unit[i];
      texUnit->GenS.Mode = GL_REFLECTION_MAP_NV;
      texUnit->GenT.Mode = GL_REFLECTION_MAP_NV;
      texUnit->GenR.Mode = GL_REFLECTION_MAP_NV;
      texUnit->GenS._ModeBit = TEXGEN_REFLECTION_MAP_NV;
      texUnit->GenT._ModeBit = TEXGEN_REFLECTION_MAP_NV;
      texUnit->GenR._ModeBit = TEXGEN_REFLECTION_MAP_NV;
   }
}
