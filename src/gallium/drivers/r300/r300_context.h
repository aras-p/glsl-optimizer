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

#ifndef R300_CONTEXT_H
#define R300_CONTEXT_H

#include "draw/draw_context.h"
#include "pipe/p_context.h"
#include "util/u_memory.h"

struct r300_blend_state {
    uint32_t blend_control;       /* R300_RB3D_BLENDCNTL: 0x4e04 */
    uint32_t alpha_blend_control; /* R300_RB3D_ABLENDCNTL: 0x4e08 */
    uint32_t rop;                 /* R300_RB3D_ROPCNTL: 0x4e18 */
    uint32_t dither;              /* R300_RB3D_DITHER_CTL: 0x4e50 */
};

#define R300_NEW_BLEND 0x1

struct r300_context {
    /* Parent class */
    struct pipe_context context;

    /* The interface to the windowing system, etc. */
    struct r300_winsys* winsys;
    /* Draw module. Used mostly for SW TCL. */
    struct draw_context* draw;

    /* Various CSO state objects. */
    /* Blend state. */
    struct r300_blend_state* blend_state;

    /* Bitmask of dirty state objects. */
    uint32_t dirty_state;
};

/* Convenience cast wrapper. */
static struct r300_context* r300_context(struct pipe_context* context) {
    return (struct r300_context*)context;
}

/* Context initialization. */
void r300_init_surface_functions(struct r300_context* r300);

struct pipe_context* r300_create_context(struct pipe_screen* screen,
                                         struct pipe_winsys* winsys,
                                         struct r300_winsys* r300_winsys);

#endif /* R300_CONTEXT_H */