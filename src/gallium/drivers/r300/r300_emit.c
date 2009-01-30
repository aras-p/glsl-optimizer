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

/* r300_emit: Functions for emitting state. */

#include "r300_emit.h"

void r300_emit_blend_state(struct r300_context* r300,
                           struct r300_blend_state* blend)
{
    CS_LOCALS(r300);
    BEGIN_CS(7);
    OUT_CS_REG_SEQ(R300_RB3D_CBLEND, 2);
    OUT_CS(blend->blend_control);
    OUT_CS(blend->alpha_blend_control);
    OUT_CS_REG(R300_RB3D_ROPCNTL, blend->rop);
    OUT_CS_REG(R300_RB3D_DITHER_CTL, blend->dither);
    END_CS;
}

void r300_emit_blend_color_state(struct r300_context* r300,
                                 struct r300_blend_color_state* bc)
{
    struct r300_screen* r300screen =
        (struct r300_screen*)r300->context.screen;
    CS_LOCALS(r300);
    if (r300screen->caps->is_r500) {
        BEGIN_CS(3);
        OUT_CS_REG_SEQ(R500_RB3D_CONSTANT_COLOR_AR, 2);
        OUT_CS(bc->blend_color_red_alpha);
        OUT_CS(bc->blend_color_green_blue);
        END_CS;
    } else {
        BEGIN_CS(2);
        OUT_CS_REG(R300_RB3D_BLEND_COLOR, bc->blend_color);
        END_CS;
    }
}

void r300_emit_dsa_state(struct r300_context* r300,
                           struct r300_dsa_state* dsa)
{
    struct r300_screen* r300screen =
        (struct r300_screen*)r300->context.screen;
    CS_LOCALS(r300);
    BEGIN_CS(r300screen->caps->is_r500 ? 8 : 8);
    OUT_CS_REG(R300_FG_ALPHA_FUNC, dsa->alpha_function);
    /* XXX figure out the r300 counterpart for this */
    if (r300screen->caps->is_r500) {
        /* OUT_CS_REG(R500_FG_ALPHA_VALUE, dsa->alpha_reference); */
    }
    OUT_CS_REG_SEQ(R300_ZB_CNTL, 3);
    OUT_CS(dsa->z_buffer_control);
    OUT_CS(dsa->z_stencil_control);
    OUT_CS(dsa->stencil_ref_mask);
    OUT_CS_REG(R300_ZB_ZTOP, dsa->z_buffer_top);
    if (r300screen->caps->is_r500) {
        /* OUT_CS_REG(R500_ZB_STENCILREFMASK_BF, dsa->stencil_ref_bf); */
    }
    END_CS;
}

void r300_emit_rs_state(struct r300_context* r300, struct r300_rs_state* rs)
{
    struct r300_screen* r300screen =
        (struct r300_screen*)r300->context.screen;
    CS_LOCALS(r300);
    BEGIN_CS(14);
    OUT_CS_REG(R300_VAP_CNTL_STATUS, rs->vap_control_status);
    OUT_CS_REG_SEQ(R300_SU_POLY_OFFSET_FRONT_SCALE, 6);
    OUT_CS(rs->depth_scale_front);
    OUT_CS(rs->depth_offset_front);
    OUT_CS(rs->depth_scale_back);
    OUT_CS(rs->depth_offset_back);
    OUT_CS(rs->polygon_offset_enable);
    OUT_CS(rs->cull_mode);
    OUT_CS_REG(R300_GA_LINE_STIPPLE_CONFIG, rs->line_stipple_config);
    OUT_CS_REG(R300_GA_LINE_STIPPLE_VALUE, rs->line_stipple_value);
    END_CS;
}

static void r300_emit_dirty_state(struct r300_context* r300)
{
    struct r300_screen* r300screen =
        (struct r300_screen*)r300->context.screen;
    CS_LOCALS(r300);

    if (!(r300->dirty_state) && !(r300->dirty_hw)) {
        return;
    }

    /* XXX check size */

    if (r300->dirty_state & R300_NEW_BLEND) {
        r300_emit_blend_state(r300, r300->blend_state);
    }

    if (r300->dirty_state & R300_NEW_BLEND_COLOR) {
        r300_emit_blend_color_state(r300, r300->blend_color_state);
    }

    if (r300->dirty_state & R300_NEW_DSA) {
        r300_emit_dsa_state(r300, r300->dsa_state);
    }

    if (r300->dirty_state & R300_NEW_RASTERIZER) {
        r300_emit_rs_state(r300, r300->rs_state);
    }

    if (r300->dirty_state & R300_NEW_SCISSOR) {
        struct r300_scissor_state* scissor = r300->scissor_state;
        /* XXX next two are contiguous regs */
        OUT_CS_REG(R300_SC_SCISSORS_TL, scissor->scissor_top_left);
        OUT_CS_REG(R300_SC_SCISSORS_BR, scissor->scissor_bottom_right);
    }

    r300->dirty_state = 0;
}
