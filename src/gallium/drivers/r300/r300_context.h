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

#include "r300_screen.h"

struct r300_blend_state {
    uint32_t blend_control;       /* R300_RB3D_CBLEND: 0x4e04 */
    uint32_t alpha_blend_control; /* R300_RB3D_ABLEND: 0x4e08 */
    uint32_t rop;                 /* R300_RB3D_ROPCNTL: 0x4e18 */
    uint32_t dither;              /* R300_RB3D_DITHER_CTL: 0x4e50 */
};

struct r300_blend_color_state {
    /* RV515 and earlier */
    uint32_t blend_color;            /* R300_RB3D_BLEND_COLOR: 0x4e10 */
    /* R520 and newer */
    uint32_t blend_color_red_alpha;  /* R500_RB3D_CONSTANT_COLOR_AR: 0x4ef8 */
    uint32_t blend_color_green_blue; /* R500_RB3D_CONSTANT_COLOR_GB: 0x4efc */
};

struct r300_dsa_state {
    uint32_t alpha_function;    /* R300_FG_ALPHA_FUNC: 0x4bd4 */
    uint32_t alpha_reference;   /* R500_FG_ALPHA_VALUE: 0x4be0 */
    uint32_t z_buffer_control;  /* R300_ZB_CNTL: 0x4f00 */
    uint32_t z_stencil_control; /* R300_ZB_ZSTENCILCNTL: 0x4f04 */
    uint32_t stencil_ref_mask;  /* R300_ZB_STENCILREFMASK: 0x4f08 */
    uint32_t z_buffer_top;      /* R300_ZB_ZTOP: 0x4f14 */
    uint32_t stencil_ref_bf;    /* R500_ZB_STENCILREFMASK_BF: 0x4fd4 */
};

struct r300_rs_state {
    uint32_t vap_control_status;    /* R300_VAP_CNTL_STATUS: 0x2140 */
    uint32_t depth_scale_front;     /* R300_SU_POLY_OFFSET_FRONT_SCALE: 0x42a4 */
    uint32_t depth_offset_front;    /* R300_SU_POLY_OFFSET_FRONT_OFFSET: 0x42a8 */
    uint32_t depth_scale_back;      /* R300_SU_POLY_OFFSET_BACK_SCALE: 0x42ac */
    uint32_t depth_offset_back;     /* R300_SU_POLY_OFFSET_BACK_OFFSET: 0x42b0 */
    uint32_t polygon_offset_enable; /* R300_SU_POLY_OFFSET_ENABLE: 0x42b4 */
    uint32_t cull_mode;             /* R300_SU_CULL_MODE: 0x42b8 */
};

struct r300_scissor_state {
    uint32_t scissor_top_left;     /* R300_SC_SCISSORS_TL: 0x43e0 */
    uint32_t scissor_bottom_right; /* R300_SC_SCISSORS_BR: 0x43e4 */
};

#define R300_NEW_BLEND          0x01
#define R300_NEW_BLEND_COLOR    0x02
#define R300_NEW_DSA            0x04
#define R300_NEW_RS             0x08
#define R300_NEW_SCISSOR        0x10
#define R300_NEW_KITCHEN_SINK   0x1f

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
    /* Blend color state. */
    struct r300_blend_color_state* blend_color_state;
    /* Depth, stencil, and alpha state. */
    struct r300_dsa_state* dsa_state;
    /* Rasterizer state. */
    struct r300_rs_state* rs_state;
    /* Scissor state. */
    struct r300_scissor_state* scissor_state;

    /* Bitmask of dirty state objects. */
    uint32_t dirty_state;
    /* Flag indicating whether or not the HW is dirty. */
    uint32_t dirty_hw;
};

/* Convenience cast wrapper. */
static struct r300_context* r300_context(struct pipe_context* context) {
    return (struct r300_context*)context;
}

/* Context initialization. */
void r300_init_state_functions(struct r300_context* r300);
void r300_init_surface_functions(struct r300_context* r300);

struct pipe_context* r300_create_context(struct pipe_screen* screen,
                                         struct pipe_winsys* winsys,
                                         struct r300_winsys* r300_winsys);

#endif /* R300_CONTEXT_H */
