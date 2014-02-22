/*
 * Mesa 3-D graphics library
 *
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include "main/glheader.h"
#include "main/mtypes.h"
#include "main/imports.h"
#include "main/arbprogram.h"
#include "main/arrayobj.h"
#include "main/blend.h"
#include "main/condrender.h"
#include "main/depth.h"
#include "main/enable.h"
#include "main/enums.h"
#include "main/fbobject.h"
#include "main/macros.h"
#include "main/matrix.h"
#include "main/multisample.h"
#include "main/readpix.h"
#include "main/shaderapi.h"
#include "main/texobj.h"
#include "main/texenv.h"
#include "main/teximage.h"
#include "main/texparam.h"
#include "main/varray.h"
#include "main/viewport.h"
#include "swrast/swrast.h"
#include "drivers/common/meta.h"
#include "../glsl/ralloc.h"

/** Return offset in bytes of the field within a vertex struct */
#define OFFSET(FIELD) ((void *) offsetof(struct vertex, FIELD))

/**
 * One-time init for drawing depth pixels.
 */
static void
init_blit_depth_pixels(struct gl_context *ctx)
{
   static const char *program =
      "!!ARBfp1.0\n"
      "TEX result.depth, fragment.texcoord[0], texture[0], %s; \n"
      "END \n";
   char program2[200];
   struct blit_state *blit = &ctx->Meta->Blit;
   struct temp_texture *tex = _mesa_meta_get_temp_texture(ctx);
   const char *texTarget;

   assert(blit->DepthFP == 0);

   /* replace %s with "RECT" or "2D" */
   assert(strlen(program) + 4 < sizeof(program2));
   if (tex->Target == GL_TEXTURE_RECTANGLE)
      texTarget = "RECT";
   else
      texTarget = "2D";
   _mesa_snprintf(program2, sizeof(program2), program, texTarget);

   _mesa_GenProgramsARB(1, &blit->DepthFP);
   _mesa_BindProgramARB(GL_FRAGMENT_PROGRAM_ARB, blit->DepthFP);
   _mesa_ProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                          strlen(program2), (const GLubyte *) program2);
}

static void
setup_glsl_msaa_blit_shader(struct gl_context *ctx,
                            struct blit_state *blit,
                            struct gl_renderbuffer *src_rb,
                            GLenum target)
{
   const char *vs_source;
   char *fs_source;
   GLuint vs, fs;
   void *mem_ctx;
   enum blit_msaa_shader shader_index;
   bool dst_is_msaa = false;
   GLenum src_datatype;
   const char *vec4_prefix;

   if (src_rb) {
      src_datatype = _mesa_get_format_datatype(src_rb->Format);
   } else {
      /* depth-or-color glCopyTexImage fallback path that passes a NULL rb and
       * doesn't handle integer.
       */
      src_datatype = GL_UNSIGNED_NORMALIZED;
   }

   if (ctx->DrawBuffer->Visual.samples > 1) {
      /* If you're calling meta_BlitFramebuffer with the destination
       * multisampled, this is the only path that will work -- swrast and
       * CopyTexImage won't work on it either.
       */
      assert(ctx->Extensions.ARB_sample_shading);

      dst_is_msaa = true;

      /* We need shader invocation per sample, not per pixel */
      _mesa_set_enable(ctx, GL_MULTISAMPLE, GL_TRUE);
      _mesa_set_enable(ctx, GL_SAMPLE_SHADING, GL_TRUE);
      _mesa_MinSampleShading(1.0);
   }

   switch (target) {
   case GL_TEXTURE_2D_MULTISAMPLE:
      if (src_rb->_BaseFormat == GL_DEPTH_COMPONENT ||
          src_rb->_BaseFormat == GL_DEPTH_STENCIL) {
         if (dst_is_msaa)
            shader_index = BLIT_MSAA_SHADER_2D_MULTISAMPLE_DEPTH_COPY;
         else
            shader_index = BLIT_MSAA_SHADER_2D_MULTISAMPLE_DEPTH_RESOLVE;
      } else {
         if (dst_is_msaa)
            shader_index = BLIT_MSAA_SHADER_2D_MULTISAMPLE_COPY;
         else
            shader_index = BLIT_MSAA_SHADER_2D_MULTISAMPLE_RESOLVE;
      }
      break;
   default:
      _mesa_problem(ctx, "Unkown texture target %s\n",
                    _mesa_lookup_enum_by_nr(target));
      shader_index = BLIT_MSAA_SHADER_2D_MULTISAMPLE_RESOLVE;
   }

   /* We rely on the enum being sorted this way. */
   STATIC_ASSERT(BLIT_MSAA_SHADER_2D_MULTISAMPLE_RESOLVE_INT ==
                 BLIT_MSAA_SHADER_2D_MULTISAMPLE_RESOLVE + 1);
   STATIC_ASSERT(BLIT_MSAA_SHADER_2D_MULTISAMPLE_RESOLVE_UINT ==
                 BLIT_MSAA_SHADER_2D_MULTISAMPLE_RESOLVE + 2);
   if (src_datatype == GL_INT) {
      shader_index++;
      vec4_prefix = "i";
   } else if (src_datatype == GL_UNSIGNED_INT) {
      shader_index += 2;
      vec4_prefix = "u";
   } else {
      vec4_prefix = "";
   }

   if (blit->msaa_shaders[shader_index]) {
      _mesa_UseProgram(blit->msaa_shaders[shader_index]);
      return;
   }

   mem_ctx = ralloc_context(NULL);

   if (shader_index == BLIT_MSAA_SHADER_2D_MULTISAMPLE_DEPTH_RESOLVE ||
       shader_index == BLIT_MSAA_SHADER_2D_MULTISAMPLE_DEPTH_COPY) {
      char *sample_index;
      const char *arb_sample_shading_extension_string;

      if (dst_is_msaa) {
         arb_sample_shading_extension_string = "#extension GL_ARB_sample_shading : enable";
         sample_index = "gl_SampleID";
      } else {
         /* Don't need that extension, since we're drawing to a single-sampled
          * destination.
          */
         arb_sample_shading_extension_string = "";
         /* From the GL 4.3 spec:
          *
          *     "If there is a multisample buffer (the value of SAMPLE_BUFFERS
          *      is one), then values are obtained from the depth samples in
          *      this buffer. It is recommended that the depth value of the
          *      centermost sample be used, though implementations may choose
          *      any function of the depth sample values at each pixel.
          *
          * We're slacking and instead of choosing centermost, we've got 0.
          */
         sample_index = "0";
      }

      vs_source = ralloc_asprintf(mem_ctx,
                                  "#version 130\n"
                                  "in vec2 position;\n"
                                  "in vec2 textureCoords;\n"
                                  "out vec2 texCoords;\n"
                                  "void main()\n"
                                  "{\n"
                                  "   texCoords = textureCoords;\n"
                                  "   gl_Position = vec4(position, 0.0, 1.0);\n"
                                  "}\n");
      fs_source = ralloc_asprintf(mem_ctx,
                                  "#version 130\n"
                                  "#extension GL_ARB_texture_multisample : enable\n"
                                  "%s\n"
                                  "uniform sampler2DMS texSampler;\n"
                                  "in vec2 texCoords;\n"
                                  "out vec4 out_color;\n"
                                  "\n"
                                  "void main()\n"
                                  "{\n"
                                  "   gl_FragDepth = texelFetch(texSampler, ivec2(texCoords), %s).r;\n"
                                  "}\n",
                                  arb_sample_shading_extension_string,
                                  sample_index);
   } else {
      /* You can create 2D_MULTISAMPLE textures with 0 sample count (meaning 1
       * sample).  Yes, this is ridiculous.
       */
      int samples = MAX2(src_rb->NumSamples, 1);
      char *sample_resolve;
      const char *arb_sample_shading_extension_string;
      const char *merge_function;

      if (dst_is_msaa) {
         arb_sample_shading_extension_string = "#extension GL_ARB_sample_shading : enable";
         sample_resolve = ralloc_asprintf(mem_ctx, "   out_color = texelFetch(texSampler, ivec2(texCoords), gl_SampleID);");
         merge_function = "";
      } else {
         if (src_datatype == GL_INT) {
            merge_function =
               "ivec4 merge(ivec4 a, ivec4 b) { return (a >> ivec4(1)) + (b >> ivec4(1)) + (a & b & ivec4(1)); }\n";
         } else if (src_datatype == GL_UNSIGNED_INT) {
            merge_function =
               "uvec4 merge(uvec4 a, uvec4 b) { return (a >> uvec4(1)) + (b >> uvec4(1)) + (a & b & uvec4(1)); }\n";
         } else {
            /* The divide will happen at the end for floats. */
            merge_function =
               "vec4 merge(vec4 a, vec4 b) { return (a + b); }\n";
         }

         arb_sample_shading_extension_string = "";

         /* We're assuming power of two samples for this resolution procedure.
          *
          * To avoid losing any floating point precision if the samples all
          * happen to have the same value, we merge pairs of values at a time
          * (so the floating point exponent just gets increased), rather than
          * doing a naive sum and dividing.
          */
         assert((samples & (samples - 1)) == 0);
         /* Fetch each individual sample. */
         sample_resolve = rzalloc_size(mem_ctx, 1);
         for (int i = 0; i < samples; i++) {
            ralloc_asprintf_append(&sample_resolve,
                                   "   %svec4 sample_1_%d = texelFetch(texSampler, ivec2(texCoords), %d);\n",
                                   vec4_prefix, i, i);
         }
         /* Now, merge each pair of samples, then merge each pair of those,
          * etc.
          */
         for (int step = 2; step <= samples; step *= 2) {
            for (int i = 0; i < samples; i += step) {
               ralloc_asprintf_append(&sample_resolve,
                                      "   %svec4 sample_%d_%d = merge(sample_%d_%d, sample_%d_%d);\n",
                                      vec4_prefix,
                                      step, i,
                                      step / 2, i,
                                      step / 2, i + step / 2);
            }
         }

         /* Scale the final result. */
         if (src_datatype == GL_UNSIGNED_INT || src_datatype == GL_INT) {
            ralloc_asprintf_append(&sample_resolve,
                                   "   out_color = sample_%d_0;\n",
                                   samples);
         } else {
            ralloc_asprintf_append(&sample_resolve,
                                   "   out_color = sample_%d_0 / %f;\n",
                                   samples, (float)samples);
         }
      }

      vs_source = ralloc_asprintf(mem_ctx,
                                  "#version 130\n"
                                  "in vec2 position;\n"
                                  "in vec2 textureCoords;\n"
                                  "out vec2 texCoords;\n"
                                  "void main()\n"
                                  "{\n"
                                  "   texCoords = textureCoords;\n"
                                  "   gl_Position = vec4(position, 0.0, 1.0);\n"
                                  "}\n");
      fs_source = ralloc_asprintf(mem_ctx,
                                  "#version 130\n"
                                  "#extension GL_ARB_texture_multisample : enable\n"
                                  "%s\n"
                                  "uniform %ssampler2DMS texSampler;\n"
                                  "in vec2 texCoords;\n"
                                  "out %svec4 out_color;\n"
                                  "\n"
                                  "%s" /* merge_function */
                                  "void main()\n"
                                  "{\n"
                                  "%s\n" /* sample_resolve */
                                  "}\n",
                                  arb_sample_shading_extension_string,
                                  vec4_prefix,
                                  vec4_prefix,
                                  merge_function,
                                  sample_resolve);
   }

   vs = _mesa_meta_compile_shader_with_debug(ctx, GL_VERTEX_SHADER, vs_source);
   fs = _mesa_meta_compile_shader_with_debug(ctx, GL_FRAGMENT_SHADER, fs_source);

   blit->msaa_shaders[shader_index] = _mesa_CreateProgramObjectARB();
   _mesa_AttachShader(blit->msaa_shaders[shader_index], fs);
   _mesa_DeleteObjectARB(fs);
   _mesa_AttachShader(blit->msaa_shaders[shader_index], vs);
   _mesa_DeleteObjectARB(vs);
   _mesa_BindAttribLocation(blit->msaa_shaders[shader_index], 0, "position");
   _mesa_BindAttribLocation(blit->msaa_shaders[shader_index], 1, "texcoords");
   _mesa_meta_link_program_with_debug(ctx, blit->msaa_shaders[shader_index]);
   ralloc_free(mem_ctx);

   _mesa_UseProgram(blit->msaa_shaders[shader_index]);
}

static void
setup_glsl_blit_framebuffer(struct gl_context *ctx,
                            struct blit_state *blit,
                            struct gl_renderbuffer *src_rb,
                            GLenum target)
{
   /* target = GL_TEXTURE_RECTANGLE is not supported in GLES 3.0 */
   assert(_mesa_is_desktop_gl(ctx) || target == GL_TEXTURE_2D);

   _mesa_meta_setup_vertex_objects(&blit->VAO, &blit->VBO, true, 2, 2, 0);

   if (target == GL_TEXTURE_2D_MULTISAMPLE) {
      setup_glsl_msaa_blit_shader(ctx, blit, src_rb, target);
   } else {
      _mesa_meta_setup_blit_shader(ctx, target, &blit->shaders);
   }
}

/**
 * Try to do a color or depth glBlitFramebuffer using texturing.
 *
 * We can do this when the src renderbuffer is actually a texture, or when the
 * driver exposes BindRenderbufferTexImage().
 */
static bool
blitframebuffer_texture(struct gl_context *ctx,
                        GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                        GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                        GLenum filter, GLint flipX, GLint flipY,
                        GLboolean glsl_version, GLboolean do_depth)
{
   const struct gl_framebuffer *readFb = ctx->ReadBuffer;
   int att_index = do_depth ? BUFFER_DEPTH : readFb->_ColorReadBufferIndex;
   const struct gl_renderbuffer_attachment *readAtt =
      &readFb->Attachment[att_index];
   struct blit_state *blit = &ctx->Meta->Blit;
   const GLint dstX = MIN2(dstX0, dstX1);
   const GLint dstY = MIN2(dstY0, dstY1);
   const GLint dstW = abs(dstX1 - dstX0);
   const GLint dstH = abs(dstY1 - dstY0);
   struct gl_texture_object *texObj;
   GLuint srcLevel;
   GLint baseLevelSave;
   GLint maxLevelSave;
   GLenum target;
   GLuint sampler, samplerSave =
      ctx->Texture.Unit[ctx->Texture.CurrentUnit].Sampler ?
      ctx->Texture.Unit[ctx->Texture.CurrentUnit].Sampler->Name : 0;
   GLuint tempTex = 0;
   struct gl_renderbuffer *rb = readAtt->Renderbuffer;

   if (rb->NumSamples && !ctx->Extensions.ARB_texture_multisample)
      return false;

   if (filter == GL_SCALED_RESOLVE_FASTEST_EXT ||
       filter == GL_SCALED_RESOLVE_NICEST_EXT) {
      filter = GL_LINEAR;
   }

   if (readAtt->Texture) {
      /* If there's a texture attached of a type we can handle, then just use
       * it directly.
       */
      srcLevel = readAtt->TextureLevel;
      texObj = readAtt->Texture;
      target = texObj->Target;

      switch (target) {
      case GL_TEXTURE_2D:
      case GL_TEXTURE_RECTANGLE:
      case GL_TEXTURE_2D_MULTISAMPLE:
         break;
      default:
         return false;
      }
   } else if (ctx->Driver.BindRenderbufferTexImage) {
      /* Otherwise, we need the driver to be able to bind a renderbuffer as
       * a texture image.
       */
      struct gl_texture_image *texImage;

      if (rb->NumSamples > 1)
         target = GL_TEXTURE_2D_MULTISAMPLE;
      else
         target = GL_TEXTURE_2D;

      _mesa_GenTextures(1, &tempTex);
      _mesa_BindTexture(target, tempTex);
      srcLevel = 0;
      texObj = _mesa_lookup_texture(ctx, tempTex);
      texImage = _mesa_get_tex_image(ctx, texObj, target, srcLevel);

      if (!ctx->Driver.BindRenderbufferTexImage(ctx, rb, texImage)) {
         _mesa_DeleteTextures(1, &tempTex);
         return false;
      } else {
         if (ctx->Driver.FinishRenderTexture &&
             !rb->NeedsFinishRenderTexture) {
            rb->NeedsFinishRenderTexture = true;
            ctx->Driver.FinishRenderTexture(ctx, rb);
         }

         if (_mesa_is_winsys_fbo(readFb)) {
            GLint temp = srcY0;
            srcY0 = rb->Height - srcY1;
            srcY1 = rb->Height - temp;
            flipY = -flipY;
         }
      }
   } else {
      return false;
   }

   baseLevelSave = texObj->BaseLevel;
   maxLevelSave = texObj->MaxLevel;

   if (glsl_version) {
      setup_glsl_blit_framebuffer(ctx, blit, rb, target);
   }
   else {
      _mesa_meta_setup_ff_tnl_for_blit(&ctx->Meta->Blit.VAO,
                                       &ctx->Meta->Blit.VBO,
                                       2);
   }

   _mesa_GenSamplers(1, &sampler);
   _mesa_BindSampler(ctx->Texture.CurrentUnit, sampler);

   /*
     printf("Blit from texture!\n");
     printf("  srcAtt %p  dstAtt %p\n", readAtt, drawAtt);
     printf("  srcTex %p  dstText %p\n", texObj, drawAtt->Texture);
   */

   /* Prepare src texture state */
   _mesa_BindTexture(target, texObj->Name);
   _mesa_SamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, filter);
   _mesa_SamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, filter);
   if (target != GL_TEXTURE_RECTANGLE_ARB) {
      _mesa_TexParameteri(target, GL_TEXTURE_BASE_LEVEL, srcLevel);
      _mesa_TexParameteri(target, GL_TEXTURE_MAX_LEVEL, srcLevel);
   }
   _mesa_SamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   _mesa_SamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   /* Always do our blits with no net sRGB decode or encode.
    *
    * However, if both the src and dst can be srgb decode/encoded, enable them
    * so that we do any blending (from scaling or from MSAA resolves) in the
    * right colorspace.
    *
    * Our choice of not doing any net encode/decode is from the GL 3.0
    * specification:
    *
    *     "Blit operations bypass the fragment pipeline. The only fragment
    *      operations which affect a blit are the pixel ownership test and the
    *      scissor test."
    *
    * The GL 4.4 specification disagrees and says that the sRGB part of the
    * fragment pipeline applies, but this was found to break applications.
    */
   if (ctx->Extensions.EXT_texture_sRGB_decode) {
      if (_mesa_get_format_color_encoding(rb->Format) == GL_SRGB &&
          ctx->DrawBuffer->Visual.sRGBCapable) {
         _mesa_SamplerParameteri(sampler, GL_TEXTURE_SRGB_DECODE_EXT,
                                 GL_DECODE_EXT);
         _mesa_set_framebuffer_srgb(ctx, GL_TRUE);
      } else {
         _mesa_SamplerParameteri(sampler, GL_TEXTURE_SRGB_DECODE_EXT,
                                 GL_SKIP_DECODE_EXT);
         /* set_framebuffer_srgb was set by _mesa_meta_begin(). */
      }
   }

   if (!glsl_version) {
      _mesa_TexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
      _mesa_set_enable(ctx, target, GL_TRUE);
   }

   /* Prepare vertex data (the VBO was previously created and bound) */
   {
      struct vertex verts[4];
      GLfloat s0, t0, s1, t1;

      if (target == GL_TEXTURE_2D) {
         const struct gl_texture_image *texImage
            = _mesa_select_tex_image(ctx, texObj, target, srcLevel);
         s0 = srcX0 / (float) texImage->Width;
         s1 = srcX1 / (float) texImage->Width;
         t0 = srcY0 / (float) texImage->Height;
         t1 = srcY1 / (float) texImage->Height;
      }
      else {
         assert(target == GL_TEXTURE_RECTANGLE_ARB ||
                target == GL_TEXTURE_2D_MULTISAMPLE);
         s0 = (float) srcX0;
         s1 = (float) srcX1;
         t0 = (float) srcY0;
         t1 = (float) srcY1;
      }

      /* Silence valgrind warnings about reading uninitialized stack. */
      memset(verts, 0, sizeof(verts));

      /* setup vertex positions */
      verts[0].x = -1.0F * flipX;
      verts[0].y = -1.0F * flipY;
      verts[1].x =  1.0F * flipX;
      verts[1].y = -1.0F * flipY;
      verts[2].x =  1.0F * flipX;
      verts[2].y =  1.0F * flipY;
      verts[3].x = -1.0F * flipX;
      verts[3].y =  1.0F * flipY;

      verts[0].tex[0] = s0;
      verts[0].tex[1] = t0;
      verts[1].tex[0] = s1;
      verts[1].tex[1] = t0;
      verts[2].tex[0] = s1;
      verts[2].tex[1] = t1;
      verts[3].tex[0] = s0;
      verts[3].tex[1] = t1;

      _mesa_BufferSubData(GL_ARRAY_BUFFER_ARB, 0, sizeof(verts), verts);
   }

   /* setup viewport */
   _mesa_set_viewport(ctx, 0, dstX, dstY, dstW, dstH);
   _mesa_ColorMask(!do_depth, !do_depth, !do_depth, !do_depth);
   _mesa_set_enable(ctx, GL_DEPTH_TEST, do_depth);
   _mesa_DepthMask(do_depth);
   _mesa_DepthFunc(GL_ALWAYS);

   _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);

   /* Restore texture object state, the texture binding will
    * be restored by _mesa_meta_end().
    */
   if (target != GL_TEXTURE_RECTANGLE_ARB) {
      _mesa_TexParameteri(target, GL_TEXTURE_BASE_LEVEL, baseLevelSave);
      _mesa_TexParameteri(target, GL_TEXTURE_MAX_LEVEL, maxLevelSave);
   }

   _mesa_BindSampler(ctx->Texture.CurrentUnit, samplerSave);
   _mesa_DeleteSamplers(1, &sampler);
   if (tempTex)
      _mesa_DeleteTextures(1, &tempTex);

   return true;
}

/**
 * Meta implementation of ctx->Driver.BlitFramebuffer() in terms
 * of texture mapping and polygon rendering.
 */
void
_mesa_meta_BlitFramebuffer(struct gl_context *ctx,
                           GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                           GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                           GLbitfield mask, GLenum filter)
{
   struct blit_state *blit = &ctx->Meta->Blit;
   struct temp_texture *tex = _mesa_meta_get_temp_texture(ctx);
   struct temp_texture *depthTex = _mesa_meta_get_temp_depth_texture(ctx);
   const GLsizei maxTexSize = tex->MaxSize;
   const GLint srcX = MIN2(srcX0, srcX1);
   const GLint srcY = MIN2(srcY0, srcY1);
   const GLint srcW = abs(srcX1 - srcX0);
   const GLint srcH = abs(srcY1 - srcY0);
   const GLint dstX = MIN2(dstX0, dstX1);
   const GLint dstY = MIN2(dstY0, dstY1);
   const GLint dstW = abs(dstX1 - dstX0);
   const GLint dstH = abs(dstY1 - dstY0);
   const GLint srcFlipX = (srcX1 - srcX0) / srcW;
   const GLint srcFlipY = (srcY1 - srcY0) / srcH;
   const GLint dstFlipX = (dstX1 - dstX0) / dstW;
   const GLint dstFlipY = (dstY1 - dstY0) / dstH;
   const GLint flipX = srcFlipX * dstFlipX;
   const GLint flipY = srcFlipY * dstFlipY;

   struct vertex verts[4];
   GLboolean newTex;
   const GLboolean use_glsl_version = ctx->Extensions.ARB_vertex_shader &&
                                      ctx->Extensions.ARB_fragment_shader &&
                                      (ctx->API != API_OPENGLES);

   /* In addition to falling back if the blit size is larger than the maximum
    * texture size, fallback if the source is multisampled.  This fallback can
    * be removed once Mesa gets support ARB_texture_multisample.
    */
   if (srcW > maxTexSize || srcH > maxTexSize) {
      /* XXX avoid this fallback */
      goto fallback;
   }

   /* Multisample texture blit support requires texture multisample. */
   if (ctx->ReadBuffer->Visual.samples > 0 &&
       !ctx->Extensions.ARB_texture_multisample) {
      goto fallback;
   }

   /* only scissor effects blit so save/clear all other relevant state */
   _mesa_meta_begin(ctx, ~MESA_META_SCISSOR);

   /* Try faster, direct texture approach first */
   if (mask & GL_COLOR_BUFFER_BIT) {
      if (blitframebuffer_texture(ctx, srcX0, srcY0, srcX1, srcY1,
                                  dstX0, dstY0, dstX1, dstY1,
                                  filter, dstFlipX, dstFlipY,
                                  use_glsl_version, false)) {
         mask &= ~GL_COLOR_BUFFER_BIT;
         if (mask == 0x0) {
            _mesa_meta_end(ctx);
            return;
         }
      }
   }

   if (mask & GL_DEPTH_BUFFER_BIT && use_glsl_version) {
      if (blitframebuffer_texture(ctx, srcX0, srcY0, srcX1, srcY1,
                                  dstX0, dstY0, dstX1, dstY1,
                                  filter, dstFlipX, dstFlipY,
                                  use_glsl_version, true)) {
         mask &= ~GL_DEPTH_BUFFER_BIT;
         if (mask == 0x0) {
            _mesa_meta_end(ctx);
            return;
         }
      }
   }

   /* Choose between glsl version and fixed function version of
    * BlitFramebuffer function.
    */
   if (use_glsl_version) {
      setup_glsl_blit_framebuffer(ctx, blit, NULL, tex->Target);
   }
   else {
      _mesa_meta_setup_ff_tnl_for_blit(&blit->VAO, &blit->VBO, 2);
   }

   /* Silence valgrind warnings about reading uninitialized stack. */
   memset(verts, 0, sizeof(verts));

   /* Continue with "normal" approach which involves copying the src rect
    * into a temporary texture and is "blitted" by drawing a textured quad.
    */
   {
      /* setup vertex positions */
      verts[0].x = -1.0F * flipX;
      verts[0].y = -1.0F * flipY;
      verts[1].x =  1.0F * flipX;
      verts[1].y = -1.0F * flipY;
      verts[2].x =  1.0F * flipX;
      verts[2].y =  1.0F * flipY;
      verts[3].x = -1.0F * flipX;
      verts[3].y =  1.0F * flipY;

   }

   if (!use_glsl_version)
      _mesa_set_enable(ctx, tex->Target, GL_TRUE);

   if (mask & GL_COLOR_BUFFER_BIT) {
      const struct gl_framebuffer *readFb = ctx->ReadBuffer;
      const struct gl_renderbuffer *colorReadRb = readFb->_ColorReadBuffer;
      const GLenum rb_base_format =
         _mesa_base_tex_format(ctx, colorReadRb->InternalFormat);

      /* Using  the exact source rectangle to create the texture does incorrect
       * linear filtering along the edges. So, allocate the texture extended along
       * edges by one pixel in x, y directions.
       */
      _mesa_meta_setup_copypix_texture(ctx, tex,
                                       srcX - 1, srcY - 1, srcW + 2, srcH + 2,
                                       rb_base_format, filter);
      /* texcoords (after texture allocation!) */
      {
         verts[0].tex[0] = 1.0F;
         verts[0].tex[1] = 1.0F;
         verts[1].tex[0] = tex->Sright - 1.0F;
         verts[1].tex[1] = 1.0F;
         verts[2].tex[0] = tex->Sright - 1.0F;
         verts[2].tex[1] = tex->Ttop - 1.0F;
         verts[3].tex[0] = 1.0F;
         verts[3].tex[1] = tex->Ttop - 1.0F;

         /* upload new vertex data */
         _mesa_BufferSubData(GL_ARRAY_BUFFER_ARB, 0, sizeof(verts), verts);
      }

      _mesa_set_viewport(ctx, 0, dstX, dstY, dstW, dstH);
      _mesa_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      _mesa_set_enable(ctx, GL_DEPTH_TEST, GL_FALSE);
      _mesa_DepthMask(GL_FALSE);
      _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);
      mask &= ~GL_COLOR_BUFFER_BIT;
   }

   if ((mask & GL_DEPTH_BUFFER_BIT) &&
       _mesa_is_desktop_gl(ctx) &&
       ctx->Extensions.ARB_depth_texture &&
       ctx->Extensions.ARB_fragment_program) {

      GLuint *tmp = malloc(srcW * srcH * sizeof(GLuint));

      if (tmp) {

         newTex = _mesa_meta_alloc_texture(depthTex, srcW, srcH,
                                           GL_DEPTH_COMPONENT);
         _mesa_ReadPixels(srcX, srcY, srcW, srcH, GL_DEPTH_COMPONENT,
                          GL_UNSIGNED_INT, tmp);
         _mesa_meta_setup_drawpix_texture(ctx, depthTex, newTex,
                                          srcW, srcH, GL_DEPTH_COMPONENT,
                                          GL_UNSIGNED_INT, tmp);

         /* texcoords (after texture allocation!) */
         {
            verts[0].tex[0] = 0.0F;
            verts[0].tex[1] = 0.0F;
            verts[1].tex[0] = depthTex->Sright;
            verts[1].tex[1] = 0.0F;
            verts[2].tex[0] = depthTex->Sright;
            verts[2].tex[1] = depthTex->Ttop;
            verts[3].tex[0] = 0.0F;
            verts[3].tex[1] = depthTex->Ttop;

            /* upload new vertex data */
            _mesa_BufferSubData(GL_ARRAY_BUFFER_ARB, 0, sizeof(verts), verts);
         }

         if (!blit->DepthFP)
            init_blit_depth_pixels(ctx);

         _mesa_BindProgramARB(GL_FRAGMENT_PROGRAM_ARB, blit->DepthFP);
         _mesa_set_enable(ctx, GL_FRAGMENT_PROGRAM_ARB, GL_TRUE);
         _mesa_ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
         _mesa_set_enable(ctx, GL_DEPTH_TEST, GL_TRUE);
         _mesa_DepthFunc(GL_ALWAYS);
         _mesa_DepthMask(GL_TRUE);

         _mesa_set_viewport(ctx, 0, dstX, dstY, dstW, dstH);
         _mesa_BufferSubData(GL_ARRAY_BUFFER_ARB, 0, sizeof(verts), verts);
         _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);
         mask &= ~GL_DEPTH_BUFFER_BIT;

         free(tmp);
      }
   }

   if (mask & GL_STENCIL_BUFFER_BIT) {
      /* XXX can't easily do stencil */
   }

   if (!use_glsl_version)
      _mesa_set_enable(ctx, tex->Target, GL_FALSE);

   _mesa_meta_end(ctx);

fallback:
   if (mask) {
      _swrast_BlitFramebuffer(ctx, srcX0, srcY0, srcX1, srcY1,
                              dstX0, dstY0, dstX1, dstY1, mask, filter);
   }
}

void
_mesa_meta_glsl_blit_cleanup(struct blit_state *blit)
{
   if (blit->VAO) {
      _mesa_DeleteVertexArrays(1, &blit->VAO);
      blit->VAO = 0;
      _mesa_DeleteBuffers(1, &blit->VBO);
      blit->VBO = 0;
   }
   if (blit->DepthFP) {
      _mesa_DeleteProgramsARB(1, &blit->DepthFP);
      blit->DepthFP = 0;
   }

   _mesa_meta_blit_shader_table_cleanup(&blit->shaders);

   _mesa_DeleteTextures(1, &blit->depthTex.TexObj);
   blit->depthTex.TexObj = 0;
}
