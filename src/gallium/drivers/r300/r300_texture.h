/*
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

#ifndef R300_TEXTURE_H
#define R300_TEXTURE_H

#include "pipe/p_video_state.h"

#include "r300_reg.h"

struct r300_texture;

void r300_init_screen_texture_functions(struct pipe_screen* screen);

unsigned r300_texture_get_stride(struct r300_screen* screen,
                                 struct r300_texture* tex, unsigned level);

unsigned r300_texture_get_offset(struct r300_texture* tex, unsigned level,
                                 unsigned zslice, unsigned face);

/* Translate a pipe_format into a useful texture format for sampling.
 *
 * R300_EASY_TX_FORMAT swizzles the texture.
 * Note the signature of R300_EASY_TX_FORMAT:
 *   R300_EASY_TX_FORMAT(B, G, R, A, FORMAT);
 *
 * The FORMAT specifies how the texture sampler will treat the texture, and
 * makes available X, Y, Z, W, ZERO, and ONE for swizzling. */
static INLINE uint32_t r300_translate_texformat(enum pipe_format format)
{
    switch (format) {
        /* X8 */
        case PIPE_FORMAT_A8_UNORM:
            return R300_EASY_TX_FORMAT(ZERO, ZERO, ZERO, X, X8);
        case PIPE_FORMAT_I8_UNORM:
            return R300_EASY_TX_FORMAT(X, X, X, X, X8);
        case PIPE_FORMAT_L8_UNORM:
            return R300_EASY_TX_FORMAT(X, X, X, ONE, X8);
        /* X16 */
        case PIPE_FORMAT_R16_UNORM:
        case PIPE_FORMAT_Z16_UNORM:
            return R300_EASY_TX_FORMAT(X, X, X, X, X16);
        case PIPE_FORMAT_R16_SNORM:
            return R300_EASY_TX_FORMAT(X, X, X, X, X16) |
                R300_TX_FORMAT_SIGNED;
        /* Y8X8 */
        case PIPE_FORMAT_A8L8_UNORM:
            return R300_EASY_TX_FORMAT(X, X, X, Y, Y8X8);
        /* W8Z8Y8X8 */
        case PIPE_FORMAT_A8R8G8B8_UNORM:
            return R300_EASY_TX_FORMAT(X, Y, Z, W, W8Z8Y8X8);
        case PIPE_FORMAT_R8G8B8A8_UNORM:
            return R300_EASY_TX_FORMAT(Y, Z, W, X, W8Z8Y8X8);
        case PIPE_FORMAT_X8R8G8B8_UNORM:
            return R300_EASY_TX_FORMAT(X, Y, Z, ONE, W8Z8Y8X8);
        case PIPE_FORMAT_R8G8B8X8_UNORM:
            return R300_EASY_TX_FORMAT(Y, Z, ONE, X, W8Z8Y8X8);
        case PIPE_FORMAT_A8R8G8B8_SRGB:
            return R300_EASY_TX_FORMAT(X, Y, Z, W, W8Z8Y8X8) |
                R300_TX_FORMAT_GAMMA;
        case PIPE_FORMAT_R8G8B8A8_SRGB:
            return R300_EASY_TX_FORMAT(Y, Z, W, X, W8Z8Y8X8) |
                R300_TX_FORMAT_GAMMA;
        /* DXT1 */
        case PIPE_FORMAT_DXT1_RGB:
            return R300_EASY_TX_FORMAT(X, Y, Z, ONE, DXT1);
        case PIPE_FORMAT_DXT1_RGBA:
            return R300_EASY_TX_FORMAT(X, Y, Z, W, DXT1);
        /* DXT3 */
        case PIPE_FORMAT_DXT3_RGBA:
            return R300_EASY_TX_FORMAT(X, Y, Z, W, DXT3);
        /* DXT5 */
        case PIPE_FORMAT_DXT5_RGBA:
            return R300_EASY_TX_FORMAT(Y, Z, W, X, DXT5);
        /* YVYU422 */
        case PIPE_FORMAT_YCBCR:
            return R300_EASY_TX_FORMAT(X, Y, Z, ONE, YVYU422) |
                R300_TX_FORMAT_YUV_TO_RGB;
        /* W24_FP */
        case PIPE_FORMAT_Z24S8_UNORM:
        case PIPE_FORMAT_Z24X8_UNORM:
            return R300_EASY_TX_FORMAT(X, X, X, X, W24_FP);

        default:
            debug_printf("r300: Implementation error: "
                "Got unsupported texture format %s in %s\n",
                pf_name(format), __FUNCTION__);
            assert(0);
            break;
    }
    return 0;
}

struct r300_video_surface
{
    struct pipe_video_surface   base;
    struct pipe_texture         *tex;
};

static INLINE struct r300_video_surface *
r300_video_surface(struct pipe_video_surface *pvs)
{
    return (struct r300_video_surface *)pvs;
}

#ifndef R300_WINSYS_H

boolean r300_get_texture_buffer(struct pipe_screen* screen,
                                struct pipe_texture* texture,
                                struct pipe_buffer** buffer,
                                unsigned* stride);

#endif /* R300_WINSYS_H */

#endif /* R300_TEXTURE_H */
