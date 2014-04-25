
/**************************************************************************
 *
 * Copyright 2003 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "main/bufferobj.h"
#include "main/image.h"
#include "main/macros.h"
#include "main/mtypes.h"
#include "main/pbo.h"
#include "main/texobj.h"
#include "main/texstore.h"
#include "main/texcompress.h"
#include "main/enums.h"

#include "brw_context.h"
#include "intel_batchbuffer.h"
#include "intel_tex.h"
#include "intel_mipmap_tree.h"
#include "intel_blit.h"

#define FILE_DEBUG_FLAG DEBUG_TEXTURE

#define ALIGN_DOWN(a, b) ROUND_DOWN_TO(a, b)
#define ALIGN_UP(a, b) ALIGN(a, b)

/* Tile dimensions.
 * Width and span are in bytes, height is in pixels (i.e. unitless).
 * A "span" is the most number of bytes we can copy from linear to tiled
 * without needing to calculate a new destination address.
 */
static const uint32_t xtile_width = 512;
static const uint32_t xtile_height = 8;
static const uint32_t xtile_span = 64;
static const uint32_t ytile_width = 128;
static const uint32_t ytile_height = 32;
static const uint32_t ytile_span = 16;

typedef void *(*mem_copy_fn)(void *dest, const void *src, size_t n);

/**
 * Each row from y0 to y1 is copied in three parts: [x0,x1), [x1,x2), [x2,x3).
 * These ranges are in bytes, i.e. pixels * bytes-per-pixel.
 * The first and last ranges must be shorter than a "span" (the longest linear
 * stretch within a tile) and the middle must equal a whole number of spans.
 * Ranges may be empty.  The region copied must land entirely within one tile.
 * 'dst' is the start of the tile and 'src' is the corresponding
 * address to copy from, though copying begins at (x0, y0).
 * To enable swizzling 'swizzle_bit' must be 1<<6, otherwise zero.
 * Swizzling flips bit 6 in the copy destination offset, when certain other
 * bits are set in it.
 */
typedef void (*tile_copy_fn)(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3,
                             uint32_t y0, uint32_t y1,
                             char *dst, const char *src,
                             uint32_t src_pitch,
                             uint32_t swizzle_bit,
                             mem_copy_fn mem_copy);


static bool
intel_blit_texsubimage(struct gl_context * ctx,
		       struct gl_texture_image *texImage,
		       GLint xoffset, GLint yoffset,
		       GLint width, GLint height,
		       GLenum format, GLenum type, const void *pixels,
		       const struct gl_pixelstore_attrib *packing)
{
   struct brw_context *brw = brw_context(ctx);
   struct intel_texture_image *intelImage = intel_texture_image(texImage);

   /* Try to do a blit upload of the subimage if the texture is
    * currently busy.
    */
   if (!intelImage->mt)
      return false;

   /* Prior to Sandybridge, the blitter can't handle Y tiling */
   if (brw->gen < 6 && intelImage->mt->tiling == I915_TILING_Y)
      return false;

   if (texImage->TexObject->Target != GL_TEXTURE_2D)
      return false;

   /* On gen6, it's probably not worth swapping to the blit ring to do
    * this because of all the overhead involved.
    */
   if (brw->gen >= 6)
      return false;

   if (!drm_intel_bo_busy(intelImage->mt->bo))
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

   struct intel_mipmap_tree *temp_mt =
      intel_miptree_create(brw, GL_TEXTURE_2D, texImage->TexFormat,
                           0, 0,
                           width, height, 1,
                           false, 0, INTEL_MIPTREE_TILING_NONE);
   if (!temp_mt)
      goto err;

   GLubyte *dst = intel_miptree_map_raw(brw, temp_mt);
   if (!dst)
      goto err;

   if (!_mesa_texstore(ctx, 2, texImage->_BaseFormat,
		       texImage->TexFormat,
		       temp_mt->pitch,
		       &dst,
		       width, height, 1,
		       format, type, pixels, packing)) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "intelTexSubImage");
   }

   intel_miptree_unmap_raw(brw, temp_mt);

   bool ret;

   ret = intel_miptree_blit(brw,
                            temp_mt, 0, 0,
                            0, 0, false,
                            intelImage->mt, texImage->Level, texImage->Face,
                            xoffset, yoffset, false,
                            width, height, GL_COPY);
   assert(ret);

   intel_miptree_release(&temp_mt);
   _mesa_unmap_teximage_pbo(ctx, packing);

   return ret;

err:
   _mesa_error(ctx, GL_OUT_OF_MEMORY, "intelTexSubImage");
   intel_miptree_release(&temp_mt);
   _mesa_unmap_teximage_pbo(ctx, packing);
   return false;
}

#ifdef __SSSE3__
static const uint8_t rgba8_permutation[16] =
   { 2,1,0,3, 6,5,4,7, 10,9,8,11, 14,13,12,15 };

typedef char v16 __attribute__((vector_size(16)));

/* NOTE: dst must be 16 byte aligned */
#define rgba8_copy_16(dst, src)                     \
   *(v16*)(dst) = __builtin_ia32_pshufb128(         \
       (v16) __builtin_ia32_loadups((float*)(src)), \
      *(v16*) rgba8_permutation                     \
   )
#endif

/**
 * Copy RGBA to BGRA - swap R and B.
 */
static inline void *
rgba8_copy(void *dst, const void *src, size_t bytes)
{
   uint8_t *d = dst;
   uint8_t const *s = src;

#ifdef __SSSE3__
   /* Fast copying for tile spans.
    *
    * As long as the destination texture is 16 aligned,
    * any 16 or 64 spans we get here should also be 16 aligned.
    */

   if (bytes == 16) {
      assert(!(((uintptr_t)dst) & 0xf));
      rgba8_copy_16(d+ 0, s+ 0);
      return dst;
   }

   if (bytes == 64) {
      assert(!(((uintptr_t)dst) & 0xf));
      rgba8_copy_16(d+ 0, s+ 0);
      rgba8_copy_16(d+16, s+16);
      rgba8_copy_16(d+32, s+32);
      rgba8_copy_16(d+48, s+48);
      return dst;
   }
#endif

   while (bytes >= 4) {
      d[0] = s[2];
      d[1] = s[1];
      d[2] = s[0];
      d[3] = s[3];
      d += 4;
      s += 4;
      bytes -= 4;
   }
   return dst;
}

/**
 * Copy texture data from linear to X tile layout.
 *
 * \copydoc tile_copy_fn
 */
static inline void
xtile_copy(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3,
           uint32_t y0, uint32_t y1,
           char *dst, const char *src,
           uint32_t src_pitch,
           uint32_t swizzle_bit,
           mem_copy_fn mem_copy)
{
   /* The copy destination offset for each range copied is the sum of
    * an X offset 'x0' or 'xo' and a Y offset 'yo.'
    */
   uint32_t xo, yo;

   src += y0 * src_pitch;

   for (yo = y0 * xtile_width; yo < y1 * xtile_width; yo += xtile_width) {
      /* Bits 9 and 10 of the copy destination offset control swizzling.
       * Only 'yo' contributes to those bits in the total offset,
       * so calculate 'swizzle' just once per row.
       * Move bits 9 and 10 three and four places respectively down
       * to bit 6 and xor them.
       */
      uint32_t swizzle = ((yo >> 3) ^ (yo >> 4)) & swizzle_bit;

      mem_copy(dst + ((x0 + yo) ^ swizzle), src + x0, x1 - x0);

      for (xo = x1; xo < x2; xo += xtile_span) {
         mem_copy(dst + ((xo + yo) ^ swizzle), src + xo, xtile_span);
      }

      mem_copy(dst + ((xo + yo) ^ swizzle), src + x2, x3 - x2);

      src += src_pitch;
   }
}

/**
 * Copy texture data from linear to Y tile layout.
 *
 * \copydoc tile_copy_fn
 */
static inline void
ytile_copy(
   uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3,
   uint32_t y0, uint32_t y1,
   char *dst, const char *src,
   uint32_t src_pitch,
   uint32_t swizzle_bit,
   mem_copy_fn mem_copy)
{
   /* Y tiles consist of columns that are 'ytile_span' wide (and the same height
    * as the tile).  Thus the destination offset for (x,y) is the sum of:
    *   (x % column_width)                    // position within column
    *   (x / column_width) * bytes_per_column // column number * bytes per column
    *   y * column_width
    *
    * The copy destination offset for each range copied is the sum of
    * an X offset 'xo0' or 'xo' and a Y offset 'yo.'
    */
   const uint32_t column_width = ytile_span;
   const uint32_t bytes_per_column = column_width * ytile_height;

   uint32_t xo0 = (x0 % ytile_span) + (x0 / ytile_span) * bytes_per_column;
   uint32_t xo1 = (x1 % ytile_span) + (x1 / ytile_span) * bytes_per_column;

   /* Bit 9 of the destination offset control swizzling.
    * Only the X offset contributes to bit 9 of the total offset,
    * so swizzle can be calculated in advance for these X positions.
    * Move bit 9 three places down to bit 6.
    */
   uint32_t swizzle0 = (xo0 >> 3) & swizzle_bit;
   uint32_t swizzle1 = (xo1 >> 3) & swizzle_bit;

   uint32_t x, yo;

   src += y0 * src_pitch;

   for (yo = y0 * column_width; yo < y1 * column_width; yo += column_width) {
      uint32_t xo = xo1;
      uint32_t swizzle = swizzle1;

      mem_copy(dst + ((xo0 + yo) ^ swizzle0), src + x0, x1 - x0);

      /* Step by spans/columns.  As it happens, the swizzle bit flips
       * at each step so we don't need to calculate it explicitly.
       */
      for (x = x1; x < x2; x += ytile_span) {
         mem_copy(dst + ((xo + yo) ^ swizzle), src + x, ytile_span);
         xo += bytes_per_column;
         swizzle ^= swizzle_bit;
      }

      mem_copy(dst + ((xo + yo) ^ swizzle), src + x2, x3 - x2);

      src += src_pitch;
   }
}

#ifdef __GNUC__
#define FLATTEN __attribute__((flatten))
#else
#define FLATTEN
#endif

/**
 * Copy texture data from linear to X tile layout, faster.
 *
 * Same as \ref xtile_copy but faster, because it passes constant parameters
 * for common cases, allowing the compiler to inline code optimized for those
 * cases.
 *
 * \copydoc tile_copy_fn
 */
static FLATTEN void
xtile_copy_faster(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3,
                  uint32_t y0, uint32_t y1,
                  char *dst, const char *src,
                  uint32_t src_pitch,
                  uint32_t swizzle_bit,
                  mem_copy_fn mem_copy)
{
   if (x0 == 0 && x3 == xtile_width && y0 == 0 && y1 == xtile_height) {
      if (mem_copy == memcpy)
         return xtile_copy(0, 0, xtile_width, xtile_width, 0, xtile_height,
                           dst, src, src_pitch, swizzle_bit, memcpy);
      else if (mem_copy == rgba8_copy)
         return xtile_copy(0, 0, xtile_width, xtile_width, 0, xtile_height,
                           dst, src, src_pitch, swizzle_bit, rgba8_copy);
   } else {
      if (mem_copy == memcpy)
         return xtile_copy(x0, x1, x2, x3, y0, y1,
                           dst, src, src_pitch, swizzle_bit, memcpy);
      else if (mem_copy == rgba8_copy)
         return xtile_copy(x0, x1, x2, x3, y0, y1,
                           dst, src, src_pitch, swizzle_bit, rgba8_copy);
   }
   xtile_copy(x0, x1, x2, x3, y0, y1,
              dst, src, src_pitch, swizzle_bit, mem_copy);
}

/**
 * Copy texture data from linear to Y tile layout, faster.
 *
 * Same as \ref ytile_copy but faster, because it passes constant parameters
 * for common cases, allowing the compiler to inline code optimized for those
 * cases.
 *
 * \copydoc tile_copy_fn
 */
static FLATTEN void
ytile_copy_faster(uint32_t x0, uint32_t x1, uint32_t x2, uint32_t x3,
                  uint32_t y0, uint32_t y1,
                  char *dst, const char *src,
                  uint32_t src_pitch,
                  uint32_t swizzle_bit,
                  mem_copy_fn mem_copy)
{
   if (x0 == 0 && x3 == ytile_width && y0 == 0 && y1 == ytile_height) {
      if (mem_copy == memcpy)
         return ytile_copy(0, 0, ytile_width, ytile_width, 0, ytile_height,
                           dst, src, src_pitch, swizzle_bit, memcpy);
      else if (mem_copy == rgba8_copy)
         return ytile_copy(0, 0, ytile_width, ytile_width, 0, ytile_height,
                           dst, src, src_pitch, swizzle_bit, rgba8_copy);
   } else {
      if (mem_copy == memcpy)
         return ytile_copy(x0, x1, x2, x3, y0, y1,
                           dst, src, src_pitch, swizzle_bit, memcpy);
      else if (mem_copy == rgba8_copy)
         return ytile_copy(x0, x1, x2, x3, y0, y1,
                           dst, src, src_pitch, swizzle_bit, rgba8_copy);
   }
   ytile_copy(x0, x1, x2, x3, y0, y1,
              dst, src, src_pitch, swizzle_bit, mem_copy);
}

/**
 * Copy from linear to tiled texture.
 *
 * Divide the region given by X range [xt1, xt2) and Y range [yt1, yt2) into
 * pieces that do not cross tile boundaries and copy each piece with a tile
 * copy function (\ref tile_copy_fn).
 * The X range is in bytes, i.e. pixels * bytes-per-pixel.
 * The Y range is in pixels (i.e. unitless).
 * 'dst' is the start of the texture and 'src' is the corresponding
 * address to copy from, though copying begins at (xt1, yt1).
 */
static void
linear_to_tiled(uint32_t xt1, uint32_t xt2,
                uint32_t yt1, uint32_t yt2,
                char *dst, const char *src,
                uint32_t dst_pitch, uint32_t src_pitch,
                bool has_swizzling,
                uint32_t tiling,
                mem_copy_fn mem_copy)
{
   tile_copy_fn tile_copy;
   uint32_t xt0, xt3;
   uint32_t yt0, yt3;
   uint32_t xt, yt;
   uint32_t tw, th, span;
   uint32_t swizzle_bit = has_swizzling ? 1<<6 : 0;

   if (tiling == I915_TILING_X) {
      tw = xtile_width;
      th = xtile_height;
      span = xtile_span;
      tile_copy = xtile_copy_faster;
   } else if (tiling == I915_TILING_Y) {
      tw = ytile_width;
      th = ytile_height;
      span = ytile_span;
      tile_copy = ytile_copy_faster;
   } else {
      assert(!"unsupported tiling");
      return;
   }

   /* Round out to tile boundaries. */
   xt0 = ALIGN_DOWN(xt1, tw);
   xt3 = ALIGN_UP  (xt2, tw);
   yt0 = ALIGN_DOWN(yt1, th);
   yt3 = ALIGN_UP  (yt2, th);

   /* Loop over all tiles to which we have something to copy.
    * 'xt' and 'yt' are the origin of the destination tile, whether copying
    * copying a full or partial tile.
    * tile_copy() copies one tile or partial tile.
    * Looping x inside y is the faster memory access pattern.
    */
   for (yt = yt0; yt < yt3; yt += th) {
      for (xt = xt0; xt < xt3; xt += tw) {
         /* The area to update is [x0,x3) x [y0,y1).
          * May not want the whole tile, hence the min and max.
          */
         uint32_t x0 = MAX2(xt1, xt);
         uint32_t y0 = MAX2(yt1, yt);
         uint32_t x3 = MIN2(xt2, xt + tw);
         uint32_t y1 = MIN2(yt2, yt + th);

         /* [x0,x3) is split into [x0,x1), [x1,x2), [x2,x3) such that
          * the middle interval is the longest span-aligned part.
          * The sub-ranges could be empty.
          */
         uint32_t x1, x2;
         x1 = ALIGN_UP(x0, span);
         if (x1 > x3)
            x1 = x2 = x3;
         else
            x2 = ALIGN_DOWN(x3, span);

         assert(x0 <= x1 && x1 <= x2 && x2 <= x3);
         assert(x1 - x0 < span && x3 - x2 < span);
         assert(x3 - x0 <= tw);
         assert((x2 - x1) % span == 0);

         /* Translate by (xt,yt) for single-tile copier. */
         tile_copy(x0-xt, x1-xt, x2-xt, x3-xt,
                   y0-yt, y1-yt,
                   dst + xt * th + yt * dst_pitch,
                   src + xt      + yt * src_pitch,
                   src_pitch,
                   swizzle_bit,
                   mem_copy);
      }
   }
}

/**
 * \brief A fast path for glTexImage and glTexSubImage.
 *
 * \param for_glTexImage Was this called from glTexImage or glTexSubImage?
 *
 * This fast path is taken when the texture format is BGRA, RGBA,
 * A or L and when the texture memory is X- or Y-tiled.  It uploads
 * the texture data by mapping the texture memory without a GTT fence, thus
 * acquiring a tiled view of the memory, and then copying sucessive
 * spans within each tile.
 *
 * This is a performance win over the conventional texture upload path because
 * it avoids the performance penalty of writing through the write-combine
 * buffer. In the conventional texture upload path,
 * texstore.c:store_texsubimage(), the texture memory is mapped through a GTT
 * fence, thus acquiring a linear view of the memory, then each row in the
 * image is memcpy'd. In this fast path, we replace each row's copy with
 * a sequence of copies over each linear span in tile.
 *
 * One use case is Google Chrome's paint rectangles.  Chrome (as
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
   struct brw_context *brw = brw_context(ctx);
   struct intel_texture_image *image = intel_texture_image(texImage);
   int src_pitch;

   /* The miptree's buffer. */
   drm_intel_bo *bo;

   int error = 0;

   uint32_t cpp;
   mem_copy_fn mem_copy = NULL;

   /* This fastpath is restricted to specific texture types:
    * a 2D BGRA, RGBA, L8 or A8 texture. It could be generalized to support
    * more types.
    *
    * FINISHME: The restrictions below on packing alignment and packing row
    * length are likely unneeded now because we calculate the source stride
    * with _mesa_image_row_stride. However, before removing the restrictions
    * we need tests.
    */
   if (!brw->has_llc ||
       type != GL_UNSIGNED_BYTE ||
       texImage->TexObject->Target != GL_TEXTURE_2D ||
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

   if ((texImage->TexFormat == MESA_FORMAT_L_UNORM8 && format == GL_LUMINANCE) ||
       (texImage->TexFormat == MESA_FORMAT_A_UNORM8 && format == GL_ALPHA)) {
      cpp = 1;
      mem_copy = memcpy;
   } else if ((texImage->TexFormat == MESA_FORMAT_B8G8R8A8_UNORM) ||
              (texImage->TexFormat == MESA_FORMAT_B8G8R8X8_UNORM)) {
      cpp = 4;
      if (format == GL_BGRA) {
         mem_copy = memcpy;
      } else if (format == GL_RGBA) {
         mem_copy = rgba8_copy;
      }
   }
   if (!mem_copy)
      return false;

   /* If this is a nontrivial texture view, let another path handle it instead. */
   if (texImage->TexObject->MinLayer)
      return false;

   if (for_glTexImage)
      ctx->Driver.AllocTextureImageBuffer(ctx, texImage);

   if (!image->mt ||
       (image->mt->tiling != I915_TILING_X &&
       image->mt->tiling != I915_TILING_Y)) {
      /* The algorithm is written only for X- or Y-tiled memory. */
      return false;
   }

   /* Since we are going to write raw data to the miptree, we need to resolve
    * any pending fast color clears before we start.
    */
   intel_miptree_resolve_color(brw, image->mt);

   bo = image->mt->bo;

   if (drm_intel_bo_references(brw->batch.bo, bo)) {
      perf_debug("Flushing before mapping a referenced bo.\n");
      intel_batchbuffer_flush(brw);
   }

   error = brw_bo_map(brw, bo, true /* write enable */, "miptree");
   if (error || bo->virtual == NULL) {
      DBG("%s: failed to map bo\n", __FUNCTION__);
      return false;
   }

   src_pitch = _mesa_image_row_stride(packing, width, format, type);

   /* We postponed printing this message until having committed to executing
    * the function.
    */
   DBG("%s: level=%d offset=(%d,%d) (w,h)=(%d,%d) format=0x%x type=0x%x "
       "mesa_format=0x%x tiling=%d "
       "packing=(alignment=%d row_length=%d skip_pixels=%d skip_rows=%d) "
       "for_glTexImage=%d\n",
       __FUNCTION__, texImage->Level, xoffset, yoffset, width, height,
       format, type, texImage->TexFormat, image->mt->tiling,
       packing->Alignment, packing->RowLength, packing->SkipPixels,
       packing->SkipRows, for_glTexImage);

   int level = texImage->Level + texImage->TexObject->MinLevel;

   /* Adjust x and y offset based on miplevel */
   xoffset += image->mt->level[level].level_x;
   yoffset += image->mt->level[level].level_y;

   linear_to_tiled(
      xoffset * cpp, (xoffset + width) * cpp,
      yoffset, yoffset + height,
      bo->virtual, pixels - yoffset * src_pitch - xoffset * cpp,
      image->mt->pitch, src_pitch,
      brw->has_swizzling,
      image->mt->tiling,
      mem_copy
   );

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
