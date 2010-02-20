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
#include "util/u_format.h"

#include "r300_reg.h"

struct r300_texture;

void r300_init_screen_texture_functions(struct pipe_screen* screen);

unsigned r300_texture_get_stride(struct r300_screen* screen,
                                 struct r300_texture* tex, unsigned level);

unsigned r300_texture_get_offset(struct r300_texture* tex, unsigned level,
                                 unsigned zslice, unsigned face);

void r300_texture_reinterpret_format(struct pipe_screen *screen,
                                     struct pipe_texture *tex,
                                     enum pipe_format new_format);

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
    uint32_t result = 0;
    const struct util_format_description *desc;
    unsigned components = 0, i;
    boolean uniform = TRUE;
    const uint32_t swizzle_shift[4] = {
        R300_TX_FORMAT_R_SHIFT,
        R300_TX_FORMAT_G_SHIFT,
        R300_TX_FORMAT_B_SHIFT,
        R300_TX_FORMAT_A_SHIFT
    };
    const uint32_t sign_bit[4] = {
        R300_TX_FORMAT_SIGNED_X,
        R300_TX_FORMAT_SIGNED_Y,
        R300_TX_FORMAT_SIGNED_Z,
        R300_TX_FORMAT_SIGNED_W,
    };

    desc = util_format_description(format);

    /* Colorspace (return non-RGB formats directly). */
    switch (desc->colorspace) {
        /* Depth stencil formats. */
        case UTIL_FORMAT_COLORSPACE_ZS:
            switch (format) {
                case PIPE_FORMAT_Z16_UNORM:
                    return R300_EASY_TX_FORMAT(X, X, X, X, X16);
                case PIPE_FORMAT_Z24X8_UNORM:
                case PIPE_FORMAT_Z24S8_UNORM:
                    return R300_EASY_TX_FORMAT(X, X, X, X, W24_FP);
                default:
                    return ~0; /* Unsupported. */
            }

        /* YUV formats. */
        case UTIL_FORMAT_COLORSPACE_YUV:
            result |= R300_TX_FORMAT_YUV_TO_RGB;

            switch (format) {
                case PIPE_FORMAT_YCBCR:
                    return R300_EASY_TX_FORMAT(X, Y, Z, ONE, YVYU422) | result;
                case PIPE_FORMAT_YCBCR_REV:
                    return R300_EASY_TX_FORMAT(X, Y, Z, ONE, VYUY422) | result;
                default:
                    return ~0; /* Unsupported/unknown. */
            }

        /* Add gamma correction. */
        case UTIL_FORMAT_COLORSPACE_SRGB:
            result |= R300_TX_FORMAT_GAMMA;
            break;

        default:;
    }

    /* Add swizzle. */
    for (i = 0; i < 4; i++) {
        switch (desc->swizzle[i]) {
            case UTIL_FORMAT_SWIZZLE_X:
            case UTIL_FORMAT_SWIZZLE_NONE:
                result |= R300_TX_FORMAT_X << swizzle_shift[i];
                break;
            case UTIL_FORMAT_SWIZZLE_Y:
                result |= R300_TX_FORMAT_Y << swizzle_shift[i];
                break;
            case UTIL_FORMAT_SWIZZLE_Z:
                result |= R300_TX_FORMAT_Z << swizzle_shift[i];
                break;
            case UTIL_FORMAT_SWIZZLE_W:
                result |= R300_TX_FORMAT_W << swizzle_shift[i];
                break;
            case UTIL_FORMAT_SWIZZLE_0:
                result |= R300_TX_FORMAT_ZERO << swizzle_shift[i];
                break;
            case UTIL_FORMAT_SWIZZLE_1:
                result |= R300_TX_FORMAT_ONE << swizzle_shift[i];
                break;
            default:
                return ~0; /* Unsupported. */
        }
    }

    /* Compressed formats. */
    if (desc->layout == UTIL_FORMAT_LAYOUT_DXT) {
        switch (format) {
            case PIPE_FORMAT_DXT1_RGB:
            case PIPE_FORMAT_DXT1_RGBA:
            case PIPE_FORMAT_DXT1_SRGB:
            case PIPE_FORMAT_DXT1_SRGBA:
                return R300_TX_FORMAT_DXT1 | result;
            case PIPE_FORMAT_DXT3_RGBA:
            case PIPE_FORMAT_DXT3_SRGBA:
                return R300_TX_FORMAT_DXT3 | result;
            case PIPE_FORMAT_DXT5_RGBA:
            case PIPE_FORMAT_DXT5_SRGBA:
                return R300_TX_FORMAT_DXT5 | result;
            default:
                return ~0; /* Unsupported/unknown. */
        }
    }

    /* Get the number of components. */
    for (i = 0; i < 4; i++) {
        if (desc->channel[i].type != UTIL_FORMAT_TYPE_VOID) {
            ++components;
        }
    }

    /* Add sign. */
    for (i = 0; i < components; i++) {
        if (desc->channel[i].type == UTIL_FORMAT_TYPE_SIGNED) {
            result |= sign_bit[i];
        }
    }

    /* See whether the components are of the same size. */
    for (i = 1; i < components; i++) {
        uniform = uniform && desc->channel[0].size == desc->channel[i].size;
    }

    /* Non-uniform formats. */
    if (!uniform) {
        switch (components) {
            case 3:
                if (desc->channel[0].size == 5 &&
                    desc->channel[1].size == 6 &&
                    desc->channel[2].size == 5) {
                    return R300_TX_FORMAT_Z5Y6X5 | result;
                }
                if (desc->channel[0].size == 5 &&
                    desc->channel[1].size == 5 &&
                    desc->channel[2].size == 6) {
                    return R300_TX_FORMAT_Z6Y5X5 | result;
                }
                return ~0; /* Unsupported/unknown. */

            case 4:
                if (desc->channel[0].size == 5 &&
                    desc->channel[1].size == 5 &&
                    desc->channel[2].size == 5 &&
                    desc->channel[3].size == 1) {
                    return R300_TX_FORMAT_W1Z5Y5X5 | result;
                }
                if (desc->channel[0].size == 10 &&
                    desc->channel[1].size == 10 &&
                    desc->channel[2].size == 10 &&
                    desc->channel[3].size == 2) {
                    return R300_TX_FORMAT_W2Z10Y10X10 | result;
                }
        }
        return ~0; /* Unsupported/unknown. */
    }

    /* And finally, uniform formats. */
    switch (desc->channel[0].type) {
        case UTIL_FORMAT_TYPE_UNSIGNED:
        case UTIL_FORMAT_TYPE_SIGNED:
            if (!desc->channel[0].normalized &&
                desc->colorspace != UTIL_FORMAT_COLORSPACE_SRGB) {
                return ~0;
            }

            switch (desc->channel[0].size) {
                case 4:
                    switch (components) {
                        case 2:
                            return R300_TX_FORMAT_Y4X4 | result;
                        case 4:
                            return R300_TX_FORMAT_W4Z4Y4X4 | result;
                    }
                    return ~0;

                case 8:
                    switch (components) {
                        case 1:
                            return R300_TX_FORMAT_X8 | result;
                        case 2:
                            return R300_TX_FORMAT_Y8X8 | result;
                        case 4:
                            return R300_TX_FORMAT_W8Z8Y8X8 | result;
                    }
                    return ~0;

                case 16:
                    switch (components) {
                        case 1:
                            return R300_TX_FORMAT_X16 | result;
                        case 2:
                            return R300_TX_FORMAT_Y16X16 | result;
                        case 4:
                            return R300_TX_FORMAT_W16Z16Y16X16 | result;
                    }
            }
            return ~0;

/* XXX Enable float textures here. */
#if 0
        case UTIL_FORMAT_TYPE_FLOAT:
            switch (desc->channel[0].size) {
                case 16:
                    switch (components) {
                        case 1:
                            return R300_TX_FORMAT_16F | result;
                        case 2:
                            return R300_TX_FORMAT_16F_16F | result;
                        case 4:
                            return R300_TX_FORMAT_16F_16F_16F_16F | result;
                    }
                    return ~0;

                case 32:
                    switch (components) {
                        case 1:
                            return R300_TX_FORMAT_32F | result;
                        case 2:
                            return R300_TX_FORMAT_32F_32F | result;
                        case 4:
                            return R300_TX_FORMAT_32F_32F_32F_32F | result;
                    }
            }
#endif
    }

    return ~0; /* Unsupported/unknown. */
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
