/*
 * Copyright 2009 Joakim Sindholt <opensource@zhasha.com>
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

#include "pipe/p_format.h"

#include "r300_reg.h"

static INLINE uint32_t r300_translate_colorformat(enum pipe_format format)
{
    switch (format) {
    case PIPE_FORMAT_A8R8G8B8_UNORM:
        return R300_COLOR_FORMAT_ARGB8888;
    case PIPE_FORMAT_I8_UNORM:
        return R300_COLOR_FORMAT_I8;
    case PIPE_FORMAT_A1R5G5B5_UNORM:
        return R300_COLOR_FORMAT_ARGB1555;
    case PIPE_FORMAT_R5G6B5_UNORM:
        return R300_COLOR_FORMAT_RGB565;
    /* XXX Not in pipe_format
    case PIPE_FORMAT_A32R32G32B32:
        return R300_COLOR_FORMAT_ARGB32323232;
    case PIPE_FORMAT_A16R16G16B16:
        return R300_COLOR_FORMAT_ARGB16161616; */
    case PIPE_FORMAT_A4R4G4B4_UNORM:
        return R300_COLOR_FORMAT_ARGB4444;
    /* XXX Not in pipe_format
    case PIPE_FORMAT_A10R10G10B10_UNORM:
        return R500_COLOR_FORMAT_ARGB10101010;
    case PIPE_FORMAT_A2R10G10B10_UNORM:
        return R500_COLOR_FORMAT_ARGB2101010;
    case PIPE_FORMAT_I10_UNORM:
        return R500_COLOR_FORMAT_I10; */
    default:
        debug_printf("r300: Implementation error: " \
            "Got unsupported color format %s in %s\n",
            pf_name(format), __FUNCTION__);
        break;
    }
    
    return 0;
}

static INLINE uint32_t r300_translate_zsformat(enum pipe_format format)
{
    switch (format) {
    case PIPE_FORMAT_Z16_UNORM:
        return R300_DEPTHFORMAT_16BIT_INT_Z;
    /* XXX R300_DEPTHFORMAT_16BIT_13E3 anyone? */
    case PIPE_FORMAT_Z24S8_UNORM:
        return R300_DEPTHFORMAT_24BIT_INT_Z_8BIT_STENCIL;
    default:
        debug_printf("r300: Implementation error: " \
            "Got unsupported ZS format %s in %s\n",
            pf_name(format), __FUNCTION__);
        break;
    }
    
    return 0;
}

#endif /* R300_STATE_INLINES_H */
