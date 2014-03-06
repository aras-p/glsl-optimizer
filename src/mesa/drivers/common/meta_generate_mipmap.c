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

/**
 * Meta operations.  Some GL operations can be expressed in terms of
 * other GL operations.  For example, glBlitFramebuffer() can be done
 * with texture mapping and glClear() can be done with polygon rendering.
 *
 * \author Brian Paul
 */

#include "main/arrayobj.h"
#include "main/buffers.h"
#include "main/enums.h"
#include "main/enable.h"
#include "main/fbobject.h"
#include "main/macros.h"
#include "main/mipmap.h"
#include "main/teximage.h"
#include "main/texobj.h"
#include "main/texparam.h"
#include "main/varray.h"
#include "main/viewport.h"
#include "drivers/common/meta.h"

/**
 * Check if the call to _mesa_meta_GenerateMipmap() will require a
 * software fallback.  The fallback path will require that the texture
 * images are mapped.
 * \return GL_TRUE if a fallback is needed, GL_FALSE otherwise
 */
static GLboolean
fallback_required(struct gl_context *ctx, GLenum target,
                  struct gl_texture_object *texObj)
{
   const GLuint fboSave = ctx->DrawBuffer->Name;
   struct gen_mipmap_state *mipmap = &ctx->Meta->Mipmap;
   struct gl_texture_image *baseImage;
   GLuint srcLevel;
   GLenum status;

   /* check for fallbacks */
   if (target == GL_TEXTURE_3D ||
       target == GL_TEXTURE_1D_ARRAY ||
       target == GL_TEXTURE_2D_ARRAY) {
      _mesa_perf_debug(ctx, MESA_DEBUG_SEVERITY_HIGH,
                       "glGenerateMipmap() to %s target\n",
                       _mesa_lookup_enum_by_nr(target));
      return GL_TRUE;
   }

   srcLevel = texObj->BaseLevel;
   baseImage = _mesa_select_tex_image(ctx, texObj, target, srcLevel);
   if (!baseImage) {
      _mesa_perf_debug(ctx, MESA_DEBUG_SEVERITY_HIGH,
                       "glGenerateMipmap() couldn't find base teximage\n");
      return GL_TRUE;
   }

   if (_mesa_is_format_compressed(baseImage->TexFormat)) {
      _mesa_perf_debug(ctx, MESA_DEBUG_SEVERITY_HIGH,
                       "glGenerateMipmap() with %s format\n",
                       _mesa_get_format_name(baseImage->TexFormat));
      return GL_TRUE;
   }

   if (_mesa_get_format_color_encoding(baseImage->TexFormat) == GL_SRGB &&
       !ctx->Extensions.EXT_texture_sRGB_decode) {
      /* The texture format is sRGB but we can't turn off sRGB->linear
       * texture sample conversion.  So we won't be able to generate the
       * right colors when rendering.  Need to use a fallback.
       */
      _mesa_perf_debug(ctx, MESA_DEBUG_SEVERITY_HIGH,
                       "glGenerateMipmap() of sRGB texture without "
                       "sRGB decode\n");
      return GL_TRUE;
   }

   /*
    * Test that we can actually render in the texture's format.
    */
   if (!mipmap->FBO)
      _mesa_GenFramebuffers(1, &mipmap->FBO);
   _mesa_BindFramebuffer(GL_FRAMEBUFFER_EXT, mipmap->FBO);

   if (target == GL_TEXTURE_1D) {
      _mesa_FramebufferTexture1D(GL_FRAMEBUFFER_EXT,
                                    GL_COLOR_ATTACHMENT0_EXT,
                                    target, texObj->Name, srcLevel);
   }
#if 0
   /* other work is needed to enable 3D mipmap generation */
   else if (target == GL_TEXTURE_3D) {
      GLint zoffset = 0;
      _mesa_FramebufferTexture3D(GL_FRAMEBUFFER_EXT,
                                    GL_COLOR_ATTACHMENT0_EXT,
                                    target, texObj->Name, srcLevel, zoffset);
   }
#endif
   else {
      /* 2D / cube */
      _mesa_FramebufferTexture2D(GL_FRAMEBUFFER_EXT,
                                    GL_COLOR_ATTACHMENT0_EXT,
                                    target, texObj->Name, srcLevel);
   }

   status = _mesa_CheckFramebufferStatus(GL_FRAMEBUFFER_EXT);

   _mesa_BindFramebuffer(GL_FRAMEBUFFER_EXT, fboSave);

   if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
      _mesa_perf_debug(ctx, MESA_DEBUG_SEVERITY_HIGH,
                       "glGenerateMipmap() got incomplete FBO\n");
      return GL_TRUE;
   }

   return GL_FALSE;
}

void
_mesa_meta_glsl_generate_mipmap_cleanup(struct gen_mipmap_state *mipmap)
{
   if (mipmap->VAO == 0)
      return;
   _mesa_DeleteVertexArrays(1, &mipmap->VAO);
   mipmap->VAO = 0;
   _mesa_DeleteBuffers(1, &mipmap->VBO);
   mipmap->VBO = 0;

   _mesa_meta_blit_shader_table_cleanup(&mipmap->shaders);
}

/**
 * Called via ctx->Driver.GenerateMipmap()
 * Note: We don't yet support 3D textures, 1D/2D array textures or texture
 * borders.
 */
void
_mesa_meta_GenerateMipmap(struct gl_context *ctx, GLenum target,
                          struct gl_texture_object *texObj)
{
   struct gen_mipmap_state *mipmap = &ctx->Meta->Mipmap;
   struct vertex verts[4];
   const GLuint baseLevel = texObj->BaseLevel;
   const GLuint maxLevel = texObj->MaxLevel;
   const GLint maxLevelSave = texObj->MaxLevel;
   const GLboolean genMipmapSave = texObj->GenerateMipmap;
   const GLuint fboSave = ctx->DrawBuffer->Name;
   const GLuint currentTexUnitSave = ctx->Texture.CurrentUnit;
   const GLboolean use_glsl_version = ctx->Extensions.ARB_vertex_shader &&
                                      ctx->Extensions.ARB_fragment_shader;
   GLenum faceTarget;
   GLuint dstLevel;
   const GLint slice = 0;
   GLuint samplerSave;

   if (fallback_required(ctx, target, texObj)) {
      _mesa_generate_mipmap(ctx, target, texObj);
      return;
   }

   if (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X &&
       target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z) {
      faceTarget = target;
      target = GL_TEXTURE_CUBE_MAP;
   } else {
      faceTarget = target;
   }

   _mesa_meta_begin(ctx, MESA_META_ALL);

   /* Choose between glsl version and fixed function version of
    * GenerateMipmap function.
    */
   if (use_glsl_version) {
      _mesa_meta_setup_vertex_objects(&mipmap->VAO, &mipmap->VBO, true,
                                      2, 3, 0);
      _mesa_meta_setup_blit_shader(ctx, target, &mipmap->shaders);
   } else {
      _mesa_meta_setup_ff_tnl_for_blit(&mipmap->VAO, &mipmap->VBO, 3);
      _mesa_set_enable(ctx, target, GL_TRUE);
   }

   samplerSave = ctx->Texture.Unit[ctx->Texture.CurrentUnit].Sampler ?
      ctx->Texture.Unit[ctx->Texture.CurrentUnit].Sampler->Name : 0;

   if (currentTexUnitSave != 0)
      _mesa_BindTexture(target, texObj->Name);

   if (!mipmap->FBO) {
      _mesa_GenFramebuffers(1, &mipmap->FBO);
   }

   if (!mipmap->Sampler) {
      _mesa_GenSamplers(1, &mipmap->Sampler);
      _mesa_BindSampler(ctx->Texture.CurrentUnit, mipmap->Sampler);

      _mesa_SamplerParameteri(mipmap->Sampler,
                              GL_TEXTURE_MIN_FILTER,
                              GL_LINEAR_MIPMAP_LINEAR);
      _mesa_SamplerParameteri(mipmap->Sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      _mesa_SamplerParameteri(mipmap->Sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      _mesa_SamplerParameteri(mipmap->Sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      _mesa_SamplerParameteri(mipmap->Sampler, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

      /* We don't want to encode or decode sRGB values; treat them as linear.
       * This is not technically correct for GLES3 but we don't get any API
       * error at the moment.
       */
      if (ctx->Extensions.EXT_texture_sRGB_decode) {
         _mesa_SamplerParameteri(mipmap->Sampler, GL_TEXTURE_SRGB_DECODE_EXT,
               GL_SKIP_DECODE_EXT);
      }
   } else {
      _mesa_BindSampler(ctx->Texture.CurrentUnit, mipmap->Sampler);
   }

   _mesa_BindFramebuffer(GL_FRAMEBUFFER_EXT, mipmap->FBO);

   _mesa_TexParameteri(target, GL_GENERATE_MIPMAP, GL_FALSE);

   /* Silence valgrind warnings about reading uninitialized stack. */
   memset(verts, 0, sizeof(verts));

   /* Setup texture coordinates */
   _mesa_meta_setup_texture_coords(faceTarget,
                                   slice,
                                   0, 0, 1, /* width, height never used here */
                                   verts[0].tex,
                                   verts[1].tex,
                                   verts[2].tex,
                                   verts[3].tex);

   /* setup vertex positions */
   verts[0].x = -1.0F;
   verts[0].y = -1.0F;
   verts[1].x =  1.0F;
   verts[1].y = -1.0F;
   verts[2].x =  1.0F;
   verts[2].y =  1.0F;
   verts[3].x = -1.0F;
   verts[3].y =  1.0F;

   /* upload vertex data */
   _mesa_BufferData(GL_ARRAY_BUFFER_ARB, sizeof(verts),
                       verts, GL_DYNAMIC_DRAW_ARB);

   /* texture is already locked, unlock now */
   _mesa_unlock_texture(ctx, texObj);

   for (dstLevel = baseLevel + 1; dstLevel <= maxLevel; dstLevel++) {
      const struct gl_texture_image *srcImage;
      const GLuint srcLevel = dstLevel - 1;
      GLsizei srcWidth, srcHeight, srcDepth;
      GLsizei dstWidth, dstHeight, dstDepth;
      GLenum status;

      srcImage = _mesa_select_tex_image(ctx, texObj, faceTarget, srcLevel);
      assert(srcImage->Border == 0);

      /* src size */
      srcWidth = srcImage->Width;
      srcHeight = srcImage->Height;
      srcDepth = srcImage->Depth;

      /* new dst size */
      dstWidth = MAX2(1, srcWidth / 2);
      dstHeight = MAX2(1, srcHeight / 2);
      dstDepth = MAX2(1, srcDepth / 2);

      if (dstWidth == srcImage->Width &&
          dstHeight == srcImage->Height &&
          dstDepth == srcImage->Depth) {
         /* all done */
         break;
      }

      /* Allocate storage for the destination mipmap image(s) */

      /* Set MaxLevel large enough to hold the new level when we allocate it */
      _mesa_TexParameteri(target, GL_TEXTURE_MAX_LEVEL, dstLevel);

      if (!_mesa_prepare_mipmap_level(ctx, texObj, dstLevel,
                                      dstWidth, dstHeight, dstDepth,
                                      srcImage->Border,
                                      srcImage->InternalFormat,
                                      srcImage->TexFormat)) {
         /* All done.  We either ran out of memory or we would go beyond the
          * last valid level of an immutable texture if we continued.
          */
         break;
      }

      /* limit minification to src level */
      _mesa_TexParameteri(target, GL_TEXTURE_MAX_LEVEL, srcLevel);

      /* Set to draw into the current dstLevel */
      if (target == GL_TEXTURE_1D) {
         _mesa_FramebufferTexture1D(GL_FRAMEBUFFER_EXT,
                                       GL_COLOR_ATTACHMENT0_EXT,
                                       target,
                                       texObj->Name,
                                       dstLevel);
      }
      else if (target == GL_TEXTURE_3D) {
         GLint zoffset = 0; /* XXX unfinished */
         _mesa_FramebufferTexture3D(GL_FRAMEBUFFER_EXT,
                                       GL_COLOR_ATTACHMENT0_EXT,
                                       target,
                                       texObj->Name,
                                       dstLevel, zoffset);
      } else {
         /* 2D / cube */
         _mesa_FramebufferTexture2D(GL_FRAMEBUFFER_EXT,
                                       GL_COLOR_ATTACHMENT0_EXT,
                                       faceTarget,
                                       texObj->Name,
                                       dstLevel);
      }

      _mesa_DrawBuffer(GL_COLOR_ATTACHMENT0_EXT);

      /* sanity check */
      status = _mesa_CheckFramebufferStatus(GL_FRAMEBUFFER_EXT);
      if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
         _mesa_problem(ctx, "Unexpected incomplete framebuffer in "
                       "_mesa_meta_GenerateMipmap()");
         break;
      }

      assert(dstWidth == ctx->DrawBuffer->Width);
      assert(dstHeight == ctx->DrawBuffer->Height);

      /* setup viewport */
      _mesa_set_viewport(ctx, 0, 0, 0, dstWidth, dstHeight);

      _mesa_DrawArrays(GL_TRIANGLE_FAN, 0, 4);
   }

   _mesa_lock_texture(ctx, texObj); /* relock */

   _mesa_BindSampler(ctx->Texture.CurrentUnit, samplerSave);

   _mesa_meta_end(ctx);

   _mesa_TexParameteri(target, GL_TEXTURE_MAX_LEVEL, maxLevelSave);
   if (genMipmapSave)
      _mesa_TexParameteri(target, GL_GENERATE_MIPMAP, genMipmapSave);

   _mesa_BindFramebuffer(GL_FRAMEBUFFER_EXT, fboSave);
}
