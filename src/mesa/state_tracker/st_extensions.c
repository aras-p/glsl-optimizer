/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "main/imports.h"
#include "main/context.h"
#include "main/extensions.h"
#include "main/macros.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_screen.h"

#include "st_context.h"
#include "st_extensions.h"


static int _min(int a, int b)
{
   return (a < b) ? a : b;
}

static float _maxf(float a, float b)
{
   return (a > b) ? a : b;
}

static int _clamp(int a, int min, int max)
{
   if (a < min)
      return min;
   else if (a > max)
      return max;
   else
      return a;
}


/**
 * Query driver to get implementation limits.
 * Note that we have to limit/clamp against Mesa's internal limits too.
 */
void st_init_limits(struct st_context *st)
{
   struct pipe_screen *screen = st->pipe->screen;
   struct gl_constants *c = &st->ctx->Const;

   c->MaxTextureLevels
      = _min(screen->get_param(screen, PIPE_CAP_MAX_TEXTURE_2D_LEVELS),
            MAX_TEXTURE_LEVELS);

   c->Max3DTextureLevels
      = _min(screen->get_param(screen, PIPE_CAP_MAX_TEXTURE_3D_LEVELS),
            MAX_3D_TEXTURE_LEVELS);

   c->MaxCubeTextureLevels
      = _min(screen->get_param(screen, PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS),
            MAX_CUBE_TEXTURE_LEVELS);

   c->MaxTextureRectSize
      = _min(1 << (c->MaxTextureLevels - 1), MAX_TEXTURE_RECT_SIZE);

   c->MaxTextureUnits
      = c->MaxTextureImageUnits
      = c->MaxTextureCoordUnits
      = _min(screen->get_param(screen, PIPE_CAP_MAX_TEXTURE_IMAGE_UNITS),
            MAX_TEXTURE_IMAGE_UNITS);

   c->MaxDrawBuffers
      = _clamp(screen->get_param(screen, PIPE_CAP_MAX_RENDER_TARGETS),
              1, MAX_DRAW_BUFFERS);

   c->MaxLineWidth
      = _maxf(1.0f, screen->get_paramf(screen, PIPE_CAP_MAX_LINE_WIDTH));
   c->MaxLineWidthAA
      = _maxf(1.0f, screen->get_paramf(screen, PIPE_CAP_MAX_LINE_WIDTH_AA));

   c->MaxPointSize
      = _maxf(1.0f, screen->get_paramf(screen, PIPE_CAP_MAX_POINT_WIDTH));
   c->MaxPointSizeAA
      = _maxf(1.0f, screen->get_paramf(screen, PIPE_CAP_MAX_POINT_WIDTH_AA));

   c->MaxTextureMaxAnisotropy
      = _maxf(2.0f, screen->get_paramf(screen, PIPE_CAP_MAX_TEXTURE_ANISOTROPY));

   c->MaxTextureLodBias
      = screen->get_paramf(screen, PIPE_CAP_MAX_TEXTURE_LOD_BIAS);

   c->MaxDrawBuffers
      = CLAMP(screen->get_param(screen, PIPE_CAP_MAX_RENDER_TARGETS),
              1, MAX_DRAW_BUFFERS);
}


/**
 * XXX this needs careful review
 */
void st_init_extensions(struct st_context *st)
{
   struct pipe_screen *screen = st->pipe->screen;
   GLcontext *ctx = st->ctx;

   /*
    * Extensions that are supported by all Gallium drivers:
    */
   ctx->Extensions.ARB_multisample = GL_TRUE; /* API support */
   ctx->Extensions.ARB_fragment_program = GL_TRUE;
   ctx->Extensions.ARB_texture_border_clamp = GL_TRUE; /* XXX temp */
   ctx->Extensions.ARB_texture_compression = GL_TRUE; /* API support only */
   ctx->Extensions.ARB_texture_cube_map = GL_TRUE;
   ctx->Extensions.ARB_texture_env_combine = GL_TRUE;
   ctx->Extensions.ARB_texture_env_crossbar = GL_TRUE;
   ctx->Extensions.ARB_texture_env_dot3 = GL_TRUE;
   ctx->Extensions.ARB_vertex_program = GL_TRUE;
   ctx->Extensions.ARB_vertex_buffer_object = GL_TRUE;

   ctx->Extensions.EXT_blend_color = GL_TRUE;
   ctx->Extensions.EXT_blend_equation_separate = GL_TRUE;
   ctx->Extensions.EXT_blend_func_separate = GL_TRUE;
   ctx->Extensions.EXT_blend_logic_op = GL_TRUE;
   ctx->Extensions.EXT_blend_minmax = GL_TRUE;
   ctx->Extensions.EXT_blend_subtract = GL_TRUE;
   ctx->Extensions.EXT_framebuffer_blit = GL_TRUE;
   ctx->Extensions.EXT_framebuffer_object = GL_TRUE;
   ctx->Extensions.EXT_fog_coord = GL_TRUE;
   ctx->Extensions.EXT_multi_draw_arrays = GL_TRUE;
   ctx->Extensions.EXT_pixel_buffer_object = GL_TRUE;
   ctx->Extensions.EXT_point_parameters = GL_TRUE;
   ctx->Extensions.EXT_secondary_color = GL_TRUE;
   ctx->Extensions.EXT_stencil_wrap = GL_TRUE;
   ctx->Extensions.EXT_texture_env_add = GL_TRUE;
   ctx->Extensions.EXT_texture_env_combine = GL_TRUE;
   ctx->Extensions.EXT_texture_env_dot3 = GL_TRUE;
   ctx->Extensions.EXT_texture_lod_bias = GL_TRUE;

   ctx->Extensions.NV_blend_square = GL_TRUE;
   ctx->Extensions.NV_texgen_reflection = GL_TRUE;

   ctx->Extensions.SGI_color_matrix = GL_TRUE;
   ctx->Extensions.SGIS_generate_mipmap = GL_TRUE; /* XXX temp */

   /*
    * Extensions that depend on the driver/hardware:
    */
   if (screen->get_param(screen, PIPE_CAP_MAX_RENDER_TARGETS) > 0) {
      ctx->Extensions.ARB_draw_buffers = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_GLSL)) {
      ctx->Extensions.ARB_fragment_shader = GL_TRUE;
      ctx->Extensions.ARB_vertex_shader = GL_TRUE;
      ctx->Extensions.ARB_shader_objects = GL_TRUE;
      ctx->Extensions.ARB_shading_language_100 = GL_TRUE;
      ctx->Extensions.ARB_shading_language_120 = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_TEXTURE_MIRROR_REPEAT) > 0) {
      ctx->Extensions.ARB_texture_mirrored_repeat = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_TEXTURE_MIRROR_CLAMP) > 0) {
      ctx->Extensions.EXT_texture_mirror_clamp = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_NPOT_TEXTURES)) {
      ctx->Extensions.ARB_texture_non_power_of_two = GL_TRUE;
      ctx->Extensions.NV_texture_rectangle = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_MAX_TEXTURE_IMAGE_UNITS) > 1) {
      ctx->Extensions.ARB_multitexture = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_TWO_SIDED_STENCIL)) {
      ctx->Extensions.ATI_separate_stencil = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_ANISOTROPIC_FILTER)) {
      ctx->Extensions.EXT_texture_filter_anisotropic = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_POINT_SPRITE)) {
      ctx->Extensions.ARB_point_sprite = GL_TRUE;
      ctx->Extensions.NV_point_sprite = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_OCCLUSION_QUERY)) {
      ctx->Extensions.ARB_occlusion_query = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_TEXTURE_SHADOW_MAP)) {
      ctx->Extensions.ARB_depth_texture = GL_TRUE;
      ctx->Extensions.ARB_shadow = GL_TRUE;
      ctx->Extensions.EXT_shadow_funcs = GL_TRUE;
      /*ctx->Extensions.ARB_shadow_ambient = GL_TRUE;*/
   }

   /* GL_EXT_packed_depth_stencil requires both the ability to render to
    * a depth/stencil buffer and texture from depth/stencil source.
    */
   if (screen->is_format_supported(screen, PIPE_FORMAT_Z24S8_UNORM,
                                   PIPE_TEXTURE_2D, 
                                   PIPE_TEXTURE_USAGE_DEPTH_STENCIL, 0) &&
       screen->is_format_supported(screen, PIPE_FORMAT_Z24S8_UNORM,
                                   PIPE_TEXTURE_2D, 
                                   PIPE_TEXTURE_USAGE_SAMPLER, 0)) {
      ctx->Extensions.EXT_packed_depth_stencil = GL_TRUE;
   }
   else if (screen->is_format_supported(screen, PIPE_FORMAT_S8Z24_UNORM,
                                        PIPE_TEXTURE_2D, 
                                        PIPE_TEXTURE_USAGE_DEPTH_STENCIL, 0) &&
            screen->is_format_supported(screen, PIPE_FORMAT_S8Z24_UNORM,
                                        PIPE_TEXTURE_2D, 
                                        PIPE_TEXTURE_USAGE_SAMPLER, 0)) {
      ctx->Extensions.EXT_packed_depth_stencil = GL_TRUE;
   }

   if (screen->is_format_supported(screen, PIPE_FORMAT_R8G8B8A8_SRGB,
                                   PIPE_TEXTURE_2D, 
                                   PIPE_TEXTURE_USAGE_SAMPLER, 0)) {
      ctx->Extensions.EXT_texture_sRGB = GL_TRUE;
   }

#if 01
   if (screen->is_format_supported(screen, PIPE_FORMAT_DXT5_RGBA,
                                   PIPE_TEXTURE_2D, 
                                   PIPE_TEXTURE_USAGE_SAMPLER, 0)) {
      ctx->Extensions.EXT_texture_compression_s3tc = GL_TRUE;
   }
#endif
   if (screen->is_format_supported(screen, PIPE_FORMAT_YCBCR, 
                                   PIPE_TEXTURE_2D, 
                                   PIPE_TEXTURE_USAGE_SAMPLER, 0) ||
       screen->is_format_supported(screen, PIPE_FORMAT_YCBCR_REV, 
                                   PIPE_TEXTURE_2D, 
                                   PIPE_TEXTURE_USAGE_SAMPLER, 0)) {
      ctx->Extensions.MESA_ycbcr_texture = GL_TRUE;
   }

}
