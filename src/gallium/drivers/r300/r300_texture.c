/*
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
 * Copyright 2010 Marek Olšák <maraeo@gmail.com>
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

#include "pipe/p_screen.h"

#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"

#include "r300_context.h"
#include "r300_reg.h"
#include "r300_texture.h"
#include "r300_transfer.h"
#include "r300_screen.h"
#include "r300_winsys.h"

#define TILE_WIDTH 0
#define TILE_HEIGHT 1

static const unsigned microblock_table[5][3][2] = {
    /*linear  tiled   square-tiled */
    {{32, 1}, {8, 4}, {0, 0}}, /*   8 bits per pixel */
    {{16, 1}, {8, 2}, {4, 4}}, /*  16 bits per pixel */
    {{ 8, 1}, {4, 2}, {0, 0}}, /*  32 bits per pixel */
    {{ 4, 1}, {0, 0}, {2, 2}}, /*  64 bits per pixel */
    {{ 2, 1}, {0, 0}, {0, 0}}  /* 128 bits per pixel */
};

/* Return true for non-compressed and non-YUV formats. */
static boolean r300_format_is_plain(enum pipe_format format)
{
    const struct util_format_description *desc = util_format_description(format);

    if (!format) {
        return FALSE;
    }

    return desc->layout == UTIL_FORMAT_LAYOUT_PLAIN;
}

/* Translate a pipe_format into a useful texture format for sampling.
 *
 * Some special formats are translated directly using R300_EASY_TX_FORMAT,
 * but the majority of them is translated in a generic way, automatically
 * supporting all the formats hw can support.
 *
 * R300_EASY_TX_FORMAT swizzles the texture.
 * Note the signature of R300_EASY_TX_FORMAT:
 *   R300_EASY_TX_FORMAT(B, G, R, A, FORMAT);
 *
 * The FORMAT specifies how the texture sampler will treat the texture, and
 * makes available X, Y, Z, W, ZERO, and ONE for swizzling. */
uint32_t r300_translate_texformat(enum pipe_format format,
                                  const unsigned char *swizzle_view)
{
    uint32_t result = 0;
    const struct util_format_description *desc;
    unsigned i;
    boolean uniform = TRUE;
    const uint32_t swizzle_shift[4] = {
        R300_TX_FORMAT_R_SHIFT,
        R300_TX_FORMAT_G_SHIFT,
        R300_TX_FORMAT_B_SHIFT,
        R300_TX_FORMAT_A_SHIFT
    };
    const uint32_t swizzle_bit[4] = {
        R300_TX_FORMAT_X,
        R300_TX_FORMAT_Y,
        R300_TX_FORMAT_Z,
        R300_TX_FORMAT_W
    };
    const uint32_t sign_bit[4] = {
        R300_TX_FORMAT_SIGNED_X,
        R300_TX_FORMAT_SIGNED_Y,
        R300_TX_FORMAT_SIGNED_Z,
        R300_TX_FORMAT_SIGNED_W,
    };
    unsigned char swizzle[4];

    desc = util_format_description(format);

    /* Colorspace (return non-RGB formats directly). */
    switch (desc->colorspace) {
        /* Depth stencil formats. */
        case UTIL_FORMAT_COLORSPACE_ZS:
            switch (format) {
                case PIPE_FORMAT_Z16_UNORM:
                    return R300_EASY_TX_FORMAT(X, X, X, X, X16);
                case PIPE_FORMAT_X8Z24_UNORM:
                case PIPE_FORMAT_S8_USCALED_Z24_UNORM:
                    return R300_EASY_TX_FORMAT(X, X, X, X, W24_FP);
                default:
                    return ~0; /* Unsupported. */
            }

        /* YUV formats. */
        case UTIL_FORMAT_COLORSPACE_YUV:
            result |= R300_TX_FORMAT_YUV_TO_RGB;

            switch (format) {
                case PIPE_FORMAT_UYVY:
                    return R300_EASY_TX_FORMAT(X, Y, Z, ONE, YVYU422) | result;
                case PIPE_FORMAT_YUYV:
                    return R300_EASY_TX_FORMAT(X, Y, Z, ONE, VYUY422) | result;
                default:
                    return ~0; /* Unsupported/unknown. */
            }

        /* Add gamma correction. */
        case UTIL_FORMAT_COLORSPACE_SRGB:
            result |= R300_TX_FORMAT_GAMMA;
            break;

        default:
            switch (format) {
                /* Same as YUV but without the YUR->RGB conversion. */
                case PIPE_FORMAT_R8G8_B8G8_UNORM:
                    return R300_EASY_TX_FORMAT(X, Y, Z, ONE, YVYU422) | result;
                case PIPE_FORMAT_G8R8_G8B8_UNORM:
                    return R300_EASY_TX_FORMAT(X, Y, Z, ONE, VYUY422) | result;
                default:;
            }
    }

    /* Get swizzle. */
    if (swizzle_view) {
        /* Compose two sets of swizzles. */
        for (i = 0; i < 4; i++) {
            swizzle[i] = swizzle_view[i] <= UTIL_FORMAT_SWIZZLE_W ?
                         desc->swizzle[swizzle_view[i]] : swizzle_view[i];
        }
    } else {
        memcpy(swizzle, desc->swizzle, sizeof(swizzle));
    }

    /* Add swizzle. */
    for (i = 0; i < 4; i++) {
        switch (swizzle[i]) {
            case UTIL_FORMAT_SWIZZLE_X:
            case UTIL_FORMAT_SWIZZLE_NONE:
                result |= swizzle_bit[0] << swizzle_shift[i];
                break;
            case UTIL_FORMAT_SWIZZLE_Y:
                result |= swizzle_bit[1] << swizzle_shift[i];
                break;
            case UTIL_FORMAT_SWIZZLE_Z:
                result |= swizzle_bit[2] << swizzle_shift[i];
                break;
            case UTIL_FORMAT_SWIZZLE_W:
                result |= swizzle_bit[3] << swizzle_shift[i];
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

    /* S3TC formats. */
    if (desc->layout == UTIL_FORMAT_LAYOUT_S3TC) {
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

    /* Add sign. */
    for (i = 0; i < desc->nr_channels; i++) {
        if (desc->channel[i].type == UTIL_FORMAT_TYPE_SIGNED) {
            result |= sign_bit[i];
        }
    }

    /* This is truly a special format.
     * It stores R8G8 and B is computed using sqrt(1 - R^2 - G^2)
     * in the sampler unit. Also known as D3DFMT_CxV8U8. */
    if (format == PIPE_FORMAT_R8G8Bx_SNORM) {
        return R300_TX_FORMAT_CxV8U8 | result;
    }

    /* RGTC formats. */
    if (desc->layout == UTIL_FORMAT_LAYOUT_RGTC) {
        switch (format) {
            case PIPE_FORMAT_RGTC1_UNORM:
            case PIPE_FORMAT_RGTC1_SNORM:
                return R500_TX_FORMAT_ATI1N | result;
            case PIPE_FORMAT_RGTC2_UNORM:
            case PIPE_FORMAT_RGTC2_SNORM:
                return R400_TX_FORMAT_ATI2N | result;
            default:
                return ~0; /* Unsupported/unknown. */
        }
    }

    /* See whether the components are of the same size. */
    for (i = 1; i < desc->nr_channels; i++) {
        uniform = uniform && desc->channel[0].size == desc->channel[i].size;
    }

    /* Non-uniform formats. */
    if (!uniform) {
        switch (desc->nr_channels) {
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
                    switch (desc->nr_channels) {
                        case 2:
                            return R300_TX_FORMAT_Y4X4 | result;
                        case 4:
                            return R300_TX_FORMAT_W4Z4Y4X4 | result;
                    }
                    return ~0;

                case 8:
                    switch (desc->nr_channels) {
                        case 1:
                            return R300_TX_FORMAT_X8 | result;
                        case 2:
                            return R300_TX_FORMAT_Y8X8 | result;
                        case 4:
                            return R300_TX_FORMAT_W8Z8Y8X8 | result;
                    }
                    return ~0;

                case 16:
                    switch (desc->nr_channels) {
                        case 1:
                            return R300_TX_FORMAT_X16 | result;
                        case 2:
                            return R300_TX_FORMAT_Y16X16 | result;
                        case 4:
                            return R300_TX_FORMAT_W16Z16Y16X16 | result;
                    }
            }
            return ~0;

        case UTIL_FORMAT_TYPE_FLOAT:
            switch (desc->channel[0].size) {
                case 16:
                    switch (desc->nr_channels) {
                        case 1:
                            return R300_TX_FORMAT_16F | result;
                        case 2:
                            return R300_TX_FORMAT_16F_16F | result;
                        case 4:
                            return R300_TX_FORMAT_16F_16F_16F_16F | result;
                    }
                    return ~0;

                case 32:
                    switch (desc->nr_channels) {
                        case 1:
                            return R300_TX_FORMAT_32F | result;
                        case 2:
                            return R300_TX_FORMAT_32F_32F | result;
                        case 4:
                            return R300_TX_FORMAT_32F_32F_32F_32F | result;
                    }
            }
    }

    return ~0; /* Unsupported/unknown. */
}

uint32_t r500_tx_format_msb_bit(enum pipe_format format)
{
    switch (format) {
        case PIPE_FORMAT_RGTC1_UNORM:
        case PIPE_FORMAT_RGTC1_SNORM:
            return R500_TXFORMAT_MSB;
        default:
            return 0;
    }
}

/* Buffer formats. */

/* Colorbuffer formats. This is the unswizzled format of the RB3D block's
 * output. For the swizzling of the targets, check the shader's format. */
static uint32_t r300_translate_colorformat(enum pipe_format format)
{
    switch (format) {
        /* 8-bit buffers. */
        case PIPE_FORMAT_A8_UNORM:
        case PIPE_FORMAT_I8_UNORM:
        case PIPE_FORMAT_L8_UNORM:
        case PIPE_FORMAT_R8_UNORM:
        case PIPE_FORMAT_R8_SNORM:
            return R300_COLOR_FORMAT_I8;

        /* 16-bit buffers. */
        case PIPE_FORMAT_B5G6R5_UNORM:
            return R300_COLOR_FORMAT_RGB565;

        case PIPE_FORMAT_B5G5R5A1_UNORM:
        case PIPE_FORMAT_B5G5R5X1_UNORM:
            return R300_COLOR_FORMAT_ARGB1555;

        case PIPE_FORMAT_B4G4R4A4_UNORM:
        case PIPE_FORMAT_B4G4R4X4_UNORM:
            return R300_COLOR_FORMAT_ARGB4444;

        /* 32-bit buffers. */
        case PIPE_FORMAT_B8G8R8A8_UNORM:
        case PIPE_FORMAT_B8G8R8X8_UNORM:
        case PIPE_FORMAT_A8R8G8B8_UNORM:
        case PIPE_FORMAT_X8R8G8B8_UNORM:
        case PIPE_FORMAT_A8B8G8R8_UNORM:
        case PIPE_FORMAT_R8G8B8A8_SNORM:
        case PIPE_FORMAT_X8B8G8R8_UNORM:
        case PIPE_FORMAT_R8G8B8X8_UNORM:
        case PIPE_FORMAT_R8SG8SB8UX8U_NORM:
            return R300_COLOR_FORMAT_ARGB8888;

        case PIPE_FORMAT_R10G10B10A2_UNORM:
        case PIPE_FORMAT_R10G10B10X2_SNORM:
        case PIPE_FORMAT_B10G10R10A2_UNORM:
        case PIPE_FORMAT_R10SG10SB10SA2U_NORM:
            return R500_COLOR_FORMAT_ARGB2101010;  /* R5xx-only? */

        /* 64-bit buffers. */
        case PIPE_FORMAT_R16G16B16A16_UNORM:
        case PIPE_FORMAT_R16G16B16A16_SNORM:
        case PIPE_FORMAT_R16G16B16A16_FLOAT:
            return R300_COLOR_FORMAT_ARGB16161616;

        /* 128-bit buffers. */
        case PIPE_FORMAT_R32G32B32A32_FLOAT:
            return R300_COLOR_FORMAT_ARGB32323232;

        /* YUV buffers. */
        case PIPE_FORMAT_UYVY:
            return R300_COLOR_FORMAT_YVYU;
        case PIPE_FORMAT_YUYV:
            return R300_COLOR_FORMAT_VYUY;
        default:
            return ~0; /* Unsupported. */
    }
}

/* Depthbuffer and stencilbuffer. Thankfully, we only support two flavors. */
static uint32_t r300_translate_zsformat(enum pipe_format format)
{
    switch (format) {
        /* 16-bit depth, no stencil */
        case PIPE_FORMAT_Z16_UNORM:
            return R300_DEPTHFORMAT_16BIT_INT_Z;
        /* 24-bit depth, ignored stencil */
        case PIPE_FORMAT_X8Z24_UNORM:
        /* 24-bit depth, 8-bit stencil */
        case PIPE_FORMAT_S8_USCALED_Z24_UNORM:
            return R300_DEPTHFORMAT_24BIT_INT_Z_8BIT_STENCIL;
        default:
            return ~0; /* Unsupported. */
    }
}

/* Shader output formats. This is essentially the swizzle from the shader
 * to the RB3D block.
 *
 * Note that formats are stored from C3 to C0. */
static uint32_t r300_translate_out_fmt(enum pipe_format format)
{
    uint32_t modifier = 0;
    unsigned i;
    const struct util_format_description *desc;
    static const uint32_t sign_bit[4] = {
        R300_OUT_SIGN(0x1),
        R300_OUT_SIGN(0x2),
        R300_OUT_SIGN(0x4),
        R300_OUT_SIGN(0x8),
    };

    desc = util_format_description(format);

    /* Specifies how the shader output is written to the fog unit. */
    if (desc->channel[0].type == UTIL_FORMAT_TYPE_FLOAT) {
        if (desc->channel[0].size == 32) {
            modifier |= R300_US_OUT_FMT_C4_32_FP;
        } else {
            modifier |= R300_US_OUT_FMT_C4_16_FP;
        }
    } else {
        if (desc->channel[0].size == 16) {
            modifier |= R300_US_OUT_FMT_C4_16;
        } else {
            /* C4_8 seems to be used for the formats whose pixel size
             * is <= 32 bits. */
            modifier |= R300_US_OUT_FMT_C4_8;
        }
    }

    /* Add sign. */
    for (i = 0; i < 4; i++)
        if (desc->channel[i].type == UTIL_FORMAT_TYPE_SIGNED) {
            modifier |= sign_bit[i];
        }

    /* Add swizzles and return. */
    switch (format) {
        /* 8-bit outputs.
         * COLORFORMAT_I8 stores the C2 component. */
        case PIPE_FORMAT_A8_UNORM:
            return modifier | R300_C2_SEL_A;
        case PIPE_FORMAT_I8_UNORM:
        case PIPE_FORMAT_L8_UNORM:
        case PIPE_FORMAT_R8_UNORM:
        case PIPE_FORMAT_R8_SNORM:
            return modifier | R300_C2_SEL_R;

        /* BGRA outputs. */
        case PIPE_FORMAT_B5G6R5_UNORM:
        case PIPE_FORMAT_B5G5R5A1_UNORM:
        case PIPE_FORMAT_B5G5R5X1_UNORM:
        case PIPE_FORMAT_B4G4R4A4_UNORM:
        case PIPE_FORMAT_B4G4R4X4_UNORM:
        case PIPE_FORMAT_B8G8R8A8_UNORM:
        case PIPE_FORMAT_B8G8R8X8_UNORM:
        case PIPE_FORMAT_B10G10R10A2_UNORM:
            return modifier |
                R300_C0_SEL_B | R300_C1_SEL_G |
                R300_C2_SEL_R | R300_C3_SEL_A;

        /* ARGB outputs. */
        case PIPE_FORMAT_A8R8G8B8_UNORM:
        case PIPE_FORMAT_X8R8G8B8_UNORM:
            return modifier |
                R300_C0_SEL_A | R300_C1_SEL_R |
                R300_C2_SEL_G | R300_C3_SEL_B;

        /* ABGR outputs. */
        case PIPE_FORMAT_A8B8G8R8_UNORM:
        case PIPE_FORMAT_X8B8G8R8_UNORM:
            return modifier |
                R300_C0_SEL_A | R300_C1_SEL_B |
                R300_C2_SEL_G | R300_C3_SEL_R;

        /* RGBA outputs. */
        case PIPE_FORMAT_R8G8B8X8_UNORM:
        case PIPE_FORMAT_R8G8B8A8_SNORM:
        case PIPE_FORMAT_R8SG8SB8UX8U_NORM:
        case PIPE_FORMAT_R10G10B10A2_UNORM:
        case PIPE_FORMAT_R10G10B10X2_SNORM:
        case PIPE_FORMAT_R10SG10SB10SA2U_NORM:
        case PIPE_FORMAT_R16G16B16A16_UNORM:
        case PIPE_FORMAT_R16G16B16A16_SNORM:
        case PIPE_FORMAT_R16G16B16A16_FLOAT:
        case PIPE_FORMAT_R32G32B32A32_FLOAT:
            return modifier |
                R300_C0_SEL_R | R300_C1_SEL_G |
                R300_C2_SEL_B | R300_C3_SEL_A;

        default:
            return ~0; /* Unsupported. */
    }
}

boolean r300_is_colorbuffer_format_supported(enum pipe_format format)
{
    return r300_translate_colorformat(format) != ~0 &&
           r300_translate_out_fmt(format) != ~0;
}

boolean r300_is_zs_format_supported(enum pipe_format format)
{
    return r300_translate_zsformat(format) != ~0;
}

boolean r300_is_sampler_format_supported(enum pipe_format format)
{
    return r300_translate_texformat(format, 0) != ~0;
}

static void r300_texture_setup_immutable_state(struct r300_screen* screen,
                                               struct r300_texture* tex)
{
    struct r300_texture_format_state* f = &tex->tx_format;
    struct pipe_resource *pt = &tex->b.b;
    boolean is_r500 = screen->caps.is_r500;

    /* Set sampler state. */
    f->format0 = R300_TX_WIDTH((pt->width0 - 1) & 0x7ff) |
                 R300_TX_HEIGHT((pt->height0 - 1) & 0x7ff);

    if (tex->uses_pitch) {
        /* rectangles love this */
        f->format0 |= R300_TX_PITCH_EN;
        f->format2 = (tex->hwpitch[0] - 1) & 0x1fff;
    } else {
        /* power of two textures (3D, mipmaps, and no pitch) */
        f->format0 |= R300_TX_DEPTH(util_logbase2(pt->depth0) & 0xf);
    }

    f->format1 = 0;
    if (pt->target == PIPE_TEXTURE_CUBE) {
        f->format1 |= R300_TX_FORMAT_CUBIC_MAP;
    }
    if (pt->target == PIPE_TEXTURE_3D) {
        f->format1 |= R300_TX_FORMAT_3D;
    }

    /* large textures on r500 */
    if (is_r500)
    {
        if (pt->width0 > 2048) {
            f->format2 |= R500_TXWIDTH_BIT11;
        }
        if (pt->height0 > 2048) {
            f->format2 |= R500_TXHEIGHT_BIT11;
        }
    }

    f->tile_config = R300_TXO_MACRO_TILE(tex->macrotile) |
                     R300_TXO_MICRO_TILE(tex->microtile);
}

static void r300_texture_setup_fb_state(struct r300_screen* screen,
                                        struct r300_texture* tex)
{
    unsigned i;

    /* Set framebuffer state. */
    if (util_format_is_depth_or_stencil(tex->b.b.format)) {
        for (i = 0; i <= tex->b.b.last_level; i++) {
            tex->fb_state.depthpitch[i] =
                tex->hwpitch[i] |
                R300_DEPTHMACROTILE(tex->mip_macrotile[i]) |
                R300_DEPTHMICROTILE(tex->microtile);
        }
        tex->fb_state.zb_format = r300_translate_zsformat(tex->b.b.format);
    } else {
        for (i = 0; i <= tex->b.b.last_level; i++) {
            tex->fb_state.colorpitch[i] =
                tex->hwpitch[i] |
                r300_translate_colorformat(tex->b.b.format) |
                R300_COLOR_TILE(tex->mip_macrotile[i]) |
                R300_COLOR_MICROTILE(tex->microtile);
        }
        tex->fb_state.us_out_fmt = r300_translate_out_fmt(tex->b.b.format);
    }
}

void r300_texture_reinterpret_format(struct pipe_screen *screen,
                                     struct pipe_resource *tex,
                                     enum pipe_format new_format)
{
    struct r300_screen *r300screen = r300_screen(screen);

    SCREEN_DBG(r300screen, DBG_TEX, "r300: texture_reinterpret_format: %s -> %s\n",
               util_format_name(tex->format), util_format_name(new_format));

    tex->format = new_format;

    r300_texture_setup_fb_state(r300_screen(screen), r300_texture(tex));
}

unsigned r300_texture_get_offset(struct r300_texture* tex, unsigned level,
                                 unsigned zslice, unsigned face)
{
    unsigned offset = tex->offset[level];

    switch (tex->b.b.target) {
        case PIPE_TEXTURE_3D:
            assert(face == 0);
            return offset + zslice * tex->layer_size[level];

        case PIPE_TEXTURE_CUBE:
            assert(zslice == 0);
            return offset + face * tex->layer_size[level];

        default:
            assert(zslice == 0 && face == 0);
            return offset;
    }
}

/**
 * Return the width (dim==TILE_WIDTH) or height (dim==TILE_HEIGHT) of one tile
 * of the given texture.
 */
static unsigned r300_texture_get_tile_size(struct r300_texture* tex,
                                           int dim, boolean macrotile)
{
    unsigned pixsize, tile_size;

    pixsize = util_format_get_blocksize(tex->b.b.format);
    tile_size = microblock_table[util_logbase2(pixsize)][tex->microtile][dim];

    if (macrotile) {
        tile_size *= 8;
    }

    assert(tile_size);
    return tile_size;
}

/* Return true if macrotiling should be enabled on the miplevel. */
static boolean r300_texture_macro_switch(struct r300_texture *tex,
                                         unsigned level,
                                         boolean rv350_mode,
                                         int dim)
{
    unsigned tile, texdim;

    tile = r300_texture_get_tile_size(tex, dim, TRUE);
    if (dim == TILE_WIDTH) {
        texdim = u_minify(tex->b.b.width0, level);
    } else {
        texdim = u_minify(tex->b.b.height0, level);
    }

    /* See TX_FILTER1_n.MACRO_SWITCH. */
    if (rv350_mode) {
        return texdim >= tile;
    } else {
        return texdim > tile;
    }
}

/**
 * Return the stride, in bytes, of the texture images of the given texture
 * at the given level.
 */
unsigned r300_texture_get_stride(struct r300_screen* screen,
                                 struct r300_texture* tex, unsigned level)
{
    unsigned tile_width, width;

    if (tex->stride_override)
        return tex->stride_override;

    /* Check the level. */
    if (level > tex->b.b.last_level) {
        SCREEN_DBG(screen, DBG_TEX, "%s: level (%u) > last_level (%u)\n",
                   __FUNCTION__, level, tex->b.b.last_level);
        return 0;
    }

    width = u_minify(tex->b.b.width0, level);

    if (r300_format_is_plain(tex->b.b.format)) {
        tile_width = r300_texture_get_tile_size(tex, TILE_WIDTH,
                                                tex->mip_macrotile[level]);
        width = align(width, tile_width);

        return util_format_get_stride(tex->b.b.format, width);
    } else {
        return align(util_format_get_stride(tex->b.b.format, width), 32);
    }
}

static unsigned r300_texture_get_nblocksy(struct r300_texture* tex,
                                          unsigned level)
{
    unsigned height, tile_height;

    height = u_minify(tex->b.b.height0, level);

    if (r300_format_is_plain(tex->b.b.format)) {
        tile_height = r300_texture_get_tile_size(tex, TILE_HEIGHT,
                                                 tex->mip_macrotile[level]);
        height = align(height, tile_height);

        /* This is needed for the kernel checker, unfortunately. */
        height = util_next_power_of_two(height);
    }

    return util_format_get_nblocksy(tex->b.b.format, height);
}

static void r300_texture_3d_fix_mipmapping(struct r300_screen *screen,
                                           struct r300_texture *tex)
{
    /* The kernels <= 2.6.34-rc4 compute the size of mipmapped 3D textures
     * incorrectly. This is a workaround to prevent CS from being rejected. */

    unsigned i, size;

    if (!screen->rws->get_value(screen->rws, R300_VID_DRM_2_3_0) &&
        tex->b.b.target == PIPE_TEXTURE_3D &&
        tex->b.b.last_level > 0) {
        size = 0;

        for (i = 0; i <= tex->b.b.last_level; i++) {
            size += r300_texture_get_stride(screen, tex, i) *
                    r300_texture_get_nblocksy(tex, i);
        }

        size *= tex->b.b.depth0;
        tex->size = size;
    }
}

static void r300_setup_miptree(struct r300_screen* screen,
                               struct r300_texture* tex)
{
    struct pipe_resource* base = &tex->b.b;
    unsigned stride, size, layer_size, nblocksy, i;
    boolean rv350_mode = screen->caps.is_rv350;

    SCREEN_DBG(screen, DBG_TEXALLOC, "r300: Making miptree for texture, format %s\n",
               util_format_name(base->format));

    for (i = 0; i <= base->last_level; i++) {
        /* Let's see if this miplevel can be macrotiled. */
        tex->mip_macrotile[i] =
            (tex->macrotile == R300_BUFFER_TILED &&
             r300_texture_macro_switch(tex, i, rv350_mode, TILE_WIDTH) &&
             r300_texture_macro_switch(tex, i, rv350_mode, TILE_HEIGHT)) ?
             R300_BUFFER_TILED : R300_BUFFER_LINEAR;

        stride = r300_texture_get_stride(screen, tex, i);
        nblocksy = r300_texture_get_nblocksy(tex, i);
        layer_size = stride * nblocksy;

        if (base->target == PIPE_TEXTURE_CUBE)
            size = layer_size * 6;
        else
            size = layer_size * u_minify(base->depth0, i);

        tex->offset[i] = tex->size;
        tex->size = tex->offset[i] + size;
        tex->layer_size[i] = layer_size;
        tex->pitch[i] = stride / util_format_get_blocksize(base->format);
        tex->hwpitch[i] =
                tex->pitch[i] * util_format_get_blockwidth(base->format);

        SCREEN_DBG(screen, DBG_TEXALLOC, "r300: Texture miptree: Level %d "
                "(%dx%dx%d px, pitch %d bytes) %d bytes total, macrotiled %s\n",
                i, u_minify(base->width0, i), u_minify(base->height0, i),
                u_minify(base->depth0, i), stride, tex->size,
                tex->mip_macrotile[i] ? "TRUE" : "FALSE");
    }
}

static void r300_setup_flags(struct r300_texture* tex)
{
    tex->uses_pitch = !util_is_power_of_two(tex->b.b.width0) ||
                      !util_is_power_of_two(tex->b.b.height0) ||
                      tex->stride_override;
}

static void r300_setup_tiling(struct pipe_screen *screen,
                              struct r300_texture *tex)
{
    struct r300_winsys_screen *rws = (struct r300_winsys_screen *)screen->winsys;
    enum pipe_format format = tex->b.b.format;
    boolean rv350_mode = r300_screen(screen)->caps.is_rv350;
    boolean is_zb = util_format_is_depth_or_stencil(format);
    boolean dbg_no_tiling = SCREEN_DBG_ON(r300_screen(screen), DBG_NO_TILING);

    if (!r300_format_is_plain(format)) {
        return;
    }

    /* If height == 1, disable microtiling except for zbuffer. */
    if (!is_zb && (tex->b.b.height0 == 1 || dbg_no_tiling)) {
        return;
    }

    /* Set microtiling. */
    switch (util_format_get_blocksize(format)) {
        case 1:
        case 4:
            tex->microtile = R300_BUFFER_TILED;
            break;

        case 2:
        case 8:
            if (rws->get_value(rws, R300_VID_SQUARE_TILING_SUPPORT)) {
                tex->microtile = R300_BUFFER_SQUARETILED;
            }
            break;
    }

    if (dbg_no_tiling) {
        return;
    }

    /* Set macrotiling. */
    if (r300_texture_macro_switch(tex, 0, rv350_mode, TILE_WIDTH) &&
        r300_texture_macro_switch(tex, 0, rv350_mode, TILE_HEIGHT)) {
        tex->macrotile = R300_BUFFER_TILED;
    }
}

static unsigned r300_texture_is_referenced(struct pipe_context *context,
					 struct pipe_resource *texture,
					 unsigned face, unsigned level)
{
    struct r300_context *r300 = r300_context(context);
    struct r300_texture *rtex = (struct r300_texture *)texture;

    if (r300->rws->is_buffer_referenced(r300->rws, rtex->buffer, R300_REF_CS))
        return PIPE_REFERENCED_FOR_READ | PIPE_REFERENCED_FOR_WRITE;

    return PIPE_UNREFERENCED;
}

static void r300_texture_destroy(struct pipe_screen *screen,
				 struct pipe_resource* texture)
{
    struct r300_texture* tex = (struct r300_texture*)texture;
    struct r300_winsys_screen *rws = (struct r300_winsys_screen *)texture->screen->winsys;

    rws->buffer_reference(rws, &tex->buffer, NULL);
    FREE(tex);
}

static boolean r300_texture_get_handle(struct pipe_screen* screen,
                                       struct pipe_resource *texture,
                                       struct winsys_handle *whandle)
{
    struct r300_winsys_screen *rws = (struct r300_winsys_screen *)screen->winsys;
    struct r300_texture* tex = (struct r300_texture*)texture;
    unsigned stride;

    if (!tex) {
        return FALSE;
    }

    stride = r300_texture_get_stride(r300_screen(screen), tex, 0);

    rws->buffer_get_handle(rws, tex->buffer, stride, whandle);

    return TRUE;
}

struct u_resource_vtbl r300_texture_vtbl = 
{
   r300_texture_get_handle,	      /* get_handle */
   r300_texture_destroy,	      /* resource_destroy */
   r300_texture_is_referenced,	      /* is_resource_referenced */
   r300_texture_get_transfer,	      /* get_transfer */
   r300_texture_transfer_destroy,     /* transfer_destroy */
   r300_texture_transfer_map,	      /* transfer_map */
   u_default_transfer_flush_region,   /* transfer_flush_region */
   r300_texture_transfer_unmap,	      /* transfer_unmap */
   u_default_transfer_inline_write    /* transfer_inline_write */
};

/* Create a new texture. */
struct pipe_resource* r300_texture_create(struct pipe_screen* screen,
                                          const struct pipe_resource* base)
{
    struct r300_texture* tex = CALLOC_STRUCT(r300_texture);
    struct r300_screen* rscreen = r300_screen(screen);
    struct r300_winsys_screen *rws = (struct r300_winsys_screen *)screen->winsys;

    if (!tex) {
        return NULL;
    }

    tex->b.b = *base;
    tex->b.vtbl = &r300_texture_vtbl;
    pipe_reference_init(&tex->b.b.reference, 1);
    tex->b.b.screen = screen;

    r300_setup_flags(tex);
    if (!(base->flags & R300_RESOURCE_FLAG_TRANSFER) &&
        !(base->bind & PIPE_BIND_SCANOUT)) {
        r300_setup_tiling(screen, tex);
    }
    r300_setup_miptree(rscreen, tex);
    r300_texture_3d_fix_mipmapping(rscreen, tex);
    r300_texture_setup_immutable_state(rscreen, tex);
    r300_texture_setup_fb_state(rscreen, tex);

    SCREEN_DBG(rscreen, DBG_TEX,
               "r300: texture_create: Macro: %s, Micro: %s, Pitch: %i, "
               "Dim: %ix%ix%i, LastLevel: %i, Format: %s\n",
               tex->macrotile ? "YES" : " NO",
               tex->microtile ? "YES" : " NO",
               tex->hwpitch[0],
               base->width0, base->height0, base->depth0, base->last_level,
               util_format_name(base->format));

    tex->buffer = rws->buffer_create(rws, 2048,
				     PIPE_BIND_SAMPLER_VIEW, /* XXX */
				     tex->size);
    rws->buffer_set_tiling(rws, tex->buffer,
			   tex->pitch[0],
			   tex->microtile,
			   tex->macrotile);

    if (!tex->buffer) {
        FREE(tex);
        return NULL;
    }

    return (struct pipe_resource*)tex;
}

/* Not required to implement u_resource_vtbl, consider moving to another file:
 */
struct pipe_surface* r300_get_tex_surface(struct pipe_screen* screen,
					  struct pipe_resource* texture,
					  unsigned face,
					  unsigned level,
					  unsigned zslice,
					  unsigned flags)
{
    struct r300_texture* tex = r300_texture(texture);
    struct pipe_surface* surface = CALLOC_STRUCT(pipe_surface);
    unsigned offset;

    offset = r300_texture_get_offset(tex, level, zslice, face);

    if (surface) {
        pipe_reference_init(&surface->reference, 1);
        pipe_resource_reference(&surface->texture, texture);
        surface->format = texture->format;
        surface->width = u_minify(texture->width0, level);
        surface->height = u_minify(texture->height0, level);
        surface->offset = offset;
        surface->usage = flags;
        surface->zslice = zslice;
        surface->texture = texture;
        surface->face = face;
        surface->level = level;
    }

    return surface;
}

/* Not required to implement u_resource_vtbl, consider moving to another file:
 */
void r300_tex_surface_destroy(struct pipe_surface* s)
{
    pipe_resource_reference(&s->texture, NULL);
    FREE(s);
}

struct pipe_resource*
r300_texture_from_handle(struct pipe_screen* screen,
			  const struct pipe_resource* base,
			  struct winsys_handle *whandle)
{
    struct r300_winsys_screen *rws = (struct r300_winsys_screen*)screen->winsys;
    struct r300_screen* rscreen = r300_screen(screen);
    struct r300_winsys_buffer *buffer;
    struct r300_texture* tex;
    unsigned stride;
    boolean override_zb_flags;

    /* Support only 2D textures without mipmaps */
    if (base->target != PIPE_TEXTURE_2D ||
        base->depth0 != 1 ||
        base->last_level != 0) {
        return NULL;
    }

    buffer = rws->buffer_from_handle(rws, screen, whandle, &stride);
    if (!buffer) {
        return NULL;
    }

    tex = CALLOC_STRUCT(r300_texture);
    if (!tex) {
        return NULL;
    }

    tex->b.b = *base;
    tex->b.vtbl = &r300_texture_vtbl;
    pipe_reference_init(&tex->b.b.reference, 1);
    tex->b.b.screen = screen;

    tex->stride_override = stride;

    /* one ref already taken */
    tex->buffer = buffer;

    rws->buffer_get_tiling(rws, buffer, &tex->microtile, &tex->macrotile);
    r300_setup_flags(tex);
    SCREEN_DBG(rscreen, DBG_TEX,
               "r300: texture_from_handle: Macro: %s, Micro: %s, "
               "Pitch: % 4i, Dim: %ix%i, Format: %s\n",
               tex->macrotile ? "YES" : " NO",
               tex->microtile ? "YES" : " NO",
               stride / util_format_get_blocksize(base->format),
               base->width0, base->height0,
               util_format_name(base->format));

    /* Enforce microtiled zbuffer. */
    override_zb_flags = util_format_is_depth_or_stencil(base->format) &&
                        tex->microtile == R300_BUFFER_LINEAR;

    if (override_zb_flags) {
        switch (util_format_get_blocksize(base->format)) {
            case 4:
                tex->microtile = R300_BUFFER_TILED;
                break;

            case 2:
                if (rws->get_value(rws, R300_VID_SQUARE_TILING_SUPPORT)) {
                    tex->microtile = R300_BUFFER_SQUARETILED;
                    break;
                }
                /* Pass through. */

            default:
                override_zb_flags = FALSE;
        }
    }

    r300_setup_miptree(rscreen, tex);
    r300_texture_setup_immutable_state(rscreen, tex);
    r300_texture_setup_fb_state(rscreen, tex);

    if (override_zb_flags) {
        rws->buffer_set_tiling(rws, tex->buffer,
                               tex->pitch[0],
                               tex->microtile,
                               tex->macrotile);
    }
    return (struct pipe_resource*)tex;
}
