/*
 * Copyright 2009 Joakim Sindholt <opensource@zhasha.com>
 *                Corbin Simpson <MostAwesomeDude@gmail.com>
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

#ifndef R300_STATE_INLINES_H
#define R300_STATE_INLINES_H

#include "draw/draw_vertex.h"

#include "pipe/p_format.h"

#include "util/u_format.h"

#include "r300_reg.h"

/* Some maths. These should probably find their way to u_math, if needed. */

static INLINE int pack_float_16_6x(float f) {
    return ((int)(f * 6.0) & 0xffff);
}

/* Blend state. */

static INLINE uint32_t r300_translate_blend_function(int blend_func)
{
    switch (blend_func) {
        case PIPE_BLEND_ADD:
            return R300_COMB_FCN_ADD_CLAMP;
        case PIPE_BLEND_SUBTRACT:
            return R300_COMB_FCN_SUB_CLAMP;
        case PIPE_BLEND_REVERSE_SUBTRACT:
            return R300_COMB_FCN_RSUB_CLAMP;
        case PIPE_BLEND_MIN:
            return R300_COMB_FCN_MIN;
        case PIPE_BLEND_MAX:
            return R300_COMB_FCN_MAX;
        default:
            debug_printf("r300: Unknown blend function %d\n", blend_func);
            assert(0);
            break;
    }
    return 0;
}

/* XXX we can also offer the D3D versions of some of these... */
static INLINE uint32_t r300_translate_blend_factor(int blend_fact)
{
    switch (blend_fact) {
        case PIPE_BLENDFACTOR_ONE:
            return R300_BLEND_GL_ONE;
        case PIPE_BLENDFACTOR_SRC_COLOR:
            return R300_BLEND_GL_SRC_COLOR;
        case PIPE_BLENDFACTOR_SRC_ALPHA:
            return R300_BLEND_GL_SRC_ALPHA;
        case PIPE_BLENDFACTOR_DST_ALPHA:
            return R300_BLEND_GL_DST_ALPHA;
        case PIPE_BLENDFACTOR_DST_COLOR:
            return R300_BLEND_GL_DST_COLOR;
        case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
            return R300_BLEND_GL_SRC_ALPHA_SATURATE;
        case PIPE_BLENDFACTOR_CONST_COLOR:
            return R300_BLEND_GL_CONST_COLOR;
        case PIPE_BLENDFACTOR_CONST_ALPHA:
            return R300_BLEND_GL_CONST_ALPHA;
        case PIPE_BLENDFACTOR_ZERO:
            return R300_BLEND_GL_ZERO;
        case PIPE_BLENDFACTOR_INV_SRC_COLOR:
            return R300_BLEND_GL_ONE_MINUS_SRC_COLOR;
        case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
            return R300_BLEND_GL_ONE_MINUS_SRC_ALPHA;
        case PIPE_BLENDFACTOR_INV_DST_ALPHA:
            return R300_BLEND_GL_ONE_MINUS_DST_ALPHA;
        case PIPE_BLENDFACTOR_INV_DST_COLOR:
            return R300_BLEND_GL_ONE_MINUS_DST_COLOR;
        case PIPE_BLENDFACTOR_INV_CONST_COLOR:
            return R300_BLEND_GL_ONE_MINUS_CONST_COLOR;
        case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
            return R300_BLEND_GL_ONE_MINUS_CONST_ALPHA;

        case PIPE_BLENDFACTOR_SRC1_COLOR:
        case PIPE_BLENDFACTOR_SRC1_ALPHA:
        case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
        case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:
            debug_printf("r300: Implementation error: "
                "Bad blend factor %d not supported!\n", blend_fact);
            assert(0);
            break;

        default:
            debug_printf("r300: Unknown blend factor %d\n", blend_fact);
            assert(0);
            break;
    }
    return 0;
}

/* DSA state. */

static INLINE uint32_t r300_translate_depth_stencil_function(int zs_func)
{
    switch (zs_func) {
        case PIPE_FUNC_NEVER:
            return R300_ZS_NEVER;
        case PIPE_FUNC_LESS:
            return R300_ZS_LESS;
        case PIPE_FUNC_EQUAL:
            return R300_ZS_EQUAL;
        case PIPE_FUNC_LEQUAL:
            return R300_ZS_LEQUAL;
        case PIPE_FUNC_GREATER:
            return R300_ZS_GREATER;
        case PIPE_FUNC_NOTEQUAL:
            return R300_ZS_NOTEQUAL;
        case PIPE_FUNC_GEQUAL:
            return R300_ZS_GEQUAL;
        case PIPE_FUNC_ALWAYS:
            return R300_ZS_ALWAYS;
        default:
            debug_printf("r300: Unknown depth/stencil function %d\n",
                zs_func);
            assert(0);
            break;
    }
    return 0;
}

static INLINE uint32_t r300_translate_stencil_op(int s_op)
{
    switch (s_op) {
        case PIPE_STENCIL_OP_KEEP:
            return R300_ZS_KEEP;
        case PIPE_STENCIL_OP_ZERO:
            return R300_ZS_ZERO;
        case PIPE_STENCIL_OP_REPLACE:
            return R300_ZS_REPLACE;
        case PIPE_STENCIL_OP_INCR:
            return R300_ZS_INCR;
        case PIPE_STENCIL_OP_DECR:
            return R300_ZS_DECR;
        case PIPE_STENCIL_OP_INCR_WRAP:
            return R300_ZS_INCR_WRAP;
        case PIPE_STENCIL_OP_DECR_WRAP:
            return R300_ZS_DECR_WRAP;
        case PIPE_STENCIL_OP_INVERT:
            return R300_ZS_INVERT;
        default:
            debug_printf("r300: Unknown stencil op %d", s_op);
            assert(0);
            break;
    }
    return 0;
}

static INLINE uint32_t r300_translate_alpha_function(int alpha_func)
{
    switch (alpha_func) {
        case PIPE_FUNC_NEVER:
            return R300_FG_ALPHA_FUNC_NEVER;
        case PIPE_FUNC_LESS:
            return R300_FG_ALPHA_FUNC_LESS;
        case PIPE_FUNC_EQUAL:
            return R300_FG_ALPHA_FUNC_EQUAL;
        case PIPE_FUNC_LEQUAL:
            return R300_FG_ALPHA_FUNC_LE;
        case PIPE_FUNC_GREATER:
            return R300_FG_ALPHA_FUNC_GREATER;
        case PIPE_FUNC_NOTEQUAL:
            return R300_FG_ALPHA_FUNC_NOTEQUAL;
        case PIPE_FUNC_GEQUAL:
            return R300_FG_ALPHA_FUNC_GE;
        case PIPE_FUNC_ALWAYS:
            return R300_FG_ALPHA_FUNC_ALWAYS;
        default:
            debug_printf("r300: Unknown alpha function %d", alpha_func);
            assert(0);
            break;
    }
    return 0;
}

static INLINE uint32_t
r300_translate_polygon_mode_front(unsigned mode) {
    switch (mode)
    {
        case PIPE_POLYGON_MODE_FILL:
            return R300_GA_POLY_MODE_FRONT_PTYPE_TRI;
        case PIPE_POLYGON_MODE_LINE:
            return R300_GA_POLY_MODE_FRONT_PTYPE_LINE;
        case PIPE_POLYGON_MODE_POINT:
            return R300_GA_POLY_MODE_FRONT_PTYPE_POINT;

        default:
            debug_printf("r300: Bad polygon mode %i in %s\n", mode,
                __FUNCTION__);
            return R300_GA_POLY_MODE_FRONT_PTYPE_TRI;
    }
}

static INLINE uint32_t
r300_translate_polygon_mode_back(unsigned mode) {
    switch (mode)
    {
        case PIPE_POLYGON_MODE_FILL:
            return R300_GA_POLY_MODE_BACK_PTYPE_TRI;
        case PIPE_POLYGON_MODE_LINE:
            return R300_GA_POLY_MODE_BACK_PTYPE_LINE;
        case PIPE_POLYGON_MODE_POINT:
            return R300_GA_POLY_MODE_BACK_PTYPE_POINT;

        default:
            debug_printf("r300: Bad polygon mode %i in %s\n", mode,
                __FUNCTION__);
            return R300_GA_POLY_MODE_BACK_PTYPE_TRI;
    }
}

/* Texture sampler state. */

static INLINE uint32_t r300_translate_wrap(int wrap)
{
    switch (wrap) {
        case PIPE_TEX_WRAP_REPEAT:
            return R300_TX_REPEAT;
        case PIPE_TEX_WRAP_CLAMP:
            return R300_TX_CLAMP;
        case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
            return R300_TX_CLAMP_TO_EDGE;
        case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
            return R300_TX_CLAMP_TO_BORDER;
        case PIPE_TEX_WRAP_MIRROR_REPEAT:
            return R300_TX_REPEAT | R300_TX_MIRRORED;
        case PIPE_TEX_WRAP_MIRROR_CLAMP:
            return R300_TX_CLAMP | R300_TX_MIRRORED;
        case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_EDGE:
            return R300_TX_CLAMP_TO_EDGE | R300_TX_MIRRORED;
        case PIPE_TEX_WRAP_MIRROR_CLAMP_TO_BORDER:
            return R300_TX_CLAMP_TO_EDGE | R300_TX_MIRRORED;
        default:
            debug_printf("r300: Unknown texture wrap %d", wrap);
            assert(0);
            return 0;
    }
}

static INLINE uint32_t r300_translate_tex_filters(int min, int mag, int mip,
                                                  int is_anisotropic)
{
    uint32_t retval = 0;
    if (is_anisotropic)
        retval |= R300_TX_MIN_FILTER_ANISO | R300_TX_MAG_FILTER_ANISO;
    else {
        switch (min) {
        case PIPE_TEX_FILTER_NEAREST:
            retval |= R300_TX_MIN_FILTER_NEAREST;
            break;
        case PIPE_TEX_FILTER_LINEAR:
            retval |= R300_TX_MIN_FILTER_LINEAR;
            break;
        default:
            debug_printf("r300: Unknown texture filter %d\n", min);
            assert(0);
            break;
        }
        switch (mag) {
        case PIPE_TEX_FILTER_NEAREST:
            retval |= R300_TX_MAG_FILTER_NEAREST;
            break;
        case PIPE_TEX_FILTER_LINEAR:
            retval |= R300_TX_MAG_FILTER_LINEAR;
            break;
        default:
            debug_printf("r300: Unknown texture filter %d\n", mag);
            assert(0);
            break;
        }
    }
    switch (mip) {
        case PIPE_TEX_MIPFILTER_NONE:
            retval |= R300_TX_MIN_FILTER_MIP_NONE;
            break;
        case PIPE_TEX_MIPFILTER_NEAREST:
            retval |= R300_TX_MIN_FILTER_MIP_NEAREST;
            break;
        case PIPE_TEX_MIPFILTER_LINEAR:
            retval |= R300_TX_MIN_FILTER_MIP_LINEAR;
            break;
        default:
            debug_printf("r300: Unknown texture filter %d\n", mip);
            assert(0);
            break;
    }

    return retval;
}

static INLINE uint32_t r300_anisotropy(unsigned max_aniso)
{
    if (max_aniso >= 16) {
        return R300_TX_MAX_ANISO_16_TO_1;
    } else if (max_aniso >= 8) {
        return R300_TX_MAX_ANISO_8_TO_1;
    } else if (max_aniso >= 4) {
        return R300_TX_MAX_ANISO_4_TO_1;
    } else if (max_aniso >= 2) {
        return R300_TX_MAX_ANISO_2_TO_1;
    } else {
        return R300_TX_MAX_ANISO_1_TO_1;
    }
}

/* Non-CSO state. (For now.) */

static INLINE uint32_t r300_translate_gb_pipes(int pipe_count)
{
    switch (pipe_count) {
        case 1:
            return R300_GB_TILE_PIPE_COUNT_RV300;
            break;
        case 2:
            return R300_GB_TILE_PIPE_COUNT_R300;
            break;
        case 3:
            return R300_GB_TILE_PIPE_COUNT_R420_3P;
            break;
        case 4:
            return R300_GB_TILE_PIPE_COUNT_R420;
            break;
    }
    return 0;
}

/* Utility function to count the number of components in RGBAZS formats.
 * XXX should go to util or p_format.h */
static INLINE unsigned pf_component_count(enum pipe_format format) {
    unsigned count = 0;

    if (util_format_get_component_bits(format, UTIL_FORMAT_COLORSPACE_RGB, 0)) {
        count++;
    }
    if (util_format_get_component_bits(format, UTIL_FORMAT_COLORSPACE_RGB, 1)) {
        count++;
    }
    if (util_format_get_component_bits(format, UTIL_FORMAT_COLORSPACE_RGB, 2)) {
        count++;
    }
    if (util_format_get_component_bits(format, UTIL_FORMAT_COLORSPACE_RGB, 3)) {
        count++;
    }
    if (util_format_get_component_bits(format, UTIL_FORMAT_COLORSPACE_ZS, 0)) {
        count++;
    }
    if (util_format_get_component_bits(format, UTIL_FORMAT_COLORSPACE_ZS, 1)) {
        count++;
    }

    return count;
}

/* Translate pipe_formats into PSC vertex types. */
static INLINE uint16_t
r300_translate_vertex_data_type(enum pipe_format format) {
    uint32_t result = 0;
    const struct util_format_description *desc;
    unsigned components = pf_component_count(format);

    desc = util_format_description(format);

    if (desc->layout != UTIL_FORMAT_LAYOUT_PLAIN) {
        debug_printf("r300: Bad format %s in %s:%d\n", util_format_name(format),
            __FUNCTION__, __LINE__);
        assert(0);
    }

    switch (desc->channel[0].type) {
        /* Half-floats, floats, doubles */
        case UTIL_FORMAT_TYPE_FLOAT:
            switch (util_format_get_component_bits(format, UTIL_FORMAT_COLORSPACE_RGB, 0)) {
                case 16:
                    /* XXX Supported only on RV350 and later. */
                    if (components > 2) {
                        result = R300_DATA_TYPE_FLT16_4;
                    } else {
                        result = R300_DATA_TYPE_FLT16_2;
                    }
                    break;
                case 32:
                    result = R300_DATA_TYPE_FLOAT_1 + (components - 1);
                    break;
                default:
                    debug_printf("r300: Bad format %s in %s:%d\n",
                        util_format_name(format), __FUNCTION__, __LINE__);
                    assert(0);
            }
            break;
        /* Unsigned ints */
        case UTIL_FORMAT_TYPE_UNSIGNED:
        /* Signed ints */
        case UTIL_FORMAT_TYPE_SIGNED:
            switch (util_format_get_component_bits(format, UTIL_FORMAT_COLORSPACE_RGB, 0)) {
                case 8:
                    result = R300_DATA_TYPE_BYTE;
                    break;
                case 16:
                    if (components > 2) {
                        result = R300_DATA_TYPE_SHORT_4;
                    } else {
                        result = R300_DATA_TYPE_SHORT_2;
                    }
                    break;
                default:
                    debug_printf("r300: Bad format %s in %s:%d\n",
                        util_format_name(format), __FUNCTION__, __LINE__);
                    debug_printf("r300: util_format_get_component_bits(format, UTIL_FORMAT_COLORSPACE_RGB, 0) == %d\n",
                        util_format_get_component_bits(format, UTIL_FORMAT_COLORSPACE_RGB, 0));
                    assert(0);
            }
            break;
        default:
            debug_printf("r300: Bad format %s in %s:%d\n",
                util_format_name(format), __FUNCTION__, __LINE__);
            assert(0);
    }

    if (desc->channel[0].type == UTIL_FORMAT_TYPE_SIGNED) {
        result |= R300_SIGNED;
    }
    if (desc->channel[0].normalized) {
        result |= R300_NORMALIZE;
    }

    return result;
}

static INLINE uint16_t
r300_translate_vertex_data_swizzle(enum pipe_format format) {
    const struct util_format_description *desc = util_format_description(format);

    assert(format);

    if (desc->layout != UTIL_FORMAT_LAYOUT_PLAIN) {
        debug_printf("r300: Bad format %s in %s:%d\n",
            util_format_name(format), __FUNCTION__, __LINE__);
        return 0;
    }

    return ((desc->swizzle[0] << R300_SWIZZLE_SELECT_X_SHIFT) |
            (desc->swizzle[1] << R300_SWIZZLE_SELECT_Y_SHIFT) |
            (desc->swizzle[2] << R300_SWIZZLE_SELECT_Z_SHIFT) |
            (desc->swizzle[3] << R300_SWIZZLE_SELECT_W_SHIFT) |
            (0xf << R300_WRITE_ENA_SHIFT));
}

#endif /* R300_STATE_INLINES_H */
