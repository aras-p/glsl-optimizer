/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright (c) 2008 VMware, Inc.
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
#include "main/macros.h"
#include "main/mfeatures.h"

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
   gl_shader_type sh;

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

   c->MaxTextureImageUnits
      = _min(screen->get_param(screen, PIPE_CAP_MAX_TEXTURE_IMAGE_UNITS),
            MAX_TEXTURE_IMAGE_UNITS);

   c->MaxVertexTextureImageUnits
      = _min(screen->get_param(screen, PIPE_CAP_MAX_VERTEX_TEXTURE_UNITS),
             MAX_VERTEX_TEXTURE_IMAGE_UNITS);

   c->MaxCombinedTextureImageUnits
      = _min(screen->get_param(screen, PIPE_CAP_MAX_COMBINED_SAMPLERS),
             MAX_COMBINED_TEXTURE_IMAGE_UNITS);

   c->MaxTextureCoordUnits
      = _min(c->MaxTextureImageUnits, MAX_TEXTURE_COORD_UNITS);

   c->MaxTextureUnits = _min(c->MaxTextureImageUnits, c->MaxTextureCoordUnits);

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
   /* called after _mesa_create_context/_mesa_init_point, fix default user
    * settable max point size up
    */
   st->ctx->Point.MaxSize = MAX2(c->MaxPointSize, c->MaxPointSizeAA);
   /* these are not queryable. Note that GL basically mandates a 1.0 minimum
    * for non-aa sizes, but we can go down to 0.0 for aa points.
    */
   c->MinPointSize = 1.0f;
   c->MinPointSizeAA = 0.0f;

   c->MaxTextureMaxAnisotropy
      = _maxf(2.0f, screen->get_paramf(screen, PIPE_CAP_MAX_TEXTURE_ANISOTROPY));

   c->MaxTextureLodBias
      = screen->get_paramf(screen, PIPE_CAP_MAX_TEXTURE_LOD_BIAS);

   c->MaxDrawBuffers
      = CLAMP(screen->get_param(screen, PIPE_CAP_MAX_RENDER_TARGETS),
              1, MAX_DRAW_BUFFERS);

   /* Quads always follow GL provoking rules. */
   c->QuadsFollowProvokingVertexConvention = GL_FALSE;

   for (sh = 0; sh < MESA_SHADER_TYPES; ++sh) {
      struct gl_shader_compiler_options *options =
         &st->ctx->ShaderCompilerOptions[sh];
      struct gl_program_constants *pc;

      switch (sh) {
      case PIPE_SHADER_FRAGMENT:
         pc = &c->FragmentProgram;
         break;
      case PIPE_SHADER_VERTEX:
         pc = &c->VertexProgram;
         break;
      case PIPE_SHADER_GEOMETRY:
         pc = &c->GeometryProgram;
         break;
      default:
         assert(0);
         continue;
      }

      pc->MaxNativeInstructions    = screen->get_shader_param(screen, sh, PIPE_SHADER_CAP_MAX_INSTRUCTIONS);
      pc->MaxNativeAluInstructions = screen->get_shader_param(screen, sh, PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS);
      pc->MaxNativeTexInstructions = screen->get_shader_param(screen, sh, PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS);
      pc->MaxNativeTexIndirections = screen->get_shader_param(screen, sh, PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS);
      pc->MaxNativeAttribs         = screen->get_shader_param(screen, sh, PIPE_SHADER_CAP_MAX_INPUTS);
      pc->MaxNativeTemps           = screen->get_shader_param(screen, sh, PIPE_SHADER_CAP_MAX_TEMPS);
      pc->MaxNativeAddressRegs     = screen->get_shader_param(screen, sh, PIPE_SHADER_CAP_MAX_ADDRS);
      pc->MaxNativeParameters      = screen->get_shader_param(screen, sh, PIPE_SHADER_CAP_MAX_CONSTS);
      pc->MaxUniformComponents     = 4 * MIN2(pc->MaxNativeParameters, MAX_UNIFORMS);

      options->EmitNoNoise = TRUE;

      /* TODO: make these more fine-grained if anyone needs it */
      options->EmitNoIfs = !screen->get_shader_param(screen, sh, PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH);
      options->EmitNoLoops = !screen->get_shader_param(screen, sh, PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH);
      options->EmitNoFunctions = !screen->get_shader_param(screen, sh, PIPE_SHADER_CAP_SUBROUTINES);
      options->EmitNoMainReturn = !screen->get_shader_param(screen, sh, PIPE_SHADER_CAP_SUBROUTINES);

      options->EmitNoCont = !screen->get_shader_param(screen, sh, PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED);

      options->EmitNoIndirectInput = !screen->get_shader_param(screen, sh,
                                        PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR);
      options->EmitNoIndirectOutput = !screen->get_shader_param(screen, sh,
                                        PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR);
      options->EmitNoIndirectTemp = !screen->get_shader_param(screen, sh,
                                        PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR);
      options->EmitNoIndirectUniform = !screen->get_shader_param(screen, sh,
                                        PIPE_SHADER_CAP_INDIRECT_CONST_ADDR);

      if(options->EmitNoLoops)
         options->MaxUnrollIterations = MIN2(screen->get_shader_param(screen, sh, PIPE_SHADER_CAP_MAX_INSTRUCTIONS), 65536);
   }

   /* PIPE_CAP_MAX_FS_INPUTS specifies the number of COLORn + GENERICn inputs
    * and is set in MaxNativeAttribs. It's always 2 colors + N generic
    * attributes. The GLSL compiler never uses COLORn for varyings, so we
    * subtract the 2 colors to get the maximum number of varyings (generic
    * attributes) supported by a driver. */
   c->MaxVarying = screen->get_shader_param(screen, PIPE_SHADER_FRAGMENT, PIPE_SHADER_CAP_MAX_INPUTS) - 2;
   c->MaxVarying = MIN2(c->MaxVarying, MAX_VARYING);

   /* XXX we'll need a better query here someday */
   if (screen->get_param(screen, PIPE_CAP_GLSL)) {
      c->GLSLVersion = 120;
   }
}


/**
 * Use pipe_screen::get_param() to query PIPE_CAP_ values to determine
 * which GL extensions are supported.
 * Quite a few extensions are always supported because they are standard
 * features or can be built on top of other gallium features.
 * Some fine tuning may still be needed.
 */
void st_init_extensions(struct st_context *st)
{
   struct pipe_screen *screen = st->pipe->screen;
   struct gl_context *ctx = st->ctx;

   /*
    * Extensions that are supported by all Gallium drivers:
    */
   ctx->Extensions.ARB_copy_buffer = GL_TRUE;
   ctx->Extensions.ARB_draw_elements_base_vertex = GL_TRUE;
   ctx->Extensions.ARB_fragment_coord_conventions = GL_TRUE;
   ctx->Extensions.ARB_fragment_program = GL_TRUE;
   ctx->Extensions.ARB_half_float_pixel = GL_TRUE;
   ctx->Extensions.ARB_map_buffer_range = GL_TRUE;
   ctx->Extensions.ARB_multisample = GL_TRUE;
   ctx->Extensions.ARB_texture_border_clamp = GL_TRUE; /* XXX temp */
   ctx->Extensions.ARB_texture_compression = GL_TRUE;
   ctx->Extensions.ARB_texture_cube_map = GL_TRUE;
   ctx->Extensions.ARB_texture_env_combine = GL_TRUE;
   ctx->Extensions.ARB_texture_env_crossbar = GL_TRUE;
   ctx->Extensions.ARB_texture_env_dot3 = GL_TRUE;
   ctx->Extensions.ARB_vertex_array_object = GL_TRUE;
   ctx->Extensions.ARB_vertex_buffer_object = GL_TRUE;
   ctx->Extensions.ARB_vertex_program = GL_TRUE;
   ctx->Extensions.ARB_window_pos = GL_TRUE;

   ctx->Extensions.EXT_blend_color = GL_TRUE;
   ctx->Extensions.EXT_blend_func_separate = GL_TRUE;
   ctx->Extensions.EXT_blend_logic_op = GL_TRUE;
   ctx->Extensions.EXT_blend_minmax = GL_TRUE;
   ctx->Extensions.EXT_blend_subtract = GL_TRUE;
   ctx->Extensions.EXT_framebuffer_blit = GL_TRUE;
   ctx->Extensions.EXT_framebuffer_object = GL_TRUE;
   ctx->Extensions.EXT_framebuffer_multisample = GL_TRUE;
   ctx->Extensions.EXT_fog_coord = GL_TRUE;
   ctx->Extensions.EXT_gpu_program_parameters = GL_TRUE;
   ctx->Extensions.EXT_multi_draw_arrays = GL_TRUE;
   ctx->Extensions.EXT_pixel_buffer_object = GL_TRUE;
   ctx->Extensions.EXT_point_parameters = GL_TRUE;
   ctx->Extensions.EXT_provoking_vertex = GL_TRUE;
   ctx->Extensions.EXT_secondary_color = GL_TRUE;
   ctx->Extensions.EXT_stencil_wrap = GL_TRUE;
   ctx->Extensions.EXT_texture_env_add = GL_TRUE;
   ctx->Extensions.EXT_texture_env_combine = GL_TRUE;
   ctx->Extensions.EXT_texture_env_dot3 = GL_TRUE;
   ctx->Extensions.EXT_texture_lod_bias = GL_TRUE;
   ctx->Extensions.EXT_vertex_array_bgra = GL_TRUE;
   if (ctx->API == API_OPENGLES || ctx->API == API_OPENGLES2)
	   ctx->Extensions.EXT_texture_format_BGRA8888 = GL_TRUE;

   ctx->Extensions.APPLE_vertex_array_object = GL_TRUE;

   ctx->Extensions.ATI_texture_env_combine3 = GL_TRUE;

   ctx->Extensions.MESA_pack_invert = GL_TRUE;

   ctx->Extensions.NV_blend_square = GL_TRUE;
   ctx->Extensions.NV_texgen_reflection = GL_TRUE;
   ctx->Extensions.NV_texture_env_combine4 = GL_TRUE;
   ctx->Extensions.NV_texture_rectangle = GL_TRUE;
#if 0
   /* possibly could support the following two */
   ctx->Extensions.NV_vertex_program = GL_TRUE;
   ctx->Extensions.NV_vertex_program1_1 = GL_TRUE;
#endif

#if FEATURE_OES_EGL_image
   ctx->Extensions.OES_EGL_image = GL_TRUE;
#endif
#if FEATURE_OES_draw_texture
   ctx->Extensions.OES_draw_texture = GL_TRUE;
#endif

   ctx->Extensions.SGIS_generate_mipmap = GL_TRUE;

   /*
    * Extensions that depend on the driver/hardware:
    */
   if (screen->get_param(screen, PIPE_CAP_MAX_RENDER_TARGETS) > 0) {
      ctx->Extensions.ARB_draw_buffers = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_TEXTURE_SWIZZLE) > 0) {
      ctx->Extensions.EXT_texture_swizzle = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_GLSL)) {
      ctx->Extensions.ARB_fragment_shader = GL_TRUE;
      ctx->Extensions.ARB_vertex_shader = GL_TRUE;
      ctx->Extensions.ARB_shader_objects = GL_TRUE;
      ctx->Extensions.ARB_shading_language_100 = GL_TRUE;
      ctx->Extensions.ARB_explicit_attrib_location = GL_TRUE;
      ctx->Extensions.EXT_separate_shader_objects = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_TEXTURE_MIRROR_REPEAT) > 0) {
      ctx->Extensions.ARB_texture_mirrored_repeat = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_BLEND_EQUATION_SEPARATE)) {
      ctx->Extensions.EXT_blend_equation_separate = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_TEXTURE_MIRROR_CLAMP) > 0) {
      ctx->Extensions.EXT_texture_mirror_clamp = GL_TRUE;
      ctx->Extensions.ATI_texture_mirror_once = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_NPOT_TEXTURES)) {
      ctx->Extensions.ARB_texture_non_power_of_two = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_MAX_TEXTURE_IMAGE_UNITS) > 1) {
      ctx->Extensions.ARB_multitexture = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_TWO_SIDED_STENCIL)) {
      ctx->Extensions.ATI_separate_stencil = GL_TRUE;
      ctx->Extensions.EXT_stencil_two_side = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_ANISOTROPIC_FILTER)) {
      ctx->Extensions.EXT_texture_filter_anisotropic = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_POINT_SPRITE)) {
      ctx->Extensions.ARB_point_sprite = GL_TRUE;
      /* GL_NV_point_sprite is not supported by gallium because we don't
       * support the GL_POINT_SPRITE_R_MODE_NV option.
       */
   }

   if (screen->get_param(screen, PIPE_CAP_OCCLUSION_QUERY)) {
      ctx->Extensions.ARB_occlusion_query = GL_TRUE;
      ctx->Extensions.ARB_occlusion_query2 = GL_TRUE;
   }
   if (screen->get_param(screen, PIPE_CAP_TIMER_QUERY)) {
     ctx->Extensions.EXT_timer_query = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_TEXTURE_SHADOW_MAP)) {
      ctx->Extensions.ARB_depth_texture = GL_TRUE;
      ctx->Extensions.ARB_fragment_program_shadow = GL_TRUE;
      ctx->Extensions.ARB_shadow = GL_TRUE;
      ctx->Extensions.EXT_shadow_funcs = GL_TRUE;
      /*ctx->Extensions.ARB_shadow_ambient = GL_TRUE;*/
   }

   /* GL_EXT_packed_depth_stencil requires both the ability to render to
    * a depth/stencil buffer and texture from depth/stencil source.
    */
   if (screen->is_format_supported(screen, PIPE_FORMAT_S8_USCALED_Z24_UNORM,
                                   PIPE_TEXTURE_2D, 0,
                                   PIPE_BIND_DEPTH_STENCIL, 0) &&
       screen->is_format_supported(screen, PIPE_FORMAT_S8_USCALED_Z24_UNORM,
                                   PIPE_TEXTURE_2D, 0,
                                   PIPE_BIND_SAMPLER_VIEW, 0)) {
      ctx->Extensions.EXT_packed_depth_stencil = GL_TRUE;
   }
   else if (screen->is_format_supported(screen, PIPE_FORMAT_Z24_UNORM_S8_USCALED,
                                        PIPE_TEXTURE_2D, 0,
                                        PIPE_BIND_DEPTH_STENCIL, 0) &&
            screen->is_format_supported(screen, PIPE_FORMAT_Z24_UNORM_S8_USCALED,
                                        PIPE_TEXTURE_2D, 0,
                                        PIPE_BIND_SAMPLER_VIEW, 0)) {
      ctx->Extensions.EXT_packed_depth_stencil = GL_TRUE;
   }

   /* sRGB support */
   if (screen->is_format_supported(screen, PIPE_FORMAT_A8B8G8R8_SRGB,
                                   PIPE_TEXTURE_2D, 0,
                                   PIPE_BIND_SAMPLER_VIEW, 0) ||
      screen->is_format_supported(screen, PIPE_FORMAT_B8G8R8A8_SRGB,
                                   PIPE_TEXTURE_2D, 0,
                                   PIPE_BIND_SAMPLER_VIEW, 0)) {
      ctx->Extensions.EXT_texture_sRGB = GL_TRUE;
      ctx->Extensions.EXT_texture_sRGB_decode = GL_TRUE;
      if (screen->is_format_supported(screen, PIPE_FORMAT_A8B8G8R8_SRGB,
                                   PIPE_TEXTURE_2D, 0,
                                   PIPE_BIND_RENDER_TARGET, 0) ||
          screen->is_format_supported(screen, PIPE_FORMAT_B8G8R8A8_SRGB,
                                   PIPE_TEXTURE_2D, 0,
                                   PIPE_BIND_RENDER_TARGET, 0)) {
         ctx->Extensions.EXT_framebuffer_sRGB = GL_TRUE;
         ctx->Const.sRGBCapable = GL_TRUE;
      }
   }

   if (screen->is_format_supported(screen, PIPE_FORMAT_R8G8_UNORM,
                                   PIPE_TEXTURE_2D, 0,
                                   PIPE_BIND_SAMPLER_VIEW, 0)) {
      ctx->Extensions.ARB_texture_rg = GL_TRUE;
   }

   /* s3tc support */
   if (screen->is_format_supported(screen, PIPE_FORMAT_DXT5_RGBA,
                                   PIPE_TEXTURE_2D, 0,
                                   PIPE_BIND_SAMPLER_VIEW, 0) &&
       ctx->Mesa_DXTn) {
      ctx->Extensions.EXT_texture_compression_s3tc = GL_TRUE;
      ctx->Extensions.S3_s3tc = GL_TRUE;
   }

   if (screen->is_format_supported(screen, PIPE_FORMAT_RGTC1_UNORM,
                                   PIPE_TEXTURE_2D, 0,
                                   PIPE_BIND_SAMPLER_VIEW, 0) &&
       screen->is_format_supported(screen, PIPE_FORMAT_RGTC1_SNORM,
				   PIPE_TEXTURE_2D, 0,
                                   PIPE_BIND_SAMPLER_VIEW, 0) &&
       screen->is_format_supported(screen, PIPE_FORMAT_RGTC2_UNORM,
                                   PIPE_TEXTURE_2D, 0,
                                   PIPE_BIND_SAMPLER_VIEW, 0) &&
       screen->is_format_supported(screen, PIPE_FORMAT_RGTC2_SNORM,
                                   PIPE_TEXTURE_2D, 0,
                                   PIPE_BIND_SAMPLER_VIEW, 0)
       ) {
     ctx->Extensions.ARB_texture_compression_rgtc = GL_TRUE;
   }

   /* ycbcr support */
   if (screen->is_format_supported(screen, PIPE_FORMAT_UYVY, 
                                   PIPE_TEXTURE_2D, 0,
                                   PIPE_BIND_SAMPLER_VIEW, 0) ||
       screen->is_format_supported(screen, PIPE_FORMAT_YUYV, 
                                   PIPE_TEXTURE_2D, 0,
                                   PIPE_BIND_SAMPLER_VIEW, 0)) {
      ctx->Extensions.MESA_ycbcr_texture = GL_TRUE;
   }

   /* GL_EXT_texture_array */
   if (screen->get_param(screen, PIPE_CAP_ARRAY_TEXTURES)) {
      ctx->Extensions.EXT_texture_array = GL_TRUE;
      ctx->Extensions.MESA_texture_array = GL_TRUE;
   }

   /* GL_ARB_framebuffer_object */
   if (ctx->Extensions.EXT_packed_depth_stencil) {
      /* we support always support GL_EXT_framebuffer_blit */
      ctx->Extensions.ARB_framebuffer_object = GL_TRUE;
   }

   if (st->pipe->render_condition) {
      ctx->Extensions.NV_conditional_render = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_INDEP_BLEND_ENABLE)) {
      ctx->Extensions.EXT_draw_buffers2 = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_INDEP_BLEND_FUNC)) {
      ctx->Extensions.ARB_draw_buffers_blend = GL_TRUE;
   }

   /* GL_ARB_half_float_vertex */
   if (screen->is_format_supported(screen, PIPE_FORMAT_R16G16B16A16_FLOAT,
                                   PIPE_BUFFER, 0,
                                   PIPE_BIND_VERTEX_BUFFER, 0)) {
      ctx->Extensions.ARB_half_float_vertex = GL_TRUE;
   }

   if (screen->get_shader_param(screen, PIPE_SHADER_GEOMETRY, PIPE_SHADER_CAP_MAX_INSTRUCTIONS) > 0) {
#if 0 /* XXX re-enable when GLSL compiler again supports geometry shaders */
      ctx->Extensions.ARB_geometry_shader4 = GL_TRUE;
#endif
   }

   if (screen->get_param(screen, PIPE_CAP_PRIMITIVE_RESTART)) {
      ctx->Extensions.NV_primitive_restart = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_DEPTH_CLAMP)) {
      ctx->Extensions.ARB_depth_clamp = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_SHADER_STENCIL_EXPORT)) {
      ctx->Extensions.ARB_shader_stencil_export = GL_TRUE;
   }

   if (screen->get_param(screen, PIPE_CAP_TGSI_INSTANCEID)) {
      ctx->Extensions.ARB_draw_instanced = GL_TRUE;
   }
   if (screen->get_param(screen, PIPE_CAP_VERTEX_ELEMENT_INSTANCE_DIVISOR)) {
      ctx->Extensions.ARB_instanced_arrays = GL_TRUE;
   }
}
