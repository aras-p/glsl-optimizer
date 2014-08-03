/*
 * Copyright © 2014 Broadcom
 * Copyright (C) 2012 Rob Clark <robclark@freedesktop.org>
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

#ifndef VC4_RESOURCE_H
#define VC4_RESOURCE_H

#include "vc4_screen.h"
#include "util/u_transfer.h"

struct vc4_resource_slice {
        uint32_t offset;
        uint32_t stride;
        uint32_t size0;
};

struct vc4_surface {
        struct pipe_surface base;
        uint32_t offset;
        uint32_t stride;
        uint32_t width;
        uint16_t height;
        uint16_t depth;
};

struct vc4_resource {
        struct u_resource base;
        struct vc4_bo *bo;
        struct vc4_resource_slice slices[VC4_MAX_MIP_LEVELS];
        int cpp;
        /** One of VC4_TILING_FORMAT_* */
        uint8_t tiling;
};

static INLINE struct vc4_resource *
vc4_resource(struct pipe_resource *prsc)
{
        return (struct vc4_resource *)prsc;
}

static INLINE struct vc4_surface *
vc4_surface(struct pipe_surface *psurf)
{
        return (struct vc4_surface *)psurf;
}

void vc4_resource_screen_init(struct pipe_screen *pscreen);
void vc4_resource_context_init(struct pipe_context *pctx);

#endif /* VC4_RESOURCE_H */
