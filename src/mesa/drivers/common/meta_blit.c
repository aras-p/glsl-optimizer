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
#include "main/fbobject.h"
#include "main/macros.h"
#include "main/matrix.h"
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
setup_glsl_blit_framebuffer(struct gl_context *ctx,
                            struct blit_state *blit,
                            GLenum target)
{
   /* target = GL_TEXTURE_RECTANGLE is not supported in GLES 3.0 */
   assert(_mesa_is_desktop_gl(ctx) || target == GL_TEXTURE_2D);

   _mesa_meta_setup_vertex_objects(&blit->VAO, &blit->VBO, true, 2, 2, 0);

   _mesa_meta_setup_blit_shader(ctx, target, &blit->shaders);
}

/**
 * Try to do a glBlitFramebuffer using no-copy texturing.
 * We can do this when the src renderbuffer is actually a texture.
 *
 * \return new buffer mask indicating the buffers left to blit using the
 *         normal path.
 */
static GLbitfield
blitframebuffer_texture(struct gl_context *ctx,
                        GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                        GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                        GLbitfield mask, GLenum filter, GLint flipX,
                        GLint flipY, GLboolean glsl_version)
{
   if (mask & GL_COLOR_BUFFER_BIT) {
      const struct gl_framebuffer *readFb = ctx->ReadBuffer;
      const struct gl_renderbuffer_attachment *readAtt =
         &readFb->Attachment[readFb->_ColorReadBufferIndex];

      if (readAtt && readAtt->Texture) {
         struct blit_state *blit = &ctx->Meta->Blit;
         const GLint dstX = MIN2(dstX0, dstX1);
         const GLint dstY = MIN2(dstY0, dstY1);
         const GLint dstW = abs(dstX1 - dstX0);
         const GLint dstH = abs(dstY1 - dstY0);
         const struct gl_texture_object *texObj = readAtt->Texture;
         const GLuint srcLevel = readAtt->TextureLevel;
         const GLint baseLevelSave = texObj->BaseLevel;
         const GLint maxLevelSave = texObj->MaxLevel;
         const GLenum target = texObj->Target;
         GLuint sampler, samplerSave =
            ctx->Texture.Unit[ctx->Texture.CurrentUnit].Sampler ?
            ctx->Texture.Unit[ctx->Texture.CurrentUnit].Sampler->Name : 0;

         if (target != GL_TEXTURE_2D && target != GL_TEXTURE_RECTANGLE_ARB) {
            /* Can't handle other texture types at this time */
            return mask;
         }

         /* Choose between glsl version and fixed function version of
          * BlitFramebuffer function.
          */
         if (glsl_version) {
            setup_glsl_blit_framebuffer(ctx, blit, target);
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

	 /* Always do our blits with no sRGB decode or encode.  Note that
          * GL_FRAMEBUFFER_SRGB has already been disabled by
          * _mesa_meta_begin().
          */
	 if (ctx->Extensions.EXT_texture_sRGB_decode) {
	    _mesa_SamplerParameteri(sampler, GL_TEXTURE_SRGB_DECODE_EXT,
				GL_SKIP_DECODE_EXT);
	 }

         if (ctx->API == API_OPENGL_COMPAT || ctx->API == API_OPENGLES) {
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
               assert(target == GL_TEXTURE_RECTANGLE_ARB);
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
         _mesa_ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
         _mesa_DepthMask(GL_FALSE);
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

         /* Done with color buffer */
         mask &= ~GL_COLOR_BUFFER_BIT;
      }
   }

   return mask;
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
   if (srcW > maxTexSize || srcH > maxTexSize
       || ctx->ReadBuffer->Visual.samples > 0) {
      /* XXX avoid this fallback */
      _swrast_BlitFramebuffer(ctx, srcX0, srcY0, srcX1, srcY1,
                              dstX0, dstY0, dstX1, dstY1, mask, filter);
      return;
   }

   /* only scissor effects blit so save/clear all other relevant state */
   _mesa_meta_begin(ctx, ~MESA_META_SCISSOR);

   /* Try faster, direct texture approach first */
   mask = blitframebuffer_texture(ctx, srcX0, srcY0, srcX1, srcY1,
                                  dstX0, dstY0, dstX1, dstY1, mask, filter,
                                  dstFlipX, dstFlipY, use_glsl_version);
   if (mask == 0x0) {
      _mesa_meta_end(ctx);
      return;
   }

   /* Choose between glsl version and fixed function version of
    * BlitFramebuffer function.
    */
   if (use_glsl_version) {
      setup_glsl_blit_framebuffer(ctx, blit, tex->Target);
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

   /* glEnable() in gles2 and gles3 doesn't allow GL_TEXTURE_{1D, 2D, etc.}
    * tokens.
    */
   if (_mesa_is_desktop_gl(ctx) || ctx->API == API_OPENGLES)
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

   if (_mesa_is_desktop_gl(ctx) || ctx->API == API_OPENGLES)
      _mesa_set_enable(ctx, tex->Target, GL_FALSE);

   _mesa_meta_end(ctx);

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
