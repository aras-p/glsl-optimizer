/**************************************************************************
 * 
 * Copyright 2010 VMware, Inc.  All Rights Reserved.
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
 * IN NO EVENT SHALL THE AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


#include "lp_tile_soa.h"
#include "lp_tile_image.h"


#define BYTES_PER_TILE (TILE_SIZE * TILE_SIZE * 4)


/**
 * Convert a tiled image into a linear image.
 * \param src_stride  source row stride in bytes (bytes per row of tiles)
 * \param dst_stride  dest row stride in bytes
 */
void
lp_tiled_to_linear(const uint8_t *src,
                   uint8_t *dst,
                   unsigned width, unsigned height,
                   enum pipe_format format,
                   unsigned src_stride,
                   unsigned dst_stride)
{
   const unsigned tiles_per_row = src_stride / BYTES_PER_TILE;
   unsigned i, j;

   for (j = 0; j < height; j += TILE_SIZE) {
      for (i = 0; i < width; i += TILE_SIZE) {
         unsigned tile_offset =
            ((j / TILE_SIZE) * tiles_per_row + i / TILE_SIZE);
         unsigned byte_offset = tile_offset * BYTES_PER_TILE;
         const uint8_t *src_tile = src + byte_offset;

         lp_tile_write_4ub(format,
                           src_tile,
                           dst,
                           dst_stride,
                           i, j, TILE_SIZE, TILE_SIZE);
      }
   }
}


/**
 * Convert a linear image into a tiled image.
 * \param src_stride  source row stride in bytes
 * \param dst_stride  dest row stride in bytes (bytes per row of tiles)
 */
void
lp_linear_to_tiled(const uint8_t *src,
                   uint8_t *dst,
                   unsigned width, unsigned height,
                   enum pipe_format format,
                   unsigned src_stride,
                   unsigned dst_stride)
{
   const unsigned tiles_per_row = dst_stride / BYTES_PER_TILE;
   unsigned i, j;

   for (j = 0; j < height; j += TILE_SIZE) {
      for (i = 0; i < width; i += TILE_SIZE) {
         unsigned tile_offset =
            ((j / TILE_SIZE) * tiles_per_row + i / TILE_SIZE);
         unsigned byte_offset = tile_offset * BYTES_PER_TILE;
         uint8_t *dst_tile = dst + byte_offset;

         lp_tile_read_4ub(format,
                          dst_tile,
                          src,
                          src_stride,
                          i, j, TILE_SIZE, TILE_SIZE);
      }
   }
}


/**
 * For testing only.
 */
void
test_tiled_linear_conversion(uint8_t *data,
                             enum pipe_format format,
                             unsigned width, unsigned height,
                             unsigned stride)
{
   /* size in tiles */
   unsigned wt = (width + TILE_SIZE - 1) / TILE_SIZE;
   unsigned ht = (height + TILE_SIZE - 1) / TILE_SIZE;

   uint8_t *tiled = malloc(wt * ht * TILE_SIZE * TILE_SIZE * 4);

   unsigned tiled_stride = wt * TILE_SIZE * TILE_SIZE * 4;

   lp_linear_to_tiled(data, tiled, width, height, format,
                      stride, tiled_stride);

   lp_tiled_to_linear(tiled, data, width, height, format,
                      tiled_stride, stride);

   free(tiled);
}

