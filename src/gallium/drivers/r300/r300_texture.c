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

/* Always include headers in the reverse order!! ~ M. */
#include "r300_texture.h"

#include "r300_context.h"
#include "r300_reg.h"
#include "r300_texture_desc.h"
#include "r300_transfer.h"
#include "r300_screen.h"
#include "r300_winsys.h"

#include "util/u_format.h"
#include "util/u_format_s3tc.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_mm.h"

#include "pipe/p_screen.h"

unsigned r300_get_swizzle_combined(const unsigned char *swizzle_format,
                                   const unsigned char *swizzle_view,
                                   boolean dxtc_swizzle)
{
    unsigned i;
    unsigned char swizzle[4];
    unsigned result = 0;
    const uint32_t swizzle_shift[4] = {
        R300_TX_FORMAT_R_SHIFT,
        R300_TX_FORMAT_G_SHIFT,
        R300_TX_FORMAT_B_SHIFT,
        R300_TX_FORMAT_A_SHIFT
    };
    uint32_t swizzle_bit[4] = {
        dxtc_swizzle ? R300_TX_FORMAT_Z : R300_TX_FORMAT_X,
        R300_TX_FORMAT_Y,
        dxtc_swizzle ? R300_TX_FORMAT_X : R300_TX_FORMAT_Z,
        R300_TX_FORMAT_W
    };

    if (swizzle_view) {
        /* Combine two sets of swizzles. */
        for (i = 0; i < 4; i++) {
            swizzle[i] = swizzle_view[i] <= UTIL_FORMAT_SWIZZLE_W ?
                         swizzle_format[swizzle_view[i]] : swizzle_view[i];
        }
    } else {
        memcpy(swizzle, swizzle_format, 4);
    }

    /* Get swizzle. */
    for (i = 0; i < 4; i++) {
        switch (swizzle[i]) {
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
            default: /* UTIL_FORMAT_SWIZZLE_X */
                result |= swizzle_bit[0] << swizzle_shift[i];
        }
    }
    return result;
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
                                  const unsigned char *swizzle_view,
                                  boolean is_r500,
                                  boolean dxtc_swizzle)
{
    uint32_t result = 0;
    const struct util_format_description *desc;
    unsigned i;
    boolean uniform = TRUE;
    const uint32_t sign_bit[4] = {
        R300_TX_FORMAT_SIGNED_X,
        R300_TX_FORMAT_SIGNED_Y,
        R300_TX_FORMAT_SIGNED_Z,
        R300_TX_FORMAT_SIGNED_W,
    };

    desc = util_format_description(format);

    /* Colorspace (return non-RGB formats directly). */
    switch (desc->colorspace) {
        /* Depth stencil formats.
         * Swizzles are added in r300_merge_textures_and_samplers. */
        case UTIL_FORMAT_COLORSPACE_ZS:
            switch (format) {
                case PIPE_FORMAT_Z16_UNORM:
                    return R300_TX_FORMAT_X16;
                case PIPE_FORMAT_X8Z24_UNORM:
                case PIPE_FORMAT_S8_USCALED_Z24_UNORM:
                    if (is_r500)
                        return R500_TX_FORMAT_Y8X24;
                    else
                        return R300_TX_FORMAT_Y16X16;
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

    result |= r300_get_swizzle_combined(desc->swizzle, swizzle_view,
                    util_format_is_compressed(format) && dxtc_swizzle);

    /* S3TC formats. */
    if (desc->layout == UTIL_FORMAT_LAYOUT_S3TC) {
        if (!util_format_s3tc_enabled) {
            return ~0; /* Unsupported. */
        }

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
                if (desc->channel[0].size == 2 &&
                    desc->channel[1].size == 3 &&
                    desc->channel[2].size == 3) {
                    return R300_TX_FORMAT_Z3Y3X2 | result;
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

    /* Find the first non-VOID channel. */
    for (i = 0; i < 4; i++) {
        if (desc->channel[i].type != UTIL_FORMAT_TYPE_VOID) {
            break;
        }
    }

    if (i == 4)
        return ~0; /* Unsupported/unknown. */

    /* And finally, uniform formats. */
    switch (desc->channel[i].type) {
        case UTIL_FORMAT_TYPE_UNSIGNED:
        case UTIL_FORMAT_TYPE_SIGNED:
            if (!desc->channel[i].normalized &&
                desc->colorspace != UTIL_FORMAT_COLORSPACE_SRGB) {
                return ~0;
            }

            switch (desc->channel[i].size) {
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
            switch (desc->channel[i].size) {
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
        case PIPE_FORMAT_X8Z24_UNORM:
        case PIPE_FORMAT_S8_USCALED_Z24_UNORM:
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
        /*case PIPE_FORMAT_A8_SNORM:*/
        case PIPE_FORMAT_I8_UNORM:
        /*case PIPE_FORMAT_I8_SNORM:*/
        case PIPE_FORMAT_L8_UNORM:
        /*case PIPE_FORMAT_L8_SNORM:*/
        case PIPE_FORMAT_L8_SRGB:
        case PIPE_FORMAT_R8_UNORM:
        case PIPE_FORMAT_R8_SNORM:
            return R300_COLOR_FORMAT_I8;

        /* 16-bit buffers. */
        case PIPE_FORMAT_L8A8_UNORM:
        /*case PIPE_FORMAT_L8A8_SNORM:*/
        case PIPE_FORMAT_L8A8_SRGB:
        case PIPE_FORMAT_R8G8_UNORM:
        case PIPE_FORMAT_R8G8_SNORM:
            return R300_COLOR_FORMAT_UV88;

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
        /*case PIPE_FORMAT_B8G8R8A8_SNORM:*/
        case PIPE_FORMAT_B8G8R8A8_SRGB:
        case PIPE_FORMAT_B8G8R8X8_UNORM:
        /*case PIPE_FORMAT_B8G8R8X8_SNORM:*/
        case PIPE_FORMAT_B8G8R8X8_SRGB:
        case PIPE_FORMAT_A8R8G8B8_UNORM:
        /*case PIPE_FORMAT_A8R8G8B8_SNORM:*/
        case PIPE_FORMAT_A8R8G8B8_SRGB:
        case PIPE_FORMAT_X8R8G8B8_UNORM:
        /*case PIPE_FORMAT_X8R8G8B8_SNORM:*/
        case PIPE_FORMAT_X8R8G8B8_SRGB:
        case PIPE_FORMAT_A8B8G8R8_UNORM:
        /*case PIPE_FORMAT_A8B8G8R8_SNORM:*/
        case PIPE_FORMAT_A8B8G8R8_SRGB:
        case PIPE_FORMAT_R8G8B8A8_UNORM:
        case PIPE_FORMAT_R8G8B8A8_SNORM:
        case PIPE_FORMAT_R8G8B8A8_SRGB:
        case PIPE_FORMAT_X8B8G8R8_UNORM:
        /*case PIPE_FORMAT_X8B8G8R8_SNORM:*/
        case PIPE_FORMAT_X8B8G8R8_SRGB:
        case PIPE_FORMAT_R8G8B8X8_UNORM:
        /*case PIPE_FORMAT_R8G8B8X8_SNORM:*/
        /*case PIPE_FORMAT_R8G8B8X8_SRGB:*/
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

    /* Find the first non-VOID channel. */
    for (i = 0; i < 4; i++) {
        if (desc->channel[i].type != UTIL_FORMAT_TYPE_VOID) {
            break;
        }
    }

    if (i == 4)
        return ~0; /* Unsupported/unknown. */

    /* Specifies how the shader output is written to the fog unit. */
    if (desc->channel[i].type == UTIL_FORMAT_TYPE_FLOAT) {
        if (desc->channel[i].size == 32) {
            modifier |= R300_US_OUT_FMT_C4_32_FP;
        } else {
            modifier |= R300_US_OUT_FMT_C4_16_FP;
        }
    } else {
        if (desc->channel[i].size == 16) {
            modifier |= R300_US_OUT_FMT_C4_16;
        } else if (desc->channel[i].size == 10) {
            modifier |= R300_US_OUT_FMT_C4_10;
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
        /* 8-bit outputs, one channel.
         * COLORFORMAT_I8 stores the C2 component. */
        case PIPE_FORMAT_A8_UNORM:
        /*case PIPE_FORMAT_A8_SNORM:*/
            return modifier | R300_C2_SEL_A;
        case PIPE_FORMAT_I8_UNORM:
        /*case PIPE_FORMAT_I8_SNORM:*/
        case PIPE_FORMAT_L8_UNORM:
        /*case PIPE_FORMAT_L8_SNORM:*/
        case PIPE_FORMAT_L8_SRGB:
        case PIPE_FORMAT_R8_UNORM:
        case PIPE_FORMAT_R8_SNORM:
            return modifier | R300_C2_SEL_R;

        /* 16-bit outputs, two channels.
         * COLORFORMAT_UV88 stores C2 and C0. */
        case PIPE_FORMAT_L8A8_UNORM:
        /*case PIPE_FORMAT_L8A8_SNORM:*/
        case PIPE_FORMAT_L8A8_SRGB:
            return modifier | R300_C0_SEL_A | R300_C2_SEL_R;
        case PIPE_FORMAT_R8G8_UNORM:
        case PIPE_FORMAT_R8G8_SNORM:
            return modifier | R300_C0_SEL_G | R300_C2_SEL_R;

        /* BGRA outputs. */
        case PIPE_FORMAT_B5G6R5_UNORM:
        case PIPE_FORMAT_B5G5R5A1_UNORM:
        case PIPE_FORMAT_B5G5R5X1_UNORM:
        case PIPE_FORMAT_B4G4R4A4_UNORM:
        case PIPE_FORMAT_B4G4R4X4_UNORM:
        case PIPE_FORMAT_B8G8R8A8_UNORM:
        /*case PIPE_FORMAT_B8G8R8A8_SNORM:*/
        case PIPE_FORMAT_B8G8R8A8_SRGB:
        case PIPE_FORMAT_B8G8R8X8_UNORM:
        /*case PIPE_FORMAT_B8G8R8X8_SNORM:*/
        case PIPE_FORMAT_B8G8R8X8_SRGB:
        case PIPE_FORMAT_B10G10R10A2_UNORM:
            return modifier |
                R300_C0_SEL_B | R300_C1_SEL_G |
                R300_C2_SEL_R | R300_C3_SEL_A;

        /* ARGB outputs. */
        case PIPE_FORMAT_A8R8G8B8_UNORM:
        /*case PIPE_FORMAT_A8R8G8B8_SNORM:*/
        case PIPE_FORMAT_A8R8G8B8_SRGB:
        case PIPE_FORMAT_X8R8G8B8_UNORM:
        /*case PIPE_FORMAT_X8R8G8B8_SNORM:*/
        case PIPE_FORMAT_X8R8G8B8_SRGB:
            return modifier |
                R300_C0_SEL_A | R300_C1_SEL_R |
                R300_C2_SEL_G | R300_C3_SEL_B;

        /* ABGR outputs. */
        case PIPE_FORMAT_A8B8G8R8_UNORM:
        /*case PIPE_FORMAT_A8B8G8R8_SNORM:*/
        case PIPE_FORMAT_A8B8G8R8_SRGB:
        case PIPE_FORMAT_X8B8G8R8_UNORM:
        /*case PIPE_FORMAT_X8B8G8R8_SNORM:*/
        case PIPE_FORMAT_X8B8G8R8_SRGB:
            return modifier |
                R300_C0_SEL_A | R300_C1_SEL_B |
                R300_C2_SEL_G | R300_C3_SEL_R;

        /* RGBA outputs. */
        case PIPE_FORMAT_R8G8B8X8_UNORM:
        /*case PIPE_FORMAT_R8G8B8X8_SNORM:*/
        /*case PIPE_FORMAT_R8G8B8X8_SRGB:*/
        case PIPE_FORMAT_R8G8B8A8_UNORM:
        case PIPE_FORMAT_R8G8B8A8_SNORM:
        case PIPE_FORMAT_R8G8B8A8_SRGB:
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
    return r300_translate_texformat(format, 0, TRUE, FALSE) != ~0;
}

void r300_texture_setup_format_state(struct r300_screen *screen,
                                     struct r300_texture_desc *desc,
                                     unsigned level,
                                     struct r300_texture_format_state *out)
{
    struct pipe_resource *pt = &desc->b.b;
    boolean is_r500 = screen->caps.is_r500;

    /* Mask out all the fields we change. */
    out->format0 = 0;
    out->format1 &= ~R300_TX_FORMAT_TEX_COORD_TYPE_MASK;
    out->format2 &= R500_TXFORMAT_MSB;
    out->tile_config = 0;

    /* Set sampler state. */
    out->format0 =
        R300_TX_WIDTH((u_minify(desc->width0, level) - 1) & 0x7ff) |
        R300_TX_HEIGHT((u_minify(desc->height0, level) - 1) & 0x7ff) |
        R300_TX_DEPTH(util_logbase2(u_minify(desc->depth0, level)) & 0xf);

    if (desc->uses_stride_addressing) {
        /* rectangles love this */
        out->format0 |= R300_TX_PITCH_EN;
        out->format2 = (desc->stride_in_pixels[level] - 1) & 0x1fff;
    }

    if (pt->target == PIPE_TEXTURE_CUBE) {
        out->format1 |= R300_TX_FORMAT_CUBIC_MAP;
    }
    if (pt->target == PIPE_TEXTURE_3D) {
        out->format1 |= R300_TX_FORMAT_3D;
    }

    /* large textures on r500 */
    if (is_r500)
    {
        if (desc->width0 > 2048) {
            out->format2 |= R500_TXWIDTH_BIT11;
        }
        if (desc->height0 > 2048) {
            out->format2 |= R500_TXHEIGHT_BIT11;
        }
    }

    out->tile_config = R300_TXO_MACRO_TILE(desc->macrotile[level]) |
                       R300_TXO_MICRO_TILE(desc->microtile);
}

static void r300_texture_setup_fb_state(struct r300_screen* screen,
                                        struct r300_texture* tex)
{
    unsigned i;

    /* Set framebuffer state. */
    if (util_format_is_depth_or_stencil(tex->desc.b.b.format)) {
        for (i = 0; i <= tex->desc.b.b.last_level; i++) {
            tex->fb_state.pitch[i] =
                tex->desc.stride_in_pixels[i] |
                R300_DEPTHMACROTILE(tex->desc.macrotile[i]) |
                R300_DEPTHMICROTILE(tex->desc.microtile);
        }
        tex->fb_state.format = r300_translate_zsformat(tex->desc.b.b.format);
    } else {
        for (i = 0; i <= tex->desc.b.b.last_level; i++) {
            tex->fb_state.pitch[i] =
                tex->desc.stride_in_pixels[i] |
                r300_translate_colorformat(tex->desc.b.b.format) |
                R300_COLOR_TILE(tex->desc.macrotile[i]) |
                R300_COLOR_MICROTILE(tex->desc.microtile);
        }
        tex->fb_state.format = r300_translate_out_fmt(tex->desc.b.b.format);
    }
}

void r300_texture_reinterpret_format(struct pipe_screen *screen,
                                     struct pipe_resource *tex,
                                     enum pipe_format new_format)
{
    struct r300_screen *r300screen = r300_screen(screen);

    SCREEN_DBG(r300screen, DBG_TEX,
        "r300: texture_reinterpret_format: %s -> %s\n",
        util_format_short_name(tex->format),
        util_format_short_name(new_format));

    tex->format = new_format;

    r300_texture_setup_fb_state(r300_screen(screen), r300_texture(tex));
}

static unsigned r300_texture_is_referenced(struct pipe_context *context,
                                           struct pipe_resource *texture,
                                           unsigned level, int layer)
{
    struct r300_context *r300 = r300_context(context);
    struct r300_texture *rtex = (struct r300_texture *)texture;

    if (r300->rws->cs_is_buffer_referenced(r300->cs,
                                           rtex->cs_buffer, R300_REF_CS))
        return PIPE_REFERENCED_FOR_READ | PIPE_REFERENCED_FOR_WRITE;

    return PIPE_UNREFERENCED;
}

static void r300_texture_destroy(struct pipe_screen *screen,
                                 struct pipe_resource* texture)
{
    struct r300_texture* tex = (struct r300_texture*)texture;
    struct r300_winsys_screen *rws = (struct r300_winsys_screen *)texture->screen->winsys;
    int i;

    rws->buffer_reference(rws, &tex->buffer, NULL);
    for (i = 0; i < R300_MAX_TEXTURE_LEVELS; i++) {
        if (tex->hiz_mem[i])
            u_mmFreeMem(tex->hiz_mem[i]);
        if (tex->zmask_mem[i])
            u_mmFreeMem(tex->zmask_mem[i]);
    }

    FREE(tex);
}

static boolean r300_texture_get_handle(struct pipe_screen* screen,
                                       struct pipe_resource *texture,
                                       struct winsys_handle *whandle)
{
    struct r300_winsys_screen *rws = (struct r300_winsys_screen *)screen->winsys;
    struct r300_texture* tex = (struct r300_texture*)texture;

    if (!tex) {
        return FALSE;
    }

    return rws->buffer_get_handle(rws, tex->buffer,
                                  tex->desc.stride_in_bytes[0], whandle);
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

/* The common texture constructor. */
static struct r300_texture*
r300_texture_create_object(struct r300_screen *rscreen,
                           const struct pipe_resource *base,
                           enum r300_buffer_tiling microtile,
                           enum r300_buffer_tiling macrotile,
                           unsigned stride_in_bytes_override,
                           unsigned max_buffer_size,
                           struct r300_winsys_buffer *buffer)
{
    struct r300_winsys_screen *rws = rscreen->rws;
    struct r300_texture *tex = CALLOC_STRUCT(r300_texture);
    if (!tex) {
        if (buffer)
            rws->buffer_reference(rws, &buffer, NULL);
        return NULL;
    }

    /* Initialize the descriptor. */
    if (!r300_texture_desc_init(rscreen, &tex->desc, base,
                                microtile, macrotile,
                                stride_in_bytes_override,
                                max_buffer_size)) {
        if (buffer)
            rws->buffer_reference(rws, &buffer, NULL);
        FREE(tex);
        return NULL;
    }
    /* Initialize the hardware state. */
    r300_texture_setup_format_state(rscreen, &tex->desc, 0, &tex->tx_format);
    r300_texture_setup_fb_state(rscreen, tex);

    tex->desc.b.vtbl = &r300_texture_vtbl;
    pipe_reference_init(&tex->desc.b.b.reference, 1);
    tex->domain = base->flags & R300_RESOURCE_FLAG_TRANSFER ?
                  R300_DOMAIN_GTT :
                  R300_DOMAIN_VRAM | R300_DOMAIN_GTT;
    tex->buffer = buffer;

    /* Create the backing buffer if needed. */
    if (!tex->buffer) {
        tex->buffer = rws->buffer_create(rws, tex->desc.size_in_bytes, 2048,
                                         base->bind, base->usage, tex->domain);

        if (!tex->buffer) {
            FREE(tex);
            return NULL;
        }
    }

    tex->cs_buffer = rws->buffer_get_cs_handle(rws, tex->buffer);

    rws->buffer_set_tiling(rws, tex->buffer,
            tex->desc.microtile, tex->desc.macrotile[0],
            tex->desc.stride_in_bytes[0]);

    return tex;
}

/* Create a new texture. */
struct pipe_resource *r300_texture_create(struct pipe_screen *screen,
                                          const struct pipe_resource *base)
{
    struct r300_screen *rscreen = r300_screen(screen);
    enum r300_buffer_tiling microtile, macrotile;

    if ((base->flags & R300_RESOURCE_FLAG_TRANSFER) ||
        (base->bind & PIPE_BIND_SCANOUT)) {
        microtile = R300_BUFFER_LINEAR;
        macrotile = R300_BUFFER_LINEAR;
    } else {
        microtile = R300_BUFFER_SELECT_LAYOUT;
        macrotile = R300_BUFFER_SELECT_LAYOUT;
    }

    return (struct pipe_resource*)
           r300_texture_create_object(rscreen, base, microtile, macrotile,
                                      0, 0, NULL);
}

struct pipe_resource *r300_texture_from_handle(struct pipe_screen *screen,
                                               const struct pipe_resource *base,
                                               struct winsys_handle *whandle)
{
    struct r300_winsys_screen *rws = (struct r300_winsys_screen*)screen->winsys;
    struct r300_screen *rscreen = r300_screen(screen);
    struct r300_winsys_buffer *buffer;
    enum r300_buffer_tiling microtile, macrotile;
    unsigned stride, size;

    /* Support only 2D textures without mipmaps */
    if ((base->target != PIPE_TEXTURE_2D &&
          base->target != PIPE_TEXTURE_RECT) ||
        base->depth0 != 1 ||
        base->last_level != 0) {
        return NULL;
    }

    buffer = rws->buffer_from_handle(rws, whandle, &stride, &size);
    if (!buffer)
        return NULL;

    rws->buffer_get_tiling(rws, buffer, &microtile, &macrotile);

    /* Enforce a microtiled zbuffer. */
    if (util_format_is_depth_or_stencil(base->format) &&
        microtile == R300_BUFFER_LINEAR) {
        switch (util_format_get_blocksize(base->format)) {
            case 4:
                microtile = R300_BUFFER_TILED;
                break;

            case 2:
                if (rws->get_value(rws, R300_VID_SQUARE_TILING_SUPPORT))
                    microtile = R300_BUFFER_SQUARETILED;
                break;
        }
    }

    return (struct pipe_resource*)
           r300_texture_create_object(rscreen, base, microtile, macrotile,
                                      stride, size, buffer);
}

/* Not required to implement u_resource_vtbl, consider moving to another file:
 */
struct pipe_surface* r300_create_surface(struct pipe_context * ctx,
                                         struct pipe_resource* texture,
                                         const struct pipe_surface *surf_tmpl)
{
    struct r300_texture* tex = r300_texture(texture);
    struct r300_surface* surface = CALLOC_STRUCT(r300_surface);
    unsigned level = surf_tmpl->u.tex.level;

    assert(surf_tmpl->u.tex.first_layer == surf_tmpl->u.tex.last_layer);

    if (surface) {
        uint32_t offset, tile_height;

        pipe_reference_init(&surface->base.reference, 1);
        pipe_resource_reference(&surface->base.texture, texture);
        surface->base.context = ctx;
        surface->base.format = surf_tmpl->format;
        surface->base.width = u_minify(texture->width0, level);
        surface->base.height = u_minify(texture->height0, level);
        surface->base.usage = surf_tmpl->usage;
        surface->base.u.tex.level = level;
        surface->base.u.tex.first_layer = surf_tmpl->u.tex.first_layer;
        surface->base.u.tex.last_layer = surf_tmpl->u.tex.last_layer;

        surface->buffer = tex->buffer;
        surface->cs_buffer = tex->cs_buffer;

        /* Prefer VRAM if there are multiple domains to choose from. */
        surface->domain = tex->domain;
        if (surface->domain & R300_DOMAIN_VRAM)
            surface->domain &= ~R300_DOMAIN_GTT;

        surface->offset = r300_texture_get_offset(&tex->desc, level,
                                                  surf_tmpl->u.tex.first_layer);
        surface->pitch = tex->fb_state.pitch[level];
        surface->format = tex->fb_state.format;

        /* Parameters for the CBZB clear. */
        surface->cbzb_allowed = tex->desc.cbzb_allowed[level];
        surface->cbzb_width = align(surface->base.width, 64);

        /* Height must be aligned to the size of a tile. */
        tile_height = r300_get_pixel_alignment(tex->desc.b.b.format,
                                               tex->desc.b.b.nr_samples,
                                               tex->desc.microtile,
                                               tex->desc.macrotile[level],
                                               DIM_HEIGHT, 0);

        surface->cbzb_height = align((surface->base.height + 1) / 2,
                                     tile_height);

        /* Offset must be aligned to 2K and must point at the beginning
         * of a scanline. */
        offset = surface->offset +
                 tex->desc.stride_in_bytes[level] * surface->cbzb_height;
        surface->cbzb_midpoint_offset = offset & ~2047;

        surface->cbzb_pitch = surface->pitch & 0x1ffffc;

        if (util_format_get_blocksizebits(surface->base.format) == 32)
            surface->cbzb_format = R300_DEPTHFORMAT_24BIT_INT_Z_8BIT_STENCIL;
        else
            surface->cbzb_format = R300_DEPTHFORMAT_16BIT_INT_Z;

        DBG(r300_context(ctx), DBG_CBZB,
            "CBZB Allowed: %s, Dim: %ix%i, Misalignment: %i, Micro: %s, Macro: %s\n",
            surface->cbzb_allowed ? "YES" : " NO",
            surface->cbzb_width, surface->cbzb_height,
            offset & 2047,
            tex->desc.microtile ? "YES" : " NO",
            tex->desc.macrotile[level] ? "YES" : " NO");
    }

    return &surface->base;
}

/* Not required to implement u_resource_vtbl, consider moving to another file:
 */
void r300_surface_destroy(struct pipe_context *ctx, struct pipe_surface* s)
{
    pipe_resource_reference(&s->texture, NULL);
    FREE(s);
}
