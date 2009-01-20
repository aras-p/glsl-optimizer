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

#include "r300_context.h"
#include "r300_cs.h"
#include "r300_screen.h"

static void r300_emit_dirty_state(struct r300_context* r300)
{
    struct r300_screen* r300screen = (struct r300_screen*)r300->context.screen;
    CS_LOCALS(r300);

    if (!(r300->dirty_state) && !(r300->dirty_hw)) {
        return;
    }

    /* XXX check size */

    if (r300->dirty_state & R300_NEW_BLEND) {
        struct r300_blend_state* blend = r300->blend_state;
        /* XXX next two are contiguous regs */
        OUT_CS_REG(R300_RB3D_CBLEND, blend->blend_control);
        OUT_CS_REG(R300_RB3D_ABLEND, blend->alpha_blend_control);
        OUT_CS_REG(R300_RB3D_ROPCNTL, blend->rop);
        OUT_CS_REG(R300_RB3D_DITHER_CTL, blend->dither);
    }

    if (r300->dirty_state & R300_NEW_BLEND_COLOR) {
        struct r300_blend_color_state* blend_color = r300->blend_color_state;
        if (r300screen->is_r500) {
            /* XXX next two are contiguous regs */
            OUT_CS_REG(R500_RB3D_CONSTANT_COLOR_AR,
                       blend_color->blend_color_red_alpha);
            OUT_CS_REG(R500_RB3D_CONSTANT_COLOR_GB,
                       blend_color->blend_color_green_blue);
        } else {
            OUT_CS_REG(R300_RB3D_BLEND_COLOR,
                       blend_color->blend_color);
        }
    }

    if (r300->dirty_state & R300_NEW_DSA) {
        struct r300_dsa_state* dsa = r300->dsa_state;
        OUT_CS_REG(R300_FG_ALPHA_FUNC, dsa->alpha_function);
        OUT_CS_REG(R500_FG_ALPHA_VALUE, dsa->alpha_reference);
        /* XXX next three are contiguous regs */
        OUT_CS_REG(R300_ZB_CNTL, dsa->z_buffer_control);
        OUT_CS_REG(R300_ZB_ZSTENCILCNTL, dsa->z_stencil_control);
        OUT_CS_REG(R300_ZB_STENCILREFMASK, dsa->stencil_ref_mask);
        OUT_CS_REG(R300_ZB_ZTOP, dsa->z_buffer_top);
        if (r300screen->is_r500) {
            OUT_CS_REG(R500_ZB_STENCILREFMASK_BF, dsa->stencil_ref_bf);
        }
    }

    if (r300->dirty_state & R300_NEW_RS) {
        struct r300_rs_state* rs = r300->rs_state;
        OUT_CS_REG(R300_VAP_CNTL_STATUS, rs->vap_control_status);
        /* XXX next six are contiguous regs */
        OUT_CS_REG(R300_SU_POLY_OFFSET_FRONT_SCALE, rs->depth_scale_front);
        OUT_CS_REG(R300_SU_POLY_OFFSET_FRONT_OFFSET, rs->depth_offset_front);
        OUT_CS_REG(R300_SU_POLY_OFFSET_BACK_SCALE, rs->depth_scale_back);
        OUT_CS_REG(R300_SU_POLY_OFFSET_BACK_OFFSET, rs->depth_offset_back);
        OUT_CS_REG(R300_SU_POLY_OFFSET_ENABLE, rs->polygon_offset_enable);
        OUT_CS_REG(R300_SU_CULL_MODE, rs->cull_mode);
    }

    if (r300->dirty_state & R300_NEW_SCISSOR) {
        struct r300_scissor_state* scissor = r300->scissor_state;
        /* XXX next two are contiguous regs */
        OUT_CS_REG(R300_SC_SCISSORS_TL, scissor->scissor_top_left);
        OUT_CS_REG(R300_SC_SCISSORS_BR, scissor->scissor_bottom_right);
    }

    r300->dirty_state = 0;
}
