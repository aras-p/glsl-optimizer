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
#include "r300_texture.h"
#include "r300_screen.h"
#include "r300_state_inlines.h"

#include "radeon_winsys.h"

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
static uint32_t r300_translate_texformat(enum pipe_format format)
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
    const uint32_t swizzle[4] = {
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

    desc = util_format_description(format);

    /* Colorspace (return non-RGB formats directly). */
    switch (desc->colorspace) {
        /* Depth stencil formats. */
        case UTIL_FORMAT_COLORSPACE_ZS:
            switch (format) {
                case PIPE_FORMAT_Z16_UNORM:
                    return R300_EASY_TX_FORMAT(X, X, X, X, X16);
                case PIPE_FORMAT_X8Z24_UNORM:
                case PIPE_FORMAT_S8Z24_UNORM:
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

        default:;
    }

    /* Add swizzle. */
    for (i = 0; i < 4; i++) {
        switch (desc->swizzle[i]) {
            case UTIL_FORMAT_SWIZZLE_X:
            case UTIL_FORMAT_SWIZZLE_NONE:
                result |= swizzle[0] << swizzle_shift[i];
                break;
            case UTIL_FORMAT_SWIZZLE_Y:
                result |= swizzle[1] << swizzle_shift[i];
                break;
            case UTIL_FORMAT_SWIZZLE_Z:
                result |= swizzle[2] << swizzle_shift[i];
                break;
            case UTIL_FORMAT_SWIZZLE_W:
                result |= swizzle[3] << swizzle_shift[i];
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
    if (desc->layout == UTIL_FORMAT_LAYOUT_COMPRESSED) {
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
        case PIPE_FORMAT_L8_SRGB:
        case PIPE_FORMAT_R8_UNORM:
        case PIPE_FORMAT_R8_SNORM:
            return R300_COLOR_FORMAT_I8;

        /* 16-bit buffers. */
        case PIPE_FORMAT_B5G6R5_UNORM:
            return R300_COLOR_FORMAT_RGB565;
        case PIPE_FORMAT_B5G5R5A1_UNORM:
            return R300_COLOR_FORMAT_ARGB1555;
        case PIPE_FORMAT_B4G4R4A4_UNORM:
            return R300_COLOR_FORMAT_ARGB4444;

        /* 32-bit buffers. */
        case PIPE_FORMAT_B8G8R8A8_UNORM:
        case PIPE_FORMAT_B8G8R8A8_SRGB:
        case PIPE_FORMAT_B8G8R8X8_UNORM:
        case PIPE_FORMAT_B8G8R8X8_SRGB:
        case PIPE_FORMAT_A8R8G8B8_UNORM:
        case PIPE_FORMAT_A8R8G8B8_SRGB:
        case PIPE_FORMAT_X8R8G8B8_UNORM:
        case PIPE_FORMAT_X8R8G8B8_SRGB:
        case PIPE_FORMAT_A8B8G8R8_UNORM:
        case PIPE_FORMAT_R8G8B8A8_SNORM:
        case PIPE_FORMAT_A8B8G8R8_SRGB:
        case PIPE_FORMAT_X8B8G8R8_UNORM:
        case PIPE_FORMAT_X8B8G8R8_SRGB:
        case PIPE_FORMAT_R8SG8SB8UX8U_NORM:
            return R300_COLOR_FORMAT_ARGB8888;
        case PIPE_FORMAT_R10G10B10A2_UNORM:
            return R500_COLOR_FORMAT_ARGB2101010;  /* R5xx-only? */

        /* 64-bit buffers. */
        case PIPE_FORMAT_R16G16B16A16_UNORM:
        case PIPE_FORMAT_R16G16B16A16_SNORM:
        //case PIPE_FORMAT_R16G16B16A16_FLOAT: /* not in pipe_format */
            return R300_COLOR_FORMAT_ARGB16161616;

/* XXX Enable float textures here. */
#if 0
        /* 128-bit buffers. */
        case PIPE_FORMAT_R32G32B32A32_FLOAT:
            return R300_COLOR_FORMAT_ARGB32323232;
#endif

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
        case PIPE_FORMAT_S8Z24_UNORM:
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
    if (desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB) {
        /* The gamma correction causes precision loss so we need
         * higher precision to maintain reasonable quality.
         * It has nothing to do with the colorbuffer format. */
        modifier |= R300_US_OUT_FMT_C4_10_GAMMA;
    } else if (desc->channel[0].type == UTIL_FORMAT_TYPE_FLOAT) {
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
        case PIPE_FORMAT_L8_SRGB:
        case PIPE_FORMAT_R8_UNORM:
        case PIPE_FORMAT_R8_SNORM:
            return modifier | R300_C2_SEL_R;

        /* ARGB 32-bit outputs. */
        case PIPE_FORMAT_B5G6R5_UNORM:
        case PIPE_FORMAT_B5G5R5A1_UNORM:
        case PIPE_FORMAT_B4G4R4A4_UNORM:
        case PIPE_FORMAT_B8G8R8A8_UNORM:
        case PIPE_FORMAT_B8G8R8A8_SRGB:
        case PIPE_FORMAT_B8G8R8X8_UNORM:
        case PIPE_FORMAT_B8G8R8X8_SRGB:
            return modifier |
                R300_C0_SEL_B | R300_C1_SEL_G |
                R300_C2_SEL_R | R300_C3_SEL_A;

        /* BGRA 32-bit outputs. */
        case PIPE_FORMAT_A8R8G8B8_UNORM:
        case PIPE_FORMAT_A8R8G8B8_SRGB:
        case PIPE_FORMAT_X8R8G8B8_UNORM:
        case PIPE_FORMAT_X8R8G8B8_SRGB:
            return modifier |
                R300_C0_SEL_A | R300_C1_SEL_R |
                R300_C2_SEL_G | R300_C3_SEL_B;

        /* RGBA 32-bit outputs. */
        case PIPE_FORMAT_A8B8G8R8_UNORM:
        case PIPE_FORMAT_R8G8B8A8_SNORM:
        case PIPE_FORMAT_A8B8G8R8_SRGB:
        case PIPE_FORMAT_X8B8G8R8_UNORM:
        case PIPE_FORMAT_X8B8G8R8_SRGB:
            return modifier |
                R300_C0_SEL_A | R300_C1_SEL_B |
                R300_C2_SEL_G | R300_C3_SEL_R;

        /* ABGR 32-bit outputs. */
        case PIPE_FORMAT_R8SG8SB8UX8U_NORM:
        case PIPE_FORMAT_R10G10B10A2_UNORM:
        /* RGBA high precision outputs (same swizzles as ABGR low precision) */
        case PIPE_FORMAT_R16G16B16A16_UNORM:
        case PIPE_FORMAT_R16G16B16A16_SNORM:
        //case PIPE_FORMAT_R16G16B16A16_FLOAT: /* not in pipe_format */
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
    return r300_translate_texformat(format) != ~0;
}

static void r300_setup_texture_state(struct r300_screen* screen, struct r300_texture* tex)
{
    struct r300_texture_format_state* state = &tex->state;
    struct pipe_texture *pt = &tex->tex;
    unsigned i;
    boolean is_r500 = screen->caps->is_r500;

    /* Set sampler state. */
    state->format0 = R300_TX_WIDTH((pt->width0 - 1) & 0x7ff) |
                     R300_TX_HEIGHT((pt->height0 - 1) & 0x7ff);

    if (tex->is_npot) {
        /* rectangles love this */
        state->format0 |= R300_TX_PITCH_EN;
        state->format2 = (tex->pitch[0] - 1) & 0x1fff;
    } else {
        /* power of two textures (3D, mipmaps, and no pitch) */
        state->format0 |= R300_TX_DEPTH(util_logbase2(pt->depth0) & 0xf);
    }

    state->format1 = r300_translate_texformat(pt->format);
    if (pt->target == PIPE_TEXTURE_CUBE) {
        state->format1 |= R300_TX_FORMAT_CUBIC_MAP;
    }
    if (pt->target == PIPE_TEXTURE_3D) {
        state->format1 |= R300_TX_FORMAT_3D;
    }

    /* large textures on r500 */
    if (is_r500)
    {
        if (pt->width0 > 2048) {
            state->format2 |= R500_TXWIDTH_BIT11;
        }
        if (pt->height0 > 2048) {
            state->format2 |= R500_TXHEIGHT_BIT11;
        }
    }

    SCREEN_DBG(screen, DBG_TEX, "r300: Set texture state (%dx%d, %d levels)\n",
               pt->width0, pt->height0, pt->last_level);

    /* Set framebuffer state. */
    if (util_format_is_depth_or_stencil(tex->tex.format)) {
        for (i = 0; i <= tex->tex.last_level; i++) {
            tex->fb_state.depthpitch[i] =
                tex->pitch[i] |
                R300_DEPTHMACROTILE(tex->mip_macrotile[i]) |
                R300_DEPTHMICROTILE(tex->microtile);
        }
        tex->fb_state.zb_format = r300_translate_zsformat(tex->tex.format);
    } else {
        for (i = 0; i <= tex->tex.last_level; i++) {
            tex->fb_state.colorpitch[i] =
                tex->pitch[i] |
                r300_translate_colorformat(tex->tex.format) |
                R300_COLOR_TILE(tex->mip_macrotile[i]) |
                R300_COLOR_MICROTILE(tex->microtile);
        }
        tex->fb_state.us_out_fmt = r300_translate_out_fmt(tex->tex.format);
    }
}

void r300_texture_reinterpret_format(struct pipe_screen *screen,
                                     struct pipe_texture *tex,
                                     enum pipe_format new_format)
{
    struct r300_screen *r300screen = r300_screen(screen);

    SCREEN_DBG(r300screen, DBG_TEX, "r300: Reinterpreting format: %s -> %s\n",
               util_format_name(tex->format), util_format_name(new_format));

    tex->format = new_format;

    r300_setup_texture_state(r300_screen(screen), (struct r300_texture*)tex);
}

unsigned r300_texture_get_offset(struct r300_texture* tex, unsigned level,
                                 unsigned zslice, unsigned face)
{
    unsigned offset = tex->offset[level];

    switch (tex->tex.target) {
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

    pixsize = util_format_get_blocksize(tex->tex.format);
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
                                         boolean rv350_mode)
{
    unsigned tile_width, width;

    tile_width = r300_texture_get_tile_size(tex, TILE_WIDTH, TRUE);
    width = u_minify(tex->tex.width0, level);

    /* See TX_FILTER1_n.MACRO_SWITCH. */
    if (rv350_mode) {
        return width >= tile_width;
    } else {
        return width > tile_width;
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
    if (level > tex->tex.last_level) {
        SCREEN_DBG(screen, DBG_TEX, "%s: level (%u) > last_level (%u)\n",
                   __FUNCTION__, level, tex->tex.last_level);
        return 0;
    }

    width = u_minify(tex->tex.width0, level);

    if (!util_format_is_compressed(tex->tex.format)) {
        tile_width = r300_texture_get_tile_size(tex, TILE_WIDTH,
                                                tex->mip_macrotile[level]);
        width = align(width, tile_width);

        return util_format_get_stride(tex->tex.format, width);
    } else {
        return align(util_format_get_stride(tex->tex.format, width), 32);
    }
}

static unsigned r300_texture_get_nblocksy(struct r300_texture* tex,
                                          unsigned level)
{
    unsigned height, tile_height;

    height = u_minify(tex->tex.height0, level);

    if (!util_format_is_compressed(tex->tex.format)) {
        tile_height = r300_texture_get_tile_size(tex, TILE_HEIGHT,
                                                 tex->mip_macrotile[level]);
        height = align(height, tile_height);
    }

    return util_format_get_nblocksy(tex->tex.format, height);
}

static void r300_setup_miptree(struct r300_screen* screen,
                               struct r300_texture* tex)
{
    struct pipe_texture* base = &tex->tex;
    unsigned stride, size, layer_size, nblocksy, i;
    boolean rv350_mode = screen->caps->family >= CHIP_FAMILY_RV350;

    SCREEN_DBG(screen, DBG_TEX, "r300: Making miptree for texture, format %s\n",
               util_format_name(base->format));

    for (i = 0; i <= base->last_level; i++) {
        /* Let's see if this miplevel can be macrotiled. */
        tex->mip_macrotile[i] = (tex->macrotile == R300_BUFFER_TILED &&
                                 r300_texture_macro_switch(tex, i, rv350_mode)) ?
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

        SCREEN_DBG(screen, DBG_TEX, "r300: Texture miptree: Level %d "
                "(%dx%dx%d px, pitch %d bytes) %d bytes total, macrotiled %s\n",
                i, u_minify(base->width0, i), u_minify(base->height0, i),
                u_minify(base->depth0, i), stride, tex->size,
                tex->mip_macrotile[i] ? "TRUE" : "FALSE");
    }
}

static void r300_setup_flags(struct r300_texture* tex)
{
    tex->is_npot = !util_is_power_of_two(tex->tex.width0) ||
                   !util_is_power_of_two(tex->tex.height0);
}

/* Create a new texture. */
static struct pipe_texture*
    r300_texture_create(struct pipe_screen* screen,
                        const struct pipe_texture* template)
{
    struct r300_texture* tex = CALLOC_STRUCT(r300_texture);
    struct r300_screen* rscreen = r300_screen(screen);
    struct radeon_winsys* winsys = (struct radeon_winsys*)screen->winsys;

    if (!tex) {
        return NULL;
    }

    tex->tex = *template;
    pipe_reference_init(&tex->tex.reference, 1);
    tex->tex.screen = screen;

    r300_setup_flags(tex);
    r300_setup_miptree(rscreen, tex);
    r300_setup_texture_state(rscreen, tex);

    tex->buffer = screen->buffer_create(screen, 2048,
                                        PIPE_BUFFER_USAGE_PIXEL,
                                        tex->size);
    winsys->buffer_set_tiling(winsys, tex->buffer,
                              tex->pitch[0],
                              tex->microtile != R300_BUFFER_LINEAR,
                              tex->macrotile != R300_BUFFER_LINEAR);

    if (!tex->buffer) {
        FREE(tex);
        return NULL;
    }

    return (struct pipe_texture*)tex;
}

static void r300_texture_destroy(struct pipe_texture* texture)
{
    struct r300_texture* tex = (struct r300_texture*)texture;

    pipe_buffer_reference(&tex->buffer, NULL);

    FREE(tex);
}

static struct pipe_surface* r300_get_tex_surface(struct pipe_screen* screen,
                                                 struct pipe_texture* texture,
                                                 unsigned face,
                                                 unsigned level,
                                                 unsigned zslice,
                                                 unsigned flags)
{
    struct r300_texture* tex = (struct r300_texture*)texture;
    struct pipe_surface* surface = CALLOC_STRUCT(pipe_surface);
    unsigned offset;

    offset = r300_texture_get_offset(tex, level, zslice, face);

    if (surface) {
        pipe_reference_init(&surface->reference, 1);
        pipe_texture_reference(&surface->texture, texture);
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

static void r300_tex_surface_destroy(struct pipe_surface* s)
{
    pipe_texture_reference(&s->texture, NULL);
    FREE(s);
}

static struct pipe_texture*
    r300_texture_blanket(struct pipe_screen* screen,
                         const struct pipe_texture* base,
                         const unsigned* stride,
                         struct pipe_buffer* buffer)
{
    struct r300_texture* tex;
    struct r300_screen* rscreen = r300_screen(screen);

    /* Support only 2D textures without mipmaps */
    if (base->target != PIPE_TEXTURE_2D ||
        base->depth0 != 1 ||
        base->last_level != 0) {
        return NULL;
    }

    tex = CALLOC_STRUCT(r300_texture);
    if (!tex) {
        return NULL;
    }

    tex->tex = *base;
    pipe_reference_init(&tex->tex.reference, 1);
    tex->tex.screen = screen;

    tex->stride_override = *stride;
    tex->pitch[0] = *stride / util_format_get_blocksize(base->format);

    r300_setup_flags(tex);
    r300_setup_texture_state(rscreen, tex);

    pipe_buffer_reference(&tex->buffer, buffer);

    return (struct pipe_texture*)tex;
}

static struct pipe_video_surface *
r300_video_surface_create(struct pipe_screen *screen,
                          enum pipe_video_chroma_format chroma_format,
                          unsigned width, unsigned height)
{
    struct r300_video_surface *r300_vsfc;
    struct pipe_texture template;

    assert(screen);
    assert(width && height);

    r300_vsfc = CALLOC_STRUCT(r300_video_surface);
    if (!r300_vsfc)
       return NULL;

    pipe_reference_init(&r300_vsfc->base.reference, 1);
    r300_vsfc->base.screen = screen;
    r300_vsfc->base.chroma_format = chroma_format;
    r300_vsfc->base.width = width;
    r300_vsfc->base.height = height;

    memset(&template, 0, sizeof(struct pipe_texture));
    template.target = PIPE_TEXTURE_2D;
    template.format = PIPE_FORMAT_B8G8R8X8_UNORM;
    template.last_level = 0;
    template.width0 = util_next_power_of_two(width);
    template.height0 = util_next_power_of_two(height);
    template.depth0 = 1;
    template.tex_usage = PIPE_TEXTURE_USAGE_SAMPLER |
                         PIPE_TEXTURE_USAGE_RENDER_TARGET;

    r300_vsfc->tex = screen->texture_create(screen, &template);
    if (!r300_vsfc->tex)
    {
        FREE(r300_vsfc);
        return NULL;
    }

    return &r300_vsfc->base;
}

static void r300_video_surface_destroy(struct pipe_video_surface *vsfc)
{
    struct r300_video_surface *r300_vsfc = r300_video_surface(vsfc);
    pipe_texture_reference(&r300_vsfc->tex, NULL);
    FREE(r300_vsfc);
}

void r300_init_screen_texture_functions(struct pipe_screen* screen)
{
    screen->texture_create = r300_texture_create;
    screen->texture_destroy = r300_texture_destroy;
    screen->get_tex_surface = r300_get_tex_surface;
    screen->tex_surface_destroy = r300_tex_surface_destroy;
    screen->texture_blanket = r300_texture_blanket;

    screen->video_surface_create = r300_video_surface_create;
    screen->video_surface_destroy= r300_video_surface_destroy;
}

boolean r300_get_texture_buffer(struct pipe_screen* screen,
                                struct pipe_texture* texture,
                                struct pipe_buffer** buffer,
                                unsigned* stride)
{
    struct r300_texture* tex = (struct r300_texture*)texture;
    if (!tex) {
        return FALSE;
    }

    pipe_buffer_reference(buffer, tex->buffer);

    if (stride) {
        *stride = r300_texture_get_stride(r300_screen(screen), tex, 0);
    }

    return TRUE;
}
