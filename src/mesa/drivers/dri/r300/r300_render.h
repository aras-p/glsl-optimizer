/*
 * Copyright 2009 Maciej Cencora <m.cencora@gmail.com>
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef __R300_RENDER_H__
#define __R300_RENDER_H__

#include "main/mtypes.h"

#define R300_FALLBACK_VERTEX_PROGRAM    (1 << 0)
#define R300_TCL_FALLBACK_MASK           0x0000ffff

#define R300_FALLBACK_LINE_SMOOTH       (1 << 16)
#define R300_FALLBACK_POINT_SMOOTH      (1 << 17)
#define R300_FALLBACK_POLYGON_SMOOTH    (1 << 18)
#define R300_FALLBACK_LINE_STIPPLE      (1 << 19)
#define R300_FALLBACK_POLYGON_STIPPLE   (1 << 20)
#define R300_FALLBACK_STENCIL_TWOSIDE   (1 << 21)
#define R300_FALLBACK_RENDER_MODE       (1 << 22)
#define R300_FALLBACK_FRAGMENT_PROGRAM  (1 << 23)
#define R300_FALLBACK_RADEON_COMMON     (1 << 29)
#define R300_FALLBACK_AOS_LIMIT         (1 << 30)
#define R300_FALLBACK_INVALID_BUFFERS   (1 << 31)
#define R300_RASTER_FALLBACK_MASK        0xffff0000

#define MASK_XYZW (R300_WRITE_ENA_X | R300_WRITE_ENA_Y | R300_WRITE_ENA_Z | R300_WRITE_ENA_W)
#define MASK_X R300_WRITE_ENA_X
#define MASK_Y R300_WRITE_ENA_Y
#define MASK_Z R300_WRITE_ENA_Z
#define MASK_W R300_WRITE_ENA_W

#if SWIZZLE_X != R300_INPUT_ROUTE_SELECT_X || \
    SWIZZLE_Y != R300_INPUT_ROUTE_SELECT_Y || \
    SWIZZLE_Z != R300_INPUT_ROUTE_SELECT_Z || \
    SWIZZLE_W != R300_INPUT_ROUTE_SELECT_W || \
    SWIZZLE_ZERO != R300_INPUT_ROUTE_SELECT_ZERO || \
    SWIZZLE_ONE != R300_INPUT_ROUTE_SELECT_ONE
#error Cannot change these!
#endif

extern const struct tnl_pipeline_stage _r300_render_stage;

extern void r300SwitchFallback(GLcontext *ctx, uint32_t bit, GLboolean mode);

extern void r300RunRenderPrimitive(GLcontext * ctx, int start, int end, int prim);

#endif
