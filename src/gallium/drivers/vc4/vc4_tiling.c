/*
 * Copyright © 2014 Broadcom
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/** @file vc4_tiling.c
 *
 * Handles information about the VC4 tiling formats, and loading and storing
 * from them.
 *
 * Texture mipmap levels on VC4 are (with the exception of 32-bit RGBA raster
 * textures for scanout) stored as groups of microtiles.  If the texture is at
 * least 4x4 microtiles (utiles), then those microtiles are arranged in a sort
 * of Hilbert-fractal-ish layout (T), otherwise the microtiles are in raster
 * order (LT).
 *
 * Specifically, the T format has:
 *
 * - 64b utiles of pixels in a raster-order grid according to cpp.  It's 4x4
 *   pixels at 32 bit depth.
 *
 * - 1k subtiles made of a 4x4 raster-order grid of 64b utiles (so usually
 *   16x16 pixels).
 *
 * - 4k tiles made of a 2x2 grid of 1k subtiles (so usually 32x32 pixels).  On
 *   even 4k tile rows, they're arranged as (BL, TL, TR, BR), and on odd rows
 *   they're (TR, BR, BL, TL), where bottom left is start of memory.
 *
 * - an image made of 4k tiles in rows either left-to-right (even rows of 4k
 *   tiles) or right-to-left (odd rows of 4k tiles).
 */

#include "vc4_screen.h"
#include "vc4_context.h"
#include "vc4_tiling.h"

/** Return the width in pixels of a 64-byte microtile. */
uint32_t
vc4_utile_width(int cpp)
{
        switch (cpp) {
        case 1:
        case 2:
                return 8;
        case 4:
                return 4;
        case 8:
                return 2;
        default:
                fprintf(stderr, "unknown cpp: %d\n", cpp);
                abort();
        }
}

/** Return the height in pixels of a 64-byte microtile. */
uint32_t
vc4_utile_height(int cpp)
{
        switch (cpp) {
        case 1:
                return 8;
        case 2:
        case 4:
        case 8:
                return 4;
        default:
                fprintf(stderr, "unknown cpp: %d\n", cpp);
                abort();
        }
}

/**
 * The texture unit decides what tiling format a particular miplevel is using
 * this function, so we lay out our miptrees accordingly.
 */
bool
vc4_size_is_lt(uint32_t width, uint32_t height, int cpp)
{
        return (width <= 4 * vc4_utile_width(cpp) ||
                height <= 4 * vc4_utile_height(cpp));
}

void
vc4_load_utile(void *dst, void *src, uint32_t dst_stride, uint32_t cpp)
{
        uint32_t utile_h = vc4_utile_height(cpp);
        uint32_t row_size = 64 / utile_h;

        for (int y = 0; y < utile_h; y++) {
                memcpy(dst, src, row_size);
                dst += dst_stride;
                src += row_size;
        }
}

void
vc4_store_utile(void *dst, void *src, uint32_t src_stride, uint32_t cpp)
{
        uint32_t utile_h = vc4_utile_height(cpp);
        uint32_t row_size = 64 / utile_h;

        for (int y = 0; y < utile_h; y++) {
                memcpy(dst, src, row_size);
                dst += row_size;
                src += src_stride;
        }
}

static void
check_box_utile_alignment(const struct pipe_box *box, int cpp)
{
        uint32_t utile_w = vc4_utile_width(cpp);
        uint32_t utile_h = vc4_utile_height(cpp);

        assert(!(box->x & (utile_w - 1)));
        assert(!(box->y & (utile_h - 1)));
        assert(!(box->width & (utile_w - 1)));
        assert(!(box->height & (utile_h - 1)));
}

static void
vc4_load_lt_image(void *dst, uint32_t dst_stride,
                  void *src, uint32_t src_stride,
                  int cpp, const struct pipe_box *box)
{
        uint32_t utile_w = vc4_utile_width(cpp);
        uint32_t utile_h = vc4_utile_height(cpp);
        uint32_t xstart = box->x / utile_w;
        uint32_t ystart = box->y / utile_h;

        for (uint32_t y = 0; y < box->height; y += utile_h) {
                for (int x = 0; x < box->width; x += utile_w) {
                        vc4_load_utile(dst + (dst_stride * y +
                                              x * cpp),
                                       src + ((ystart + y) * src_stride +
                                              (xstart + x) * 64 / utile_w),
                                       dst_stride, cpp);
                }
        }
}

static void
vc4_store_lt_image(void *dst, uint32_t dst_stride,
                   void *src, uint32_t src_stride,
                   int cpp, const struct pipe_box *box)
{
        uint32_t utile_w = vc4_utile_width(cpp);
        uint32_t utile_h = vc4_utile_height(cpp);
        uint32_t xstart = box->x / utile_w;
        uint32_t ystart = box->y / utile_h;

        for (uint32_t y = 0; y < box->height; y += utile_h) {
                for (int x = 0; x < box->width; x += utile_w) {
                        vc4_store_utile(dst + ((ystart + y) * dst_stride +
                                               (xstart + x) * 64 / utile_w),
                                        src + (src_stride * y +
                                               x * cpp),
                                        src_stride, cpp);
                }
        }
}

/**
 * Takes a utile x and y (and the number of utiles of width of the image) and
 * returns the offset to the utile within a VC4_TILING_FORMAT_TF image.
 */
static uint32_t
t_utile_address(uint32_t utile_x, uint32_t utile_y,
                uint32_t utile_stride)
{
        /* T images have to be aligned to 8 utiles (4x4 subtiles, which are
         * 2x2 in a 4k tile).
         */
        assert(!(utile_stride & 7));
        uint32_t tile_stride = utile_stride >> 3;
        /* 4k tile offsets. */
        uint32_t tile_x = utile_x >> 3;
        uint32_t tile_y = utile_y >> 3;
        bool odd_tile_y = tile_y & 1;

        /* Odd lines of 4k tiles go right-to-left. */
        if (odd_tile_y)
                tile_x = tile_stride - tile_x - 1;

        uint32_t tile_offset = 4096 * (tile_y * tile_stride + tile_x);

        uint32_t stile_x = (utile_x >> 2) & 1;
        uint32_t stile_y = (utile_y >> 2) & 1;
        uint32_t stile_index = (stile_y << 1) + stile_x;
        static const uint32_t odd_stile_map[4] = {2, 1, 3, 0};
        static const uint32_t even_stile_map[4] = {0, 3, 1, 2};

        uint32_t stile_offset = 1024 * (odd_tile_y ?
                                        odd_stile_map[stile_index] :
                                        even_stile_map[stile_index]);

        uint32_t utile_offset = 64 * ((utile_y & 3) * 4 + (utile_x & 3));

#if 0
        fprintf(stderr, "utile %d,%d -> %d + %d + %d (stride %d,%d) = %d\n",
                utile_x, utile_y,
                tile_offset, stile_offset, utile_offset,
                utile_stride, tile_stride,
                tile_offset + stile_offset + utile_offset);
#endif

        return tile_offset + stile_offset + utile_offset;
}

static void
vc4_load_t_image(void *dst, uint32_t dst_stride,
                 void *src, uint32_t src_stride,
                 int cpp, const struct pipe_box *box)
{
        uint32_t utile_w = vc4_utile_width(cpp);
        uint32_t utile_h = vc4_utile_height(cpp);
        uint32_t utile_stride = src_stride / cpp / utile_w;
        uint32_t xstart = box->x / utile_w;
        uint32_t ystart = box->y / utile_h;

        for (uint32_t y = 0; y < box->height / utile_h; y++) {
                for (int x = 0; x < box->width / utile_w; x++) {
                        vc4_load_utile(dst + (y * utile_h * dst_stride +
                                              x * utile_w * cpp),
                                       src + t_utile_address(xstart + x,
                                                             ystart + y,
                                                             utile_stride),
                                       dst_stride, cpp);
                }
        }
}

static void
vc4_store_t_image(void *dst, uint32_t dst_stride,
                  void *src, uint32_t src_stride,
                  int cpp, const struct pipe_box *box)
{
        uint32_t utile_w = vc4_utile_width(cpp);
        uint32_t utile_h = vc4_utile_height(cpp);
        uint32_t utile_stride = dst_stride / cpp / utile_w;
        uint32_t xstart = box->x / utile_w;
        uint32_t ystart = box->y / utile_h;

        for (uint32_t y = 0; y < box->height / utile_h; y++) {
                for (int x = 0; x < box->width / utile_w; x++) {
                        vc4_store_utile(dst + t_utile_address(xstart + x,
                                                              ystart + y,
                                                              utile_stride),
                                        src + (y * utile_h * src_stride +
                                               x * utile_w * cpp),
                                        src_stride, cpp);
                }
        }
}

/**
 * Loads pixel data from the start (microtile-aligned) box in @src to the
 * start of @dst according to the given tiling format.
 */
void
vc4_load_tiled_image(void *dst, uint32_t dst_stride,
                     void *src, uint32_t src_stride,
                     uint8_t tiling_format, int cpp,
                     const struct pipe_box *box)
{
        check_box_utile_alignment(box, cpp);

        if (tiling_format == VC4_TILING_FORMAT_LT) {
                vc4_load_lt_image(dst, dst_stride,
                                  src, src_stride,
                                  cpp, box);
        } else {
                assert(tiling_format == VC4_TILING_FORMAT_T);
                vc4_load_t_image(dst, dst_stride,
                                 src, src_stride,
                                 cpp, box);
        }
}

/**
 * Stores pixel data from the start of @src into a (microtile-aligned) box in
 * @dst according to the given tiling format.
 */
void
vc4_store_tiled_image(void *dst, uint32_t dst_stride,
                      void *src, uint32_t src_stride,
                      uint8_t tiling_format, int cpp,
                      const struct pipe_box *box)
{
        check_box_utile_alignment(box, cpp);

        if (tiling_format == VC4_TILING_FORMAT_LT) {
                vc4_store_lt_image(dst, dst_stride,
                                   src, src_stride,
                                   cpp, box);
        } else {
                assert(tiling_format == VC4_TILING_FORMAT_T);
                vc4_store_t_image(dst, dst_stride,
                                  src, src_stride,
                                  cpp, box);
        }
}

