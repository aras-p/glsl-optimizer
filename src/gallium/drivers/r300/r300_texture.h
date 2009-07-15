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

#include "pipe/p_screen.h"

#include "util/u_math.h"

#include "r300_context.h"
#include "r300_reg.h"

void r300_init_screen_texture_functions(struct pipe_screen* screen);

/* Note the signature of R300_EASY_TX_FORMAT(A, R, G, B, FORMAT)... */
static INLINE uint32_t r300_translate_texformat(enum pipe_format format)
{
    switch (format) {
        /* X8 */
        case PIPE_FORMAT_I8_UNORM:
            return R300_EASY_TX_FORMAT(X, X, X, X, X8);
        /* W8Z8Y8X8 */
        case PIPE_FORMAT_A8R8G8B8_UNORM:
            return R300_EASY_TX_FORMAT(X, Y, Z, W, W8Z8Y8X8);
        case PIPE_FORMAT_R8G8B8A8_UNORM:
            return R300_EASY_TX_FORMAT(Y, Z, W, X, W8Z8Y8X8);
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

#ifndef R300_WINSYS_H

boolean r300_get_texture_buffer(struct pipe_texture* texture,
                                struct pipe_buffer** buffer,
                                unsigned* stride);

#endif /* R300_WINSYS_H */

#endif /* R300_TEXTURE_H */
