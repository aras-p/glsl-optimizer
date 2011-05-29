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

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_gen_mipmap.h"

#include "st_debug.h"
#include "st_context.h"
#include "st_texture.h"
#include "st_gen_mipmap.h"
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
                 struct st_texture_object *stObj,
                 uint baseLevel, uint lastLevel)
{
   struct pipe_context *pipe = st->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_sampler_view *psv = st_get_texture_sampler_view(stObj, pipe);
   const uint face = _mesa_tex_target_to_face(target);

   assert(psv->texture == stObj->pt);
#if 0
   assert(target != GL_TEXTURE_3D); /* implemented but untested */
#endif

   /* check if we can render in the texture's format */
   /* XXX should probably kill this and always use util_gen_mipmap
      since this implements a sw fallback as well */
   if (!screen->is_format_supported(screen, psv->format, psv->texture->target,
                                    0, PIPE_BIND_RENDER_TARGET)) {
      return FALSE;
   }

   /* Disable conditional rendering. */
   if (st->render_condition) {
      pipe->render_condition(pipe, NULL, 0);
   }

   util_gen_mipmap(st->gen_mipmap, psv, face, baseLevel, lastLevel,
                   PIPE_TEX_FILTER_LINEAR);

   if (st->render_condition) {
      pipe->render_condition(pipe, st->render_condition, st->condition_mode);
   }

   return TRUE;
}


/**
 * Helper function to decompress an image.  The result is a 32-bpp RGBA
 * image with stride==width.
 */
static void
decompress_image(enum pipe_format format, int datatype,
                 const uint8_t *src, void *dst,
                 unsigned width, unsigned height, unsigned src_stride)
{
   const struct util_format_description *desc = util_format_description(format);
   const uint bw = util_format_get_blockwidth(format);
   const uint bh = util_format_get_blockheight(format);
   uint dst_stride = 4 * MAX2(width, bw);

   if (datatype == GL_FLOAT) {
      desc->unpack_rgba_float((float *)dst, dst_stride * sizeof(GLfloat), src, src_stride, width, height);
      if (width < bw || height < bh) {
	 float *dst_p = (float *)dst;
	 /* We're decompressing an image smaller than the compression
	  * block size.  We don't want garbage pixel values in the region
	  * outside (width x height) so replicate pixels from the (width
	  * x height) region to fill out the (bw x bh) block size.
	  */
	 uint x, y;
	 for (y = 0; y < bh; y++) {
	    for (x = 0; x < bw; x++) {
	       if (x >= width || y >= height) {
		  uint p = (y * bw + x) * 4;
		  dst_p[p + 0] = dst_p[0];
		  dst_p[p + 1] = dst_p[1];
		  dst_p[p + 2] = dst_p[2];
		  dst_p[p + 3] = dst_p[3];
	       }
	    }
	 }
      }
   } else {
      desc->unpack_rgba_8unorm((uint8_t *)dst, dst_stride, src, src_stride, width, height);
      if (width < bw || height < bh) {
	 uint8_t *dst_p = (uint8_t *)dst;
	 /* We're decompressing an image smaller than the compression
	  * block size.  We don't want garbage pixel values in the region
	  * outside (width x height) so replicate pixels from the (width
	  * x height) region to fill out the (bw x bh) block size.
	  */
	 uint x, y;
	 for (y = 0; y < bh; y++) {
	    for (x = 0; x < bw; x++) {
	       if (x >= width || y >= height) {
		  uint p = (y * bw + x) * 4;
		  dst_p[p + 0] = dst_p[0];
		  dst_p[p + 1] = dst_p[1];
		  dst_p[p + 2] = dst_p[2];
		  dst_p[p + 3] = dst_p[3];
	       }
	    }
	 }
      }
   }
}

/**
 * Helper function to compress an image.  The source is a 32-bpp RGBA image
 * with stride==width.
 */
static void
compress_image(enum pipe_format format, int datatype,
               const void *src, uint8_t *dst,
               unsigned width, unsigned height, unsigned dst_stride)
{
   const struct util_format_description *desc = util_format_description(format);
   const uint src_stride = 4 * width;

   if (datatype == GL_FLOAT)
      desc->pack_rgba_float(dst, dst_stride, (GLfloat *)src, src_stride * sizeof(GLfloat), width, height);
   else
      desc->pack_rgba_8unorm(dst, dst_stride, (uint8_t *)src, src_stride, width, height);
}


/**
 * Software fallback for generate mipmap levels.
 */
static void
fallback_generate_mipmap(struct gl_context *ctx, GLenum target,
                         struct gl_texture_object *texObj)
{
   struct pipe_context *pipe = st_context(ctx)->pipe;
   struct pipe_resource *pt = st_get_texobj_resource(texObj);
   const uint baseLevel = texObj->BaseLevel;
   const uint lastLevel = pt->last_level;
   const uint face = _mesa_tex_target_to_face(target);
   uint dstLevel;
   GLenum datatype;
   GLuint comps;
   GLboolean compressed;

   if (ST_DEBUG & DEBUG_FALLBACK)
      debug_printf("%s: fallback processing\n", __FUNCTION__);

   assert(target != GL_TEXTURE_3D); /* not done yet */

   compressed =
      _mesa_is_format_compressed(texObj->Image[face][baseLevel]->TexFormat);

   if (compressed) {
      GLenum type =
         _mesa_get_format_datatype(texObj->Image[face][baseLevel]->TexFormat);

      datatype = type == GL_UNSIGNED_NORMALIZED ? GL_UNSIGNED_BYTE : GL_FLOAT;
      comps = 4;
   }
   else {
      _mesa_format_to_type_and_comps(texObj->Image[face][baseLevel]->TexFormat,
                                     &datatype, &comps);
      assert(comps > 0 && "bad texture format in fallback_generate_mipmap()");
   }

   for (dstLevel = baseLevel + 1; dstLevel <= lastLevel; dstLevel++) {
      const uint srcLevel = dstLevel - 1;
      const uint srcWidth = u_minify(pt->width0, srcLevel);
      const uint srcHeight = u_minify(pt->height0, srcLevel);
      const uint srcDepth = u_minify(pt->depth0, srcLevel);
      const uint dstWidth = u_minify(pt->width0, dstLevel);
      const uint dstHeight = u_minify(pt->height0, dstLevel);
      const uint dstDepth = u_minify(pt->depth0, dstLevel);
      struct pipe_transfer *srcTrans, *dstTrans;
      const ubyte *srcData;
      ubyte *dstData;
      int srcStride, dstStride;

      srcTrans = pipe_get_transfer(pipe, pt, srcLevel,
                                   face,
                                   PIPE_TRANSFER_READ, 0, 0,
                                   srcWidth, srcHeight);

      dstTrans = pipe_get_transfer(pipe, pt, dstLevel,
                                   face,
                                   PIPE_TRANSFER_WRITE, 0, 0,
                                   dstWidth, dstHeight);

      srcData = (ubyte *) pipe_transfer_map(pipe, srcTrans);
      dstData = (ubyte *) pipe_transfer_map(pipe, dstTrans);

      srcStride = srcTrans->stride / util_format_get_blocksize(srcTrans->resource->format);
      dstStride = dstTrans->stride / util_format_get_blocksize(dstTrans->resource->format);

     /* this cannot work correctly for 3d since it does
        not respect layerStride. */
      if (compressed) {
         const enum pipe_format format = pt->format;
         const uint bw = util_format_get_blockwidth(format);
         const uint bh = util_format_get_blockheight(format);
         const uint srcWidth2 = align(srcWidth, bw);
         const uint srcHeight2 = align(srcHeight, bh);
         const uint dstWidth2 = align(dstWidth, bw);
         const uint dstHeight2 = align(dstHeight, bh);
         uint8_t *srcTemp, *dstTemp;

         assert(comps == 4);

         srcTemp = malloc(srcWidth2 * srcHeight2 * comps * (datatype == GL_FLOAT ? 4 : 1));
         dstTemp = malloc(dstWidth2 * dstHeight2 * comps * (datatype == GL_FLOAT ? 4 : 1));

         /* decompress the src image: srcData -> srcTemp */
         decompress_image(format, datatype, srcData, srcTemp, srcWidth2, srcHeight2, srcTrans->stride);

         _mesa_generate_mipmap_level(target, datatype, comps,
                                     0 /*border*/,
                                     srcWidth2, srcHeight2, srcDepth,
                                     srcTemp,
                                     srcWidth2, /* stride in texels */
                                     dstWidth2, dstHeight2, dstDepth,
                                     dstTemp,
                                     dstWidth2); /* stride in texels */

         /* compress the new image: dstTemp -> dstData */
         compress_image(format, datatype, dstTemp, dstData, dstWidth2, dstHeight2, dstTrans->stride);

         free(srcTemp);
         free(dstTemp);
      }
      else {
         _mesa_generate_mipmap_level(target, datatype, comps,
                                     0 /*border*/,
                                     srcWidth, srcHeight, srcDepth,
                                     srcData,
                                     srcStride, /* stride in texels */
                                     dstWidth, dstHeight, dstDepth,
                                     dstData,
                                     dstStride); /* stride in texels */
      }

      pipe_transfer_unmap(pipe, srcTrans);
      pipe_transfer_unmap(pipe, dstTrans);

      pipe->transfer_destroy(pipe, srcTrans);
      pipe->transfer_destroy(pipe, dstTrans);
   }
}


/**
 * Compute the expected number of mipmap levels in the texture given
 * the width/height/depth of the base image and the GL_TEXTURE_BASE_LEVEL/
 * GL_TEXTURE_MAX_LEVEL settings.  This will tell us how many mipmap
 * levels should be generated.
 */
static GLuint
compute_num_levels(struct gl_context *ctx,
                   struct gl_texture_object *texObj,
                   GLenum target)
{
   if (target == GL_TEXTURE_RECTANGLE_ARB) {
      return 1;
   }
   else {
      const struct gl_texture_image *baseImage = 
         _mesa_get_tex_image(ctx, texObj, target, texObj->BaseLevel);
      GLuint size, numLevels;

      size = MAX2(baseImage->Width2, baseImage->Height2);
      size = MAX2(size, baseImage->Depth2);

      numLevels = texObj->BaseLevel;

      while (size > 0) {
         numLevels++;
         size >>= 1;
      }

      numLevels = MIN2(numLevels, texObj->MaxLevel + 1);

      assert(numLevels >= 1);

      return numLevels;
   }
}


/**
 * Called via ctx->Driver.GenerateMipmap().
 */
void
st_generate_mipmap(struct gl_context *ctx, GLenum target,
                   struct gl_texture_object *texObj)
{
   struct st_context *st = st_context(ctx);
   struct st_texture_object *stObj = st_texture_object(texObj);
   struct pipe_resource *pt = st_get_texobj_resource(texObj);
   const uint baseLevel = texObj->BaseLevel;
   uint lastLevel;
   uint dstLevel;

   if (!pt)
      return;

   /* not sure if this ultimately actually should work,
      but we're not supporting multisampled textures yet. */
   assert(pt->nr_samples < 2);

   /* find expected last mipmap level to generate*/
   lastLevel = compute_num_levels(ctx, texObj, target) - 1;

   if (lastLevel == 0)
      return;

   /* The texture isn't in a "complete" state yet so set the expected
    * lastLevel here, since it won't get done in st_finalize_texture().
    */
   stObj->lastLevel = lastLevel;

   if (pt->last_level < lastLevel) {
      /* The current gallium texture doesn't have space for all the
       * mipmap levels we need to generate.  So allocate a new texture.
       */
      struct pipe_resource *oldTex = stObj->pt;

      /* create new texture with space for more levels */
      stObj->pt = st_texture_create(st,
                                    oldTex->target,
                                    oldTex->format,
                                    lastLevel,
                                    oldTex->width0,
                                    oldTex->height0,
                                    oldTex->depth0,
                                    oldTex->array_size,
                                    oldTex->bind);

      /* This will copy the old texture's base image into the new texture
       * which we just allocated.
       */
      st_finalize_texture(ctx, st->pipe, texObj);

      /* release the old tex (will likely be freed too) */
      pipe_resource_reference(&oldTex, NULL);
      pipe_sampler_view_reference(&stObj->sampler_view, NULL);
   }
   else {
      /* Make sure that the base texture image data is present in the
       * texture buffer.
       */
      st_finalize_texture(ctx, st->pipe, texObj);
   }

   pt = stObj->pt;

   assert(pt->last_level >= lastLevel);

   /* Try to generate the mipmap by rendering/texturing.  If that fails,
    * use the software fallback.
    */
   if (!st_render_mipmap(st, target, stObj, baseLevel, lastLevel)) {
      /* since the util code actually also has a fallback, should
         probably make it never fail and kill this */
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
                                 dstDepth, border, srcImage->InternalFormat,
                                 srcImage->TexFormat);

      stImage = st_texture_image(dstImage);
      stImage->level = dstLevel;

      pipe_resource_reference(&stImage->pt, pt);
   }
}
