/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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
#include "main/mipmap.h"
#include "main/teximage.h"
#include "main/texformat.h"

#include "shader/prog_instruction.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_inlines.h"
#include "util/u_gen_mipmap.h"

#include "cso_cache/cso_cache.h"
#include "cso_cache/cso_context.h"

#include "st_context.h"
#include "st_draw.h"
#include "st_gen_mipmap.h"
#include "st_program.h"
#include "st_texture.h"
#include "st_cb_texture.h"


/**
 * one-time init for generate mipmap
 * XXX Note: there may be other times we need no-op/simple state like this.
 * In that case, some code refactoring would be good.
 */
void
st_init_generate_mipmap(struct st_context *st)
{
   st->gen_mipmap = util_create_gen_mipmap(st->pipe, st->cso_context);
}


void
st_destroy_generate_mipmap(struct st_context *st)
{
   util_destroy_gen_mipmap(st->gen_mipmap);
   st->gen_mipmap = NULL;
}


/**
 * Generate mipmap levels using hardware rendering.
 * \return TRUE if successful, FALSE if not possible
 */
static boolean
st_render_mipmap(struct st_context *st,
                 GLenum target,
                 struct pipe_texture *pt,
                 uint baseLevel, uint lastLevel)
{
   struct pipe_context *pipe = st->pipe;
   struct pipe_screen *screen = pipe->screen;
   const uint face = _mesa_tex_target_to_face(target);

   assert(target != GL_TEXTURE_3D); /* not done yet */

   /* check if we can render in the texture's format */
   if (!screen->is_format_supported(screen, pt->format, target,
                                    PIPE_TEXTURE_USAGE_RENDER_TARGET, 0)) {
      return FALSE;
   }

   util_gen_mipmap(st->gen_mipmap, pt, face, baseLevel, lastLevel,
                   PIPE_TEX_FILTER_LINEAR);

   return TRUE;
}


static void
fallback_generate_mipmap(GLcontext *ctx, GLenum target,
                         struct gl_texture_object *texObj)
{
   struct pipe_context *pipe = ctx->st->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_texture *pt = st_get_texobj_texture(texObj);
   const uint baseLevel = texObj->BaseLevel;
   const uint lastLevel = pt->last_level;
   const uint face = _mesa_tex_target_to_face(target), zslice = 0;
   uint dstLevel;
   GLenum datatype;
   GLuint comps;

   assert(target != GL_TEXTURE_3D); /* not done yet */

   _mesa_format_to_type_and_comps(texObj->Image[face][baseLevel]->TexFormat,
                                  &datatype, &comps);

   for (dstLevel = baseLevel + 1; dstLevel <= lastLevel; dstLevel++) {
      const uint srcLevel = dstLevel - 1;
      struct pipe_surface *srcSurf, *dstSurf;
      const ubyte *srcData;
      ubyte *dstData;

      srcSurf = screen->get_tex_surface(screen, pt, face, srcLevel, zslice,
                                        PIPE_BUFFER_USAGE_CPU_READ);
      dstSurf = screen->get_tex_surface(screen, pt, face, dstLevel, zslice,
                                        PIPE_BUFFER_USAGE_CPU_WRITE);

      srcData = (ubyte *) pipe_buffer_map(pipe->screen, srcSurf->buffer,
                                          PIPE_BUFFER_USAGE_CPU_READ)
              + srcSurf->offset;
      dstData = (ubyte *) pipe_buffer_map(pipe->screen, dstSurf->buffer,
                                          PIPE_BUFFER_USAGE_CPU_WRITE)
              + dstSurf->offset;

      _mesa_generate_mipmap_level(target, datatype, comps,
                   0 /*border*/,
                   pt->width[srcLevel], pt->height[srcLevel], pt->depth[srcLevel],
                   srcSurf->stride, /* stride in bytes */
                   srcData,
                   pt->width[dstLevel], pt->height[dstLevel], pt->depth[dstLevel],
                   dstSurf->stride, /* stride in bytes */
                   dstData);

      pipe_buffer_unmap(pipe->screen, srcSurf->buffer);
      pipe_buffer_unmap(pipe->screen, dstSurf->buffer);

      pipe_surface_reference(&srcSurf, NULL);
      pipe_surface_reference(&dstSurf, NULL);
   }
}


void
st_generate_mipmap(GLcontext *ctx, GLenum target,
                   struct gl_texture_object *texObj)
{
   struct st_context *st = ctx->st;
   struct pipe_texture *pt = st_get_texobj_texture(texObj);
   const uint baseLevel = texObj->BaseLevel;
   const uint lastLevel = pt->last_level;
   uint dstLevel;

   if (!st_render_mipmap(st, target, pt, baseLevel, lastLevel)) {
      fallback_generate_mipmap(ctx, target, texObj);
   }

   /* Fill in the Mesa gl_texture_image fields */
   for (dstLevel = baseLevel + 1; dstLevel <= lastLevel; dstLevel++) {
      const uint srcLevel = dstLevel - 1;
      const struct gl_texture_image *srcImage
         = _mesa_get_tex_image(ctx, texObj, target, srcLevel);
      struct gl_texture_image *dstImage;
      struct st_texture_image *stImage;
      uint dstWidth = pt->width[dstLevel];
      uint dstHeight = pt->height[dstLevel];
      uint dstDepth = pt->depth[dstLevel];
      uint border = srcImage->Border;

      dstImage = _mesa_get_tex_image(ctx, texObj, target, dstLevel);
      if (!dstImage) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "generating mipmaps");
         return;
      }

      if (dstImage->ImageOffsets)
         _mesa_free(dstImage->ImageOffsets);

      /* Free old image data */
      if (dstImage->Data)
         ctx->Driver.FreeTexImageData(ctx, dstImage);

      /* initialize new image */
      _mesa_init_teximage_fields(ctx, target, dstImage, dstWidth, dstHeight,
                                 dstDepth, border, srcImage->InternalFormat);

      dstImage->TexFormat = srcImage->TexFormat;

      stImage = (struct st_texture_image *) dstImage;
      pipe_texture_reference(&stImage->pt, pt);
   }
}
