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
#include "main/macros.h"
#include "main/mipmap.h"
#include "main/teximage.h"
#include "main/texformat.h"

#include "shader/prog_instruction.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_gen_mipmap.h"
#include "util/u_math.h"

#include "cso_cache/cso_cache.h"
#include "cso_cache/cso_context.h"

#include "st_debug.h"
#include "st_context.h"
#include "st_gen_mipmap.h"
#include "st_texture.h"
#include "st_cb_texture.h"
#include "st_inlines.h"


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
   if (!screen->is_format_supported(screen, pt->format, pt->target,
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
   
   if (ST_DEBUG & DEBUG_FALLBACK)
      debug_printf("%s: fallback processing\n", __FUNCTION__);

   assert(target != GL_TEXTURE_3D); /* not done yet */

   _mesa_format_to_type_and_comps(texObj->Image[face][baseLevel]->TexFormat,
                                  &datatype, &comps);

   for (dstLevel = baseLevel + 1; dstLevel <= lastLevel; dstLevel++) {
      const uint srcLevel = dstLevel - 1;
      struct pipe_transfer *srcTrans, *dstTrans;
      const ubyte *srcData;
      ubyte *dstData;
      int srcStride, dstStride;

      srcTrans = st_cond_flush_get_tex_transfer(st_context(ctx), pt, face,
						srcLevel, zslice,
						PIPE_TRANSFER_READ, 0, 0,
						u_minify(pt->width0, srcLevel),
						u_minify(pt->height0, srcLevel));

      dstTrans = st_cond_flush_get_tex_transfer(st_context(ctx), pt, face,
						dstLevel, zslice,
						PIPE_TRANSFER_WRITE, 0, 0,
						u_minify(pt->width0, dstLevel),
						u_minify(pt->height0, dstLevel));

      srcData = (ubyte *) screen->transfer_map(screen, srcTrans);
      dstData = (ubyte *) screen->transfer_map(screen, dstTrans);

      srcStride = srcTrans->stride / util_format_get_blocksize(srcTrans->texture->format);
      dstStride = dstTrans->stride / util_format_get_blocksize(dstTrans->texture->format);

      _mesa_generate_mipmap_level(target, datatype, comps,
                                  0 /*border*/,
                                  u_minify(pt->width0, srcLevel),
                                  u_minify(pt->height0, srcLevel),
                                  u_minify(pt->depth0, srcLevel),
                                  srcData,
                                  srcStride, /* stride in texels */
                                  u_minify(pt->width0, dstLevel),
                                  u_minify(pt->height0, dstLevel),
                                  u_minify(pt->depth0, dstLevel),
                                  dstData,
                                  dstStride); /* stride in texels */

      screen->transfer_unmap(screen, srcTrans);
      screen->transfer_unmap(screen, dstTrans);

      screen->tex_transfer_destroy(srcTrans);
      screen->tex_transfer_destroy(dstTrans);
   }
}


/**
 * Compute the expected number of mipmap levels in the texture given
 * the width/height/depth of the base image and the GL_TEXTURE_BASE_LEVEL/
 * GL_TEXTURE_MAX_LEVEL settings.  This will tell us how many mipmap
 * level should be generated.
 */
static GLuint
compute_num_levels(GLcontext *ctx,
                   struct gl_texture_object *texObj,
                   GLenum target)
{
   if (target == GL_TEXTURE_RECTANGLE_ARB) {
      return 1;
   }
   else {
      const GLuint maxLevels = texObj->MaxLevel - texObj->BaseLevel + 1;
      const struct gl_texture_image *baseImage = 
         _mesa_get_tex_image(ctx, texObj, target, texObj->BaseLevel);
      GLuint size, numLevels;

      size = MAX2(baseImage->Width2, baseImage->Height2);
      size = MAX2(size, baseImage->Depth2);

      numLevels = 0;

      while (size > 0) {
         numLevels++;
         size >>= 1;
      }

      numLevels = MIN2(numLevels, maxLevels);

      return numLevels;
   }
}


void
st_generate_mipmap(GLcontext *ctx, GLenum target,
                   struct gl_texture_object *texObj)
{
   struct st_context *st = ctx->st;
   struct pipe_texture *pt = st_get_texobj_texture(texObj);
   const uint baseLevel = texObj->BaseLevel;
   uint lastLevel;
   uint dstLevel;

   if (!pt)
      return;

   /* find expected last mipmap level */
   lastLevel = compute_num_levels(ctx, texObj, target) - 1;

   if (lastLevel == 0)
      return;

   if (pt->last_level < lastLevel) {
      /* The current gallium texture doesn't have space for all the
       * mipmap levels we need to generate.  So allocate a new texture.
       */
      struct st_texture_object *stObj = st_texture_object(texObj);
      struct pipe_texture *oldTex = stObj->pt;
      GLboolean needFlush;

      /* create new texture with space for more levels */
      stObj->pt = st_texture_create(st,
                                    oldTex->target,
                                    oldTex->format,
                                    lastLevel,
                                    oldTex->width0,
                                    oldTex->height0,
                                    oldTex->depth0,
                                    oldTex->tex_usage);

      /* The texture isn't in a "complete" state yet so set the expected
       * lastLevel here, since it won't get done in st_finalize_texture().
       */
      stObj->lastLevel = lastLevel;

      /* This will copy the old texture's base image into the new texture
       * which we just allocated.
       */
      st_finalize_texture(ctx, st->pipe, texObj, &needFlush);

      /* release the old tex (will likely be freed too) */
      pipe_texture_reference(&oldTex, NULL);

      pt = stObj->pt;
   }

   assert(lastLevel <= pt->last_level);

   /* Recall that the Mesa BaseLevel image is stored in the gallium
    * texture's level[0] position.  So pass baseLevel=0 here.
    */
   if (!st_render_mipmap(st, target, pt, 0, lastLevel)) {
      fallback_generate_mipmap(ctx, target, texObj);
   }

   /* Fill in the Mesa gl_texture_image fields */
   for (dstLevel = baseLevel + 1; dstLevel <= lastLevel; dstLevel++) {
      const uint srcLevel = dstLevel - 1;
      const struct gl_texture_image *srcImage
         = _mesa_get_tex_image(ctx, texObj, target, srcLevel);
      struct gl_texture_image *dstImage;
      struct st_texture_image *stImage;
      uint dstWidth = u_minify(pt->width0, dstLevel);
      uint dstHeight = u_minify(pt->height0, dstLevel);
      uint dstDepth = u_minify(pt->depth0, dstLevel); 
      uint border = srcImage->Border;

      dstImage = _mesa_get_tex_image(ctx, texObj, target, dstLevel);
      if (!dstImage) {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "generating mipmaps");
         return;
      }

      /* Free old image data */
      if (dstImage->Data)
         ctx->Driver.FreeTexImageData(ctx, dstImage);

      /* initialize new image */
      _mesa_init_teximage_fields(ctx, target, dstImage, dstWidth, dstHeight,
                                 dstDepth, border, srcImage->InternalFormat);

      dstImage->TexFormat = srcImage->TexFormat;

      stImage = st_texture_image(dstImage);
      stImage->level = dstLevel;

      pipe_texture_reference(&stImage->pt, pt);
   }
}
