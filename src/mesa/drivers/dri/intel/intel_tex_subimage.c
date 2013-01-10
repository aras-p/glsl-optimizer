
/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "main/bufferobj.h"
#include "main/macros.h"
#include "main/mtypes.h"
#include "main/pbo.h"
#include "main/texobj.h"
#include "main/texstore.h"
#include "main/texcompress.h"
#include "main/enums.h"

#include "intel_batchbuffer.h"
#include "intel_context.h"
#include "intel_tex.h"
#include "intel_mipmap_tree.h"
#include "intel_blit.h"

#define FILE_DEBUG_FLAG DEBUG_TEXTURE

static bool
intel_blit_texsubimage(struct gl_context * ctx,
		       struct gl_texture_image *texImage,
		       GLint xoffset, GLint yoffset,
		       GLint width, GLint height,
		       GLenum format, GLenum type, const void *pixels,
		       const struct gl_pixelstore_attrib *packing)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_image *intelImage = intel_texture_image(texImage);
   GLuint dstRowStride = 0;
   drm_intel_bo *temp_bo = NULL;
   unsigned int blit_x = 0, blit_y = 0;
   unsigned long pitch;
   uint32_t tiling_mode = I915_TILING_NONE;
   GLubyte *dstMap;

   /* Try to do a blit upload of the subimage if the texture is
    * currently busy.
    */
   if (!intelImage->mt)
      return false;

   /* The blitter can't handle Y tiling */
   if (intelImage->mt->region->tiling == I915_TILING_Y)
      return false;

   if (texImage->TexObject->Target != GL_TEXTURE_2D)
      return false;

   /* On gen6, it's probably not worth swapping to the blit ring to do
    * this because of all the overhead involved.
    */
   if (intel->gen >= 6)
      return false;

   if (!drm_intel_bo_busy(intelImage->mt->region->bo))
      return false;

   DBG("BLT subimage %s target %s level %d offset %d,%d %dx%d\n",
       __FUNCTION__,
       _mesa_lookup_enum_by_nr(texImage->TexObject->Target),
       texImage->Level, xoffset, yoffset, width, height);

   pixels = _mesa_validate_pbo_teximage(ctx, 2, width, height, 1,
					format, type, pixels, packing,
					"glTexSubImage");
   if (!pixels)
      return false;

   temp_bo = drm_intel_bo_alloc_tiled(intel->bufmgr,
				      "subimage blit bo",
				      width, height,
				      intelImage->mt->cpp,
				      &tiling_mode,
				      &pitch,
				      0);
   if (temp_bo == NULL) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "intelTexSubImage");
      return false;
   }

   if (drm_intel_gem_bo_map_gtt(temp_bo)) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "intelTexSubImage");
      return false;
   }

   dstMap = temp_bo->virtual;
   dstRowStride = pitch;

   intel_miptree_get_image_offset(intelImage->mt, texImage->Level,
				  intelImage->base.Base.Face,
				  &blit_x, &blit_y);
   blit_x += xoffset;
   blit_y += yoffset;

   if (!_mesa_texstore(ctx, 2, texImage->_BaseFormat,
		       texImage->TexFormat,
		       dstRowStride,
		       &dstMap,
		       width, height, 1,
		       format, type, pixels, packing)) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "intelTexSubImage");
   }

   bool ret;

   drm_intel_gem_bo_unmap_gtt(temp_bo);

   ret = intelEmitCopyBlit(intel,
			   intelImage->mt->cpp,
			   dstRowStride,
			   temp_bo, 0, false,
			   intelImage->mt->region->pitch,
			   intelImage->mt->region->bo, 0,
			   intelImage->mt->region->tiling,
			   0, 0, blit_x, blit_y, width, height,
			   GL_COPY);
   assert(ret);

   drm_intel_bo_unreference(temp_bo);
   _mesa_unmap_teximage_pbo(ctx, packing);

   return true;
}

/**
 * \brief A fast path for glTexImage and glTexSubImage.
 *
 * \param for_glTexImage Was this called from glTexImage or glTexSubImage?
 *
 * This fast path is taken when the hardware natively supports the texture
 * format (such as GL_BGRA) and when the texture memory is X-tiled. It uploads
 * the texture data by mapping the texture memory without a GTT fence, thus
 * acquiring a tiled view of the memory, and then memcpy'ing sucessive
 * subspans within each tile.
 *
 * This is a performance win over the conventional texture upload path because
 * it avoids the performance penalty of writing through the write-combine
 * buffer. In the conventional texture upload path,
 * texstore.c:store_texsubimage(), the texture memory is mapped through a GTT
 * fence, thus acquiring a linear view of the memory, then each row in the
 * image is memcpy'd. In this fast path, we replace each row's memcpy with
 * a sequence of memcpy's over each bit6 swizzle span in the row.
 *
 * This fast path's use case is Google Chrome's paint rectangles.  Chrome (as
 * of version 21) renders each page as a tiling of 256x256 GL_BGRA textures.
 * Each page's content is initially uploaded with glTexImage2D and damaged
 * regions are updated with glTexSubImage2D. On some workloads, the
 * performance gain of this fastpath on Sandybridge is over 5x.
 */
bool
intel_texsubimage_tiled_memcpy(struct gl_context * ctx,
                               GLuint dims,
                               struct gl_texture_image *texImage,
                               GLint xoffset, GLint yoffset, GLint zoffset,
                               GLsizei width, GLsizei height, GLsizei depth,
                               GLenum format, GLenum type,
                               const GLvoid *pixels,
                               const struct gl_pixelstore_attrib *packing,
                               bool for_glTexImage)
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_texture_image *image = intel_texture_image(texImage);

   /* The miptree's buffer. */
   drm_intel_bo *bo;

   int error = 0;

   /* This fastpath is restricted to a specific texture type: level 0 of
    * a 2D BGRA texture. It could be generalized to support more types by
    * varying the arithmetic loop below.
    */
   if (!intel->has_llc ||
       format != GL_BGRA ||
       type != GL_UNSIGNED_BYTE ||
       texImage->TexFormat != MESA_FORMAT_ARGB8888 ||
       texImage->TexObject->Target != GL_TEXTURE_2D ||
       texImage->Level != 0 ||
       pixels == NULL ||
       _mesa_is_bufferobj(packing->BufferObj) ||
       packing->Alignment > 4 ||
       packing->SkipPixels > 0 ||
       packing->SkipRows > 0 ||
       (packing->RowLength != 0 && packing->RowLength != width) ||
       packing->SwapBytes ||
       packing->LsbFirst ||
       packing->Invert)
      return false;

   if (for_glTexImage)
      ctx->Driver.AllocTextureImageBuffer(ctx, texImage);

   if (!image->mt ||
       image->mt->region->tiling != I915_TILING_X) {
      /* The algorithm below is written only for X-tiled memory. */
      return false;
   }

   bo = image->mt->region->bo;

   if (drm_intel_bo_references(intel->batch.bo, bo)) {
      perf_debug("Flushing before mapping a referenced bo.\n");
      intel_batchbuffer_flush(intel);
   }

   if (unlikely(INTEL_DEBUG & DEBUG_PERF)) {
      if (drm_intel_bo_busy(bo)) {
         perf_debug("Mapping a busy BO, causing a stall on the GPU.\n");
      }
   }

   error = drm_intel_bo_map(bo, true /*write_enable*/);
   if (error || bo->virtual == NULL) {
      DBG("%s: failed to map bo\n", __FUNCTION__);
      return false;
   }

   /* We postponed printing this message until having committed to executing
    * the function.
    */
   DBG("%s: level=%d offset=(%d,%d) (w,h)=(%d,%d)\n",
       __FUNCTION__, texImage->Level, xoffset, yoffset, width, height);

   /* In the tiling algorithm below, some variables are in units of pixels,
    * others are in units of bytes, and others (such as height) are unitless.
    * Each variable name is suffixed with its units.
    */

   const uint32_t x_max_pixels = xoffset + width;
   const uint32_t y_max_pixels = yoffset + height;

   const uint32_t tile_size_bytes = 4096;

   const uint32_t tile_width_bytes = 512;
   const uint32_t tile_width_pixels = 128;

   const uint32_t tile_height = 8;

   const uint32_t cpp = 4; /* chars per pixel of GL_BGRA */
   const uint32_t swizzle_width_pixels = 16;

   const uint32_t stride_bytes = image->mt->region->pitch;
   const uint32_t width_tiles = stride_bytes / tile_width_bytes;

   for (uint32_t y_pixels = yoffset; y_pixels < y_max_pixels; ++y_pixels) {
      const uint32_t y_offset_bytes = (y_pixels / tile_height) * width_tiles * tile_size_bytes
                                    + (y_pixels % tile_height) * tile_width_bytes;

      for (uint32_t x_pixels = xoffset; x_pixels < x_max_pixels; x_pixels += swizzle_width_pixels) {
         const uint32_t x_offset_bytes = (x_pixels / tile_width_pixels) * tile_size_bytes
                                       + (x_pixels % tile_width_pixels) * cpp;

         intptr_t offset_bytes = y_offset_bytes + x_offset_bytes;
         if (intel->has_swizzling) {
#if 0
            /* Clear, unoptimized version. */
            bool bit6 = (offset_bytes >> 6) & 1;
            bool bit9 = (offset_bytes >> 9) & 1;
            bool bit10 = (offset_bytes >> 10) & 1;

            if (bit9 ^ bit10)
               offset_bytes ^= (1 << 6);
#else
            /* Optimized, obfuscated version. */
            offset_bytes ^= ((offset_bytes >> 3) ^ (offset_bytes >> 4))
                          & (1 << 6);
#endif
         }

         const uint32_t swizzle_bound_pixels = ALIGN(x_pixels + 1, swizzle_width_pixels);
         const uint32_t memcpy_bound_pixels = MIN2(x_max_pixels, swizzle_bound_pixels);
         const uint32_t copy_size = cpp * (memcpy_bound_pixels - x_pixels);

         memcpy(bo->virtual + offset_bytes, pixels, copy_size);
         pixels += copy_size;
         x_pixels -= (x_pixels % swizzle_width_pixels);
      }
   }

   drm_intel_bo_unmap(bo);
   return true;
}

static void
intelTexSubImage(struct gl_context * ctx,
                 GLuint dims,
                 struct gl_texture_image *texImage,
                 GLint xoffset, GLint yoffset, GLint zoffset,
                 GLsizei width, GLsizei height, GLsizei depth,
                 GLenum format, GLenum type,
                 const GLvoid * pixels,
                 const struct gl_pixelstore_attrib *packing)
{
   bool ok;

   ok = intel_texsubimage_tiled_memcpy(ctx, dims, texImage,
                                       xoffset, yoffset, zoffset,
                                       width, height, depth,
                                       format, type, pixels, packing,
                                       false /*for_glTexImage*/);
   if (ok)
     return;

   /* The intel_blit_texsubimage() function only handles 2D images */
   if (dims != 2 || !intel_blit_texsubimage(ctx, texImage,
			       xoffset, yoffset,
			       width, height,
			       format, type, pixels, packing)) {
      _mesa_store_texsubimage(ctx, dims, texImage,
                              xoffset, yoffset, zoffset,
                              width, height, depth,
                              format, type, pixels, packing);
   }
}

void
intelInitTextureSubImageFuncs(struct dd_function_table *functions)
{
   functions->TexSubImage = intelTexSubImage;
}
